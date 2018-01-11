//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// renderervk_utils:
//    Helper functions for the Vulkan Renderer.
//

#include "libANGLE/renderer/vulkan/renderervk_utils.h"

#include "common/aligned_memory.h"
#include "libANGLE/Context.h"
#include "libANGLE/SizedMRUCache.h"
#include "libANGLE/renderer/vulkan/CommandBufferNode.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

// FIXME: remove if split file
#include "libANGLE/renderer/vulkan/ProgramVk.h"

namespace rx
{

namespace
{
GLenum DefaultGLErrorCode(VkResult result)
{
    switch (result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_TOO_MANY_OBJECTS:
            return GL_OUT_OF_MEMORY;
        default:
            return GL_INVALID_OPERATION;
    }
}

EGLint DefaultEGLErrorCode(VkResult result)
{
    switch (result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_TOO_MANY_OBJECTS:
            return EGL_BAD_ALLOC;
        case VK_ERROR_INITIALIZATION_FAILED:
            return EGL_NOT_INITIALIZED;
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_DEVICE_LOST:
            return EGL_CONTEXT_LOST;
        default:
            return EGL_BAD_ACCESS;
    }
}

// Gets access flags that are common between source and dest layouts.
VkAccessFlags GetBasicLayoutAccessFlags(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_ACCESS_MEMORY_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_GENERAL:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return 0;
        default:
            // TODO(jmadill): Investigate other flags.
            UNREACHABLE();
            return 0;
    }
}

VkImageUsageFlags GetStagingImageUsageFlags(vk::StagingUsage usage)
{
    switch (usage)
    {
        case vk::StagingUsage::Read:
            return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        case vk::StagingUsage::Write:
            return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        case vk::StagingUsage::Both:
            return (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        default:
            UNREACHABLE();
            return 0;
    }
}

VkImageUsageFlags GetStagingBufferUsageFlags(vk::StagingUsage usage)
{
    switch (usage)
    {
        case vk::StagingUsage::Read:
            return VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        case vk::StagingUsage::Write:
            return VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        case vk::StagingUsage::Both:
            return (VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
        default:
            UNREACHABLE();
            return 0;
    }
}

VkSampleCountFlagBits ConvertSamples(GLint sampleCount)
{
    switch (sampleCount)
    {
        case 0:
        case 1:
            return VK_SAMPLE_COUNT_1_BIT;
        case 2:
            return VK_SAMPLE_COUNT_2_BIT;
        case 4:
            return VK_SAMPLE_COUNT_4_BIT;
        case 8:
            return VK_SAMPLE_COUNT_8_BIT;
        case 16:
            return VK_SAMPLE_COUNT_16_BIT;
        case 32:
            return VK_SAMPLE_COUNT_32_BIT;
        default:
            UNREACHABLE();
            return VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM;
    }
}

void UnpackAttachmentDesc(VkAttachmentDescription *desc,
                          const vk::PackedAttachmentDesc &packedDesc,
                          const vk::PackedAttachmentOpsDesc &ops)
{
    desc->flags          = static_cast<VkAttachmentDescriptionFlags>(packedDesc.flags);
    desc->format         = static_cast<VkFormat>(packedDesc.format);
    desc->samples        = ConvertSamples(packedDesc.samples);
    desc->loadOp         = static_cast<VkAttachmentLoadOp>(ops.loadOp);
    desc->storeOp        = static_cast<VkAttachmentStoreOp>(ops.storeOp);
    desc->stencilLoadOp  = static_cast<VkAttachmentLoadOp>(ops.stencilLoadOp);
    desc->stencilStoreOp = static_cast<VkAttachmentStoreOp>(ops.stencilStoreOp);
    desc->initialLayout  = static_cast<VkImageLayout>(ops.initialLayout);
    desc->finalLayout    = static_cast<VkImageLayout>(ops.finalLayout);
}

void UnpackStencilState(const vk::PackedStencilOpState &packedState, VkStencilOpState *stateOut)
{
    stateOut->failOp      = static_cast<VkStencilOp>(packedState.failOp);
    stateOut->passOp      = static_cast<VkStencilOp>(packedState.passOp);
    stateOut->depthFailOp = static_cast<VkStencilOp>(packedState.depthFailOp);
    stateOut->compareOp   = static_cast<VkCompareOp>(packedState.compareOp);
    stateOut->compareMask = packedState.compareMask;
    stateOut->writeMask   = packedState.writeMask;
    stateOut->reference   = packedState.reference;
}

void UnpackBlendAttachmentState(vk::PackedColorBlendAttachmentState &packedState,
                                VkPipelineColorBlendAttachmentState *stateOut)
{
    stateOut->blendEnable         = static_cast<VkBool32>(packedState.blendEnable);
    stateOut->srcColorBlendFactor = static_cast<VkBlendFactor>(packedState.srcColorBlendFactor);
    stateOut->dstColorBlendFactor = static_cast<VkBlendFactor>(packedState.dstColorBlendFactor);
    stateOut->colorBlendOp        = static_cast<VkBlendOp>(packedState.colorBlendOp);
    stateOut->srcAlphaBlendFactor = static_cast<VkBlendFactor>(packedState.srcAlphaBlendFactor);
    stateOut->dstAlphaBlendFactor = static_cast<VkBlendFactor>(packedState.dstAlphaBlendFactor);
    stateOut->alphaBlendOp        = static_cast<VkBlendOp>(packedState.alphaBlendOp);
    stateOut->colorWriteMask      = static_cast<VkColorComponentFlags>(packedState.colorWriteMask);
}

}  // anonymous namespace

// Mirrors std_validation_str in loader.h
// TODO(jmadill): Possibly wrap the loader into a safe source file. Can't be included trivially.
const char *g_VkStdValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
const char *g_VkLoaderLayersPathEnv    = "VK_LAYER_PATH";

const char *VulkanResultString(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "Command successfully completed.";
        case VK_NOT_READY:
            return "A fence or query has not yet completed.";
        case VK_TIMEOUT:
            return "A wait operation has not completed in the specified time.";
        case VK_EVENT_SET:
            return "An event is signaled.";
        case VK_EVENT_RESET:
            return "An event is unsignaled.";
        case VK_INCOMPLETE:
            return "A return array was too small for the result.";
        case VK_SUBOPTIMAL_KHR:
            return "A swapchain no longer matches the surface properties exactly, but can still be "
                   "used to present to the surface successfully.";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "Initialization of an object could not be completed for implementation-specific "
                   "reasons.";
        case VK_ERROR_DEVICE_LOST:
            return "The logical or physical device has been lost.";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "The requested version of Vulkan is not supported by the driver or is otherwise "
                   "incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "A requested format is not supported on this device.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "The requested window is already connected to a VkSurfaceKHR, or to some other "
                   "non-Vulkan API.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "A surface has changed in such a way that it is no longer compatible with the "
                   "swapchain.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "The display used by a swapchain does not use the same presentable image "
                   "layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "The validation layers detected invalid API usage.";
        default:
            return "Unknown vulkan error code.";
    }
}

bool HasStandardValidationLayer(const std::vector<VkLayerProperties> &layerProps)
{
    for (const auto &layerProp : layerProps)
    {
        if (std::string(layerProp.layerName) == g_VkStdValidationLayerName)
        {
            return true;
        }
    }

    return false;
}

namespace vk
{

Error::Error(VkResult result) : mResult(result), mFile(nullptr), mLine(0)
{
    ASSERT(result == VK_SUCCESS);
}

Error::Error(VkResult result, const char *file, unsigned int line)
    : mResult(result), mFile(file), mLine(line)
{
}

Error::~Error()
{
}

Error::Error(const Error &other) = default;
Error &Error::operator=(const Error &other) = default;

gl::Error Error::toGL(GLenum glErrorCode) const
{
    if (!isError())
    {
        return gl::NoError();
    }

    // TODO(jmadill): Set extended error code to 'vulkan internal error'.
    return gl::Error(glErrorCode, glErrorCode, toString());
}

egl::Error Error::toEGL(EGLint eglErrorCode) const
{
    if (!isError())
    {
        return egl::NoError();
    }

    // TODO(jmadill): Set extended error code to 'vulkan internal error'.
    return egl::Error(eglErrorCode, eglErrorCode, toString());
}

std::string Error::toString() const
{
    std::stringstream errorStream;
    errorStream << "Internal Vulkan error: " << VulkanResultString(mResult) << ", in " << mFile
                << ", line " << mLine << ".";
    return errorStream.str();
}

Error::operator gl::Error() const
{
    return toGL(DefaultGLErrorCode(mResult));
}

Error::operator egl::Error() const
{
    return toEGL(DefaultEGLErrorCode(mResult));
}

bool Error::isError() const
{
    return (mResult != VK_SUCCESS);
}

// CommandPool implementation.
CommandPool::CommandPool()
{
}

void CommandPool::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyCommandPool(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error CommandPool::init(VkDevice device, const VkCommandPoolCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateCommandPool(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// CommandBuffer implementation.
CommandBuffer::CommandBuffer()
{
}

VkCommandBuffer CommandBuffer::releaseHandle()
{
    VkCommandBuffer handle = mHandle;
    mHandle                = nullptr;
    return handle;
}

Error CommandBuffer::init(VkDevice device, const VkCommandBufferAllocateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkAllocateCommandBuffers(device, &createInfo, &mHandle));
    return NoError();
}

Error CommandBuffer::begin(const VkCommandBufferBeginInfo &info)
{
    ASSERT(valid());
    ANGLE_VK_TRY(vkBeginCommandBuffer(mHandle, &info));
    return NoError();
}

Error CommandBuffer::end()
{
    ASSERT(valid());
    ANGLE_VK_TRY(vkEndCommandBuffer(mHandle));
    return NoError();
}

Error CommandBuffer::reset()
{
    ASSERT(valid());
    ANGLE_VK_TRY(vkResetCommandBuffer(mHandle, 0));
    return NoError();
}

void CommandBuffer::singleImageBarrier(VkPipelineStageFlags srcStageMask,
                                       VkPipelineStageFlags dstStageMask,
                                       VkDependencyFlags dependencyFlags,
                                       const VkImageMemoryBarrier &imageMemoryBarrier)
{
    ASSERT(valid());
    vkCmdPipelineBarrier(mHandle, srcStageMask, dstStageMask, dependencyFlags, 0, nullptr, 0,
                         nullptr, 1, &imageMemoryBarrier);
}

void CommandBuffer::singleBufferBarrier(VkPipelineStageFlags srcStageMask,
                                        VkPipelineStageFlags dstStageMask,
                                        VkDependencyFlags dependencyFlags,
                                        const VkBufferMemoryBarrier &bufferBarrier)
{
    ASSERT(valid());
    vkCmdPipelineBarrier(mHandle, srcStageMask, dstStageMask, dependencyFlags, 0, nullptr, 1,
                         &bufferBarrier, 0, nullptr);
}

void CommandBuffer::destroy(VkDevice device, const vk::CommandPool &commandPool)
{
    if (valid())
    {
        ASSERT(commandPool.valid());
        vkFreeCommandBuffers(device, commandPool.getHandle(), 1, &mHandle);
        mHandle = VK_NULL_HANDLE;
    }
}

void CommandBuffer::copyBuffer(const vk::Buffer &srcBuffer,
                               const vk::Buffer &destBuffer,
                               uint32_t regionCount,
                               const VkBufferCopy *regions)
{
    ASSERT(valid());
    ASSERT(srcBuffer.valid() && destBuffer.valid());
    vkCmdCopyBuffer(mHandle, srcBuffer.getHandle(), destBuffer.getHandle(), regionCount, regions);
}

void CommandBuffer::clearSingleColorImage(const vk::Image &image, const VkClearColorValue &color)
{
    ASSERT(valid());
    ASSERT(image.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL ||
           image.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = 1;

    vkCmdClearColorImage(mHandle, image.getHandle(), image.getCurrentLayout(), &color, 1, &range);
}

void CommandBuffer::copySingleImage(const vk::Image &srcImage,
                                    const vk::Image &destImage,
                                    const gl::Box &copyRegion,
                                    VkImageAspectFlags aspectMask)
{
    VkImageCopy region;
    region.srcSubresource.aspectMask     = aspectMask;
    region.srcSubresource.mipLevel       = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount     = 1;
    region.srcOffset.x                   = copyRegion.x;
    region.srcOffset.y                   = copyRegion.y;
    region.srcOffset.z                   = copyRegion.z;
    region.dstSubresource.aspectMask     = aspectMask;
    region.dstSubresource.mipLevel       = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount     = 1;
    region.dstOffset.x                   = copyRegion.x;
    region.dstOffset.y                   = copyRegion.y;
    region.dstOffset.z                   = copyRegion.z;
    region.extent.width                  = copyRegion.width;
    region.extent.height                 = copyRegion.height;
    region.extent.depth                  = copyRegion.depth;

    copyImage(srcImage, destImage, 1, &region);
}

void CommandBuffer::copyImage(const vk::Image &srcImage,
                              const vk::Image &dstImage,
                              uint32_t regionCount,
                              const VkImageCopy *regions)
{
    ASSERT(valid() && srcImage.valid() && dstImage.valid());
    ASSERT(srcImage.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           srcImage.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL);
    ASSERT(dstImage.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
           dstImage.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL);
    vkCmdCopyImage(mHandle, srcImage.getHandle(), srcImage.getCurrentLayout(), dstImage.getHandle(),
                   dstImage.getCurrentLayout(), 1, regions);
}

void CommandBuffer::beginRenderPass(const VkRenderPassBeginInfo &beginInfo,
                                    VkSubpassContents subpassContents)
{
    ASSERT(valid());
    vkCmdBeginRenderPass(mHandle, &beginInfo, subpassContents);
}

void CommandBuffer::endRenderPass()
{
    ASSERT(mHandle != VK_NULL_HANDLE);
    vkCmdEndRenderPass(mHandle);
}

void CommandBuffer::draw(uint32_t vertexCount,
                         uint32_t instanceCount,
                         uint32_t firstVertex,
                         uint32_t firstInstance)
{
    ASSERT(valid());
    vkCmdDraw(mHandle, vertexCount, instanceCount, firstVertex, firstInstance);
}

void CommandBuffer::drawIndexed(uint32_t indexCount,
                                uint32_t instanceCount,
                                uint32_t firstIndex,
                                int32_t vertexOffset,
                                uint32_t firstInstance)
{
    ASSERT(valid());
    vkCmdDrawIndexed(mHandle, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void CommandBuffer::bindPipeline(VkPipelineBindPoint pipelineBindPoint,
                                 const vk::Pipeline &pipeline)
{
    ASSERT(valid() && pipeline.valid());
    vkCmdBindPipeline(mHandle, pipelineBindPoint, pipeline.getHandle());
}

void CommandBuffer::bindVertexBuffers(uint32_t firstBinding,
                                      uint32_t bindingCount,
                                      const VkBuffer *buffers,
                                      const VkDeviceSize *offsets)
{
    ASSERT(valid());
    vkCmdBindVertexBuffers(mHandle, firstBinding, bindingCount, buffers, offsets);
}

void CommandBuffer::bindIndexBuffer(const vk::Buffer &buffer,
                                    VkDeviceSize offset,
                                    VkIndexType indexType)
{
    ASSERT(valid());
    vkCmdBindIndexBuffer(mHandle, buffer.getHandle(), offset, indexType);
}

void CommandBuffer::bindDescriptorSets(VkPipelineBindPoint bindPoint,
                                       const vk::PipelineLayout &layout,
                                       uint32_t firstSet,
                                       uint32_t descriptorSetCount,
                                       const VkDescriptorSet *descriptorSets,
                                       uint32_t dynamicOffsetCount,
                                       const uint32_t *dynamicOffsets)
{
    ASSERT(valid());
    vkCmdBindDescriptorSets(mHandle, bindPoint, layout.getHandle(), firstSet, descriptorSetCount,
                            descriptorSets, dynamicOffsetCount, dynamicOffsets);
}

void CommandBuffer::executeCommands(uint32_t commandBufferCount,
                                    const vk::CommandBuffer *commandBuffers)
{
    ASSERT(valid());
    vkCmdExecuteCommands(mHandle, commandBufferCount, commandBuffers[0].ptr());
}

// Image implementation.
Image::Image() : mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

void Image::setHandle(VkImage handle)
{
    mHandle = handle;
}

void Image::reset()
{
    mHandle = VK_NULL_HANDLE;
}

void Image::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyImage(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Image::init(VkDevice device, const VkImageCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateImage(device, &createInfo, nullptr, &mHandle));
    mCurrentLayout = createInfo.initialLayout;
    return NoError();
}

void Image::changeLayoutTop(VkImageAspectFlags aspectMask,
                            VkImageLayout newLayout,
                            CommandBuffer *commandBuffer)
{
    if (newLayout == mCurrentLayout)
    {
        // No-op.
        return;
    }

    changeLayoutWithStages(aspectMask, newLayout, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, commandBuffer);
}

void Image::changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                   VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStageMask,
                                   VkPipelineStageFlags dstStageMask,
                                   CommandBuffer *commandBuffer)
{
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext               = nullptr;
    imageMemoryBarrier.srcAccessMask       = 0;
    imageMemoryBarrier.dstAccessMask       = 0;
    imageMemoryBarrier.oldLayout           = mCurrentLayout;
    imageMemoryBarrier.newLayout           = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image               = mHandle;

    // TODO(jmadill): Is this needed for mipped/layer images?
    imageMemoryBarrier.subresourceRange.aspectMask     = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
    imageMemoryBarrier.subresourceRange.levelCount     = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount     = 1;

    // TODO(jmadill): Test all the permutations of the access flags.
    imageMemoryBarrier.srcAccessMask = GetBasicLayoutAccessFlags(mCurrentLayout);

    if (mCurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        imageMemoryBarrier.srcAccessMask |= VK_ACCESS_HOST_WRITE_BIT;
    }

    imageMemoryBarrier.dstAccessMask = GetBasicLayoutAccessFlags(newLayout);

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask |=
            (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
        imageMemoryBarrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
    }

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    commandBuffer->singleImageBarrier(srcStageMask, dstStageMask, 0, imageMemoryBarrier);

    mCurrentLayout = newLayout;
}

void Image::getMemoryRequirements(VkDevice device, VkMemoryRequirements *requirementsOut) const
{
    ASSERT(valid());
    vkGetImageMemoryRequirements(device, mHandle, requirementsOut);
}

Error Image::bindMemory(VkDevice device, const vk::DeviceMemory &deviceMemory)
{
    ASSERT(valid() && deviceMemory.valid());
    ANGLE_VK_TRY(vkBindImageMemory(device, mHandle, deviceMemory.getHandle(), 0));
    return NoError();
}

// ImageView implementation.
ImageView::ImageView()
{
}

void ImageView::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyImageView(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error ImageView::init(VkDevice device, const VkImageViewCreateInfo &createInfo)
{
    ANGLE_VK_TRY(vkCreateImageView(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// Semaphore implementation.
Semaphore::Semaphore()
{
}

void Semaphore::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroySemaphore(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Semaphore::init(VkDevice device)
{
    ASSERT(!valid());

    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    ANGLE_VK_TRY(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &mHandle));

    return NoError();
}

// Framebuffer implementation.
Framebuffer::Framebuffer()
{
}

void Framebuffer::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyFramebuffer(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Framebuffer::init(VkDevice device, const VkFramebufferCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateFramebuffer(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

void Framebuffer::setHandle(VkFramebuffer handle)
{
    mHandle = handle;
}

// DeviceMemory implementation.
DeviceMemory::DeviceMemory()
{
}

void DeviceMemory::destroy(VkDevice device)
{
    if (valid())
    {
        vkFreeMemory(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error DeviceMemory::allocate(VkDevice device, const VkMemoryAllocateInfo &allocInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkAllocateMemory(device, &allocInfo, nullptr, &mHandle));
    return NoError();
}

Error DeviceMemory::map(VkDevice device,
                        VkDeviceSize offset,
                        VkDeviceSize size,
                        VkMemoryMapFlags flags,
                        uint8_t **mapPointer)
{
    ASSERT(valid());
    ANGLE_VK_TRY(
        vkMapMemory(device, mHandle, offset, size, flags, reinterpret_cast<void **>(mapPointer)));
    return NoError();
}

void DeviceMemory::unmap(VkDevice device)
{
    ASSERT(valid());
    vkUnmapMemory(device, mHandle);
}

// RenderPass implementation.
RenderPass::RenderPass()
{
}

void RenderPass::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyRenderPass(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error RenderPass::init(VkDevice device, const VkRenderPassCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateRenderPass(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// StagingImage implementation.
StagingImage::StagingImage() : mSize(0)
{
}

StagingImage::StagingImage(StagingImage &&other)
    : mImage(std::move(other.mImage)),
      mDeviceMemory(std::move(other.mDeviceMemory)),
      mSize(other.mSize)
{
    other.mSize = 0;
}

void StagingImage::destroy(VkDevice device)
{
    mImage.destroy(device);
    mDeviceMemory.destroy(device);
}

Error StagingImage::init(VkDevice device,
                         uint32_t queueFamilyIndex,
                         const vk::MemoryProperties &memoryProperties,
                         TextureDimension dimension,
                         VkFormat format,
                         const gl::Extents &extent,
                         StagingUsage usage)
{
    VkImageCreateInfo createInfo;

    createInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext         = nullptr;
    createInfo.flags         = 0;
    createInfo.imageType     = VK_IMAGE_TYPE_2D;
    createInfo.format        = format;
    createInfo.extent.width  = static_cast<uint32_t>(extent.width);
    createInfo.extent.height = static_cast<uint32_t>(extent.height);
    createInfo.extent.depth  = static_cast<uint32_t>(extent.depth);
    createInfo.mipLevels     = 1;
    createInfo.arrayLayers   = 1;
    createInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling        = VK_IMAGE_TILING_LINEAR;
    createInfo.usage                 = GetStagingImageUsageFlags(usage);
    createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices   = &queueFamilyIndex;

    // Use Preinitialized for writable staging images - in these cases we want to map the memory
    // before we do a copy. For readback images, use an undefined layout.
    createInfo.initialLayout = usage == vk::StagingUsage::Read ? VK_IMAGE_LAYOUT_UNDEFINED
                                                               : VK_IMAGE_LAYOUT_PREINITIALIZED;

    ANGLE_TRY(mImage.init(device, createInfo));

    VkMemoryRequirements memoryRequirements;
    mImage.getMemoryRequirements(device, &memoryRequirements);

    // Find the right kind of memory index.
    uint32_t memoryIndex = memoryProperties.findCompatibleMemoryIndex(
        memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

    VkMemoryAllocateInfo allocateInfo;
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext           = nullptr;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = memoryIndex;

    ANGLE_TRY(mDeviceMemory.allocate(device, allocateInfo));
    ANGLE_TRY(mImage.bindMemory(device, mDeviceMemory));

    mSize = memoryRequirements.size;

    return NoError();
}

void StagingImage::dumpResources(Serial serial, std::vector<vk::GarbageObject> *garbageQueue)
{
    mImage.dumpResources(serial, garbageQueue);
    mDeviceMemory.dumpResources(serial, garbageQueue);
}

// Buffer implementation.
Buffer::Buffer()
{
}

void Buffer::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyBuffer(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Buffer::init(VkDevice device, const VkBufferCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateBuffer(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

Error Buffer::bindMemory(VkDevice device, const DeviceMemory &deviceMemory)
{
    ASSERT(valid() && deviceMemory.valid());
    ANGLE_VK_TRY(vkBindBufferMemory(device, mHandle, deviceMemory.getHandle(), 0));
    return NoError();
}

// ShaderModule implementation.
ShaderModule::ShaderModule()
{
}

void ShaderModule::destroy(VkDevice device)
{
    if (mHandle != VK_NULL_HANDLE)
    {
        vkDestroyShaderModule(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error ShaderModule::init(VkDevice device, const VkShaderModuleCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateShaderModule(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// Pipeline implementation.
Pipeline::Pipeline()
{
}

void Pipeline::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyPipeline(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Pipeline::initGraphics(VkDevice device, const VkGraphicsPipelineCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &createInfo, nullptr, &mHandle));
    return NoError();
}

// PipelineLayout implementation.
PipelineLayout::PipelineLayout()
{
}

void PipelineLayout::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyPipelineLayout(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error PipelineLayout::init(VkDevice device, const VkPipelineLayoutCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreatePipelineLayout(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// DescriptorSetLayout implementation.
DescriptorSetLayout::DescriptorSetLayout()
{
}

void DescriptorSetLayout::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyDescriptorSetLayout(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error DescriptorSetLayout::init(VkDevice device, const VkDescriptorSetLayoutCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// DescriptorPool implementation.
DescriptorPool::DescriptorPool()
{
}

void DescriptorPool::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyDescriptorPool(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error DescriptorPool::init(VkDevice device, const VkDescriptorPoolCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateDescriptorPool(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

Error DescriptorPool::allocateDescriptorSets(VkDevice device,
                                             const VkDescriptorSetAllocateInfo &allocInfo,
                                             VkDescriptorSet *descriptorSetsOut)
{
    ASSERT(valid());
    ANGLE_VK_TRY(vkAllocateDescriptorSets(device, &allocInfo, descriptorSetsOut));
    return NoError();
}

// Sampler implementation.
Sampler::Sampler()
{
}

void Sampler::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroySampler(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Sampler::init(VkDevice device, const VkSamplerCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateSampler(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

// Fence implementation.
Fence::Fence()
{
}

void Fence::destroy(VkDevice device)
{
    if (valid())
    {
        vkDestroyFence(device, mHandle, nullptr);
        mHandle = VK_NULL_HANDLE;
    }
}

Error Fence::init(VkDevice device, const VkFenceCreateInfo &createInfo)
{
    ASSERT(!valid());
    ANGLE_VK_TRY(vkCreateFence(device, &createInfo, nullptr, &mHandle));
    return NoError();
}

VkResult Fence::getStatus(VkDevice device) const
{
    return vkGetFenceStatus(device, mHandle);
}

// MemoryProperties implementation.
MemoryProperties::MemoryProperties() : mMemoryProperties{0}
{
}

void MemoryProperties::init(VkPhysicalDevice physicalDevice)
{
    ASSERT(mMemoryProperties.memoryTypeCount == 0);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mMemoryProperties);
    ASSERT(mMemoryProperties.memoryTypeCount > 0);
}

uint32_t MemoryProperties::findCompatibleMemoryIndex(uint32_t bitMask, uint32_t propertyFlags) const
{
    ASSERT(mMemoryProperties.memoryTypeCount > 0);

    // TODO(jmadill): Cache compatible memory indexes after finding them once.
    for (size_t memoryIndex : angle::BitSet32<32>(bitMask))
    {
        ASSERT(memoryIndex < mMemoryProperties.memoryTypeCount);

        if ((mMemoryProperties.memoryTypes[memoryIndex].propertyFlags & propertyFlags) ==
            propertyFlags)
        {
            return static_cast<uint32_t>(memoryIndex);
        }
    }

    UNREACHABLE();
    return std::numeric_limits<uint32_t>::max();
}

// StagingBuffer implementation.
StagingBuffer::StagingBuffer() : mSize(0)
{
}

void StagingBuffer::destroy(VkDevice device)
{
    mBuffer.destroy(device);
    mDeviceMemory.destroy(device);
    mSize = 0;
}

vk::Error StagingBuffer::init(ContextVk *contextVk, VkDeviceSize size, StagingUsage usage)
{
    VkBufferCreateInfo createInfo;
    createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.flags                 = 0;
    createInfo.size                  = size;
    createInfo.usage                 = GetStagingBufferUsageFlags(usage);
    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;

    ANGLE_TRY(mBuffer.init(contextVk->getDevice(), createInfo));
    ANGLE_TRY(AllocateBufferMemory(contextVk, static_cast<size_t>(size), &mBuffer, &mDeviceMemory,
                                   &mSize));

    return vk::NoError();
}

void StagingBuffer::dumpResources(Serial serial, std::vector<vk::GarbageObject> *garbageQueue)
{
    mBuffer.dumpResources(serial, garbageQueue);
    mDeviceMemory.dumpResources(serial, garbageQueue);
}

Optional<uint32_t> FindMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProps,
                                  const VkMemoryRequirements &requirements,
                                  uint32_t propertyFlagMask)
{
    for (uint32_t typeIndex = 0; typeIndex < memoryProps.memoryTypeCount; ++typeIndex)
    {
        if ((requirements.memoryTypeBits & (1u << typeIndex)) != 0 &&
            ((memoryProps.memoryTypes[typeIndex].propertyFlags & propertyFlagMask) ==
             propertyFlagMask))
        {
            return typeIndex;
        }
    }

    return Optional<uint32_t>::Invalid();
}

Error AllocateBufferMemory(ContextVk *contextVk,
                           size_t size,
                           Buffer *buffer,
                           DeviceMemory *deviceMemoryOut,
                           size_t *requiredSizeOut)
{
    VkDevice device = contextVk->getDevice();

    // Find a compatible memory pool index. If the index doesn't change, we could cache it.
    // Not finding a valid memory pool means an out-of-spec driver, or internal error.
    // TODO(jmadill): More efficient memory allocation.
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer->getHandle(), &memoryRequirements);

    // The requirements size is not always equal to the specified API size.
    ASSERT(memoryRequirements.size >= size);
    *requiredSizeOut = static_cast<size_t>(memoryRequirements.size);

    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(contextVk->getRenderer()->getPhysicalDevice(),
                                        &memoryProperties);

    auto memoryTypeIndex =
        FindMemoryType(memoryProperties, memoryRequirements,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ANGLE_VK_CHECK(memoryTypeIndex.valid(), VK_ERROR_INCOMPATIBLE_DRIVER);

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext           = nullptr;
    allocInfo.memoryTypeIndex = memoryTypeIndex.value();
    allocInfo.allocationSize  = memoryRequirements.size;

    ANGLE_TRY(deviceMemoryOut->allocate(device, allocInfo));
    ANGLE_TRY(buffer->bindMemory(device, *deviceMemoryOut));

    return NoError();
}

// GarbageObject implementation.
GarbageObject::GarbageObject()
    : mSerial(), mHandleType(HandleType::Invalid), mHandle(VK_NULL_HANDLE)
{
}

GarbageObject::GarbageObject(const GarbageObject &other) = default;

GarbageObject &GarbageObject::operator=(const GarbageObject &other) = default;

bool GarbageObject::destroyIfComplete(VkDevice device, Serial completedSerial)
{
    if (completedSerial >= mSerial)
    {
        destroy(device);
        return true;
    }

    return false;
}

void GarbageObject::destroy(VkDevice device)
{
    switch (mHandleType)
    {
        case HandleType::Semaphore:
            vkDestroySemaphore(device, reinterpret_cast<VkSemaphore>(mHandle), nullptr);
            break;
        case HandleType::CommandBuffer:
            // Command buffers are pool allocated.
            UNREACHABLE();
            break;
        case HandleType::Fence:
            vkDestroyFence(device, reinterpret_cast<VkFence>(mHandle), nullptr);
            break;
        case HandleType::DeviceMemory:
            vkFreeMemory(device, reinterpret_cast<VkDeviceMemory>(mHandle), nullptr);
            break;
        case HandleType::Buffer:
            vkDestroyBuffer(device, reinterpret_cast<VkBuffer>(mHandle), nullptr);
            break;
        case HandleType::Image:
            vkDestroyImage(device, reinterpret_cast<VkImage>(mHandle), nullptr);
            break;
        case HandleType::ImageView:
            vkDestroyImageView(device, reinterpret_cast<VkImageView>(mHandle), nullptr);
            break;
        case HandleType::ShaderModule:
            vkDestroyShaderModule(device, reinterpret_cast<VkShaderModule>(mHandle), nullptr);
            break;
        case HandleType::PipelineLayout:
            vkDestroyPipelineLayout(device, reinterpret_cast<VkPipelineLayout>(mHandle), nullptr);
            break;
        case HandleType::RenderPass:
            vkDestroyRenderPass(device, reinterpret_cast<VkRenderPass>(mHandle), nullptr);
            break;
        case HandleType::Pipeline:
            vkDestroyPipeline(device, reinterpret_cast<VkPipeline>(mHandle), nullptr);
            break;
        case HandleType::DescriptorSetLayout:
            vkDestroyDescriptorSetLayout(device, reinterpret_cast<VkDescriptorSetLayout>(mHandle),
                                         nullptr);
            break;
        case HandleType::Sampler:
            vkDestroySampler(device, reinterpret_cast<VkSampler>(mHandle), nullptr);
            break;
        case HandleType::DescriptorPool:
            vkDestroyDescriptorPool(device, reinterpret_cast<VkDescriptorPool>(mHandle), nullptr);
            break;
        case HandleType::Framebuffer:
            vkDestroyFramebuffer(device, reinterpret_cast<VkFramebuffer>(mHandle), nullptr);
            break;
        case HandleType::CommandPool:
            vkDestroyCommandPool(device, reinterpret_cast<VkCommandPool>(mHandle), nullptr);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

// RenderPassDesc implementation.
RenderPassDesc::RenderPassDesc()
{
    UNUSED_VARIABLE(mPadding);
    memset(this, 0, sizeof(RenderPassDesc));
}

RenderPassDesc::~RenderPassDesc()
{
}

RenderPassDesc::RenderPassDesc(const RenderPassDesc &other)
{
    memcpy(this, &other, sizeof(RenderPassDesc));
}

void RenderPassDesc::packAttachment(uint32_t index, const vk::Format &format, GLsizei samples)
{
    PackedAttachmentDesc &desc = mAttachmentDescs[index];

    // TODO(jmadill): We would only need this flag for duplicated attachments.
    desc.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    ASSERT(desc.samples < std::numeric_limits<uint8_t>::max());
    desc.samples = static_cast<uint8_t>(samples);
    ASSERT(format.vkTextureFormat < std::numeric_limits<uint16_t>::max());
    desc.format = static_cast<uint16_t>(format.vkTextureFormat);
}

void RenderPassDesc::packColorAttachment(const vk::Format &format, GLsizei samples)
{
    ASSERT(mDepthStencilAttachmentCount == 0);
    ASSERT(mColorAttachmentCount < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS);
    packAttachment(mColorAttachmentCount++, format, samples);
}

void RenderPassDesc::packDepthStencilAttachment(const vk::Format &format, GLsizei samples)
{
    ASSERT(mDepthStencilAttachmentCount == 0);
    packAttachment(mDepthStencilAttachmentCount++, format, samples);
}

RenderPassDesc &RenderPassDesc::operator=(const RenderPassDesc &other)
{
    memcpy(this, &other, sizeof(RenderPassDesc));
    return *this;
}

size_t RenderPassDesc::hash() const
{
    return angle::ComputeGenericHash(*this);
}

uint32_t RenderPassDesc::attachmentCount() const
{
    return (mColorAttachmentCount + mDepthStencilAttachmentCount);
}

uint32_t RenderPassDesc::colorAttachmentCount() const
{
    return mColorAttachmentCount;
}

uint32_t RenderPassDesc::depthStencilAttachmentCount() const
{
    return mDepthStencilAttachmentCount;
}

const PackedAttachmentDesc &RenderPassDesc::operator[](size_t index) const
{
    ASSERT(index < mAttachmentDescs.size());
    return mAttachmentDescs[index];
}

bool operator==(const RenderPassDesc &lhs, const RenderPassDesc &rhs)
{
    return (memcmp(&lhs, &rhs, sizeof(RenderPassDesc)) == 0);
}

// AttachmentOpsArray implementation.
AttachmentOpsArray::AttachmentOpsArray()
{
    memset(&mOps, 0, sizeof(PackedAttachmentOpsDesc) * mOps.size());
}

AttachmentOpsArray::~AttachmentOpsArray()
{
}

AttachmentOpsArray::AttachmentOpsArray(const AttachmentOpsArray &other)
{
    memcpy(&mOps, &other.mOps, sizeof(PackedAttachmentOpsDesc) * mOps.size());
}

AttachmentOpsArray &AttachmentOpsArray::operator=(const AttachmentOpsArray &other)
{
    memcpy(&mOps, &other.mOps, sizeof(PackedAttachmentOpsDesc) * mOps.size());
    return *this;
}

const PackedAttachmentOpsDesc &AttachmentOpsArray::operator[](size_t index) const
{
    return mOps[index];
}

PackedAttachmentOpsDesc &AttachmentOpsArray::operator[](size_t index)
{
    return mOps[index];
}

void AttachmentOpsArray::initDummyOp(size_t index, VkImageLayout finalLayout)
{
    PackedAttachmentOpsDesc &ops = mOps[index];

    ops.loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ops.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ops.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ops.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ops.initialLayout  = static_cast<uint16_t>(VK_IMAGE_LAYOUT_UNDEFINED);
    ops.finalLayout    = static_cast<uint16_t>(finalLayout);
}

size_t AttachmentOpsArray::hash() const
{
    return angle::ComputeGenericHash(mOps);
}

bool operator==(const AttachmentOpsArray &lhs, const AttachmentOpsArray &rhs)
{
    return (memcmp(&lhs, &rhs, sizeof(AttachmentOpsArray)) == 0);
}

Error InitializeRenderPassFromDesc(VkDevice device,
                                   const RenderPassDesc &desc,
                                   const AttachmentOpsArray &ops,
                                   RenderPass *renderPass)
{
    uint32_t attachmentCount = desc.attachmentCount();
    ASSERT(attachmentCount > 0);

    gl::DrawBuffersArray<VkAttachmentReference> colorAttachmentRefs;

    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        VkAttachmentReference &colorRef = colorAttachmentRefs[colorIndex];
        colorRef.attachment             = colorIndex;
        colorRef.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthStencilAttachmentRef;
    if (desc.depthStencilAttachmentCount() > 0)
    {
        ASSERT(desc.depthStencilAttachmentCount() == 1);
        depthStencilAttachmentRef.attachment = desc.colorAttachmentCount();
        depthStencilAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDesc;

    subpassDesc.flags                = 0;
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments    = nullptr;
    subpassDesc.colorAttachmentCount = desc.colorAttachmentCount();
    subpassDesc.pColorAttachments    = colorAttachmentRefs.data();
    subpassDesc.pResolveAttachments  = nullptr;
    subpassDesc.pDepthStencilAttachment =
        (desc.depthStencilAttachmentCount() > 0 ? &depthStencilAttachmentRef : nullptr);
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments    = nullptr;

    // Unpack the packed and split representation into the format required by Vulkan.
    gl::AttachmentArray<VkAttachmentDescription> attachmentDescs;
    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        UnpackAttachmentDesc(&attachmentDescs[colorIndex], desc[colorIndex], ops[colorIndex]);
    }

    if (desc.depthStencilAttachmentCount() > 0)
    {
        uint32_t depthStencilIndex = desc.colorAttachmentCount();
        UnpackAttachmentDesc(&attachmentDescs[depthStencilIndex], desc[depthStencilIndex],
                             ops[depthStencilIndex]);
    }

    VkRenderPassCreateInfo createInfo;
    createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext           = nullptr;
    createInfo.flags           = 0;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments    = attachmentDescs.data();
    createInfo.subpassCount    = 1;
    createInfo.pSubpasses      = &subpassDesc;
    createInfo.dependencyCount = 0;
    createInfo.pDependencies   = nullptr;

    ANGLE_TRY(renderPass->init(device, createInfo));
    return vk::NoError();
}

// PipelineDesc implementation.
// Use aligned allocation and free so we can use the alignas keyword.
void *PipelineDesc::operator new(std::size_t size)
{
    return angle::AlignedAlloc(size, 32);
}

void PipelineDesc::operator delete(void *ptr)
{
    return angle::AlignedFree(ptr);
}

PipelineDesc::PipelineDesc()
{
    memset(this, 0, sizeof(PipelineDesc));
}

PipelineDesc::~PipelineDesc()
{
}

PipelineDesc::PipelineDesc(const PipelineDesc &other)
{
    memcpy(this, &other, sizeof(PipelineDesc));
}

PipelineDesc &PipelineDesc::operator=(const PipelineDesc &other)
{
    memcpy(this, &other, sizeof(PipelineDesc));
    return *this;
}

size_t PipelineDesc::hash() const
{
    return angle::ComputeGenericHash(*this);
}

bool PipelineDesc::operator==(const PipelineDesc &other) const
{
    return (memcmp(this, &other, sizeof(PipelineDesc)) == 0);
}

void PipelineDesc::initDefaults()
{
    mInputAssemblyInfo.topology = static_cast<uint32_t>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    mInputAssemblyInfo.primitiveRestartEnable = 0;

    mRasterizationStateInfo.depthClampEnable           = 0;
    mRasterizationStateInfo.rasterizationDiscardEnable = 0;
    mRasterizationStateInfo.polygonMode     = static_cast<uint16_t>(VK_POLYGON_MODE_FILL);
    mRasterizationStateInfo.cullMode        = static_cast<uint16_t>(VK_CULL_MODE_NONE);
    mRasterizationStateInfo.frontFace       = static_cast<uint16_t>(VK_FRONT_FACE_CLOCKWISE);
    mRasterizationStateInfo.depthBiasEnable = 0;
    mRasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    mRasterizationStateInfo.depthBiasClamp          = 0.0f;
    mRasterizationStateInfo.depthBiasSlopeFactor    = 0.0f;
    mRasterizationStateInfo.lineWidth               = 1.0f;

    mMultisampleStateInfo.rasterizationSamples = 1;
    mMultisampleStateInfo.sampleShadingEnable  = 0;
    mMultisampleStateInfo.minSampleShading     = 0.0f;
    for (int maskIndex = 0; maskIndex < gl::MAX_SAMPLE_MASK_WORDS; ++maskIndex)
    {
        mMultisampleStateInfo.sampleMask[maskIndex] = 0;
    }
    mMultisampleStateInfo.alphaToCoverageEnable = 0;
    mMultisampleStateInfo.alphaToOneEnable      = 0;

    mDepthStencilStateInfo.depthTestEnable       = 0;
    mDepthStencilStateInfo.depthWriteEnable      = 1;
    mDepthStencilStateInfo.depthCompareOp        = static_cast<uint8_t>(VK_COMPARE_OP_LESS);
    mDepthStencilStateInfo.depthBoundsTestEnable = 0;
    mDepthStencilStateInfo.stencilTestEnable     = 0;
    mDepthStencilStateInfo.minDepthBounds        = 0.0f;
    mDepthStencilStateInfo.maxDepthBounds        = 0.0f;
    mDepthStencilStateInfo.front.failOp          = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.passOp          = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.depthFailOp     = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.compareOp       = static_cast<uint8_t>(VK_COMPARE_OP_ALWAYS);
    mDepthStencilStateInfo.front.compareMask     = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.front.writeMask       = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.front.reference       = 0;
    mDepthStencilStateInfo.back.failOp           = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.passOp           = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.depthFailOp      = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.compareOp        = static_cast<uint8_t>(VK_COMPARE_OP_ALWAYS);
    mDepthStencilStateInfo.back.compareMask      = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.back.writeMask        = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.back.reference        = 0;

    // TODO(jmadill): Blend state/MRT.
    PackedColorBlendAttachmentState blendAttachmentState;
    blendAttachmentState.blendEnable         = 0;
    blendAttachmentState.srcColorBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.dstColorBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.colorBlendOp        = static_cast<uint8_t>(VK_BLEND_OP_ADD);
    blendAttachmentState.srcAlphaBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.dstAlphaBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.alphaBlendOp        = static_cast<uint8_t>(VK_BLEND_OP_ADD);
    blendAttachmentState.colorWriteMask =
        static_cast<uint8_t>(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    mColorBlendStateInfo.logicOpEnable     = 0;
    mColorBlendStateInfo.logicOp           = static_cast<uint32_t>(VK_LOGIC_OP_CLEAR);
    mColorBlendStateInfo.attachmentCount   = 1;
    mColorBlendStateInfo.blendConstants[0] = 0.0f;
    mColorBlendStateInfo.blendConstants[1] = 0.0f;
    mColorBlendStateInfo.blendConstants[2] = 0.0f;
    mColorBlendStateInfo.blendConstants[3] = 0.0f;

    std::fill(&mColorBlendStateInfo.attachments[0],
              &mColorBlendStateInfo.attachments[gl::IMPLEMENTATION_MAX_DRAW_BUFFERS],
              blendAttachmentState);
}

Error PipelineDesc::initializePipeline(RendererVk *renderer,
                                       ProgramVk *programVk,
                                       Pipeline *pipelineOut)
{
    VkPipelineShaderStageCreateInfo shaderStages[2];
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    std::array<VkPipelineColorBlendAttachmentState, gl::IMPLEMENTATION_MAX_DRAW_BUFFERS>
        blendAttachmentState;
    VkPipelineColorBlendStateCreateInfo blendState;
    VkGraphicsPipelineCreateInfo createInfo;

    ASSERT(programVk->getVertexModuleSerial() == mShaderStageInfo[0].moduleSerial);
    shaderStages[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].pNext               = nullptr;
    shaderStages[0].flags               = 0;
    shaderStages[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module              = programVk->getLinkedVertexModule().getHandle();
    shaderStages[0].pName               = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    ASSERT(programVk->getFragmentModuleSerial() == mShaderStageInfo[1].moduleSerial);
    shaderStages[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].pNext               = nullptr;
    shaderStages[1].flags               = 0;
    shaderStages[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module              = programVk->getLinkedFragmentModule().getHandle();
    shaderStages[1].pName               = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // TODO(jmadill): Possibly use different path for ES 3.1 split bindings/attribs.
    gl::AttribArray<VkVertexInputBindingDescription> bindingDescs;
    gl::AttribArray<VkVertexInputAttributeDescription> attributeDescs;

    uint32_t vertexAttribCount = 0;

    for (uint32_t attribIndex = 0; attribIndex < gl::MAX_VERTEX_ATTRIBS; ++attribIndex)
    {
        VkVertexInputBindingDescription &bindingDesc       = bindingDescs[attribIndex];
        VkVertexInputAttributeDescription &attribDesc      = attributeDescs[attribIndex];
        const PackedVertexInputBindingDesc &packedBinding  = mVertexInputBindings[attribIndex];
        const PackedVertexInputAttributeDesc &packedAttrib = mVertexInputAttribs[attribIndex];

        // TODO(jmadill): Support for gaps in vertex attribute specification.
        if (packedAttrib.format == 0)
            continue;

        vertexAttribCount = attribIndex + 1;

        bindingDesc.binding   = attribIndex;
        bindingDesc.inputRate = static_cast<VkVertexInputRate>(packedBinding.inputRate);
        bindingDesc.stride    = static_cast<uint32_t>(packedBinding.stride);

        attribDesc.binding  = attribIndex;
        attribDesc.format   = static_cast<VkFormat>(packedAttrib.format);
        attribDesc.location = static_cast<uint32_t>(packedAttrib.location);
        attribDesc.offset   = packedAttrib.offset;
    }

    // The binding descriptions are filled in at draw time.
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = nullptr;
    vertexInputState.flags = 0;
    vertexInputState.vertexBindingDescriptionCount   = vertexAttribCount;
    vertexInputState.pVertexBindingDescriptions      = bindingDescs.data();
    vertexInputState.vertexAttributeDescriptionCount = vertexAttribCount;
    vertexInputState.pVertexAttributeDescriptions    = attributeDescs.data();

    // Primitive topology is filled in at draw time.
    inputAssemblyState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext    = nullptr;
    inputAssemblyState.flags    = 0;
    inputAssemblyState.topology = static_cast<VkPrimitiveTopology>(mInputAssemblyInfo.topology);
    inputAssemblyState.primitiveRestartEnable =
        static_cast<VkBool32>(mInputAssemblyInfo.primitiveRestartEnable);

    // Set initial viewport and scissor state.

    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext         = nullptr;
    viewportState.flags         = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &mViewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &mScissor;

    // Rasterizer state.
    rasterState.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterState.pNext            = nullptr;
    rasterState.flags            = 0;
    rasterState.depthClampEnable = static_cast<VkBool32>(mRasterizationStateInfo.depthClampEnable);
    rasterState.rasterizerDiscardEnable =
        static_cast<VkBool32>(mRasterizationStateInfo.rasterizationDiscardEnable);
    rasterState.polygonMode     = static_cast<VkPolygonMode>(mRasterizationStateInfo.polygonMode);
    rasterState.cullMode        = static_cast<VkCullModeFlags>(mRasterizationStateInfo.cullMode);
    rasterState.frontFace       = static_cast<VkFrontFace>(mRasterizationStateInfo.frontFace);
    rasterState.depthBiasEnable = static_cast<VkBool32>(mRasterizationStateInfo.depthBiasEnable);
    rasterState.depthBiasConstantFactor = mRasterizationStateInfo.depthBiasConstantFactor;
    rasterState.depthBiasClamp          = mRasterizationStateInfo.depthBiasClamp;
    rasterState.depthBiasSlopeFactor    = mRasterizationStateInfo.depthBiasSlopeFactor;
    rasterState.lineWidth               = mRasterizationStateInfo.lineWidth;

    // Multisample state.
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.pNext = nullptr;
    multisampleState.flags = 0;
    multisampleState.rasterizationSamples =
        ConvertSamples(mMultisampleStateInfo.rasterizationSamples);
    multisampleState.sampleShadingEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.sampleShadingEnable);
    multisampleState.minSampleShading = mMultisampleStateInfo.minSampleShading;
    // TODO(jmadill): sample masks
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.alphaToCoverageEnable);
    multisampleState.alphaToOneEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.alphaToOneEnable);

    // Depth/stencil state.
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.pNext = nullptr;
    depthStencilState.flags = 0;
    depthStencilState.depthTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthTestEnable);
    depthStencilState.depthWriteEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthWriteEnable);
    depthStencilState.depthCompareOp =
        static_cast<VkCompareOp>(mDepthStencilStateInfo.depthCompareOp);
    depthStencilState.depthBoundsTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthBoundsTestEnable);
    depthStencilState.stencilTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.stencilTestEnable);
    UnpackStencilState(mDepthStencilStateInfo.front, &depthStencilState.front);
    UnpackStencilState(mDepthStencilStateInfo.back, &depthStencilState.back);
    depthStencilState.minDepthBounds = mDepthStencilStateInfo.minDepthBounds;
    depthStencilState.maxDepthBounds = mDepthStencilStateInfo.maxDepthBounds;

    blendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.pNext           = 0;
    blendState.flags           = 0;
    blendState.logicOpEnable   = static_cast<VkBool32>(mColorBlendStateInfo.logicOpEnable);
    blendState.logicOp         = static_cast<VkLogicOp>(mColorBlendStateInfo.logicOp);
    blendState.attachmentCount = mColorBlendStateInfo.attachmentCount;
    blendState.pAttachments    = blendAttachmentState.data();

    for (int i = 0; i < 4; i++)
    {
        blendState.blendConstants[i] = mColorBlendStateInfo.blendConstants[i];
    }

    for (uint32_t colorIndex = 0; colorIndex < blendState.attachmentCount; ++colorIndex)
    {
        UnpackBlendAttachmentState(mColorBlendStateInfo.attachments[colorIndex],
                                   &blendAttachmentState[colorIndex]);
    }

    // TODO(jmadill): Dynamic state.

    // Pull in a compatible RenderPass.
    RenderPass *compatibleRenderPass = nullptr;

    ANGLE_TRY(renderer->getCompatibleRenderPass(mRenderPassDesc, &compatibleRenderPass));

    createInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stageCount          = 2;
    createInfo.pStages             = shaderStages;
    createInfo.pVertexInputState   = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pTessellationState  = nullptr;
    createInfo.pViewportState      = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState   = &multisampleState;
    createInfo.pDepthStencilState  = &depthStencilState;
    createInfo.pColorBlendState    = &blendState;
    createInfo.pDynamicState       = nullptr;
    createInfo.layout              = renderer->getGraphicsPipelineLayout().getHandle();
    createInfo.renderPass          = compatibleRenderPass->getHandle();
    createInfo.subpass             = 0;
    createInfo.basePipelineHandle  = VK_NULL_HANDLE;
    createInfo.basePipelineIndex   = 0;

    ANGLE_TRY(pipelineOut->initGraphics(renderer->getDevice(), createInfo));

    return NoError();
}

void PipelineDesc::updateShaders(ProgramVk *programVk)
{
    ASSERT(programVk->getVertexModuleSerial() < std::numeric_limits<uint32_t>::max());
    mShaderStageInfo[0].moduleSerial =
        static_cast<uint32_t>(programVk->getVertexModuleSerial().getValue());
    ASSERT(programVk->getFragmentModuleSerial() < std::numeric_limits<uint32_t>::max());
    mShaderStageInfo[1].moduleSerial =
        static_cast<uint32_t>(programVk->getFragmentModuleSerial().getValue());
}

void PipelineDesc::updateViewport(const gl::Rectangle &viewport, float nearPlane, float farPlane)
{
    mViewport.x        = static_cast<float>(viewport.x);
    mViewport.y        = static_cast<float>(viewport.y);
    mViewport.width    = static_cast<float>(viewport.width);
    mViewport.height   = static_cast<float>(viewport.height);
    mViewport.minDepth = nearPlane;
    mViewport.maxDepth = farPlane;

    // TODO(jmadill): Scissor.
    mScissor.offset.x      = viewport.x;
    mScissor.offset.y      = viewport.y;
    mScissor.extent.width  = viewport.width;
    mScissor.extent.height = viewport.height;
}

void PipelineDesc::resetVertexInputState()
{
    memset(&mVertexInputBindings, 0, sizeof(VertexInputBindings));
    memset(&mVertexInputAttribs, 0, sizeof(VertexInputAttributes));
}

void PipelineDesc::updateVertexInputInfo(uint32_t attribIndex,
                                         const gl::VertexBinding &binding,
                                         const gl::VertexAttribute &attrib)
{
    PackedVertexInputBindingDesc &bindingDesc = mVertexInputBindings[attribIndex];

    size_t attribSize = gl::ComputeVertexAttributeTypeSize(attrib);
    ASSERT(attribSize <= std::numeric_limits<uint16_t>::max());

    bindingDesc.stride    = static_cast<uint16_t>(attribSize);
    bindingDesc.inputRate = static_cast<uint16_t>(
        binding.getDivisor() > 0 ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX);

    gl::VertexFormatType vertexFormatType = gl::GetVertexFormatType(attrib);
    VkFormat vkFormat                     = vk::GetNativeVertexFormat(vertexFormatType);
    ASSERT(vkFormat <= std::numeric_limits<uint16_t>::max());

    PackedVertexInputAttributeDesc &attribDesc = mVertexInputAttribs[attribIndex];
    attribDesc.format                          = static_cast<uint16_t>(vkFormat);
    attribDesc.location                        = static_cast<uint16_t>(attribIndex);
    attribDesc.offset = static_cast<uint32_t>(ComputeVertexAttributeOffset(attrib, binding));
}

void PipelineDesc::updateTopology(GLenum drawMode)
{
    mInputAssemblyInfo.topology = static_cast<uint32_t>(gl_vk::GetPrimitiveTopology(drawMode));
}

void PipelineDesc::updateCullMode(const gl::RasterizerState &rasterState)
{
    mRasterizationStateInfo.cullMode = static_cast<uint16_t>(gl_vk::GetCullMode(rasterState));
}

void PipelineDesc::updateFrontFace(const gl::RasterizerState &rasterState)
{
    mRasterizationStateInfo.frontFace =
        static_cast<uint16_t>(gl_vk::GetFrontFace(rasterState.frontFace));
}

void PipelineDesc::updateLineWidth(float lineWidth)
{
    mRasterizationStateInfo.lineWidth = lineWidth;
}

void PipelineDesc::updateRenderPassDesc(const RenderPassDesc &renderPassDesc)
{
    mRenderPassDesc = renderPassDesc;
}

}  // namespace vk

namespace gl_vk
{
VkPrimitiveTopology GetPrimitiveTopology(GLenum mode)
{
    switch (mode)
    {
        case GL_TRIANGLES:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        case GL_POINTS:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        case GL_LINES:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case GL_LINE_STRIP:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case GL_TRIANGLE_FAN:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
        case GL_TRIANGLE_STRIP:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case GL_LINE_LOOP:
            // TODO(jmadill): Implement line loop support.
            UNIMPLEMENTED();
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        default:
            UNREACHABLE();
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
    }
}

VkCullModeFlags GetCullMode(const gl::RasterizerState &rasterState)
{
    if (!rasterState.cullFace)
    {
        return VK_CULL_MODE_NONE;
    }

    switch (rasterState.cullMode)
    {
        case gl::CullFaceMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case gl::CullFaceMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        case gl::CullFaceMode::FrontAndBack:
            return VK_CULL_MODE_FRONT_AND_BACK;
        default:
            UNREACHABLE();
            return VK_CULL_MODE_NONE;
    }
}

VkFrontFace GetFrontFace(GLenum frontFace)
{
    switch (frontFace)
    {
        case GL_CW:
            return VK_FRONT_FACE_CLOCKWISE;
        case GL_CCW:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        default:
            UNREACHABLE();
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }
}

}  // namespace gl_vk

ResourceVk::ResourceVk() : mCurrentWriteNode(nullptr)
{
}

ResourceVk::~ResourceVk()
{
}

void ResourceVk::updateQueueSerial(Serial queueSerial)
{
    ASSERT(queueSerial >= mStoredQueueSerial);

    if (queueSerial > mStoredQueueSerial)
    {
        mCurrentWriteNode = nullptr;
        mCurrentReadNodes.clear();
        mStoredQueueSerial = queueSerial;
    }
}

Serial ResourceVk::getQueueSerial() const
{
    return mStoredQueueSerial;
}

bool ResourceVk::isCurrentlyRecording(Serial currentSerial) const
{
    return (mStoredQueueSerial == currentSerial && mCurrentWriteNode != nullptr);
}

vk::CommandBufferNode *ResourceVk::getCurrentWriteNode(Serial currentSerial)
{
    ASSERT(currentSerial == mStoredQueueSerial);
    return mCurrentWriteNode;
}

vk::CommandBufferNode *ResourceVk::getNewWriteNode(RendererVk *renderer)
{
    vk::CommandBufferNode *newCommands = renderer->allocateCommandNode();
    setWriteNode(renderer->getCurrentQueueSerial(), newCommands);
    return newCommands;
}

void ResourceVk::setWriteNode(Serial serial, vk::CommandBufferNode *newCommands)
{
    updateQueueSerial(serial);

    // Make sure any open reads and writes finish before we execute |newCommands|.
    if (!mCurrentReadNodes.empty())
    {
        newCommands->addDependencies(mCurrentReadNodes);
        mCurrentReadNodes.clear();
    }

    if (mCurrentWriteNode)
    {
        newCommands->addDependency(mCurrentWriteNode);
    }

    mCurrentWriteNode = newCommands;
}

vk::Error ResourceVk::recordWriteCommands(RendererVk *renderer,
                                          vk::CommandBuffer **commandBufferOut)
{
    vk::CommandBufferNode *commands = getNewWriteNode(renderer);

    VkDevice device = renderer->getDevice();
    ANGLE_TRY(commands->startRecording(device, renderer->getCommandPool(), commandBufferOut));
    return vk::NoError();
}

void ResourceVk::updateDependencies(vk::CommandBufferNode *readNode, Serial serial)
{
    if (isCurrentlyRecording(serial))
    {
        // Link the current write node to "readNode".
        readNode->addDependency(getCurrentWriteNode(serial));
        ASSERT(mStoredQueueSerial == serial);
    }
    else
    {
        updateQueueSerial(serial);
    }

    // Track "readNode" in this resource.
    mCurrentReadNodes.push_back(readNode);
}

}  // namespace rx

std::ostream &operator<<(std::ostream &stream, const rx::vk::Error &error)
{
    stream << error.toString();
    return stream;
}

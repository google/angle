//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_utils:
//    Helper functions for the Vulkan Renderer.
//

#include "libANGLE/renderer/vulkan/vk_utils.h"

#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{

namespace
{

constexpr int kLineLoopStreamingBufferMinSize = 1024 * 1024;

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

// Mirrors std_validation_str in loader.c
const char *g_VkStdValidationLayerName = "VK_LAYER_LUNARG_standard_validation";
const char *g_VkValidationLayerNames[] = {
    "VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation",
    "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
    "VK_LAYER_GOOGLE_unique_objects"};
const uint32_t g_VkNumValidationLayerNames =
    sizeof(g_VkValidationLayerNames) / sizeof(g_VkValidationLayerNames[0]);

bool HasValidationLayer(const std::vector<VkLayerProperties> &layerProps, const char *layerName)
{
    for (const auto &layerProp : layerProps)
    {
        if (std::string(layerProp.layerName) == layerName)
        {
            return true;
        }
    }

    return false;
}

bool HasStandardValidationLayer(const std::vector<VkLayerProperties> &layerProps)
{
    return HasValidationLayer(layerProps, g_VkStdValidationLayerName);
}

bool HasValidationLayers(const std::vector<VkLayerProperties> &layerProps)
{
    for (const auto &layerName : g_VkValidationLayerNames)
    {
        if (!HasValidationLayer(layerProps, layerName))
        {
            return false;
        }
    }

    return true;
}

vk::Error FindAndAllocateCompatibleMemory(VkDevice device,
                                          const vk::MemoryProperties &memoryProperties,
                                          VkMemoryPropertyFlags memoryPropertyFlags,
                                          const VkMemoryRequirements &memoryRequirements,
                                          vk::DeviceMemory *deviceMemoryOut)
{
    uint32_t memoryTypeIndex = 0;
    ANGLE_TRY(memoryProperties.findCompatibleMemoryIndex(memoryRequirements, memoryPropertyFlags,
                                                         &memoryTypeIndex));

    VkMemoryAllocateInfo allocInfo;
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext           = nullptr;
    allocInfo.memoryTypeIndex = memoryTypeIndex;
    allocInfo.allocationSize  = memoryRequirements.size;

    ANGLE_TRY(deviceMemoryOut->allocate(device, allocInfo));
    return vk::NoError();
}

template <typename T>
vk::Error AllocateBufferOrImageMemory(RendererVk *renderer,
                                      VkMemoryPropertyFlags memoryPropertyFlags,
                                      T *bufferOrImage,
                                      vk::DeviceMemory *deviceMemoryOut,
                                      size_t *requiredSizeOut)
{
    VkDevice device                              = renderer->getDevice();
    const vk::MemoryProperties &memoryProperties = renderer->getMemoryProperties();

    // Call driver to determine memory requirements.
    VkMemoryRequirements memoryRequirements;
    bufferOrImage->getMemoryRequirements(device, &memoryRequirements);

    // The requirements size is not always equal to the specified API size.
    *requiredSizeOut = static_cast<size_t>(memoryRequirements.size);

    ANGLE_TRY(FindAndAllocateCompatibleMemory(device, memoryProperties, memoryPropertyFlags,
                                              memoryRequirements, deviceMemoryOut));
    ANGLE_TRY(bufferOrImage->bindMemory(device, *deviceMemoryOut));

    return vk::NoError();
}
}  // anonymous namespace

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

bool GetAvailableValidationLayers(const std::vector<VkLayerProperties> &layerProps,
                                  bool mustHaveLayers,
                                  const char *const **enabledLayerNames,
                                  uint32_t *enabledLayerCount)
{
    if (HasStandardValidationLayer(layerProps))
    {
        *enabledLayerNames = &g_VkStdValidationLayerName;
        *enabledLayerCount = 1;
    }
    else if (HasValidationLayers(layerProps))
    {
        *enabledLayerNames = g_VkValidationLayerNames;
        *enabledLayerCount = g_VkNumValidationLayerNames;
    }
    else
    {
        // Generate an error if the layers were explicitly requested, warning otherwise.
        if (mustHaveLayers)
        {
            ERR() << "Vulkan validation layers are missing.";
        }
        else
        {
            WARN() << "Vulkan validation layers are missing.";
        }

        *enabledLayerNames = nullptr;
        *enabledLayerCount = 0;
        return false;
    }

    return true;
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

void CommandBuffer::copyBuffer(const VkBuffer &srcBuffer,
                               const VkBuffer &destBuffer,
                               uint32_t regionCount,
                               const VkBufferCopy *regions)
{
    ASSERT(valid());
    vkCmdCopyBuffer(mHandle, srcBuffer, destBuffer, regionCount, regions);
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

void CommandBuffer::clearSingleDepthStencilImage(const vk::Image &image,
                                                 VkImageAspectFlags aspectFlags,
                                                 const VkClearDepthStencilValue &depthStencil)
{
    VkImageSubresourceRange clearRange = {
        /*aspectMask*/ aspectFlags,
        /*baseMipLevel*/ 0,
        /*levelCount*/ 1,
        /*baseArrayLayer*/ 0,
        /*layerCount*/ 1,
    };

    clearDepthStencilImage(image, depthStencil, 1, &clearRange);
}

void CommandBuffer::clearDepthStencilImage(const vk::Image &image,
                                           const VkClearDepthStencilValue &depthStencil,
                                           uint32_t rangeCount,
                                           const VkImageSubresourceRange *ranges)
{
    ASSERT(valid());
    ASSERT(image.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL ||
           image.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    vkCmdClearDepthStencilImage(mHandle, image.getHandle(), image.getCurrentLayout(), &depthStencil,
                                rangeCount, ranges);
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

void CommandBuffer::bindIndexBuffer(const VkBuffer &buffer,
                                    VkDeviceSize offset,
                                    VkIndexType indexType)
{
    ASSERT(valid());
    vkCmdBindIndexBuffer(mHandle, buffer, offset, indexType);
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

Error StagingImage::init(ContextVk *contextVk,
                         TextureDimension dimension,
                         const Format &format,
                         const gl::Extents &extent,
                         StagingUsage usage)
{
    VkDevice device           = contextVk->getDevice();
    RendererVk *renderer      = contextVk->getRenderer();
    uint32_t queueFamilyIndex = renderer->getQueueFamilyIndex();

    VkImageCreateInfo createInfo;

    createInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.pNext                 = nullptr;
    createInfo.flags                 = 0;
    createInfo.imageType             = VK_IMAGE_TYPE_2D;
    createInfo.format                = format.vkTextureFormat;
    createInfo.extent.width          = static_cast<uint32_t>(extent.width);
    createInfo.extent.height         = static_cast<uint32_t>(extent.height);
    createInfo.extent.depth          = static_cast<uint32_t>(extent.depth);
    createInfo.mipLevels             = 1;
    createInfo.arrayLayers           = 1;
    createInfo.samples               = VK_SAMPLE_COUNT_1_BIT;
    createInfo.tiling                = VK_IMAGE_TILING_LINEAR;
    createInfo.usage                 = GetStagingImageUsageFlags(usage);
    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices   = &queueFamilyIndex;

    // Use Preinitialized for writable staging images - in these cases we want to map the memory
    // before we do a copy. For readback images, use an undefined layout.
    createInfo.initialLayout = usage == vk::StagingUsage::Read ? VK_IMAGE_LAYOUT_UNDEFINED
                                                               : VK_IMAGE_LAYOUT_PREINITIALIZED;

    ANGLE_TRY(mImage.init(device, createInfo));

    // Allocate and bind host visible and coherent Image memory.
    // TODO(ynovikov): better approach would be to request just visible memory,
    // and call vkInvalidateMappedMemoryRanges if the allocated memory is not coherent.
    // This would solve potential issues of:
    // 1) not having (enough) coherent memory and 2) coherent memory being slower
    VkMemoryPropertyFlags memoryPropertyFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ANGLE_TRY(AllocateImageMemory(renderer, memoryPropertyFlags, &mImage, &mDeviceMemory, &mSize));

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

void Buffer::getMemoryRequirements(VkDevice device, VkMemoryRequirements *memoryRequirementsOut)
{
    ASSERT(valid());
    vkGetBufferMemoryRequirements(device, mHandle, memoryRequirementsOut);
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

Error MemoryProperties::findCompatibleMemoryIndex(const VkMemoryRequirements &memoryRequirements,
                                                  VkMemoryPropertyFlags memoryPropertyFlags,
                                                  uint32_t *typeIndexOut) const
{
    ASSERT(mMemoryProperties.memoryTypeCount > 0 && mMemoryProperties.memoryTypeCount <= 32);

    // Find a compatible memory pool index. If the index doesn't change, we could cache it.
    // Not finding a valid memory pool means an out-of-spec driver, or internal error.
    // TODO(jmadill): Determine if it is possible to cache indexes.
    // TODO(jmadill): More efficient memory allocation.
    for (size_t memoryIndex : angle::BitSet32<32>(memoryRequirements.memoryTypeBits))
    {
        ASSERT(memoryIndex < mMemoryProperties.memoryTypeCount);

        if ((mMemoryProperties.memoryTypes[memoryIndex].propertyFlags & memoryPropertyFlags) ==
            memoryPropertyFlags)
        {
            *typeIndexOut = static_cast<uint32_t>(memoryIndex);
            return NoError();
        }
    }

    // TODO(jmadill): Add error message to error.
    return vk::Error(VK_ERROR_INCOMPATIBLE_DRIVER);
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

    VkMemoryPropertyFlags flags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    ANGLE_TRY(mBuffer.init(contextVk->getDevice(), createInfo));
    ANGLE_TRY(
        AllocateBufferMemory(contextVk->getRenderer(), flags, &mBuffer, &mDeviceMemory, &mSize));

    return vk::NoError();
}

void StagingBuffer::dumpResources(Serial serial, std::vector<vk::GarbageObject> *garbageQueue)
{
    mBuffer.dumpResources(serial, garbageQueue);
    mDeviceMemory.dumpResources(serial, garbageQueue);
}

Error AllocateBufferMemory(RendererVk *renderer,
                           VkMemoryPropertyFlags memoryPropertyFlags,
                           Buffer *buffer,
                           DeviceMemory *deviceMemoryOut,
                           size_t *requiredSizeOut)
{
    return AllocateBufferOrImageMemory(renderer, memoryPropertyFlags, buffer, deviceMemoryOut,
                                       requiredSizeOut);
}

Error AllocateImageMemory(RendererVk *renderer,
                          VkMemoryPropertyFlags memoryPropertyFlags,
                          Image *image,
                          DeviceMemory *deviceMemoryOut,
                          size_t *requiredSizeOut)
{
    return AllocateBufferOrImageMemory(renderer, memoryPropertyFlags, image, deviceMemoryOut,
                                       requiredSizeOut);
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

LineLoopHandler::LineLoopHandler()
    : mObserverBinding(this, 0u),
      mStreamingLineLoopIndicesData(
          new StreamingBuffer(VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                              kLineLoopStreamingBufferMinSize)),
      mLineLoopIndexBuffer(VK_NULL_HANDLE),
      mLineLoopIndexBufferOffset(VK_NULL_HANDLE)
{
}

LineLoopHandler::~LineLoopHandler() = default;

void LineLoopHandler::bindIndexBuffer(VkIndexType indexType, vk::CommandBuffer **commandBuffer)
{
    (*commandBuffer)->bindIndexBuffer(mLineLoopIndexBuffer, mLineLoopIndexBufferOffset, indexType);
}

gl::Error LineLoopHandler::createIndexBuffer(ContextVk *contextVk, int firstVertex, int count)
{
    int lastVertex = firstVertex + count;
    if (mLineLoopIndexBuffer == VK_NULL_HANDLE || !mLineLoopBufferFirstIndex.valid() ||
        !mLineLoopBufferLastIndex.valid() || mLineLoopBufferFirstIndex != firstVertex ||
        mLineLoopBufferLastIndex != lastVertex)
    {
        uint32_t *indices = nullptr;

        ANGLE_TRY(mStreamingLineLoopIndicesData->allocate(
            contextVk, sizeof(uint32_t) * (count + 1), reinterpret_cast<uint8_t **>(&indices),
            &mLineLoopIndexBuffer, &mLineLoopIndexBufferOffset));

        auto unsignedFirstVertex = static_cast<uint32_t>(firstVertex);
        for (auto vertexIndex = unsignedFirstVertex; vertexIndex < (count + unsignedFirstVertex);
             vertexIndex++)
        {
            *indices++ = vertexIndex;
        }
        *indices = unsignedFirstVertex;

        // Since we are not using the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag when creating the
        // device memory in the StreamingBuffer, we always need to make sure we flush it after
        // writing.
        ANGLE_TRY(mStreamingLineLoopIndicesData->flush(contextVk));

        mLineLoopBufferFirstIndex = firstVertex;
        mLineLoopBufferLastIndex  = lastVertex;
    }

    return gl::NoError();
}

gl::Error LineLoopHandler::createIndexBufferFromElementArrayBuffer(ContextVk *contextVk,
                                                                   BufferVk *bufferVk,
                                                                   VkIndexType indexType,
                                                                   int count)
{
    ASSERT(indexType == VK_INDEX_TYPE_UINT16 || indexType == VK_INDEX_TYPE_UINT32);

    if (bufferVk == mObserverBinding.getSubject() && mLineLoopIndexBuffer != VK_NULL_HANDLE)
    {
        return gl::NoError();
    }

    // We want to know if the bufferVk changes at any point in time, because if it does we need to
    // recopy our data on the next call.
    mObserverBinding.bind(bufferVk);

    uint32_t *indices = nullptr;

    auto unitSize = (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
    ANGLE_TRY(mStreamingLineLoopIndicesData->allocate(
        contextVk, unitSize * (count + 1), reinterpret_cast<uint8_t **>(&indices),
        &mLineLoopIndexBuffer, &mLineLoopIndexBufferOffset));

    VkBufferCopy copy1 = {0, mLineLoopIndexBufferOffset,
                          static_cast<VkDeviceSize>(count) * unitSize};
    VkBufferCopy copy2 = {
        0, mLineLoopIndexBufferOffset + static_cast<VkDeviceSize>(count) * unitSize, unitSize};
    std::array<VkBufferCopy, 2> copies = {{copy1, copy2}};

    vk::CommandBuffer *commandBuffer;
    mStreamingLineLoopIndicesData->beginWriteResource(contextVk->getRenderer(), &commandBuffer);

    Serial currentSerial = contextVk->getRenderer()->getCurrentQueueSerial();
    bufferVk->onReadResource(mStreamingLineLoopIndicesData->getCurrentWritingNode(currentSerial),
                             currentSerial);
    commandBuffer->copyBuffer(bufferVk->getVkBuffer().getHandle(), mLineLoopIndexBuffer, 2,
                              copies.data());

    ANGLE_TRY(mStreamingLineLoopIndicesData->flush(contextVk));
    return gl::NoError();
}

void LineLoopHandler::destroy(VkDevice device)
{
    mObserverBinding.reset();
    mStreamingLineLoopIndicesData->destroy(device);
}

gl::Error LineLoopHandler::draw(int count, CommandBuffer *commandBuffer)
{
    // Our first index is always 0 because that's how we set it up in the
    // bindLineLoopIndexBuffer.
    commandBuffer->drawIndexed(count + 1, 1, 0, 0, 0);

    return gl::NoError();
}

ResourceVk *LineLoopHandler::getLineLoopBufferResource()
{
    return mStreamingLineLoopIndicesData.get();
}

void LineLoopHandler::onSubjectStateChange(const gl::Context *context,
                                           angle::SubjectIndex index,
                                           angle::SubjectMessage message)
{
    // Indicate we want to recopy on next draw since something changed in the buffer.
    if (message == angle::SubjectMessage::STATE_CHANGE)
    {
        mLineLoopIndexBuffer = VK_NULL_HANDLE;
    }
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
    // Invert CW and CCW to have the same behavior as OpenGL.
    switch (frontFace)
    {
        case GL_CW:
            return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        case GL_CCW:
            return VK_FRONT_FACE_CLOCKWISE;
        default:
            UNREACHABLE();
            return VK_FRONT_FACE_CLOCKWISE;
    }
}

}  // namespace gl_vk

ResourceVk::ResourceVk() : mCurrentWritingNode(nullptr)
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
        mCurrentWritingNode = nullptr;
        mCurrentReadingNodes.clear();
        mStoredQueueSerial = queueSerial;
    }
}

Serial ResourceVk::getQueueSerial() const
{
    return mStoredQueueSerial;
}

bool ResourceVk::hasCurrentWritingNode(Serial currentSerial) const
{
    return (mStoredQueueSerial == currentSerial && mCurrentWritingNode != nullptr &&
            !mCurrentWritingNode->hasChildren());
}

vk::CommandGraphNode *ResourceVk::getCurrentWritingNode(Serial currentSerial)
{
    ASSERT(currentSerial == mStoredQueueSerial);
    return mCurrentWritingNode;
}

vk::CommandGraphNode *ResourceVk::getNewWritingNode(RendererVk *renderer)
{
    vk::CommandGraphNode *newCommands = renderer->allocateCommandNode();
    onWriteResource(newCommands, renderer->getCurrentQueueSerial());
    return newCommands;
}

vk::Error ResourceVk::beginWriteResource(RendererVk *renderer, vk::CommandBuffer **commandBufferOut)
{
    vk::CommandGraphNode *commands = getNewWritingNode(renderer);

    VkDevice device = renderer->getDevice();
    ANGLE_TRY(commands->beginOutsideRenderPassRecording(device, renderer->getCommandPool(),
                                                        commandBufferOut));
    return vk::NoError();
}

void ResourceVk::onWriteResource(vk::CommandGraphNode *writingNode, Serial serial)
{
    updateQueueSerial(serial);

    // Make sure any open reads and writes finish before we execute 'newCommands'.
    if (!mCurrentReadingNodes.empty())
    {
        vk::CommandGraphNode::SetHappensBeforeDependencies(mCurrentReadingNodes, writingNode);
        mCurrentReadingNodes.clear();
    }

    if (mCurrentWritingNode && mCurrentWritingNode != writingNode)
    {
        vk::CommandGraphNode::SetHappensBeforeDependency(mCurrentWritingNode, writingNode);
    }

    mCurrentWritingNode = writingNode;
}

void ResourceVk::onReadResource(vk::CommandGraphNode *readingNode, Serial serial)
{
    if (hasCurrentWritingNode(serial))
    {
        // Ensure 'readOperation' happens after the current write commands.
        vk::CommandGraphNode::SetHappensBeforeDependency(getCurrentWritingNode(serial),
                                                         readingNode);
        ASSERT(mStoredQueueSerial == serial);
    }
    else
    {
        updateQueueSerial(serial);
    }

    // Add the read operation to the list of nodes currently reading this resource.
    mCurrentReadingNodes.push_back(readingNode);
}

}  // namespace rx

std::ostream &operator<<(std::ostream &stream, const rx::vk::Error &error)
{
    stream << error.toString();
    return stream;
}

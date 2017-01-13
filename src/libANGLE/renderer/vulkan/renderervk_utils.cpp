//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// renderervk_utils:
//    Helper functions for the Vulkan Renderer.
//

#include "renderervk_utils.h"

#include "common/debug.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

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
            return 0;
        default:
            // TODO(jmadill): Investigate other flags.
            UNREACHABLE();
            return 0;
    }
}
}  // anonymous namespace

// Mirrors std_validation_str in loader.h
// TODO(jmadill): Possibly wrap the loader into a safe source file. Can't be included trivially.
const char *g_VkStdValidationLayerName = "VK_LAYER_LUNARG_standard_validation";

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
    const std::string &message = getExtendedMessage();
    return gl::Error(glErrorCode, message.c_str());
}

egl::Error Error::toEGL(EGLint eglErrorCode) const
{
    if (!isError())
    {
        return egl::Error(EGL_SUCCESS);
    }

    // TODO(jmadill): Set extended error code to 'vulkan internal error'.
    const std::string &message = getExtendedMessage();
    return egl::Error(eglErrorCode, message.c_str());
}

std::string Error::getExtendedMessage() const
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

// CommandBuffer implementation.
CommandBuffer::CommandBuffer(VkDevice device, VkCommandPool commandPool)
    : WrappedObject(device), mCommandPool(commandPool)
{
}

Error CommandBuffer::begin()
{
    if (mHandle == VK_NULL_HANDLE)
    {
        ASSERT(validDevice());
        VkCommandBufferAllocateInfo commandBufferInfo;
        commandBufferInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        commandBufferInfo.pNext              = nullptr;
        commandBufferInfo.commandPool        = mCommandPool;
        commandBufferInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        commandBufferInfo.commandBufferCount = 1;

        ANGLE_VK_TRY(vkAllocateCommandBuffers(mDevice, &commandBufferInfo, &mHandle));
    }
    else
    {
        reset();
    }

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext = nullptr;
    // TODO(jmadill): Use other flags?
    beginInfo.flags            = 0;
    beginInfo.pInheritanceInfo = nullptr;

    ANGLE_VK_TRY(vkBeginCommandBuffer(mHandle, &beginInfo));

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

CommandBuffer::~CommandBuffer()
{
    if (mHandle)
    {
        ASSERT(validDevice());
        vkFreeCommandBuffers(mDevice, mCommandPool, 1, &mHandle);
    }
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
    ASSERT(valid());
    ASSERT(srcImage.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL ||
           srcImage.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL);
    ASSERT(destImage.getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL ||
           destImage.getCurrentLayout() == VK_IMAGE_LAYOUT_GENERAL);

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

    vkCmdCopyImage(mHandle, srcImage.getHandle(), srcImage.getCurrentLayout(),
                   destImage.getHandle(), destImage.getCurrentLayout(), 1, &region);
}

// Image implementation.
Image::Image() : mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

Image::Image(VkDevice device) : WrappedObject(device), mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
}

Image::Image(VkImage image) : mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED)
{
    mHandle = image;
}

Image::Image(Image &&other) : WrappedObject(std::move(other)), mCurrentLayout(other.mCurrentLayout)
{
    other.mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
}

Image &Image::operator=(Image &&other)
{
    assignOpBase(std::move(other));
    std::swap(mCurrentLayout, other.mCurrentLayout);
    return *this;
}

Image::~Image()
{
    // If the device handle is null, we aren't managing the handle.
    if (valid())
    {
        vkDestroyImage(mDevice, mHandle, nullptr);
    }
}

Error Image::init(const VkImageCreateInfo &createInfo)
{
    ASSERT(mHandle == VK_NULL_HANDLE && validDevice());
    ANGLE_VK_TRY(vkCreateImage(mDevice, &createInfo, nullptr, &mHandle));
    return NoError();
}

void Image::changeLayoutTop(VkImageAspectFlags aspectMask,
                            VkImageLayout newLayout,
                            CommandBuffer *commandBuffer)
{
    changeLayoutWithStages(aspectMask, newLayout, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                           VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, commandBuffer);
}

void Image::changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                   VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStageMask,
                                   VkPipelineStageFlags dstStageMask,
                                   CommandBuffer *commandBuffer)
{
    if (newLayout == mCurrentLayout)
    {
        // No-op.
        return;
    }

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

void Image::getMemoryRequirements(VkMemoryRequirements *requirementsOut) const
{
    ASSERT(valid());
    vkGetImageMemoryRequirements(mDevice, mHandle, requirementsOut);
}

Error Image::bindMemory(const vk::DeviceMemory &deviceMemory)
{
    ASSERT(valid() && deviceMemory.valid());
    ANGLE_VK_TRY(vkBindImageMemory(mDevice, mHandle, deviceMemory.getHandle(), 0));
    return NoError();
}

// ImageView implementation.
ImageView::ImageView()
{
}

ImageView::ImageView(VkDevice device) : WrappedObject(device)
{
}

ImageView::ImageView(ImageView &&other) : WrappedObject(std::move(other))
{
}

ImageView &ImageView::operator=(ImageView &&other)
{
    assignOpBase(std::move(other));
    return *this;
}

ImageView::~ImageView()
{
    if (mHandle != VK_NULL_HANDLE)
    {
        ASSERT(validDevice());
        vkDestroyImageView(mDevice, mHandle, nullptr);
    }
}

Error ImageView::init(const VkImageViewCreateInfo &createInfo)
{
    ASSERT(validDevice());
    ANGLE_VK_TRY(vkCreateImageView(mDevice, &createInfo, nullptr, &mHandle));
    return NoError();
}

// Semaphore implementation.
Semaphore::Semaphore()
{
}

Semaphore::Semaphore(VkDevice device) : WrappedObject(device)
{
}

Semaphore::Semaphore(Semaphore &&other) : WrappedObject(std::move(other))
{
}

Semaphore::~Semaphore()
{
    if (mHandle != VK_NULL_HANDLE)
    {
        ASSERT(validDevice());
        vkDestroySemaphore(mDevice, mHandle, nullptr);
    }
}

Semaphore &Semaphore::operator=(Semaphore &&other)
{
    assignOpBase(std::move(other));
    return *this;
}

Error Semaphore::init()
{
    ASSERT(validDevice() && !valid());

    VkSemaphoreCreateInfo semaphoreInfo;
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphoreInfo.pNext = nullptr;
    semaphoreInfo.flags = 0;

    ANGLE_VK_TRY(vkCreateSemaphore(mDevice, &semaphoreInfo, nullptr, &mHandle));

    return NoError();
}

// Framebuffer implementation.
Framebuffer::Framebuffer()
{
}

Framebuffer::Framebuffer(VkDevice device) : WrappedObject(device)
{
}

Framebuffer::Framebuffer(Framebuffer &&other) : WrappedObject(std::move(other))
{
}

Framebuffer::~Framebuffer()
{
    if (mHandle != VK_NULL_HANDLE)
    {
        ASSERT(validDevice());
        vkDestroyFramebuffer(mDevice, mHandle, nullptr);
    }
}

Framebuffer &Framebuffer::operator=(Framebuffer &&other)
{
    assignOpBase(std::move(other));
    return *this;
}

Error Framebuffer::init(const VkFramebufferCreateInfo &createInfo)
{
    ASSERT(validDevice() && !valid());
    ANGLE_VK_TRY(vkCreateFramebuffer(mDevice, &createInfo, nullptr, &mHandle));
    return NoError();
}

// DeviceMemory implementation.
DeviceMemory::DeviceMemory()
{
}

DeviceMemory::DeviceMemory(VkDevice device) : WrappedObject(device)
{
}

DeviceMemory::DeviceMemory(DeviceMemory &&other) : WrappedObject(std::move(other))
{
}

DeviceMemory::~DeviceMemory()
{
    if (mHandle != VK_NULL_HANDLE)
    {
        ASSERT(validDevice());
        vkFreeMemory(mDevice, mHandle, nullptr);
    }
}

DeviceMemory &DeviceMemory::operator=(DeviceMemory &&other)
{
    assignOpBase(std::move(other));
    return *this;
}

Error DeviceMemory::allocate(const VkMemoryAllocateInfo &allocInfo)
{
    ASSERT(validDevice() && !valid());
    ANGLE_VK_TRY(vkAllocateMemory(mDevice, &allocInfo, nullptr, &mHandle));
    return NoError();
}

Error DeviceMemory::map(VkDeviceSize offset,
                        VkDeviceSize size,
                        VkMemoryMapFlags flags,
                        uint8_t **mapPointer)
{
    ASSERT(valid());
    ANGLE_VK_TRY(
        vkMapMemory(mDevice, mHandle, offset, size, flags, reinterpret_cast<void **>(mapPointer)));
    return NoError();
}

void DeviceMemory::unmap()
{
    ASSERT(valid());
    vkUnmapMemory(mDevice, mHandle);
}

// StagingImage implementation.
StagingImage::StagingImage() : mSize(0)
{
}

StagingImage::StagingImage(VkDevice device) : mImage(device), mDeviceMemory(device), mSize(0)
{
}

StagingImage::StagingImage(StagingImage &&other)
    : mImage(std::move(other.mImage)),
      mDeviceMemory(std::move(other.mDeviceMemory)),
      mSize(other.mSize)
{
    other.mSize = 0;
}

StagingImage::~StagingImage()
{
}

StagingImage &StagingImage::operator=(StagingImage &&other)
{
    std::swap(mImage, other.mImage);
    std::swap(mDeviceMemory, other.mDeviceMemory);
    std::swap(mSize, other.mSize);
    return *this;
}

Error StagingImage::init(uint32_t queueFamilyIndex,
                         uint32_t hostVisibleMemoryIndex,
                         TextureDimension dimension,
                         VkFormat format,
                         const gl::Extents &extent)
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
    createInfo.usage         = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
    createInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 1;
    createInfo.pQueueFamilyIndices   = &queueFamilyIndex;
    createInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    ANGLE_TRY(mImage.init(createInfo));

    VkMemoryRequirements memoryRequirements;
    mImage.getMemoryRequirements(&memoryRequirements);

    // Ensure we can read this memory.
    ANGLE_VK_CHECK((memoryRequirements.memoryTypeBits & (1 << hostVisibleMemoryIndex)) != 0,
                   VK_ERROR_VALIDATION_FAILED_EXT);

    VkMemoryAllocateInfo allocateInfo;
    allocateInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocateInfo.pNext           = nullptr;
    allocateInfo.allocationSize  = memoryRequirements.size;
    allocateInfo.memoryTypeIndex = hostVisibleMemoryIndex;

    ANGLE_TRY(mDeviceMemory.allocate(allocateInfo));
    ANGLE_TRY(mImage.bindMemory(mDeviceMemory));

    mSize = memoryRequirements.size;

    return NoError();
}

}  // namespace vk

}  // namespace rx

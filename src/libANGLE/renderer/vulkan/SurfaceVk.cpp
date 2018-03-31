//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SurfaceVk.cpp:
//    Implements the class methods for SurfaceVk.
//

#include "libANGLE/renderer/vulkan/SurfaceVk.h"

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{

namespace
{

VkPresentModeKHR GetDesiredPresentMode(const std::vector<VkPresentModeKHR> &presentModes,
                                       EGLint minSwapInterval,
                                       EGLint maxSwapInterval)
{
    ASSERT(!presentModes.empty());

    // Use FIFO mode for v-sync, since it throttles you to the display rate. Mailbox is more
    // similar to triple-buffering. For now we hard-code Mailbox for perf testing.
    // TODO(jmadill): Properly select present mode and re-create display if changed.
    VkPresentModeKHR bestChoice = VK_PRESENT_MODE_MAILBOX_KHR;

    for (VkPresentModeKHR presentMode : presentModes)
    {
        if (presentMode == bestChoice)
        {
            return bestChoice;
        }
    }

    WARN() << "Present mode " << bestChoice << " not available. Falling back to "
           << presentModes[0];
    return presentModes[0];
}

}  // namespace

OffscreenSurfaceVk::OffscreenSurfaceVk(const egl::SurfaceState &surfaceState,
                                       EGLint width,
                                       EGLint height)
    : SurfaceImpl(surfaceState), mWidth(width), mHeight(height)
{
}

OffscreenSurfaceVk::~OffscreenSurfaceVk()
{
}

egl::Error OffscreenSurfaceVk::initialize(const egl::Display *display)
{
    return egl::NoError();
}

FramebufferImpl *OffscreenSurfaceVk::createDefaultFramebuffer(const gl::FramebufferState &state)
{
    // Use a user FBO for an offscreen RT.
    return FramebufferVk::CreateUserFBO(state);
}

egl::Error OffscreenSurfaceVk::swap(const gl::Context *context)
{
    return egl::NoError();
}

egl::Error OffscreenSurfaceVk::postSubBuffer(const gl::Context * /*context*/,
                                             EGLint /*x*/,
                                             EGLint /*y*/,
                                             EGLint /*width*/,
                                             EGLint /*height*/)
{
    return egl::NoError();
}

egl::Error OffscreenSurfaceVk::querySurfacePointerANGLE(EGLint /*attribute*/, void ** /*value*/)
{
    UNREACHABLE();
    return egl::EglBadCurrentSurface();
}

egl::Error OffscreenSurfaceVk::bindTexImage(gl::Texture * /*texture*/, EGLint /*buffer*/)
{
    return egl::NoError();
}

egl::Error OffscreenSurfaceVk::releaseTexImage(EGLint /*buffer*/)
{
    return egl::NoError();
}

egl::Error OffscreenSurfaceVk::getSyncValues(EGLuint64KHR * /*ust*/,
                                             EGLuint64KHR * /*msc*/,
                                             EGLuint64KHR * /*sbc*/)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

void OffscreenSurfaceVk::setSwapInterval(EGLint /*interval*/)
{
}

EGLint OffscreenSurfaceVk::getWidth() const
{
    return mWidth;
}

EGLint OffscreenSurfaceVk::getHeight() const
{
    return mHeight;
}

EGLint OffscreenSurfaceVk::isPostSubBufferSupported() const
{
    return EGL_FALSE;
}

EGLint OffscreenSurfaceVk::getSwapBehavior() const
{
    return EGL_BUFFER_PRESERVED;
}

gl::Error OffscreenSurfaceVk::getAttachmentRenderTarget(
    const gl::Context * /*context*/,
    GLenum /*binding*/,
    const gl::ImageIndex & /*imageIndex*/,
    FramebufferAttachmentRenderTarget ** /*rtOut*/)
{
    UNREACHABLE();
    return gl::InternalError();
}

gl::Error OffscreenSurfaceVk::initializeContents(const gl::Context *context,
                                                 const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return gl::NoError();
}

WindowSurfaceVk::SwapchainImage::SwapchainImage()  = default;
WindowSurfaceVk::SwapchainImage::~SwapchainImage() = default;

WindowSurfaceVk::SwapchainImage::SwapchainImage(SwapchainImage &&other)
    : image(std::move(other.image)),
      imageView(std::move(other.imageView)),
      framebuffer(std::move(other.framebuffer)),
      imageAcquiredSemaphore(std::move(other.imageAcquiredSemaphore)),
      commandsCompleteSemaphore(std::move(other.commandsCompleteSemaphore))
{
}

WindowSurfaceVk::WindowSurfaceVk(const egl::SurfaceState &surfaceState,
                                 EGLNativeWindowType window,
                                 EGLint width,
                                 EGLint height)
    : SurfaceImpl(surfaceState),
      mNativeWindowType(window),
      mSurface(VK_NULL_HANDLE),
      mInstance(VK_NULL_HANDLE),
      mSwapchain(VK_NULL_HANDLE),
      mColorRenderTarget(),
      mDepthStencilRenderTarget(),
      mCurrentSwapchainImageIndex(0)
{
    mColorRenderTarget.resource = this;
}

WindowSurfaceVk::~WindowSurfaceVk()
{
    ASSERT(mSurface == VK_NULL_HANDLE);
    ASSERT(mSwapchain == VK_NULL_HANDLE);
}

void WindowSurfaceVk::destroy(const egl::Display *display)
{
    const DisplayVk *displayVk = vk::GetImpl(display);
    RendererVk *renderer       = displayVk->getRenderer();
    VkDevice device            = renderer->getDevice();
    VkInstance instance        = renderer->getInstance();

    // We might not need to flush the pipe here.
    renderer->finish(display->getProxyContext());

    mAcquireNextImageSemaphore.destroy(device);

    mDepthStencilImage.release(renderer->getCurrentQueueSerial(), renderer);
    mDepthStencilImageView.destroy(device);

    for (SwapchainImage &swapchainImage : mSwapchainImages)
    {
        // Although we don't own the swapchain image handles, we need to keep our shutdown clean.
        swapchainImage.image.resetImageWeakReference();
        swapchainImage.image.destroy(device);
        swapchainImage.imageView.destroy(device);
        swapchainImage.framebuffer.destroy(device);
        swapchainImage.imageAcquiredSemaphore.destroy(device);
        swapchainImage.commandsCompleteSemaphore.destroy(device);
    }

    if (mSwapchain)
    {
        vkDestroySwapchainKHR(device, mSwapchain, nullptr);
        mSwapchain = VK_NULL_HANDLE;
    }

    if (mSurface)
    {
        vkDestroySurfaceKHR(instance, mSurface, nullptr);
        mSurface = VK_NULL_HANDLE;
    }
}

egl::Error WindowSurfaceVk::initialize(const egl::Display *display)
{
    const DisplayVk *displayVk = vk::GetImpl(display);
    return initializeImpl(displayVk->getRenderer()).toEGL(EGL_BAD_SURFACE);
}

vk::Error WindowSurfaceVk::initializeImpl(RendererVk *renderer)
{
    gl::Extents windowSize;
    ANGLE_TRY_RESULT(createSurfaceVk(renderer), windowSize);

    uint32_t presentQueue = 0;
    ANGLE_TRY_RESULT(renderer->selectPresentQueueForSurface(mSurface), presentQueue);
    UNUSED_VARIABLE(presentQueue);

    const VkPhysicalDevice &physicalDevice = renderer->getPhysicalDevice();

    VkSurfaceCapabilitiesKHR surfaceCaps;
    ANGLE_VK_TRY(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface, &surfaceCaps));

    // Adjust width and height to the swapchain if necessary.
    uint32_t width  = surfaceCaps.currentExtent.width;
    uint32_t height = surfaceCaps.currentExtent.height;

    // TODO(jmadill): Support devices which don't support copy. We use this for ReadPixels.
    ANGLE_VK_CHECK((surfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0,
                   VK_ERROR_INITIALIZATION_FAILED);

    EGLAttrib attribWidth  = mState.attributes.get(EGL_WIDTH, 0);
    EGLAttrib attribHeight = mState.attributes.get(EGL_HEIGHT, 0);

    if (surfaceCaps.currentExtent.width == 0xFFFFFFFFu)
    {
        ASSERT(surfaceCaps.currentExtent.height == 0xFFFFFFFFu);

        if (attribWidth == 0)
        {
            width = windowSize.width;
        }
        if (attribHeight == 0)
        {
            height = windowSize.height;
        }
    }

    gl::Extents extents(static_cast<int>(width), static_cast<int>(height), 1);

    uint32_t presentModeCount = 0;
    ANGLE_VK_TRY(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface,
                                                           &presentModeCount, nullptr));
    ASSERT(presentModeCount > 0);

    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    ANGLE_VK_TRY(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface,
                                                           &presentModeCount, presentModes.data()));

    // Select appropriate present mode based on vsync parameter.
    // TODO(jmadill): More complete implementation, which allows for changing and more values.
    const EGLint minSwapInterval = mState.config->minSwapInterval;
    const EGLint maxSwapInterval = mState.config->maxSwapInterval;
    ASSERT(minSwapInterval == 0 || minSwapInterval == 1);
    ASSERT(maxSwapInterval == 0 || maxSwapInterval == 1);

    VkPresentModeKHR swapchainPresentMode =
        GetDesiredPresentMode(presentModes, minSwapInterval, maxSwapInterval);

    // Determine number of swapchain images. Aim for one more than the minimum.
    uint32_t minImageCount = surfaceCaps.minImageCount + 1;
    if (surfaceCaps.maxImageCount > 0 && minImageCount > surfaceCaps.maxImageCount)
    {
        minImageCount = surfaceCaps.maxImageCount;
    }

    // Default to identity transform.
    VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    if ((surfaceCaps.supportedTransforms & preTransform) == 0)
    {
        preTransform = surfaceCaps.currentTransform;
    }

    uint32_t surfaceFormatCount = 0;
    ANGLE_VK_TRY(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &surfaceFormatCount,
                                                      nullptr));

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    ANGLE_VK_TRY(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &surfaceFormatCount,
                                                      surfaceFormats.data()));

    const vk::Format &format = renderer->getFormat(mState.config->renderTargetFormat);
    VkFormat nativeFormat    = format.vkTextureFormat;

    if (surfaceFormatCount == 1u && surfaceFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        // This is fine.
    }
    else
    {
        bool foundFormat = false;
        for (const VkSurfaceFormatKHR &surfaceFormat : surfaceFormats)
        {
            if (surfaceFormat.format == nativeFormat)
            {
                foundFormat = true;
                break;
            }
        }

        ANGLE_VK_CHECK(foundFormat, VK_ERROR_INITIALIZATION_FAILED);
    }

    VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ((surfaceCaps.supportedCompositeAlpha & compositeAlpha) == 0)
    {
        compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    ANGLE_VK_CHECK((surfaceCaps.supportedCompositeAlpha & compositeAlpha) != 0,
                   VK_ERROR_INITIALIZATION_FAILED);

    // We need transfer src for reading back from the backbuffer.
    VkImageUsageFlags imageUsageFlags =
        (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
         VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    VkSwapchainCreateInfoKHR swapchainInfo;
    swapchainInfo.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.pNext                 = nullptr;
    swapchainInfo.flags                 = 0;
    swapchainInfo.surface               = mSurface;
    swapchainInfo.minImageCount         = minImageCount;
    swapchainInfo.imageFormat           = nativeFormat;
    swapchainInfo.imageColorSpace       = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    swapchainInfo.imageExtent.width     = width;
    swapchainInfo.imageExtent.height    = height;
    swapchainInfo.imageArrayLayers      = 1;
    swapchainInfo.imageUsage            = imageUsageFlags;
    swapchainInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices   = nullptr;
    swapchainInfo.preTransform          = preTransform;
    swapchainInfo.compositeAlpha        = compositeAlpha;
    swapchainInfo.presentMode           = swapchainPresentMode;
    swapchainInfo.clipped               = VK_TRUE;
    swapchainInfo.oldSwapchain          = VK_NULL_HANDLE;

    VkDevice device = renderer->getDevice();
    ANGLE_VK_TRY(vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &mSwapchain));

    // Intialize the swapchain image views.
    uint32_t imageCount = 0;
    ANGLE_VK_TRY(vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, nullptr));

    std::vector<VkImage> swapchainImages(imageCount);
    ANGLE_VK_TRY(vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, swapchainImages.data()));

    // Allocate a command buffer for clearing our images to black.
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));

    VkClearColorValue transparentBlack;
    transparentBlack.float32[0] = 0.0f;
    transparentBlack.float32[1] = 0.0f;
    transparentBlack.float32[2] = 0.0f;
    transparentBlack.float32[3] = 0.0f;

    mSwapchainImages.resize(imageCount);

    ANGLE_TRY(mAcquireNextImageSemaphore.init(device));

    for (uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        SwapchainImage &member = mSwapchainImages[imageIndex];
        member.image.init2DWeakReference(swapchainImages[imageIndex], extents, format, 1);
        member.image.initImageView(device, VK_IMAGE_ASPECT_COLOR_BIT, gl::SwizzleState(),
                                   &member.imageView);

        // Set transfer dest layout, and clear the image to black.
        member.image.clearColor(transparentBlack, commandBuffer);

        ANGLE_TRY(member.imageAcquiredSemaphore.init(device));
        ANGLE_TRY(member.commandsCompleteSemaphore.init(device));
    }

    // Get the first available swapchain iamge.
    ANGLE_TRY(nextSwapchainImage(renderer));

    // Initialize depth/stencil if requested.
    if (mState.config->depthStencilFormat != GL_NONE)
    {
        const vk::Format &dsFormat = renderer->getFormat(mState.config->depthStencilFormat);

        const VkImageUsageFlags usage =
            (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
             VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);

        ANGLE_TRY(mDepthStencilImage.init2D(device, extents, dsFormat, 1, usage));
        ANGLE_TRY(mDepthStencilImage.initMemory(device, renderer->getMemoryProperties(),
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        const VkImageAspectFlags aspect =
            (dsFormat.textureFormat().depthBits > 0 ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (dsFormat.textureFormat().stencilBits > 0 ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

        VkClearDepthStencilValue depthStencilClearValue = {1.0f, 0};

        // Set transfer dest layout, and clear the image.
        mDepthStencilImage.clearDepthStencil(aspect, depthStencilClearValue, commandBuffer);

        ANGLE_TRY(mDepthStencilImage.initImageView(device, aspect, gl::SwizzleState(),
                                                   &mDepthStencilImageView));

        mDepthStencilRenderTarget.resource  = this;
        mDepthStencilRenderTarget.image     = &mDepthStencilImage;
        mDepthStencilRenderTarget.imageView = &mDepthStencilImageView;

        // TODO(jmadill): Figure out how to pass depth/stencil image views to the RenderTargetVk.
    }

    return vk::NoError();
}

FramebufferImpl *WindowSurfaceVk::createDefaultFramebuffer(const gl::FramebufferState &state)
{
    return FramebufferVk::CreateDefaultFBO(state, this);
}

egl::Error WindowSurfaceVk::swap(const gl::Context *context)
{
    const DisplayVk *displayVk = vk::GetImpl(context->getCurrentDisplay());
    RendererVk *renderer       = displayVk->getRenderer();

    vk::CommandBuffer *swapCommands = nullptr;
    ANGLE_TRY(beginWriteResource(renderer, &swapCommands));

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    image.image.changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                                       VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                       VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, swapCommands);

    ANGLE_TRY(
        renderer->flush(context, image.imageAcquiredSemaphore, image.commandsCompleteSemaphore));

    VkPresentInfoKHR presentInfo;
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.pNext              = nullptr;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = image.commandsCompleteSemaphore.ptr();
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &mSwapchain;
    presentInfo.pImageIndices      = &mCurrentSwapchainImageIndex;
    presentInfo.pResults           = nullptr;

    ANGLE_VK_TRY(vkQueuePresentKHR(renderer->getQueue(), &presentInfo));

    // Get the next available swapchain image.
    ANGLE_TRY(nextSwapchainImage(renderer));

    return vk::NoError();
}

vk::Error WindowSurfaceVk::nextSwapchainImage(RendererVk *renderer)
{
    VkDevice device = renderer->getDevice();

    ANGLE_VK_TRY(vkAcquireNextImageKHR(device, mSwapchain, UINT64_MAX,
                                       mAcquireNextImageSemaphore.getHandle(), VK_NULL_HANDLE,
                                       &mCurrentSwapchainImageIndex));

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    // Swap the unused swapchain semaphore and the now active spare semaphore.
    std::swap(image.imageAcquiredSemaphore, mAcquireNextImageSemaphore);

    // Update RenderTarget pointers.
    mColorRenderTarget.image     = &image.image;
    mColorRenderTarget.imageView = &image.imageView;

    return vk::NoError();
}

egl::Error WindowSurfaceVk::postSubBuffer(const gl::Context *context,
                                          EGLint x,
                                          EGLint y,
                                          EGLint width,
                                          EGLint height)
{
    // TODO(jmadill)
    return egl::NoError();
}

egl::Error WindowSurfaceVk::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    UNREACHABLE();
    return egl::EglBadCurrentSurface();
}

egl::Error WindowSurfaceVk::bindTexImage(gl::Texture *texture, EGLint buffer)
{
    return egl::NoError();
}

egl::Error WindowSurfaceVk::releaseTexImage(EGLint buffer)
{
    return egl::NoError();
}

egl::Error WindowSurfaceVk::getSyncValues(EGLuint64KHR * /*ust*/,
                                          EGLuint64KHR * /*msc*/,
                                          EGLuint64KHR * /*sbc*/)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

void WindowSurfaceVk::setSwapInterval(EGLint interval)
{
}

EGLint WindowSurfaceVk::getWidth() const
{
    return static_cast<EGLint>(mColorRenderTarget.image->getExtents().width);
}

EGLint WindowSurfaceVk::getHeight() const
{
    return static_cast<EGLint>(mColorRenderTarget.image->getExtents().height);
}

EGLint WindowSurfaceVk::isPostSubBufferSupported() const
{
    // TODO(jmadill)
    return EGL_FALSE;
}

EGLint WindowSurfaceVk::getSwapBehavior() const
{
    // TODO(jmadill)
    return EGL_BUFFER_DESTROYED;
}

gl::Error WindowSurfaceVk::getAttachmentRenderTarget(const gl::Context * /*context*/,
                                                     GLenum binding,
                                                     const gl::ImageIndex & /*target*/,
                                                     FramebufferAttachmentRenderTarget **rtOut)
{
    if (binding == GL_BACK)
    {
        *rtOut = &mColorRenderTarget;
    }
    else
    {
        ASSERT(binding == GL_DEPTH || binding == GL_STENCIL || binding == GL_DEPTH_STENCIL);
        *rtOut = &mDepthStencilRenderTarget;
    }

    return gl::NoError();
}

gl::ErrorOrResult<vk::Framebuffer *> WindowSurfaceVk::getCurrentFramebuffer(
    VkDevice device,
    const vk::RenderPass &compatibleRenderPass)
{
    vk::Framebuffer &currentFramebuffer = mSwapchainImages[mCurrentSwapchainImageIndex].framebuffer;

    if (currentFramebuffer.valid())
    {
        // Validation layers should detect if the render pass is really compatible.
        return &currentFramebuffer;
    }

    VkFramebufferCreateInfo framebufferInfo;

    const gl::Extents &extents            = mColorRenderTarget.image->getExtents();
    std::array<VkImageView, 2> imageViews = {{VK_NULL_HANDLE, mDepthStencilImageView.getHandle()}};

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext           = nullptr;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = compatibleRenderPass.getHandle();
    framebufferInfo.attachmentCount = (mDepthStencilImage.valid() ? 2u : 1u);
    framebufferInfo.pAttachments    = imageViews.data();
    framebufferInfo.width           = static_cast<uint32_t>(extents.width);
    framebufferInfo.height          = static_cast<uint32_t>(extents.height);
    framebufferInfo.layers          = 1;

    for (SwapchainImage &swapchainImage : mSwapchainImages)
    {
        imageViews[0] = swapchainImage.imageView.getHandle();
        ANGLE_TRY(swapchainImage.framebuffer.init(device, framebufferInfo));
    }

    ASSERT(currentFramebuffer.valid());
    return &currentFramebuffer;
}

gl::Error WindowSurfaceVk::initializeContents(const gl::Context *context,
                                              const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return gl::NoError();
}

}  // namespace rx

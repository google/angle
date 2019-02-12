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
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "third_party/trace_event/trace_event.h"

namespace rx
{

namespace
{

VkPresentModeKHR GetDesiredPresentMode(const std::vector<VkPresentModeKHR> &presentModes,
                                       EGLint interval)
{
    ASSERT(!presentModes.empty());

    // If v-sync is enabled, use FIFO, which throttles you to the display rate and is guaranteed to
    // always be supported.
    if (interval > 0)
    {
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    // Otherwise, choose either of the following, if available, in order specified here:
    //
    // - Mailbox is similar to triple-buffering.
    // - Immediate is similar to single-buffering.
    //
    // If neither is supported, we fallback to FIFO.

    bool mailboxAvailable   = false;
    bool immediateAvailable = false;

    for (VkPresentModeKHR presentMode : presentModes)
    {
        switch (presentMode)
        {
            case VK_PRESENT_MODE_MAILBOX_KHR:
                mailboxAvailable = true;
                break;
            case VK_PRESENT_MODE_IMMEDIATE_KHR:
                immediateAvailable = true;
                break;
            default:
                break;
        }
    }

    // Note again that VK_PRESENT_MODE_FIFO_KHR is guaranteed to be available.
    return mailboxAvailable
               ? VK_PRESENT_MODE_MAILBOX_KHR
               : immediateAvailable ? VK_PRESENT_MODE_IMMEDIATE_KHR : VK_PRESENT_MODE_FIFO_KHR;
}

constexpr VkImageUsageFlags kSurfaceVKImageUsageFlags =
    VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
constexpr VkImageUsageFlags kSurfaceVKColorImageUsageFlags =
    kSurfaceVKImageUsageFlags | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
constexpr VkImageUsageFlags kSurfaceVKDepthStencilImageUsageFlags =
    kSurfaceVKImageUsageFlags | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

}  // namespace

OffscreenSurfaceVk::AttachmentImage::AttachmentImage()
{
    renderTarget.init(&image, &imageView, 0, nullptr);
}

OffscreenSurfaceVk::AttachmentImage::~AttachmentImage() = default;

angle::Result OffscreenSurfaceVk::AttachmentImage::initialize(DisplayVk *displayVk,
                                                              EGLint width,
                                                              EGLint height,
                                                              const vk::Format &vkFormat)
{
    RendererVk *renderer = displayVk->getRenderer();

    const angle::Format &textureFormat = vkFormat.textureFormat();
    bool isDepthOrStencilFormat   = textureFormat.depthBits > 0 || textureFormat.stencilBits > 0;
    const VkImageUsageFlags usage = isDepthOrStencilFormat ? kSurfaceVKDepthStencilImageUsageFlags
                                                           : kSurfaceVKColorImageUsageFlags;

    gl::Extents extents(std::max(static_cast<int>(width), 1), std::max(static_cast<int>(height), 1),
                        1);
    ANGLE_TRY(image.init(displayVk, gl::TextureType::_2D, extents, vkFormat, 1, usage, 1, 1));

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ANGLE_TRY(image.initMemory(displayVk, renderer->getMemoryProperties(), flags));

    VkImageAspectFlags aspect = vk::GetFormatAspectFlags(textureFormat);

    ANGLE_TRY(image.initImageView(displayVk, gl::TextureType::_2D, aspect, gl::SwizzleState(),
                                  &imageView, 1));

    return angle::Result::Continue;
}

void OffscreenSurfaceVk::AttachmentImage::destroy(const egl::Display *display)
{
    const DisplayVk *displayVk = vk::GetImpl(display);
    RendererVk *renderer       = displayVk->getRenderer();

    image.releaseImage(renderer);
    image.releaseStagingBuffer(renderer);
    renderer->releaseObject(renderer->getCurrentQueueSerial(), &imageView);
}

OffscreenSurfaceVk::OffscreenSurfaceVk(const egl::SurfaceState &surfaceState,
                                       EGLint width,
                                       EGLint height)
    : SurfaceImpl(surfaceState), mWidth(width), mHeight(height)
{}

OffscreenSurfaceVk::~OffscreenSurfaceVk() {}

egl::Error OffscreenSurfaceVk::initialize(const egl::Display *display)
{
    DisplayVk *displayVk = vk::GetImpl(display);
    angle::Result result = initializeImpl(displayVk);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

angle::Result OffscreenSurfaceVk::initializeImpl(DisplayVk *displayVk)
{
    RendererVk *renderer      = displayVk->getRenderer();
    const egl::Config *config = mState.config;

    if (config->renderTargetFormat != GL_NONE)
    {
        ANGLE_TRY(mColorAttachment.initialize(displayVk, mWidth, mHeight,
                                              renderer->getFormat(config->renderTargetFormat)));
    }

    if (config->depthStencilFormat != GL_NONE)
    {
        ANGLE_TRY(mDepthStencilAttachment.initialize(
            displayVk, mWidth, mHeight, renderer->getFormat(config->depthStencilFormat)));
    }

    return angle::Result::Continue;
}

void OffscreenSurfaceVk::destroy(const egl::Display *display)
{
    mColorAttachment.destroy(display);
    mDepthStencilAttachment.destroy(display);
}

FramebufferImpl *OffscreenSurfaceVk::createDefaultFramebuffer(const gl::Context *context,
                                                              const gl::FramebufferState &state)
{
    RendererVk *renderer = vk::GetImpl(context)->getRenderer();

    // Use a user FBO for an offscreen RT.
    return FramebufferVk::CreateUserFBO(renderer, state);
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

egl::Error OffscreenSurfaceVk::bindTexImage(const gl::Context * /*context*/,
                                            gl::Texture * /*texture*/,
                                            EGLint /*buffer*/)
{
    return egl::NoError();
}

egl::Error OffscreenSurfaceVk::releaseTexImage(const gl::Context * /*context*/, EGLint /*buffer*/)
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

void OffscreenSurfaceVk::setSwapInterval(EGLint /*interval*/) {}

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
    return EGL_BUFFER_DESTROYED;
}

angle::Result OffscreenSurfaceVk::getAttachmentRenderTarget(
    const gl::Context *context,
    GLenum binding,
    const gl::ImageIndex &imageIndex,
    FramebufferAttachmentRenderTarget **rtOut)
{
    if (binding == GL_BACK)
    {
        *rtOut = &mColorAttachment.renderTarget;
    }
    else
    {
        ASSERT(binding == GL_DEPTH || binding == GL_STENCIL || binding == GL_DEPTH_STENCIL);
        *rtOut = &mDepthStencilAttachment.renderTarget;
    }

    return angle::Result::Continue;
}

angle::Result OffscreenSurfaceVk::initializeContents(const gl::Context *context,
                                                     const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

vk::ImageHelper *OffscreenSurfaceVk::getColorAttachmentImage()
{
    return &mColorAttachment.image;
}

WindowSurfaceVk::SwapchainImage::SwapchainImage()  = default;
WindowSurfaceVk::SwapchainImage::~SwapchainImage() = default;

WindowSurfaceVk::SwapchainImage::SwapchainImage(SwapchainImage &&other)
    : image(std::move(other.image)),
      imageView(std::move(other.imageView)),
      framebuffer(std::move(other.framebuffer))
{}

WindowSurfaceVk::WindowSurfaceVk(const egl::SurfaceState &surfaceState,
                                 EGLNativeWindowType window,
                                 EGLint width,
                                 EGLint height)
    : SurfaceImpl(surfaceState),
      mNativeWindowType(window),
      mSurface(VK_NULL_HANDLE),
      mInstance(VK_NULL_HANDLE),
      mSwapchain(VK_NULL_HANDLE),
      mSwapchainPresentMode(VK_PRESENT_MODE_FIFO_KHR),
      mDesiredSwapchainPresentMode(VK_PRESENT_MODE_FIFO_KHR),
      mMinImageCount(0),
      mPreTransform(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR),
      mCompositeAlpha(VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR),
      mCurrentSwapchainImageIndex(0),
      mCurrentSwapHistoryIndex(0)
{
    mDepthStencilRenderTarget.init(&mDepthStencilImage, &mDepthStencilImageView, 0, nullptr);
}

WindowSurfaceVk::~WindowSurfaceVk()
{
    ASSERT(mSurface == VK_NULL_HANDLE);
    ASSERT(mSwapchain == VK_NULL_HANDLE);
}

void WindowSurfaceVk::destroy(const egl::Display *display)
{
    DisplayVk *displayVk = vk::GetImpl(display);
    RendererVk *renderer = displayVk->getRenderer();
    VkDevice device      = renderer->getDevice();
    VkInstance instance  = renderer->getInstance();

    // We might not need to flush the pipe here.
    (void)renderer->finish(displayVk);

    releaseSwapchainImages(renderer);

    for (SwapHistory &swap : mSwapHistory)
    {
        if (swap.swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, swap.swapchain, nullptr);
            swap.swapchain = VK_NULL_HANDLE;
        }
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
    DisplayVk *displayVk = vk::GetImpl(display);
    angle::Result result = initializeImpl(displayVk);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

angle::Result WindowSurfaceVk::initializeImpl(DisplayVk *displayVk)
{
    RendererVk *renderer = displayVk->getRenderer();

    gl::Extents windowSize;
    ANGLE_TRY(createSurfaceVk(displayVk, &windowSize));

    uint32_t presentQueue = 0;
    ANGLE_TRY(renderer->selectPresentQueueForSurface(displayVk, mSurface, &presentQueue));
    ANGLE_UNUSED_VARIABLE(presentQueue);

    const VkPhysicalDevice &physicalDevice = renderer->getPhysicalDevice();

    ANGLE_VK_TRY(displayVk, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface,
                                                                      &mSurfaceCaps));

    // Adjust width and height to the swapchain if necessary.
    uint32_t width  = mSurfaceCaps.currentExtent.width;
    uint32_t height = mSurfaceCaps.currentExtent.height;

    // TODO(jmadill): Support devices which don't support copy. We use this for ReadPixels.
    ANGLE_VK_CHECK(displayVk,
                   (mSurfaceCaps.supportedUsageFlags & VK_IMAGE_USAGE_TRANSFER_SRC_BIT) != 0,
                   VK_ERROR_INITIALIZATION_FAILED);

    EGLAttrib attribWidth  = mState.attributes.get(EGL_WIDTH, 0);
    EGLAttrib attribHeight = mState.attributes.get(EGL_HEIGHT, 0);

    if (mSurfaceCaps.currentExtent.width == 0xFFFFFFFFu)
    {
        ASSERT(mSurfaceCaps.currentExtent.height == 0xFFFFFFFFu);

        width  = (attribWidth != 0) ? static_cast<uint32_t>(attribWidth) : windowSize.width;
        height = (attribHeight != 0) ? static_cast<uint32_t>(attribHeight) : windowSize.height;
    }

    gl::Extents extents(static_cast<int>(width), static_cast<int>(height), 1);

    uint32_t presentModeCount = 0;
    ANGLE_VK_TRY(displayVk, vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, mSurface,
                                                                      &presentModeCount, nullptr));
    ASSERT(presentModeCount > 0);

    mPresentModes.resize(presentModeCount);
    ANGLE_VK_TRY(displayVk, vkGetPhysicalDeviceSurfacePresentModesKHR(
                                physicalDevice, mSurface, &presentModeCount, mPresentModes.data()));

    // Select appropriate present mode based on vsync parameter.  Default to 1 (FIFO), though it
    // will get clamped to the min/max values specified at display creation time.
    setSwapInterval(1);

    // Default to identity transform.
    mPreTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    if ((mSurfaceCaps.supportedTransforms & mPreTransform) == 0)
    {
        mPreTransform = mSurfaceCaps.currentTransform;
    }

    uint32_t surfaceFormatCount = 0;
    ANGLE_VK_TRY(displayVk, vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface,
                                                                 &surfaceFormatCount, nullptr));

    std::vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatCount);
    ANGLE_VK_TRY(displayVk,
                 vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, mSurface, &surfaceFormatCount,
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

        ANGLE_VK_CHECK(displayVk, foundFormat, VK_ERROR_INITIALIZATION_FAILED);
    }

    mCompositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    if ((mSurfaceCaps.supportedCompositeAlpha & mCompositeAlpha) == 0)
    {
        mCompositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }
    ANGLE_VK_CHECK(displayVk, (mSurfaceCaps.supportedCompositeAlpha & mCompositeAlpha) != 0,
                   VK_ERROR_INITIALIZATION_FAILED);

    ANGLE_TRY(recreateSwapchain(displayVk, extents, mCurrentSwapHistoryIndex));

    // Get the first available swapchain iamge.
    return nextSwapchainImage(displayVk);
}

angle::Result WindowSurfaceVk::recreateSwapchain(DisplayVk *displayVk,
                                                 const gl::Extents &extents,
                                                 uint32_t swapHistoryIndex)
{
    RendererVk *renderer = displayVk->getRenderer();
    VkDevice device      = renderer->getDevice();

    VkSwapchainKHR oldSwapchain = mSwapchain;
    mSwapchain                  = VK_NULL_HANDLE;

    if (oldSwapchain)
    {
        // Note: the old swapchain must be destroyed regardless of whether creating the new
        // swapchain succeeds.  We can only destroy the swapchain once rendering to all its images
        // have finished.  We therefore store the handle to the swapchain being destroyed in the
        // swap history (alongside the serial of the last submission) so it can be destroyed once we
        // wait on that serial as part of the CPU throttling.
        //
        // TODO(syoussefi): the spec specifically allows multiple retired swapchains to exist:
        //
        // > Multiple retired swapchains can be associated with the same VkSurfaceKHR through
        // > multiple uses of oldSwapchain that outnumber calls to vkDestroySwapchainKHR.
        //
        // However, a bug in the validation layers currently forces us to limit this to one retired
        // swapchain.  Once the issue is resolved, the following for loop can be removed.
        // http://anglebug.com/3095
        for (SwapHistory &swap : mSwapHistory)
        {
            if (swap.swapchain != VK_NULL_HANDLE)
            {
                ANGLE_TRY(renderer->finishToSerial(displayVk, swap.serial));
                vkDestroySwapchainKHR(renderer->getDevice(), swap.swapchain, nullptr);
                swap.swapchain = VK_NULL_HANDLE;
            }
        }
        mSwapHistory[swapHistoryIndex].swapchain = oldSwapchain;
    }

    releaseSwapchainImages(renderer);

    const vk::Format &format = renderer->getFormat(mState.config->renderTargetFormat);
    VkFormat nativeFormat    = format.vkTextureFormat;

    // We need transfer src for reading back from the backbuffer.
    constexpr VkImageUsageFlags kImageUsageFlags = kSurfaceVKColorImageUsageFlags;

    VkSwapchainCreateInfoKHR swapchainInfo = {};
    swapchainInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainInfo.flags                    = 0;
    swapchainInfo.surface                  = mSurface;
    swapchainInfo.minImageCount            = mMinImageCount;
    swapchainInfo.imageFormat              = nativeFormat;
    swapchainInfo.imageColorSpace          = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
    // Note: Vulkan doesn't allow 0-width/height swapchains.
    swapchainInfo.imageExtent.width        = std::max(extents.width, 1);
    swapchainInfo.imageExtent.height       = std::max(extents.height, 1);
    swapchainInfo.imageArrayLayers         = 1;
    swapchainInfo.imageUsage               = kImageUsageFlags;
    swapchainInfo.imageSharingMode         = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount    = 0;
    swapchainInfo.pQueueFamilyIndices      = nullptr;
    swapchainInfo.preTransform             = mPreTransform;
    swapchainInfo.compositeAlpha           = mCompositeAlpha;
    swapchainInfo.presentMode              = mDesiredSwapchainPresentMode;
    swapchainInfo.clipped                  = VK_TRUE;
    swapchainInfo.oldSwapchain             = oldSwapchain;

    // TODO(syoussefi): Once EGL_SWAP_BEHAVIOR_PRESERVED_BIT is supported, the contents of the old
    // swapchain need to carry over to the new one.  http://anglebug.com/2942
    ANGLE_VK_TRY(displayVk, vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &mSwapchain));
    mSwapchainPresentMode = mDesiredSwapchainPresentMode;

    // Intialize the swapchain image views.
    uint32_t imageCount = 0;
    ANGLE_VK_TRY(displayVk, vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, nullptr));

    std::vector<VkImage> swapchainImages(imageCount);
    ANGLE_VK_TRY(displayVk,
                 vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, swapchainImages.data()));

    VkClearColorValue transparentBlack = {};
    transparentBlack.float32[0]        = 0.0f;
    transparentBlack.float32[1]        = 0.0f;
    transparentBlack.float32[2]        = 0.0f;
    transparentBlack.float32[3]        = 0.0f;

    mSwapchainImages.resize(imageCount);
    ANGLE_TRY(resizeSwapHistory(displayVk, imageCount));

    for (uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        SwapchainImage &member = mSwapchainImages[imageIndex];
        member.image.init2DWeakReference(swapchainImages[imageIndex], extents, format, 1);

        ANGLE_TRY(member.image.initImageView(displayVk, gl::TextureType::_2D,
                                             VK_IMAGE_ASPECT_COLOR_BIT, gl::SwizzleState(),
                                             &member.imageView, 1));

        // Allocate a command buffer for clearing our images to black.
        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(member.image.recordCommands(displayVk, &commandBuffer));

        // Set transfer dest layout, and clear the image to black.
        member.image.clearColor(transparentBlack, 0, 1, commandBuffer);
    }

    // Initialize depth/stencil if requested.
    if (mState.config->depthStencilFormat != GL_NONE)
    {
        const vk::Format &dsFormat = renderer->getFormat(mState.config->depthStencilFormat);

        const VkImageUsageFlags dsUsage = kSurfaceVKDepthStencilImageUsageFlags;

        ANGLE_TRY(mDepthStencilImage.init(displayVk, gl::TextureType::_2D, extents, dsFormat, 1,
                                          dsUsage, 1, 1));
        ANGLE_TRY(mDepthStencilImage.initMemory(displayVk, renderer->getMemoryProperties(),
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        const VkImageAspectFlags aspect = vk::GetDepthStencilAspectFlags(dsFormat.textureFormat());
        VkClearDepthStencilValue depthStencilClearValue = {1.0f, 0};

        // Clear the image.
        vk::CommandBuffer *commandBuffer = nullptr;
        ANGLE_TRY(mDepthStencilImage.recordCommands(displayVk, &commandBuffer));
        mDepthStencilImage.clearDepthStencil(aspect, aspect, depthStencilClearValue, commandBuffer);

        ANGLE_TRY(mDepthStencilImage.initImageView(displayVk, gl::TextureType::_2D, aspect,
                                                   gl::SwizzleState(), &mDepthStencilImageView, 1));

        // We will need to pass depth/stencil image views to the RenderTargetVk in the future.
    }

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::checkForOutOfDateSwapchain(DisplayVk *displayVk,
                                                          uint32_t swapHistoryIndex,
                                                          bool presentOutOfDate)
{
    bool swapIntervalChanged = mSwapchainPresentMode != mDesiredSwapchainPresentMode;

    // Check for window resize and recreate swapchain if necessary.
    gl::Extents currentExtents;
    ANGLE_TRY(getCurrentWindowSize(displayVk, &currentExtents));

    gl::Extents swapchainExtents(getWidth(), getHeight(), 0);

    // If window size has changed, check with surface capabilities.  It has been observed on
    // Android that `getCurrentWindowSize()` returns 1920x1080 for example, while surface
    // capabilities returns the size the surface was created with.
    if (currentExtents != swapchainExtents)
    {
        const VkPhysicalDevice &physicalDevice = displayVk->getRenderer()->getPhysicalDevice();
        ANGLE_VK_TRY(displayVk, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface,
                                                                          &mSurfaceCaps));

        uint32_t width  = mSurfaceCaps.currentExtent.width;
        uint32_t height = mSurfaceCaps.currentExtent.height;

        if (width != 0xFFFFFFFFu)
        {
            ASSERT(height != 0xFFFFFFFFu);
            currentExtents.width  = width;
            currentExtents.height = height;
        }
    }

    // If anything has changed, recreate the swapchain.
    if (presentOutOfDate || swapIntervalChanged || currentExtents != swapchainExtents)
    {
        ANGLE_TRY(recreateSwapchain(displayVk, currentExtents, swapHistoryIndex));
    }

    return angle::Result::Continue;
}

void WindowSurfaceVk::releaseSwapchainImages(RendererVk *renderer)
{
    if (mDepthStencilImage.valid())
    {
        Serial depthStencilSerial = mDepthStencilImage.getStoredQueueSerial();
        mDepthStencilImage.releaseImage(renderer);
        mDepthStencilImage.releaseStagingBuffer(renderer);

        if (mDepthStencilImageView.valid())
        {
            renderer->releaseObject(depthStencilSerial, &mDepthStencilImageView);
        }
    }

    for (SwapchainImage &swapchainImage : mSwapchainImages)
    {
        Serial imageSerial = swapchainImage.image.getStoredQueueSerial();

        // We don't own the swapchain image handles, so we just remove our reference to it.
        swapchainImage.image.resetImageWeakReference();
        swapchainImage.image.destroy(renderer->getDevice());

        if (swapchainImage.imageView.valid())
        {
            renderer->releaseObject(imageSerial, &swapchainImage.imageView);
        }

        if (swapchainImage.framebuffer.valid())
        {
            renderer->releaseObject(imageSerial, &swapchainImage.framebuffer);
        }
    }

    mSwapchainImages.clear();
}

FramebufferImpl *WindowSurfaceVk::createDefaultFramebuffer(const gl::Context *context,
                                                           const gl::FramebufferState &state)
{
    RendererVk *renderer = vk::GetImpl(context)->getRenderer();
    return FramebufferVk::CreateDefaultFBO(renderer, state, this);
}

egl::Error WindowSurfaceVk::swapWithDamage(const gl::Context *context,
                                           EGLint *rects,
                                           EGLint n_rects)
{
    DisplayVk *displayVk = vk::GetImpl(context->getCurrentDisplay());
    angle::Result result = swapImpl(displayVk, rects, n_rects);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

egl::Error WindowSurfaceVk::swap(const gl::Context *context)
{
    DisplayVk *displayVk = vk::GetImpl(context->getCurrentDisplay());
    angle::Result result = swapImpl(displayVk, nullptr, 0);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

angle::Result WindowSurfaceVk::swapImpl(DisplayVk *displayVk, EGLint *rects, EGLint n_rects)
{
    RendererVk *renderer = displayVk->getRenderer();

    // Throttle the submissions to avoid getting too far ahead of the GPU.
    {
        TRACE_EVENT0("gpu.angle", "WindowSurfaceVk::swapImpl: Throttle CPU");
        SwapHistory &swap = mSwapHistory[mCurrentSwapHistoryIndex];
        ANGLE_TRY(renderer->finishToSerial(displayVk, swap.serial));
        if (swap.swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(renderer->getDevice(), swap.swapchain, nullptr);
            swap.swapchain = VK_NULL_HANDLE;
        }
    }

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    vk::CommandBuffer *swapCommands = nullptr;
    ANGLE_TRY(image.image.recordCommands(displayVk, &swapCommands));

    image.image.changeLayout(VK_IMAGE_ASPECT_COLOR_BIT, vk::ImageLayout::Present, swapCommands);

    ANGLE_TRY(renderer->flush(displayVk));

    // Remember the serial of the last submission.
    mSwapHistory[mCurrentSwapHistoryIndex].serial = renderer->getLastSubmittedQueueSerial();
    uint32_t currentSwapHistoryIndex              = mCurrentSwapHistoryIndex;
    ++mCurrentSwapHistoryIndex;
    mCurrentSwapHistoryIndex =
        mCurrentSwapHistoryIndex == mSwapHistory.size() ? 0 : mCurrentSwapHistoryIndex;

    // Ask the renderer what semaphore it signaled in the last flush.
    const vk::Semaphore *commandsCompleteSemaphore =
        renderer->getSubmitLastSignaledSemaphore(displayVk);

    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = commandsCompleteSemaphore ? 1 : 0;
    presentInfo.pWaitSemaphores =
        commandsCompleteSemaphore ? commandsCompleteSemaphore->ptr() : nullptr;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains    = &mSwapchain;
    presentInfo.pImageIndices  = &mCurrentSwapchainImageIndex;
    presentInfo.pResults       = nullptr;

    VkPresentRegionsKHR presentRegions = {};
    if (renderer->getFeatures().supportsIncrementalPresent && (n_rects > 0))
    {
        VkPresentRegionKHR presentRegion = {};
        std::vector<VkRectLayerKHR> vk_rects(n_rects);
        EGLint *egl_rects = rects;
        presentRegion.rectangleCount = n_rects;
        for (EGLint rect = 0; rect < n_rects; rect++)
        {
            vk_rects[rect].offset.x      = *egl_rects++;
            vk_rects[rect].offset.y      = *egl_rects++;
            vk_rects[rect].extent.width  = *egl_rects++;
            vk_rects[rect].extent.height = *egl_rects++;
            vk_rects[rect].layer         = 0;
        }
        presentRegion.pRectangles = vk_rects.data();

        presentRegions.sType          = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
        presentRegions.pNext          = nullptr;
        presentRegions.swapchainCount = 1;
        presentRegions.pRegions       = &presentRegion;

        presentInfo.pNext = &presentRegions;
    }

    VkResult result = vkQueuePresentKHR(renderer->getQueue(), &presentInfo);

    // If SUBOPTIMAL/OUT_OF_DATE is returned, it's ok, we just need to recreate the swapchain before
    // continuing.
    bool swapchainOutOfDate = result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR;
    if (!swapchainOutOfDate)
    {
        ANGLE_VK_TRY(displayVk, result);
    }

    ANGLE_TRY(checkForOutOfDateSwapchain(displayVk, currentSwapHistoryIndex, swapchainOutOfDate));

    {
        // Note: TRACE_EVENT0 is put here instead of inside the function to workaround this issue:
        // http://anglebug.com/2927
        TRACE_EVENT0("gpu.angle", "nextSwapchainImage");
        // Get the next available swapchain image.
        ANGLE_TRY(nextSwapchainImage(displayVk));
    }

    ANGLE_TRY(renderer->syncPipelineCacheVk(displayVk));

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::nextSwapchainImage(DisplayVk *displayVk)
{
    VkDevice device      = displayVk->getDevice();
    RendererVk *renderer = displayVk->getRenderer();

    const vk::Semaphore *acquireNextImageSemaphore = nullptr;
    ANGLE_TRY(renderer->allocateSubmitWaitSemaphore(displayVk, &acquireNextImageSemaphore));

    ANGLE_VK_TRY(displayVk, vkAcquireNextImageKHR(device, mSwapchain, UINT64_MAX,
                                                  acquireNextImageSemaphore->getHandle(),
                                                  VK_NULL_HANDLE, &mCurrentSwapchainImageIndex));

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    // Update RenderTarget pointers.
    mColorRenderTarget.updateSwapchainImage(&image.image, &image.imageView);

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::resizeSwapHistory(DisplayVk *displayVk, size_t imageCount)
{
    // The number of swapchain images can change if the present mode is changed.  If that number is
    // increased, we need to rearrange the history (which is a circular buffer) so it remains
    // continuous.  If it shrinks, we have to additionally make sure we clean up any old swapchains
    // that can no longer fit in the history.
    //
    // Assume the following history buffer identified with serials:
    //
    //   mCurrentSwapHistoryIndex
    //           V
    //   +----+----+----+
    //   | 11 |  9 | 10 |
    //   +----+----+----+
    //
    // When shrinking to size 2, we want to clean up 9, and rearrange to the following:
    //
    //   mCurrentSwapHistoryIndex
    //      V
    //   +----+----+
    //   | 10 | 11 |
    //   +----+----+
    //
    // When expanding back to 3, we want to rearrange to the following:
    //
    //   mCurrentSwapHistoryIndex
    //      V
    //   +----+----+----+
    //   |  0 | 10 | 11 |
    //   +----+----+----+

    if (mSwapHistory.size() == imageCount)
    {
        return angle::Result::Continue;
    }

    RendererVk *renderer = displayVk->getRenderer();

    // First, clean up anything that won't fit in the resized history.
    if (imageCount < mSwapHistory.size())
    {
        size_t toClean = mSwapHistory.size() - imageCount;
        for (size_t i = 0; i < toClean; ++i)
        {
            size_t historyIndex = (mCurrentSwapHistoryIndex + i) % mSwapHistory.size();
            SwapHistory &swap   = mSwapHistory[historyIndex];

            ANGLE_TRY(renderer->finishToSerial(displayVk, swap.serial));
            if (swap.swapchain != VK_NULL_HANDLE)
            {
                vkDestroySwapchainKHR(renderer->getDevice(), swap.swapchain, nullptr);
                swap.swapchain = VK_NULL_HANDLE;
            }
        }
    }

    // Now, move the history, from most recent to oldest (as much as fits), into a new vector.
    std::vector<SwapHistory> resizedHistory(imageCount);

    size_t toCopy = std::min(imageCount, mSwapHistory.size());
    for (size_t i = 0; i < toCopy; ++i)
    {
        size_t historyIndex =
            (mCurrentSwapHistoryIndex + mSwapHistory.size() - i - 1) % mSwapHistory.size();
        size_t resizedHistoryIndex          = imageCount - i - 1;
        resizedHistory[resizedHistoryIndex] = mSwapHistory[historyIndex];
    }

    // Set this as the new history.  Note that after rearranging in either case, the oldest history
    // is at index 0.
    mSwapHistory             = std::move(resizedHistory);
    mCurrentSwapHistoryIndex = 0;

    return angle::Result::Continue;
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

egl::Error WindowSurfaceVk::bindTexImage(const gl::Context *context,
                                         gl::Texture *texture,
                                         EGLint buffer)
{
    return egl::NoError();
}

egl::Error WindowSurfaceVk::releaseTexImage(const gl::Context *context, EGLint buffer)
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
    const EGLint minSwapInterval = mState.config->minSwapInterval;
    const EGLint maxSwapInterval = mState.config->maxSwapInterval;
    ASSERT(minSwapInterval == 0 || minSwapInterval == 1);
    ASSERT(maxSwapInterval == 0 || maxSwapInterval == 1);

    interval = gl::clamp(interval, minSwapInterval, maxSwapInterval);

    mDesiredSwapchainPresentMode = GetDesiredPresentMode(mPresentModes, interval);

    // Determine the number of swapchain images:
    //
    // - On mailbox, we use minImageCount.  The drivers may increase the number so that non-blocking
    //   mailbox actually makes sense.
    // - On immediate, we use max(2, minImageCount).  The vkQueuePresentKHR call immediately frees
    //   up the other image, so there is no point in having any more images.
    // - On fifo, we use max(3, minImageCount).  Triple-buffering allows us to present an image,
    //   have one in the queue and record in another.  Note: on certain configurations (windows +
    //   nvidia + windowed mode), we could get away with a smaller number.
    mMinImageCount = mSurfaceCaps.minImageCount;
    if (mDesiredSwapchainPresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
    {
        mMinImageCount = std::max(2u, mMinImageCount);
    }
    else if (mDesiredSwapchainPresentMode == VK_PRESENT_MODE_FIFO_KHR)
    {
        mMinImageCount = std::max(3u, mMinImageCount);
    }

    // Make sure we don't exceed maxImageCount.
    if (mSurfaceCaps.maxImageCount > 0 && mMinImageCount > mSurfaceCaps.maxImageCount)
    {
        mMinImageCount = mSurfaceCaps.maxImageCount;
    }

    // On the next swap, if the desired present mode is different from the current one, the
    // swapchain will be recreated.
}

EGLint WindowSurfaceVk::getWidth() const
{
    return static_cast<EGLint>(mColorRenderTarget.getImageExtents().width);
}

EGLint WindowSurfaceVk::getHeight() const
{
    return static_cast<EGLint>(mColorRenderTarget.getImageExtents().height);
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

angle::Result WindowSurfaceVk::getAttachmentRenderTarget(const gl::Context *context,
                                                         GLenum binding,
                                                         const gl::ImageIndex &imageIndex,
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

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::getCurrentFramebuffer(vk::Context *context,
                                                     const vk::RenderPass &compatibleRenderPass,
                                                     vk::Framebuffer **framebufferOut)
{
    vk::Framebuffer &currentFramebuffer = mSwapchainImages[mCurrentSwapchainImageIndex].framebuffer;

    if (currentFramebuffer.valid())
    {
        // Validation layers should detect if the render pass is really compatible.
        *framebufferOut = &currentFramebuffer;
        return angle::Result::Continue;
    }

    VkFramebufferCreateInfo framebufferInfo = {};

    const gl::Extents &extents            = mColorRenderTarget.getImageExtents();
    std::array<VkImageView, 2> imageViews = {{VK_NULL_HANDLE, mDepthStencilImageView.getHandle()}};

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
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
        ANGLE_VK_TRY(context,
                     swapchainImage.framebuffer.init(context->getDevice(), framebufferInfo));
    }

    ASSERT(currentFramebuffer.valid());
    *framebufferOut = &currentFramebuffer;
    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::initializeContents(const gl::Context *context,
                                                  const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

}  // namespace rx

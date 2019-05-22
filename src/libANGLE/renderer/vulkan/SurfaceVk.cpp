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
GLint GetSampleCount(const egl::Config *config)
{
    GLint samples = 1;
    if (config->sampleBuffers && config->samples > 1)
    {
        samples = config->samples;
    }
    return samples;
}

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

SurfaceVk::SurfaceVk(const egl::SurfaceState &surfaceState) : SurfaceImpl(surfaceState) {}

SurfaceVk::~SurfaceVk() = default;

angle::Result SurfaceVk::getAttachmentRenderTarget(const gl::Context *context,
                                                   GLenum binding,
                                                   const gl::ImageIndex &imageIndex,
                                                   FramebufferAttachmentRenderTarget **rtOut)
{
    ContextVk *contextVk = vk::GetImpl(context);

    if (binding == GL_BACK)
    {
        ANGLE_TRY(mColorRenderTarget.flushStagedUpdates(contextVk));
        *rtOut = &mColorRenderTarget;
    }
    else
    {
        ASSERT(binding == GL_DEPTH || binding == GL_STENCIL || binding == GL_DEPTH_STENCIL);
        ANGLE_TRY(mDepthStencilRenderTarget.flushStagedUpdates(contextVk));
        *rtOut = &mDepthStencilRenderTarget;
    }

    return angle::Result::Continue;
}

OffscreenSurfaceVk::AttachmentImage::AttachmentImage() {}

OffscreenSurfaceVk::AttachmentImage::~AttachmentImage() = default;

angle::Result OffscreenSurfaceVk::AttachmentImage::initialize(DisplayVk *displayVk,
                                                              EGLint width,
                                                              EGLint height,
                                                              const vk::Format &vkFormat,
                                                              GLint samples)
{
    RendererVk *renderer = displayVk->getRenderer();

    const angle::Format &textureFormat = vkFormat.imageFormat();
    bool isDepthOrStencilFormat   = textureFormat.depthBits > 0 || textureFormat.stencilBits > 0;
    const VkImageUsageFlags usage = isDepthOrStencilFormat ? kSurfaceVKDepthStencilImageUsageFlags
                                                           : kSurfaceVKColorImageUsageFlags;

    gl::Extents extents(std::max(static_cast<int>(width), 1), std::max(static_cast<int>(height), 1),
                        1);
    ANGLE_TRY(image.init(displayVk, gl::TextureType::_2D, extents, vkFormat, samples, usage, 1, 1));

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    ANGLE_TRY(image.initMemory(displayVk, renderer->getMemoryProperties(), flags));

    VkImageAspectFlags aspect = vk::GetFormatAspectFlags(textureFormat);

    ANGLE_TRY(image.initImageView(displayVk, gl::TextureType::_2D, aspect, gl::SwizzleState(),
                                  &imageView, 0, 1));

    // Clear the image if it has emulated channels.
    image.stageClearIfEmulatedFormat(gl::ImageIndex::Make2D(0), vkFormat);

    return angle::Result::Continue;
}

void OffscreenSurfaceVk::AttachmentImage::destroy(const egl::Display *display)
{
    DisplayVk *displayVk = vk::GetImpl(display);

    std::vector<vk::GarbageObjectBase> garbageObjects;
    image.releaseImage(displayVk, &garbageObjects);
    image.releaseStagingBuffer(displayVk, &garbageObjects);

    // It should be safe to immediately destroy the backing images of a surface on surface
    // destruction.
    // If this assumption is broken, we could track the last submit fence for the last context that
    // used this surface to garbage collect the surfaces.
    for (vk::GarbageObjectBase &garbage : garbageObjects)
    {
        garbage.destroy(displayVk->getDevice());
    }

    imageView.destroy(displayVk->getDevice());
}

OffscreenSurfaceVk::OffscreenSurfaceVk(const egl::SurfaceState &surfaceState,
                                       EGLint width,
                                       EGLint height)
    : SurfaceVk(surfaceState), mWidth(width), mHeight(height)
{
    mColorRenderTarget.init(&mColorAttachment.image, &mColorAttachment.imageView, nullptr, 0, 0);
    mDepthStencilRenderTarget.init(&mDepthStencilAttachment.image,
                                   &mDepthStencilAttachment.imageView, nullptr, 0, 0);
}

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

    GLint samples = GetSampleCount(mState.config);
    ANGLE_VK_CHECK(displayVk, samples > 0, VK_ERROR_INITIALIZATION_FAILED);

    if (config->renderTargetFormat != GL_NONE)
    {
        ANGLE_TRY(mColorAttachment.initialize(
            displayVk, mWidth, mHeight, renderer->getFormat(config->renderTargetFormat), samples));
    }

    if (config->depthStencilFormat != GL_NONE)
    {
        ANGLE_TRY(mDepthStencilAttachment.initialize(
            displayVk, mWidth, mHeight, renderer->getFormat(config->depthStencilFormat), samples));
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

angle::Result OffscreenSurfaceVk::initializeContents(const gl::Context *context,
                                                     const gl::ImageIndex &imageIndex)
{
    ContextVk *contextVk = vk::GetImpl(context);

    if (mColorAttachment.image.valid())
    {
        mColorAttachment.image.stageSubresourceRobustClear(
            imageIndex, mColorAttachment.image.getFormat().angleFormat());
        ANGLE_TRY(mColorAttachment.image.flushAllStagedUpdates(contextVk));
    }

    if (mDepthStencilAttachment.image.valid())
    {
        mDepthStencilAttachment.image.stageSubresourceRobustClear(
            imageIndex, mDepthStencilAttachment.image.getFormat().angleFormat());
        ANGLE_TRY(mDepthStencilAttachment.image.flushAllStagedUpdates(contextVk));
    }
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

WindowSurfaceVk::SwapHistory::SwapHistory() = default;
WindowSurfaceVk::SwapHistory::SwapHistory(SwapHistory &&other)
{
    *this = std::move(other);
}

WindowSurfaceVk::SwapHistory &WindowSurfaceVk::SwapHistory::operator=(SwapHistory &&other)
{
    std::swap(sharedFence, other.sharedFence);
    std::swap(semaphores, other.semaphores);
    std::swap(swapchain, other.swapchain);
    return *this;
}

WindowSurfaceVk::SwapHistory::~SwapHistory() = default;

void WindowSurfaceVk::SwapHistory::destroy(VkDevice device)
{
    if (swapchain != VK_NULL_HANDLE)
    {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
        swapchain = VK_NULL_HANDLE;
    }

    sharedFence.reset(device);

    for (vk::Semaphore &semaphore : semaphores)
    {
        semaphore.destroy(device);
    }
    semaphores.clear();
}

angle::Result WindowSurfaceVk::SwapHistory::waitFence(ContextVk *contextVk)
{
    if (sharedFence.isReferenced())
    {
        ANGLE_VK_TRY(contextVk, sharedFence.get().wait(contextVk->getDevice(),
                                                       std::numeric_limits<uint64_t>::max()));
    }
    return angle::Result::Continue;
}

WindowSurfaceVk::WindowSurfaceVk(const egl::SurfaceState &surfaceState,
                                 EGLNativeWindowType window,
                                 EGLint width,
                                 EGLint height)
    : SurfaceVk(surfaceState),
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
    // Initialize the color render target with the multisampled targets.  If not multisampled, the
    // render target will be updated to refer to a swapchain image on every acquire.
    mColorRenderTarget.init(&mColorImageMS, &mColorImageViewMS, nullptr, 0, 0);
    mDepthStencilRenderTarget.init(&mDepthStencilImage, &mDepthStencilImageView, nullptr, 0, 0);
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
    (void)renderer->queueWaitIdle(displayVk);

    destroySwapChainImages(displayVk);

    for (SwapHistory &swap : mSwapHistory)
    {
        swap.destroy(device);
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

    for (vk::Semaphore &flushSemaphore : mFlushSemaphoreChain)
    {
        flushSemaphore.destroy(device);
    }
    mFlushSemaphoreChain.clear();
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
                   (mSurfaceCaps.supportedUsageFlags & kSurfaceVKColorImageUsageFlags) ==
                       kSurfaceVKColorImageUsageFlags,
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
    setSwapInterval(renderer->getFeatures().disableFifoPresentMode.enabled ? 0 : 1);

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
    VkFormat nativeFormat    = format.vkImageFormat;

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

    ANGLE_TRY(createSwapChain(displayVk, extents, VK_NULL_HANDLE));

    return nextSwapchainImage(displayVk);
}

angle::Result WindowSurfaceVk::recreateSwapchain(ContextVk *contextVk,
                                                 const gl::Extents &extents,
                                                 uint32_t swapHistoryIndex)
{
    VkSwapchainKHR oldSwapchain = mSwapchain;
    mSwapchain                  = VK_NULL_HANDLE;

    if (oldSwapchain)
    {
        // Note: the old swapchain must be destroyed regardless of whether creating the new
        // swapchain succeeds.  We can only destroy the swapchain once rendering to all its images
        // have finished.  We therefore store the handle to the swapchain being destroyed in the
        // swap history (alongside the serial of the last submission) so it can be destroyed once we
        // wait on that serial as part of the CPU throttling.
        mSwapHistory[swapHistoryIndex].swapchain = oldSwapchain;
    }

    releaseSwapchainImages(contextVk);

    return createSwapChain(contextVk, extents, oldSwapchain);
}

angle::Result WindowSurfaceVk::createSwapChain(vk::Context *context,
                                               const gl::Extents &extents,
                                               VkSwapchainKHR oldSwapchain)
{
    ASSERT(mSwapchain == VK_NULL_HANDLE);

    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    const vk::Format &format = renderer->getFormat(mState.config->renderTargetFormat);
    VkFormat nativeFormat    = format.vkImageFormat;

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
    swapchainInfo.imageExtent.width     = std::max(extents.width, 1);
    swapchainInfo.imageExtent.height    = std::max(extents.height, 1);
    swapchainInfo.imageArrayLayers      = 1;
    swapchainInfo.imageUsage            = kImageUsageFlags;
    swapchainInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchainInfo.queueFamilyIndexCount = 0;
    swapchainInfo.pQueueFamilyIndices   = nullptr;
    swapchainInfo.preTransform          = mPreTransform;
    swapchainInfo.compositeAlpha        = mCompositeAlpha;
    swapchainInfo.presentMode           = mDesiredSwapchainPresentMode;
    swapchainInfo.clipped               = VK_TRUE;
    swapchainInfo.oldSwapchain          = oldSwapchain;

    // TODO(syoussefi): Once EGL_SWAP_BEHAVIOR_PRESERVED_BIT is supported, the contents of the old
    // swapchain need to carry over to the new one.  http://anglebug.com/2942
    ANGLE_VK_TRY(context, vkCreateSwapchainKHR(device, &swapchainInfo, nullptr, &mSwapchain));
    mSwapchainPresentMode = mDesiredSwapchainPresentMode;

    // Intialize the swapchain image views.
    uint32_t imageCount = 0;
    ANGLE_VK_TRY(context, vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, nullptr));

    std::vector<VkImage> swapchainImages(imageCount);
    ANGLE_VK_TRY(context,
                 vkGetSwapchainImagesKHR(device, mSwapchain, &imageCount, swapchainImages.data()));

    // If multisampling is enabled, create a multisampled image which gets resolved just prior to
    // present.
    GLint samples = GetSampleCount(mState.config);
    ANGLE_VK_CHECK(context, samples > 0, VK_ERROR_INITIALIZATION_FAILED);

    if (samples > 1)
    {
        const VkImageUsageFlags usage = kSurfaceVKColorImageUsageFlags;

        ANGLE_TRY(mColorImageMS.init(context, gl::TextureType::_2D, extents, format, samples, usage,
                                     1, 1));
        ANGLE_TRY(mColorImageMS.initMemory(context, renderer->getMemoryProperties(),
                                           VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        ANGLE_TRY(mColorImageMS.initImageView(context, gl::TextureType::_2D,
                                              VK_IMAGE_ASPECT_COLOR_BIT, gl::SwizzleState(),
                                              &mColorImageViewMS, 0, 1));

        // Clear the image if it has emulated channels.
        mColorImageMS.stageClearIfEmulatedFormat(gl::ImageIndex::Make2D(0), format);
    }

    mSwapchainImages.resize(imageCount);

    for (uint32_t imageIndex = 0; imageIndex < imageCount; ++imageIndex)
    {
        SwapchainImage &member = mSwapchainImages[imageIndex];
        member.image.init2DWeakReference(swapchainImages[imageIndex], extents, format, 1);

        if (!mColorImageMS.valid())
        {
            // If the multisampled image is used, we don't need a view on the swapchain image, as
            // it's only used as a resolve destination.  This has the added benefit that we can't
            // accidentally use this image.
            ANGLE_TRY(member.image.initImageView(context, gl::TextureType::_2D,
                                                 VK_IMAGE_ASPECT_COLOR_BIT, gl::SwizzleState(),
                                                 &member.imageView, 0, 1));

            // Clear the image if it has emulated channels.  If a multisampled image exists, this
            // image will be unused until a pre-present resolve, at which point it will be fully
            // initialized and wouldn't need a clear.
            member.image.stageClearIfEmulatedFormat(gl::ImageIndex::Make2D(0), format);
        }
    }

    // Initialize depth/stencil if requested.
    if (mState.config->depthStencilFormat != GL_NONE)
    {
        const vk::Format &dsFormat = renderer->getFormat(mState.config->depthStencilFormat);

        const VkImageUsageFlags dsUsage = kSurfaceVKDepthStencilImageUsageFlags;

        ANGLE_TRY(mDepthStencilImage.init(context, gl::TextureType::_2D, extents, dsFormat, samples,
                                          dsUsage, 1, 1));
        ANGLE_TRY(mDepthStencilImage.initMemory(context, renderer->getMemoryProperties(),
                                                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT));

        const VkImageAspectFlags aspect = vk::GetDepthStencilAspectFlags(dsFormat.imageFormat());

        ANGLE_TRY(mDepthStencilImage.initImageView(context, gl::TextureType::_2D, aspect,
                                                   gl::SwizzleState(), &mDepthStencilImageView, 0,
                                                   1));

        // We will need to pass depth/stencil image views to the RenderTargetVk in the future.

        // Clear the image if it has emulated channels.
        mDepthStencilImage.stageClearIfEmulatedFormat(gl::ImageIndex::Make2D(0), dsFormat);
    }

    return angle::Result::Continue;
}

bool WindowSurfaceVk::isMultiSampled() const
{
    return mColorImageMS.valid();
}

angle::Result WindowSurfaceVk::checkForOutOfDateSwapchain(ContextVk *contextVk,
                                                          uint32_t swapHistoryIndex,
                                                          bool presentOutOfDate)
{
    bool swapIntervalChanged = mSwapchainPresentMode != mDesiredSwapchainPresentMode;

    // Check for window resize and recreate swapchain if necessary.
    gl::Extents currentExtents;
    ANGLE_TRY(getCurrentWindowSize(contextVk, &currentExtents));

    gl::Extents swapchainExtents(getWidth(), getHeight(), 0);

    // If window size has changed, check with surface capabilities.  It has been observed on
    // Android that `getCurrentWindowSize()` returns 1920x1080 for example, while surface
    // capabilities returns the size the surface was created with.
    if (currentExtents != swapchainExtents)
    {
        const VkPhysicalDevice &physicalDevice = contextVk->getRenderer()->getPhysicalDevice();
        ANGLE_VK_TRY(contextVk, vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, mSurface,
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
        ANGLE_TRY(recreateSwapchain(contextVk, currentExtents, swapHistoryIndex));
    }

    return angle::Result::Continue;
}

void WindowSurfaceVk::releaseSwapchainImages(ContextVk *contextVk)
{
    if (mDepthStencilImage.valid())
    {
        Serial depthStencilSerial = mDepthStencilImage.getStoredQueueSerial();
        mDepthStencilImage.releaseImage(contextVk);
        mDepthStencilImage.releaseStagingBuffer(contextVk);

        if (mDepthStencilImageView.valid())
        {
            contextVk->releaseObject(depthStencilSerial, &mDepthStencilImageView);
        }
    }

    if (mColorImageMS.valid())
    {
        Serial serial = mColorImageMS.getStoredQueueSerial();
        mColorImageMS.releaseImage(contextVk);
        mColorImageMS.releaseStagingBuffer(contextVk);

        contextVk->releaseObject(serial, &mColorImageViewMS);
        contextVk->releaseObject(serial, &mFramebufferMS);
    }

    for (SwapchainImage &swapchainImage : mSwapchainImages)
    {
        Serial imageSerial = swapchainImage.image.getStoredQueueSerial();

        // We don't own the swapchain image handles, so we just remove our reference to it.
        swapchainImage.image.resetImageWeakReference();
        swapchainImage.image.destroy(contextVk->getDevice());

        if (swapchainImage.imageView.valid())
        {
            contextVk->releaseObject(imageSerial, &swapchainImage.imageView);
        }

        if (swapchainImage.framebuffer.valid())
        {
            contextVk->releaseObject(imageSerial, &swapchainImage.framebuffer);
        }
    }

    mSwapchainImages.clear();
}

void WindowSurfaceVk::destroySwapChainImages(DisplayVk *displayVk)
{
    std::vector<vk::GarbageObjectBase> garbageObjects;
    if (mDepthStencilImage.valid())
    {
        mDepthStencilImage.releaseImage(displayVk, &garbageObjects);
        mDepthStencilImage.releaseStagingBuffer(displayVk, &garbageObjects);

        if (mDepthStencilImageView.valid())
        {
            mDepthStencilImageView.dumpResources(&garbageObjects);
        }
    }

    if (mColorImageMS.valid())
    {
        mColorImageMS.releaseImage(displayVk, &garbageObjects);
        mColorImageMS.releaseImage(displayVk, &garbageObjects);

        mColorImageViewMS.dumpResources(&garbageObjects);
        mFramebufferMS.dumpResources(&garbageObjects);
    }

    for (vk::GarbageObjectBase &garbage : garbageObjects)
    {
        garbage.destroy(displayVk->getDevice());
    }

    for (SwapchainImage &swapchainImage : mSwapchainImages)
    {
        // We don't own the swapchain image handles, so we just remove our reference to it.
        swapchainImage.image.resetImageWeakReference();
        swapchainImage.image.destroy(displayVk->getDevice());

        if (swapchainImage.imageView.valid())
        {
            swapchainImage.imageView.destroy(displayVk->getDevice());
        }

        if (swapchainImage.framebuffer.valid())
        {
            swapchainImage.framebuffer.destroy(displayVk->getDevice());
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
    DisplayVk *displayVk = vk::GetImpl(context->getDisplay());
    angle::Result result = swapImpl(context, rects, n_rects);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

egl::Error WindowSurfaceVk::swap(const gl::Context *context)
{
    DisplayVk *displayVk = vk::GetImpl(context->getDisplay());
    angle::Result result = swapImpl(context, nullptr, 0);
    return angle::ToEGL(result, displayVk, EGL_BAD_SURFACE);
}

angle::Result WindowSurfaceVk::present(ContextVk *contextVk,
                                       EGLint *rects,
                                       EGLint n_rects,
                                       bool &swapchainOutOfDate)
{
    // Throttle the submissions to avoid getting too far ahead of the GPU.
    SwapHistory &swap = mSwapHistory[mCurrentSwapHistoryIndex];
    {
        TRACE_EVENT0("gpu.angle", "WindowSurfaceVk::present: Throttle CPU");
        ANGLE_TRY(swap.waitFence(contextVk));
        swap.destroy(contextVk->getDevice());
    }

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    vk::CommandBuffer *swapCommands = nullptr;
    if (mColorImageMS.valid())
    {
        // Transition the multisampled image to TRANSFER_SRC for resolve.
        vk::CommandBuffer *multisampledTransition = nullptr;
        ANGLE_TRY(mColorImageMS.recordCommands(contextVk, &multisampledTransition));

        mColorImageMS.changeLayout(VK_IMAGE_ASPECT_COLOR_BIT, vk::ImageLayout::TransferSrc,
                                   multisampledTransition);

        // Setup graph dependency between the swapchain image and the multisampled one.
        image.image.addReadDependency(&mColorImageMS);

        VkImageResolve resolveRegion                = {};
        resolveRegion.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        resolveRegion.srcSubresource.mipLevel       = 0;
        resolveRegion.srcSubresource.baseArrayLayer = 0;
        resolveRegion.srcSubresource.layerCount     = 1;
        resolveRegion.srcOffset                     = {};
        resolveRegion.dstSubresource                = resolveRegion.srcSubresource;
        resolveRegion.dstOffset                     = {};
        gl_vk::GetExtent(image.image.getExtents(), &resolveRegion.extent);

        ANGLE_TRY(image.image.recordCommands(contextVk, &swapCommands));
        mColorImageMS.resolve(&image.image, resolveRegion, swapCommands);
    }
    else
    {
        ANGLE_TRY(image.image.recordCommands(contextVk, &swapCommands));
    }
    image.image.changeLayout(VK_IMAGE_ASPECT_COLOR_BIT, vk::ImageLayout::Present, swapCommands);

    ANGLE_TRY(contextVk->flushImpl(nullptr));

    // The semaphore chain must at least have the semaphore returned by vkAquireImage in it. It will
    // likely have more based on how much work was flushed this frame.
    ASSERT(!mFlushSemaphoreChain.empty());

    VkPresentInfoKHR presentInfo   = {};
    presentInfo.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = mFlushSemaphoreChain.back().ptr();
    presentInfo.swapchainCount     = 1;
    presentInfo.pSwapchains        = &mSwapchain;
    presentInfo.pImageIndices      = &mCurrentSwapchainImageIndex;
    presentInfo.pResults           = nullptr;

    VkPresentRegionKHR presentRegion   = {};
    VkPresentRegionsKHR presentRegions = {};
    std::vector<VkRectLayerKHR> vkRects;
    if (contextVk->getFeatures().supportsIncrementalPresent.enabled && (n_rects > 0))
    {
        EGLint width  = getWidth();
        EGLint height = getHeight();

        EGLint *eglRects             = rects;
        presentRegion.rectangleCount = n_rects;
        vkRects.resize(n_rects);
        for (EGLint i = 0; i < n_rects; i++)
        {
            VkRectLayerKHR &rect = vkRects[i];

            // Make sure the damage rects are within swapchain bounds.
            rect.offset.x      = gl::clamp(*eglRects++, 0, width);
            rect.offset.y      = gl::clamp(*eglRects++, 0, height);
            rect.extent.width  = gl::clamp(*eglRects++, 0, width - rect.offset.x);
            rect.extent.height = gl::clamp(*eglRects++, 0, height - rect.offset.y);
            rect.layer         = 0;
        }
        presentRegion.pRectangles = vkRects.data();

        presentRegions.sType          = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
        presentRegions.pNext          = nullptr;
        presentRegions.swapchainCount = 1;
        presentRegions.pRegions       = &presentRegion;

        presentInfo.pNext = &presentRegions;
    }

    // Update the swap history for this presentation
    swap.sharedFence = contextVk->getLastSubmittedFence();
    swap.semaphores  = std::move(mFlushSemaphoreChain);

    ++mCurrentSwapHistoryIndex;
    mCurrentSwapHistoryIndex =
        mCurrentSwapHistoryIndex == mSwapHistory.size() ? 0 : mCurrentSwapHistoryIndex;

    VkResult result = contextVk->getRenderer()->queuePresent(presentInfo);

    // If SUBOPTIMAL/OUT_OF_DATE is returned, it's ok, we just need to recreate the swapchain before
    // continuing.
    swapchainOutOfDate = result == VK_SUBOPTIMAL_KHR || result == VK_ERROR_OUT_OF_DATE_KHR;
    if (!swapchainOutOfDate)
    {
        ANGLE_VK_TRY(contextVk, result);
    }

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::swapImpl(const gl::Context *context, EGLint *rects, EGLint n_rects)
{
    ContextVk *contextVk = vk::GetImpl(context);
    DisplayVk *displayVk = vk::GetImpl(context->getDisplay());

    bool swapchainOutOfDate;
    // Save this now, since present() will increment the value.
    size_t currentSwapHistoryIndex = mCurrentSwapHistoryIndex;

    ANGLE_TRY(present(contextVk, rects, n_rects, swapchainOutOfDate));

    ANGLE_TRY(checkForOutOfDateSwapchain(contextVk, currentSwapHistoryIndex, swapchainOutOfDate));

    {
        // Note: TRACE_EVENT0 is put here instead of inside the function to workaround this issue:
        // http://anglebug.com/2927
        TRACE_EVENT0("gpu.angle", "nextSwapchainImage");
        // Get the next available swapchain image.
        ANGLE_TRY(nextSwapchainImage(contextVk));
    }

    RendererVk *renderer = contextVk->getRenderer();
    ANGLE_TRY(renderer->syncPipelineCacheVk(displayVk));

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::nextSwapchainImage(vk::Context *context)
{
    VkDevice device = context->getDevice();

    vk::Scoped<vk::Semaphore> aquireImageSemaphore(device);
    ANGLE_VK_TRY(context, aquireImageSemaphore.get().init(device));

    ANGLE_VK_TRY(context, vkAcquireNextImageKHR(device, mSwapchain, UINT64_MAX,
                                                aquireImageSemaphore.get().getHandle(),
                                                VK_NULL_HANDLE, &mCurrentSwapchainImageIndex));

    // After presenting, the flush semaphore chain is cleared. The semaphore returned by
    // vkAcquireNextImage will start a new chain.
    ASSERT(mFlushSemaphoreChain.empty());
    mFlushSemaphoreChain.push_back(aquireImageSemaphore.release());

    SwapchainImage &image = mSwapchainImages[mCurrentSwapchainImageIndex];

    // This swap chain image is new, reset any dependency information it has.
    //
    // When the Vulkan backend is multithreading, different contexts can have very different current
    // serials.  If a surface is rendered to by multiple contexts in different frames, the last
    // context to write a particular swap chain image has no bearing on the current context writing
    // to that image.
    //
    // Clear the image's queue serial because it's possible that it appears to be in the future to
    // the next context that writes to the image.
    image.image.resetQueueSerial();

    // Update RenderTarget pointers to this swapchain image if not multisampling.  Note: a possible
    // optimization is to defer the |vkAcquireNextImageKHR| call itself to |present()| if
    // multisampling, as the swapchain image is essentially unused until then.
    if (!mColorImageMS.valid())
    {
        mColorRenderTarget.updateSwapchainImage(&image.image, &image.imageView);
    }

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

    // - On mailbox, we need at least three images; one is being displayed to the user until the
    //   next v-sync, and the application alternatingly renders to the other two, one being
    //   recorded, and the other queued for presentation if v-sync happens in the meantime.
    // - On immediate, we need at least two images; the application alternates between the two
    //   images.
    // - On fifo, we use at least three images.  Triple-buffering allows us to present an image,
    //   have one in the queue, and record in another.  Note: on certain configurations (windows +
    //   nvidia + windowed mode), we could get away with a smaller number.
    //
    // For simplicity, we always allocate at least three images.
    mMinImageCount = std::max(3u, mSurfaceCaps.minImageCount);

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
    return static_cast<EGLint>(mColorRenderTarget.getExtents().width);
}

EGLint WindowSurfaceVk::getHeight() const
{
    return static_cast<EGLint>(mColorRenderTarget.getExtents().height);
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

angle::Result WindowSurfaceVk::getCurrentFramebuffer(vk::Context *context,
                                                     const vk::RenderPass &compatibleRenderPass,
                                                     vk::Framebuffer **framebufferOut)
{
    vk::Framebuffer &currentFramebuffer =
        isMultiSampled() ? mFramebufferMS
                         : mSwapchainImages[mCurrentSwapchainImageIndex].framebuffer;

    if (currentFramebuffer.valid())
    {
        // Validation layers should detect if the render pass is really compatible.
        *framebufferOut = &currentFramebuffer;
        return angle::Result::Continue;
    }

    VkFramebufferCreateInfo framebufferInfo = {};

    const gl::Extents extents             = mColorRenderTarget.getExtents();
    std::array<VkImageView, 2> imageViews = {{VK_NULL_HANDLE, mDepthStencilImageView.getHandle()}};

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = compatibleRenderPass.getHandle();
    framebufferInfo.attachmentCount = (mDepthStencilImage.valid() ? 2u : 1u);
    framebufferInfo.pAttachments    = imageViews.data();
    framebufferInfo.width           = static_cast<uint32_t>(extents.width);
    framebufferInfo.height          = static_cast<uint32_t>(extents.height);
    framebufferInfo.layers          = 1;

    if (isMultiSampled())
    {
        // If multisampled, there is only a single color image and framebuffer.
        imageViews[0] = mColorImageViewMS.getHandle();
        ANGLE_VK_TRY(context, mFramebufferMS.init(context->getDevice(), framebufferInfo));
    }
    else
    {
        for (SwapchainImage &swapchainImage : mSwapchainImages)
        {
            imageViews[0] = swapchainImage.imageView.getHandle();
            ANGLE_VK_TRY(context,
                         swapchainImage.framebuffer.init(context->getDevice(), framebufferInfo));
        }
    }

    ASSERT(currentFramebuffer.valid());
    *framebufferOut = &currentFramebuffer;
    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::generateSemaphoresForFlush(vk::Context *context,
                                                          const vk::Semaphore **outWaitSemaphore,
                                                          const vk::Semaphore **outSignalSempahore)
{
    // The flush semaphore chain should always start with a semaphore in it, created by the
    // vkAquireImage call. This semaphore must be waited on before any rendering to the swap chain
    // image can occur.
    ASSERT(!mFlushSemaphoreChain.empty());

    vk::Semaphore nextSemaphore;
    ANGLE_VK_TRY(context, nextSemaphore.init(context->getDevice()));
    mFlushSemaphoreChain.push_back(std::move(nextSemaphore));

    *outWaitSemaphore   = &mFlushSemaphoreChain[mFlushSemaphoreChain.size() - 2];
    *outSignalSempahore = &mFlushSemaphoreChain[mFlushSemaphoreChain.size() - 1];

    return angle::Result::Continue;
}

angle::Result WindowSurfaceVk::initializeContents(const gl::Context *context,
                                                  const gl::ImageIndex &imageIndex)
{
    ContextVk *contextVk = vk::GetImpl(context);

    ASSERT(mSwapchainImages.size() > 0);
    ASSERT(mCurrentSwapchainImageIndex < mSwapchainImages.size());

    vk::ImageHelper *image =
        isMultiSampled() ? &mColorImageMS : &mSwapchainImages[mCurrentSwapchainImageIndex].image;
    image->stageSubresourceRobustClear(imageIndex, image->getFormat().angleFormat());
    ANGLE_TRY(image->flushAllStagedUpdates(contextVk));

    if (mDepthStencilImage.valid())
    {
        mDepthStencilImage.stageSubresourceRobustClear(
            gl::ImageIndex::Make2D(0), mDepthStencilImage.getFormat().angleFormat());
        ANGLE_TRY(mDepthStencilImage.flushAllStagedUpdates(contextVk));
    }

    return angle::Result::Continue;
}

}  // namespace rx

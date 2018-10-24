//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SurfaceVk.h:
//    Defines the class interface for SurfaceVk, implementing SurfaceImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_SURFACEVK_H_
#define LIBANGLE_RENDERER_VULKAN_SURFACEVK_H_

#include <vulkan/vulkan.h>

#include "libANGLE/renderer/SurfaceImpl.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
class RendererVk;

class OffscreenSurfaceVk : public SurfaceImpl
{
  public:
    OffscreenSurfaceVk(const egl::SurfaceState &surfaceState, EGLint width, EGLint height);
    ~OffscreenSurfaceVk() override;

    egl::Error initialize(const egl::Display *display) override;
    void destroy(const egl::Display *display) override;

    FramebufferImpl *createDefaultFramebuffer(const gl::Context *context,
                                              const gl::FramebufferState &state) override;
    egl::Error swap(const gl::Context *context) override;
    egl::Error postSubBuffer(const gl::Context *context,
                             EGLint x,
                             EGLint y,
                             EGLint width,
                             EGLint height) override;
    egl::Error querySurfacePointerANGLE(EGLint attribute, void **value) override;
    egl::Error bindTexImage(const gl::Context *context,
                            gl::Texture *texture,
                            EGLint buffer) override;
    egl::Error releaseTexImage(const gl::Context *context, EGLint buffer) override;
    egl::Error getSyncValues(EGLuint64KHR *ust, EGLuint64KHR *msc, EGLuint64KHR *sbc) override;
    void setSwapInterval(EGLint interval) override;

    // width and height can change with client window resizing
    EGLint getWidth() const override;
    EGLint getHeight() const override;

    EGLint isPostSubBufferSupported() const override;
    EGLint getSwapBehavior() const override;

    angle::Result getAttachmentRenderTarget(const gl::Context *context,
                                            GLenum binding,
                                            const gl::ImageIndex &imageIndex,
                                            FramebufferAttachmentRenderTarget **rtOut) override;

    angle::Result initializeContents(const gl::Context *context,
                                     const gl::ImageIndex &imageIndex) override;

  private:
    struct AttachmentImage final : angle::NonCopyable
    {
        AttachmentImage();
        ~AttachmentImage();

        angle::Result initialize(DisplayVk *displayVk,
                                 EGLint width,
                                 EGLint height,
                                 const vk::Format &vkFormat);
        void destroy(const egl::Display *display);

        vk::ImageHelper image;
        vk::ImageView imageView;
        RenderTargetVk renderTarget;
    };

    angle::Result initializeImpl(DisplayVk *displayVk);

    EGLint mWidth;
    EGLint mHeight;

    AttachmentImage mColorAttachment;
    AttachmentImage mDepthStencilAttachment;
};

class WindowSurfaceVk : public SurfaceImpl
{
  public:
    WindowSurfaceVk(const egl::SurfaceState &surfaceState,
                    EGLNativeWindowType window,
                    EGLint width,
                    EGLint height);
    ~WindowSurfaceVk() override;

    void destroy(const egl::Display *display) override;

    egl::Error initialize(const egl::Display *display) override;
    FramebufferImpl *createDefaultFramebuffer(const gl::Context *context,
                                              const gl::FramebufferState &state) override;
    egl::Error swap(const gl::Context *context) override;
    egl::Error swapWithDamage(const gl::Context *context, EGLint *rects, EGLint n_rects) override;
    egl::Error postSubBuffer(const gl::Context *context,
                             EGLint x,
                             EGLint y,
                             EGLint width,
                             EGLint height) override;
    egl::Error querySurfacePointerANGLE(EGLint attribute, void **value) override;
    egl::Error bindTexImage(const gl::Context *context,
                            gl::Texture *texture,
                            EGLint buffer) override;
    egl::Error releaseTexImage(const gl::Context *context, EGLint buffer) override;
    egl::Error getSyncValues(EGLuint64KHR *ust, EGLuint64KHR *msc, EGLuint64KHR *sbc) override;
    void setSwapInterval(EGLint interval) override;

    // width and height can change with client window resizing
    EGLint getWidth() const override;
    EGLint getHeight() const override;

    EGLint isPostSubBufferSupported() const override;
    EGLint getSwapBehavior() const override;

    angle::Result getAttachmentRenderTarget(const gl::Context *context,
                                            GLenum binding,
                                            const gl::ImageIndex &imageIndex,
                                            FramebufferAttachmentRenderTarget **rtOut) override;

    angle::Result initializeContents(const gl::Context *context,
                                     const gl::ImageIndex &imageIndex) override;

    angle::Result getCurrentFramebuffer(vk::Context *context,
                                        const vk::RenderPass &compatibleRenderPass,
                                        vk::Framebuffer **framebufferOut);

  protected:
    EGLNativeWindowType mNativeWindowType;
    VkSurfaceKHR mSurface;
    VkInstance mInstance;

  private:
    virtual angle::Result createSurfaceVk(vk::Context *context, gl::Extents *extentsOut) = 0;
    angle::Result initializeImpl(DisplayVk *displayVk);
    angle::Result nextSwapchainImage(DisplayVk *displayVk);
    angle::Result swapImpl(DisplayVk *displayVk);

    VkSwapchainKHR mSwapchain;
    VkPresentModeKHR mSwapchainPresentMode;

    RenderTargetVk mColorRenderTarget;
    RenderTargetVk mDepthStencilRenderTarget;

    uint32_t mCurrentSwapchainImageIndex;

    struct SwapchainImage : angle::NonCopyable
    {
        SwapchainImage();
        SwapchainImage(SwapchainImage &&other);
        ~SwapchainImage();

        vk::ImageHelper image;
        vk::ImageView imageView;
        vk::Framebuffer framebuffer;
    };

    std::vector<SwapchainImage> mSwapchainImages;

    // A circular buffer, with the same size as mSwapchainImages (N), that stores the serial of the
    // renderer on every swap.  In FIFO present modes, the CPU is throttled by waiting for the
    // Nth previous serial to finish.
    std::vector<Serial> mSwapSerials;
    size_t mCurrentSwapSerialIndex;

    vk::ImageHelper mDepthStencilImage;
    vk::ImageView mDepthStencilImageView;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_SURFACEVK_H_

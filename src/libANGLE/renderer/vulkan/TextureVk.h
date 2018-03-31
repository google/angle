//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TextureVk.h:
//    Defines the class interface for TextureVk, implementing TextureImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_
#define LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_

#include "libANGLE/renderer/TextureImpl.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{

class StagingStorage final : angle::NonCopyable
{
  public:
    StagingStorage();
    ~StagingStorage();

    void release(RendererVk *renderer);

    gl::Error stageSubresourceUpdate(ContextVk *contextVk,
                                     const gl::Extents &extents,
                                     const gl::InternalFormat &formatInfo,
                                     const gl::PixelUnpackState &unpack,
                                     GLenum type,
                                     const uint8_t *pixels);

    vk::Error flushUpdatesToImage(RendererVk *renderer,
                                  vk::ImageHelper *image,
                                  vk::CommandBuffer *commandBuffer);

  private:
    vk::DynamicBuffer mStagingBuffer;
    VkBuffer mCurrentBufferHandle;
    VkBufferImageCopy mCurrentCopyRegion;
};

class TextureVk : public TextureImpl, public vk::CommandGraphResource
{
  public:
    TextureVk(const gl::TextureState &state);
    ~TextureVk() override;
    gl::Error onDestroy(const gl::Context *context) override;

    gl::Error setImage(const gl::Context *context,
                       const gl::ImageIndex &index,
                       GLenum internalFormat,
                       const gl::Extents &size,
                       GLenum format,
                       GLenum type,
                       const gl::PixelUnpackState &unpack,
                       const uint8_t *pixels) override;
    gl::Error setSubImage(const gl::Context *context,
                          const gl::ImageIndex &index,
                          const gl::Box &area,
                          GLenum format,
                          GLenum type,
                          const gl::PixelUnpackState &unpack,
                          const uint8_t *pixels) override;

    gl::Error setCompressedImage(const gl::Context *context,
                                 const gl::ImageIndex &index,
                                 GLenum internalFormat,
                                 const gl::Extents &size,
                                 const gl::PixelUnpackState &unpack,
                                 size_t imageSize,
                                 const uint8_t *pixels) override;
    gl::Error setCompressedSubImage(const gl::Context *context,
                                    const gl::ImageIndex &index,
                                    const gl::Box &area,
                                    GLenum format,
                                    const gl::PixelUnpackState &unpack,
                                    size_t imageSize,
                                    const uint8_t *pixels) override;

    gl::Error copyImage(const gl::Context *context,
                        const gl::ImageIndex &index,
                        const gl::Rectangle &sourceArea,
                        GLenum internalFormat,
                        gl::Framebuffer *source) override;
    gl::Error copySubImage(const gl::Context *context,
                           const gl::ImageIndex &index,
                           const gl::Offset &destOffset,
                           const gl::Rectangle &sourceArea,
                           gl::Framebuffer *source) override;

    gl::Error setStorage(const gl::Context *context,
                         gl::TextureType type,
                         size_t levels,
                         GLenum internalFormat,
                         const gl::Extents &size) override;

    gl::Error setEGLImageTarget(const gl::Context *context,
                                gl::TextureType type,
                                egl::Image *image) override;

    gl::Error setImageExternal(const gl::Context *context,
                               gl::TextureType type,
                               egl::Stream *stream,
                               const egl::Stream::GLTextureDescription &desc) override;

    gl::Error generateMipmap(const gl::Context *context) override;

    gl::Error setBaseLevel(const gl::Context *context, GLuint baseLevel) override;

    gl::Error bindTexImage(const gl::Context *context, egl::Surface *surface) override;
    gl::Error releaseTexImage(const gl::Context *context) override;

    gl::Error getAttachmentRenderTarget(const gl::Context *context,
                                        GLenum binding,
                                        const gl::ImageIndex &imageIndex,
                                        FramebufferAttachmentRenderTarget **rtOut) override;

    void syncState(const gl::Texture::DirtyBits &dirtyBits) override;

    gl::Error setStorageMultisample(const gl::Context *context,
                                    gl::TextureType type,
                                    GLsizei samples,
                                    GLint internalformat,
                                    const gl::Extents &size,
                                    bool fixedSampleLocations) override;

    gl::Error initializeContents(const gl::Context *context,
                                 const gl::ImageIndex &imageIndex) override;

    const vk::ImageHelper &getImage() const;
    const vk::ImageView &getImageView() const;
    const vk::Sampler &getSampler() const;

    vk::Error ensureImageInitialized(RendererVk *renderer);

  private:
    void releaseImage(const gl::Context *context, RendererVk *renderer);

    vk::ImageHelper mImage;
    vk::ImageView mImageView;
    vk::Sampler mSampler;

    RenderTargetVk mRenderTarget;

    StagingStorage mStagingStorage;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_

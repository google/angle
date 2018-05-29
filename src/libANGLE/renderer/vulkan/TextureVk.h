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

class PixelBuffer final : angle::NonCopyable
{
  public:
    PixelBuffer(RendererVk *renderer);
    ~PixelBuffer();

    void release(RendererVk *renderer);

    gl::Error stageSubresourceUpdate(ContextVk *contextVk,
                                     const gl::ImageIndex &index,
                                     const gl::Extents &extents,
                                     const gl::Offset &offset,
                                     const gl::InternalFormat &formatInfo,
                                     const gl::PixelUnpackState &unpack,
                                     GLenum type,
                                     const uint8_t *pixels);

    gl::Error stageSubresourceUpdateFromFramebuffer(const gl::Context *context,
                                                    const gl::ImageIndex &index,
                                                    const gl::Rectangle &sourceArea,
                                                    const gl::Offset &dstOffset,
                                                    const gl::Extents &dstExtent,
                                                    const gl::InternalFormat &formatInfo,
                                                    FramebufferVk *framebufferVk);

    vk::Error flushUpdatesToImage(RendererVk *renderer,
                                  vk::ImageHelper *image,
                                  vk::CommandBuffer *commandBuffer);

    bool empty() const;

  private:
    struct SubresourceUpdate
    {
        SubresourceUpdate();
        SubresourceUpdate(VkBuffer bufferHandle, const VkBufferImageCopy &copyRegion);
        SubresourceUpdate(const SubresourceUpdate &other);

        VkBuffer bufferHandle;
        VkBufferImageCopy copyRegion;
    };

    vk::DynamicBuffer mStagingBuffer;
    std::vector<SubresourceUpdate> mSubresourceUpdates;
};

class TextureVk : public TextureImpl, public vk::CommandGraphResource
{
  public:
    TextureVk(const gl::TextureState &state, RendererVk *renderer);
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

    gl::Error syncState(const gl::Context *context,
                        const gl::Texture::DirtyBits &dirtyBits) override;

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
    gl::Error copySubImageImpl(const gl::Context *context,
                               const gl::ImageIndex &index,
                               const gl::Offset &destOffset,
                               const gl::Rectangle &sourceArea,
                               const gl::InternalFormat &internalFormat,
                               gl::Framebuffer *source);
    vk::Error initImage(RendererVk *renderer,
                        const vk::Format &format,
                        const gl::Extents &extents,
                        const uint32_t levelCount,
                        vk::CommandBuffer *commandBuffer);
    void releaseImage(const gl::Context *context, RendererVk *renderer);
    vk::Error getCommandBufferForWrite(RendererVk *renderer, vk::CommandBuffer **commandBufferOut);
    uint32_t getLevelCount() const;

    vk::ImageHelper mImage;
    vk::ImageView mBaseLevelImageView;
    vk::ImageView mMipmapImageView;
    vk::Sampler mSampler;

    RenderTargetVk mRenderTarget;

    PixelBuffer mPixelBuffer;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_

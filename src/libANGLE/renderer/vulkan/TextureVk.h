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

    void removeStagedUpdates(const gl::ImageIndex &index);

    angle::Result stageSubresourceUpdate(ContextVk *contextVk,
                                         const gl::ImageIndex &index,
                                         const gl::Extents &extents,
                                         const gl::Offset &offset,
                                         const gl::InternalFormat &formatInfo,
                                         const gl::PixelUnpackState &unpack,
                                         GLenum type,
                                         const uint8_t *pixels);

    angle::Result stageSubresourceUpdateAndGetData(ContextVk *contextVk,
                                                   size_t allocationSize,
                                                   const gl::ImageIndex &imageIndex,
                                                   const gl::Extents &extents,
                                                   const gl::Offset &offset,
                                                   uint8_t **destData);

    angle::Result stageSubresourceUpdateFromFramebuffer(const gl::Context *context,
                                                        const gl::ImageIndex &index,
                                                        const gl::Rectangle &sourceArea,
                                                        const gl::Offset &dstOffset,
                                                        const gl::Extents &dstExtent,
                                                        const gl::InternalFormat &formatInfo,
                                                        FramebufferVk *framebufferVk);

    // This will use the underlying dynamic buffer to allocate some memory to be used as a src or
    // dst.
    angle::Result allocate(ContextVk *contextVk,
                           size_t sizeInBytes,
                           uint8_t **ptrOut,
                           VkBuffer *handleOut,
                           VkDeviceSize *offsetOut,
                           bool *newBufferAllocatedOut);

    angle::Result flushUpdatesToImage(ContextVk *contextVk,
                                      uint32_t levelCount,
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

class TextureVk : public TextureImpl
{
  public:
    TextureVk(const gl::TextureState &state, RendererVk *renderer);
    ~TextureVk() override;
    void onDestroy(const gl::Context *context) override;

    angle::Result setImage(const gl::Context *context,
                           const gl::ImageIndex &index,
                           GLenum internalFormat,
                           const gl::Extents &size,
                           GLenum format,
                           GLenum type,
                           const gl::PixelUnpackState &unpack,
                           const uint8_t *pixels) override;
    angle::Result setSubImage(const gl::Context *context,
                              const gl::ImageIndex &index,
                              const gl::Box &area,
                              GLenum format,
                              GLenum type,
                              const gl::PixelUnpackState &unpack,
                              gl::Buffer *unpackBuffer,
                              const uint8_t *pixels) override;

    angle::Result setCompressedImage(const gl::Context *context,
                                     const gl::ImageIndex &index,
                                     GLenum internalFormat,
                                     const gl::Extents &size,
                                     const gl::PixelUnpackState &unpack,
                                     size_t imageSize,
                                     const uint8_t *pixels) override;
    angle::Result setCompressedSubImage(const gl::Context *context,
                                        const gl::ImageIndex &index,
                                        const gl::Box &area,
                                        GLenum format,
                                        const gl::PixelUnpackState &unpack,
                                        size_t imageSize,
                                        const uint8_t *pixels) override;

    angle::Result copyImage(const gl::Context *context,
                            const gl::ImageIndex &index,
                            const gl::Rectangle &sourceArea,
                            GLenum internalFormat,
                            gl::Framebuffer *source) override;
    angle::Result copySubImage(const gl::Context *context,
                               const gl::ImageIndex &index,
                               const gl::Offset &destOffset,
                               const gl::Rectangle &sourceArea,
                               gl::Framebuffer *source) override;

    angle::Result copyTexture(const gl::Context *context,
                              const gl::ImageIndex &index,
                              GLenum internalFormat,
                              GLenum type,
                              size_t sourceLevel,
                              bool unpackFlipY,
                              bool unpackPremultiplyAlpha,
                              bool unpackUnmultiplyAlpha,
                              const gl::Texture *source) override;
    angle::Result copySubTexture(const gl::Context *context,
                                 const gl::ImageIndex &index,
                                 const gl::Offset &destOffset,
                                 size_t sourceLevel,
                                 const gl::Box &sourceBox,
                                 bool unpackFlipY,
                                 bool unpackPremultiplyAlpha,
                                 bool unpackUnmultiplyAlpha,
                                 const gl::Texture *source) override;

    angle::Result setStorage(const gl::Context *context,
                             gl::TextureType type,
                             size_t levels,
                             GLenum internalFormat,
                             const gl::Extents &size) override;

    angle::Result setEGLImageTarget(const gl::Context *context,
                                    gl::TextureType type,
                                    egl::Image *image) override;

    angle::Result setImageExternal(const gl::Context *context,
                                   gl::TextureType type,
                                   egl::Stream *stream,
                                   const egl::Stream::GLTextureDescription &desc) override;

    angle::Result generateMipmap(const gl::Context *context) override;

    angle::Result setBaseLevel(const gl::Context *context, GLuint baseLevel) override;

    angle::Result bindTexImage(const gl::Context *context, egl::Surface *surface) override;
    angle::Result releaseTexImage(const gl::Context *context) override;

    angle::Result getAttachmentRenderTarget(const gl::Context *context,
                                            GLenum binding,
                                            const gl::ImageIndex &imageIndex,
                                            FramebufferAttachmentRenderTarget **rtOut) override;

    angle::Result syncState(const gl::Context *context,
                            const gl::Texture::DirtyBits &dirtyBits) override;

    angle::Result setStorageMultisample(const gl::Context *context,
                                        gl::TextureType type,
                                        GLsizei samples,
                                        GLint internalformat,
                                        const gl::Extents &size,
                                        bool fixedSampleLocations) override;

    angle::Result initializeContents(const gl::Context *context,
                                     const gl::ImageIndex &imageIndex) override;

    const vk::ImageHelper &getImage() const
    {
        ASSERT(mImage.valid());
        return mImage;
    }

    vk::ImageHelper &getImage()
    {
        ASSERT(mImage.valid());
        return mImage;
    }

    const vk::ImageView &getImageView() const;
    const vk::Sampler &getSampler() const;

    angle::Result ensureImageInitialized(ContextVk *contextVk);

  private:
    angle::Result redefineImage(const gl::Context *context,
                                const gl::ImageIndex &index,
                                const gl::InternalFormat &internalFormat,
                                const gl::Extents &size);

    angle::Result copyImageDataToBuffer(ContextVk *contextVk,
                                        size_t sourceLevel,
                                        uint32_t layerCount,
                                        const gl::Rectangle &sourceArea,
                                        uint8_t **outDataPtr);

    angle::Result generateMipmapsWithCPU(const gl::Context *context);

    angle::Result generateMipmapLevelsWithCPU(ContextVk *contextVk,
                                              const angle::Format &sourceFormat,
                                              GLuint layer,
                                              GLuint firstMipLevel,
                                              GLuint maxMipLevel,
                                              size_t sourceWidth,
                                              size_t sourceHeight,
                                              size_t sourceRowPitch,
                                              uint8_t *sourceData);

    angle::Result copySubImageImpl(const gl::Context *context,
                                   const gl::ImageIndex &index,
                                   const gl::Offset &destOffset,
                                   const gl::Rectangle &sourceArea,
                                   const gl::InternalFormat &internalFormat,
                                   gl::Framebuffer *source);

    angle::Result copySubTextureImpl(ContextVk *contextVk,
                                     const gl::ImageIndex &index,
                                     const gl::Offset &destOffset,
                                     const gl::InternalFormat &destFormat,
                                     size_t sourceLevel,
                                     const gl::Rectangle &sourceArea,
                                     bool unpackFlipY,
                                     bool unpackPremultiplyAlpha,
                                     bool unpackUnmultiplyAlpha,
                                     TextureVk *source);

    angle::Result initImage(ContextVk *contextVk,
                            const vk::Format &format,
                            const gl::Extents &extents,
                            const uint32_t levelCount,
                            vk::CommandBuffer *commandBuffer);
    void releaseImage(const gl::Context *context, RendererVk *renderer);
    uint32_t getLevelCount() const;
    angle::Result initCubeMapRenderTargets(ContextVk *contextVk);

    vk::ImageHelper mImage;
    vk::ImageView mBaseLevelImageView;
    vk::ImageView mMipmapImageView;
    vk::Sampler mSampler;

    RenderTargetVk mRenderTarget;
    std::vector<vk::ImageView> mCubeMapFaceImageViews;
    std::vector<RenderTargetVk> mCubeMapRenderTargets;

    PixelBuffer mPixelBuffer;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_TEXTUREVK_H_

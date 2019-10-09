
//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TextureMtl.mm:
//    Implements the class methods for TextureMtl.
//

#include "libANGLE/renderer/metal/TextureMtl.h"

namespace rx
{

// TextureMtl implementation
TextureMtl::TextureMtl(const gl::TextureState &state) : TextureImpl(state) {}

TextureMtl::~TextureMtl() = default;

void TextureMtl::onDestroy(const gl::Context *context)
{
    UNIMPLEMENTED();
}

angle::Result TextureMtl::setImage(const gl::Context *context,
                                   const gl::ImageIndex &index,
                                   GLenum internalFormat,
                                   const gl::Extents &size,
                                   GLenum format,
                                   GLenum type,
                                   const gl::PixelUnpackState &unpack,
                                   const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::setSubImage(const gl::Context *context,
                                      const gl::ImageIndex &index,
                                      const gl::Box &area,
                                      GLenum format,
                                      GLenum type,
                                      const gl::PixelUnpackState &unpack,
                                      gl::Buffer *unpackBuffer,
                                      const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::setCompressedImage(const gl::Context *context,
                                             const gl::ImageIndex &index,
                                             GLenum internalFormat,
                                             const gl::Extents &size,
                                             const gl::PixelUnpackState &unpack,
                                             size_t imageSize,
                                             const uint8_t *pixels)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::setCompressedSubImage(const gl::Context *context,
                                                const gl::ImageIndex &index,
                                                const gl::Box &area,
                                                GLenum format,
                                                const gl::PixelUnpackState &unpack,
                                                size_t imageSize,
                                                const uint8_t *pixels)
{

    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::copyImage(const gl::Context *context,
                                    const gl::ImageIndex &index,
                                    const gl::Rectangle &sourceArea,
                                    GLenum internalFormat,
                                    gl::Framebuffer *source)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::copySubImage(const gl::Context *context,
                                       const gl::ImageIndex &index,
                                       const gl::Offset &destOffset,
                                       const gl::Rectangle &sourceArea,
                                       gl::Framebuffer *source)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::copyTexture(const gl::Context *context,
                                      const gl::ImageIndex &index,
                                      GLenum internalFormat,
                                      GLenum type,
                                      size_t sourceLevel,
                                      bool unpackFlipY,
                                      bool unpackPremultiplyAlpha,
                                      bool unpackUnmultiplyAlpha,
                                      const gl::Texture *source)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::copySubTexture(const gl::Context *context,
                                         const gl::ImageIndex &index,
                                         const gl::Offset &destOffset,
                                         size_t sourceLevel,
                                         const gl::Box &sourceBox,
                                         bool unpackFlipY,
                                         bool unpackPremultiplyAlpha,
                                         bool unpackUnmultiplyAlpha,
                                         const gl::Texture *source)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::copyCompressedTexture(const gl::Context *context,
                                                const gl::Texture *source)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::setStorage(const gl::Context *context,
                                     gl::TextureType type,
                                     size_t mipmaps,
                                     GLenum internalFormat,
                                     const gl::Extents &size)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::setStorageExternalMemory(const gl::Context *context,
                                                   gl::TextureType type,
                                                   size_t levels,
                                                   GLenum internalFormat,
                                                   const gl::Extents &size,
                                                   gl::MemoryObject *memoryObject,
                                                   GLuint64 offset)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::setStorageMultisample(const gl::Context *context,
                                                gl::TextureType type,
                                                GLsizei samples,
                                                GLint internalformat,
                                                const gl::Extents &size,
                                                bool fixedSampleLocations)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::setEGLImageTarget(const gl::Context *context,
                                            gl::TextureType type,
                                            egl::Image *image)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::setImageExternal(const gl::Context *context,
                                           gl::TextureType type,
                                           egl::Stream *stream,
                                           const egl::Stream::GLTextureDescription &desc)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result TextureMtl::generateMipmap(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::setBaseLevel(const gl::Context *context, GLuint baseLevel)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::bindTexImage(const gl::Context *context, egl::Surface *surface)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::releaseTexImage(const gl::Context *context)
{
    UNIMPLEMENTED();

    return angle::Result::Stop;
}

angle::Result TextureMtl::getAttachmentRenderTarget(const gl::Context *context,
                                                    GLenum binding,
                                                    const gl::ImageIndex &imageIndex,
                                                    GLsizei samples,
                                                    FramebufferAttachmentRenderTarget **rtOut)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result TextureMtl::syncState(const gl::Context *context,
                                    const gl::Texture::DirtyBits &dirtyBits)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

angle::Result TextureMtl::initializeContents(const gl::Context *context,
                                             const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

}

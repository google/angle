//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TextureNULL.h:
//    Defines the class interface for TextureNULL, implementing TextureImpl.
//

#ifndef LIBANGLE_RENDERER_NULL_TEXTURENULL_H_
#define LIBANGLE_RENDERER_NULL_TEXTURENULL_H_

#include "libANGLE/renderer/TextureImpl.h"

namespace rx
{

class TextureNULL : public TextureImpl
{
  public:
    TextureNULL(const gl::TextureState &state);
    ~TextureNULL() override;

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
                          gl::Buffer *unpackBuffer,
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
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_NULL_TEXTURENULL_H_

//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureImpl.h: Defines the abstract rx::TextureImpl classes.

#ifndef LIBGLESV2_RENDERER_TEXTUREIMPL_H_
#define LIBGLESV2_RENDERER_TEXTUREIMPL_H_

#include "common/angleutils.h"

namespace egl
{
class Surface;
}

namespace gl
{
class Framebuffer;
struct PixelUnpackState;
struct SamplerState;
}

namespace rx
{

class Image;
class RenderTarget;
class Renderer;
class TextureStorageInterface;

class Texture2DImpl
{
  public:
    virtual ~Texture2DImpl() {}

    // TODO: If this methods could go away that would be ideal;
    // TextureStorage should only be necessary for the D3D backend, and as such
    // higher level code should not rely on it.
    virtual TextureStorageInterface *getNativeTexture() = 0;

    virtual Image *getImage(int level) const = 0;

    virtual void setUsage(GLenum usage) = 0;
    virtual bool hasDirtyImages() const = 0;
    virtual void resetDirty() = 0;

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const = 0;
    virtual void bindTexImage(egl::Surface *surface) = 0;
    virtual void releaseTexImage() = 0;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels) = 0;
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels) = 0;
    virtual void copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height) = 0;
    virtual void generateMipmaps() = 0;

    virtual unsigned int getRenderTargetSerial(GLint level) = 0;

    virtual RenderTarget *getRenderTarget(GLint level) = 0;
    virtual RenderTarget *getDepthSencil(GLint level) = 0;

    virtual void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height) = 0;
};

class TextureCubeImpl
{
  public:
    virtual ~TextureCubeImpl() {}

    virtual TextureStorageInterface *getNativeTexture() = 0;

    virtual Image *getImage(GLenum target, int level) const = 0;

    virtual void setUsage(GLenum usage) = 0;
    virtual bool hasDirtyImages() const = 0;
    virtual void resetDirty() = 0;

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const = 0;
    virtual bool isCubeComplete() const = 0;

    virtual void setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels) = 0;
    virtual void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels) = 0;
    virtual void copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei size) = 0;
    virtual void generateMipmaps() = 0;

    virtual unsigned int getRenderTargetSerial(GLenum target, GLint level) = 0;

    virtual RenderTarget *getRenderTarget(GLenum target, GLint level) = 0;
    virtual RenderTarget *getDepthStencil(GLenum target, GLint level) = 0;
};

class Texture3DImpl
{
  public:
    virtual ~Texture3DImpl() {}

    virtual TextureStorageInterface *getNativeTexture() = 0;

    virtual Image *getImage(int level) const = 0;

    virtual void setUsage(GLenum usage) = 0;
    virtual bool hasDirtyImages() const = 0;
    virtual void resetDirty() = 0;

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const = 0;
    virtual bool isMipmapComplete() const = 0;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels) = 0;
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels) = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = 0;
    virtual void generateMipmaps() = 0;

    virtual unsigned int getRenderTargetSerial(GLint level, GLint layer) = 0;

    virtual RenderTarget *getRenderTarget(GLint level) = 0;
    virtual RenderTarget *getRenderTarget(GLint level, GLint layer) = 0;
    virtual RenderTarget *getDepthStencil(GLint level, GLint layer) = 0;
};

class Texture2DArrayImpl
{
  public:
    virtual ~Texture2DArrayImpl() {}

    virtual TextureStorageInterface *getNativeTexture() = 0;

    virtual Image *getImage(int level, int layer) const = 0;
    virtual GLsizei getLayerCount(int level) const = 0;

    virtual void setUsage(GLenum usage) = 0;
    virtual bool hasDirtyImages() const = 0;
    virtual void resetDirty() = 0;

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const = 0;
    virtual bool isMipmapComplete() const = 0;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels) = 0;
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels) = 0;
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels) = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth) = 0;
    virtual void generateMipmaps() = 0;

    virtual unsigned int getRenderTargetSerial(GLint level, GLint layer) = 0;

    virtual RenderTarget *getRenderTarget(GLint level, GLint layer) = 0;
    virtual RenderTarget *getDepthStencil(GLint level, GLint layer) = 0;
};

}

#endif // LIBGLESV2_RENDERER_TEXTUREIMPL_H_

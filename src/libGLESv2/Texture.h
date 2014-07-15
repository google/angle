//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.h: Defines the abstract gl::Texture class and its concrete derived
// classes Texture2D and TextureCubeMap. Implements GL texture objects and
// related functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include <vector>

#include "angle_gl.h"

#include "common/debug.h"
#include "common/RefCountObject.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/constants.h"

namespace egl
{
class Surface;
}

namespace rx
{
class Renderer;
class Texture2DImpl;
class TextureCubeImpl;
class Texture3DImpl;
class TextureStorageInterface;
class TextureStorageInterface3D;
class TextureStorageInterface2DArray;
class RenderTarget;
class Image;
}

namespace gl
{
class Framebuffer;
class FramebufferAttachment;

bool IsMipmapFiltered(const SamplerState &samplerState);

class Texture : public RefCountObject
{
  public:
    Texture(GLuint id, GLenum target);

    virtual ~Texture();

    GLenum getTarget() const;

    const SamplerState &getSamplerState() const { return mSamplerState; }
    SamplerState &getSamplerState() { return mSamplerState; }
    void getSamplerStateWithNativeOffset(SamplerState *sampler);

    virtual void setUsage(GLenum usage);
    GLenum getUsage() const;

    GLint getBaseLevelWidth() const;
    GLint getBaseLevelHeight() const;
    GLint getBaseLevelDepth() const;
    GLenum getBaseLevelInternalFormat() const;

    virtual bool isSamplerComplete(const SamplerState &samplerState) const = 0;

    virtual rx::TextureStorageInterface *getNativeTexture() = 0;

    virtual void generateMipmaps() = 0;
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source) = 0;

    virtual bool hasDirtyImages() const = 0;
    virtual void resetDirty() = 0;
    unsigned int getTextureSerial();

    bool isImmutable() const;
    int immutableLevelCount();

    static const GLuint INCOMPLETE_TEXTURE_ID = static_cast<GLuint>(-1);   // Every texture takes an id at creation time. The value is arbitrary because it is never registered with the resource manager.

  protected:
    int mipLevels() const;

    SamplerState mSamplerState;
    GLenum mUsage;

    bool mImmutable;

    GLenum mTarget;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);

    virtual const rx::Image *getBaseLevelImage() const = 0;
};

// TODO: This class is only here to make incremental Texture refactoring easier
class TextureWithRenderer : public Texture
{
  public:
    TextureWithRenderer(rx::Renderer *renderer, GLuint id, GLenum target);
    virtual ~TextureWithRenderer();

    virtual rx::TextureStorageInterface *getNativeTexture();

    virtual bool hasDirtyImages() const;
    virtual void resetDirty();

  protected:
    void setImage(const PixelUnpackState &unpack, GLenum type, const void *pixels, rx::Image *image);
    bool subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                  GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels, rx::Image *image);
    void setCompressedImage(GLsizei imageSize, const void *pixels, rx::Image *image);
    bool subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                            GLenum format, GLsizei imageSize, const void *pixels, rx::Image *image);
    bool isFastUnpackable(const PixelUnpackState &unpack, GLenum sizedInternalFormat);
    bool fastUnpackPixels(const PixelUnpackState &unpack, const void *pixels, const Box &destArea,
                          GLenum sizedInternalFormat, GLenum type, rx::RenderTarget *destRenderTarget);

    GLint creationLevels(GLsizei width, GLsizei height, GLsizei depth) const;

    virtual void initializeStorage(bool renderTarget) = 0;
    virtual void updateStorage() = 0;
    virtual bool ensureRenderTarget() = 0;

    rx::Renderer *mRenderer;
    bool mDirtyImages;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureWithRenderer);

    virtual rx::TextureStorageInterface *getBaseLevelStorage() = 0;
};

class Texture2D : public Texture
{
  public:
    Texture2D(rx::Texture2DImpl *impl, GLuint id);

    ~Texture2D();

    virtual rx::TextureStorageInterface *getNativeTexture();
    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const;
    virtual void resetDirty();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    void setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

    virtual bool isSamplerComplete(const SamplerState &samplerState) const;
    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual void generateMipmaps();

    unsigned int getRenderTargetSerial(GLint level);

  protected:
    friend class Texture2DAttachment;
    rx::RenderTarget *getRenderTarget(GLint level);
    rx::RenderTarget *getDepthSencil(GLint level);

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    virtual const rx::Image *getBaseLevelImage() const;

    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height);

    rx::Texture2DImpl *mTexture;
    egl::Surface *mSurface;
};

class TextureCubeMap : public Texture
{
  public:
    TextureCubeMap(rx::TextureCubeImpl *impl, GLuint id);

    ~TextureCubeMap();

    virtual rx::TextureStorageInterface *getNativeTexture();
    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const;
    virtual void resetDirty();

    GLsizei getWidth(GLenum target, GLint level) const;
    GLsizei getHeight(GLenum target, GLint level) const;
    GLenum getInternalFormat(GLenum target, GLint level) const;
    GLenum getActualFormat(GLenum target, GLint level) const;
    bool isCompressed(GLenum target, GLint level) const;
    bool isDepth(GLenum target, GLint level) const;

    void setImagePosX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setImageNegX(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setImagePosY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setImageNegY(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setImagePosZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setImageNegZ(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);

    void setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);

    void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    void copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);
    void storage(GLsizei levels, GLenum internalformat, GLsizei size);

    virtual bool isSamplerComplete(const SamplerState &samplerState) const;
    bool isCubeComplete() const;

    virtual void generateMipmaps();

    unsigned int getRenderTargetSerial(GLenum target, GLint level);

  protected:
    friend class TextureCubeMapAttachment;
    rx::RenderTarget *getRenderTarget(GLenum target, GLint level);
    rx::RenderTarget *getDepthStencil(GLenum target, GLint level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    virtual const rx::Image *getBaseLevelImage() const;

    rx::TextureCubeImpl *mTexture;
};

class Texture3D : public Texture
{
  public:
    Texture3D(rx::Texture3DImpl *impl, GLuint id);

    ~Texture3D();

    virtual rx::TextureStorageInterface *getNativeTexture();
    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const;
    virtual void resetDirty();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getDepth(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels);
    void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void generateMipmaps();
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);

    virtual bool isSamplerComplete(const SamplerState &samplerState) const;
    virtual bool isMipmapComplete() const;

    unsigned int getRenderTargetSerial(GLint level, GLint layer);

  protected:
    friend class Texture3DAttachment;
    rx::RenderTarget *getRenderTarget(GLint level);
    rx::RenderTarget *getRenderTarget(GLint level, GLint layer);
    rx::RenderTarget *getDepthStencil(GLint level, GLint layer);

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture3D);

    virtual const rx::Image *getBaseLevelImage() const;

    rx::Texture3DImpl *mTexture;
};

class Texture2DArray : public TextureWithRenderer
{
  public:
    Texture2DArray(rx::Renderer *renderer, GLuint id);

    ~Texture2DArray();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getLayers(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isCompressed(GLint level) const;
    bool isDepth(GLint level) const;

    void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const PixelUnpackState &unpack, const void *pixels);
    void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels);
    void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual void generateMipmaps();
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, Framebuffer *source);

    virtual bool isSamplerComplete(const SamplerState &samplerState) const;
    virtual bool isMipmapComplete() const;

    unsigned int getRenderTargetSerial(GLint level, GLint layer);

  protected:
    friend class Texture2DArrayAttachment;
    rx::RenderTarget *getRenderTarget(GLint level, GLint layer);
    rx::RenderTarget *getDepthStencil(GLint level, GLint layer);

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2DArray);

    virtual void initializeStorage(bool renderTarget);
    rx::TextureStorageInterface2DArray *createCompleteStorage(bool renderTarget) const;
    void setCompleteTexStorage(rx::TextureStorageInterface2DArray *newCompleteTexStorage);

    virtual void updateStorage();
    virtual bool ensureRenderTarget();

    virtual rx::TextureStorageInterface *getBaseLevelStorage();
    virtual const rx::Image *getBaseLevelImage() const;

    void deleteImages();
    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLint layerTarget, GLsizei width, GLsizei height);

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    void updateStorageLevel(int level);

    // Storing images as an array of single depth textures since D3D11 treats each array level of a
    // Texture2D object as a separate subresource.  Each layer would have to be looped over
    // to update all the texture layers since they cannot all be updated at once and it makes the most
    // sense for the Image class to not have to worry about layer subresource as well as mip subresources.
    GLsizei mLayerCounts[IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    rx::Image **mImageArray[IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    rx::TextureStorageInterface2DArray *mTexStorage;
};

}

#endif   // LIBGLESV2_TEXTURE_H_

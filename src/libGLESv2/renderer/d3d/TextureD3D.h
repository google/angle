//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureD3D.h: Implementations of the Texture interfaces shared betweeen the D3D backends.

#ifndef LIBGLESV2_RENDERER_TEXTURED3D_H_
#define LIBGLESV2_RENDERER_TEXTURED3D_H_

#include "libGLESv2/renderer/TextureImpl.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/constants.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{

class Image;
class ImageD3D;
class Renderer;
class TextureStorageInterface;
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;
class TextureStorageInterface3D;
class TextureStorageInterface2DArray;

bool IsMipmapFiltered(const gl::SamplerState &samplerState);

class TextureD3D
{
  public:
    TextureD3D(Renderer *renderer);
    virtual ~TextureD3D();

    GLint getBaseLevelWidth() const;
    GLint getBaseLevelHeight() const;
    GLint getBaseLevelDepth() const;
    GLenum getBaseLevelInternalFormat() const;

    bool isImmutable() const { return mImmutable; }

  protected:
    void setImage(const gl::PixelUnpackState &unpack, GLenum type, const void *pixels, Image *image);
    bool subImage(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                  GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels, Image *image);
    void setCompressedImage(GLsizei imageSize, const void *pixels, Image *image);
    bool subImageCompressed(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                            GLenum format, GLsizei imageSize, const void *pixels, Image *image);
    bool isFastUnpackable(const gl::PixelUnpackState &unpack, GLenum sizedInternalFormat);
    bool fastUnpackPixels(const gl::PixelUnpackState &unpack, const void *pixels, const gl::Box &destArea,
                                  GLenum sizedInternalFormat, GLenum type, RenderTarget *destRenderTarget);

    GLint creationLevels(GLsizei width, GLsizei height, GLsizei depth) const;
    int mipLevels() const;

    Renderer *mRenderer;

    GLenum mUsage;

    bool mDirtyImages;

    bool mImmutable;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D);

    virtual TextureStorageInterface *getBaseLevelStorage() = 0;
    virtual const ImageD3D *getBaseLevelImage() const = 0;
};

class TextureD3D_2D : public Texture2DImpl, public TextureD3D
{
  public:
    TextureD3D_2D(Renderer *renderer);
    virtual ~TextureD3D_2D();

    static TextureD3D_2D *makeTextureD3D_2D(Texture2DImpl *texture);

    virtual TextureStorageInterface *getNativeTexture();

    virtual Image *getImage(int level) const;

    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const { return mDirtyImages; }
    virtual void resetDirty();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    GLenum getActualFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    virtual void copyImage(GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const;
    virtual void bindTexImage(egl::Surface *surface);
    virtual void releaseTexImage();

    virtual void generateMipmaps();

    virtual unsigned int getRenderTargetSerial(GLint level);

    virtual RenderTarget *getRenderTarget(GLint level);
    virtual RenderTarget *getDepthSencil(GLint level);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_2D);

    void initializeStorage(bool renderTarget);
    TextureStorageInterface2D *createCompleteStorage(bool renderTarget) const;
    void setCompleteTexStorage(TextureStorageInterface2D *newCompleteTexStorage);

    void updateStorage();
    bool ensureRenderTarget();
    virtual TextureStorageInterface *getBaseLevelStorage();
    virtual const ImageD3D *getBaseLevelImage() const;

    bool isMipmapComplete() const;
    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;

    void updateStorageLevel(int level);

    virtual void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    TextureStorageInterface2D *mTexStorage;
    ImageD3D *mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
};

class TextureD3D_Cube : public TextureCubeImpl, public TextureD3D
{
  public:
    TextureD3D_Cube(Renderer *renderer);
    virtual ~TextureD3D_Cube();

    static TextureD3D_Cube *makeTextureD3D_Cube(TextureCubeImpl *texture);

    virtual TextureStorageInterface *getNativeTexture();

    virtual Image *getImage(GLenum target, int level) const;

    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const { return mDirtyImages; }
    virtual void resetDirty();

    GLenum getInternalFormat(GLenum target, GLint level) const;
    bool isDepth(GLenum target, GLint level) const;

    virtual void setImage(int faceIndex, GLint level, GLsizei width, GLsizei height, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void setCompressedImage(GLenum target, GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei imageSize, const void *pixels);
    virtual void subImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void subImageCompressed(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *pixels);
    virtual void copyImage(GLenum target, GLint level, GLenum format, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei size);

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const;
    virtual bool isCubeComplete() const;

    virtual void generateMipmaps();

    virtual unsigned int getRenderTargetSerial(GLenum target, GLint level);

    virtual RenderTarget *getRenderTarget(GLenum target, GLint level);
    virtual RenderTarget *getDepthStencil(GLenum target, GLint level);

    static int targetToIndex(GLenum target);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_Cube);

    void initializeStorage(bool renderTarget);
    TextureStorageInterfaceCube *createCompleteStorage(bool renderTarget) const;
    void setCompleteTexStorage(TextureStorageInterfaceCube *newCompleteTexStorage);

    void updateStorage();
    bool ensureRenderTarget();
    virtual TextureStorageInterface *getBaseLevelStorage();
    virtual const ImageD3D *getBaseLevelImage() const;

    bool isMipmapCubeComplete() const;
    bool isValidFaceLevel(int faceIndex, int level) const;
    bool isFaceLevelComplete(int faceIndex, int level) const;
    void updateStorageFaceLevel(int faceIndex, int level);

    void redefineImage(int faceIndex, GLint level, GLenum internalformat, GLsizei width, GLsizei height);
    void commitRect(int faceIndex, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    ImageD3D *mImageArray[6][gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    TextureStorageInterfaceCube *mTexStorage;
};

class TextureD3D_3D : public Texture3DImpl, public TextureD3D
{
  public:
    TextureD3D_3D(Renderer *renderer);
    virtual ~TextureD3D_3D();

    static TextureD3D_3D *makeTextureD3D_3D(Texture3DImpl *texture);

    virtual TextureStorageInterface *getNativeTexture();

    virtual Image *getImage(int level) const;

    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const { return mDirtyImages; }
    virtual void resetDirty();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getDepth(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const;
    virtual bool isMipmapComplete() const;

    virtual void generateMipmaps();

    virtual unsigned int getRenderTargetSerial(GLint level, GLint layer);

    virtual RenderTarget *getRenderTarget(GLint level);
    virtual RenderTarget *getRenderTarget(GLint level, GLint layer);
    virtual RenderTarget *getDepthStencil(GLint level, GLint layer);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_3D);

    virtual void initializeStorage(bool renderTarget);
    TextureStorageInterface3D *createCompleteStorage(bool renderTarget) const;
    void setCompleteTexStorage(TextureStorageInterface3D *newCompleteTexStorage);

    void updateStorage();
    bool ensureRenderTarget();
    virtual TextureStorageInterface *getBaseLevelStorage();
    virtual const ImageD3D *getBaseLevelImage() const;

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    void updateStorageLevel(int level);

    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth);

    ImageD3D *mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    TextureStorageInterface3D *mTexStorage;
};

class TextureD3D_2DArray : public Texture2DArrayImpl, public TextureD3D
{
  public:
    TextureD3D_2DArray(Renderer *renderer);
    virtual ~TextureD3D_2DArray();

    static TextureD3D_2DArray *makeTextureD3D_2DArray(Texture2DArrayImpl *texture);

    virtual TextureStorageInterface *getNativeTexture();

    virtual Image *getImage(int level, int layer) const;
    virtual GLsizei getLayerCount(int level) const;

    virtual void setUsage(GLenum usage);
    virtual bool hasDirtyImages() const { return mDirtyImages; }
    virtual void resetDirty();

    GLsizei getWidth(GLint level) const;
    GLsizei getHeight(GLint level) const;
    GLsizei getLayers(GLint level) const;
    GLenum getInternalFormat(GLint level) const;
    bool isDepth(GLint level) const;

    virtual void setImage(GLint level, GLsizei width, GLsizei height, GLsizei depth, GLenum internalFormat, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void setCompressedImage(GLint level, GLenum format, GLsizei width, GLsizei height, GLsizei depth, GLsizei imageSize, const void *pixels);
    virtual void subImage(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const gl::PixelUnpackState &unpack, const void *pixels);
    virtual void subImageCompressed(GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *pixels);
    virtual void copySubImage(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);
    virtual void storage(GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);

    virtual bool isSamplerComplete(const gl::SamplerState &samplerState) const;
    virtual bool isMipmapComplete() const;

    virtual void generateMipmaps();

    virtual unsigned int getRenderTargetSerial(GLint level, GLint layer);

    virtual RenderTarget *getRenderTarget(GLint level, GLint layer);
    virtual RenderTarget *getDepthStencil(GLint level, GLint layer);

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureD3D_2DArray);

    virtual void initializeStorage(bool renderTarget);
    TextureStorageInterface2DArray *createCompleteStorage(bool renderTarget) const;
    void setCompleteTexStorage(TextureStorageInterface2DArray *newCompleteTexStorage);

    void updateStorage();
    bool ensureRenderTarget();
    virtual TextureStorageInterface *getBaseLevelStorage();
    virtual const ImageD3D *getBaseLevelImage() const;

    bool isValidLevel(int level) const;
    bool isLevelComplete(int level) const;
    void updateStorageLevel(int level);

    void deleteImages();
    void redefineImage(GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLint layerTarget, GLsizei width, GLsizei height);

    // Storing images as an array of single depth textures since D3D11 treats each array level of a
    // Texture2D object as a separate subresource.  Each layer would have to be looped over
    // to update all the texture layers since they cannot all be updated at once and it makes the most
    // sense for the Image class to not have to worry about layer subresource as well as mip subresources.
    GLsizei mLayerCounts[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];
    ImageD3D **mImageArray[gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS];

    TextureStorageInterface2DArray *mTexStorage;
};

}

#endif // LIBGLESV2_RENDERER_TEXTURED3D_H_

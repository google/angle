//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Texture.h: Defines the abstract gl::Texture class and its concrete derived
// classes Texture2D and TextureCubeMap. Implements GL texture objects and
// related functionality. [OpenGL ES 2.0.24] section 3.7 page 63.

#ifndef LIBGLESV2_TEXTURE_H_
#define LIBGLESV2_TEXTURE_H_

#include "Renderbuffer.h"

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include <vector>

namespace gl
{
enum
{
    MAX_TEXTURE_SIZE = 2048,
    MAX_CUBE_MAP_TEXTURE_SIZE = 2048,

    MAX_TEXTURE_LEVELS = 12   // 1+log2 of MAX_TEXTURE_SIZE
};

class Texture : public Colorbuffer
{
  public:
    Texture();

    ~Texture();

    virtual GLenum getTarget() const = 0;

    bool setMinFilter(GLenum filter);
    bool setMagFilter(GLenum filter);
    bool setWrapS(GLenum wrap);
    bool setWrapT(GLenum wrap);

    GLenum getMinFilter() const;
    GLenum getMagFilter() const;
    GLenum getWrapS() const;
    GLenum getWrapT() const;

    virtual bool isComplete() const = 0;

    IDirect3DBaseTexture9 *getTexture();
    IDirect3DSurface9 *getRenderTarget(GLenum target);
    IDirect3DSurface9 *getRenderTarget() { return getRenderTarget(GL_TEXTURE_2D); } // FIXME: to be removed once FBO rendering is completed.

  protected:
    // Helper structure representing a single image layer
    struct Image
    {
        Image();
        ~Image();

        GLsizei width;
        GLsizei height;
        GLenum format;

        bool dirty;

        IDirect3DSurface9 *surface;
    };

    static D3DFORMAT selectFormat(GLenum format);
    static int pixelSize(GLenum format, GLenum type);
    int imagePitch(const Image& img) const;

    GLenum mMinFilter;
    GLenum mMagFilter;
    GLenum mWrapS;
    GLenum mWrapT;

    void setImage(GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);
    void subImage(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels, Image *img);

    // The pointer returned is weak and it is assumed the derived class will keep a strong pointer until the next createTexture() call.
    virtual IDirect3DBaseTexture9 *createTexture() = 0;
    virtual void updateTexture() = 0;
    virtual IDirect3DBaseTexture9 *convertToRenderTarget() = 0;
    virtual IDirect3DSurface9 *getSurface(GLenum target) = 0;

    virtual bool dirtyImageData() const = 0;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture);

    IDirect3DBaseTexture9 *mBaseTexture; // This is a weak pointer. The derived class is assumed to own a strong pointer.

    bool mDirtyMetaData;

    void loadImageData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type,
                       GLint unpackAlignment, const void *input, std::size_t outputPitch, void *output) const;

    GLsizei computePitch(GLsizei width, GLenum format, GLenum type, GLint alignment) const;
};

class Texture2D : public Texture
{
  public:
    Texture2D();

    ~Texture2D();

    GLenum getTarget() const;

    void setImage(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void subImage(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    bool isComplete() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Texture2D);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();
    virtual IDirect3DSurface9 *getSurface(GLenum target);

    virtual bool dirtyImageData() const;

    void commitRect(GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    Image mImageArray[MAX_TEXTURE_LEVELS];

    IDirect3DTexture9 *mTexture;
};

class TextureCubeMap : public Texture
{
  public:
    TextureCubeMap();

    ~TextureCubeMap();

    GLenum getTarget() const;

    void setImagePosX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegX(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegY(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImagePosZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void setImageNegZ(GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    void subImage(GLenum face, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);

    bool isComplete() const;

  private:
    DISALLOW_COPY_AND_ASSIGN(TextureCubeMap);

    virtual IDirect3DBaseTexture9 *createTexture();
    virtual void updateTexture();
    virtual IDirect3DBaseTexture9 *convertToRenderTarget();
    virtual IDirect3DSurface9 *getSurface(GLenum target);

    virtual bool dirtyImageData() const;

    static unsigned int faceIndex(GLenum face);

    void setImage(int face, GLint level, GLenum internalFormat, GLsizei width, GLsizei height, GLenum format, GLenum type, GLint unpackAlignment, const void *pixels);
    void commitRect(GLenum faceTarget, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    Image mImageArray[6][MAX_TEXTURE_LEVELS];

    IDirect3DCubeTexture9 *mTexture;
};
}

#endif   // LIBGLESV2_TEXTURE_H_

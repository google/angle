//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Defines the gl::Image class, which acts as the interface to
// the actual underlying surfaces of a Texture.

#ifndef LIBGLESV2_RENDERER_IMAGE_H_
#define LIBGLESV2_RENDERER_IMAGE_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "common/debug.h"

namespace gl
{
class TextureStorage2D;
class TextureStorageCubeMap;

class Image
{
  public:
    Image();
    ~Image();

    static void Image::CopyLockableSurfaces(IDirect3DSurface9 *dest, IDirect3DSurface9 *source);

    bool redefine(GLint internalformat, GLsizei width, GLsizei height, bool forceRelease);
    void markDirty() {mDirty = true;}
    void markClean() {mDirty = false;}

    bool isRenderableFormat() const;
    D3DFORMAT getD3DFormat() const;
    GLenum getActualFormat() const;

    GLsizei getWidth() const {return mWidth;}
    GLsizei getHeight() const {return mHeight;}
    GLenum getInternalFormat() const {return mInternalFormat;}
    bool isDirty() const {return mSurface && mDirty;}
    IDirect3DSurface9 *getSurface();

    void setManagedSurface(TextureStorage2D *storage, int level);
    void setManagedSurface(TextureStorageCubeMap *storage, int face, int level);
    bool updateSurface(TextureStorage2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    bool updateSurface(TextureStorageCubeMap *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    void loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                  GLint unpackAlignment, const void *input);

    void loadAlphaData(GLsizei width, GLsizei height,
                       int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadAlphaDataSSE2(GLsizei width, GLsizei height,
                           int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadAlphaFloatData(GLsizei width, GLsizei height,
                            int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadAlphaHalfFloatData(GLsizei width, GLsizei height,
                                int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceData(GLsizei width, GLsizei height,
                           int inputPitch, const void *input, size_t outputPitch, void *output, bool native) const;
    void loadLuminanceFloatData(GLsizei width, GLsizei height,
                                int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceHalfFloatData(GLsizei width, GLsizei height,
                                    int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceAlphaData(GLsizei width, GLsizei height,
                                int inputPitch, const void *input, size_t outputPitch, void *output, bool native) const;
    void loadLuminanceAlphaFloatData(GLsizei width, GLsizei height,
                                     int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadLuminanceAlphaHalfFloatData(GLsizei width, GLsizei height,
                                         int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBUByteData(GLsizei width, GLsizei height,
                          int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGB565Data(GLsizei width, GLsizei height,
                        int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBFloatData(GLsizei width, GLsizei height,
                          int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBHalfFloatData(GLsizei width, GLsizei height,
                              int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAUByteDataSSE2(GLsizei width, GLsizei height,
                               int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAUByteData(GLsizei width, GLsizei height,
                           int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBA4444Data(GLsizei width, GLsizei height,
                          int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBA5551Data(GLsizei width, GLsizei height,
                          int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAFloatData(GLsizei width, GLsizei height,
                           int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadRGBAHalfFloatData(GLsizei width, GLsizei height,
                               int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadBGRAData(GLsizei width, GLsizei height,
                      int inputPitch, const void *input, size_t outputPitch, void *output) const;
    void loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                            const void *input);

    void copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, IDirect3DSurface9 *renderTarget);

  private:
    DISALLOW_COPY_AND_ASSIGN(Image);

    void createSurface();
    void setManagedSurface(IDirect3DSurface9 *surface);
    bool updateSurface(IDirect3DSurface9 *dest, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    HRESULT lock(D3DLOCKED_RECT *lockedRect, const RECT *rect);
    void unlock();

    GLsizei mWidth;
    GLsizei mHeight;
    GLint mInternalFormat;

    bool mDirty;

    D3DPOOL mD3DPool;   // can only be D3DPOOL_SYSTEMMEM or D3DPOOL_MANAGED since it needs to be lockable.
    D3DFORMAT mD3DFormat;
    GLenum mActualFormat;

    IDirect3DSurface9 *mSurface;
};
}

#endif   // LIBGLESV2_RENDERER_IMAGE_H_

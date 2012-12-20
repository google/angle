//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Defines the rx::Image class, an abstract base class for the 
// renderer-specific classes which will define the interface to the underlying
// surfaces or resources.

#ifndef LIBGLESV2_RENDERER_IMAGE_H_
#define LIBGLESV2_RENDERER_IMAGE_H_

#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class TextureStorage2D;
class TextureStorageCubeMap;

class Image
{
  public:
    Image();
    virtual ~Image() {};

    GLsizei getWidth() const { return mWidth; }
    GLsizei getHeight() const { return mHeight; }
    GLenum getInternalFormat() const { return mInternalFormat; }
    GLenum getActualFormat() const { return mActualFormat; }

    void markDirty() {mDirty = true;}
    void markClean() {mDirty = false;}
    virtual bool isDirty() const = 0;

    virtual void setManagedSurface(TextureStorage2D *storage, int level) {};
    virtual void setManagedSurface(TextureStorageCubeMap *storage, int face, int level) {};
    virtual bool updateSurface(TextureStorage2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height) = 0;
    virtual bool updateSurface(TextureStorageCubeMap *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height) = 0;

    virtual bool redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease) = 0;

    virtual bool isRenderableFormat() const = 0;
    
    virtual void loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                  GLint unpackAlignment, const void *input) = 0;
    virtual void loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    const void *input) = 0;

    virtual void copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;

    static void loadAlphaData(GLsizei width, GLsizei height,
                              int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadAlphaDataSSE2(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadAlphaFloatData(GLsizei width, GLsizei height,
                                   int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadAlphaHalfFloatData(GLsizei width, GLsizei height,
                                       int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadLuminanceData(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output, bool native);
    static void loadLuminanceFloatData(GLsizei width, GLsizei height,
                                       int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadLuminanceHalfFloatData(GLsizei width, GLsizei height,
                                           int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadLuminanceAlphaData(GLsizei width, GLsizei height,
                                       int inputPitch, const void *input, size_t outputPitch, void *output, bool native);
    static void loadLuminanceAlphaFloatData(GLsizei width, GLsizei height,
                                            int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadLuminanceAlphaHalfFloatData(GLsizei width, GLsizei height,
                                                int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBUByteData(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGB565Data(GLsizei width, GLsizei height,
                               int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBFloatData(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBHalfFloatData(GLsizei width, GLsizei height,
                                     int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBAUByteDataSSE2(GLsizei width, GLsizei height,
                                      int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBAUByteData(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBA4444Data(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBA5551Data(GLsizei width, GLsizei height,
                                 int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBAFloatData(GLsizei width, GLsizei height,
                                  int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadRGBAHalfFloatData(GLsizei width, GLsizei height,
                                      int inputPitch, const void *input, size_t outputPitch, void *output);
    static void loadBGRAData(GLsizei width, GLsizei height,
                             int inputPitch, const void *input, size_t outputPitch, void *output);

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLint mInternalFormat;
    GLenum mActualFormat;

    bool mDirty;

  private:
    DISALLOW_COPY_AND_ASSIGN(Image);
};

}

#endif // LIBGLESV2_RENDERER_IMAGE_H_

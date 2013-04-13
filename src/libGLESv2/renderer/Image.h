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
class TextureStorageInterface2D;
class TextureStorageInterfaceCube;
class TextureStorageInterface3D;

class Image
{
  public:
    Image();
    virtual ~Image() {};

    GLsizei getWidth() const { return mWidth; }
    GLsizei getHeight() const { return mHeight; }
    GLsizei getDepth() const { return mDepth; }
    GLenum getInternalFormat() const { return mInternalFormat; }
    GLenum getActualFormat() const { return mActualFormat; }

    void markDirty() {mDirty = true;}
    void markClean() {mDirty = false;}
    virtual bool isDirty() const = 0;

    virtual void setManagedSurface(TextureStorageInterface2D *storage, int level) {};
    virtual void setManagedSurface(TextureStorageInterfaceCube *storage, int face, int level) {};
    virtual void setManagedSurface(TextureStorageInterface3D *storage, int level) {};
    virtual bool updateSurface(TextureStorageInterface2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height) = 0;
    virtual bool updateSurface(TextureStorageInterfaceCube *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height) = 0;
    virtual bool updateSurface(TextureStorageInterface3D *storage, int level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth) = 0;

    virtual bool redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, bool forceRelease) = 0;

    virtual bool isRenderableFormat() const = 0;
    
    virtual void loadData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                          GLint unpackAlignment, const void *input) = 0;
    virtual void loadCompressedData(GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth,
                                    const void *input) = 0;

    virtual void copy(GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source) = 0;

    static void loadAlphaDataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                    int inputRowPitch, int inputDepthPitch, const void *input,
                                    size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadAlphaDataToNative(GLsizei width, GLsizei height, GLsizei depth,
                                      int inputRowPitch, int inputDepthPitch, const void *input,
                                      size_t outRowputPitch, size_t outputDepthPitch, void *output);
    static void loadAlphaDataToBGRASSE2(GLsizei width, GLsizei height, GLsizei depth,
                                        int inputRowPitch, int inputDepthPitch, const void *input,
                                        size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadAlphaFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                         int inputRowPitch, int inputDepthPitch, const void *input,
                                         size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadAlphaHalfFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                            int inputRowPitch, int inputDepthPitch, const void *input,
                                            size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadLuminanceDataToNativeOrBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                                int inputRowPitch, int inputDepthPitch, const void *input,
                                                size_t outputRowPitch, size_t outputDepthPitch, void *output,
                                                bool native);
    static void loadLuminanceFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                             int inputRowPitch, int inputDepthPitch, const void *input,
                                             size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadLuminanceFloatDataToRGB(GLsizei width, GLsizei height, GLsizei depth,
                                            int inputRowPitch, int inputDepthPitch, const void *input,
                                            size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadLuminanceHalfFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                                 int inputRowPitch, int inputDepthPitch, const void *input,
                                                 size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadLuminanceAlphaDataToNativeOrBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                                     int inputRowPitch, int inputDepthPitch, const void *input,
                                                     size_t outputRowPitch, size_t outputDepthPitch, void *output,
                                                     bool native);
    static void loadLuminanceAlphaFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                                  int inputRowPitch, int inputDepthPitch, const void *input,
                                                  size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadLuminanceAlphaHalfFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                                      int inputRowPitch, int inputDepthPitch, const void *input,
                                                      size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBUByteDataToBGRX(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBUByteDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGB565DataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                     int inputRowPitch, int inputDepthPitch, const void *input,
                                     size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGB565DataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                     int inputRowPitch, int inputDepthPitch, const void *input,
                                     size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBFloatDataToNative(GLsizei width, GLsizei height, GLsizei depth,
                                         int inputRowPitch, int inputDepthPitch, const void *input,
                                         size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBHalfFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                           int inputRowPitch, int inputDepthPitch, const void *input,
                                           size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBAUByteDataToBGRASSE2(GLsizei width, GLsizei height, GLsizei depth,
                                            int inputRowPitch, int inputDepthPitch, const void *input,
                                            size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBAUByteDataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                        int inputRowPitch, int inputDepthPitch, const void *input,
                                        size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBAUByteDataToNative(GLsizei width, GLsizei height, GLsizei depth,
                                          int inputRowPitch, int inputDepthPitch, const void *input,
                                          size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBA4444DataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBA4444DataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBA5551DataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBA5551DataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                       int inputRowPitch, int inputDepthPitch, const void *input,
                                       size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBAFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                        int inputRowPitch, int inputDepthPitch, const void *input,
                                        size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadRGBAHalfFloatDataToRGBA(GLsizei width, GLsizei height, GLsizei depth,
                                            int inputRowPitch, int inputDepthPitch, const void *input,
                                            size_t outputRowPitch, size_t outputDepthPitch, void *output);
    static void loadBGRADataToBGRA(GLsizei width, GLsizei height, GLsizei depth,
                                   int inputRowPitch, int inputDepthPitch, const void *input,
                                   size_t outputRowPitch, size_t outputDepthPitch, void *output);

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLsizei mDepth;
    GLint mInternalFormat;
    GLenum mActualFormat;

    bool mDirty;

  private:
    DISALLOW_COPY_AND_ASSIGN(Image);
};

}

#endif // LIBGLESV2_RENDERER_IMAGE_H_

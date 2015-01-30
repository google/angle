//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Defines the rx::Image class, an abstract base class for the
// renderer-specific classes which will define the interface to the underlying
// surfaces or resources.

#ifndef LIBANGLE_RENDERER_IMAGE_H_
#define LIBANGLE_RENDERER_IMAGE_H_

#include "common/debug.h"
#include "libANGLE/Error.h"

#include <GLES2/gl2.h>

namespace gl
{
class Framebuffer;
struct Rectangle;
struct Extents;
struct Box;
struct Offset;
struct ImageIndex;
}

namespace rx
{
class RendererD3D;
class RenderTarget;
class TextureStorage;

class Image
{
  public:
    Image();
    virtual ~Image() {};

    GLsizei getWidth() const { return mWidth; }
    GLsizei getHeight() const { return mHeight; }
    GLsizei getDepth() const { return mDepth; }
    GLenum getInternalFormat() const { return mInternalFormat; }
    GLenum getTarget() const { return mTarget; }
    bool isRenderableFormat() const { return mRenderable; }

    void markDirty() {mDirty = true;}
    void markClean() {mDirty = false;}
    virtual bool isDirty() const = 0;

    virtual bool redefine(GLenum target, GLenum internalformat, const gl::Extents &size, bool forceRelease) = 0;

    virtual gl::Error loadData(const gl::Box &area, GLint unpackAlignment, GLenum type, const void *input) = 0;
    virtual gl::Error loadCompressedData(const gl::Box &area, const void *input) = 0;

    virtual gl::Error copy(const gl::Offset &destOffset, const gl::Rectangle &sourceArea, const gl::Framebuffer *source) = 0;
    virtual gl::Error copy(const gl::Offset &destOffset, const gl::Box &sourceArea,
                           const gl::ImageIndex &sourceIndex, TextureStorage *source) = 0;

  protected:
    GLsizei mWidth;
    GLsizei mHeight;
    GLsizei mDepth;
    GLenum mInternalFormat;
    bool mRenderable;
    GLenum mTarget;

    bool mDirty;

  private:
    DISALLOW_COPY_AND_ASSIGN(Image);
};

}

#endif // LIBANGLE_RENDERER_IMAGE_H_

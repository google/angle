//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image11.h: Defines the rx::Image11 class, which acts as the interface to
// the actual underlying resources of a Texture

#ifndef LIBGLESV2_RENDERER_IMAGE11_H_
#define LIBGLESV2_RENDERER_IMAGE11_H_

#include <GLES2/gl2.h>

#include "libGLESv2/renderer/Image.h"

#include "common/debug.h"

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer;
class Renderer11;
class TextureStorage2D;
class TextureStorageCubeMap;

class Image11 : public Image
{
  public:
    Image11();
    virtual ~Image11();

    static Image11 *makeImage11(Image *img);

    virtual bool isDirty() const;

    virtual bool updateSurface(TextureStorage2D *storage, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);
    virtual bool updateSurface(TextureStorageCubeMap *storage, int face, int level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    virtual bool redefine(Renderer *renderer, GLint internalformat, GLsizei width, GLsizei height, bool forceRelease);

    virtual bool isRenderableFormat() const;
    DXGI_FORMAT getDXGIFormat() const;
    
    virtual void loadData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                  GLint unpackAlignment, const void *input);
    virtual void loadCompressedData(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                    const void *input);

    virtual void copy(GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height, gl::Framebuffer *source);

  private:
    DISALLOW_COPY_AND_ASSIGN(Image11);

    void createStagingTexture();
    bool updateStagingTexture(GLint xoffset, GLint yoffset, GLsizei width, GLsizei height);

    HRESULT map(D3D11_MAPPED_SUBRESOURCE *map);
    void unmap();

    Renderer11 *mRenderer;

    DXGI_FORMAT mDXGIFormat;
    ID3D11Texture2D *mStagingTexture;
};

}

#endif // LIBGLESV2_RENDERER_IMAGE11_H_

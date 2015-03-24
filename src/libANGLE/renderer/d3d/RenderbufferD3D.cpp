//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderbufferD3d.cpp: Implements the RenderbufferD3D class, a specialization of RenderbufferImpl


#include "libANGLE/renderer/d3d/RenderbufferD3D.h"

#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/RenderTargetD3D.h"

namespace rx
{
RenderbufferD3D::RenderbufferD3D(RendererD3D *renderer) : mRenderer(renderer)
{
    mRenderTarget = NULL;
}

RenderbufferD3D::~RenderbufferD3D()
{
    SafeDelete(mRenderTarget);
}

RenderbufferD3D *RenderbufferD3D::makeRenderbufferD3D(RenderbufferImpl *renderbuffer)
{
    ASSERT(HAS_DYNAMIC_TYPE(RenderbufferD3D*, renderbuffer));
    return static_cast<RenderbufferD3D*>(renderbuffer);
}

gl::Error RenderbufferD3D::setStorage(GLenum internalformat, size_t width, size_t height)
{
    return setStorageMultisample(0, internalformat, width, height);
}

gl::Error RenderbufferD3D::setStorageMultisample(size_t samples, GLenum internalformat, size_t width, size_t height)
{
    // If the renderbuffer parameters are queried, the calling function
    // will expect one of the valid renderbuffer formats for use in
    // glRenderbufferStorage, but we should create depth and stencil buffers
    // as DEPTH24_STENCIL8
    GLenum creationFormat = internalformat;
    if (internalformat == GL_DEPTH_COMPONENT16 || internalformat == GL_STENCIL_INDEX8)
    {
        creationFormat = GL_DEPTH24_STENCIL8_OES;
    }

    RenderTargetD3D *newRT = NULL;
    gl::Error error = mRenderer->createRenderTarget(width, height, creationFormat, samples, &newRT);
    if (error.isError())
    {
        return error;
    }

    SafeDelete(mRenderTarget);
    mRenderTarget = newRT;

    return gl::Error(GL_NO_ERROR);
}

RenderTargetD3D *RenderbufferD3D::getRenderTarget()
{
    return mRenderTarget;
}

unsigned int RenderbufferD3D::getRenderTargetSerial() const
{
    return (mRenderTarget ? mRenderTarget->getSerial() : 0);
}

}

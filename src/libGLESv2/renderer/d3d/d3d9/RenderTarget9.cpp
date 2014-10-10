//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderTarget9.cpp: Implements a D3D9-specific wrapper for IDirect3DSurface9
// pointers retained by renderbuffers.

#include "libGLESv2/renderer/d3d/d3d9/RenderTarget9.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/formatutils9.h"
#include "libGLESv2/main.h"

namespace rx
{

// TODO: AddRef the incoming surface to take ownership instead of expecting that its ref is being given.
RenderTarget9::RenderTarget9(IDirect3DSurface9 *surface, GLenum internalFormat, GLsizei width, GLsizei height, GLsizei depth,
                             GLsizei samples)
    : mRenderTarget(surface)
{
    mWidth = width;
    mHeight = height;
    mDepth = depth;
    mSamples = samples;
    mInternalFormat = internalFormat;
    mActualFormat = internalFormat;

    if (mRenderTarget)
    {
        D3DSURFACE_DESC description;
        mRenderTarget->GetDesc(&description);

        const d3d9::D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(description.Format);
        mActualFormat = d3dFormatInfo.internalFormat;
    }
}

RenderTarget9::~RenderTarget9()
{
    SafeRelease(mRenderTarget);
}

RenderTarget9 *RenderTarget9::makeRenderTarget9(RenderTarget *target)
{
    ASSERT(HAS_DYNAMIC_TYPE(rx::RenderTarget9*, target));
    return static_cast<rx::RenderTarget9*>(target);
}

void RenderTarget9::invalidate(GLint x, GLint y, GLsizei width, GLsizei height)
{
    // Currently a no-op
}

IDirect3DSurface9 *RenderTarget9::getSurface()
{
    // Caller is responsible for releasing the returned surface reference.
    // TODO: remove the AddRef to match RenderTarget11
    if (mRenderTarget)
    {
        mRenderTarget->AddRef();
    }

    return mRenderTarget;
}

}

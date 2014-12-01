//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer9.cpp: Implements the Framebuffer9 class.

#include "libANGLE/renderer/d3d/d3d9/Framebuffer9.h"
#include "libANGLE/renderer/d3d/d3d9/formatutils9.h"
#include "libANGLE/renderer/d3d/d3d9/TextureStorage9.h"
#include "libANGLE/renderer/d3d/d3d9/Renderer9.h"
#include "libANGLE/renderer/d3d/d3d9/renderer9_utils.h"
#include "libANGLE/renderer/d3d/d3d9/RenderTarget9.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Texture.h"

namespace rx
{

Framebuffer9::Framebuffer9(Renderer9 *renderer)
    : FramebufferD3D(renderer),
      mRenderer(renderer)
{
    ASSERT(mRenderer != nullptr);
}

Framebuffer9::~Framebuffer9()
{
}

gl::Error Framebuffer9::clear(const gl::State &state, const gl::ClearParameters &clearParams)
{
    const gl::FramebufferAttachment *colorBuffer = mColorBuffers[0];
    const gl::FramebufferAttachment *depthStencilBuffer = (mDepthbuffer != nullptr) ? mDepthbuffer
                                                                                    : mStencilbuffer;

    gl::Error error = mRenderer->applyRenderTarget(colorBuffer, depthStencilBuffer);
    if (error.isError())
    {
        return error;
    }

    float nearZ, farZ;
    state.getDepthRange(&nearZ, &farZ);
    mRenderer->setViewport(state.getViewport(), nearZ, farZ, GL_TRIANGLES, state.getRasterizerState().frontFace, true);

    mRenderer->setScissorRectangle(state.getScissor(), state.isScissorTestEnabled());

    return mRenderer->clear(clearParams, mColorBuffers[0], depthStencilBuffer);
}

}

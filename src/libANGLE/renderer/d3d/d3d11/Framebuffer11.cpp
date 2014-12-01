//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer11.cpp: Implements the Framebuffer11 class.

#include "libANGLE/renderer/d3d/d3d11/Framebuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Clear11.h"
#include "libANGLE/renderer/d3d/d3d11/TextureStorage11.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Texture.h"

namespace rx
{

Framebuffer11::Framebuffer11(Renderer11 *renderer)
    : FramebufferD3D(renderer),
      mRenderer(renderer)
{
    ASSERT(mRenderer != nullptr);
}

Framebuffer11::~Framebuffer11()
{
}

static void InvalidateAttachmentSwizzles(const gl::FramebufferAttachment *attachment)
{
    if (attachment && attachment->type() == GL_TEXTURE)
    {
        gl::Texture *texture = attachment->getTexture();

        TextureD3D *textureD3D = TextureD3D::makeTextureD3D(texture->getImplementation());
        TextureStorage *texStorage = textureD3D->getNativeTexture();
        if (texStorage)
        {
            TextureStorage11 *texStorage11 = TextureStorage11::makeTextureStorage11(texStorage);
            if (!texStorage11)
            {
                ERR("texture storage pointer unexpectedly null.");
                return;
            }

            texStorage11->invalidateSwizzleCacheLevel(attachment->mipLevel());
        }
    }
}

void Framebuffer11::invalidateSwizzles()
{
    std::for_each(mColorBuffers.begin(), mColorBuffers.end(), InvalidateAttachmentSwizzles);
    InvalidateAttachmentSwizzles(mDepthbuffer);
    InvalidateAttachmentSwizzles(mStencilbuffer);
}

gl::Error Framebuffer11::clear(const gl::State &state, const gl::ClearParameters &clearParams)
{
    Clear11 *clearer = mRenderer->getClearer();
    gl::Error error = clearer->clearFramebuffer(clearParams, mColorBuffers, mDrawBuffers,
                                                mDepthbuffer, mStencilbuffer);
    if (error.isError())
    {
        return error;
    }

    invalidateSwizzles();

    return gl::Error(GL_NO_ERROR);
}

}

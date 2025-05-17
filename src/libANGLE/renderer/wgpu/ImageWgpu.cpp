//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ImageWgpu.cpp:
//    Implements the class methods for ImageWgpu.
//

#include "libANGLE/renderer/wgpu/ImageWgpu.h"
#include "libANGLE/Display.h"
#include "libANGLE/renderer/wgpu/DisplayWgpu.h"
#include "libANGLE/renderer/wgpu/RenderbufferWgpu.h"
#include "libANGLE/renderer/wgpu/TextureWgpu.h"

#include "common/debug.h"

namespace rx
{

ImageWgpu::ImageWgpu(const egl::ImageState &state, const gl::Context *context)
    : ImageImpl(state), mContext(context)
{}

ImageWgpu::~ImageWgpu() {}

egl::Error ImageWgpu::initialize(const egl::Display *display)
{
    if (egl::IsTextureTarget(mState.target))
    {
        if (mState.imageIndex.getLevelIndex() != 0)
        {
            UNIMPLEMENTED();
            return egl::Error(EGL_BAD_ACCESS,
                              "Creation of EGLImages from non-zero mip levels is unimplemented.");
        }
        if (mState.imageIndex.getType() != gl::TextureType::_2D)
        {
            UNIMPLEMENTED();
            return egl::Error(EGL_BAD_ACCESS,
                              "Creation of EGLImages from non-2D textures is unimplemented.");
        }

        TextureWgpu *textureWgpu = webgpu::GetImpl(GetAs<gl::Texture>(mState.source));

        ASSERT(mContext != nullptr);
        angle::Result initResult = textureWgpu->ensureImageInitialized(mContext);
        if (initResult != angle::Result::Continue)
        {
            return egl::Error(EGL_BAD_ACCESS, "Failed to initialize source texture.");
        }

        mImage = textureWgpu->getImage();
    }
    else if (egl::IsRenderbufferTarget(mState.target))
    {
        RenderbufferWgpu *renderbufferWgpu =
            webgpu::GetImpl(GetAs<gl::Renderbuffer>(mState.source));
        mImage = renderbufferWgpu->getImage();

        ASSERT(mContext != nullptr);
    }
    else if (egl::IsExternalImageTarget(mState.target))
    {
        UNIMPLEMENTED();
    }
    else
    {
        UNREACHABLE();
        return egl::Error(EGL_BAD_ACCESS);
    }

    ASSERT(mImage->isInitialized());
    mOwnsImage = false;

    return egl::NoError();
}

angle::Result ImageWgpu::orphan(const gl::Context *context, egl::ImageSibling *sibling)
{
    if (sibling == mState.source)
    {
        if (egl::IsTextureTarget(mState.target))
        {
            TextureWgpu *textureWgpu = webgpu::GetImpl(GetAs<gl::Texture>(mState.source));
            ASSERT(mImage == textureWgpu->getImage());
            textureWgpu->releaseOwnershipOfImage(context);
        }
        else if (egl::IsRenderbufferTarget(mState.target))
        {
            RenderbufferWgpu *renderbufferWgpu =
                webgpu::GetImpl(GetAs<gl::Renderbuffer>(mState.source));
            ASSERT(mImage == renderbufferWgpu->getImage());
            renderbufferWgpu->releaseOwnershipOfImage(context);
        }
        else
        {
            UNREACHABLE();
            return angle::Result::Stop;
        }

        mOwnsImage = true;
    }

    return angle::Result::Continue;
}

}  // namespace rx

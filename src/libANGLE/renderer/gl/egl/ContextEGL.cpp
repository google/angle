//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/renderer/gl/egl/ContextEGL.h"

#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/renderer/gl/egl/DisplayEGL.h"

namespace rx
{
ContextEGL::ContextEGL(const gl::State &state,
                       gl::ErrorSet *errorSet,
                       DisplayEGL *display,
                       const gl::Context *shareContext,
                       const std::shared_ptr<RendererEGL> &renderer)
    : ContextGL(state, errorSet, renderer),
      mDisplay(display),
      mShareContext(shareContext),
      mRendererEGL(renderer)
{}

ContextEGL::~ContextEGL() {}

angle::Result ContextEGL::initialize()
{
    ANGLE_TRY(ContextGL::initialize());

    if (!mRendererEGL)
    {
        EGLContext nativeShareContext = EGL_NO_CONTEXT;
        if (mShareContext)
        {
            ContextEGL *shareContextEGL = GetImplAs<ContextEGL>(mShareContext);
            nativeShareContext          = shareContextEGL->getContext();
        }

        egl::Error error = mDisplay->createRenderer(nativeShareContext, &mRendererEGL);
        if (error.isError())
        {
            ERR() << "Failed to create a shared renderer: " << error.getMessage();
            return angle::Result::Stop;
        }

        mRenderer = mRendererEGL;
    }

    return angle::Result::Continue;
}

EGLContext ContextEGL::getContext() const
{
    ASSERT(mRendererEGL);
    return mRendererEGL->getContext();
}

std::shared_ptr<RendererEGL> ContextEGL::getRenderer() const
{
    return mRendererEGL;
}

}  // namespace rx

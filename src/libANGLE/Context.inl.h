//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Context.inl.h: Defines inline functions of gl::Context class
// Has to be included after libANGLE/Context.h when using one
// of the defined functions

#ifndef LIBANGLE_CONTEXT_INL_H_
#define LIBANGLE_CONTEXT_INL_H_

#include "libANGLE/GLES1Renderer.h"
#include "libANGLE/renderer/ContextImpl.h"

#define ANGLE_HANDLE_ERR(X) \
    (void)(X);              \
    return;
#define ANGLE_CONTEXT_TRY(EXPR) ANGLE_TRY_TEMPLATE(EXPR, ANGLE_HANDLE_ERR);

namespace gl
{

ANGLE_INLINE angle::Result Context::syncDirtyBits()
{
    const State::DirtyBits &dirtyBits = mGLState.getDirtyBits();
    ANGLE_TRY(mImplementation->syncState(this, dirtyBits, mAllDirtyBits));
    mGLState.clearDirtyBits();
    return angle::Result::Continue;
}

ANGLE_INLINE angle::Result Context::syncDirtyBits(const State::DirtyBits &bitMask)
{
    const State::DirtyBits &dirtyBits = (mGLState.getDirtyBits() & bitMask);
    ANGLE_TRY(mImplementation->syncState(this, dirtyBits, bitMask));
    mGLState.clearDirtyBits(dirtyBits);
    return angle::Result::Continue;
}

ANGLE_INLINE angle::Result Context::syncDirtyObjects(const State::DirtyObjects &objectMask)
{
    return mGLState.syncDirtyObjects(this, objectMask);
}

ANGLE_INLINE angle::Result Context::prepareForDraw(PrimitiveMode mode)
{
    if (mGLES1Renderer)
    {
        ANGLE_TRY(mGLES1Renderer->prepareForDraw(mode, this, &mGLState));
    }

    ANGLE_TRY(syncDirtyObjects(mDrawDirtyObjects));
    ASSERT(!isRobustResourceInitEnabled() ||
           !mGLState.getDrawFramebuffer()->hasResourceThatNeedsInit());
    return syncDirtyBits();
}

ANGLE_INLINE void Context::drawElements(PrimitiveMode mode,
                                        GLsizei count,
                                        DrawElementsType type,
                                        const void *indices)
{
    // No-op if count draws no primitives for given mode
    if (noopDraw(mode, count))
    {
        return;
    }

    ANGLE_CONTEXT_TRY(prepareForDraw(mode));
    ANGLE_CONTEXT_TRY(mImplementation->drawElements(this, mode, count, type, indices));
}

}  // namespace gl

#endif  // LIBANGLE_CONTEXT_INL_H_

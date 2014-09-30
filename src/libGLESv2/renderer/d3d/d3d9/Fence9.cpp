//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence9.cpp: Defines the rx::Fence9 class.

#include "libGLESv2/renderer/d3d/d3d9/Fence9.h"
#include "libGLESv2/renderer/d3d/d3d9/renderer9_utils.h"
#include "libGLESv2/renderer/d3d/d3d9/Renderer9.h"
#include "libGLESv2/main.h"

namespace rx
{

Fence9::Fence9(rx::Renderer9 *renderer)
    : mRenderer(renderer),
      mQuery(NULL)
{
}

Fence9::~Fence9()
{
    SafeRelease(mQuery);
}

gl::Error Fence9::set()
{
    if (!mQuery)
    {
        gl::Error error = mRenderer->allocateEventQuery(&mQuery);
        if (error.isError())
        {
            return error;
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    if (FAILED(result))
    {
        ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
        SafeRelease(mQuery);
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to end event query, result: 0x%X.", result);
    }

    return gl::Error(GL_NO_ERROR);
}

gl::Error Fence9::test(bool flushCommandBuffer, GLboolean *outFinished)
{
    ASSERT(mQuery);

    DWORD getDataFlags = (flushCommandBuffer ? D3DGETDATA_FLUSH : 0);
    HRESULT result = mQuery->GetData(NULL, 0, getDataFlags);

    if (d3d9::isDeviceLostError(result))
    {
        mRenderer->notifyDeviceLost();
        return gl::Error(GL_OUT_OF_MEMORY, "Device was lost while querying result of an event query.");
    }
    else if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to get query data, result: 0x%X.", result);
    }

    ASSERT(result == S_OK || result == S_FALSE);
    *outFinished = ((result == S_OK) ? GL_TRUE : GL_FALSE);
    return gl::Error(GL_NO_ERROR);
}

}

//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Fence11.cpp: Defines the rx::Fence11 class which implements rx::FenceImpl.

#include "libGLESv2/renderer/d3d/d3d11/Fence11.h"
#include "libGLESv2/renderer/d3d/d3d11/Renderer11.h"
#include "libGLESv2/main.h"

namespace rx
{

Fence11::Fence11(rx::Renderer11 *renderer)
    : mRenderer(renderer),
      mQuery(NULL)
{
}

Fence11::~Fence11()
{
    SafeRelease(mQuery);
}

gl::Error Fence11::set()
{
    if (!mQuery)
    {
        D3D11_QUERY_DESC queryDesc;
        queryDesc.Query = D3D11_QUERY_EVENT;
        queryDesc.MiscFlags = 0;

        HRESULT result = mRenderer->getDevice()->CreateQuery(&queryDesc, &mQuery);
        if (FAILED(result))
        {
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to create event query, result: 0x%X.", result);
        }
    }

    mRenderer->getDeviceContext()->End(mQuery);
    return gl::Error(GL_NO_ERROR);
}

gl::Error Fence11::test(bool flushCommandBuffer, GLboolean *outFinished)
{
    ASSERT(mQuery);

    UINT getDataFlags = (flushCommandBuffer ? 0 : D3D11_ASYNC_GETDATA_DONOTFLUSH);
    HRESULT result = mRenderer->getDeviceContext()->GetData(mQuery, NULL, 0, getDataFlags);

    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Failed to get query data, result: 0x%X.", result);
    }
    else if (mRenderer->isDeviceLost())
    {
        return gl::Error(GL_OUT_OF_MEMORY, "Device was lost while querying result of an event query.");
    }

    ASSERT(result == S_OK || result == S_FALSE);
    *outFinished = ((result == S_OK) ? GL_TRUE : GL_FALSE);
    return gl::Error(GL_NO_ERROR);
}

}

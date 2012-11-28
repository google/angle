//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Query.cpp: Implements the gl::Query class

#include "libGLESv2/Query.h"

#include "libGLESv2/renderer/renderer9_utils.h"

namespace gl
{

Query::Query(rx::Renderer *renderer, GLuint id, GLenum type) : RefCountObject(id)
{ 
    ASSERT(dynamic_cast<rx::Renderer9*>(renderer) != NULL); // D3D9_REPLACE
    mRenderer = static_cast<rx::Renderer9*>(renderer);

    mQuery = NULL;
    mStatus = GL_FALSE;
    mResult = GL_FALSE;
    mType = type;
}

Query::~Query()
{
    if (mQuery != NULL)
    {
        mQuery->Release();
        mQuery = NULL;
    }
}

void Query::begin()
{
    if (mQuery == NULL)
    {
        // D3D9_REPLACE
        if (FAILED(mRenderer->getDevice()->CreateQuery(D3DQUERYTYPE_OCCLUSION, &mQuery)))
        {
            return error(GL_OUT_OF_MEMORY);
        }
    }

    HRESULT result = mQuery->Issue(D3DISSUE_BEGIN);
    ASSERT(SUCCEEDED(result));
}

void Query::end()
{
    if (mQuery == NULL)
    {
        return error(GL_INVALID_OPERATION);
    }

    HRESULT result = mQuery->Issue(D3DISSUE_END);
    ASSERT(SUCCEEDED(result));
    
    mStatus = GL_FALSE;
    mResult = GL_FALSE;
}

GLuint Query::getResult()
{
    if (mQuery != NULL)
    {
        while (!testQuery())
        {
            Sleep(0);
            // explicitly check for device loss
            // some drivers seem to return S_FALSE even if the device is lost
            // instead of D3DERR_DEVICELOST like they should
            if (mRenderer->testDeviceLost(true))
            {
                return error(GL_OUT_OF_MEMORY, 0);
            }
        }
    }

    return (GLuint)mResult;
}

GLboolean Query::isResultAvailable()
{
    if (mQuery != NULL)
    {
        testQuery();
    }
    
    return mStatus;
}

GLenum Query::getType() const
{
    return mType;
}

GLboolean Query::testQuery()
{
    if (mQuery != NULL && mStatus != GL_TRUE)
    {
        DWORD numPixels = 0;

        HRESULT hres = mQuery->GetData(&numPixels, sizeof(DWORD), D3DGETDATA_FLUSH);
        if (hres == S_OK)
        {
            mStatus = GL_TRUE;

            switch (mType)
            {
              case GL_ANY_SAMPLES_PASSED_EXT:
              case GL_ANY_SAMPLES_PASSED_CONSERVATIVE_EXT:
                mResult = (numPixels > 0) ? GL_TRUE : GL_FALSE;
                break;
              default:
                ASSERT(false);
            }
        }
        else if (checkDeviceLost(hres))
        {
            return error(GL_OUT_OF_MEMORY, GL_TRUE);
        }
        
        return mStatus;
    }

    return GL_TRUE; // prevent blocking when query is null
}
}

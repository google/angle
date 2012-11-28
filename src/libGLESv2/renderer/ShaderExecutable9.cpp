//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable9.cpp: Implements a D3D9-specific class to contain shader
// executable implementation details.

#include "libGLESv2/renderer/ShaderExecutable9.h"

#include "common/debug.h"

namespace rx
{

ShaderExecutable9::ShaderExecutable9(IDirect3DPixelShader9 *executable, gl::D3DConstantTable *constantTable)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
    mConstantTable = constantTable;
}

ShaderExecutable9::ShaderExecutable9(IDirect3DVertexShader9 *executable, gl::D3DConstantTable *constantTable)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
    mConstantTable = constantTable;
}

ShaderExecutable9::~ShaderExecutable9()
{
    if (mVertexExecutable)
    {
        mVertexExecutable->Release();
    }
    if (mPixelExecutable)
    {
        mPixelExecutable->Release();
    }

    delete mConstantTable;
}

ShaderExecutable9 *ShaderExecutable9::makeShaderExecutable9(ShaderExecutable *executable)
{
    ASSERT(dynamic_cast<ShaderExecutable9*>(executable) != NULL);
    return static_cast<ShaderExecutable9*>(executable);
}

bool ShaderExecutable9::getVertexFunction(void *pData, UINT *pSizeOfData)
{
    HRESULT hr = D3DERR_INVALIDCALL;
    if (mVertexExecutable)
    {
        hr = mVertexExecutable->GetFunction(pData, pSizeOfData);
    }
    return SUCCEEDED(hr);
}

bool ShaderExecutable9::getPixelFunction(void *pData, UINT *pSizeOfData)
{
    HRESULT hr = D3DERR_INVALIDCALL;
    if (mPixelExecutable)
    {
        hr = mPixelExecutable->GetFunction(pData, pSizeOfData);
    }
    return SUCCEEDED(hr);
}

IDirect3DVertexShader9 *ShaderExecutable9::getVertexShader()
{
    return mVertexExecutable;
}

IDirect3DPixelShader9 *ShaderExecutable9::getPixelShader()
{
    return mPixelExecutable;
}

gl::D3DConstantTable *ShaderExecutable9::getConstantTable()
{
    return mConstantTable;
}

}
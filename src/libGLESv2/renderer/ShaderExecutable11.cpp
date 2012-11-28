//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable11.cpp: Implements a D3D11-specific class to contain shader
// executable implementation details.

#include "libGLESv2/renderer/ShaderExecutable11.h"

#include "common/debug.h"

namespace rx
{

ShaderExecutable11::ShaderExecutable11(ID3D11PixelShader *executable)
{
    mPixelExecutable = executable;
    mVertexExecutable = NULL;
}

ShaderExecutable11::ShaderExecutable11(ID3D11VertexShader *executable)
{
    mVertexExecutable = executable;
    mPixelExecutable = NULL;
}

ShaderExecutable11::~ShaderExecutable11()
{
    if (mVertexExecutable)
    {
        mVertexExecutable->Release();
    }
    if (mPixelExecutable)
    {
        mPixelExecutable->Release();
    }
}

ShaderExecutable11 *ShaderExecutable11::makeShaderExecutable11(ShaderExecutable *executable)
{
    ASSERT(dynamic_cast<ShaderExecutable11*>(executable) != NULL);
    return static_cast<ShaderExecutable11*>(executable);
}

bool ShaderExecutable11::getVertexFunction(void *pData, UINT *pSizeOfData)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool ShaderExecutable11::getPixelFunction(void *pData, UINT *pSizeOfData)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

ID3D11VertexShader *ShaderExecutable11::getVertexShader()
{
    return mVertexExecutable;
}

ID3D11PixelShader *ShaderExecutable11::getPixelShader()
{
    return mPixelExecutable;
}

gl::D3DConstantTable *ShaderExecutable11::getConstantTable()
{
    return NULL;
}

}
//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable.h: Defines a renderer-agnostic class to contain shader
// executable implementation details.

#ifndef LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_
#define LIBGLESV2_RENDERER_SHADEREXECUTABLE_H_

#include "common/angleutils.h"
#include "libGLESv2/D3DConstantTable.h"

namespace rx
{

class ShaderExecutable
{
  public:
    ShaderExecutable() {};
    virtual ~ShaderExecutable() {};

    virtual bool getVertexFunction(void *pData, UINT *pSizeOfData) = 0;
    virtual bool getPixelFunction(void *pData, UINT *pSizeOfData) = 0;

    virtual gl::D3DConstantTable *getConstantTable() = 0; // D3D9_REMOVE

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable);
};

}

#endif // LIBGLESV2_RENDERER_SHADEREXECUTABLE9_H_

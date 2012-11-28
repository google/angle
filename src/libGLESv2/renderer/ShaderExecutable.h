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

namespace rx
{

class ShaderExecutable
{
  public:
    ShaderExecutable() {};
    virtual ~ShaderExecutable() {};

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderExecutable);
};

}

#endif // LIBGLESV2_RENDERER_SHADEREXECUTABLE9_H_
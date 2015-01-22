//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderGL.h: Defines the class interface for ShaderGL.

#ifndef LIBANGLE_RENDERER_GL_SHADERGL_H_
#define LIBANGLE_RENDERER_GL_SHADERGL_H_

#include "libANGLE/renderer/ShaderImpl.h"

namespace rx
{

class ShaderGL : public ShaderImpl
{
  public:
    ShaderGL();
    ~ShaderGL() override;

    bool compile(gl::Compiler *compiler, const std::string &source) override;
    std::string getDebugInfo() const override;

  private:
    DISALLOW_COPY_AND_ASSIGN(ShaderGL);
};

}

#endif // LIBANGLE_RENDERER_GL_SHADERGL_H_

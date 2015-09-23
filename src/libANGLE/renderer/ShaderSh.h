//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderSh:
//   Common class representing a shader compile with ANGLE's translator.
//   TODO(jmadill): Move this to the GL layer.
//

#ifndef LIBANGLE_RENDERER_SHADERSH_H_
#define LIBANGLE_RENDERER_SHADERSH_H_

#include "libANGLE/renderer/ShaderImpl.h"

namespace gl
{
struct Limitations;
}

namespace rx
{

class ShaderSh : public ShaderImpl
{
  public:
    ShaderSh(gl::Shader::Data *data, const gl::Limitations &rendererLimitations);
    ~ShaderSh();

    bool compile(gl::Compiler *compiler, const std::string &source, int additionalOptions) override;

  protected:
    const gl::Limitations &mRendererLimitations;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_SHADERSH_H_

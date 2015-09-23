//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderImpl.h: Defines the abstract rx::ShaderImpl class.

#ifndef LIBANGLE_RENDERER_SHADERIMPL_H_
#define LIBANGLE_RENDERER_SHADERIMPL_H_

#include "common/angleutils.h"
#include "libANGLE/Shader.h"

namespace rx
{

class ShaderImpl : angle::NonCopyable
{
  public:
    ShaderImpl(gl::Shader::Data *data) : mData(data) {}
    virtual ~ShaderImpl() { }

    virtual bool compile(gl::Compiler *compiler,
                         const std::string &source,
                         int additionalOptions) = 0;
    virtual std::string getDebugInfo() const = 0;

  protected:
    // TODO(jmadill): Use a const reference when possible.
    gl::Shader::Data *mData;
};

}

#endif // LIBANGLE_RENDERER_SHADERIMPL_H_

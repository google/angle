//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderNULL.cpp:
//    Implements the class methods for ShaderNULL.
//

#include "libANGLE/renderer/null/ShaderNULL.h"

#include "common/debug.h"

namespace rx
{

ShaderNULL::ShaderNULL(const gl::ShaderState &data) : ShaderImpl(data)
{
}

ShaderNULL::~ShaderNULL()
{
}

ShCompileOptions ShaderNULL::prepareSourceAndReturnOptions(const gl::Context *context,
                                                           std::stringstream *sourceStream,
                                                           std::string *sourcePath)
{
    *sourceStream << mData.getSource();
    return 0;
}

bool ShaderNULL::postTranslateCompile(const gl::Context *context,
                                      gl::Compiler *compiler,
                                      std::string *infoLog)
{
    return true;
}

std::string ShaderNULL::getDebugInfo(const gl::Context *context) const
{
    return "";
}

}  // namespace rx

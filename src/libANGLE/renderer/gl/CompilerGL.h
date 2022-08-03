//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// CompilerGL.h: Defines the class interface for CompilerGL.

#ifndef LIBANGLE_RENDERER_GL_COMPILERGL_H_
#define LIBANGLE_RENDERER_GL_COMPILERGL_H_

#include "libANGLE/renderer/CompilerImpl.h"

namespace rx
{
class ContextGL;
class FunctionsGL;

class CompilerGL : public CompilerImpl
{
  public:
    CompilerGL(const ContextGL *);
    ~CompilerGL() override {}

    ShShaderOutput getTranslatorOutputType() const override;
    CompilerBackendFeatures getBackendFeatures() const override { return mBackendFeatures; }

  private:
    ShShaderOutput mTranslatorOutputType;
    CompilerBackendFeatures mBackendFeatures;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_GL_COMPILERGL_H_

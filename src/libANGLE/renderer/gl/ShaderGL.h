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
class FunctionsGL;
struct WorkaroundsGL;
enum class MultiviewImplementationTypeGL;

class ShaderGL : public ShaderImpl
{
  public:
    ShaderGL(const gl::ShaderState &data,
             GLuint shaderID,
             MultiviewImplementationTypeGL multiviewImplementationType,
             const FunctionsGL *functions);
    ~ShaderGL() override;

    void destroy() override;

    // ShaderImpl implementation
    ShCompileOptions prepareSourceAndReturnOptions(const gl::Context *context,
                                                   std::stringstream *sourceStream,
                                                   std::string *sourcePath) override;
    bool postTranslateCompile(gl::ShCompilerInstance *compiler, std::string *infoLog) override;
    std::string getDebugInfo() const override;

    GLuint getShaderID() const;

  private:
    GLuint mShaderID;
    MultiviewImplementationTypeGL mMultiviewImplementationType;
    const FunctionsGL *mFunctions;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_GL_SHADERGL_H_

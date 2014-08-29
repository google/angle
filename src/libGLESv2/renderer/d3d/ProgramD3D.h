//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.h: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#ifndef LIBGLESV2_RENDERER_PROGRAMD3D_H_
#define LIBGLESV2_RENDERER_PROGRAMD3D_H_

#include "libGLESv2/renderer/ProgramImpl.h"

#include <string>
#include <vector>

namespace gl
{
struct LinkedUniform;
struct VariableLocation;
struct VertexFormat;
}

namespace rx
{

class UniformStorage;

class ProgramD3D : public ProgramImpl
{
  public:
    ProgramD3D(rx::Renderer *renderer);
    virtual ~ProgramD3D();

    static ProgramD3D *makeProgramD3D(ProgramImpl *impl);
    static const ProgramD3D *makeProgramD3D(const ProgramImpl *impl);

    Renderer *getRenderer() { return mRenderer; }
    DynamicHLSL *getDynamicHLSL() { return mDynamicHLSL; }
    const std::vector<rx::PixelShaderOutputVariable> &getPixelShaderKey() { return mPixelShaderKey; }

    GLenum getBinaryFormat() { return GL_PROGRAM_BINARY_ANGLE; }
    bool load(gl::InfoLog &infoLog, gl::BinaryInputStream *stream);
    bool save(gl::BinaryOutputStream *stream);

    ShaderExecutable *getPixelExecutableForOutputLayout(gl::InfoLog &infoLog, const std::vector<GLenum> &outputSignature,
                                                        const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                        bool separatedOutputBuffers);
    ShaderExecutable *getVertexExecutableForInputLayout(gl::InfoLog &infoLog,
                                                        const gl::VertexFormat inputLayout[gl::MAX_VERTEX_ATTRIBS],
                                                        const sh::Attribute shaderAttributes[],
                                                        const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                                        bool separatedOutputBuffers);

    bool link(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
              const std::vector<std::string> &transformFeedbackVaryings, int *registers,
              std::vector<gl::LinkedVarying> *linkedVaryings, std::map<int, gl::VariableLocation> *outputVariables);

    // D3D only
    void initializeUniformStorage(const std::vector<gl::LinkedUniform*> &uniforms);

    const UniformStorage &getVertexUniformStorage() const { return *mVertexUniformStorage; }
    const UniformStorage &getFragmentUniformStorage() const { return *mFragmentUniformStorage; }

    void reset();

  private:
    DISALLOW_COPY_AND_ASSIGN(ProgramD3D);

    Renderer *mRenderer;
    DynamicHLSL *mDynamicHLSL;

    std::string mVertexHLSL;
    rx::D3DWorkaroundType mVertexWorkarounds;

    std::string mPixelHLSL;
    rx::D3DWorkaroundType mPixelWorkarounds;
    bool mUsesFragDepth;
    std::vector<rx::PixelShaderOutputVariable> mPixelShaderKey;

    UniformStorage *mVertexUniformStorage;
    UniformStorage *mFragmentUniformStorage;
};

}

#endif // LIBGLESV2_RENDERER_PROGRAMD3D_H_

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

    const std::vector<rx::PixelShaderOutputVariable> &getPixelShaderKey() { return mPixelShaderKey; }
    int getShaderVersion() const { return mShaderVersion; }

    bool usesPointSize() const;
    bool usesPointSpriteEmulation() const;
    bool usesGeometryShader() const;

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
    ShaderExecutable *getGeometryExecutable(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
                                            const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                            bool separatedOutputBuffers, int registers);
    ShaderExecutable *loadExecutable(const void *function, size_t length, rx::ShaderType type,
                                     const std::vector<gl::LinkedVarying> &transformFeedbackLinkedVaryings,
                                     bool separatedOutputBuffers);

    bool link(gl::InfoLog &infoLog, gl::Shader *fragmentShader, gl::Shader *vertexShader,
              const std::vector<std::string> &transformFeedbackVaryings, int *registers,
              std::vector<gl::LinkedVarying> *linkedVaryings, std::map<int, gl::VariableLocation> *outputVariables);

    void getInputLayoutSignature(const gl::VertexFormat inputLayout[], GLenum signature[]) const;

    void initializeUniformStorage(const std::vector<gl::LinkedUniform*> &uniforms);
    gl::Error applyUniforms(const std::vector<gl::LinkedUniform*> &uniforms);
    gl::Error applyUniformBuffers(const std::vector<gl::UniformBlock*> uniformBlocks, const std::vector<gl::Buffer*> boundBuffers,
                             const gl::Caps &caps);
    bool assignUniformBlockRegister(gl::InfoLog &infoLog, gl::UniformBlock *uniformBlock, GLenum shader,
                                    unsigned int registerIndex, const gl::Caps &caps);
    unsigned int getReservedUniformVectors(GLenum shader);

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

    bool mUsesPointSize;

    UniformStorage *mVertexUniformStorage;
    UniformStorage *mFragmentUniformStorage;

    int mShaderVersion;
};

}

#endif // LIBGLESV2_RENDERER_PROGRAMD3D_H_

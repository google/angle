//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Program.h: Defines the gl::Program class. Implements GL program objects
// and related functionality. [OpenGL ES 2.0.24] section 2.10.3 page 28.

#ifndef LIBGLESV2_PROGRAM_BINARY_H_
#define LIBGLESV2_PROGRAM_BINARY_H_

#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>
#include <vector>

#include "common/RefCountObject.h"
#include "angletypes.h"
#include "common/mathutil.h"
#include "libGLESv2/Uniform.h"
#include "libGLESv2/Shader.h"
#include "libGLESv2/Constants.h"

namespace rx
{
class ShaderExecutable;
class Renderer;
struct TranslatedAttribute;
}

namespace gl
{
class FragmentShader;
class VertexShader;
class InfoLog;
class AttributeBindings;
struct Varying;
class Buffer;

// Struct used for correlating uniforms/elements of uniform arrays to handles
struct VariableLocation
{
    VariableLocation()
    {
    }

    VariableLocation(const std::string &name, unsigned int element, unsigned int index);

    std::string name;
    unsigned int element;
    unsigned int index;
};

// This is the result of linking a program. It is the state that would be passed to ProgramBinary.
class ProgramBinary : public RefCountObject
{
  public:
    explicit ProgramBinary(rx::Renderer *renderer);
    ~ProgramBinary();

    rx::ShaderExecutable *getPixelExecutable();
    rx::ShaderExecutable *getVertexExecutable();
    rx::ShaderExecutable *getGeometryExecutable();

    GLuint getAttributeLocation(const char *name);
    int getSemanticIndex(int attributeIndex);

    GLint getSamplerMapping(SamplerType type, unsigned int samplerIndex);
    TextureType getSamplerTextureType(SamplerType type, unsigned int samplerIndex);
    GLint getUsedSamplerRange(SamplerType type);
    bool usesPointSize() const;
    bool usesPointSpriteEmulation() const;
    bool usesGeometryShader() const;

    GLint getUniformLocation(std::string name);
    GLuint getUniformIndex(std::string name);
    GLuint getUniformBlockIndex(std::string name);
    bool setUniform1fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform2fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform3fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform4fv(GLint location, GLsizei count, const GLfloat *v);
    bool setUniform1iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform2iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform3iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform4iv(GLint location, GLsizei count, const GLint *v);
    bool setUniform1uiv(GLint location, GLsizei count, const GLuint *v);
    bool setUniform2uiv(GLint location, GLsizei count, const GLuint *v);
    bool setUniform3uiv(GLint location, GLsizei count, const GLuint *v);
    bool setUniform4uiv(GLint location, GLsizei count, const GLuint *v);
    bool setUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix2x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix3x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix2x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix4x2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix3x4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
    bool setUniformMatrix4x3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

    bool getUniformfv(GLint location, GLsizei *bufSize, GLfloat *params);
    bool getUniformiv(GLint location, GLsizei *bufSize, GLint *params);
    bool getUniformuiv(GLint location, GLsizei *bufSize, GLuint *params);

    void dirtyAllUniforms();
    void applyUniforms();
    bool applyUniformBuffers(const std::vector<gl::Buffer*> boundBuffers);

    bool load(InfoLog &infoLog, const void *binary, GLsizei length);
    bool save(void* binary, GLsizei bufSize, GLsizei *length);
    GLint getLength();

    bool link(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader);
    void getAttachedShaders(GLsizei maxCount, GLsizei *count, GLuint *shaders);

    void getActiveAttribute(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
    GLint getActiveAttributeCount() const;
    GLint getActiveAttributeMaxLength() const;

    void getActiveUniform(GLuint index, GLsizei bufsize, GLsizei *length, GLint *size, GLenum *type, GLchar *name) const;
    GLint getActiveUniformCount() const;
    GLint getActiveUniformMaxLength() const;
    GLint getActiveUniformi(GLuint index, GLenum pname) const;

    void getActiveUniformBlockName(GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName) const;
    void getActiveUniformBlockiv(GLuint uniformBlockIndex, GLenum pname, GLint *params) const;
    GLuint getActiveUniformBlockCount() const;
    GLuint getActiveUniformBlockMaxLength() const;
    UniformBlock *getUniformBlockByIndex(GLuint blockIndex);

    GLint getFragDataLocation(const char *name) const;

    void validate(InfoLog &infoLog);
    bool validateSamplers(InfoLog *infoLog);
    bool isValidated() const;

    unsigned int getSerial() const;
    int getShaderVersion() const;

    void sortAttributesByLayout(rx::TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS], int sortedSemanticIndices[MAX_VERTEX_ATTRIBS]) const;

    static std::string decorateAttribute(const std::string &name);    // Prepend an underscore

  private:
    DISALLOW_COPY_AND_ASSIGN(ProgramBinary);

    int packVaryings(InfoLog &infoLog, const Varying *packing[][4], FragmentShader *fragmentShader);
    bool linkVaryings(InfoLog &infoLog, int registers, const Varying *packing[][4],
                      std::string& pixelHLSL, std::string& vertexHLSL,
                      FragmentShader *fragmentShader, VertexShader *vertexShader);
    std::string generateVaryingHLSL(FragmentShader *fragmentShader, const std::string &varyingSemantic) const;

    bool linkAttributes(InfoLog &infoLog, const AttributeBindings &attributeBindings, FragmentShader *fragmentShader, VertexShader *vertexShader);

    typedef sh::BlockMemberInfoArray::const_iterator BlockInfoItr;

    bool areMatchingUniforms(InfoLog &infoLog, const std::string& uniformName, const sh::Uniform &vertexUniform, const sh::Uniform &fragmentUniform);
    bool linkUniforms(InfoLog &infoLog, const sh::ActiveUniforms &vertexUniforms, const sh::ActiveUniforms &fragmentUniforms);
    bool defineUniform(GLenum shader, const sh::Uniform &constant, InfoLog &infoLog);
    bool areMatchingInterfaceBlocks(InfoLog &infoLog, const sh::InterfaceBlock &vertexInterfaceBlock, const sh::InterfaceBlock &fragmentInterfaceBlock);
    bool linkUniformBlocks(InfoLog &infoLog, const sh::ActiveInterfaceBlocks &vertexUniformBlocks, const sh::ActiveInterfaceBlocks &fragmentUniformBlocks);
    void defineUniformBlockMembers(const sh::ActiveUniforms &uniforms, const std::string &prefix, int blockIndex, BlockInfoItr *blockInfoItr, std::vector<unsigned int> *blockUniformIndexes);
    bool defineUniformBlock(InfoLog &infoLog, GLenum shader, const sh::InterfaceBlock &interfaceBlock);
    bool assignUniformBlockRegister(InfoLog &infoLog, UniformBlock *uniformBlock, GLenum shader, unsigned int registerIndex);
    void defineOutputVariables(FragmentShader *fragmentShader);

    std::string generateGeometryShaderHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const;
    std::string generatePointSpriteHLSL(int registers, const Varying *packing[][4], FragmentShader *fragmentShader, VertexShader *vertexShader) const;

    template <typename T>
    bool setUniform(GLint location, GLsizei count, const T* v, GLenum targetUniformType);

    template <int cols, int rows>
    bool setUniformMatrixfv(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value, GLenum targetUniformType);

    template <typename T>
    bool getUniformv(GLint location, GLsizei *bufSize, T *params, GLenum uniformType);

    static TextureType getTextureType(GLenum samplerType, InfoLog &infoLog);

    rx::Renderer *const mRenderer;

    rx::ShaderExecutable *mPixelExecutable;
    rx::ShaderExecutable *mVertexExecutable;
    rx::ShaderExecutable *mGeometryExecutable;

    sh::ShaderVariable mLinkedAttribute[MAX_VERTEX_ATTRIBS];
    int mSemanticIndex[MAX_VERTEX_ATTRIBS];

    struct Sampler
    {
        Sampler();

        bool active;
        GLint logicalTextureUnit;
        TextureType textureType;
    };

    Sampler mSamplersPS[MAX_TEXTURE_IMAGE_UNITS];
    Sampler mSamplersVS[IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS];
    GLuint mUsedVertexSamplerRange;
    GLuint mUsedPixelSamplerRange;
    bool mUsesPointSize;
    int mShaderVersion;

    UniformArray mUniforms;
    UniformBlockArray mUniformBlocks;
    typedef std::vector<VariableLocation> UniformIndex;
    UniformIndex mUniformIndex;
    typedef std::map<int, VariableLocation> ShaderVariableIndex;
    ShaderVariableIndex mOutputVariables;

    bool mValidated;

    const unsigned int mSerial;

    static unsigned int issueSerial();
    static unsigned int mCurrentSerial;
};
}

#endif   // LIBGLESV2_PROGRAM_BINARY_H_

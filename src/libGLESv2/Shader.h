//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Shader.h: Defines the abstract gl::Shader class and its concrete derived
// classes VertexShader and FragmentShader. Implements GL shader objects and
// related functionality. [OpenGL ES 2.0.24] section 2.10 page 24 and section
// 3.8 page 84.

#ifndef LIBGLESV2_SHADER_H_
#define LIBGLESV2_SHADER_H_

#include "Context.h"

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3dx9.h>

namespace gl
{
class Shader
{
  public:
    Shader();

    virtual ~Shader();

    virtual GLenum getType() = 0;

    void deleteSource();
    void setSource(GLsizei count, const char **string, const GLint *length);

    virtual void compile() = 0;
    bool isCompiled();
    const char *linkHLSL();

    void attach();
    void detach();
    bool isAttached() const;
    bool isDeletable() const;
    void flagForDeletion();

    static void releaseCompiler();

  protected:
    DISALLOW_COPY_AND_ASSIGN(Shader);

    void compileToHLSL(void *compiler);

    int mAttachCount;     // Number of program objects this shader is attached to
    bool mDeleteStatus;   // Flag to indicate that the shader can be deleted when no longer in use

    char *mSource;
    char *mHlsl;
    char *mErrors;

    static void *mFragmentCompiler;
    static void *mVertexCompiler;
};

class InputMapping
{
  public:
    InputMapping();
    InputMapping(const char *attribute, int semanticIndex);

    ~InputMapping();

    InputMapping &operator=(const InputMapping &inputMapping);

    char *mAttribute;
    int mSemanticIndex;   // TEXCOORDi
};

class VertexShader : public Shader
{
  public:
    VertexShader();

    ~VertexShader();

    GLenum getType();
    void compile();
    const char *linkHLSL(const char *pixelHLSL);
    const char *getAttributeName(unsigned int attributeIndex);
    bool isActiveAttribute(const char *attributeName);
    int getInputMapping(const char *attributeName);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexShader);

    void parseAttributes();

    InputMapping mInputMapping[MAX_VERTEX_ATTRIBS];
};

class FragmentShader : public Shader
{
  public:
    FragmentShader();

    ~FragmentShader();

    GLenum getType();
    void compile();

  private:
    DISALLOW_COPY_AND_ASSIGN(FragmentShader);
};
}

#endif   // LIBGLESV2_SHADER_H_

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

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3dx9.h>

#include "libGLESv2/Context.h"

namespace gl
{
class Shader
{
  public:
    explicit Shader(GLuint handle);

    virtual ~Shader();

    virtual GLenum getType() = 0;
    GLuint getHandle() const;

    void deleteSource();
    void setSource(GLsizei count, const char **string, const GLint *length);
    int getInfoLogLength() const;
    void getInfoLog(GLsizei bufSize, GLsizei *length, char *infoLog);
    int getSourceLength() const;
    void getSource(GLsizei bufSize, GLsizei *length, char *source);

    virtual void compile() = 0;
    bool isCompiled();
    const char *getHLSL();

    void attach();
    void detach();
    bool isAttached() const;
    bool isDeletable() const;
    bool isFlaggedForDeletion() const;
    void flagForDeletion();

    static void releaseCompiler();

  protected:
    DISALLOW_COPY_AND_ASSIGN(Shader);

    void compileToHLSL(void *compiler);

    const GLuint mHandle;
    int mAttachCount;     // Number of program objects this shader is attached to
    bool mDeleteStatus;   // Flag to indicate that the shader can be deleted when no longer in use

    char *mSource;
    char *mHlsl;
    char *mInfoLog;

    static void *mFragmentCompiler;
    static void *mVertexCompiler;
};

class VertexShader : public Shader
{
  public:
    explicit VertexShader(GLuint handle);

    ~VertexShader();

    GLenum getType();
    void compile();
    const char *getAttributeName(unsigned int attributeIndex);
    int getSemanticIndex(const std::string &attributeName);

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexShader);

    void parseAttributes();

    std::string mAttributeName[MAX_VERTEX_ATTRIBS + 1];   // One extra to report link error
};

class FragmentShader : public Shader
{
  public:
    explicit FragmentShader(GLuint handle);

    ~FragmentShader();

    GLenum getType();
    void compile();

  private:
    DISALLOW_COPY_AND_ASSIGN(FragmentShader);
};
}

#endif   // LIBGLESV2_SHADER_H_

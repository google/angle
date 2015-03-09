//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerGL.h: Defines a class for caching applied OpenGL state

#ifndef LIBANGLE_RENDERER_GL_STATEMANAGERGL_H_
#define LIBANGLE_RENDERER_GL_STATEMANAGERGL_H_

#include "common/debug.h"
#include "libANGLE/Error.h"
#include "libANGLE/renderer/gl/functionsgl_typedefs.h"

#include <map>

namespace gl
{
struct Caps;
struct Data;
}

namespace rx
{

class FunctionsGL;

class StateManagerGL
{
  public:
    StateManagerGL(const FunctionsGL *functions, const gl::Caps &rendererCaps);

    void useProgram(GLuint program);
    void bindVertexArray(GLuint vao);
    void bindBuffer(GLenum type, GLuint buffer);
    void activeTexture(size_t unit);
    void bindTexture(GLenum type, GLuint texture);
    void setPixelUnpackState(GLint alignment, GLint rowLength);

    gl::Error setDrawArraysState(const gl::Data &data, GLint first, GLsizei count);
    gl::Error setDrawElementsState(const gl::Data &data, GLsizei count, GLenum type, const GLvoid *indices,
                                   const GLvoid **outIndices);

  private:
    DISALLOW_COPY_AND_ASSIGN(StateManagerGL);

    gl::Error setGenericDrawState(const gl::Data &data);

    const FunctionsGL *mFunctions;

    GLuint mProgram;
    GLuint mVAO;
    std::map<GLenum, GLuint> mBuffers;

    size_t mTextureUnitIndex;
    std::map<GLenum, std::vector<GLuint>> mTextures;

    GLint mUnpackAlignment;
    GLint mUnpackRowLength;
};

}

#endif // LIBANGLE_RENDERER_GL_STATEMANAGERGL_H_

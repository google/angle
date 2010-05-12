//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer.h: Defines the gl::Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#ifndef LIBGLESV2_BUFFER_H_
#define LIBGLESV2_BUFFER_H_

#include <cstddef>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/angleutils.h"

namespace gl
{

class BufferBackEnd;
class TranslatedVertexBuffer;

class Buffer
{
  public:
    explicit Buffer(BufferBackEnd *backEnd);

    GLenum bufferData(const void *data, GLsizeiptr size, GLenum usage);
    GLenum bufferSubData(const void *data, GLsizeiptr size, GLintptr offset);

    void *data() { return &mContents[0]; }
    GLsizeiptr size() const { return mContents.size(); }
    GLenum usage() const { return mUsage; }

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    std::vector<GLubyte> mContents;
    GLenum mUsage;

    BufferBackEnd *mBackEnd;
};

}

#endif   // LIBGLESV2_BUFFER_H_

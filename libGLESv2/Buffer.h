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

#define GL_APICALL
#include <GLES2/gl2.h>
#include <d3d9.h>

#include "angleutils.h"

namespace gl
{
class Buffer
{
  public:
    Buffer();

    ~Buffer();

    void storeData(GLsizeiptr size, const void *data);

    IDirect3DVertexBuffer9 *getVertexBuffer();
    IDirect3DIndexBuffer9 *getIndexBuffer();

  private:
    DISALLOW_COPY_AND_ASSIGN(Buffer);

    void erase();

    unsigned int mSize;
    void *mData;

    IDirect3DVertexBuffer9 *mVertexBuffer;
    IDirect3DIndexBuffer9 *mIndexBuffer;
};
}

#endif   // LIBGLESV2_BUFFER_H_

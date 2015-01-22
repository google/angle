//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferGL.h: Defines the class interface for BufferGL.

#ifndef LIBANGLE_RENDERER_GL_BUFFERGL_H_
#define LIBANGLE_RENDERER_GL_BUFFERGL_H_

#include "libANGLE/renderer/BufferImpl.h"

namespace rx
{

class BufferGL : public BufferImpl
{
  public:
    BufferGL();
    ~BufferGL() override;

    gl::Error setData(const void* data, size_t size, GLenum usage) override;
    gl::Error setSubData(const void* data, size_t size, size_t offset) override;
    gl::Error copySubData(BufferImpl* source, GLintptr sourceOffset, GLintptr destOffset, GLsizeiptr size) override;
    gl::Error map(size_t offset, size_t length, GLbitfield access, GLvoid **mapPtr) override;
    gl::Error unmap() override;
    void markTransformFeedbackUsage() override;

    // This method may not have a corresponding GL-backed function. It is necessary
    // for validation, for certain indexed draw calls.
    gl::Error getData(const uint8_t **outData) override;

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferGL);
};

}

#endif // LIBANGLE_RENDERER_GL_BUFFERGL_H_

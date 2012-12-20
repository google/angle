//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VertexBuffer.h: Defines the abstract VertexBuffer class and VertexBufferInterface
// class with derivations, classes that perform graphics API agnostic vertex buffer operations.

#ifndef LIBGLESV2_RENDERER_VERTEXBUFFER_H_
#define LIBGLESV2_RENDERER_VERTEXBUFFER_H_

#include <vector>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "libGLESv2/Context.h"

namespace rx
{

class VertexBuffer
{
  public:
    VertexBuffer();
    virtual ~VertexBuffer();

    virtual bool initialize(unsigned int size, bool dynamicUsage) = 0;

    virtual bool storeVertexAttributes(const gl::VertexAttribute &attrib, GLint start, GLsizei count,
                                       GLsizei instances, unsigned int offset) = 0;
    virtual bool storeRawData(const void* data, unsigned int size, unsigned int offset) = 0;

    virtual unsigned int getSpaceRequired(const gl::VertexAttribute &attrib, GLsizei count,
                                          GLsizei instances) const = 0;

    virtual unsigned int getBufferSize() const = 0;
    virtual bool setBufferSize(unsigned int size) = 0;
    virtual bool discard() = 0;

    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBuffer);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

class VertexBufferInterface
{
  public:
    VertexBufferInterface(rx::Renderer9 *renderer, std::size_t size, DWORD usageFlags);
    virtual ~VertexBufferInterface();

    void unmap();
    virtual void *map(const gl::VertexAttribute &attribute, std::size_t requiredSpace, std::size_t *streamOffset) = 0;

    std::size_t size() const { return mBufferSize; }
    virtual void reserveRequiredSpace() = 0;
    void addRequiredSpace(UINT requiredSpace);

    IDirect3DVertexBuffer9 *getBuffer() const;
    unsigned int getSerial() const;

  protected:
    rx::Renderer9 *const mRenderer;   // D3D9_REPLACE
    IDirect3DVertexBuffer9 *mVertexBuffer;

    unsigned int mSerial;
    static unsigned int issueSerial();
    static unsigned int mCurrentSerial;

    std::size_t mBufferSize;
    std::size_t mWritePosition;
    std::size_t mRequiredSpace;

  private:
    DISALLOW_COPY_AND_ASSIGN(VertexBufferInterface);
};

class StreamingVertexBufferInterface : public VertexBufferInterface
{
  public:
    StreamingVertexBufferInterface(rx::Renderer9 *renderer, std::size_t initialSize);
    ~StreamingVertexBufferInterface();

    void *map(const gl::VertexAttribute &attribute, std::size_t requiredSpace, std::size_t *streamOffset);
    void reserveRequiredSpace();
};

class StaticVertexBufferInterface : public VertexBufferInterface
{
  public:
    explicit StaticVertexBufferInterface(rx::Renderer9 *renderer);
    ~StaticVertexBufferInterface();

    void *map(const gl::VertexAttribute &attribute, std::size_t requiredSpace, std::size_t *streamOffset);
    void reserveRequiredSpace();

    std::size_t lookupAttribute(const gl::VertexAttribute &attribute);   // Returns the offset into the vertex buffer, or -1 if not found

  private:
    struct VertexElement
    {
        GLenum type;
        GLint size;
        GLsizei stride;
        bool normalized;
        int attributeOffset;

        std::size_t streamOffset;
    };

    std::vector<VertexElement> mCache;
};

}

#endif // LIBGLESV2_RENDERER_VERTEXBUFFER_H_
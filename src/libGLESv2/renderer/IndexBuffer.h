//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// IndexBuffer.h: Defines the abstract IndexBuffer class and IndexBufferInterface
// class with derivations, classes that perform graphics API agnostic index buffer operations.

#ifndef LIBGLESV2_RENDERER_INDEXBUFFER_H_
#define LIBGLESV2_RENDERER_INDEXBUFFER_H_

#include <vector>
#include <cstddef>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "libGLESv2/Context.h"
#include "libGLESv2/renderer/Renderer.h"

namespace rx
{

class Renderer9;

class IndexBuffer
{
  public:
    IndexBuffer();
    virtual ~IndexBuffer();

    virtual bool initialize(unsigned int bufferSize, GLenum indexType, bool dynamic) = 0;

    virtual bool mapBuffer(unsigned int offset, unsigned int size, void** outMappedMemory) = 0;
    virtual bool unmapBuffer() = 0;

    virtual bool discard() = 0;

    virtual GLenum getIndexType() const = 0;
    virtual unsigned int getBufferSize() const = 0;
    virtual bool setSize(unsigned int bufferSize, GLenum indexType) = 0;

    unsigned int getSerial() const;

  protected:
    void updateSerial();

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBuffer);

    unsigned int mSerial;
    static unsigned int mNextSerial;
};

class IndexBufferInterface
{
  public:
    IndexBufferInterface(rx::Renderer9 *renderer, UINT size, D3DFORMAT format);
    virtual ~IndexBufferInterface();

    UINT size() const { return mBufferSize; }
    virtual void *map(UINT requiredSpace, UINT *offset) = 0;
    void unmap();
    virtual void reserveSpace(UINT requiredSpace, GLenum type) = 0;

    IDirect3DIndexBuffer9 *getBuffer() const;
    unsigned int getSerial() const;

  protected:
    rx::Renderer9 *const mRenderer;   // D3D9_REPLACE

    IDirect3DIndexBuffer9 *mIndexBuffer;
    UINT mBufferSize;

    unsigned int mSerial;
    static unsigned int issueSerial();
    static unsigned int mCurrentSerial;

  private:
    DISALLOW_COPY_AND_ASSIGN(IndexBufferInterface);
};

class StreamingIndexBufferInterface : public IndexBufferInterface
{
  public:
    StreamingIndexBufferInterface(rx::Renderer9 *renderer, UINT initialSize, D3DFORMAT format);
    ~StreamingIndexBufferInterface();

    virtual void *map(UINT requiredSpace, UINT *offset);
    virtual void reserveSpace(UINT requiredSpace, GLenum type);

  private:
    UINT mWritePosition;
};

class StaticIndexBufferInterface : public IndexBufferInterface
{
  public:
    explicit StaticIndexBufferInterface(rx::Renderer9 *renderer);
    ~StaticIndexBufferInterface();

    virtual void *map(UINT requiredSpace, UINT *offset);
    virtual void reserveSpace(UINT requiredSpace, GLenum type);

    bool lookupType(GLenum type);
    UINT lookupRange(intptr_t offset, GLsizei count, UINT *minIndex, UINT *maxIndex);   // Returns the offset into the index buffer, or -1 if not found
    void addRange(intptr_t offset, GLsizei count, UINT minIndex, UINT maxIndex, UINT streamOffset);

  private:
    GLenum mCacheType;

    struct IndexRange
    {
        intptr_t offset;
        GLsizei count;

        bool operator<(const IndexRange& rhs) const
        {
            if (offset != rhs.offset)
            {
                return offset < rhs.offset;
            }
            if (count != rhs.count)
            {
                return count < rhs.count;
            }
            return false;
        }
    };

    struct IndexResult
    {
        UINT minIndex;
        UINT maxIndex;
        UINT streamOffset;
    };

    std::map<IndexRange, IndexResult> mCache;
};

}

#endif // LIBGLESV2_RENDERER_INDEXBUFFER_H_
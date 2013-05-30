//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage11.h Defines the BufferStorage11 class.

#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{
class Renderer11;
class DirectBufferStorage11;

class BufferStorage11 : public BufferStorage
{
  public:
    explicit BufferStorage11(Renderer11 *renderer);
    virtual ~BufferStorage11();

    static BufferStorage11 *makeBufferStorage11(BufferStorage *bufferStorage);

    virtual void *getData();
    virtual void setData(const void* data, unsigned int size, unsigned int offset);
    virtual void copyData(BufferStorage* sourceStorage, unsigned int size,
                          unsigned int sourceOffset, unsigned int destOffset);
    virtual void clear();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;
    virtual void markBufferUsage();

    ID3D11Buffer *getBuffer(GLenum usage);

  private:
    Renderer11 *mRenderer;

    ID3D11Buffer *mStagingBuffer;
    unsigned int mStagingBufferSize;

    typedef std::vector<DirectBufferStorage11*> DirectBufferList;
    DirectBufferList mDirectBuffers;
    unsigned int mSize;

    void *mResolvedData;
    unsigned int mResolvedDataSize;
    bool mResolvedDataValid;

    unsigned int mReadUsageCount;
    unsigned int mWriteUsageCount;
};

// Each instance of BufferStorageD3DBuffer11 is specialized for a class of D3D binding points
// - vertex/index buffers
// - uniform buffers
// - (possibly in the future, transform feedback buffers)
class DirectBufferStorage11
{
  public:
    DirectBufferStorage11(Renderer11 *renderer, const GLenum target);
    ~DirectBufferStorage11();

    bool hasTarget(const GLenum target) const;
    bool updateFromStagingBuffer(ID3D11Buffer *stagingBuffer, const size_t size, const size_t offset);

    ID3D11Buffer *getD3DBuffer() { return mDirectBuffer; }
    bool isDirty() const { return mDirty; }
    void markDirty() { mDirty = true; }

  private:
    Renderer11 *mRenderer;
    const GLenum mTarget;
    ID3D11Buffer *mDirectBuffer;
    size_t mBufferSize;
    bool mDirty;

    void fillBufferDesc(D3D11_BUFFER_DESC* bufferDesc, unsigned int bufferSize);
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_

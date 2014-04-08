//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// BufferStorage11.h Defines the BufferStorage11 class.

#ifndef LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_
#define LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_

#include "libGLESv2/renderer/BufferStorage.h"

namespace rx
{
class Renderer;
class Renderer11;

enum BufferUsage
{
    BUFFER_USAGE_STAGING,
    BUFFER_USAGE_VERTEX_OR_TRANSFORM_FEEDBACK,
    BUFFER_USAGE_INDEX,
    BUFFER_USAGE_PIXEL_UNPACK,
    BUFFER_USAGE_UNIFORM,
};

typedef size_t DataRevision;

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
    virtual void markTransformFeedbackUsage();
    virtual unsigned int getSize() const;
    virtual bool supportsDirectBinding() const;

    ID3D11Buffer *getBuffer(BufferUsage usage);
    ID3D11ShaderResourceView *getSRV(DXGI_FORMAT srvFormat);

    virtual bool isMapped() const;
    virtual void *map(GLbitfield access);
    virtual void unmap();

  private:
    class TypedBufferStorage11;
    class NativeBuffer11;

    Renderer11 *mRenderer;
    bool mIsMapped;

    std::map<BufferUsage, TypedBufferStorage11*> mTypedBuffers;

    typedef std::pair<ID3D11Buffer *, ID3D11ShaderResourceView *> BufferSRVPair;
    std::map<DXGI_FORMAT, BufferSRVPair> mBufferResourceViews;

    std::vector<unsigned char> mResolvedData;
    DataRevision mResolvedDataRevision;

    unsigned int mReadUsageCount;
    unsigned int mWriteUsageCount;

    size_t mSize;

    void markBufferUsage();
    NativeBuffer11 *getStagingBuffer();

    TypedBufferStorage11 *getStorage(BufferUsage usage);
    TypedBufferStorage11 *getLatestStorage() const;
};

}

#endif // LIBGLESV2_RENDERER_BUFFERSTORAGE11_H_

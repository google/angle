//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemory.h: Defines the abstract cl::Memory class, which is a memory object
// and the base class for OpenCL objects such as Buffer, Image and Pipe.

#ifndef LIBANGLE_CLMEMORY_H_
#define LIBANGLE_CLMEMORY_H_

#include "libANGLE/CLObject.h"
#include "libANGLE/renderer/CLMemoryImpl.h"

namespace cl
{

class Memory : public _cl_mem, public Object
{
  public:
    using PropArray = std::vector<cl_mem_properties>;

    ~Memory() override;

    virtual cl_mem_object_type getType() const = 0;

    const Context &getContext() const;
    const PropArray &getProperties() const;
    MemFlags getFlags() const;
    void *getHostPtr() const;
    const MemoryPtr &getParent() const;
    size_t getOffset() const;

    cl_int getInfo(MemInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const;

  protected:
    Memory(const Buffer &buffer,
           Context &context,
           PropArray &&properties,
           MemFlags flags,
           size_t size,
           void *hostPtr,
           cl_int &errorCode);

    Memory(const Buffer &buffer,
           Buffer &parent,
           MemFlags flags,
           size_t offset,
           size_t size,
           cl_int &errorCode);

    Memory(const Image &image,
           Context &context,
           PropArray &&properties,
           MemFlags flags,
           const cl_image_format &format,
           const ImageDescriptor &desc,
           Memory *parent,
           void *hostPtr,
           cl_int &errorCode);

    const ContextPtr mContext;
    const PropArray mProperties;
    const MemFlags mFlags;
    void *const mHostPtr = nullptr;
    const MemoryPtr mParent;
    const size_t mOffset = 0u;
    const rx::CLMemoryImpl::Ptr mImpl;
    const size_t mSize;

    cl_uint mMapCount = 0u;

    friend class Buffer;
    friend class Context;
};

inline const Context &Memory::getContext() const
{
    return *mContext;
}

inline const Memory::PropArray &Memory::getProperties() const
{
    return mProperties;
}

inline MemFlags Memory::getFlags() const
{
    return mFlags;
}

inline void *Memory::getHostPtr() const
{
    return mHostPtr;
}

inline const MemoryPtr &Memory::getParent() const
{
    return mParent;
}

inline size_t Memory::getOffset() const
{
    return mOffset;
}

}  // namespace cl

#endif  // LIBANGLE_CLMEMORY_H_

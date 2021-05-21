//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLBuffer.h: Defines the cl::Buffer class, which is a collection of elements.

#ifndef LIBANGLE_CLBUFFER_H_
#define LIBANGLE_CLBUFFER_H_

#include "libANGLE/CLMemory.h"

namespace cl
{

class Buffer final : public Memory
{
  public:
    ~Buffer() override;

    cl_mem_object_type getType() const final;

    bool isSubBuffer() const;
    bool isRegionValid(const cl_buffer_region &region) const;

    cl_mem createSubBuffer(cl_mem_flags flags,
                           cl_buffer_create_type createType,
                           const void *createInfo,
                           cl_int *errcodeRet);

    static bool IsValid(const _cl_mem *buffer);

  private:
    Buffer(Context &context,
           PropArray &&properties,
           cl_mem_flags flags,
           size_t size,
           void *hostPtr,
           cl_int *errcodeRet);

    Buffer(Buffer &parent, cl_mem_flags flags, size_t offset, size_t size, cl_int *errcodeRet);

    friend class Context;
};

inline cl_mem_object_type Buffer::getType() const
{
    return CL_MEM_OBJECT_BUFFER;
}

inline bool Buffer::isSubBuffer() const
{
    return bool(mParent);
}

inline bool Buffer::isRegionValid(const cl_buffer_region &region) const
{
    return region.origin < mSize && region.origin + region.size <= mSize;
}

inline bool Buffer::IsValid(const _cl_mem *buffer)
{
    return Memory::IsValid(buffer) &&
           static_cast<const Memory *>(buffer)->getType() == CL_MEM_OBJECT_BUFFER;
}

}  // namespace cl

#endif  // LIBANGLE_CLBUFFER_H_

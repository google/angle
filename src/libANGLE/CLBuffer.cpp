//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLBuffer.cpp: Implements the cl::Buffer class.

#include "libANGLE/CLBuffer.h"

#include "libANGLE/CLContext.h"

namespace cl
{

Buffer::~Buffer() = default;

cl_mem Buffer::createSubBuffer(cl_mem_flags flags,
                               cl_buffer_create_type createType,
                               const void *createInfo,
                               cl_int *errcodeRet)
{
    const cl_buffer_region &region = *static_cast<const cl_buffer_region *>(createInfo);
    return mContext->createMemory(new Buffer(*this, flags, region.origin, region.size, errcodeRet),
                                  errcodeRet);
}

Buffer::Buffer(Context &context,
               PropArray &&properties,
               cl_mem_flags flags,
               size_t size,
               void *hostPtr,
               cl_int *errcodeRet)
    : Memory(*this, context, std::move(properties), flags, size, hostPtr, errcodeRet)
{}

Buffer::Buffer(Buffer &parent, cl_mem_flags flags, size_t offset, size_t size, cl_int *errcodeRet)
    : Memory(*this, parent, flags, offset, size, errcodeRet)
{}

}  // namespace cl

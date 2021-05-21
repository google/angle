//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemoryCL.cpp: Implements the class methods for CLMemoryCL.

#include "libANGLE/renderer/cl/CLMemoryCL.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/Debug.h"

namespace rx
{

CLMemoryCL::CLMemoryCL(const cl::Memory &memory, cl_mem native)
    : CLMemoryImpl(memory), mNative(native)
{}

CLMemoryCL::~CLMemoryCL()
{
    if (mNative->getDispatch().clReleaseMemObject(mNative) != CL_SUCCESS)
    {
        ERR() << "Error while releasing CL memory object";
    }
}

size_t CLMemoryCL::getSize() const
{
    size_t size = 0u;
    if (mNative->getDispatch().clGetMemObjectInfo(mNative, CL_MEM_SIZE, sizeof(size), &size,
                                                  nullptr) != CL_SUCCESS)
    {
        ERR() << "Failed to query CL memory object size";
        return 0u;
    }
    return size;
}

CLMemoryImpl::Ptr CLMemoryCL::createSubBuffer(const cl::Buffer &buffer,
                                              size_t size,
                                              cl_int *errcodeRet)
{
    const cl_buffer_region region = {buffer.getOffset(), size};
    const cl_mem nativeBuffer     = mNative->getDispatch().clCreateSubBuffer(
        mNative, buffer.getFlags(), CL_BUFFER_CREATE_TYPE_REGION, &region, errcodeRet);
    return CLMemoryImpl::Ptr(nativeBuffer != nullptr ? new CLMemoryCL(buffer, nativeBuffer)
                                                     : nullptr);
}

}  // namespace rx

//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemory.cpp: Implements the cl::Memory class.

#include "libANGLE/CLMemory.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLContext.h"

#include <cstring>

namespace cl
{

Memory::~Memory() = default;

cl_int Memory::getInfo(MemInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    static_assert(
        std::is_same<cl_uint, cl_bool>::value && std::is_same<cl_uint, cl_mem_object_type>::value,
        "OpenCL type mismatch");

    cl_uint valUInt       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case MemInfo::Type:
            valUInt   = getType();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case MemInfo::Flags:
            copyValue = &mFlags;
            copySize  = sizeof(mFlags);
            break;
        case MemInfo::Size:
            copyValue = &mSize;
            copySize  = sizeof(mSize);
            break;
        case MemInfo::HostPtr:
            copyValue = &mHostPtr;
            copySize  = sizeof(mHostPtr);
            break;
        case MemInfo::MapCount:
            copyValue = &mMapCount;
            copySize  = sizeof(mMapCount);
            break;
        case MemInfo::ReferenceCount:
            valUInt   = getRefCount();
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case MemInfo::Context:
            valPointer = mContext->getNative();
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case MemInfo::AssociatedMemObject:
            valPointer = Memory::CastNative(mParent.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case MemInfo::Offset:
            copyValue = &mOffset;
            copySize  = sizeof(mOffset);
            break;
        case MemInfo::UsesSVM_Pointer:
            valUInt   = CL_FALSE;  // TODO(jplate) Check for SVM pointer anglebug.com/6002
            copyValue = &valUInt;
            copySize  = sizeof(valUInt);
            break;
        case MemInfo::Properties:
            copyValue = mProperties.data();
            copySize  = mProperties.size() * sizeof(decltype(mProperties)::value_type);
            break;
        default:
            return CL_INVALID_VALUE;
    }

    if (value != nullptr)
    {
        // CL_INVALID_VALUE if size in bytes specified by param_value_size is < size of return type
        // as described in the Memory Object Info table and param_value is not NULL.
        if (valueSize < copySize)
        {
            return CL_INVALID_VALUE;
        }
        if (copyValue != nullptr)
        {
            std::memcpy(value, copyValue, copySize);
        }
    }
    if (valueSizeRet != nullptr)
    {
        *valueSizeRet = copySize;
    }
    return CL_SUCCESS;
}

Memory::Memory(const Buffer &buffer,
               Context &context,
               PropArray &&properties,
               MemFlags flags,
               size_t size,
               void *hostPtr,
               cl_int &errorCode)
    : mContext(&context),
      mProperties(std::move(properties)),
      mFlags(flags),
      mHostPtr(flags.isSet(CL_MEM_USE_HOST_PTR) ? hostPtr : nullptr),
      mImpl(context.getImpl().createBuffer(buffer, size, hostPtr, errorCode)),
      mSize(size)
{}

Memory::Memory(const Buffer &buffer,
               Buffer &parent,
               MemFlags flags,
               size_t offset,
               size_t size,
               cl_int &errorCode)
    : mContext(parent.mContext),
      mFlags(flags),
      mHostPtr(parent.mHostPtr != nullptr ? static_cast<char *>(parent.mHostPtr) + offset
                                          : nullptr),
      mParent(&parent),
      mOffset(offset),
      mImpl(parent.mImpl->createSubBuffer(buffer, size, errorCode)),
      mSize(size)
{}

Memory::Memory(const Image &image,
               Context &context,
               PropArray &&properties,
               MemFlags flags,
               const cl_image_format &format,
               const ImageDescriptor &desc,
               Memory *parent,
               void *hostPtr,
               cl_int &errorCode)
    : mContext(&context),
      mProperties(std::move(properties)),
      mFlags(flags),
      mHostPtr(flags.isSet(CL_MEM_USE_HOST_PTR) ? hostPtr : nullptr),
      mParent(parent),
      mImpl(context.getImpl().createImage(image, format, desc, hostPtr, errorCode)),
      mSize(mImpl ? mImpl->getSize(errorCode) : 0u)
{}

}  // namespace cl

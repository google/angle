//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemory.cpp: Implements the cl::Memory class.

#include "libANGLE/CLMemory.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLContext.h"
#include "libANGLE/CLPlatform.h"

#include <cstring>

namespace cl
{

Memory::~Memory() = default;

bool Memory::release()
{
    const bool released = removeRef();
    if (released)
    {
        mContext->destroyMemory(this);
    }
    return released;
}

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
            copyValue = getRefCountPtr();
            copySize  = sizeof(*getRefCountPtr());
            break;
        case MemInfo::Context:
            valPointer = static_cast<cl_context>(mContext.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case MemInfo::AssociatedMemObject:
            valPointer = static_cast<cl_mem>(mParent.get());
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

bool Memory::IsValid(const _cl_mem *memory)
{
    const Platform::PtrList &platforms = Platform::GetPlatforms();
    return std::find_if(platforms.cbegin(), platforms.cend(), [=](const PlatformPtr &platform) {
               return platform->hasMemory(memory);
           }) != platforms.cend();
}

Memory::Memory(const Buffer &buffer,
               Context &context,
               PropArray &&properties,
               cl_mem_flags flags,
               size_t size,
               void *hostPtr,
               cl_int *errcodeRet)
    : _cl_mem(context.getDispatch()),
      mContext(&context),
      mProperties(std::move(properties)),
      mFlags(flags),
      mHostPtr((flags & CL_MEM_USE_HOST_PTR) != 0u ? hostPtr : nullptr),
      mImpl(context.mImpl->createBuffer(buffer, size, hostPtr, errcodeRet)),
      mSize(size)
{}

Memory::Memory(const Buffer &buffer,
               Buffer &parent,
               cl_mem_flags flags,
               size_t offset,
               size_t size,
               cl_int *errcodeRet)
    : _cl_mem(parent.getDispatch()),
      mContext(parent.mContext),
      mFlags(flags),
      mHostPtr(parent.mHostPtr != nullptr ? static_cast<char *>(parent.mHostPtr) + offset
                                          : nullptr),
      mParent(&parent),
      mOffset(offset),
      mImpl(parent.mImpl->createSubBuffer(buffer, size, errcodeRet)),
      mSize(size)
{}

Memory::Memory(const Image &image,
               Context &context,
               PropArray &&properties,
               cl_mem_flags flags,
               const cl_image_format &format,
               const ImageDescriptor &desc,
               Memory *parent,
               void *hostPtr,
               cl_int *errcodeRet)
    : _cl_mem(context.getDispatch()),
      mContext(&context),
      mProperties(std::move(properties)),
      mFlags(flags),
      mHostPtr((flags & CL_MEM_USE_HOST_PTR) != 0u ? hostPtr : nullptr),
      mParent(parent),
      mImpl(context.mImpl->createImage(image, format, desc, hostPtr, errcodeRet)),
      mSize(mImpl ? mImpl->getSize() : 0u)
{}

}  // namespace cl

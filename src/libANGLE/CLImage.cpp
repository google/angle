//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLImage.cpp: Implements the cl::Image class.

#include "libANGLE/CLImage.h"

#include "libANGLE/cl_utils.h"

#include <cstring>

namespace cl
{

Image::~Image() = default;

cl_int Image::getInfo(ImageInfo name, size_t valueSize, void *value, size_t *valueSizeRet) const
{
    size_t valSizeT       = 0u;
    void *valPointer      = nullptr;
    const void *copyValue = nullptr;
    size_t copySize       = 0u;

    switch (name)
    {
        case ImageInfo::Format:
            copyValue = &mFormat;
            copySize  = sizeof(mFormat);
            break;
        case ImageInfo::ElementSize:
            valSizeT  = GetElementSize(mFormat);
            copyValue = &valSizeT;
            copySize  = sizeof(valSizeT);
            break;
        case ImageInfo::RowPitch:
            copyValue = &mDesc.rowPitch;
            copySize  = sizeof(mDesc.rowPitch);
            break;
        case ImageInfo::SlicePitch:
            copyValue = &mDesc.slicePitch;
            copySize  = sizeof(mDesc.slicePitch);
            break;
        case ImageInfo::Width:
            copyValue = &mDesc.width;
            copySize  = sizeof(mDesc.width);
            break;
        case ImageInfo::Height:
            copyValue = &mDesc.height;
            copySize  = sizeof(mDesc.height);
            break;
        case ImageInfo::Depth:
            copyValue = &mDesc.depth;
            copySize  = sizeof(mDesc.depth);
            break;
        case ImageInfo::ArraySize:
            copyValue = &mDesc.arraySize;
            copySize  = sizeof(mDesc.arraySize);
            break;
        case ImageInfo::Buffer:
            valPointer = static_cast<cl_mem>(mParent.get());
            copyValue  = &valPointer;
            copySize   = sizeof(valPointer);
            break;
        case ImageInfo::NumMipLevels:
            copyValue = &mDesc.numMipLevels;
            copySize  = sizeof(mDesc.numMipLevels);
            break;
        case ImageInfo::NumSamples:
            copyValue = &mDesc.numSamples;
            copySize  = sizeof(mDesc.numSamples);
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

bool Image::IsValid(const _cl_mem *image)
{
    if (!Memory::IsValid(image))
    {
        return false;
    }
    switch (static_cast<const Memory *>(image)->getType())
    {
        case CL_MEM_OBJECT_IMAGE1D:
        case CL_MEM_OBJECT_IMAGE2D:
        case CL_MEM_OBJECT_IMAGE3D:
        case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        case CL_MEM_OBJECT_IMAGE1D_BUFFER:
            break;
        default:
            return false;
    }
    return true;
}

Image::Image(Context &context,
             PropArray &&properties,
             cl_mem_flags flags,
             const cl_image_format &format,
             const ImageDescriptor &desc,
             Memory *parent,
             void *hostPtr,
             cl_int *errcodeRet)
    : Memory(*this,
             context,
             std::move(properties),
             flags,
             format,
             desc,
             parent,
             hostPtr,
             errcodeRet),
      mFormat(format),
      mDesc(desc)
{}

}  // namespace cl

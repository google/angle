//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLMemoryVk.cpp: Implements the class methods for CLMemoryVk.

#include <cstdint>

#include "libANGLE/renderer/vulkan/CLContextVk.h"
#include "libANGLE/renderer/vulkan/CLMemoryVk.h"
#include "libANGLE/renderer/vulkan/vk_cl_utils.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"

#include "libANGLE/renderer/CLMemoryImpl.h"
#include "libANGLE/renderer/Format.h"
#include "libANGLE/renderer/FormatID_autogen.h"

#include "libANGLE/CLBuffer.h"
#include "libANGLE/CLContext.h"
#include "libANGLE/CLImage.h"
#include "libANGLE/CLMemory.h"
#include "libANGLE/Error.h"
#include "libANGLE/cl_utils.h"

namespace rx
{

CLMemoryVk::CLMemoryVk(const cl::Memory &memory)
    : CLMemoryImpl(memory),
      mContext(&memory.getContext().getImpl<CLContextVk>()),
      mRenderer(mContext->getRenderer()),
      mMappedMemory(nullptr),
      mMapCount(0),
      mParent(nullptr)
{}

CLMemoryVk::~CLMemoryVk()
{
    mContext->mAssociatedObjects->mMemories.erase(mMemory.getNative());
}

VkBufferUsageFlags CLMemoryVk::getVkUsageFlags()
{
    return cl_vk::GetBufferUsageFlags(mMemory.getFlags());
}

VkMemoryPropertyFlags CLMemoryVk::getVkMemPropertyFlags()
{
    return cl_vk::GetMemoryPropertyFlags(mMemory.getFlags());
}

angle::Result CLMemoryVk::map(uint8_t *&ptrOut, size_t offset)
{
    if (!isMapped())
    {
        ANGLE_TRY(mapImpl());
    }
    ptrOut = mMappedMemory + offset;
    return angle::Result::Continue;
}

angle::Result CLMemoryVk::copyTo(void *dst, size_t srcOffset, size_t size)
{
    uint8_t *src = nullptr;
    ANGLE_TRY(map(src, srcOffset));
    std::memcpy(dst, src, size);
    unmap();
    return angle::Result::Continue;
}

angle::Result CLMemoryVk::copyTo(CLMemoryVk *dst, size_t srcOffset, size_t dstOffset, size_t size)
{
    uint8_t *dstPtr = nullptr;
    ANGLE_TRY(dst->map(dstPtr, dstOffset));
    ANGLE_TRY(copyTo(dstPtr, srcOffset, size));
    dst->unmap();
    return angle::Result::Continue;
}

angle::Result CLMemoryVk::copyFrom(const void *src, size_t srcOffset, size_t size)
{
    uint8_t *dst = nullptr;
    ANGLE_TRY(map(dst, srcOffset));
    std::memcpy(dst, src, size);
    unmap();
    return angle::Result::Continue;
}

// Create a sub-buffer from the given buffer object
angle::Result CLMemoryVk::createSubBuffer(const cl::Buffer &buffer,
                                          cl::MemFlags flags,
                                          size_t size,
                                          CLMemoryImpl::Ptr *subBufferOut)
{
    ASSERT(buffer.isSubBuffer());

    CLBufferVk *bufferVk = new CLBufferVk(buffer);
    if (!bufferVk)
    {
        ANGLE_CL_RETURN_ERROR(CL_OUT_OF_HOST_MEMORY);
    }
    ANGLE_TRY(bufferVk->create(nullptr));
    *subBufferOut = CLMemoryImpl::Ptr(bufferVk);

    return angle::Result::Continue;
}

CLBufferVk::CLBufferVk(const cl::Buffer &buffer) : CLMemoryVk(buffer)
{
    if (buffer.isSubBuffer())
    {
        mParent = &buffer.getParent()->getImpl<CLBufferVk>();
    }
    mDefaultBufferCreateInfo             = {};
    mDefaultBufferCreateInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    mDefaultBufferCreateInfo.size        = buffer.getSize();
    mDefaultBufferCreateInfo.usage       = getVkUsageFlags();
    mDefaultBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
}

CLBufferVk::~CLBufferVk()
{
    if (isMapped())
    {
        unmap();
    }
    mBuffer.destroy(mRenderer);
}

vk::BufferHelper &CLBufferVk::getBuffer()
{
    if (isSubBuffer())
    {
        return getParent()->getBuffer();
    }
    return mBuffer;
}

angle::Result CLBufferVk::create(void *hostPtr)
{
    if (!isSubBuffer())
    {
        VkBufferCreateInfo createInfo  = mDefaultBufferCreateInfo;
        createInfo.size                = getSize();
        VkMemoryPropertyFlags memFlags = getVkMemPropertyFlags();
        if (IsError(mBuffer.init(mContext, createInfo, memFlags)))
        {
            ANGLE_CL_RETURN_ERROR(CL_OUT_OF_RESOURCES);
        }
        if (mMemory.getFlags().intersects(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
        {
            ASSERT(hostPtr);
            ANGLE_CL_IMPL_TRY_ERROR(setDataImpl(static_cast<uint8_t *>(hostPtr), getSize(), 0),
                                    CL_OUT_OF_RESOURCES);
        }
    }
    return angle::Result::Continue;
}

angle::Result CLBufferVk::mapImpl()
{
    ASSERT(!isMapped());

    if (isSubBuffer())
    {
        ANGLE_TRY(mParent->map(mMappedMemory, getOffset()));
        return angle::Result::Continue;
    }
    ANGLE_TRY(mBuffer.map(mContext, &mMappedMemory));
    return angle::Result::Continue;
}

void CLBufferVk::unmapImpl()
{
    if (!isSubBuffer())
    {
        mBuffer.unmap(mRenderer);
    }
    mMappedMemory = nullptr;
}

angle::Result CLBufferVk::setDataImpl(const uint8_t *data, size_t size, size_t offset)
{
    // buffer cannot be in use state
    ASSERT(mBuffer.valid());
    ASSERT(!isCurrentlyInUse());
    ASSERT(size + offset <= getSize());
    ASSERT(data != nullptr);

    // Assuming host visible buffers for now
    // TODO: http://anglebug.com/42267019
    if (!mBuffer.isHostVisible())
    {
        UNIMPLEMENTED();
        ANGLE_CL_RETURN_ERROR(CL_OUT_OF_RESOURCES);
    }

    uint8_t *mapPointer = nullptr;
    ANGLE_TRY(mBuffer.mapWithOffset(mContext, &mapPointer, offset));
    ASSERT(mapPointer != nullptr);

    std::memcpy(mapPointer, data, size);
    mBuffer.unmap(mRenderer);

    return angle::Result::Continue;
}

bool CLBufferVk::isCurrentlyInUse() const
{
    return !mRenderer->hasResourceUseFinished(mBuffer.getResourceUse());
}

VkImageUsageFlags CLImageVk::getVkImageUsageFlags()
{
    VkImageUsageFlags usageFlags =
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    if (mMemory.getFlags().intersects(CL_MEM_WRITE_ONLY))
    {
        usageFlags |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    else if (mMemory.getFlags().intersects(CL_MEM_READ_ONLY))
    {
        usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    else
    {
        usageFlags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
    }

    return usageFlags;
}

VkImageType CLImageVk::getVkImageType(const cl::ImageDescriptor &desc)
{
    VkImageType imageType = VK_IMAGE_TYPE_MAX_ENUM;

    switch (desc.type)
    {
        case cl::MemObjectType::Image1D_Buffer:
        case cl::MemObjectType::Image1D:
        case cl::MemObjectType::Image1D_Array:
            return VK_IMAGE_TYPE_1D;
        case cl::MemObjectType::Image2D:
        case cl::MemObjectType::Image2D_Array:
            return VK_IMAGE_TYPE_2D;
        case cl::MemObjectType::Image3D:
            return VK_IMAGE_TYPE_3D;
        default:
            UNREACHABLE();
    }

    return imageType;
}

angle::Result CLImageVk::createStagingBuffer(size_t size)
{
    const VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
                                           nullptr,
                                           0,
                                           size,
                                           getVkUsageFlags(),
                                           VK_SHARING_MODE_EXCLUSIVE,
                                           0,
                                           nullptr};
    if (IsError(mStagingBuffer.init(mContext, createInfo, getVkMemPropertyFlags())))
    {
        ANGLE_CL_RETURN_ERROR(CL_OUT_OF_RESOURCES);
    }

    mStagingBufferInitialized = true;
    return angle::Result::Continue;
}

angle::Result CLImageVk::copyStagingFrom(void *ptr, size_t offset, size_t size)
{
    uint8_t *ptrOut;
    uint8_t *ptrIn = static_cast<uint8_t *>(ptr);
    ANGLE_TRY(getStagingBuffer().map(mContext, &ptrOut));
    std::memcpy(ptrOut, ptrIn + offset, size);
    ANGLE_TRY(getStagingBuffer().flush(mRenderer));
    getStagingBuffer().unmap(mContext->getRenderer());
    return angle::Result::Continue;
}

angle::Result CLImageVk::copyStagingTo(void *ptr, size_t offset, size_t size)
{
    uint8_t *ptrOut;
    ANGLE_TRY(getStagingBuffer().map(mContext, &ptrOut));
    ANGLE_TRY(getStagingBuffer().invalidate(mRenderer));
    std::memcpy(ptr, ptrOut + offset, size);
    getStagingBuffer().unmap(mContext->getRenderer());
    return angle::Result::Continue;
}

CLImageVk::CLImageVk(const cl::Image &image)
    : CLMemoryVk(image),
      mFormat(angle::FormatID::NONE),
      mArrayLayers(1),
      mImageSize(0),
      mElementSize(0),
      mDesc(image.getDescriptor()),
      mStagingBufferInitialized(false)
{}

CLImageVk::~CLImageVk()
{
    if (isMapped())
    {
        unmap();
    }

    mImage.destroy(mRenderer);
    mImageView.destroy(mContext->getDevice());
}

angle::Result CLImageVk::create(void *hostPtr)
{
    const cl::Image &image         = reinterpret_cast<const cl::Image &>(mMemory);
    const cl::ImageDescriptor desc = image.getDescriptor();
    const cl_image_format format   = image.getFormat();
    switch (format.image_channel_order)
    {
        case CL_R:
        case CL_LUMINANCE:
        case CL_INTENSITY:
            mFormat = angle::Format::CLRFormatToID(format.image_channel_data_type);
            break;
        case CL_RG:
            mFormat = angle::Format::CLRGFormatToID(format.image_channel_data_type);
            break;
        case CL_RGB:
            mFormat = angle::Format::CLRGBFormatToID(format.image_channel_data_type);
            break;
        case CL_RGBA:
            mFormat = angle::Format::CLRGBAFormatToID(format.image_channel_data_type);
            break;
        case CL_BGRA:
            mFormat = angle::Format::CLBGRAFormatToID(format.image_channel_data_type);
            break;
        case CL_sRGBA:
            mFormat = angle::Format::CLsRGBAFormatToID(format.image_channel_data_type);
            break;
        case CL_DEPTH:
            mFormat = angle::Format::CLDEPTHFormatToID(format.image_channel_data_type);
            break;
        case CL_DEPTH_STENCIL:
            mFormat = angle::Format::CLDEPTHSTENCILFormatToID(format.image_channel_data_type);
            break;
        default:
            UNIMPLEMENTED();
            break;
    }

    mExtent.width  = static_cast<uint32_t>(desc.width);
    mExtent.height = static_cast<uint32_t>(desc.height);
    mExtent.depth  = static_cast<uint32_t>(desc.depth);
    switch (desc.type)
    {
        case cl::MemObjectType::Image1D_Buffer:
        case cl::MemObjectType::Image1D:
        case cl::MemObjectType::Image1D_Array:
            mExtent.height = 1;
            mExtent.depth  = 1;
            break;
        case cl::MemObjectType::Image2D:
        case cl::MemObjectType::Image2D_Array:
            mExtent.depth = 1;
            break;
        case cl::MemObjectType::Image3D:
            break;
        default:
            UNREACHABLE();
    }

    switch (desc.type)
    {
        case cl::MemObjectType::Image1D_Buffer:
        case cl::MemObjectType::Image1D:
            mImageViewType = VK_IMAGE_VIEW_TYPE_1D;
            break;
        case cl::MemObjectType::Image1D_Array:
            mImageViewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
            break;
        case cl::MemObjectType::Image2D:
            mImageViewType = VK_IMAGE_VIEW_TYPE_2D;
            break;
        case cl::MemObjectType::Image2D_Array:
            mImageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
            break;
        case cl::MemObjectType::Image3D:
            mImageViewType = VK_IMAGE_VIEW_TYPE_3D;
            break;
        default:
            UNREACHABLE();
    }

    mElementSize = cl::GetElementSize(format);
    mDesc        = desc;
    mImageFormat = format;

    if (desc.slicePitch > 0)
    {
        mImageSize = (desc.slicePitch * mExtent.depth);
    }
    else if (desc.rowPitch > 0)
    {
        mImageSize = (desc.rowPitch * mExtent.height * mExtent.depth);
    }
    else
    {
        mImageSize = (mExtent.height * mExtent.width * mExtent.depth * mElementSize);
    }

    if ((desc.type == cl::MemObjectType::Image1D_Array) ||
        (desc.type == cl::MemObjectType::Image2D_Array))
    {
        mArrayLayers = static_cast<uint32_t>(desc.arraySize);
        mImageSize *= desc.arraySize;
    }

    ANGLE_CL_IMPL_TRY_ERROR(
        mImage.initStaging(mContext, false, mRenderer->getMemoryProperties(), getVkImageType(desc),
                           mExtent, mFormat, mFormat, VK_SAMPLE_COUNT_1_BIT, getVkImageUsageFlags(),
                           1, mArrayLayers),
        CL_OUT_OF_RESOURCES);

    if (mMemory.getFlags().intersects(CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR))
    {
        ASSERT(hostPtr);
        ANGLE_CL_IMPL_TRY_ERROR(createStagingBuffer(mImageSize), CL_OUT_OF_RESOURCES);
        ANGLE_CL_IMPL_TRY_ERROR(copyStagingFrom(hostPtr, 0, mImageSize), CL_OUT_OF_RESOURCES);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset      = 0;
        copyRegion.bufferRowLength   = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel       = (int)desc.numMipLevels;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount     = 1;

        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {mExtent.width, mExtent.height, mExtent.depth};

        ANGLE_CL_IMPL_TRY_ERROR(mImage.copyToBufferOneOff(mContext, &mStagingBuffer, copyRegion),
                                CL_OUT_OF_RESOURCES);
    }

    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.flags                 = 0;
    viewInfo.image                 = getImage().getImage().getHandle();
    viewInfo.format                = getImage().getActualVkFormat();

    viewInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = static_cast<uint32_t>(getArraySize());
    viewInfo.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;

    viewInfo.viewType = mImageViewType;
    ANGLE_VK_TRY(mContext, mImageView.init(mContext->getDevice(), viewInfo));

    return angle::Result::Continue;
}

bool CLImageVk::isCurrentlyInUse() const
{
    return !mRenderer->hasResourceUseFinished(mImage.getResourceUse());
}

bool CLImageVk::containsHostMemExtension()
{
    const vk::ExtensionNameList &enabledDeviceExtensions = mRenderer->getEnabledDeviceExtensions();
    return std::find(enabledDeviceExtensions.begin(), enabledDeviceExtensions.end(),
                     "VK_EXT_external_memory_host") != enabledDeviceExtensions.end();
}

angle::Result CLImageVk::mapImpl()
{
    ASSERT(!isMapped());

    ASSERT(isStagingBufferInitialized());
    ANGLE_TRY(getStagingBuffer().map(mContext, &mMappedMemory));

    return angle::Result::Continue;
}
void CLImageVk::unmapImpl()
{
    getStagingBuffer().unmap(mContext->getRenderer());
    mMappedMemory = nullptr;
}

}  // namespace rx

//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/renderer/wgpu/wgpu_helpers.h"

#include <algorithm>

#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/wgpu/ContextWgpu.h"
#include "libANGLE/renderer/wgpu/DisplayWgpu.h"
#include "libANGLE/renderer/wgpu/FramebufferWgpu.h"

namespace rx
{
namespace webgpu
{
namespace
{
WGPUTextureDescriptor TextureDescriptorFromTexture(const webgpu::TextureHandle &texture)
{
    WGPUTextureDescriptor descriptor = WGPU_TEXTURE_DESCRIPTOR_INIT;
    descriptor.usage                 = wgpuTextureGetUsage(texture.get());
    descriptor.dimension             = wgpuTextureGetDimension(texture.get());
    descriptor.size   = {wgpuTextureGetWidth(texture.get()), wgpuTextureGetHeight(texture.get()),
                         wgpuTextureGetDepthOrArrayLayers(texture.get())};
    descriptor.format = wgpuTextureGetFormat(texture.get());
    descriptor.mipLevelCount   = wgpuTextureGetMipLevelCount(texture.get());
    descriptor.sampleCount     = wgpuTextureGetSampleCount(texture.get());
    descriptor.viewFormatCount = 0;
    return descriptor;
}

size_t GetSafeBufferMapOffset(size_t offset)
{
    static_assert(gl::isPow2(kBufferMapOffsetAlignment));
    return roundDownPow2(offset, kBufferMapOffsetAlignment);
}

size_t GetSafeBufferMapSize(size_t offset, size_t size)
{
    // The offset is rounded down for alignment and the size is rounded up. The safe size must cover
    // both of these offsets.
    size_t offsetChange = offset % kBufferMapOffsetAlignment;
    static_assert(gl::isPow2(kBufferMapSizeAlignment));
    return roundUpPow2(size + offsetChange, kBufferMapSizeAlignment);
}

uint8_t *AdjustMapPointerForOffset(uint8_t *mapPtr, size_t offset)
{
    // Fix up a map pointer that has been adjusted for alignment
    size_t offsetChange = offset % kBufferMapOffsetAlignment;
    return mapPtr + offsetChange;
}

const uint8_t *AdjustMapPointerForOffset(const uint8_t *mapPtr, size_t offset)
{
    return AdjustMapPointerForOffset(const_cast<uint8_t *>(mapPtr), offset);
}

}  // namespace

ImageHelper::ImageHelper() {}

ImageHelper::~ImageHelper() {}

angle::Result ImageHelper::initImage(angle::FormatID intendedFormatID,
                                     angle::FormatID actualFormatID,
                                     DeviceHandle device,
                                     gl::LevelIndex firstAllocatedLevel,
                                     WGPUTextureDescriptor textureDescriptor)
{
    mIntendedFormatID    = intendedFormatID;
    mActualFormatID      = actualFormatID;
    mTextureDescriptor   = textureDescriptor;
    mFirstAllocatedLevel = firstAllocatedLevel;
    mTexture = TextureHandle::Acquire(wgpuDeviceCreateTexture(device.get(), &mTextureDescriptor));
    mInitialized         = true;

    return angle::Result::Continue;
}

angle::Result ImageHelper::initExternal(angle::FormatID intendedFormatID,
                                        angle::FormatID actualFormatID,
                                        webgpu::TextureHandle externalTexture)
{
    mIntendedFormatID    = intendedFormatID;
    mActualFormatID      = actualFormatID;
    mTextureDescriptor   = TextureDescriptorFromTexture(externalTexture);
    mFirstAllocatedLevel = gl::LevelIndex(0);
    mTexture             = externalTexture;
    mInitialized         = true;

    return angle::Result::Continue;
}

angle::Result ImageHelper::flushStagedUpdates(ContextWgpu *contextWgpu)
{
    if (mSubresourceQueue.empty())
    {
        return angle::Result::Continue;
    }
    for (gl::LevelIndex currentMipLevel = mFirstAllocatedLevel;
         currentMipLevel < mFirstAllocatedLevel + getLevelCount(); ++currentMipLevel)
    {
        ANGLE_TRY(flushSingleLevelUpdates(contextWgpu, currentMipLevel, nullptr, 0));
    }
    return angle::Result::Continue;
}

angle::Result ImageHelper::flushSingleLevelUpdates(ContextWgpu *contextWgpu,
                                                   gl::LevelIndex levelGL,
                                                   ClearValuesArray *deferredClears,
                                                   uint32_t deferredClearIndex)
{
    std::vector<SubresourceUpdate> *currentLevelQueue = getLevelUpdates(levelGL);
    if (!currentLevelQueue || currentLevelQueue->empty())
    {
        return angle::Result::Continue;
    }
    WGPUTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    dst.texture                  = mTexture.get();
    std::vector<PackedRenderPassColorAttachment> colorAttachments;
    TextureViewHandle textureView;
    ANGLE_TRY(createTextureViewSingleLevel(levelGL, 0, textureView));
    bool updateDepth      = false;
    bool updateStencil    = false;
    float depthValue      = 1;
    uint32_t stencilValue = 0;
    for (const SubresourceUpdate &srcUpdate : *currentLevelQueue)
    {
        if (!isTextureLevelInAllocatedImage(srcUpdate.targetLevel))
        {
            continue;
        }
        switch (srcUpdate.updateSource)
        {
            case UpdateSource::Texture:
            {
                LevelIndex wgpuLevel    = toWgpuLevel(srcUpdate.targetLevel);
                dst.mipLevel            = wgpuLevel.get();
                WGPUExtent3D copyExtent = getLevelSize(wgpuLevel);

                ANGLE_TRY(contextWgpu->flush(webgpu::RenderPassClosureReason::CopyBufferToTexture));
                contextWgpu->ensureCommandEncoderCreated();
                CommandEncoderHandle encoder = contextWgpu->getCurrentCommandEncoder();

                WGPUTexelCopyBufferInfo copyInfo = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
                copyInfo.layout                  = srcUpdate.textureDataLayout;
                copyInfo.buffer                  = srcUpdate.textureData.get();
                wgpuCommandEncoderCopyBufferToTexture(encoder.get(), &copyInfo, &dst, &copyExtent);
            }
            break;

            case UpdateSource::Clear:
                if (deferredClears)
                {
                    if (deferredClearIndex == kUnpackedDepthIndex)
                    {
                        if (srcUpdate.clearData.hasStencil)
                        {
                            deferredClears->store(kUnpackedStencilIndex,
                                                  srcUpdate.clearData.clearValues);
                        }
                        if (!srcUpdate.clearData.hasDepth)
                        {
                            break;
                        }
                    }
                    deferredClears->store(deferredClearIndex, srcUpdate.clearData.clearValues);
                }
                else
                {
                    colorAttachments.push_back(CreateNewClearColorAttachment(
                        srcUpdate.clearData.clearValues.clearColor,
                        srcUpdate.clearData.clearValues.depthSlice, textureView));
                    if (srcUpdate.clearData.hasDepth)
                    {
                        updateDepth = true;
                        depthValue  = srcUpdate.clearData.clearValues.depthValue;
                    }
                    if (srcUpdate.clearData.hasStencil)
                    {
                        updateStencil = true;
                        stencilValue  = srcUpdate.clearData.clearValues.stencilValue;
                    }
                }
                break;
        }
    }
    FramebufferWgpu *frameBuffer =
        GetImplAs<FramebufferWgpu>(contextWgpu->getState().getDrawFramebuffer());

    if (!colorAttachments.empty())
    {
        frameBuffer->addNewColorAttachments(colorAttachments);
    }
    if (updateDepth || updateStencil)
    {
        frameBuffer->updateDepthStencilAttachment(CreateNewDepthStencilAttachment(
            depthValue, stencilValue, textureView, updateDepth, updateStencil));
    }
    currentLevelQueue->clear();

    return angle::Result::Continue;
}

WGPUTextureDescriptor ImageHelper::createTextureDescriptor(WGPUTextureUsage usage,
                                                           WGPUTextureDimension dimension,
                                                           WGPUExtent3D size,
                                                           WGPUTextureFormat format,
                                                           std::uint32_t mipLevelCount,
                                                           std::uint32_t sampleCount)
{
    WGPUTextureDescriptor textureDescriptor   = WGPU_TEXTURE_DESCRIPTOR_INIT;
    textureDescriptor.usage                   = usage;
    textureDescriptor.dimension               = dimension;
    textureDescriptor.size                    = size;
    textureDescriptor.format                  = format;
    textureDescriptor.mipLevelCount           = mipLevelCount;
    textureDescriptor.sampleCount             = sampleCount;
    textureDescriptor.viewFormatCount         = 0;
    return textureDescriptor;
}

angle::Result ImageHelper::stageTextureUpload(ContextWgpu *contextWgpu,
                                              const webgpu::Format &webgpuFormat,
                                              GLenum type,
                                              const gl::Extents &glExtents,
                                              GLuint inputRowPitch,
                                              GLuint inputDepthPitch,
                                              uint32_t outputRowPitch,
                                              uint32_t outputDepthPitch,
                                              uint32_t allocationSize,
                                              const gl::ImageIndex &index,
                                              const uint8_t *pixels)
{
    if (pixels == nullptr)
    {
        return angle::Result::Continue;
    }
    webgpu::DeviceHandle device = contextWgpu->getDevice();
    gl::LevelIndex levelGL(index.getLevelIndex());
    BufferHelper bufferHelper;
    WGPUBufferUsage usage = WGPUBufferUsage_CopySrc | WGPUBufferUsage_CopyDst;
    ANGLE_TRY(bufferHelper.initBuffer(device, allocationSize, usage, MapAtCreation::Yes));
    LoadImageFunctionInfo loadFunctionInfo = webgpuFormat.getTextureLoadFunction(type);
    uint8_t *data                          = bufferHelper.getMapWritePointer(0, allocationSize);
    loadFunctionInfo.loadFunction(contextWgpu->getImageLoadContext(), glExtents.width,
                                  glExtents.height, glExtents.depth, pixels, inputRowPitch,
                                  inputDepthPitch, data, outputRowPitch, outputDepthPitch);
    ANGLE_TRY(bufferHelper.unmap());

    WGPUTexelCopyBufferLayout textureDataLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    textureDataLayout.bytesPerRow             = outputRowPitch;
    textureDataLayout.rowsPerImage            = outputDepthPitch;
    appendSubresourceUpdate(
        levelGL, SubresourceUpdate(UpdateSource::Texture, levelGL, bufferHelper.getBuffer(),
                                   textureDataLayout));
    return angle::Result::Continue;
}

void ImageHelper::stageClear(gl::LevelIndex targetLevel,
                             ClearValues clearValues,
                             bool hasDepth,
                             bool hasStencil)
{
    appendSubresourceUpdate(targetLevel, SubresourceUpdate(UpdateSource::Clear, targetLevel,
                                                           clearValues, hasDepth, hasStencil));
}

void ImageHelper::removeStagedUpdates(gl::LevelIndex levelToRemove)
{
    std::vector<SubresourceUpdate> *updateToClear = getLevelUpdates(levelToRemove);
    if (updateToClear)
    {
        updateToClear->clear();
    }
}

void ImageHelper::resetImage()
{
    if (mTexture)
    {
        wgpuTextureDestroy(mTexture.get());
    }
    mTexture             = nullptr;
    mTextureDescriptor   = WGPU_TEXTURE_DESCRIPTOR_INIT;
    mInitialized         = false;
    mFirstAllocatedLevel = gl::LevelIndex(0);
}
// static
angle::Result ImageHelper::getReadPixelsParams(rx::ContextWgpu *contextWgpu,
                                               const gl::PixelPackState &packState,
                                               gl::Buffer *packBuffer,
                                               GLenum format,
                                               GLenum type,
                                               const gl::Rectangle &area,
                                               const gl::Rectangle &clippedArea,
                                               rx::PackPixelsParams *paramsOut,
                                               GLuint *skipBytesOut)
{
    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(format, type);

    GLuint outputPitch = 0;
    ANGLE_CHECK_GL_MATH(contextWgpu,
                        sizedFormatInfo.computeRowPitch(type, area.width, packState.alignment,
                                                        packState.rowLength, &outputPitch));
    ANGLE_CHECK_GL_MATH(contextWgpu, sizedFormatInfo.computeSkipBytes(
                                         type, outputPitch, 0, packState, false, skipBytesOut));

    ANGLE_TRY(GetPackPixelsParams(sizedFormatInfo, outputPitch, packState, packBuffer, area,
                                  clippedArea, paramsOut, skipBytesOut));
    return angle::Result::Continue;
}

angle::Result ImageHelper::readPixels(rx::ContextWgpu *contextWgpu,
                                      const gl::Rectangle &area,
                                      const rx::PackPixelsParams &packPixelsParams,
                                      void *pixels)
{
    if (mActualFormatID == angle::FormatID::NONE)
    {
        // Unimplemented texture format
        UNIMPLEMENTED();
        return angle::Result::Stop;
    }

    DeviceHandle device = contextWgpu->getDisplay()->getDevice();
    CommandEncoderHandle encoder =
        CommandEncoderHandle::Acquire(wgpuDeviceCreateCommandEncoder(device.get(), nullptr));
    QueueHandle queue = contextWgpu->getDisplay()->getQueue();

    const angle::Format &actualFormat = angle::Format::Get(mActualFormatID);
    uint32_t textureBytesPerRow =
        roundUp(actualFormat.pixelBytes * area.width, kCopyBufferAlignment);
    WGPUTexelCopyBufferLayout textureDataLayout = WGPU_TEXEL_COPY_BUFFER_LAYOUT_INIT;
    textureDataLayout.bytesPerRow  = textureBytesPerRow;
    textureDataLayout.rowsPerImage = area.height;

    size_t allocationSize = textureBytesPerRow * area.height;

    BufferHelper bufferHelper;
    ANGLE_TRY(bufferHelper.initBuffer(device, allocationSize,
                                      WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst,
                                      MapAtCreation::No));
    WGPUTexelCopyBufferInfo copyBuffer = WGPU_TEXEL_COPY_BUFFER_INFO_INIT;
    copyBuffer.buffer                  = bufferHelper.getBuffer().get();
    copyBuffer.layout = textureDataLayout;

    WGPUTexelCopyTextureInfo copyTexture WGPU_TEXEL_COPY_TEXTURE_INFO_INIT;
    copyTexture.origin.x = area.x;
    copyTexture.origin.y = area.y;
    copyTexture.texture  = mTexture.get();
    copyTexture.mipLevel = toWgpuLevel(mFirstAllocatedLevel).get();

    WGPUExtent3D copySize = WGPU_EXTENT_3D_INIT;
    copySize.width  = area.width;
    copySize.height = area.height;
    wgpuCommandEncoderCopyTextureToBuffer(encoder.get(), &copyTexture, &copyBuffer, &copySize);

    CommandBufferHandle commandBuffer =
        CommandBufferHandle::Acquire(wgpuCommandEncoderFinish(encoder.get(), nullptr));
    wgpuQueueSubmit(queue.get(), 1, &commandBuffer.get());
    encoder = nullptr;

    ANGLE_TRY(bufferHelper.mapImmediate(contextWgpu, WGPUMapMode_Read, 0, allocationSize));
    const uint8_t *readPixelBuffer = bufferHelper.getMapReadPointer(0, allocationSize);
    PackPixels(packPixelsParams, actualFormat, textureBytesPerRow, readPixelBuffer,
               static_cast<uint8_t *>(pixels));
    return angle::Result::Continue;
}

angle::Result ImageHelper::createTextureViewSingleLevel(gl::LevelIndex targetLevel,
                                                        uint32_t layerIndex,
                                                        TextureViewHandle &textureViewOut)
{
    return createTextureView(targetLevel, /*levelCount=*/1, layerIndex, /*arrayLayerCount=*/1,
                             textureViewOut, WGPUTextureViewDimension_Undefined);
}

angle::Result ImageHelper::createFullTextureView(TextureViewHandle &textureViewOut,
                                                 WGPUTextureViewDimension desiredViewDimension)
{
    return createTextureView(mFirstAllocatedLevel, mTextureDescriptor.mipLevelCount, 0,
                             mTextureDescriptor.size.depthOrArrayLayers, textureViewOut,
                             desiredViewDimension);
}

angle::Result ImageHelper::createTextureView(gl::LevelIndex targetLevel,
                                             uint32_t levelCount,
                                             uint32_t layerIndex,
                                             uint32_t arrayLayerCount,
                                             TextureViewHandle &textureViewOut,
                                             WGPUTextureViewDimension desiredViewDimension)
{
    if (!isTextureLevelInAllocatedImage(targetLevel))
    {
        return angle::Result::Stop;
    }
    WGPUTextureViewDescriptor textureViewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_INIT;
    textureViewDesc.aspect                    = WGPUTextureAspect_All;
    textureViewDesc.baseArrayLayer  = layerIndex;
    textureViewDesc.arrayLayerCount = arrayLayerCount;
    textureViewDesc.baseMipLevel    = toWgpuLevel(targetLevel).get();
    textureViewDesc.mipLevelCount   = levelCount;
    if (desiredViewDimension == WGPUTextureViewDimension_Undefined)
    {
        switch (mTextureDescriptor.dimension)
        {
            case WGPUTextureDimension_Undefined:
                textureViewDesc.dimension = WGPUTextureViewDimension_Undefined;
                break;
            case WGPUTextureDimension_1D:
                textureViewDesc.dimension = WGPUTextureViewDimension_1D;
                break;
            case WGPUTextureDimension_2D:
                textureViewDesc.dimension = WGPUTextureViewDimension_2D;
                break;
            case WGPUTextureDimension_3D:
                textureViewDesc.dimension = WGPUTextureViewDimension_3D;
                break;
            default:
                UNIMPLEMENTED();
                return angle::Result::Stop;
        }
    }
    else
    {
        textureViewDesc.dimension = desiredViewDimension;
    }
    textureViewDesc.format = mTextureDescriptor.format;
    textureViewOut =
        TextureViewHandle::Acquire(wgpuTextureCreateView(mTexture.get(), &textureViewDesc));
    return angle::Result::Continue;
}

gl::LevelIndex ImageHelper::getLastAllocatedLevel()
{
    return mFirstAllocatedLevel + mTextureDescriptor.mipLevelCount - 1;
}

LevelIndex ImageHelper::toWgpuLevel(gl::LevelIndex levelIndexGl) const
{
    return gl_wgpu::getLevelIndex(levelIndexGl, mFirstAllocatedLevel);
}

gl::LevelIndex ImageHelper::toGlLevel(LevelIndex levelIndexWgpu) const
{
    return wgpu_gl::GetLevelIndex(levelIndexWgpu, mFirstAllocatedLevel);
}

bool ImageHelper::isTextureLevelInAllocatedImage(gl::LevelIndex textureLevel)
{
    if (!mInitialized || textureLevel < mFirstAllocatedLevel)
    {
        return false;
    }
    LevelIndex wgpuTextureLevel = toWgpuLevel(textureLevel);
    return wgpuTextureLevel < LevelIndex(mTextureDescriptor.mipLevelCount);
}

WGPUExtent3D ImageHelper::getLevelSize(LevelIndex wgpuLevel)
{
    WGPUExtent3D copyExtent = mTextureDescriptor.size;
    // https://www.w3.org/TR/webgpu/#abstract-opdef-logical-miplevel-specific-texture-extent
    copyExtent.width  = std::max(1u, copyExtent.width >> wgpuLevel.get());
    copyExtent.height = std::max(1u, copyExtent.height >> wgpuLevel.get());
    if (mTextureDescriptor.dimension == WGPUTextureDimension_3D)
    {
        copyExtent.depthOrArrayLayers =
            std::max(1u, copyExtent.depthOrArrayLayers >> wgpuLevel.get());
    }
    return copyExtent;
}

void ImageHelper::appendSubresourceUpdate(gl::LevelIndex level, SubresourceUpdate &&update)
{
    if (mSubresourceQueue.size() <= static_cast<size_t>(level.get()))
    {
        mSubresourceQueue.resize(level.get() + 1);
    }
    mSubresourceQueue[level.get()].emplace_back(std::move(update));
    onStateChange(angle::SubjectMessage::SubjectChanged);
}

std::vector<SubresourceUpdate> *ImageHelper::getLevelUpdates(gl::LevelIndex level)
{
    return static_cast<size_t>(level.get()) < mSubresourceQueue.size()
               ? &mSubresourceQueue[level.get()]
               : nullptr;
}

BufferHelper::BufferHelper() {}

BufferHelper::~BufferHelper() {}

void BufferHelper::reset()
{
    mBuffer = nullptr;
    mMappedState.reset();
}

angle::Result BufferHelper::initBuffer(webgpu::DeviceHandle device,
                                       size_t size,
                                       WGPUBufferUsage usage,
                                       MapAtCreation mappedAtCreation)
{
    size_t safeBufferSize = rx::roundUpPow2(size, kBufferSizeAlignment);
    WGPUBufferDescriptor descriptor = WGPU_BUFFER_DESCRIPTOR_INIT;
    descriptor.size             = safeBufferSize;
    descriptor.usage            = usage;
    descriptor.mappedAtCreation = mappedAtCreation == MapAtCreation::Yes;

    mBuffer = webgpu::BufferHandle::Acquire(wgpuDeviceCreateBuffer(device.get(), &descriptor));

    if (mappedAtCreation == MapAtCreation::Yes)
    {
        mMappedState = {WGPUMapMode_Read | WGPUMapMode_Write, 0, safeBufferSize};
    }
    else
    {
        mMappedState.reset();
    }

    mRequestedSize = size;

    return angle::Result::Continue;
}

angle::Result BufferHelper::mapImmediate(ContextWgpu *context,
                                         WGPUMapMode mode,
                                         size_t offset,
                                         size_t size)
{
    ASSERT(!mMappedState.has_value());

    WGPUMapAsyncStatus mapResult               = WGPUMapAsyncStatus_Error;
    WGPUBufferMapCallbackInfo mapAsyncCallback = WGPU_BUFFER_MAP_CALLBACK_INFO_INIT;
    mapAsyncCallback.mode                      = WGPUCallbackMode_WaitAnyOnly;
    mapAsyncCallback.callback = [](WGPUMapAsyncStatus status, struct WGPUStringView message,
                                   void *userdata1, void *userdata2) {
        WGPUMapAsyncStatus *pStatus = reinterpret_cast<WGPUMapAsyncStatus *>(userdata1);
        ASSERT(userdata2 == nullptr);

        *pStatus = status;
    };
    mapAsyncCallback.userdata1 = &mapResult;

    size_t safeBufferMapOffset = GetSafeBufferMapOffset(offset);
    size_t safeBufferMapSize   = GetSafeBufferMapSize(offset, size);
    WGPUFutureWaitInfo waitInfo;
    waitInfo.future = wgpuBufferMapAsync(mBuffer.get(), mode, safeBufferMapOffset,
                                         safeBufferMapSize, mapAsyncCallback);

    webgpu::InstanceHandle instance = context->getDisplay()->getInstance();
    ANGLE_WGPU_TRY(context, wgpuInstanceWaitAny(instance.get(), 1, &waitInfo, -1));
    ANGLE_WGPU_TRY(context, mapResult);

    ASSERT(waitInfo.completed);

    mMappedState = {mode, safeBufferMapOffset, safeBufferMapSize};

    return angle::Result::Continue;
}

angle::Result BufferHelper::unmap()
{
    if (mMappedState.has_value())
    {
        wgpuBufferUnmap(mBuffer.get());
        mMappedState.reset();
    }
    return angle::Result::Continue;
}

uint8_t *BufferHelper::getMapWritePointer(size_t offset, size_t size) const
{
    ASSERT(wgpuBufferGetMapState(mBuffer.get()) == WGPUBufferMapState_Mapped);
    ASSERT(mMappedState.has_value());
    ASSERT(mMappedState->offset <= offset);
    ASSERT(mMappedState->offset + mMappedState->size >= offset + size);

    void *mapPtr = wgpuBufferGetMappedRange(mBuffer.get(), GetSafeBufferMapOffset(offset),
                                            GetSafeBufferMapSize(offset, size));
    ASSERT(mapPtr);

    return AdjustMapPointerForOffset(static_cast<uint8_t *>(mapPtr), offset);
}

const uint8_t *BufferHelper::getMapReadPointer(size_t offset, size_t size) const
{
    ASSERT(wgpuBufferGetMapState(mBuffer.get()) == WGPUBufferMapState_Mapped);
    ASSERT(mMappedState.has_value());
    ASSERT(mMappedState->offset <= offset);
    ASSERT(mMappedState->offset + mMappedState->size >= offset + size);

    // wgpuBufferGetConstMappedRange is used for reads whereas wgpuBufferGetMappedRange is only used
    // for writes.
    const void *mapPtr = wgpuBufferGetConstMappedRange(
        mBuffer.get(), GetSafeBufferMapOffset(offset), GetSafeBufferMapSize(offset, size));
    ASSERT(mapPtr);

    return AdjustMapPointerForOffset(static_cast<const uint8_t *>(mapPtr), offset);
}

const std::optional<BufferMapState> &BufferHelper::getMappedState() const
{
    return mMappedState;
}

bool BufferHelper::canMapForRead() const
{
    return (mMappedState.has_value() && (mMappedState->mode & WGPUMapMode_Read)) ||
           (mBuffer && (wgpuBufferGetUsage(mBuffer.get()) & WGPUBufferUsage_MapRead));
}

bool BufferHelper::canMapForWrite() const
{
    return (mMappedState.has_value() && (mMappedState->mode & WGPUMapMode_Write)) ||
           (mBuffer && (wgpuBufferGetUsage(mBuffer.get()) & WGPUBufferUsage_MapWrite));
}

bool BufferHelper::isMappedForRead() const
{
    return mMappedState.has_value() && (mMappedState->mode & WGPUMapMode_Read);
}
bool BufferHelper::isMappedForWrite() const
{
    return mMappedState.has_value() && (mMappedState->mode & WGPUMapMode_Write);
}

webgpu::BufferHandle BufferHelper::getBuffer() const
{
    return mBuffer;
}

uint64_t BufferHelper::requestedSize() const
{
    return mRequestedSize;
}

uint64_t BufferHelper::actualSize() const
{
    return mBuffer ? wgpuBufferGetSize(mBuffer.get()) : 0;
}

angle::Result BufferHelper::readDataImmediate(ContextWgpu *context,
                                              size_t offset,
                                              size_t size,
                                              webgpu::RenderPassClosureReason reason,
                                              BufferReadback *result)
{
    ASSERT(result);

    if (getMappedState())
    {
        ANGLE_TRY(unmap());
    }

    // Create a staging buffer just big enough for this copy but aligned for both copying and
    // mapping.
    const size_t stagingBufferSize = roundUpPow2(
        size, std::max(webgpu::kBufferCopyToBufferAlignment, webgpu::kBufferMapOffsetAlignment));

    ANGLE_TRY(result->buffer.initBuffer(context->getDisplay()->getDevice(), stagingBufferSize,
                                        WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead,
                                        webgpu::MapAtCreation::No));

    // Copy the source buffer to staging and flush the commands
    context->ensureCommandEncoderCreated();
    webgpu::CommandEncoderHandle &commandEncoder = context->getCurrentCommandEncoder();
    size_t safeCopyOffset   = rx::roundDownPow2(offset, webgpu::kBufferCopyToBufferAlignment);
    size_t offsetAdjustment = offset - safeCopyOffset;
    size_t copySize = roundUpPow2(size + offsetAdjustment, webgpu::kBufferCopyToBufferAlignment);
    wgpuCommandEncoderCopyBufferToBuffer(commandEncoder.get(), mBuffer.get(), safeCopyOffset,
                                         result->buffer.getBuffer().get(), 0, copySize);

    ANGLE_TRY(context->flush(reason));

    // Read back from the staging buffer and compute the index range
    ANGLE_TRY(result->buffer.mapImmediate(context, WGPUMapMode_Read, offsetAdjustment, size));
    result->data = result->buffer.getMapReadPointer(offsetAdjustment, size);

    return angle::Result::Continue;
}

}  // namespace webgpu
}  // namespace rx

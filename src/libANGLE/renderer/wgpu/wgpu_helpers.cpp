//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/renderer/wgpu/wgpu_helpers.h"
#include "libANGLE/formatutils.h"

#include "libANGLE/renderer/wgpu/ContextWgpu.h"
#include "libANGLE/renderer/wgpu/DisplayWgpu.h"
#include "wgpu_helpers.h"

namespace rx
{
namespace webgpu
{
ImageHelper::ImageHelper() {}

ImageHelper::~ImageHelper() {}

angle::Result ImageHelper::initImage(wgpu::Device &device,
                                     gl::LevelIndex firstAllocatedLevel,
                                     wgpu::TextureDescriptor textureDescriptor)
{
    mTextureDescriptor   = textureDescriptor;
    mFirstAllocatedLevel = firstAllocatedLevel;
    mTexture             = device.CreateTexture(&mTextureDescriptor);
    mInitialized         = true;

    return angle::Result::Continue;
}

void ImageHelper::flushStagedUpdates(ContextWgpu *contextWgpu)
{
    wgpu::Device device          = contextWgpu->getDevice();
    wgpu::Queue queue            = contextWgpu->getQueue();
    wgpu::CommandEncoder encoder = device.CreateCommandEncoder();
    wgpu::ImageCopyTexture dst;
    dst.texture = mTexture;
    for (const QueuedDataUpload &src : mBufferQueue)
    {
        if (src.targetLevel < mFirstAllocatedLevel ||
            src.targetLevel >= (mFirstAllocatedLevel + mTextureDescriptor.mipLevelCount))
        {
            continue;
        }
        LevelIndex targetLevelWgpu = toWgpuLevel(src.targetLevel);
        dst.mipLevel               = targetLevelWgpu.get();
        encoder.CopyBufferToTexture(&src.buffer, &dst, &mTextureDescriptor.size);
    }
    wgpu::CommandBuffer commandBuffer = encoder.Finish();
    queue.Submit(1, &commandBuffer);
    mBufferQueue.clear();
}

wgpu::TextureDescriptor ImageHelper::createTextureDescriptor(wgpu::TextureUsage usage,
                                                             wgpu::TextureDimension dimension,
                                                             wgpu::Extent3D size,
                                                             wgpu::TextureFormat format,
                                                             std::uint32_t mipLevelCount,
                                                             std::uint32_t sampleCount,
                                                             std::size_t viewFormatCount)
{
    wgpu::TextureDescriptor textureDescriptor = {};
    textureDescriptor.usage                   = usage;
    textureDescriptor.dimension               = dimension;
    textureDescriptor.size                    = size;
    textureDescriptor.format                  = format;
    textureDescriptor.mipLevelCount           = mipLevelCount;
    textureDescriptor.sampleCount             = sampleCount;
    textureDescriptor.viewFormatCount         = viewFormatCount;
    return textureDescriptor;
}

angle::Result ImageHelper::stageTextureUpload(ContextWgpu *contextWgpu,
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
    wgpu::Device device = contextWgpu->getDevice();
    wgpu::Queue queue   = contextWgpu->getQueue();
    gl::LevelIndex levelGL(index.getLevelIndex());
    BufferHelper bufferHelper;
    wgpu::BufferUsage usage = wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::CopyDst;
    ANGLE_TRY(bufferHelper.initBuffer(device, allocationSize, usage, MapAtCreation::Yes));
    LoadImageFunctionInfo loadFunctionInfo = {angle::LoadToNative<GLubyte, 4>, false};
    uint8_t *data                          = bufferHelper.getMapWritePointer(0, allocationSize);
    loadFunctionInfo.loadFunction(contextWgpu->getImageLoadContext(), glExtents.width,
                                  glExtents.height, glExtents.depth, pixels, inputRowPitch,
                                  inputDepthPitch, data, outputRowPitch, outputDepthPitch);
    ANGLE_TRY(bufferHelper.unmap());

    wgpu::TextureDataLayout textureDataLayout = {};
    textureDataLayout.bytesPerRow             = outputRowPitch;
    textureDataLayout.rowsPerImage            = outputDepthPitch;
    wgpu::ImageCopyBuffer imageCopyBuffer;
    imageCopyBuffer.layout      = textureDataLayout;
    imageCopyBuffer.buffer      = bufferHelper.getBuffer();
    QueuedDataUpload dataUpload = {imageCopyBuffer, levelGL};
    mBufferQueue.push_back(dataUpload);
    return angle::Result::Continue;
}

void ImageHelper::removeStagedUpdates(gl::LevelIndex levelToRemove)
{
    for (auto it = mBufferQueue.begin(); it != mBufferQueue.end(); it++)
    {
        if (it->targetLevel == levelToRemove)
        {
            mBufferQueue.erase(it);
        }
    }
}

void ImageHelper::resetImage()
{
    mTexture.Destroy();
    mTextureDescriptor   = {};
    mInitialized         = false;
    mFirstAllocatedLevel = gl::LevelIndex(0);
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
    return wgpu_gl::getLevelIndex(levelIndexWgpu, mFirstAllocatedLevel);
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

BufferHelper::BufferHelper() {}

BufferHelper::~BufferHelper() {}

void BufferHelper::reset()
{
    mBuffer = nullptr;
    mMappedState.reset();
}

angle::Result BufferHelper::initBuffer(wgpu::Device device,
                                       size_t size,
                                       wgpu::BufferUsage usage,
                                       MapAtCreation mappedAtCreation)
{
    wgpu::BufferDescriptor descriptor;
    descriptor.size             = size;
    descriptor.usage            = usage;
    descriptor.mappedAtCreation = mappedAtCreation == MapAtCreation::Yes;

    mBuffer = device.CreateBuffer(&descriptor);

    if (mappedAtCreation == MapAtCreation::Yes)
    {
        mMappedState = {wgpu::MapMode::Read | wgpu::MapMode::Write, 0, size};
    }
    else
    {
        mMappedState.reset();
    }

    return angle::Result::Continue;
}

angle::Result BufferHelper::mapImmediate(ContextWgpu *context,
                                         wgpu::MapMode mode,
                                         size_t offset,
                                         size_t size)
{
    ASSERT(!mMappedState.has_value());

    WGPUBufferMapAsyncStatus mapResult = WGPUBufferMapAsyncStatus_Unknown;

    wgpu::BufferMapCallbackInfo callbackInfo;
    callbackInfo.mode     = wgpu::CallbackMode::WaitAnyOnly;
    callbackInfo.callback = [](WGPUBufferMapAsyncStatus status, void *userdata) {
        *static_cast<WGPUBufferMapAsyncStatus *>(userdata) = status;
    };
    callbackInfo.userdata = &mapResult;

    wgpu::FutureWaitInfo waitInfo;
    waitInfo.future = mBuffer.MapAsync(mode, offset, size, callbackInfo);

    wgpu::Instance instance = context->getDisplay()->getInstance();
    ANGLE_WGPU_TRY(context, instance.WaitAny(1, &waitInfo, -1));
    ANGLE_WGPU_TRY(context, mapResult);

    ASSERT(waitInfo.completed);

    mMappedState = {mode, offset, size};

    return angle::Result::Continue;
}

angle::Result BufferHelper::unmap()
{
    ASSERT(mMappedState.has_value());
    mBuffer.Unmap();
    mMappedState.reset();
    return angle::Result::Continue;
}

uint8_t *BufferHelper::getMapWritePointer(size_t offset, size_t size) const
{
    ASSERT(mBuffer.GetMapState() == wgpu::BufferMapState::Mapped);
    ASSERT(mMappedState.has_value());
    ASSERT(mMappedState->offset <= offset);
    ASSERT(mMappedState->offset + mMappedState->size >= offset + size);

    void *mapPtr = mBuffer.GetMappedRange(offset, size);
    ASSERT(mapPtr);

    return static_cast<uint8_t *>(mapPtr);
}

const std::optional<BufferMapState> &BufferHelper::getMappedState() const
{
    return mMappedState;
}

bool BufferHelper::canMapForRead() const
{
    return (mMappedState.has_value() && (mMappedState->mode & wgpu::MapMode::Read)) ||
           (mBuffer && (mBuffer.GetUsage() & wgpu::BufferUsage::MapRead));
}

bool BufferHelper::canMapForWrite() const
{
    return (mMappedState.has_value() && (mMappedState->mode & wgpu::MapMode::Write)) ||
           (mBuffer && (mBuffer.GetUsage() & wgpu::BufferUsage::MapWrite));
}

wgpu::Buffer &BufferHelper::getBuffer()
{
    return mBuffer;
}

uint64_t BufferHelper::size() const
{
    return mBuffer ? mBuffer.GetSize() : 0;
}
}  // namespace webgpu
}  // namespace rx

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_helpers:
//   Helper utilitiy classes that manage Vulkan resources.

#include "libANGLE/renderer/vulkan/vk_helpers.h"

#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{
namespace vk
{
namespace
{
// TODO(jmadill): Pick non-arbitrary max.
constexpr uint32_t kDynamicDescriptorPoolMaxSets = 2048;

constexpr VkBufferUsageFlags kLineLoopDynamicBufferUsage =
    (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
constexpr int kLineLoopDynamicBufferMinSize = 1024 * 1024;

VkImageUsageFlags GetStagingImageUsageFlags(StagingUsage usage)
{
    switch (usage)
    {
        case StagingUsage::Read:
            return VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        case StagingUsage::Write:
            return VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        case StagingUsage::Both:
            return (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        default:
            UNREACHABLE();
            return 0;
    }
}

// Gets access flags that are common between source and dest layouts.
VkAccessFlags GetBasicLayoutAccessFlags(VkImageLayout layout)
{
    switch (layout)
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_ACCESS_MEMORY_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_GENERAL:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return 0;
        default:
            // TODO(jmadill): Investigate other flags.
            UNREACHABLE();
            return 0;
    }
}

VkImageCreateFlags GetImageCreateFlags(gl::TextureType textureType)
{
    if (textureType == gl::TextureType::CubeMap)
    {
        return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }
    else
    {
        return 0;
    }
}

uint32_t GetImageLayerCount(gl::TextureType textureType)
{
    if (textureType == gl::TextureType::CubeMap)
    {
        return gl::CUBE_FACE_COUNT;
    }
    else
    {
        return 1;
    }
}
}  // anonymous namespace

// DynamicBuffer implementation.
DynamicBuffer::DynamicBuffer(VkBufferUsageFlags usage, size_t minSize)
    : mUsage(usage),
      mMinSize(minSize),
      mNextWriteOffset(0),
      mLastFlushOffset(0),
      mSize(0),
      mAlignment(0),
      mMappedMemory(nullptr)
{
}

void DynamicBuffer::init(size_t alignment)
{
    ASSERT(alignment > 0);
    mAlignment = alignment;
}

DynamicBuffer::~DynamicBuffer()
{
    ASSERT(mAlignment == 0);
}

bool DynamicBuffer::valid()
{
    return mAlignment > 0;
}

Error DynamicBuffer::allocate(RendererVk *renderer,
                              size_t sizeInBytes,
                              uint8_t **ptrOut,
                              VkBuffer *handleOut,
                              uint32_t *offsetOut,
                              bool *newBufferAllocatedOut)
{
    ASSERT(valid());

    size_t sizeToAllocate = roundUp(sizeInBytes, mAlignment);

    angle::base::CheckedNumeric<size_t> checkedNextWriteOffset = mNextWriteOffset;
    checkedNextWriteOffset += sizeToAllocate;

    if (!checkedNextWriteOffset.IsValid() || checkedNextWriteOffset.ValueOrDie() > mSize)
    {
        VkDevice device = renderer->getDevice();

        if (mMappedMemory)
        {
            ANGLE_TRY(flush(device));
            mMemory.unmap(device);
            mMappedMemory = nullptr;
        }

        Serial currentSerial = renderer->getCurrentQueueSerial();
        renderer->releaseObject(currentSerial, &mBuffer);
        renderer->releaseObject(currentSerial, &mMemory);

        VkBufferCreateInfo createInfo;
        createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.pNext                 = nullptr;
        createInfo.flags                 = 0;
        createInfo.size                  = std::max(sizeToAllocate, mMinSize);
        createInfo.usage                 = mUsage;
        createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
        ANGLE_TRY(mBuffer.init(device, createInfo));

        ANGLE_TRY(AllocateBufferMemory(renderer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &mBuffer,
                                       &mMemory, &mSize));
        ANGLE_TRY(mMemory.map(device, 0, mSize, 0, &mMappedMemory));
        mNextWriteOffset = 0;
        mLastFlushOffset = 0;

        if (newBufferAllocatedOut != nullptr)
        {
            *newBufferAllocatedOut = true;
        }
    }
    else if (newBufferAllocatedOut != nullptr)
    {
        *newBufferAllocatedOut = false;
    }

    ASSERT(mBuffer.valid());

    if (handleOut != nullptr)
    {
        *handleOut = mBuffer.getHandle();
    }

    ASSERT(mMappedMemory);
    *ptrOut    = mMappedMemory + mNextWriteOffset;
    *offsetOut = mNextWriteOffset;
    mNextWriteOffset += static_cast<uint32_t>(sizeToAllocate);

    return NoError();
}

Error DynamicBuffer::flush(VkDevice device)
{
    if (mNextWriteOffset > mLastFlushOffset)
    {
        VkMappedMemoryRange range;
        range.sType  = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.pNext  = nullptr;
        range.memory = mMemory.getHandle();
        range.offset = mLastFlushOffset;
        range.size   = mNextWriteOffset - mLastFlushOffset;
        ANGLE_VK_TRY(vkFlushMappedMemoryRanges(device, 1, &range));

        mLastFlushOffset = mNextWriteOffset;
    }
    return NoError();
}

void DynamicBuffer::release(RendererVk *renderer)
{
    mAlignment           = 0;
    Serial currentSerial = renderer->getCurrentQueueSerial();
    renderer->releaseObject(currentSerial, &mBuffer);
    renderer->releaseObject(currentSerial, &mMemory);
}

void DynamicBuffer::destroy(VkDevice device)
{
    mAlignment = 0;
    mBuffer.destroy(device);
    mMemory.destroy(device);
}

VkBuffer DynamicBuffer::getCurrentBufferHandle() const
{
    return mBuffer.getHandle();
}

void DynamicBuffer::setMinimumSizeForTesting(size_t minSize)
{
    // This will really only have an effect next time we call allocate.
    mMinSize = minSize;

    // Forces a new allocation on the next allocate.
    mSize = 0;
}

// DynamicDescriptorPool implementation.
DynamicDescriptorPool::DynamicDescriptorPool()
    : mCurrentAllocatedDescriptorSetCount(0),
      mUniformBufferDescriptorsPerSet(0),
      mCombinedImageSamplerDescriptorsPerSet(0)
{
}

DynamicDescriptorPool::~DynamicDescriptorPool()
{
}

void DynamicDescriptorPool::destroy(RendererVk *rendererVk)
{
    ASSERT(mCurrentDescriptorSetPool.valid());
    mCurrentDescriptorSetPool.destroy(rendererVk->getDevice());
}

Error DynamicDescriptorPool::init(const VkDevice &device,
                                  uint32_t uniformBufferDescriptorsPerSet,
                                  uint32_t combinedImageSamplerDescriptorsPerSet)
{
    ASSERT(!mCurrentDescriptorSetPool.valid() && mCurrentAllocatedDescriptorSetCount == 0);

    mUniformBufferDescriptorsPerSet        = uniformBufferDescriptorsPerSet;
    mCombinedImageSamplerDescriptorsPerSet = combinedImageSamplerDescriptorsPerSet;

    ANGLE_TRY(allocateNewPool(device));
    return NoError();
}

Error DynamicDescriptorPool::allocateDescriptorSets(
    ContextVk *contextVk,
    const VkDescriptorSetLayout *descriptorSetLayout,
    uint32_t descriptorSetCount,
    VkDescriptorSet *descriptorSetsOut)
{
    if (descriptorSetCount + mCurrentAllocatedDescriptorSetCount > kDynamicDescriptorPoolMaxSets)
    {
        RendererVk *renderer = contextVk->getRenderer();
        Serial currentSerial = renderer->getCurrentQueueSerial();

        // We will bust the limit of descriptor set with this allocation so we need to get a new
        // pool for it.
        renderer->releaseObject(currentSerial, &mCurrentDescriptorSetPool);
        ANGLE_TRY(allocateNewPool(contextVk->getDevice()));
    }

    VkDescriptorSetAllocateInfo allocInfo;
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.pNext              = nullptr;
    allocInfo.descriptorPool     = mCurrentDescriptorSetPool.getHandle();
    allocInfo.descriptorSetCount = descriptorSetCount;
    allocInfo.pSetLayouts        = descriptorSetLayout;

    ANGLE_TRY(mCurrentDescriptorSetPool.allocateDescriptorSets(contextVk->getDevice(), allocInfo,
                                                               descriptorSetsOut));
    mCurrentAllocatedDescriptorSetCount += allocInfo.descriptorSetCount;
    return NoError();
}

Error DynamicDescriptorPool::allocateNewPool(const VkDevice &device)
{
    VkDescriptorPoolSize poolSizes[DescriptorPoolIndexCount];
    poolSizes[UniformBufferIndex].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[UniformBufferIndex].descriptorCount =
        mUniformBufferDescriptorsPerSet * kDynamicDescriptorPoolMaxSets / DescriptorPoolIndexCount;
    poolSizes[TextureIndex].type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[TextureIndex].descriptorCount = mCombinedImageSamplerDescriptorsPerSet *
                                              kDynamicDescriptorPoolMaxSets /
                                              DescriptorPoolIndexCount;

    VkDescriptorPoolCreateInfo descriptorPoolInfo;
    descriptorPoolInfo.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext   = nullptr;
    descriptorPoolInfo.flags   = 0;
    descriptorPoolInfo.maxSets = kDynamicDescriptorPoolMaxSets;

    // Reserve pools for uniform blocks and textures.
    descriptorPoolInfo.poolSizeCount = DescriptorPoolIndexCount;
    descriptorPoolInfo.pPoolSizes    = poolSizes;

    mCurrentAllocatedDescriptorSetCount = 0;
    ANGLE_TRY(mCurrentDescriptorSetPool.init(device, descriptorPoolInfo));
    return NoError();
}

LineLoopHelper::LineLoopHelper()
    : mDynamicIndexBuffer(kLineLoopDynamicBufferUsage, kLineLoopDynamicBufferMinSize)
{
    // We need to use an alignment of the maximum size we're going to allocate, which is
    // VK_INDEX_TYPE_UINT32. When we switch from a drawElement to a drawArray call, the allocations
    // can vary in size. According to the Vulkan spec, when calling vkCmdBindIndexBuffer: 'The
    // sum of offset and the address of the range of VkDeviceMemory object that is backing buffer,
    // must be a multiple of the type indicated by indexType'.
    mDynamicIndexBuffer.init(sizeof(uint32_t));
}

LineLoopHelper::~LineLoopHelper() = default;

gl::Error LineLoopHelper::getIndexBufferForDrawArrays(RendererVk *renderer,
                                                      const gl::DrawCallParams &drawCallParams,
                                                      VkBuffer *bufferHandleOut,
                                                      VkDeviceSize *offsetOut)
{
    uint32_t *indices    = nullptr;
    size_t allocateBytes = sizeof(uint32_t) * (drawCallParams.vertexCount() + 1);
    uint32_t offset      = 0;
    ANGLE_TRY(mDynamicIndexBuffer.allocate(renderer, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           &offset, nullptr));
    *offsetOut = static_cast<VkDeviceSize>(offset);

    uint32_t unsignedFirstVertex = static_cast<uint32_t>(drawCallParams.firstVertex());
    uint32_t vertexCount         = (drawCallParams.vertexCount() + unsignedFirstVertex);
    for (uint32_t vertexIndex = unsignedFirstVertex; vertexIndex < vertexCount; vertexIndex++)
    {
        *indices++ = vertexIndex;
    }
    *indices = unsignedFirstVertex;

    // Since we are not using the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag when creating the
    // device memory in the StreamingBuffer, we always need to make sure we flush it after
    // writing.
    ANGLE_TRY(mDynamicIndexBuffer.flush(renderer->getDevice()));

    return gl::NoError();
}

gl::Error LineLoopHelper::getIndexBufferForElementArrayBuffer(RendererVk *renderer,
                                                              BufferVk *elementArrayBufferVk,
                                                              VkIndexType indexType,
                                                              int indexCount,
                                                              intptr_t elementArrayOffset,
                                                              VkBuffer *bufferHandleOut,
                                                              VkDeviceSize *bufferOffsetOut)
{
    ASSERT(indexType == VK_INDEX_TYPE_UINT16 || indexType == VK_INDEX_TYPE_UINT32);

    uint32_t *indices = nullptr;
    uint32_t destinationOffset = 0;

    auto unitSize = (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
    size_t allocateBytes = unitSize * (indexCount + 1);
    ANGLE_TRY(mDynamicIndexBuffer.allocate(renderer, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           &destinationOffset, nullptr));
    *bufferOffsetOut = static_cast<VkDeviceSize>(destinationOffset);

    VkDeviceSize sourceOffset = static_cast<VkDeviceSize>(elementArrayOffset);
    uint64_t unitCount        = static_cast<VkDeviceSize>(indexCount);
    VkBufferCopy copy1        = {sourceOffset, destinationOffset, unitCount * unitSize};
    VkBufferCopy copy2        = {sourceOffset, destinationOffset + unitCount * unitSize, unitSize};
    std::array<VkBufferCopy, 2> copies = {{copy1, copy2}};

    vk::CommandBuffer *commandBuffer;
    beginWriteResource(renderer, &commandBuffer);

    Serial currentSerial = renderer->getCurrentQueueSerial();
    elementArrayBufferVk->onReadResource(getCurrentWritingNode(), currentSerial);
    commandBuffer->copyBuffer(elementArrayBufferVk->getVkBuffer().getHandle(), *bufferHandleOut, 2,
                              copies.data());

    ANGLE_TRY(mDynamicIndexBuffer.flush(renderer->getDevice()));
    return gl::NoError();
}

gl::Error LineLoopHelper::getIndexBufferForClientElementArray(RendererVk *renderer,
                                                              const void *indicesInput,
                                                              VkIndexType indexType,
                                                              int indexCount,
                                                              VkBuffer *bufferHandleOut,
                                                              VkDeviceSize *bufferOffsetOut)
{
    // TODO(lucferron): we'll eventually need to support uint8, emulated on 16 since Vulkan only
    // supports 16 / 32.
    ASSERT(indexType == VK_INDEX_TYPE_UINT16 || indexType == VK_INDEX_TYPE_UINT32);

    uint8_t *indices = nullptr;
    uint32_t offset  = 0;

    auto unitSize = (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
    size_t allocateBytes = unitSize * (indexCount + 1);
    ANGLE_TRY(mDynamicIndexBuffer.allocate(renderer, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           &offset, nullptr));
    *bufferOffsetOut = static_cast<VkDeviceSize>(offset);

    memcpy(indices, indicesInput, unitSize * indexCount);
    memcpy(indices + unitSize * indexCount, indicesInput, unitSize);

    ANGLE_TRY(mDynamicIndexBuffer.flush(renderer->getDevice()));
    return gl::NoError();
}

void LineLoopHelper::destroy(VkDevice device)
{
    mDynamicIndexBuffer.destroy(device);
}

// static
void LineLoopHelper::Draw(int count, CommandBuffer *commandBuffer)
{
    // Our first index is always 0 because that's how we set it up in createIndexBuffer*.
    commandBuffer->drawIndexed(count + 1, 1, 0, 0, 0);
}

// ImageHelper implementation.
ImageHelper::ImageHelper()
    : mFormat(nullptr),
      mSamples(0),
      mAllocatedMemorySize(0),
      mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
      mLayerCount(0)
{
}

ImageHelper::ImageHelper(ImageHelper &&other)
    : mImage(std::move(other.mImage)),
      mDeviceMemory(std::move(other.mDeviceMemory)),
      mExtents(other.mExtents),
      mFormat(other.mFormat),
      mSamples(other.mSamples),
      mAllocatedMemorySize(other.mAllocatedMemorySize),
      mCurrentLayout(other.mCurrentLayout),
      mLayerCount(other.mLayerCount)
{
    other.mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    other.mLayerCount    = 0;
}

ImageHelper::~ImageHelper()
{
    ASSERT(!valid());
}

bool ImageHelper::valid() const
{
    return mImage.valid();
}

Error ImageHelper::init(VkDevice device,
                        gl::TextureType textureType,
                        const gl::Extents &extents,
                        const Format &format,
                        GLint samples,
                        VkImageUsageFlags usage)
{
    ASSERT(!valid());

    mExtents    = extents;
    mFormat     = &format;
    mSamples    = samples;
    mLayerCount = GetImageLayerCount(textureType);

    VkImageCreateInfo imageInfo;
    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext                 = nullptr;
    imageInfo.flags                 = GetImageCreateFlags(textureType);
    imageInfo.imageType             = gl_vk::GetImageType(textureType);
    imageInfo.format                = format.vkTextureFormat;
    imageInfo.extent.width          = static_cast<uint32_t>(extents.width);
    imageInfo.extent.height         = static_cast<uint32_t>(extents.height);
    imageInfo.extent.depth          = 1;
    imageInfo.mipLevels             = 1;
    imageInfo.arrayLayers           = mLayerCount;
    imageInfo.samples               = gl_vk::GetSamples(samples);
    imageInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage                 = usage;
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    ANGLE_TRY(mImage.init(device, imageInfo));
    return NoError();
}

void ImageHelper::release(Serial serial, RendererVk *renderer)
{
    renderer->releaseObject(serial, &mImage);
    renderer->releaseObject(serial, &mDeviceMemory);
}

void ImageHelper::resetImageWeakReference()
{
    mImage.reset();
}

Error ImageHelper::initMemory(VkDevice device,
                              const MemoryProperties &memoryProperties,
                              VkMemoryPropertyFlags flags)
{
    // TODO(jmadill): Memory sub-allocation. http://anglebug.com/2162
    ANGLE_TRY(AllocateImageMemory(device, memoryProperties, flags, &mImage, &mDeviceMemory,
                                  &mAllocatedMemorySize));
    return NoError();
}

Error ImageHelper::initImageView(VkDevice device,
                                 gl::TextureType textureType,
                                 VkImageAspectFlags aspectMask,
                                 const gl::SwizzleState &swizzleMap,
                                 ImageView *imageViewOut)
{
    VkImageViewCreateInfo viewInfo;
    viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.pNext                           = nullptr;
    viewInfo.flags                           = 0;
    viewInfo.image                           = mImage.getHandle();
    viewInfo.viewType                        = gl_vk::GetImageViewType(textureType);
    viewInfo.format                          = mFormat->vkTextureFormat;
    viewInfo.components.r                    = gl_vk::GetSwizzle(swizzleMap.swizzleRed);
    viewInfo.components.g                    = gl_vk::GetSwizzle(swizzleMap.swizzleGreen);
    viewInfo.components.b                    = gl_vk::GetSwizzle(swizzleMap.swizzleBlue);
    viewInfo.components.a                    = gl_vk::GetSwizzle(swizzleMap.swizzleAlpha);
    viewInfo.subresourceRange.aspectMask     = aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount     = mLayerCount;

    ANGLE_TRY(imageViewOut->init(device, viewInfo));
    return NoError();
}

void ImageHelper::destroy(VkDevice device)
{
    mImage.destroy(device);
    mDeviceMemory.destroy(device);
    mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    mLayerCount    = 0;
}

void ImageHelper::init2DWeakReference(VkImage handle,
                                      const gl::Extents &extents,
                                      const Format &format,
                                      GLint samples)
{
    ASSERT(!valid());

    mExtents    = extents;
    mFormat     = &format;
    mSamples    = samples;
    mLayerCount = 1;

    mImage.setHandle(handle);
}

Error ImageHelper::init2DStaging(VkDevice device,
                                 const MemoryProperties &memoryProperties,
                                 const Format &format,
                                 const gl::Extents &extents,
                                 StagingUsage usage)
{
    ASSERT(!valid());

    mExtents    = extents;
    mFormat     = &format;
    mSamples    = 1;
    mLayerCount = 1;

    // Use Preinitialized for writable staging images - in these cases we want to map the memory
    // before we do a copy. For readback images, use an undefined layout.
    mCurrentLayout =
        usage == StagingUsage::Read ? VK_IMAGE_LAYOUT_UNDEFINED : VK_IMAGE_LAYOUT_PREINITIALIZED;

    VkImageCreateInfo imageInfo;
    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext                 = nullptr;
    imageInfo.flags                 = 0;
    imageInfo.imageType             = VK_IMAGE_TYPE_2D;
    imageInfo.format                = format.vkTextureFormat;
    imageInfo.extent.width          = static_cast<uint32_t>(extents.width);
    imageInfo.extent.height         = static_cast<uint32_t>(extents.height);
    imageInfo.extent.depth          = 1;
    imageInfo.mipLevels             = 1;
    imageInfo.arrayLayers           = 1;
    imageInfo.samples               = gl_vk::GetSamples(mSamples);
    imageInfo.tiling                = VK_IMAGE_TILING_LINEAR;
    imageInfo.usage                 = GetStagingImageUsageFlags(usage);
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = mCurrentLayout;

    ANGLE_TRY(mImage.init(device, imageInfo));

    // Allocate and bind host visible and coherent Image memory.
    // TODO(ynovikov): better approach would be to request just visible memory,
    // and call vkInvalidateMappedMemoryRanges if the allocated memory is not coherent.
    // This would solve potential issues of:
    // 1) not having (enough) coherent memory and 2) coherent memory being slower
    VkMemoryPropertyFlags memoryPropertyFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ANGLE_TRY(initMemory(device, memoryProperties, memoryPropertyFlags));

    return NoError();
}

void ImageHelper::dumpResources(Serial serial, std::vector<GarbageObject> *garbageQueue)
{
    mImage.dumpResources(serial, garbageQueue);
    mDeviceMemory.dumpResources(serial, garbageQueue);
}

const Image &ImageHelper::getImage() const
{
    return mImage;
}

const DeviceMemory &ImageHelper::getDeviceMemory() const
{
    return mDeviceMemory;
}

const gl::Extents &ImageHelper::getExtents() const
{
    return mExtents;
}

const Format &ImageHelper::getFormat() const
{
    return *mFormat;
}

GLint ImageHelper::getSamples() const
{
    return mSamples;
}

size_t ImageHelper::getAllocatedMemorySize() const
{
    return mAllocatedMemorySize;
}

void ImageHelper::changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                         VkImageLayout newLayout,
                                         VkPipelineStageFlags srcStageMask,
                                         VkPipelineStageFlags dstStageMask,
                                         CommandBuffer *commandBuffer)
{
    VkImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.pNext               = nullptr;
    imageMemoryBarrier.srcAccessMask       = 0;
    imageMemoryBarrier.dstAccessMask       = 0;
    imageMemoryBarrier.oldLayout           = mCurrentLayout;
    imageMemoryBarrier.newLayout           = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image               = mImage.getHandle();

    // TODO(jmadill): Is this needed for mipped/layer images?
    imageMemoryBarrier.subresourceRange.aspectMask     = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
    imageMemoryBarrier.subresourceRange.levelCount     = 1;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.layerCount     = mLayerCount;

    // TODO(jmadill): Test all the permutations of the access flags.
    imageMemoryBarrier.srcAccessMask = GetBasicLayoutAccessFlags(mCurrentLayout);

    if (mCurrentLayout == VK_IMAGE_LAYOUT_PREINITIALIZED)
    {
        imageMemoryBarrier.srcAccessMask |= VK_ACCESS_HOST_WRITE_BIT;
    }

    imageMemoryBarrier.dstAccessMask = GetBasicLayoutAccessFlags(newLayout);

    if (newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask |=
            (VK_ACCESS_HOST_WRITE_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
        imageMemoryBarrier.dstAccessMask |= VK_ACCESS_SHADER_READ_BIT;
    }

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
        imageMemoryBarrier.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    }

    commandBuffer->singleImageBarrier(srcStageMask, dstStageMask, 0, imageMemoryBarrier);

    mCurrentLayout = newLayout;
}

void ImageHelper::clearColor(const VkClearColorValue &color, CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           commandBuffer);

    VkImageSubresourceRange range;
    range.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel   = 0;
    range.levelCount     = 1;
    range.baseArrayLayer = 0;
    range.layerCount     = mLayerCount;

    commandBuffer->clearColorImage(mImage, mCurrentLayout, color, 1, &range);
}

void ImageHelper::clearDepthStencil(VkImageAspectFlags aspectFlags,
                                    const VkClearDepthStencilValue &depthStencil,
                                    CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    changeLayoutWithStages(aspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           commandBuffer);

    VkImageSubresourceRange clearRange = {
        /*aspectMask*/ aspectFlags,
        /*baseMipLevel*/ 0,
        /*levelCount*/ 1,
        /*baseArrayLayer*/ 0,
        /*layerCount*/ 1,
    };

    commandBuffer->clearDepthStencilImage(mImage, mCurrentLayout, depthStencil, 1, &clearRange);
}

// static
void ImageHelper::Copy(ImageHelper *srcImage,
                       ImageHelper *dstImage,
                       const gl::Offset &srcOffset,
                       const gl::Offset &dstOffset,
                       const gl::Extents &copySize,
                       VkImageAspectFlags aspectMask,
                       CommandBuffer *commandBuffer)
{
    ASSERT(commandBuffer->valid() && srcImage->valid() && dstImage->valid());

    if (srcImage->getCurrentLayout() != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
        srcImage->getCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL)
    {
        srcImage->changeLayoutWithStages(
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);
    }

    if (dstImage->getCurrentLayout() != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        dstImage->getCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL)
    {
        dstImage->changeLayoutWithStages(
            VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);
    }

    VkImageCopy region;
    region.srcSubresource.aspectMask     = aspectMask;
    region.srcSubresource.mipLevel       = 0;
    region.srcSubresource.baseArrayLayer = 0;
    region.srcSubresource.layerCount     = 1;
    region.srcOffset.x                   = srcOffset.x;
    region.srcOffset.y                   = srcOffset.y;
    region.srcOffset.z                   = srcOffset.z;
    region.dstSubresource.aspectMask     = aspectMask;
    region.dstSubresource.mipLevel       = 0;
    region.dstSubresource.baseArrayLayer = 0;
    region.dstSubresource.layerCount     = 1;
    region.dstOffset.x                   = dstOffset.x;
    region.dstOffset.y                   = dstOffset.y;
    region.dstOffset.z                   = dstOffset.z;
    region.extent.width                  = copySize.width;
    region.extent.height                 = copySize.height;
    region.extent.depth                  = copySize.depth;

    commandBuffer->copyImage(srcImage->getImage(), srcImage->getCurrentLayout(),
                             dstImage->getImage(), dstImage->getCurrentLayout(), 1, &region);
}
}  // namespace vk
}  // namespace rx

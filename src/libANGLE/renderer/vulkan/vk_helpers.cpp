//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_helpers:
//   Helper utilitiy classes that manage Vulkan resources.

#include "libANGLE/renderer/vulkan/vk_helpers.h"

#include "common/utilities.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{
namespace vk
{
namespace
{
constexpr VkBufferUsageFlags kLineLoopDynamicBufferUsage =
    (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
constexpr int kLineLoopDynamicBufferMinSize = 1024 * 1024;

// This is an arbitrary max. We can change this later if necessary.
constexpr uint32_t kDefaultDescriptorPoolMaxSets = 2048;

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
}  // anonymous namespace

// DynamicBuffer implementation.
DynamicBuffer::DynamicBuffer(VkBufferUsageFlags usage, size_t minSize)
    : mUsage(usage),
      mMinSize(minSize),
      mHostCoherent(false),
      mNextAllocationOffset(0),
      mLastFlushOrInvalidateOffset(0),
      mSize(0),
      mAlignment(0),
      mMappedMemory(nullptr)
{
}

void DynamicBuffer::init(size_t alignment, RendererVk *renderer)
{
    // Workaround for the mock ICD not supporting allocations greater than 0x1000.
    // Could be removed if https://github.com/KhronosGroup/Vulkan-Tools/issues/84 is fixed.
    if (renderer->isMockICDEnabled())
    {
        mMinSize = std::min<size_t>(mMinSize, 0x1000);
    }

    ASSERT(alignment > 0);
    mAlignment = std::max(
        alignment,
        static_cast<size_t>(renderer->getPhysicalDeviceProperties().limits.nonCoherentAtomSize));
}

DynamicBuffer::~DynamicBuffer()
{
}

angle::Result DynamicBuffer::allocate(Context *context,
                                      size_t sizeInBytes,
                                      uint8_t **ptrOut,
                                      VkBuffer *handleOut,
                                      VkDeviceSize *offsetOut,
                                      bool *newBufferAllocatedOut)
{
    size_t sizeToAllocate = roundUp(sizeInBytes, mAlignment);

    angle::base::CheckedNumeric<size_t> checkedNextWriteOffset = mNextAllocationOffset;
    checkedNextWriteOffset += sizeToAllocate;

    if (!checkedNextWriteOffset.IsValid() || checkedNextWriteOffset.ValueOrDie() >= mSize)
    {
        if (mMappedMemory)
        {
            ANGLE_TRY(flush(context));
            unmap(context->getDevice());
        }

        mRetainedBuffers.emplace_back(std::move(mBuffer), std::move(mMemory));

        mSize = std::max(sizeToAllocate, mMinSize);

        VkBufferCreateInfo createInfo    = {};
        createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        createInfo.flags                 = 0;
        createInfo.size                  = mSize;
        createInfo.usage                 = mUsage;
        createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices   = nullptr;
        ANGLE_VK_TRY(context, mBuffer.init(context->getDevice(), createInfo));

        VkMemoryPropertyFlags actualMemoryPropertyFlags = 0;
        ANGLE_TRY(AllocateBufferMemory(context, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                       &actualMemoryPropertyFlags, &mBuffer, &mMemory));
        mHostCoherent = (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ==
                         (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & actualMemoryPropertyFlags));

        ANGLE_VK_TRY(context, mMemory.map(context->getDevice(), 0, mSize, 0, &mMappedMemory));
        mNextAllocationOffset        = 0;
        mLastFlushOrInvalidateOffset = 0;

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
    *ptrOut    = mMappedMemory + mNextAllocationOffset;
    *offsetOut = static_cast<VkDeviceSize>(mNextAllocationOffset);
    mNextAllocationOffset += static_cast<uint32_t>(sizeToAllocate);
    return angle::Result::Continue();
}

angle::Result DynamicBuffer::flush(Context *context)
{
    if (!mHostCoherent && (mNextAllocationOffset > mLastFlushOrInvalidateOffset))
    {
        VkMappedMemoryRange range = {};
        range.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory              = mMemory.getHandle();
        range.offset              = mLastFlushOrInvalidateOffset;
        range.size                = mNextAllocationOffset - mLastFlushOrInvalidateOffset;
        ANGLE_VK_TRY(context, vkFlushMappedMemoryRanges(context->getDevice(), 1, &range));

        mLastFlushOrInvalidateOffset = mNextAllocationOffset;
    }
    return angle::Result::Continue();
}

angle::Result DynamicBuffer::invalidate(Context *context)
{
    if (!mHostCoherent && (mNextAllocationOffset > mLastFlushOrInvalidateOffset))
    {
        VkMappedMemoryRange range = {};
        range.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        range.memory              = mMemory.getHandle();
        range.offset              = mLastFlushOrInvalidateOffset;
        range.size                = mNextAllocationOffset - mLastFlushOrInvalidateOffset;
        ANGLE_VK_TRY(context, vkInvalidateMappedMemoryRanges(context->getDevice(), 1, &range));

        mLastFlushOrInvalidateOffset = mNextAllocationOffset;
    }
    return angle::Result::Continue();
}

void DynamicBuffer::release(RendererVk *renderer)
{
    unmap(renderer->getDevice());
    reset();
    releaseRetainedBuffers(renderer);

    Serial currentSerial = renderer->getCurrentQueueSerial();
    renderer->releaseObject(currentSerial, &mBuffer);
    renderer->releaseObject(currentSerial, &mMemory);
}

void DynamicBuffer::releaseRetainedBuffers(RendererVk *renderer)
{
    for (BufferAndMemory &toFree : mRetainedBuffers)
    {
        Serial currentSerial = renderer->getCurrentQueueSerial();
        renderer->releaseObject(currentSerial, &toFree.buffer);
        renderer->releaseObject(currentSerial, &toFree.memory);
    }

    mRetainedBuffers.clear();
}

void DynamicBuffer::destroy(VkDevice device)
{
    unmap(device);
    reset();

    for (BufferAndMemory &toFree : mRetainedBuffers)
    {
        toFree.buffer.destroy(device);
        toFree.memory.destroy(device);
    }

    mRetainedBuffers.clear();

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

void DynamicBuffer::unmap(VkDevice device)
{
    if (mMappedMemory)
    {
        mMemory.unmap(device);
        mMappedMemory = nullptr;
    }
}

void DynamicBuffer::reset()
{
    mSize                        = 0;
    mNextAllocationOffset        = 0;
    mLastFlushOrInvalidateOffset = 0;
}

// DescriptorPoolHelper implementation.
DescriptorPoolHelper::DescriptorPoolHelper() : mFreeDescriptorSets(0)
{
}

DescriptorPoolHelper::~DescriptorPoolHelper() = default;

bool DescriptorPoolHelper::hasCapacity(uint32_t descriptorSetCount) const
{
    return mFreeDescriptorSets >= descriptorSetCount;
}

angle::Result DescriptorPoolHelper::init(Context *context,
                                         const VkDescriptorPoolSize &poolSize,
                                         uint32_t maxSets)
{
    if (mDescriptorPool.valid())
    {
        // This could be improved by recycling the descriptor pool.
        mDescriptorPool.destroy(context->getDevice());
    }

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags                      = 0;
    descriptorPoolInfo.maxSets                    = maxSets;
    descriptorPoolInfo.poolSizeCount              = 1u;
    descriptorPoolInfo.pPoolSizes                 = &poolSize;

    mFreeDescriptorSets = maxSets;

    ANGLE_VK_TRY(context, mDescriptorPool.init(context->getDevice(), descriptorPoolInfo));
    return angle::Result::Continue();
}

void DescriptorPoolHelper::destroy(VkDevice device)
{
    mDescriptorPool.destroy(device);
}

angle::Result DescriptorPoolHelper::allocateSets(Context *context,
                                                 const VkDescriptorSetLayout *descriptorSetLayout,
                                                 uint32_t descriptorSetCount,
                                                 VkDescriptorSet *descriptorSetsOut)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = mDescriptorPool.getHandle();
    allocInfo.descriptorSetCount          = descriptorSetCount;
    allocInfo.pSetLayouts                 = descriptorSetLayout;

    ASSERT(mFreeDescriptorSets >= descriptorSetCount);
    mFreeDescriptorSets -= descriptorSetCount;

    ANGLE_VK_TRY(context, mDescriptorPool.allocateDescriptorSets(context->getDevice(), allocInfo,
                                                                 descriptorSetsOut));
    return angle::Result::Continue();
}

// DynamicDescriptorPool implementation.
DynamicDescriptorPool::DynamicDescriptorPool()
    : mMaxSetsPerPool(kDefaultDescriptorPoolMaxSets), mCurrentPoolIndex(0), mPoolSize{}
{
}

DynamicDescriptorPool::~DynamicDescriptorPool() = default;

angle::Result DynamicDescriptorPool::init(ContextVk *contextVk,
                                          VkDescriptorType descriptorType,
                                          uint32_t descriptorsPerSet)
{
    ASSERT(mCurrentPoolIndex == 0);
    ASSERT(mDescriptorPools.empty() || (mDescriptorPools.size() == 1 &&
                                        mDescriptorPools[0]->get().hasCapacity(mMaxSetsPerPool)));

    mPoolSize.type            = descriptorType;
    mPoolSize.descriptorCount = descriptorsPerSet * mMaxSetsPerPool;

    mDescriptorPools.push_back(new SharedDescriptorPoolHelper());
    return mDescriptorPools[0]->get().init(contextVk, mPoolSize, mMaxSetsPerPool);
}

void DynamicDescriptorPool::destroy(VkDevice device)
{
    for (SharedDescriptorPoolHelper *pool : mDescriptorPools)
    {
        ASSERT(!pool->isReferenced());
        pool->get().destroy(device);
        delete pool;
    }

    mDescriptorPools.clear();
}

angle::Result DynamicDescriptorPool::allocateSets(ContextVk *contextVk,
                                                  const VkDescriptorSetLayout *descriptorSetLayout,
                                                  uint32_t descriptorSetCount,
                                                  SharedDescriptorPoolBinding *bindingOut,
                                                  VkDescriptorSet *descriptorSetsOut)
{
    if (!bindingOut->valid() || !bindingOut->get().hasCapacity(descriptorSetCount))
    {
        if (!mDescriptorPools[mCurrentPoolIndex]->get().hasCapacity(descriptorSetCount))
        {
            ANGLE_TRY(allocateNewPool(contextVk));
        }

        // Make sure the old binding knows the descriptor sets can still be in-use. We only need
        // to update the serial when we move to a new pool. This is because we only check serials
        // when we move to a new pool.
        if (bindingOut->valid())
        {
            Serial currentSerial = contextVk->getRenderer()->getCurrentQueueSerial();
            bindingOut->get().updateSerial(currentSerial);
        }

        bindingOut->set(mDescriptorPools[mCurrentPoolIndex]);
    }

    return bindingOut->get().allocateSets(contextVk, descriptorSetLayout, descriptorSetCount,
                                          descriptorSetsOut);
}

angle::Result DynamicDescriptorPool::allocateNewPool(ContextVk *contextVk)
{
    RendererVk *renderer = contextVk->getRenderer();

    bool found = false;

    for (size_t poolIndex = 0; poolIndex < mDescriptorPools.size(); ++poolIndex)
    {
        if (!mDescriptorPools[poolIndex]->isReferenced() &&
            !renderer->isSerialInUse(mDescriptorPools[poolIndex]->get().getSerial()))
        {
            // The newly allocated pool must be a different index from the current pool.
            ASSERT(poolIndex != mCurrentPoolIndex);
            mCurrentPoolIndex = poolIndex;
            found             = true;
            break;
        }
    }

    if (!found)
    {
        mDescriptorPools.push_back(new SharedDescriptorPoolHelper());
        mCurrentPoolIndex = mDescriptorPools.size() - 1;

        static constexpr size_t kMaxPools = 99999;
        ANGLE_VK_CHECK(contextVk, mDescriptorPools.size() < kMaxPools, VK_ERROR_TOO_MANY_OBJECTS);
    }

    return mDescriptorPools[mCurrentPoolIndex]->get().init(contextVk, mPoolSize, mMaxSetsPerPool);
}

void DynamicDescriptorPool::setMaxSetsPerPoolForTesting(uint32_t maxSetsPerPool)
{
    mMaxSetsPerPool = maxSetsPerPool;
}

// DynamicallyGrowingPool implementation
template <typename Pool>
DynamicallyGrowingPool<Pool>::DynamicallyGrowingPool()
    : mPoolSize(0), mCurrentPool(0), mCurrentFreeEntry(0)
{
}

template <typename Pool>
DynamicallyGrowingPool<Pool>::~DynamicallyGrowingPool() = default;

template <typename Pool>
angle::Result DynamicallyGrowingPool<Pool>::initEntryPool(Context *context, uint32_t poolSize)
{
    ASSERT(mPools.empty() && mPoolStats.empty());
    mPoolSize = poolSize;
    return angle::Result::Continue();
}

template <typename Pool>
void DynamicallyGrowingPool<Pool>::destroyEntryPool()
{
    mPools.clear();
    mPoolStats.clear();
}

template <typename Pool>
bool DynamicallyGrowingPool<Pool>::findFreeEntryPool(Context *context)
{
    Serial lastCompletedQueueSerial = context->getRenderer()->getLastCompletedQueueSerial();
    for (size_t i = 0; i < mPools.size(); ++i)
    {
        if (mPoolStats[i].freedCount == mPoolSize &&
            mPoolStats[i].serial <= lastCompletedQueueSerial)
        {
            mCurrentPool      = i;
            mCurrentFreeEntry = 0;

            mPoolStats[i].freedCount = 0;

            return true;
        }
    }

    return false;
}

template <typename Pool>
angle::Result DynamicallyGrowingPool<Pool>::allocateNewEntryPool(Context *context, Pool &&pool)
{
    mPools.push_back(std::move(pool));

    PoolStats poolStats = {0, Serial()};
    mPoolStats.push_back(poolStats);

    mCurrentPool      = mPools.size() - 1;
    mCurrentFreeEntry = 0;

    return angle::Result::Continue();
}

template <typename Pool>
void DynamicallyGrowingPool<Pool>::onEntryFreed(Context *context, size_t poolIndex)
{
    ASSERT(poolIndex < mPoolStats.size() && mPoolStats[poolIndex].freedCount < mPoolSize);

    // Take note of the current serial to avoid reallocating a query in the same pool
    mPoolStats[poolIndex].serial = context->getRenderer()->getCurrentQueueSerial();
    ++mPoolStats[poolIndex].freedCount;
}

// DynamicQueryPool implementation
DynamicQueryPool::DynamicQueryPool() = default;

DynamicQueryPool::~DynamicQueryPool() = default;

angle::Result DynamicQueryPool::init(Context *context, VkQueryType type, uint32_t poolSize)
{
    ANGLE_TRY(initEntryPool(context, poolSize));

    mQueryType = type;
    ANGLE_TRY(allocateNewPool(context));

    return angle::Result::Continue();
}

void DynamicQueryPool::destroy(VkDevice device)
{
    for (QueryPool &queryPool : mPools)
    {
        queryPool.destroy(device);
    }

    destroyEntryPool();
}

angle::Result DynamicQueryPool::allocateQuery(Context *context, QueryHelper *queryOut)
{
    ASSERT(!queryOut->getQueryPool());

    size_t poolIndex    = 0;
    uint32_t queryIndex = 0;
    ANGLE_TRY(allocateQuery(context, &poolIndex, &queryIndex));

    queryOut->init(this, poolIndex, queryIndex);

    return angle::Result::Continue();
}

void DynamicQueryPool::freeQuery(Context *context, QueryHelper *query)
{
    if (query->getQueryPool())
    {
        size_t poolIndex = query->getQueryPoolIndex();
        ASSERT(query->getQueryPool()->valid());

        freeQuery(context, poolIndex, query->getQuery());

        query->deinit();
    }
}

angle::Result DynamicQueryPool::allocateQuery(Context *context,
                                              size_t *poolIndex,
                                              uint32_t *queryIndex)
{
    if (mCurrentFreeEntry >= mPoolSize)
    {
        // No more queries left in this pool, create another one.
        ANGLE_TRY(allocateNewPool(context));
    }

    *poolIndex  = mCurrentPool;
    *queryIndex = mCurrentFreeEntry++;

    return angle::Result::Continue();
}

void DynamicQueryPool::freeQuery(Context *context, size_t poolIndex, uint32_t queryIndex)
{
    ANGLE_UNUSED_VARIABLE(queryIndex);
    onEntryFreed(context, poolIndex);
}

angle::Result DynamicQueryPool::allocateNewPool(Context *context)
{
    if (findFreeEntryPool(context))
    {
        return angle::Result::Continue();
    }

    VkQueryPoolCreateInfo queryPoolInfo = {};
    queryPoolInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.flags                 = 0;
    queryPoolInfo.queryType             = mQueryType;
    queryPoolInfo.queryCount            = mPoolSize;
    queryPoolInfo.pipelineStatistics    = 0;

    vk::QueryPool queryPool;

    ANGLE_VK_TRY(context, queryPool.init(context->getDevice(), queryPoolInfo));

    return allocateNewEntryPool(context, std::move(queryPool));
}

// QueryHelper implementation
QueryHelper::QueryHelper()
    : QueryGraphResource(), mDynamicQueryPool(nullptr), mQueryPoolIndex(0), mQuery(0)
{
}

QueryHelper::~QueryHelper()
{
}

void QueryHelper::init(const DynamicQueryPool *dynamicQueryPool,
                       const size_t queryPoolIndex,
                       uint32_t query)
{
    mDynamicQueryPool = dynamicQueryPool;
    mQueryPoolIndex   = queryPoolIndex;
    mQuery            = query;
}

void QueryHelper::deinit()
{
    mDynamicQueryPool = nullptr;
    mQueryPoolIndex   = 0;
    mQuery            = 0;
}

// DynamicSemaphorePool implementation
DynamicSemaphorePool::DynamicSemaphorePool() = default;

DynamicSemaphorePool::~DynamicSemaphorePool() = default;

angle::Result DynamicSemaphorePool::init(Context *context, uint32_t poolSize)
{
    ANGLE_TRY(initEntryPool(context, poolSize));
    ANGLE_TRY(allocateNewPool(context));
    return angle::Result::Continue();
}

void DynamicSemaphorePool::destroy(VkDevice device)
{
    for (auto &semaphorePool : mPools)
    {
        for (Semaphore &semaphore : semaphorePool)
        {
            semaphore.destroy(device);
        }
    }

    destroyEntryPool();
}

angle::Result DynamicSemaphorePool::allocateSemaphore(Context *context,
                                                      SemaphoreHelper *semaphoreOut)
{
    ASSERT(!semaphoreOut->getSemaphore());

    if (mCurrentFreeEntry >= mPoolSize)
    {
        // No more queries left in this pool, create another one.
        ANGLE_TRY(allocateNewPool(context));
    }

    semaphoreOut->init(mCurrentPool, &mPools[mCurrentPool][mCurrentFreeEntry++]);

    return angle::Result::Continue();
}

void DynamicSemaphorePool::freeSemaphore(Context *context, SemaphoreHelper *semaphore)
{
    if (semaphore->getSemaphore())
    {
        onEntryFreed(context, semaphore->getSemaphorePoolIndex());
        semaphore->deinit();
    }
}

angle::Result DynamicSemaphorePool::allocateNewPool(Context *context)
{
    if (findFreeEntryPool(context))
    {
        return angle::Result::Continue();
    }

    std::vector<Semaphore> newPool(mPoolSize);

    for (Semaphore &semaphore : newPool)
    {
        ANGLE_VK_TRY(context, semaphore.init(context->getDevice()));
    }

    // This code is safe as long as the growth of the outer vector in vector<vector<T>> is done by
    // moving the inner vectors, making sure references to the inner vector remain intact.
    Semaphore *assertMove = mPools.size() > 0 ? mPools[0].data() : nullptr;

    ANGLE_TRY(allocateNewEntryPool(context, std::move(newPool)));

    ASSERT(assertMove == nullptr || assertMove == mPools[0].data());

    return angle::Result::Continue();
}

// SemaphoreHelper implementation
SemaphoreHelper::SemaphoreHelper() : mSemaphorePoolIndex(0), mSemaphore(0)
{
}

SemaphoreHelper::~SemaphoreHelper()
{
}

SemaphoreHelper::SemaphoreHelper(SemaphoreHelper &&other)
    : mSemaphorePoolIndex(other.mSemaphorePoolIndex), mSemaphore(other.mSemaphore)
{
    other.mSemaphore = nullptr;
}

SemaphoreHelper &SemaphoreHelper::operator=(SemaphoreHelper &&other)
{
    std::swap(mSemaphorePoolIndex, other.mSemaphorePoolIndex);
    std::swap(mSemaphore, other.mSemaphore);
    return *this;
}

void SemaphoreHelper::init(const size_t semaphorePoolIndex, const vk::Semaphore *semaphore)
{
    mSemaphorePoolIndex = semaphorePoolIndex;
    mSemaphore          = semaphore;
}

void SemaphoreHelper::deinit()
{
    mSemaphorePoolIndex = 0;
    mSemaphore          = nullptr;
}

// LineLoopHelper implementation.
LineLoopHelper::LineLoopHelper(RendererVk *renderer)
    : mDynamicIndexBuffer(kLineLoopDynamicBufferUsage, kLineLoopDynamicBufferMinSize)
{
    // We need to use an alignment of the maximum size we're going to allocate, which is
    // VK_INDEX_TYPE_UINT32. When we switch from a drawElement to a drawArray call, the allocations
    // can vary in size. According to the Vulkan spec, when calling vkCmdBindIndexBuffer: 'The
    // sum of offset and the address of the range of VkDeviceMemory object that is backing buffer,
    // must be a multiple of the type indicated by indexType'.
    mDynamicIndexBuffer.init(sizeof(uint32_t), renderer);
}

LineLoopHelper::~LineLoopHelper() = default;

angle::Result LineLoopHelper::getIndexBufferForDrawArrays(ContextVk *contextVk,
                                                          uint32_t clampedVertexCount,
                                                          GLint firstVertex,
                                                          VkBuffer *bufferHandleOut,
                                                          VkDeviceSize *offsetOut)
{
    uint32_t *indices    = nullptr;
    size_t allocateBytes = sizeof(uint32_t) * (static_cast<size_t>(clampedVertexCount) + 1);

    mDynamicIndexBuffer.releaseRetainedBuffers(contextVk->getRenderer());
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           offsetOut, nullptr));

    // Note: there could be an overflow in this addition.
    uint32_t unsignedFirstVertex = static_cast<uint32_t>(firstVertex);
    uint32_t vertexCount         = (clampedVertexCount + unsignedFirstVertex);
    for (uint32_t vertexIndex = unsignedFirstVertex; vertexIndex < vertexCount; vertexIndex++)
    {
        *indices++ = vertexIndex;
    }
    *indices = unsignedFirstVertex;

    // Since we are not using the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag when creating the
    // device memory in the StreamingBuffer, we always need to make sure we flush it after
    // writing.
    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));

    return angle::Result::Continue();
}

angle::Result LineLoopHelper::getIndexBufferForElementArrayBuffer(ContextVk *contextVk,
                                                                  BufferVk *elementArrayBufferVk,
                                                                  GLenum glIndexType,
                                                                  int indexCount,
                                                                  intptr_t elementArrayOffset,
                                                                  VkBuffer *bufferHandleOut,
                                                                  VkDeviceSize *bufferOffsetOut)
{
    if (glIndexType == GL_UNSIGNED_BYTE)
    {
        // Needed before reading buffer or we could get stale data.
        ANGLE_TRY(contextVk->getRenderer()->finish(contextVk));

        void *srcDataMapping = nullptr;
        ANGLE_TRY(elementArrayBufferVk->mapImpl(contextVk, &srcDataMapping));
        ANGLE_TRY(streamIndices(contextVk, glIndexType, indexCount,
                                static_cast<const uint8_t *>(srcDataMapping) + elementArrayOffset,
                                bufferHandleOut, bufferOffsetOut));
        ANGLE_TRY(elementArrayBufferVk->unmapImpl(contextVk));
        return angle::Result::Continue();
    }

    VkIndexType indexType = gl_vk::GetIndexType(glIndexType);
    ASSERT(indexType == VK_INDEX_TYPE_UINT16 || indexType == VK_INDEX_TYPE_UINT32);
    uint32_t *indices = nullptr;

    auto unitSize = (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
    size_t allocateBytes = unitSize * (indexCount + 1) + 1;

    mDynamicIndexBuffer.releaseRetainedBuffers(contextVk->getRenderer());
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           bufferOffsetOut, nullptr));

    VkDeviceSize sourceOffset                  = static_cast<VkDeviceSize>(elementArrayOffset);
    uint64_t unitCount                         = static_cast<VkDeviceSize>(indexCount);
    angle::FixedVector<VkBufferCopy, 3> copies = {
        {sourceOffset, *bufferOffsetOut, unitCount * unitSize},
        {sourceOffset, *bufferOffsetOut + unitCount * unitSize, unitSize},
    };
    if (contextVk->getRenderer()->getFeatures().extraCopyBufferRegion)
        copies.push_back({sourceOffset, *bufferOffsetOut + (unitCount + 1) * unitSize, 1});

    ANGLE_TRY(elementArrayBufferVk->copyToBuffer(contextVk, *bufferHandleOut, copies.size(),
                                                 copies.data()));
    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));
    return angle::Result::Continue();
}

angle::Result LineLoopHelper::streamIndices(ContextVk *contextVk,
                                            GLenum glIndexType,
                                            GLsizei indexCount,
                                            const uint8_t *srcPtr,
                                            VkBuffer *bufferHandleOut,
                                            VkDeviceSize *bufferOffsetOut)
{
    VkIndexType indexType = gl_vk::GetIndexType(glIndexType);

    uint8_t *indices = nullptr;

    auto unitSize = (indexType == VK_INDEX_TYPE_UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));
    size_t allocateBytes = unitSize * (indexCount + 1);
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), bufferHandleOut,
                                           bufferOffsetOut, nullptr));

    if (glIndexType == GL_UNSIGNED_BYTE)
    {
        // Vulkan doesn't support uint8 index types, so we need to emulate it.
        ASSERT(indexType == VK_INDEX_TYPE_UINT16);
        uint16_t *indicesDst = reinterpret_cast<uint16_t *>(indices);
        for (int i = 0; i < indexCount; i++)
        {
            indicesDst[i] = srcPtr[i];
        }

        indicesDst[indexCount] = srcPtr[0];
    }
    else
    {
        memcpy(indices, srcPtr, unitSize * indexCount);
        memcpy(indices + unitSize * indexCount, srcPtr, unitSize);
    }

    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));
    return angle::Result::Continue();
}

void LineLoopHelper::release(RendererVk *renderer)
{
    mDynamicIndexBuffer.release(renderer);
}

void LineLoopHelper::destroy(VkDevice device)
{
    mDynamicIndexBuffer.destroy(device);
}

// static
void LineLoopHelper::Draw(uint32_t count, CommandBuffer *commandBuffer)
{
    // Our first index is always 0 because that's how we set it up in createIndexBuffer*.
    // Note: this could theoretically overflow and wrap to zero.
    commandBuffer->drawIndexed(count + 1, 1, 0, 0, 0);
}

// BufferHelper implementation.
BufferHelper::BufferHelper()
    : RecordableGraphResource(CommandGraphResourceType::Buffer),
      mMemoryPropertyFlags{},
      mSize(0),
      mCurrentWriteAccess(0),
      mCurrentReadAccess(0)
{
}

BufferHelper::~BufferHelper() = default;

angle::Result BufferHelper::init(Context *context,
                                 const VkBufferCreateInfo &createInfo,
                                 VkMemoryPropertyFlags memoryPropertyFlags)
{
    mSize = createInfo.size;
    ANGLE_VK_TRY(context, mBuffer.init(context->getDevice(), createInfo));
    return vk::AllocateBufferMemory(context, memoryPropertyFlags, &mMemoryPropertyFlags, &mBuffer,
                                    &mDeviceMemory);
}

void BufferHelper::release(RendererVk *renderer)
{
    mSize = 0;

    renderer->releaseObject(getStoredQueueSerial(), &mBuffer);
    renderer->releaseObject(getStoredQueueSerial(), &mBufferView);
    renderer->releaseObject(getStoredQueueSerial(), &mDeviceMemory);
}

void BufferHelper::onFramebufferRead(FramebufferHelper *framebuffer, VkAccessFlagBits accessType)
{
    addReadDependency(framebuffer);

    if ((mCurrentWriteAccess != 0) && ((mCurrentReadAccess & accessType) == 0))
    {
        framebuffer->addGlobalMemoryBarrier(mCurrentWriteAccess, accessType);
        mCurrentReadAccess |= accessType;
    }
}

angle::Result BufferHelper::copyFromBuffer(Context *context,
                                           const Buffer &buffer,
                                           const VkBufferCopy &copyRegion)
{
    // 'recordCommands' will implicitly stop any reads from using the old buffer data.
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(recordCommands(context, &commandBuffer));

    if (mCurrentReadAccess != 0 || mCurrentWriteAccess != 0)
    {
        // Insert a barrier to ensure reads/writes are complete.
        // Use a global memory barrier to keep things simple.
        VkMemoryBarrier memoryBarrier = {};
        memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask   = mCurrentReadAccess;
        memoryBarrier.dstAccessMask   = VK_ACCESS_TRANSFER_WRITE_BIT;

        commandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 1, &memoryBarrier, 0,
                                       nullptr, 0, nullptr);

        mCurrentWriteAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
        mCurrentReadAccess  = 0;
    }

    commandBuffer->copyBuffer(buffer, mBuffer, 1, &copyRegion);

    return angle::Result::Continue();
}

angle::Result BufferHelper::initBufferView(Context *context, const Format &format)
{
    ASSERT(!mBufferView.valid());
    ASSERT(format.valid());

    VkBufferViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType                  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    viewCreateInfo.buffer                 = mBuffer.getHandle();
    viewCreateInfo.format                 = format.vkBufferFormat;
    viewCreateInfo.offset                 = 0;
    viewCreateInfo.range                  = mSize;

    ANGLE_VK_TRY(context, mBufferView.init(context->getDevice(), viewCreateInfo));

    return angle::Result::Continue();
}

// ImageHelper implementation.
ImageHelper::ImageHelper()
    : RecordableGraphResource(CommandGraphResourceType::Image),
      mFormat(nullptr),
      mSamples(0),
      mCurrentLayout(VK_IMAGE_LAYOUT_UNDEFINED),
      mLayerCount(0)
{
}

ImageHelper::ImageHelper(ImageHelper &&other)
    : RecordableGraphResource(CommandGraphResourceType::Image),
      mImage(std::move(other.mImage)),
      mDeviceMemory(std::move(other.mDeviceMemory)),
      mExtents(other.mExtents),
      mFormat(other.mFormat),
      mSamples(other.mSamples),
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

angle::Result ImageHelper::init(Context *context,
                                gl::TextureType textureType,
                                const gl::Extents &extents,
                                const Format &format,
                                GLint samples,
                                VkImageUsageFlags usage,
                                uint32_t mipLevels)
{
    ASSERT(!valid());

    mExtents    = extents;
    mFormat     = &format;
    mSamples    = samples;
    mLayerCount = GetImageLayerCount(textureType);

    VkImageCreateInfo imageInfo     = {};
    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags                 = GetImageCreateFlags(textureType);
    imageInfo.imageType             = gl_vk::GetImageType(textureType);
    imageInfo.format                = format.vkTextureFormat;
    imageInfo.extent.width          = static_cast<uint32_t>(extents.width);
    imageInfo.extent.height         = static_cast<uint32_t>(extents.height);
    imageInfo.extent.depth          = 1;
    imageInfo.mipLevels             = mipLevels;
    imageInfo.arrayLayers           = mLayerCount;
    imageInfo.samples               = gl_vk::GetSamples(samples);
    imageInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage                 = usage;
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    mCurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    ANGLE_VK_TRY(context, mImage.init(context->getDevice(), imageInfo));
    return angle::Result::Continue();
}

void ImageHelper::release(RendererVk *renderer)
{
    renderer->releaseObject(getStoredQueueSerial(), &mImage);
    renderer->releaseObject(getStoredQueueSerial(), &mDeviceMemory);
}

void ImageHelper::resetImageWeakReference()
{
    mImage.reset();
}

angle::Result ImageHelper::initMemory(Context *context,
                                      const MemoryProperties &memoryProperties,
                                      VkMemoryPropertyFlags flags)
{
    // TODO(jmadill): Memory sub-allocation. http://anglebug.com/2162
    ANGLE_TRY(AllocateImageMemory(context, flags, &mImage, &mDeviceMemory));
    return angle::Result::Continue();
}

angle::Result ImageHelper::initImageView(Context *context,
                                         gl::TextureType textureType,
                                         VkImageAspectFlags aspectMask,
                                         const gl::SwizzleState &swizzleMap,
                                         ImageView *imageViewOut,
                                         uint32_t levelCount)
{
    return initLayerImageView(context, textureType, aspectMask, swizzleMap, imageViewOut,
                              levelCount, 0, mLayerCount);
}

angle::Result ImageHelper::initLayerImageView(Context *context,
                                              gl::TextureType textureType,
                                              VkImageAspectFlags aspectMask,
                                              const gl::SwizzleState &swizzleMap,
                                              ImageView *imageViewOut,
                                              uint32_t levelCount,
                                              uint32_t baseArrayLayer,
                                              uint32_t layerCount)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.flags                 = 0;
    viewInfo.image                 = mImage.getHandle();
    viewInfo.viewType              = gl_vk::GetImageViewType(textureType);
    viewInfo.format                = mFormat->vkTextureFormat;
    if (swizzleMap.swizzleRequired())
    {
        viewInfo.components.r = gl_vk::GetSwizzle(swizzleMap.swizzleRed);
        viewInfo.components.g = gl_vk::GetSwizzle(swizzleMap.swizzleGreen);
        viewInfo.components.b = gl_vk::GetSwizzle(swizzleMap.swizzleBlue);
        viewInfo.components.a = gl_vk::GetSwizzle(swizzleMap.swizzleAlpha);
    }
    else
    {
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    viewInfo.subresourceRange.aspectMask     = aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = 0;
    viewInfo.subresourceRange.levelCount     = levelCount;
    viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    viewInfo.subresourceRange.layerCount     = layerCount;

    ANGLE_VK_TRY(context, imageViewOut->init(context->getDevice(), viewInfo));
    return angle::Result::Continue();
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

angle::Result ImageHelper::init2DStaging(Context *context,
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

    VkImageCreateInfo imageInfo     = {};
    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
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

    ANGLE_VK_TRY(context, mImage.init(context->getDevice(), imageInfo));

    // Allocate and bind host visible and coherent Image memory.
    // TODO(ynovikov): better approach would be to request just visible memory,
    // and call vkInvalidateMappedMemoryRanges if the allocated memory is not coherent.
    // This would solve potential issues of:
    // 1) not having (enough) coherent memory and 2) coherent memory being slower
    VkMemoryPropertyFlags memoryPropertyFlags =
        (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    ANGLE_TRY(initMemory(context, memoryProperties, memoryPropertyFlags));

    return angle::Result::Continue();
}

VkImageAspectFlags ImageHelper::getAspectFlags() const
{
    return GetFormatAspectFlags(mFormat->textureFormat());
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

void ImageHelper::changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                         VkImageLayout newLayout,
                                         VkPipelineStageFlags srcStageMask,
                                         VkPipelineStageFlags dstStageMask,
                                         CommandBuffer *commandBuffer)
{
    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType                = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.srcAccessMask        = 0;
    imageMemoryBarrier.dstAccessMask        = 0;
    imageMemoryBarrier.oldLayout            = mCurrentLayout;
    imageMemoryBarrier.newLayout            = newLayout;
    imageMemoryBarrier.srcQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex  = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.image                = mImage.getHandle();

    // TODO(jmadill): Is this needed for mipped/layer images?
    imageMemoryBarrier.subresourceRange.aspectMask     = aspectMask;
    imageMemoryBarrier.subresourceRange.baseMipLevel   = 0;
    imageMemoryBarrier.subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
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

    commandBuffer->pipelineBarrier(srcStageMask, dstStageMask, 0, 0, nullptr, 0, nullptr, 1,
                                   &imageMemoryBarrier);

    mCurrentLayout = newLayout;
}

void ImageHelper::clearColor(const VkClearColorValue &color,
                             uint32_t baseMipLevel,
                             uint32_t levelCount,
                             CommandBuffer *commandBuffer)

{
    clearColorLayer(color, baseMipLevel, levelCount, 0, mLayerCount, commandBuffer);
}

void ImageHelper::clearColorLayer(const VkClearColorValue &color,
                                  uint32_t baseMipLevel,
                                  uint32_t levelCount,
                                  uint32_t baseArrayLayer,
                                  uint32_t layerCount,
                                  CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           commandBuffer);

    VkImageSubresourceRange range = {};
    range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel            = baseMipLevel;
    range.levelCount              = levelCount;
    range.baseArrayLayer          = baseArrayLayer;
    range.layerCount              = layerCount;

    commandBuffer->clearColorImage(mImage, mCurrentLayout, color, 1, &range);
}

void ImageHelper::clearDepthStencil(VkImageAspectFlags imageAspectFlags,
                                    VkImageAspectFlags clearAspectFlags,
                                    const VkClearDepthStencilValue &depthStencil,
                                    CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    changeLayoutWithStages(imageAspectFlags, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                           commandBuffer);

    VkImageSubresourceRange clearRange = {
        /*aspectMask*/ clearAspectFlags,
        /*baseMipLevel*/ 0,
        /*levelCount*/ 1,
        /*baseArrayLayer*/ 0,
        /*layerCount*/ 1,
    };

    commandBuffer->clearDepthStencilImage(mImage, mCurrentLayout, depthStencil, 1, &clearRange);
}

gl::Extents ImageHelper::getSize(const gl::ImageIndex &index) const
{
    ASSERT(mExtents.depth == 1);
    GLint mipLevel = index.getLevelIndex();
    // Level 0 should be the size of the extents, after that every time you increase a level
    // you shrink the extents by half.
    return gl::Extents(std::max(1, mExtents.width >> mipLevel),
                       std::max(1, mExtents.height >> mipLevel), mExtents.depth);
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
            srcImage->getAspectFlags(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);
    }

    if (dstImage->getCurrentLayout() != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        dstImage->getCurrentLayout() != VK_IMAGE_LAYOUT_GENERAL)
    {
        dstImage->changeLayoutWithStages(
            dstImage->getAspectFlags(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, commandBuffer);
    }

    VkImageCopy region                   = {};
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

angle::Result ImageHelper::generateMipmapsWithBlit(ContextVk *contextVk, GLuint maxLevel)
{
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(recordCommands(contextVk, &commandBuffer));

    changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                           commandBuffer);

    // We are able to use blitImage since the image format we are using supports it. This
    // is a faster way we can generate the mips.
    int32_t mipWidth  = mExtents.width;
    int32_t mipHeight = mExtents.height;

    // Manually manage the image memory barrier because it uses a lot more parameters than our
    // usual one.
    VkImageMemoryBarrier barrier            = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = mImage.getHandle();
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = mLayerCount;
    barrier.subresourceRange.levelCount     = 1;

    for (uint32_t mipLevel = 1; mipLevel <= maxLevel; mipLevel++)
    {
        int32_t nextMipWidth  = std::max<int32_t>(1, mipWidth >> 1);
        int32_t nextMipHeight = std::max<int32_t>(1, mipHeight >> 1);

        barrier.subresourceRange.baseMipLevel = mipLevel - 1;
        barrier.oldLayout                     = mCurrentLayout;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

        // We can do it for all layers at once.
        commandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                       VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                                       &barrier);

        VkImageBlit blit                   = {};
        blit.srcOffsets[0]                 = {0, 0, 0};
        blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel       = mipLevel - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount     = mLayerCount;
        blit.dstOffsets[0]                 = {0, 0, 0};
        blit.dstOffsets[1]                 = {nextMipWidth, nextMipHeight, 1};
        blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel       = mipLevel;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount     = mLayerCount;

        mipWidth  = nextMipWidth;
        mipHeight = nextMipHeight;

        commandBuffer->blitImage(mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
    }

    // Transition the last mip level to the same layout as all the other ones, so we can declare
    // our whole image layout to be SRC_OPTIMAL.
    barrier.subresourceRange.baseMipLevel = maxLevel;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    // We can do it for all layers at once.
    commandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, 0, nullptr, 0, nullptr, 1, &barrier);

    // This is just changing the internal state of the image helper so that the next call
    // to changeLayoutWithStages will use this layout as the "oldLayout" argument.
    mCurrentLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    return angle::Result::Continue();
}

// FramebufferHelper implementation.
FramebufferHelper::FramebufferHelper()
    : RecordableGraphResource(CommandGraphResourceType::Framebuffer)
{
}

FramebufferHelper::~FramebufferHelper() = default;

angle::Result FramebufferHelper::init(ContextVk *contextVk,
                                      const VkFramebufferCreateInfo &createInfo)
{
    ANGLE_VK_TRY(contextVk, mFramebuffer.init(contextVk->getDevice(), createInfo));
    return angle::Result::Continue();
}

void FramebufferHelper::release(RendererVk *renderer)
{
    renderer->releaseObject(getStoredQueueSerial(), &mFramebuffer);
}

// ShaderProgramHelper implementation.
ShaderProgramHelper::ShaderProgramHelper() = default;

ShaderProgramHelper::~ShaderProgramHelper() = default;

bool ShaderProgramHelper::valid() const
{
    // This will need to be extended for compute shader support.
    return mShaders[gl::ShaderType::Vertex].valid();
}

void ShaderProgramHelper::destroy(VkDevice device)
{
    mGraphicsPipelines.destroy(device);
    mComputePipeline.destroy(device);
    for (BindingPointer<ShaderAndSerial> &shader : mShaders)
    {
        shader.reset();
    }
}

void ShaderProgramHelper::release(RendererVk *renderer)
{
    mGraphicsPipelines.release(renderer);
    renderer->releaseObject(mComputePipeline.getSerial(), &mComputePipeline.get());
    for (BindingPointer<ShaderAndSerial> &shader : mShaders)
    {
        shader.reset();
    }
}

void ShaderProgramHelper::setShader(gl::ShaderType shaderType, RefCounted<ShaderAndSerial> *shader)
{
    mShaders[shaderType].set(shader);
}

angle::Result ShaderProgramHelper::getGraphicsPipeline(
    Context *context,
    const PipelineLayout &pipelineLayout,
    const GraphicsPipelineDesc &pipelineDesc,
    const gl::AttributesMask &activeAttribLocationsMask,
    PipelineAndSerial **pipelineOut)
{
    RendererVk *renderer = context->getRenderer();

    // Pull in a compatible RenderPass.
    vk::RenderPass *compatibleRenderPass = nullptr;
    ANGLE_TRY(renderer->getCompatibleRenderPass(context, pipelineDesc.getRenderPassDesc(),
                                                &compatibleRenderPass));

    return mGraphicsPipelines.getPipeline(
        context, renderer->getPipelineCache(), *compatibleRenderPass, pipelineLayout,
        activeAttribLocationsMask, mShaders[gl::ShaderType::Vertex].get().get(),
        mShaders[gl::ShaderType::Fragment].get().get(), pipelineDesc, pipelineOut);
}

angle::Result ShaderProgramHelper::getComputePipeline(Context *context,
                                                      const PipelineLayout &pipelineLayout,
                                                      PipelineAndSerial **pipelineOut)
{
    if (mComputePipeline.valid())
    {
        *pipelineOut = &mComputePipeline;
        return angle::Result::Continue();
    }

    RendererVk *renderer = context->getRenderer();

    VkPipelineShaderStageCreateInfo shaderStage = {};
    VkComputePipelineCreateInfo createInfo      = {};

    shaderStage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.flags               = 0;
    shaderStage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module              = mShaders[gl::ShaderType::Compute].get().get().getHandle();
    shaderStage.pName               = "main";
    shaderStage.pSpecializationInfo = nullptr;

    createInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.flags              = 0;
    createInfo.stage              = shaderStage;
    createInfo.layout             = pipelineLayout.getHandle();
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    createInfo.basePipelineIndex  = 0;

    ANGLE_VK_TRY(context, mComputePipeline.get().initCompute(context->getDevice(), createInfo,
                                                             renderer->getPipelineCache()));

    *pipelineOut = &mComputePipeline;
    return angle::Result::Continue();
}

}  // namespace vk
}  // namespace rx

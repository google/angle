//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_helpers:
//   Helper utilitiy classes that manage Vulkan resources.

#ifndef LIBANGLE_RENDERER_VULKAN_VK_HELPERS_H_
#define LIBANGLE_RENDERER_VULKAN_VK_HELPERS_H_

#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace gl
{
class ImageIndex;
}  // namespace gl;

namespace rx
{
namespace vk
{
// A dynamic buffer is conceptually an infinitely long buffer. Each time you write to the buffer,
// you will always write to a previously unused portion. After a series of writes, you must flush
// the buffer data to the device. Buffer lifetime currently assumes that each new allocation will
// last as long or longer than each prior allocation.
//
// Dynamic buffers are used to implement a variety of data streaming operations in Vulkan, such
// as for immediate vertex array and element array data, uniform updates, and other dynamic data.
class DynamicBuffer : angle::NonCopyable
{
  public:
    DynamicBuffer(VkBufferUsageFlags usage, size_t minSize);
    ~DynamicBuffer();

    // Init is called after the buffer creation so that the alignment can be specified later.
    void init(size_t alignment, RendererVk *renderer);

    // This call will allocate a new region at the end of the buffer. It internally may trigger
    // a new buffer to be created (which is returned in 'newBufferAllocatedOut'. This param may
    // be nullptr.
    angle::Result allocate(Context *context,
                           size_t sizeInBytes,
                           uint8_t **ptrOut,
                           VkBuffer *handleOut,
                           VkDeviceSize *offsetOut,
                           bool *newBufferAllocatedOut);

    // After a sequence of writes, call flush to ensure the data is visible to the device.
    angle::Result flush(Context *context);

    // After a sequence of writes, call invalidate to ensure the data is visible to the host.
    angle::Result invalidate(Context *context);

    // This releases resources when they might currently be in use.
    void release(RendererVk *renderer);

    // This releases all the buffers that have been allocated since this was last called.
    void releaseRetainedBuffers(RendererVk *renderer);

    // This frees resources immediately.
    void destroy(VkDevice device);

    VkBuffer getCurrentBufferHandle() const;

    // For testing only!
    void setMinimumSizeForTesting(size_t minSize);

  private:
    void unmap(VkDevice device);
    void reset();

    VkBufferUsageFlags mUsage;
    size_t mMinSize;
    Buffer mBuffer;
    DeviceMemory mMemory;
    bool mHostCoherent;
    uint32_t mNextAllocationOffset;
    uint32_t mLastFlushOrInvalidateOffset;
    size_t mSize;
    size_t mAlignment;
    uint8_t *mMappedMemory;

    std::vector<BufferAndMemory> mRetainedBuffers;
};

// Uses DescriptorPool to allocate descriptor sets as needed. If a descriptor pool becomes full, we
// allocate new pools internally as needed. RendererVk takes care of the lifetime of the discarded
// pools. Note that we used a fixed layout for descriptor pools in ANGLE. Uniform buffers must
// use set zero and combined Image Samplers must use set 1. We conservatively count each new set
// using the maximum number of descriptor sets and buffers with each allocation. Currently: 2
// (Vertex/Fragment) uniform buffers and 64 (MAX_ACTIVE_TEXTURES) image/samplers.

// Shared handle to a descriptor pool. Each helper is allocated from the dynamic descriptor pool.
// Can be used to share descriptor pools between multiple ProgramVks and the ContextVk.
class DescriptorPoolHelper
{
  public:
    DescriptorPoolHelper();
    ~DescriptorPoolHelper();

    bool valid() { return mDescriptorPool.valid(); }

    bool hasCapacity(uint32_t descriptorSetCount) const;
    angle::Result init(Context *context, const VkDescriptorPoolSize &poolSize, uint32_t maxSets);
    void destroy(VkDevice device);

    angle::Result allocateSets(Context *context,
                               const VkDescriptorSetLayout *descriptorSetLayout,
                               uint32_t descriptorSetCount,
                               VkDescriptorSet *descriptorSetsOut);

    void updateSerial(Serial serial) { mMostRecentSerial = serial; }

    Serial getSerial() const { return mMostRecentSerial; }

  private:
    uint32_t mFreeDescriptorSets;
    DescriptorPool mDescriptorPool;
    Serial mMostRecentSerial;
};

using SharedDescriptorPoolHelper  = RefCounted<DescriptorPoolHelper>;
using SharedDescriptorPoolBinding = BindingPointer<DescriptorPoolHelper>;

class DynamicDescriptorPool final : angle::NonCopyable
{
  public:
    DynamicDescriptorPool();
    ~DynamicDescriptorPool();

    // The DynamicDescriptorPool only handles one pool size at this time.
    angle::Result init(ContextVk *contextVk,
                       VkDescriptorType descriptorType,
                       uint32_t descriptorsPerSet);
    void destroy(VkDevice device);

    // We use the descriptor type to help count the number of free sets.
    // By convention, sets are indexed according to the constants in vk_cache_utils.h.
    angle::Result allocateSets(ContextVk *contextVk,
                               const VkDescriptorSetLayout *descriptorSetLayout,
                               uint32_t descriptorSetCount,
                               SharedDescriptorPoolBinding *bindingOut,
                               VkDescriptorSet *descriptorSetsOut);

    // For testing only!
    void setMaxSetsPerPoolForTesting(uint32_t maxSetsPerPool);

  private:
    angle::Result allocateNewPool(ContextVk *contextVk);

    uint32_t mMaxSetsPerPool;
    size_t mCurrentPoolIndex;
    std::vector<SharedDescriptorPoolHelper *> mDescriptorPools;
    VkDescriptorPoolSize mPoolSize;
};

template <typename Pool>
class DynamicallyGrowingPool : angle::NonCopyable
{
  public:
    DynamicallyGrowingPool();
    virtual ~DynamicallyGrowingPool();

    bool isValid() { return mPoolSize > 0; }

  protected:
    angle::Result initEntryPool(Context *context, uint32_t poolSize);
    void destroyEntryPool();

    // Checks to see if any pool is already free, in which case it sets it as current pool and
    // returns true.
    bool findFreeEntryPool(Context *context);

    // Allocates a new entry and initializes it with the given pool.
    angle::Result allocateNewEntryPool(Context *context, Pool &&pool);

    // Called by the implementation whenever an entry is freed.
    void onEntryFreed(Context *context, size_t poolIndex);

    // The pool size, to know when a pool is completely freed.
    uint32_t mPoolSize;

    std::vector<Pool> mPools;

    struct PoolStats
    {
        // A count corresponding to each pool indicating how many of its allocated entries
        // have been freed. Once that value reaches mPoolSize for each pool, that pool is considered
        // free and reusable.  While keeping a bitset would allow allocation of each index, the
        // slight runtime overhead of finding free indices is not worth the slight memory overhead
        // of creating new pools when unnecessary.
        uint32_t freedCount;
        // The serial of the renderer is stored on each object free to make sure no
        // new allocations are made from the pool until it's not in use.
        Serial serial;
    };
    std::vector<PoolStats> mPoolStats;

    // Index into mPools indicating pool we are currently allocating from.
    size_t mCurrentPool;
    // Index inside mPools[mCurrentPool] indicating which index can be allocated next.
    uint32_t mCurrentFreeEntry;
};

// DynamicQueryPool allocates indices out of QueryPool as needed.  Once a QueryPool is exhausted,
// another is created.  The query pools live permanently, but are recycled as indices get freed.

// These are arbitrary default sizes for query pools.
constexpr uint32_t kDefaultOcclusionQueryPoolSize = 64;
constexpr uint32_t kDefaultTimestampQueryPoolSize = 64;

class QueryHelper;

class DynamicQueryPool final : public DynamicallyGrowingPool<QueryPool>
{
  public:
    DynamicQueryPool();
    ~DynamicQueryPool();

    angle::Result init(Context *context, VkQueryType type, uint32_t poolSize);
    void destroy(VkDevice device);

    angle::Result allocateQuery(Context *context, QueryHelper *queryOut);
    void freeQuery(Context *context, QueryHelper *query);

    // Special allocator that doesn't work with QueryHelper, which is a CommandGraphResource.
    // Currently only used with RendererVk::GpuEventQuery.
    angle::Result allocateQuery(Context *context, size_t *poolIndex, uint32_t *queryIndex);
    void freeQuery(Context *context, size_t poolIndex, uint32_t queryIndex);

    const QueryPool *getQueryPool(size_t index) const { return &mPools[index]; }

  private:
    angle::Result allocateNewPool(Context *context);

    // Information required to create new query pools
    VkQueryType mQueryType;
};

// Queries in vulkan are identified by the query pool and an index for a query within that pool.
// Unlike other pools, such as descriptor pools where an allocation returns an independent object
// from the pool, the query allocations are not done through a Vulkan function and are only an
// integer index.
//
// Furthermore, to support arbitrarily large number of queries, DynamicQueryPool creates query pools
// of a fixed size as needed and allocates indices within those pools.
//
// The QueryHelper class below keeps the pool and index pair together.
class QueryHelper final : public QueryGraphResource
{
  public:
    QueryHelper();
    ~QueryHelper();

    void init(const DynamicQueryPool *dynamicQueryPool,
              const size_t queryPoolIndex,
              uint32_t query);
    void deinit();

    const QueryPool *getQueryPool() const
    {
        return mDynamicQueryPool ? mDynamicQueryPool->getQueryPool(mQueryPoolIndex) : nullptr;
    }
    uint32_t getQuery() const { return mQuery; }

    // Used only by DynamicQueryPool.
    size_t getQueryPoolIndex() const { return mQueryPoolIndex; }

  private:
    const DynamicQueryPool *mDynamicQueryPool;
    size_t mQueryPoolIndex;
    uint32_t mQuery;
};

// DynamicSemaphorePool allocates semaphores as needed.  It uses a std::vector
// as a pool to allocate many semaphores at once.  The pools live permanently,
// but are recycled as semaphores get freed.

// These are arbitrary default sizes for semaphore pools.
constexpr uint32_t kDefaultSemaphorePoolSize = 64;

class SemaphoreHelper;

class DynamicSemaphorePool final : public DynamicallyGrowingPool<std::vector<Semaphore>>
{
  public:
    DynamicSemaphorePool();
    ~DynamicSemaphorePool();

    angle::Result init(Context *context, uint32_t poolSize);
    void destroy(VkDevice device);

    bool isValid() { return mPoolSize > 0; }

    // autoFree can be used to allocate a semaphore that's expected to be freed at the end of the
    // frame.  This renders freeSemaphore unnecessary and saves an eventual search.
    angle::Result allocateSemaphore(Context *context, SemaphoreHelper *semaphoreOut);
    void freeSemaphore(Context *context, SemaphoreHelper *semaphore);

  private:
    angle::Result allocateNewPool(Context *context);
};

// Semaphores that are allocated from the semaphore pool are encapsulated in a helper object,
// keeping track of where in the pool they are allocated from.
class SemaphoreHelper final : angle::NonCopyable
{
  public:
    SemaphoreHelper();
    ~SemaphoreHelper();

    SemaphoreHelper(SemaphoreHelper &&other);
    SemaphoreHelper &operator=(SemaphoreHelper &&other);

    void init(const size_t semaphorePoolIndex, const Semaphore *semaphore);
    void deinit();

    const Semaphore *getSemaphore() const { return mSemaphore; }

    // Used only by DynamicSemaphorePool.
    size_t getSemaphorePoolIndex() const { return mSemaphorePoolIndex; }

  private:
    size_t mSemaphorePoolIndex;
    const Semaphore *mSemaphore;
};

// This class' responsibility is to create index buffers needed to support line loops in Vulkan.
// In the setup phase of drawing, the createIndexBuffer method should be called with the
// current draw call parameters. If an element array buffer is bound for an indexed draw, use
// createIndexBufferFromElementArrayBuffer.
//
// If the user wants to draw a loop between [v1, v2, v3], we will create an indexed buffer with
// these indexes: [0, 1, 2, 3, 0] to emulate the loop.
class LineLoopHelper final : angle::NonCopyable
{
  public:
    LineLoopHelper(RendererVk *renderer);
    ~LineLoopHelper();

    angle::Result getIndexBufferForDrawArrays(ContextVk *contextVk,
                                              uint32_t clampedVertexCount,
                                              GLint firstVertex,
                                              VkBuffer *bufferHandleOut,
                                              VkDeviceSize *offsetOut);

    angle::Result getIndexBufferForElementArrayBuffer(ContextVk *contextVk,
                                                      BufferVk *elementArrayBufferVk,
                                                      GLenum glIndexType,
                                                      int indexCount,
                                                      intptr_t elementArrayOffset,
                                                      VkBuffer *bufferHandleOut,
                                                      VkDeviceSize *bufferOffsetOut);

    angle::Result streamIndices(ContextVk *contextVk,
                                GLenum glIndexType,
                                GLsizei indexCount,
                                const uint8_t *srcPtr,
                                VkBuffer *bufferHandleOut,
                                VkDeviceSize *bufferOffsetOut);

    void destroy(VkDevice device);

    static void Draw(uint32_t count, CommandBuffer *commandBuffer);

  private:
    DynamicBuffer mDynamicIndexBuffer;
};

class FramebufferHelper;

class BufferHelper final : public RecordableGraphResource
{
  public:
    BufferHelper();
    ~BufferHelper();

    angle::Result init(ContextVk *contextVk,
                       const VkBufferCreateInfo &createInfo,
                       VkMemoryPropertyFlags memoryPropertyFlags);
    void release(RendererVk *renderer);

    bool valid() const { return mBuffer.valid(); }
    const Buffer &getBuffer() const { return mBuffer; }
    const DeviceMemory &getDeviceMemory() const { return mDeviceMemory; }

    // Helper for setting the graph dependencies *and* setting the appropriate barrier.
    void onFramebufferRead(FramebufferHelper *framebuffer, VkAccessFlagBits accessType);

    // Also implicitly sets up the correct barriers.
    angle::Result copyFromBuffer(ContextVk *contextVk,
                                 const Buffer &buffer,
                                 const VkBufferCopy &copyRegion);

  private:
    // Vulkan objects.
    Buffer mBuffer;
    DeviceMemory mDeviceMemory;

    // Cached properties.
    VkMemoryPropertyFlags mMemoryPropertyFlags;

    // For memory barriers.
    VkFlags mCurrentWriteAccess;
    VkFlags mCurrentReadAccess;
};

class ImageHelper final : public RecordableGraphResource
{
  public:
    ImageHelper();
    ImageHelper(ImageHelper &&other);
    ~ImageHelper();

    angle::Result init(Context *context,
                       gl::TextureType textureType,
                       const gl::Extents &extents,
                       const Format &format,
                       GLint samples,
                       VkImageUsageFlags usage,
                       uint32_t mipLevels);
    angle::Result initMemory(Context *context,
                             const MemoryProperties &memoryProperties,
                             VkMemoryPropertyFlags flags);
    angle::Result initLayerImageView(Context *context,
                                     gl::TextureType textureType,
                                     VkImageAspectFlags aspectMask,
                                     const gl::SwizzleState &swizzleMap,
                                     ImageView *imageViewOut,
                                     uint32_t levelCount,
                                     uint32_t baseArrayLayer,
                                     uint32_t layerCount);
    angle::Result initImageView(Context *context,
                                gl::TextureType textureType,
                                VkImageAspectFlags aspectMask,
                                const gl::SwizzleState &swizzleMap,
                                ImageView *imageViewOut,
                                uint32_t levelCount);
    angle::Result init2DStaging(Context *context,
                                const MemoryProperties &memoryProperties,
                                const Format &format,
                                const gl::Extents &extent,
                                StagingUsage usage);

    void release(RendererVk *renderer);

    bool valid() const { return mImage.valid(); }

    VkImageAspectFlags getAspectFlags() const;
    void destroy(VkDevice device);
    void dumpResources(Serial serial, std::vector<GarbageObject> *garbageQueue);

    void init2DWeakReference(VkImage handle,
                             const gl::Extents &extents,
                             const Format &format,
                             GLint samples);
    void resetImageWeakReference();

    const Image &getImage() const;
    const DeviceMemory &getDeviceMemory() const;

    const gl::Extents &getExtents() const;
    const Format &getFormat() const;
    GLint getSamples() const;

    VkImageLayout getCurrentLayout() const { return mCurrentLayout; }

    void changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                VkImageLayout newLayout,
                                VkPipelineStageFlags srcStageMask,
                                VkPipelineStageFlags dstStageMask,
                                CommandBuffer *commandBuffer);

    void clearColor(const VkClearColorValue &color,
                    uint32_t baseMipLevel,
                    uint32_t levelCount,
                    CommandBuffer *commandBuffer);

    void clearColorLayer(const VkClearColorValue &color,
                         uint32_t baseMipLevel,
                         uint32_t levelCount,
                         uint32_t baseArrayLayer,
                         uint32_t layerCount,
                         CommandBuffer *commandBuffer);

    void clearDepthStencil(VkImageAspectFlags imageAspectFlags,
                           VkImageAspectFlags clearAspectFlags,
                           const VkClearDepthStencilValue &depthStencil,
                           CommandBuffer *commandBuffer);
    gl::Extents getSize(const gl::ImageIndex &index) const;

    static void Copy(ImageHelper *srcImage,
                     ImageHelper *dstImage,
                     const gl::Offset &srcOffset,
                     const gl::Offset &dstOffset,
                     const gl::Extents &copySize,
                     VkImageAspectFlags aspectMask,
                     CommandBuffer *commandBuffer);

    angle::Result generateMipmapsWithBlit(ContextVk *contextVk, GLuint maxLevel);

  private:
    // Vulkan objects.
    Image mImage;
    DeviceMemory mDeviceMemory;

    // Image properties.
    gl::Extents mExtents;
    const Format *mFormat;
    GLint mSamples;

    // Current state.
    VkImageLayout mCurrentLayout;

    // Cached properties.
    uint32_t mLayerCount;
};

class FramebufferHelper : public RecordableGraphResource
{
  public:
    FramebufferHelper();
    ~FramebufferHelper();

    angle::Result init(ContextVk *contextVk, const VkFramebufferCreateInfo &createInfo);
    void release(RendererVk *renderer);

    bool valid() { return mFramebuffer.valid(); }

    const Framebuffer &getFramebuffer() const
    {
        ASSERT(mFramebuffer.valid());
        return mFramebuffer;
    }

    Framebuffer &getFramebuffer()
    {
        ASSERT(mFramebuffer.valid());
        return mFramebuffer;
    }

  private:
    // Vulkan object.
    Framebuffer mFramebuffer;
};

}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_HELPERS_H_

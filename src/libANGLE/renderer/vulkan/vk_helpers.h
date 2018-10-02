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

// This is an arbitrary max. We can change this later if necessary.
constexpr uint32_t kDefaultDescriptorPoolMaxSets = 2048;

class DynamicDescriptorPool final : angle::NonCopyable
{
  public:
    DynamicDescriptorPool();
    ~DynamicDescriptorPool();

    // The DynamicDescriptorPool only handles one pool size at at time.
    angle::Result init(Context *context, const VkDescriptorPoolSize &poolSize);
    void destroy(VkDevice device);

    // We use the descriptor type to help count the number of free sets.
    // By convention, sets are indexed according to the constants in vk_cache_utils.h.
    angle::Result allocateSets(Context *context,
                               const VkDescriptorSetLayout *descriptorSetLayout,
                               uint32_t descriptorSetCount,
                               VkDescriptorSet *descriptorSetsOut);

    // For testing only!
    void setMaxSetsPerPoolForTesting(uint32_t maxSetsPerPool);

  private:
    angle::Result allocateNewPool(Context *context);

    uint32_t mMaxSetsPerPool;
    uint32_t mCurrentSetsCount;
    DescriptorPool mCurrentDescriptorPool;
    VkDescriptorPoolSize mPoolSize;
    uint32_t mFreeDescriptorSets;
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
                                              const gl::DrawCallParams &drawCallParams,
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

class BufferHelper final : public CommandGraphResource
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

  private:
    // Vulkan objects.
    Buffer mBuffer;
    DeviceMemory mDeviceMemory;

    // Cached properties.
    VkMemoryPropertyFlags mMemoryPropertyFlags;
};

class ImageHelper final : public CommandGraphResource
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
    void updateLayout(VkImageLayout layout) { mCurrentLayout = layout; }

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

    void clearDepthStencil(VkImageAspectFlags aspectFlags,
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

class FramebufferHelper : public CommandGraphResource
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

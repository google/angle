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

    bool valid();

    // This call will allocate a new region at the end of the buffer. It internally may trigger
    // a new buffer to be created (which is returned in 'newBufferAllocatedOut'. This param may
    // be nullptr.
    Error allocate(RendererVk *renderer,
                   size_t sizeInBytes,
                   uint8_t **ptrOut,
                   VkBuffer *handleOut,
                   uint32_t *offsetOut,
                   bool *newBufferAllocatedOut);

    // After a sequence of writes, call flush to ensure the data is visible to the device.
    Error flush(VkDevice device);

    // After a sequence of writes, call invalidate to ensure the data is visible to the host.
    Error invalidate(VkDevice device);

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
    VkBufferUsageFlags mUsage;
    size_t mMinSize;
    Buffer mBuffer;
    DeviceMemory mMemory;
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

class DynamicDescriptorPool final : angle::NonCopyable
{
  public:
    DynamicDescriptorPool();
    ~DynamicDescriptorPool();
    void destroy(RendererVk *rendererVk);
    Error init(const VkDevice &device,
               uint32_t uniformBufferDescriptorsPerSet,
               uint32_t combinedImageSamplerDescriptorsPerSet);

    // It is an undefined behavior to pass a different descriptorSetLayout from call to call.
    Error allocateDescriptorSets(ContextVk *contextVk,
                                 const VkDescriptorSetLayout *descriptorSetLayout,
                                 uint32_t descriptorSetCount,
                                 VkDescriptorSet *descriptorSetsOut);

    // For testing only!
    void setMaxSetsPerPoolForTesting(uint32_t maxSetsPerPool);

  private:
    Error allocateNewPool(const VkDevice &device);

    uint32_t mMaxSetsPerPool;
    DescriptorPool mCurrentDescriptorSetPool;
    size_t mCurrentAllocatedDescriptorSetCount;
    uint32_t mUniformBufferDescriptorsPerSet;
    uint32_t mCombinedImageSamplerDescriptorsPerSet;
};

// This class' responsibility is to create index buffers needed to support line loops in Vulkan.
// In the setup phase of drawing, the createIndexBuffer method should be called with the
// current draw call parameters. If an element array buffer is bound for an indexed draw, use
// createIndexBufferFromElementArrayBuffer.
//
// If the user wants to draw a loop between [v1, v2, v3], we will create an indexed buffer with
// these indexes: [0, 1, 2, 3, 0] to emulate the loop.
class LineLoopHelper final : public vk::CommandGraphResource
{
  public:
    LineLoopHelper(RendererVk *renderer);
    ~LineLoopHelper();

    gl::Error getIndexBufferForDrawArrays(RendererVk *renderer,
                                          const gl::DrawCallParams &drawCallParams,
                                          VkBuffer *bufferHandleOut,
                                          VkDeviceSize *offsetOut);
    gl::Error getIndexBufferForElementArrayBuffer(RendererVk *renderer,
                                                  BufferVk *elementArrayBufferVk,
                                                  VkIndexType indexType,
                                                  int indexCount,
                                                  intptr_t elementArrayOffset,
                                                  VkBuffer *bufferHandleOut,
                                                  VkDeviceSize *bufferOffsetOut);
    gl::Error getIndexBufferForClientElementArray(RendererVk *renderer,
                                                  const gl::DrawCallParams &drawCallParams,
                                                  VkBuffer *bufferHandleOut,
                                                  VkDeviceSize *bufferOffsetOut);

    void destroy(VkDevice device);

    static void Draw(uint32_t count, CommandBuffer *commandBuffer);

  private:
    DynamicBuffer mDynamicIndexBuffer;
};

class ImageHelper final : angle::NonCopyable
{
  public:
    ImageHelper();
    ImageHelper(ImageHelper &&other);
    ~ImageHelper();

    bool valid() const;

    Error init(VkDevice device,
               gl::TextureType textureType,
               const gl::Extents &extents,
               const Format &format,
               GLint samples,
               VkImageUsageFlags usage,
               uint32_t mipLevels);
    Error initMemory(VkDevice device,
                     const MemoryProperties &memoryProperties,
                     VkMemoryPropertyFlags flags);
    Error initImageView(VkDevice device,
                        gl::TextureType textureType,
                        VkImageAspectFlags aspectMask,
                        const gl::SwizzleState &swizzleMap,
                        ImageView *imageViewOut,
                        uint32_t levelCount);
    Error init2DStaging(VkDevice device,
                        const MemoryProperties &memoryProperties,
                        const Format &format,
                        const gl::Extents &extent,
                        StagingUsage usage);

    void release(Serial serial, RendererVk *renderer);
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
    size_t getAllocatedMemorySize() const;

    VkImageLayout getCurrentLayout() const { return mCurrentLayout; }
    void updateLayout(VkImageLayout layout) { mCurrentLayout = layout; }

    void changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                VkImageLayout newLayout,
                                VkPipelineStageFlags srcStageMask,
                                VkPipelineStageFlags dstStageMask,
                                CommandBuffer *commandBuffer);

    void clearColor(const VkClearColorValue &color,
                    uint32_t mipLevel,
                    uint32_t levelCount,
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
    size_t mAllocatedMemorySize;

    // Current state.
    VkImageLayout mCurrentLayout;

    // Cached properties.
    uint32_t mLayerCount;
};
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_HELPERS_H_

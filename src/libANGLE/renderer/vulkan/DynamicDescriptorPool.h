//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DynamicDescriptorPool:
//    Uses DescriptorPool to allocate descriptor sets as needed. If the descriptor pool
//    is full, we simply allocate a new pool to keep allocating descriptor sets as needed and
//    leave the renderer take care of the life time of the pools that become unused.
//

#ifndef LIBANGLE_RENDERER_VULKAN_DYNAMIC_DESCRIPTOR_POOL_H_
#define LIBANGLE_RENDERER_VULKAN_DYNAMIC_DESCRIPTOR_POOL_H_

#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{

enum DescriptorPoolIndex : uint8_t
{
    UniformBufferIndex       = 0,
    TextureIndex             = 1,
    DescriptorPoolIndexCount = 2
};

class DynamicDescriptorPool final : public ResourceVk
{
  public:
    DynamicDescriptorPool();
    ~DynamicDescriptorPool();
    void destroy(RendererVk *rendererVk);
    vk::Error init(const VkDevice &device,
                   uint32_t uniformBufferDescriptorsPerSet,
                   uint32_t combinedImageSamplerDescriptorsPerSet);

    // It is an undefined behavior to pass a different descriptorSetLayout from call to call.
    vk::Error allocateDescriptorSets(ContextVk *contextVk,
                                     const VkDescriptorSetLayout *descriptorSetLayout,
                                     uint32_t descriptorSetCount,
                                     VkDescriptorSet *descriptorSetsOut);

  private:
    vk::Error allocateNewPool(const VkDevice &device);

    vk::DescriptorPool mCurrentDescriptorSetPool;
    size_t mCurrentAllocatedDescriptorSetCount;
    uint32_t mUniformBufferDescriptorsPerSet;
    uint32_t mCombinedImageSamplerDescriptorsPerSet;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_DYNAMIC_DESCRIPTOR_POOL_H_

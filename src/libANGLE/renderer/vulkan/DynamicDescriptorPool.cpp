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

#include "DynamicDescriptorPool.h"

#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"

namespace rx
{
namespace
{

// TODO(jmadill): Pick non-arbitrary max.
constexpr uint32_t kMaxSets = 2048;
}  // anonymous namespace

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
    rendererVk->releaseResource(*this, &mCurrentDescriptorSetPool);
}

vk::Error DynamicDescriptorPool::init(const VkDevice &device,
                                      uint32_t uniformBufferDescriptorsPerSet,
                                      uint32_t combinedImageSamplerDescriptorsPerSet)
{
    ASSERT(!mCurrentDescriptorSetPool.valid() && mCurrentAllocatedDescriptorSetCount == 0);

    mUniformBufferDescriptorsPerSet        = uniformBufferDescriptorsPerSet;
    mCombinedImageSamplerDescriptorsPerSet = combinedImageSamplerDescriptorsPerSet;

    ANGLE_TRY(allocateNewPool(device));
    return vk::NoError();
}

vk::Error DynamicDescriptorPool::allocateDescriptorSets(
    ContextVk *contextVk,
    const VkDescriptorSetLayout *descriptorSetLayout,
    uint32_t descriptorSetCount,
    VkDescriptorSet *descriptorSetsOut)
{
    updateQueueSerial(contextVk->getRenderer()->getCurrentQueueSerial());

    if (descriptorSetCount + mCurrentAllocatedDescriptorSetCount > kMaxSets)
    {
        // We will bust the limit of descriptor set with this allocation so we need to get a new
        // pool for it.
        contextVk->getRenderer()->releaseResource(*this, &mCurrentDescriptorSetPool);
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
    return vk::NoError();
}

vk::Error DynamicDescriptorPool::allocateNewPool(const VkDevice &device)
{
    VkDescriptorPoolSize poolSizes[DescriptorPoolIndexCount];
    poolSizes[UniformBufferIndex].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[UniformBufferIndex].descriptorCount =
        mUniformBufferDescriptorsPerSet * kMaxSets / DescriptorPoolIndexCount;
    poolSizes[TextureIndex].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[TextureIndex].descriptorCount =
        mCombinedImageSamplerDescriptorsPerSet * kMaxSets / DescriptorPoolIndexCount;

    VkDescriptorPoolCreateInfo descriptorPoolInfo;
    descriptorPoolInfo.sType   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext   = nullptr;
    descriptorPoolInfo.flags   = 0;
    descriptorPoolInfo.maxSets = kMaxSets;

    // Reserve pools for uniform blocks and textures.
    descriptorPoolInfo.poolSizeCount = DescriptorPoolIndexCount;
    descriptorPoolInfo.pPoolSizes    = poolSizes;

    mCurrentAllocatedDescriptorSetCount = 0;
    ANGLE_TRY(mCurrentDescriptorSetPool.init(device, descriptorPoolInfo));
    return vk::NoError();
}
}  // namespace rx

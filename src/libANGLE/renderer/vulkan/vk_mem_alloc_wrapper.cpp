//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vma_allocator_wrapper.cpp:
//    Hides VMA functions so we can use separate warning sets.
//

#include "vk_mem_alloc_wrapper.h"

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

namespace vma
{
VkResult CreateBuffer(VmaAllocator allocator,
                      const VkBufferCreateInfo *pBufferCreateInfo,
                      VkMemoryPropertyFlags requiredFlags,
                      VkMemoryPropertyFlags preferredFlags,
                      VkBuffer *pBuffer,
                      VmaAllocation *pAllocation)
{
    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.requiredFlags           = requiredFlags;
    allocationCreateInfo.preferredFlags          = preferredFlags;
    VmaAllocationInfo allocationInfo             = {};
    return vmaCreateBuffer(allocator, pBufferCreateInfo, &allocationCreateInfo, pBuffer,
                           pAllocation, &allocationInfo);
}

void DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation)
{
    vmaDestroyBuffer(allocator, buffer, allocation);
}

void GetMemoryProperties(VmaAllocator allocator,
                         const VkPhysicalDeviceMemoryProperties **ppPhysicalDeviceMemoryProperties)
{
    return vmaGetMemoryProperties(allocator, ppPhysicalDeviceMemoryProperties);
}

void GetMemoryTypeProperties(VmaAllocator allocator,
                             uint32_t memoryTypeIndex,
                             VkMemoryPropertyFlags *pFlags)
{
    vmaGetMemoryTypeProperties(allocator, memoryTypeIndex, pFlags);
}

VkResult MapMemory(VmaAllocator allocator, VmaAllocation allocation, void **ppData)
{
    return vmaMapMemory(allocator, allocation, ppData);
}

void UnmapMemory(VmaAllocator allocator, VmaAllocation allocation)
{
    return vmaUnmapMemory(allocator, allocation);
}

void FlushAllocation(VmaAllocator allocator,
                     VmaAllocation allocation,
                     VkDeviceSize offset,
                     VkDeviceSize size)
{
    vmaFlushAllocation(allocator, allocation, offset, size);
}

void InvalidateAllocation(VmaAllocator allocator,
                          VmaAllocation allocation,
                          VkDeviceSize offset,
                          VkDeviceSize size)
{
    vmaInvalidateAllocation(allocator, allocation, offset, size);
}
}  // namespace vma

//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vma_allocator_wrapper.h:
//    Hides VMA functions so we can use separate warning sets.
//

#ifndef LIBANGLE_RENDERER_VULKAN_VK_MEM_ALLOC_WRAPPER_H_
#define LIBANGLE_RENDERER_VULKAN_VK_MEM_ALLOC_WRAPPER_H_

#include <volk.h>

VK_DEFINE_HANDLE(VmaAllocator)
VK_DEFINE_HANDLE(VmaPool)
VK_DEFINE_HANDLE(VmaAllocation)
VK_DEFINE_HANDLE(VmaDefragmentationContext)

namespace vma
{

// Please add additional functions or parameters here as needed.

VkResult CreateBuffer(VmaAllocator allocator,
                      const VkBufferCreateInfo *pBufferCreateInfo,
                      VkMemoryPropertyFlags requiredFlags,
                      VkMemoryPropertyFlags preferredFlags,
                      VkBuffer *pBuffer,
                      VmaAllocation *pAllocation);

void DestroyBuffer(VmaAllocator allocator, VkBuffer buffer, VmaAllocation allocation);

void GetMemoryProperties(VmaAllocator allocator,
                         const VkPhysicalDeviceMemoryProperties **ppPhysicalDeviceMemoryProperties);

void GetMemoryTypeProperties(VmaAllocator allocator,
                             uint32_t memoryTypeIndex,
                             VkMemoryPropertyFlags *pFlags);

VkResult MapMemory(VmaAllocator allocator, VmaAllocation allocation, void **ppData);

void UnmapMemory(VmaAllocator allocator, VmaAllocation allocation);

void FlushAllocation(VmaAllocator allocator,
                     VmaAllocation allocation,
                     VkDeviceSize offset,
                     VkDeviceSize size);

void InvalidateAllocation(VmaAllocator allocator,
                          VmaAllocation allocation,
                          VkDeviceSize offset,
                          VkDeviceSize size);

}  // namespace vma

#endif  // LIBANGLE_RENDERER_VULKAN_VK_MEM_ALLOC_WRAPPER_H_
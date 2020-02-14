//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramExecutableVk.h: Collects the information and interfaces common to both ProgramVks and
// ProgramPipelineVks in order to execute/draw with either.

#ifndef LIBANGLE_RENDERER_VULKAN_PROGRAMEXECUTABLEVK_H_
#define LIBANGLE_RENDERER_VULKAN_PROGRAMEXECUTABLEVK_H_

#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/InfoLog.h"
#include "libANGLE/renderer/glslang_wrapper_utils.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{

// State for the default uniform blocks.
struct DefaultUniformBlock final : private angle::NonCopyable
{
    DefaultUniformBlock();
    ~DefaultUniformBlock();

    vk::DynamicBuffer storage;

    // Shadow copies of the shader uniform data.
    angle::MemoryBuffer uniformData;

    // Since the default blocks are laid out in std140, this tells us where to write on a call
    // to a setUniform method. They are arranged in uniform location order.
    std::vector<sh::BlockMemberInfo> uniformLayout;
};

class ProgramExecutableVk
{
  public:
    ProgramExecutableVk();
    virtual ~ProgramExecutableVk();

    void reset(ContextVk *contextVk);

    void clearVariableInfoMap();
    ShaderInterfaceVariableInfoMap &getShaderInterfaceVariableInfoMap() { return mVariableInfoMap; }

    angle::Result createPipelineLayout(const gl::Context *glContext,
                                       const gl::ProgramExecutable &glExecutable,
                                       const gl::ProgramState &programState);

    angle::Result updateTexturesDescriptorSet(const gl::ProgramState &programState,
                                              ContextVk *contextVk);
    angle::Result updateShaderResourcesDescriptorSet(const gl::ProgramState &programState,
                                                     ContextVk *contextVk,
                                                     vk::ResourceUseList *resourceUseList,
                                                     CommandBufferHelper *commandBufferHelper);
    angle::Result updateTransformFeedbackDescriptorSet(
        const gl::ProgramState &programState,
        gl::ShaderMap<DefaultUniformBlock> &defaultUniformBlocks,
        ContextVk *contextVk);

    angle::Result updateDescriptorSets(ContextVk *contextVk, vk::CommandBuffer *commandBuffer);

  private:
    friend class ProgramVk;
    friend class ProgramPipelineVk;

    void updateBindingOffsets(const gl::ProgramState &programState);
    uint32_t getUniformBlockBindingsOffset() const { return 0; }
    uint32_t getStorageBlockBindingsOffset() const { return mStorageBlockBindingsOffset; }
    uint32_t getAtomicCounterBufferBindingsOffset() const
    {
        return mAtomicCounterBufferBindingsOffset;
    }
    uint32_t getImageBindingsOffset() const { return mImageBindingsOffset; }

    angle::Result allocateDescriptorSet(ContextVk *contextVk, uint32_t descriptorSetIndex);
    angle::Result allocateDescriptorSetAndGetInfo(ContextVk *contextVk,
                                                  uint32_t descriptorSetIndex,
                                                  bool *newPoolAllocatedOut);

    void updateDefaultUniformsDescriptorSet(
        const gl::ProgramState &programState,
        gl::ShaderMap<DefaultUniformBlock> &defaultUniformBlocks,
        ContextVk *contextVk);
    void updateTransformFeedbackDescriptorSetImpl(const gl::ProgramState &programState,
                                                  ContextVk *contextVk);
    void updateBuffersDescriptorSet(ContextVk *contextVk,
                                    vk::ResourceUseList *resourceUseList,
                                    CommandBufferHelper *commandBufferHelper,
                                    const std::vector<gl::InterfaceBlock> &blocks,
                                    VkDescriptorType descriptorType);
    void updateAtomicCounterBuffersDescriptorSet(const gl::ProgramState &programState,
                                                 ContextVk *contextVk,
                                                 vk::ResourceUseList *resourceUseList,
                                                 CommandBufferHelper *commandBufferHelper);
    angle::Result updateImagesDescriptorSet(const gl::ProgramState &programState,
                                            ContextVk *contextVk);

    // In their descriptor set, uniform buffers are placed first, then storage buffers, then atomic
    // counter buffers and then images.  These cached values contain the offsets where storage
    // buffer, atomic counter buffer and image bindings start.
    uint32_t mStorageBlockBindingsOffset;
    uint32_t mAtomicCounterBufferBindingsOffset;
    uint32_t mImageBindingsOffset;

    // This is a special "empty" placeholder buffer for when a shader has no uniforms or doesn't
    // use all slots in the atomic counter buffer array.
    //
    // It is necessary because we want to keep a compatible pipeline layout in all cases,
    // and Vulkan does not tolerate having null handles in a descriptor set.
    vk::BufferHelper mEmptyBuffer;

    // Descriptor sets for uniform blocks and textures for this program.
    std::vector<VkDescriptorSet> mDescriptorSets;
    vk::DescriptorSetLayoutArray<VkDescriptorSet> mEmptyDescriptorSets;
    std::vector<vk::BufferHelper *> mDescriptorBuffersCache;

    std::unordered_map<vk::TextureDescriptorDesc, VkDescriptorSet> mTextureDescriptorsCache;

    // We keep a reference to the pipeline and descriptor set layouts. This ensures they don't get
    // deleted while this program is in use.
    vk::BindingPointer<vk::PipelineLayout> mPipelineLayout;
    vk::DescriptorSetLayoutPointerArray mDescriptorSetLayouts;

    // Keep bindings to the descriptor pools. This ensures the pools stay valid while the Program
    // is in use.
    vk::DescriptorSetLayoutArray<vk::RefCountedDescriptorPoolBinding> mDescriptorPoolBindings;

    // Store descriptor pools here. We store the descriptors in the Program to facilitate descriptor
    // cache management. It can also allow fewer descriptors for shaders which use fewer
    // textures/buffers.
    vk::DescriptorSetLayoutArray<vk::DynamicDescriptorPool> mDynamicDescriptorPools;

    gl::ShaderVector<uint32_t> mDynamicBufferOffsets;

    ShaderInterfaceVariableInfoMap mVariableInfoMap;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_PROGRAMEXECUTABLEVK_H_

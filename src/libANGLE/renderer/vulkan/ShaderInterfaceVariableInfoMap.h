//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderInterfaceVariableInfoMap: Maps shader interface variable SPIR-V ids to their Vulkan
// mapping.

#ifndef LIBANGLE_RENDERER_VULKAN_SHADERINTERFACEVARIABLEINFOMAP_H_
#define LIBANGLE_RENDERER_VULKAN_SHADERINTERFACEVARIABLEINFOMAP_H_

#include "common/FastVector.h"
#include "libANGLE/renderer/ProgramImpl.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/spv_utils.h"

#include <functional>

#include <stdio.h>

namespace rx
{

enum class ShaderVariableType
{
    AtomicCounter,
    Attribute,
    DefaultUniform,
    DriverUniform,
    FramebufferFetch,
    Image,
    Output,
    SecondaryOutput,
    ShaderStorageBuffer,
    Texture,
    TransformFeedback,
    UniformBuffer,
    Varying,
    EnumCount,
};

struct TypeAndIndex
{
    ShaderVariableType variableType;
    uint32_t index;
};

class ShaderInterfaceVariableInfoMap final : angle::NonCopyable
{
  public:
    using VariableInfoArray     = std::vector<ShaderInterfaceVariableInfo>;
    using VariableTypeToInfoMap = angle::PackedEnumMap<ShaderVariableType, VariableInfoArray>;
    using IdToTypeAndIndexMap   = angle::HashMap<uint32_t, TypeAndIndex>;

    static constexpr size_t kResourceFastMapMax = 32;
    using ResourceIndexMap                      = angle::FastMap<uint32_t, kResourceFastMapMax>;
    using VariableTypeToIndexMap = angle::PackedEnumMap<ShaderVariableType, ResourceIndexMap>;

    ShaderInterfaceVariableInfoMap();
    ~ShaderInterfaceVariableInfoMap();

    void clear();
    void load(VariableTypeToInfoMap &&data,
              gl::ShaderMap<IdToTypeAndIndexMap> &&idToTypeAndIndexMap,
              VariableTypeToIndexMap &&indexedResourceIndexMap,
              gl::ShaderMap<gl::PerVertexMemberBitSet> &&inputPerVertexActiveMembers,
              gl::ShaderMap<gl::PerVertexMemberBitSet> &&outputPerVertexActiveMembers);

    ShaderInterfaceVariableInfo &add(gl::ShaderType shaderType,
                                     ShaderVariableType variableType,
                                     uint32_t id);
    void addIndexedResource(gl::ShaderBitSet shaderTypes,
                            ShaderVariableType variableType,
                            const gl::ShaderMap<uint32_t> &idInShaderTypes,
                            uint32_t descriptorSet,
                            uint32_t binding,
                            uint32_t resourceIndex);
    ShaderInterfaceVariableInfo &addOrGet(gl::ShaderType shaderType,
                                          ShaderVariableType variableType,
                                          uint32_t id);

    void setInputPerVertexActiveMembers(gl::ShaderType shaderType,
                                        gl::PerVertexMemberBitSet activeMembers);
    void setOutputPerVertexActiveMembers(gl::ShaderType shaderType,
                                         gl::PerVertexMemberBitSet activeMembers);
    ShaderInterfaceVariableInfo &getMutable(gl::ShaderType shaderType,
                                            ShaderVariableType variableType,
                                            uint32_t id);

    const ShaderInterfaceVariableInfo &getDefaultUniformInfo(gl::ShaderType shaderType) const;
    const ShaderInterfaceVariableInfo &getIndexedVariableInfo(ShaderVariableType variableType,
                                                              uint32_t variableIndex) const;
    bool hasAtomicCounterInfo() const;
    const ShaderInterfaceVariableInfo &getAtomicCounterInfo() const;
    const ShaderInterfaceVariableInfo &getFramebufferFetchInfo() const;
    bool hasTransformFeedbackInfo(gl::ShaderType shaderType, uint32_t bufferIndex) const;
    const ShaderInterfaceVariableInfo &getTransformFeedbackInfo(uint32_t bufferIndex) const;

    uint32_t getDefaultUniformBinding(gl::ShaderType shaderType) const;
    uint32_t getXfbBufferBinding(uint32_t xfbBufferIndex) const;
    uint32_t getAtomicCounterBufferBinding(uint32_t atomicCounterBufferIndex) const;

    bool hasVariable(gl::ShaderType shaderType, uint32_t id) const;
    const ShaderInterfaceVariableInfo &getVariableById(gl::ShaderType shaderType,
                                                       uint32_t id) const;
    void mapIndexedResourceToInfoOfElementZero(gl::ShaderBitSet shaderTypes,
                                               ShaderVariableType variableType,
                                               const gl::ShaderMap<uint32_t> &idInShaderTypes,
                                               uint32_t resourceIndex);
    const VariableInfoArray &getAttributes() const;
    const VariableTypeToInfoMap &getData() const { return mData; }
    const gl::ShaderMap<IdToTypeAndIndexMap> &getIdToTypeAndIndexMap() const
    {
        return mIdToTypeAndIndexMap;
    }
    const VariableTypeToIndexMap &getIndexedResourceMap() const { return mIndexedResourceIndexMap; }
    const gl::ShaderMap<gl::PerVertexMemberBitSet> &getInputPerVertexActiveMembers() const
    {
        return mInputPerVertexActiveMembers;
    }
    const gl::ShaderMap<gl::PerVertexMemberBitSet> &getOutputPerVertexActiveMembers() const
    {
        return mOutputPerVertexActiveMembers;
    }

  private:
    VariableTypeToInfoMap mData;
    gl::ShaderMap<IdToTypeAndIndexMap> mIdToTypeAndIndexMap;
    VariableTypeToIndexMap mIndexedResourceIndexMap;

    // Active members of `in gl_PerVertex` and `out gl_PerVertex`
    gl::ShaderMap<gl::PerVertexMemberBitSet> mInputPerVertexActiveMembers;
    gl::ShaderMap<gl::PerVertexMemberBitSet> mOutputPerVertexActiveMembers;
};

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getDefaultUniformInfo(gl::ShaderType shaderType) const
{
    return getVariableById(shaderType, sh::vk::spirv::kIdDefaultUniformsBlock);
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getIndexedVariableInfo(ShaderVariableType variableType,
                                                       uint32_t resourceIndex) const
{
    ASSERT(resourceIndex < mIndexedResourceIndexMap[variableType].size());
    uint32_t variableIndex = mIndexedResourceIndexMap[variableType][resourceIndex];
    ASSERT(variableIndex < mData[variableType].size());
    return mData[variableType][variableIndex];
}

ANGLE_INLINE bool ShaderInterfaceVariableInfoMap::hasAtomicCounterInfo() const
{
    return !mData[ShaderVariableType::AtomicCounter].empty();
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getAtomicCounterInfo() const
{
    ASSERT(mData[ShaderVariableType::AtomicCounter].size() == 1);
    return mData[ShaderVariableType::AtomicCounter][0];
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getFramebufferFetchInfo() const
{
    ASSERT(!mData[ShaderVariableType::FramebufferFetch].empty());
    return mData[ShaderVariableType::FramebufferFetch][0];
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getTransformFeedbackInfo(uint32_t bufferIndex) const
{
    ASSERT(bufferIndex < mData[ShaderVariableType::TransformFeedback].size());
    return mData[ShaderVariableType::TransformFeedback][bufferIndex];
}

ANGLE_INLINE uint32_t
ShaderInterfaceVariableInfoMap::getDefaultUniformBinding(gl::ShaderType shaderType) const
{
    return getDefaultUniformInfo(shaderType).binding;
}

ANGLE_INLINE uint32_t
ShaderInterfaceVariableInfoMap::getXfbBufferBinding(uint32_t xfbBufferIndex) const
{
    return getTransformFeedbackInfo(xfbBufferIndex).binding;
}

ANGLE_INLINE uint32_t ShaderInterfaceVariableInfoMap::getAtomicCounterBufferBinding(
    uint32_t atomicCounterBufferIndex) const
{
    ASSERT(hasAtomicCounterInfo());
    return getAtomicCounterInfo().binding + atomicCounterBufferIndex;
}
}  // namespace rx
#endif  // LIBANGLE_RENDERER_VULKAN_SHADERINTERFACEVARIABLEINFOMAP_H_

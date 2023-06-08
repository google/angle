//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderInterfaceVariableInfoMap: Maps shader interface variable SPIR-V ids to their Vulkan
// mapping.
//

#include "libANGLE/renderer/vulkan/ShaderInterfaceVariableInfoMap.h"

namespace rx
{
ShaderInterfaceVariableInfo::ShaderInterfaceVariableInfo() {}

// ShaderInterfaceVariableInfoMap implementation.
ShaderInterfaceVariableInfoMap::ShaderInterfaceVariableInfoMap() = default;

ShaderInterfaceVariableInfoMap::~ShaderInterfaceVariableInfoMap() = default;

void ShaderInterfaceVariableInfoMap::clear()
{
    for (gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        for (ShaderVariableType variableType : angle::AllEnums<ShaderVariableType>())
        {
            mData[shaderType][variableType].clear();
            mIndexedResourceIndexMap[shaderType][variableType].clear();
        }
        mIdToTypeAndIndexMap[shaderType].clear();
    }
    std::fill(mInputPerVertexActiveMembers.begin(), mInputPerVertexActiveMembers.end(),
              gl::PerVertexMemberBitSet{});
    std::fill(mOutputPerVertexActiveMembers.begin(), mOutputPerVertexActiveMembers.end(),
              gl::PerVertexMemberBitSet{});
}

void ShaderInterfaceVariableInfoMap::load(
    gl::ShaderMap<VariableTypeToInfoMap> &&data,
    gl::ShaderMap<IdToTypeAndIndexMap> &&idToTypeAndIndexMap,
    gl::ShaderMap<VariableTypeToIndexMap> &&indexedResourceIndexMap,
    gl::ShaderMap<gl::PerVertexMemberBitSet> &&inputPerVertexActiveMembers,
    gl::ShaderMap<gl::PerVertexMemberBitSet> &&outputPerVertexActiveMembers)
{
    mData.swap(data);
    mIdToTypeAndIndexMap.swap(idToTypeAndIndexMap);
    mIndexedResourceIndexMap.swap(indexedResourceIndexMap);
    mInputPerVertexActiveMembers.swap(inputPerVertexActiveMembers);
    mOutputPerVertexActiveMembers.swap(outputPerVertexActiveMembers);
}

void ShaderInterfaceVariableInfoMap::setActiveStages(gl::ShaderType shaderType,
                                                     ShaderVariableType variableType,
                                                     uint32_t id,
                                                     gl::ShaderBitSet activeStages)
{
    ASSERT(hasVariable(shaderType, id));
    uint32_t index = mIdToTypeAndIndexMap[shaderType][id].index;
    mData[shaderType][variableType][index].activeStages = activeStages;
}

void ShaderInterfaceVariableInfoMap::setInputPerVertexActiveMembers(
    gl::ShaderType shaderType,
    gl::PerVertexMemberBitSet activeMembers)
{
    // Input gl_PerVertex is only meaningful for tessellation and geometry stages
    ASSERT(shaderType == gl::ShaderType::TessControl ||
           shaderType == gl::ShaderType::TessEvaluation || shaderType == gl::ShaderType::Geometry ||
           activeMembers.none());
    mInputPerVertexActiveMembers[shaderType] = activeMembers;
}

void ShaderInterfaceVariableInfoMap::setOutputPerVertexActiveMembers(
    gl::ShaderType shaderType,
    gl::PerVertexMemberBitSet activeMembers)
{
    // Output gl_PerVertex is only meaningful for vertex, tessellation and geometry stages
    ASSERT(shaderType == gl::ShaderType::Vertex || shaderType == gl::ShaderType::TessControl ||
           shaderType == gl::ShaderType::TessEvaluation || shaderType == gl::ShaderType::Geometry ||
           activeMembers.none());
    mOutputPerVertexActiveMembers[shaderType] = activeMembers;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getMutable(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    uint32_t id)
{
    ASSERT(hasVariable(shaderType, id));
    uint32_t index = mIdToTypeAndIndexMap[shaderType][id].index;
    return mData[shaderType][variableType][index];
}

void ShaderInterfaceVariableInfoMap::markAsDuplicate(gl::ShaderType shaderType,
                                                     ShaderVariableType variableType,
                                                     uint32_t id)
{
    ASSERT(hasVariable(shaderType, id));
    uint32_t index                                     = mIdToTypeAndIndexMap[shaderType][id].index;
    mData[shaderType][variableType][index].isDuplicate = true;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::add(gl::ShaderType shaderType,
                                                                 ShaderVariableType variableType,
                                                                 uint32_t id)
{
    ASSERT(!hasVariable(shaderType, id));
    uint32_t index = static_cast<uint32_t>(mData[shaderType][variableType].size());
    mIdToTypeAndIndexMap[shaderType][id] = {variableType, index};
    mData[shaderType][variableType].resize(index + 1);
    return mData[shaderType][variableType][index];
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::addOrGet(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    uint32_t id)
{
    if (!hasVariable(shaderType, id))
    {
        return add(shaderType, variableType, id);
    }
    else
    {
        uint32_t index = mIdToTypeAndIndexMap[shaderType][id].index;
        return mData[shaderType][variableType][index];
    }
}

bool ShaderInterfaceVariableInfoMap::hasVariable(gl::ShaderType shaderType, uint32_t id) const
{
    ASSERT(id >= sh::vk::spirv::kIdShaderVariablesBegin);
    auto iter = mIdToTypeAndIndexMap[shaderType].find(id);
    return (iter != mIdToTypeAndIndexMap[shaderType].end());
}

const ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getVariableById(
    gl::ShaderType shaderType,
    uint32_t id) const
{
    ASSERT(id >= sh::vk::spirv::kIdShaderVariablesBegin);
    auto iter = mIdToTypeAndIndexMap[shaderType].find(id);
    ASSERT(iter != mIdToTypeAndIndexMap[shaderType].end());
    TypeAndIndex typeAndIndex = iter->second;
    return mData[shaderType][typeAndIndex.variableType][typeAndIndex.index];
}

bool ShaderInterfaceVariableInfoMap::hasTransformFeedbackInfo(gl::ShaderType shaderType,
                                                              uint32_t bufferIndex) const
{
    return hasVariable(shaderType, SpvGetXfbBufferBlockId(bufferIndex));
}

void ShaderInterfaceVariableInfoMap::mapIndexedResourceById(gl::ShaderType shaderType,
                                                            ShaderVariableType variableType,
                                                            uint32_t resourceIndex,
                                                            uint32_t id)
{
    ASSERT(hasVariable(shaderType, id));
    const auto &iter                 = mIdToTypeAndIndexMap[shaderType].find(id);
    const TypeAndIndex &typeAndIndex = iter->second;
    ASSERT(typeAndIndex.variableType == variableType);
    mapIndexedResource(shaderType, variableType, resourceIndex, typeAndIndex.index);
}

void ShaderInterfaceVariableInfoMap::mapIndexedResource(gl::ShaderType shaderType,
                                                        ShaderVariableType variableType,
                                                        uint32_t resourceIndex,
                                                        uint32_t variableIndex)
{
    mIndexedResourceIndexMap[shaderType][variableType][resourceIndex] = variableIndex;
}

const ShaderInterfaceVariableInfoMap::VariableInfoArray &
ShaderInterfaceVariableInfoMap::getAttributes() const
{
    return mData[gl::ShaderType::Vertex][ShaderVariableType::Attribute];
}
}  // namespace rx

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
namespace
{
uint32_t HashSPIRVId(uint32_t id)
{
    ASSERT(id >= sh::vk::spirv::kIdShaderVariablesBegin);
    return id - sh::vk::spirv::kIdShaderVariablesBegin;
}
}  // anonymous namespace

ShaderInterfaceVariableInfo::ShaderInterfaceVariableInfo() {}

// ShaderInterfaceVariableInfoMap implementation.
ShaderInterfaceVariableInfoMap::ShaderInterfaceVariableInfoMap() = default;

ShaderInterfaceVariableInfoMap::~ShaderInterfaceVariableInfoMap() = default;

void ShaderInterfaceVariableInfoMap::clear()
{
    for (ShaderVariableType variableType : angle::AllEnums<ShaderVariableType>())
    {
        mData[variableType].clear();
        mIndexedResourceIndexMap[variableType].clear();
    }
    for (gl::ShaderType shaderType : gl::AllShaderTypes())
    {
        mIdToTypeAndIndexMap[shaderType].clear();
    }
    std::fill(mInputPerVertexActiveMembers.begin(), mInputPerVertexActiveMembers.end(),
              gl::PerVertexMemberBitSet{});
    std::fill(mOutputPerVertexActiveMembers.begin(), mOutputPerVertexActiveMembers.end(),
              gl::PerVertexMemberBitSet{});
}

void ShaderInterfaceVariableInfoMap::load(
    VariableTypeToInfoMap &&data,
    gl::ShaderMap<IdToTypeAndIndexMap> &&idToTypeAndIndexMap,
    VariableTypeToIndexMap &&indexedResourceIndexMap,
    gl::ShaderMap<gl::PerVertexMemberBitSet> &&inputPerVertexActiveMembers,
    gl::ShaderMap<gl::PerVertexMemberBitSet> &&outputPerVertexActiveMembers)
{
    mData.swap(data);
    mIdToTypeAndIndexMap.swap(idToTypeAndIndexMap);
    mIndexedResourceIndexMap.swap(indexedResourceIndexMap);
    mInputPerVertexActiveMembers.swap(inputPerVertexActiveMembers);
    mOutputPerVertexActiveMembers.swap(outputPerVertexActiveMembers);
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

bool ShaderInterfaceVariableInfoMap::hasTypeAndIndexById(gl::ShaderType shaderType,
                                                         uint32_t id) const
{
    const uint32_t hashedId = HashSPIRVId(id);
    return hashedId < mIdToTypeAndIndexMap[shaderType].size() &&
           mIdToTypeAndIndexMap[shaderType].at(hashedId).variableType !=
               ShaderVariableType::InvalidEnum;
}

void ShaderInterfaceVariableInfoMap::addTypeAndIndexById(gl::ShaderType shaderType,
                                                         uint32_t id,
                                                         TypeAndIndex typeAndIndex)
{
    mIdToTypeAndIndexMap[shaderType][HashSPIRVId(id)] = typeAndIndex;
}

const TypeAndIndex &ShaderInterfaceVariableInfoMap::getTypeAndIndexById(gl::ShaderType shaderType,
                                                                        uint32_t id) const
{
    return mIdToTypeAndIndexMap[shaderType].at(HashSPIRVId(id));
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getMutable(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    uint32_t id)
{
    ASSERT(hasVariable(shaderType, id));
    uint32_t index = getTypeAndIndexById(shaderType, id).index;
    return mData[variableType][index];
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::add(gl::ShaderType shaderType,
                                                                 ShaderVariableType variableType,
                                                                 uint32_t id)
{
    ASSERT(!hasVariable(shaderType, id));
    uint32_t index = static_cast<uint32_t>(mData[variableType].size());
    addTypeAndIndexById(shaderType, id, {variableType, index});
    mData[variableType].resize(index + 1);
    return mData[variableType][index];
}

void ShaderInterfaceVariableInfoMap::addIndexedResource(
    gl::ShaderBitSet shaderTypes,
    ShaderVariableType variableType,
    const gl::ShaderMap<uint32_t> &idInShaderTypes,
    uint32_t descriptorSet,
    uint32_t binding,
    uint32_t resourceIndex)
{
    uint32_t index = static_cast<uint32_t>(mData[variableType].size());
    mData[variableType].resize(index + 1);
    ShaderInterfaceVariableInfo *info = &mData[variableType][index];

    info->descriptorSet = descriptorSet;
    info->binding       = binding;
    info->activeStages  = shaderTypes;

    mIndexedResourceIndexMap[variableType][resourceIndex] = index;

    for (const gl::ShaderType shaderType : shaderTypes)
    {
        const uint32_t id = idInShaderTypes[shaderType];
        ASSERT(!hasVariable(shaderType, id));
        addTypeAndIndexById(shaderType, id, {variableType, index});
    }
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
        uint32_t index = getTypeAndIndexById(shaderType, id).index;
        return mData[variableType][index];
    }
}

bool ShaderInterfaceVariableInfoMap::hasVariable(gl::ShaderType shaderType, uint32_t id) const
{
    return hasTypeAndIndexById(shaderType, id);
}

const ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getVariableById(
    gl::ShaderType shaderType,
    uint32_t id) const
{
    TypeAndIndex typeAndIndex = getTypeAndIndexById(shaderType, id);
    return mData[typeAndIndex.variableType][typeAndIndex.index];
}

void ShaderInterfaceVariableInfoMap::mapIndexedResourceToInfoOfElementZero(
    gl::ShaderBitSet shaderTypes,
    ShaderVariableType variableType,
    const gl::ShaderMap<uint32_t> &idInShaderTypes,
    uint32_t resourceIndex)
{
    // Called only for non-index-zero array elements, associate resourceIndex with the info of
    // element 0.  This is technically not needed if the rest of the code processes array elements
    // in order and carries over the info from element 0, but is here for convenience.
    for (const gl::ShaderType shaderType : shaderTypes)
    {
        const uint32_t id = idInShaderTypes[shaderType];

        ASSERT(hasVariable(shaderType, id));

        // Get the index of the info previously associated with element 0.
        const TypeAndIndex &typeAndIndex = getTypeAndIndexById(shaderType, id);
        ASSERT(typeAndIndex.variableType == variableType);
        // Map this resource to the same index.
        mIndexedResourceIndexMap[variableType][resourceIndex] = typeAndIndex.index;
    }
}

bool ShaderInterfaceVariableInfoMap::hasTransformFeedbackInfo(gl::ShaderType shaderType,
                                                              uint32_t bufferIndex) const
{
    return hasVariable(shaderType, SpvGetXfbBufferBlockId(bufferIndex));
}

const ShaderInterfaceVariableInfoMap::VariableInfoArray &
ShaderInterfaceVariableInfoMap::getAttributes() const
{
    return mData[ShaderVariableType::Attribute];
}
}  // namespace rx

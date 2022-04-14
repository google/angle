//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderInterfaceVariableInfoMap.cpp:
//    Implements helper class for shader compilers
//

#include "libANGLE/renderer/ShaderInterfaceVariableInfoMap.h"
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
        for (VariableNameToInfoMap &typeMap : mData[shaderType])
        {
            typeMap.clear();
        }
        mNameToTypeMap[shaderType].clear();
    }
}

const ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::get(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    const std::string &variableName) const
{
    auto it = mData[shaderType][variableType].find(variableName);
    ASSERT(it != mData[shaderType][variableType].end());
    return it->second;
}

void ShaderInterfaceVariableInfoMap::setActiveStages(gl::ShaderType shaderType,
                                                     ShaderVariableType variableType,
                                                     const std::string &variableName,
                                                     gl::ShaderBitSet activeStages)
{
    auto it = mData[shaderType][variableType].find(variableName);
    ASSERT(it != mData[shaderType][variableType].end());
    it->second.activeStages = activeStages;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getMutable(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    const std::string &variableName)
{
    auto it = mData[shaderType][variableType].find(variableName);
    ASSERT(it != mData[shaderType][variableType].end());
    return it->second;
}

void ShaderInterfaceVariableInfoMap::markAsDuplicate(gl::ShaderType shaderType,
                                                     ShaderVariableType variableType,
                                                     const std::string &variableName)
{
    ASSERT(hasVariable(shaderType, variableName));
    mData[shaderType][variableType][variableName].isDuplicate = true;
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::add(gl::ShaderType shaderType,
                                                                 ShaderVariableType variableType,
                                                                 const std::string &variableName)
{
    ASSERT(!hasVariable(shaderType, variableName));
    mNameToTypeMap[shaderType][variableName] = variableType;
    return mData[shaderType][variableType][variableName];
}

ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::addOrGet(
    gl::ShaderType shaderType,
    ShaderVariableType variableType,
    const std::string &variableName)
{
    mNameToTypeMap[shaderType][variableName] = variableType;
    return mData[shaderType][variableType][variableName];
}

ShaderInterfaceVariableInfoMap::Iterator ShaderInterfaceVariableInfoMap::getIterator(
    gl::ShaderType shaderType,
    ShaderVariableType variableType) const
{
    return Iterator(mData[shaderType][variableType].begin(), mData[shaderType][variableType].end());
}

bool ShaderInterfaceVariableInfoMap::hasVariable(gl::ShaderType shaderType,
                                                 const std::string &variableName) const
{
    auto iter = mNameToTypeMap[shaderType].find(variableName);
    return (iter != mNameToTypeMap[shaderType].end());
}

const ShaderInterfaceVariableInfo &ShaderInterfaceVariableInfoMap::getVariableByName(
    gl::ShaderType shaderType,
    const std::string &variableName) const
{
    auto iter = mNameToTypeMap[shaderType].find(variableName);
    ASSERT(iter != mNameToTypeMap[shaderType].end());
    ShaderVariableType variableType = iter->second;
    return get(shaderType, variableType, variableName);
}

bool ShaderInterfaceVariableInfoMap::hasTransformFeedbackInfo(gl::ShaderType shaderType,
                                                              uint32_t bufferIndex) const
{
    std::string bufferName = rx::GetXfbBufferName(bufferIndex);
    return hasVariable(shaderType, bufferName);
}
}  // namespace rx

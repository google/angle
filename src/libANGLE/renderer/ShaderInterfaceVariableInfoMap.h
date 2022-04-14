//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Header for the shared ShaderInterfaceVariableInfoMap class, used by both the
// Direct-to-Metal and Metal-SPIRV backends

#ifndef LIBANGLE_RENDERER_SHADERINTERFACEVARIABLEINFOMAP_H_
#define LIBANGLE_RENDERER_SHADERINTERFACEVARIABLEINFOMAP_H_

#include <functional>

#include <stdio.h>
#include "libANGLE/renderer/ProgramImpl.h"
#include "libANGLE/renderer/glslang_wrapper_utils.h"
#include "libANGLE/renderer/renderer_utils.h"
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

// TODO: http://anglebug.com/4524: Need a different hash key than a string, since that's slow to
// calculate.
class ShaderInterfaceVariableInfoMap final : angle::NonCopyable
{
  public:
    ShaderInterfaceVariableInfoMap();
    ~ShaderInterfaceVariableInfoMap();

    void clear();
    ShaderInterfaceVariableInfo &add(gl::ShaderType shaderType,
                                     ShaderVariableType variableType,
                                     const std::string &variableName);
    void markAsDuplicate(gl::ShaderType shaderType,
                         ShaderVariableType variableType,
                         const std::string &variableName);
    ShaderInterfaceVariableInfo &addOrGet(gl::ShaderType shaderType,
                                          ShaderVariableType variableType,
                                          const std::string &variableName);
    size_t variableCount(gl::ShaderType shaderType, ShaderVariableType variableType) const
    {
        return mData[shaderType][variableType].size();
    }

    void setActiveStages(gl::ShaderType shaderType,
                         ShaderVariableType variableType,
                         const std::string &variableName,
                         gl::ShaderBitSet activeStages);
    ShaderInterfaceVariableInfo &getMutable(gl::ShaderType shaderType,
                                            ShaderVariableType variableType,
                                            const std::string &variableName);

    const ShaderInterfaceVariableInfo &getDefaultUniformInfo(gl::ShaderType shaderType) const;
    const ShaderInterfaceVariableInfo &getIndexedVariableInfo(
        const gl::ProgramExecutable &executable,
        gl::ShaderType shaderType,
        ShaderVariableType variableType,
        uint32_t variableIndex) const;
    bool hasAtomicCounterInfo(gl::ShaderType shaderType) const;
    const ShaderInterfaceVariableInfo &getAtomicCounterInfo(gl::ShaderType shaderType) const;
    const ShaderInterfaceVariableInfo &getFramebufferFetchInfo(
        const gl::ProgramExecutable &executable,
        gl::ShaderType shaderType) const;
    bool hasTransformFeedbackInfo(gl::ShaderType shaderType, uint32_t bufferIndex) const;
    const ShaderInterfaceVariableInfo &getTransformFeedbackInfo(gl::ShaderType shaderType,
                                                                uint32_t bufferIndex) const;

    using VariableNameToInfoMap = angle::HashMap<std::string, ShaderInterfaceVariableInfo>;
    using VariableTypeToInfoMap = angle::PackedEnumMap<ShaderVariableType, VariableNameToInfoMap>;

    class Iterator final
    {
      public:
        Iterator(VariableNameToInfoMap::const_iterator beginIt,
                 VariableNameToInfoMap::const_iterator endIt)
            : mBeginIt(beginIt), mEndIt(endIt)
        {}
        VariableNameToInfoMap::const_iterator begin() { return mBeginIt; }
        VariableNameToInfoMap::const_iterator end() { return mEndIt; }

      private:
        VariableNameToInfoMap::const_iterator mBeginIt;
        VariableNameToInfoMap::const_iterator mEndIt;
    };

    Iterator getIterator(gl::ShaderType shaderType, ShaderVariableType variableType) const;

    bool hasVariable(gl::ShaderType shaderType, const std::string &variableName) const;
    const ShaderInterfaceVariableInfo &getVariableByName(gl::ShaderType shaderType,
                                                         const std::string &variableName) const;

  private:
    const ShaderInterfaceVariableInfo &get(gl::ShaderType shaderType,
                                           ShaderVariableType variableType,
                                           const std::string &variableName) const;
    gl::ShaderMap<VariableTypeToInfoMap> mData;
    gl::ShaderMap<angle::HashMap<std::string, ShaderVariableType>> mNameToTypeMap;
};

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getDefaultUniformInfo(gl::ShaderType shaderType) const
{
    const char *uniformName = kDefaultUniformNames[shaderType];
    return get(shaderType, ShaderVariableType::DefaultUniform, uniformName);
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getIndexedVariableInfo(const gl::ProgramExecutable &executable,
                                                       gl::ShaderType shaderType,
                                                       ShaderVariableType variableType,
                                                       uint32_t variableIndex) const
{
    switch (variableType)
    {
        case ShaderVariableType::Image:
        {
            const std::vector<gl::LinkedUniform> &uniforms = executable.getUniforms();
            uint32_t uniformIndex = executable.getUniformIndexFromImageIndex(variableIndex);
            const gl::LinkedUniform &imageUniform = uniforms[uniformIndex];
            const std::string samplerName         = GlslangGetMappedSamplerName(imageUniform.name);
            return get(shaderType, variableType, samplerName);
        }
        case ShaderVariableType::ShaderStorageBuffer:
        {
            const std::vector<gl::InterfaceBlock> &blocks = executable.getShaderStorageBlocks();
            const gl::InterfaceBlock &block               = blocks[variableIndex];
            const std::string blockName                   = block.mappedName;
            return get(shaderType, variableType, blockName);
        }
        case ShaderVariableType::Texture:
        {
            const std::vector<gl::LinkedUniform> &uniforms = executable.getUniforms();
            uint32_t uniformIndex = executable.getUniformIndexFromSamplerIndex(variableIndex);
            const gl::LinkedUniform &samplerUniform = uniforms[uniformIndex];
            const std::string samplerName = GlslangGetMappedSamplerName(samplerUniform.name);
            return get(shaderType, variableType, samplerName);
        }
        case ShaderVariableType::UniformBuffer:
        {
            const std::vector<gl::InterfaceBlock> &blocks = executable.getUniformBlocks();
            const gl::InterfaceBlock &block               = blocks[variableIndex];
            const std::string blockName                   = block.mappedName;
            return get(shaderType, variableType, blockName);
        }

        default:
            break;
    }

    UNREACHABLE();
    return mData[shaderType].begin()->begin()->second;
}

ANGLE_INLINE bool ShaderInterfaceVariableInfoMap::hasAtomicCounterInfo(
    gl::ShaderType shaderType) const
{
    return !mData[shaderType][ShaderVariableType::AtomicCounter].empty();
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getAtomicCounterInfo(gl::ShaderType shaderType) const
{
    std::string blockName(sh::vk::kAtomicCountersBlockName);
    return get(shaderType, ShaderVariableType::AtomicCounter, blockName);
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getFramebufferFetchInfo(const gl::ProgramExecutable &executable,
                                                        gl::ShaderType shaderType) const
{
    const std::vector<gl::LinkedUniform> &uniforms = executable.getUniforms();
    const uint32_t baseUniformIndex                = executable.getFragmentInoutRange().low();
    const gl::LinkedUniform &baseInputAttachment   = uniforms.at(baseUniformIndex);
    std::string baseMappedName                     = baseInputAttachment.mappedName;
    return get(shaderType, ShaderVariableType::FramebufferFetch, baseMappedName);
}

ANGLE_INLINE const ShaderInterfaceVariableInfo &
ShaderInterfaceVariableInfoMap::getTransformFeedbackInfo(gl::ShaderType shaderType,
                                                         uint32_t bufferIndex) const
{
    const std::string bufferName = GetXfbBufferName(bufferIndex);
    return get(shaderType, ShaderVariableType::TransformFeedback, bufferName);
}
}  // namespace rx
#endif  // LIBANGLE_RENDERER_SHADERINTERFACEVARIABLEINFOMAP_H_

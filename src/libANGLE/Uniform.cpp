//
// Copyright 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/Uniform.h"
#include "common/BinaryStream.h"
#include "libANGLE/ProgramLinkedResources.h"

#include <cstring>

namespace gl
{

ActiveVariable::ActiveVariable()
{
    std::fill(mIds.begin(), mIds.end(), 0);
}

ActiveVariable::~ActiveVariable() {}

ActiveVariable::ActiveVariable(const ActiveVariable &rhs)            = default;
ActiveVariable &ActiveVariable::operator=(const ActiveVariable &rhs) = default;

void ActiveVariable::setActive(ShaderType shaderType, bool used, uint32_t id)
{
    ASSERT(shaderType != ShaderType::InvalidEnum);
    mActiveUseBits.set(shaderType, used);
    mIds[shaderType] = id;
}

void ActiveVariable::unionReferencesWith(const LinkedUniform &other)
{
    mActiveUseBits |= other.mActiveUseBits;
    for (const ShaderType shaderType : AllShaderTypes())
    {
        ASSERT(mIds[shaderType] == 0 || other.getId(shaderType) == 0 ||
               mIds[shaderType] == other.getId(shaderType));
        if (mIds[shaderType] == 0)
        {
            mIds[shaderType] = other.getId(shaderType);
        }
    }
}

LinkedUniform::LinkedUniform() = default;

LinkedUniform::LinkedUniform(GLenum typeIn,
                             GLenum precisionIn,
                             const std::vector<unsigned int> &arraySizesIn,
                             const int bindingIn,
                             const int offsetIn,
                             const int locationIn,
                             const int bufferIndexIn,
                             const sh::BlockMemberInfo &blockInfoIn)
{
    // arrays are always flattened, which means at most 1D array
    ASSERT(arraySizesIn.size() <= 1);

    memset(this, 0, sizeof(*this));
    SetBitField(type, typeIn);
    SetBitField(precision, precisionIn);
    location = locationIn;
    SetBitField(binding, bindingIn);
    SetBitField(offset, offsetIn);
    SetBitField(bufferIndex, bufferIndexIn);
    outerArraySizeProduct = 1;
    SetBitField(arraySize, arraySizesIn.empty() ? 1u : arraySizesIn[0]);
    SetBitField(flagBits.isArray, !arraySizesIn.empty());
    if (!(blockInfoIn == sh::kDefaultBlockMemberInfo))
    {
        flagBits.isBlock               = 1;
        flagBits.blockIsRowMajorMatrix = blockInfoIn.isRowMajorMatrix;
        SetBitField(blockOffset, blockInfoIn.offset);
        SetBitField(blockArrayStride, blockInfoIn.arrayStride);
        SetBitField(blockMatrixStride, blockInfoIn.matrixStride);
    }
}

LinkedUniform::LinkedUniform(const UsedUniform &usedUniform)
{
    ASSERT(!usedUniform.isArrayOfArrays());
    ASSERT(!usedUniform.isStruct());
    ASSERT(usedUniform.active);
    ASSERT(usedUniform.blockInfo == sh::kDefaultBlockMemberInfo);

    // Note: Ensure every data member is initialized.
    flagBitsAsUByte = 0;
    SetBitField(type, usedUniform.type);
    SetBitField(precision, usedUniform.precision);
    SetBitField(imageUnitFormat, usedUniform.imageUnitFormat);
    location          = usedUniform.location;
    blockOffset       = 0;
    blockArrayStride  = 0;
    blockMatrixStride = 0;
    SetBitField(binding, usedUniform.binding);
    SetBitField(offset, usedUniform.offset);

    SetBitField(bufferIndex, usedUniform.bufferIndex);
    SetBitField(parentArrayIndex, usedUniform.parentArrayIndex());
    SetBitField(outerArraySizeProduct, ArraySizeProduct(usedUniform.outerArraySizes));
    SetBitField(outerArrayOffset, usedUniform.outerArrayOffset);
    SetBitField(arraySize, usedUniform.isArray() ? usedUniform.getArraySizeProduct() : 1u);
    SetBitField(flagBits.isArray, usedUniform.isArray());

    id             = usedUniform.id;
    mActiveUseBits = usedUniform.activeVariable.activeShaders();
    mIds           = usedUniform.activeVariable.getIds();

    SetBitField(flagBits.isFragmentInOut, usedUniform.isFragmentInOut);
    SetBitField(flagBits.texelFetchStaticUse, usedUniform.texelFetchStaticUse);
    ASSERT(!usedUniform.isArray() || arraySize == usedUniform.getArraySizeProduct());
}

BufferVariable::BufferVariable()
{
    memset(&pod, 0, sizeof(pod));
    pod.bufferIndex       = -1;
    pod.blockInfo         = sh::kDefaultBlockMemberInfo;
    pod.topLevelArraySize = -1;
}

BufferVariable::BufferVariable(GLenum type,
                               GLenum precision,
                               const std::string &name,
                               const std::vector<unsigned int> &arraySizes,
                               const int bufferIndex,
                               int topLevelArraySize,
                               const sh::BlockMemberInfo &blockInfo)
    : name(name)
{
    memset(&pod, 0, sizeof(pod));
    SetBitField(pod.type, type);
    SetBitField(pod.precision, precision);
    SetBitField(pod.bufferIndex, bufferIndex);
    pod.blockInfo = blockInfo;
    SetBitField(pod.topLevelArraySize, topLevelArraySize);
    pod.isArray = !arraySizes.empty();
    SetBitField(pod.basicTypeElementCount, arraySizes.empty() ? 1u : arraySizes.back());
}

ShaderVariableBuffer::ShaderVariableBuffer()
{
    memset(&pod, 0, sizeof(pod));
}

void ShaderVariableBuffer::unionReferencesWith(const LinkedUniform &other)
{
    pod.activeUseBits |= other.mActiveUseBits;
    for (const ShaderType shaderType : AllShaderTypes())
    {
        ASSERT(pod.ids[shaderType] == 0 || other.getId(shaderType) == 0 ||
               pod.ids[shaderType] == other.getId(shaderType));
        if (pod.ids[shaderType] == 0)
        {
            pod.ids[shaderType] = other.getId(shaderType);
        }
    }
}

InterfaceBlock::InterfaceBlock()
{
    memset(&pod, 0, sizeof(pod));
}

InterfaceBlock::InterfaceBlock(const std::string &name,
                               const std::string &mappedName,
                               bool isArray,
                               bool isReadOnly,
                               unsigned int arrayElementIn,
                               unsigned int firstFieldArraySizeIn,
                               int binding)
    : name(name), mappedName(mappedName)
{
    memset(&pod, 0, sizeof(pod));

    SetBitField(pod.isArray, isArray);
    SetBitField(pod.isReadOnly, isReadOnly);
    SetBitField(pod.binding, binding);
    pod.arrayElement        = arrayElementIn;
    pod.firstFieldArraySize = firstFieldArraySizeIn;
}

std::string InterfaceBlock::nameWithArrayIndex() const
{
    std::stringstream fullNameStr;
    fullNameStr << name;
    if (pod.isArray)
    {
        fullNameStr << "[" << pod.arrayElement << "]";
    }

    return fullNameStr.str();
}

std::string InterfaceBlock::mappedNameWithArrayIndex() const
{
    std::stringstream fullNameStr;
    fullNameStr << mappedName;
    if (pod.isArray)
    {
        fullNameStr << "[" << pod.arrayElement << "]";
    }

    return fullNameStr.str();
}
}  // namespace gl

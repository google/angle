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

void ActiveVariable::unionReferencesWith(const ActiveVariable &other)
{
    mActiveUseBits |= other.mActiveUseBits;
    for (const ShaderType shaderType : AllShaderTypes())
    {
        ASSERT(mIds[shaderType] == 0 || other.mIds[shaderType] == 0 ||
               mIds[shaderType] == other.mIds[shaderType]);
        if (mIds[shaderType] == 0)
        {
            mIds[shaderType] = other.mIds[shaderType];
        }
    }
}

LinkedUniform::LinkedUniform() {}

LinkedUniform::LinkedUniform(GLenum typeIn,
                             GLenum precisionIn,
                             const std::vector<unsigned int> &arraySizesIn,
                             const int bindingIn,
                             const int offsetIn,
                             const int locationIn,
                             const int bufferIndexIn,
                             const sh::BlockMemberInfo &blockInfoIn)
{
    // Note: Ensure every data member is initialized.
    type                          = typeIn;
    precision                     = precisionIn;
    imageUnitFormat               = GL_NONE;
    location                      = locationIn;
    binding                       = bindingIn;
    offset                        = offsetIn;
    bufferIndex                   = bufferIndexIn;
    blockInfo                     = blockInfoIn;
    id                            = 0;
    flattenedOffsetInParentArrays = -1;
    outerArraySizeProduct         = 1;
    outerArrayOffset              = 0;
    arraySize                     = arraySizesIn.empty() ? 1 : arraySizesIn[0];

    flagBitsAsUInt   = 0;
    flagBits.isArray = !arraySizesIn.empty();
    ASSERT(arraySizesIn.size() <= 1);

    typeInfo = &GetUniformTypeInfo(typeIn);
}

LinkedUniform::LinkedUniform(const LinkedUniform &other)
{
    memcpy(this, &other, sizeof(LinkedUniform));
}

LinkedUniform::LinkedUniform(const UsedUniform &usedUniform)
{
    ASSERT(!usedUniform.isArrayOfArrays());
    ASSERT(!usedUniform.isStruct());
    ASSERT(usedUniform.active);

    // Note: Ensure every data member is initialized.
    type                          = usedUniform.type;
    precision                     = usedUniform.precision;
    imageUnitFormat               = usedUniform.imageUnitFormat;
    location                      = usedUniform.location;
    binding                       = usedUniform.binding;
    offset                        = usedUniform.offset;
    bufferIndex                   = usedUniform.bufferIndex;
    blockInfo                     = usedUniform.blockInfo;
    id                            = usedUniform.id;
    flattenedOffsetInParentArrays = usedUniform.getFlattenedOffsetInParentArrays();
    outerArraySizeProduct         = ArraySizeProduct(usedUniform.outerArraySizes);
    outerArrayOffset              = usedUniform.outerArrayOffset;
    arraySize                     = usedUniform.isArray() ? usedUniform.getArraySizeProduct() : 1u;

    activeVariable = usedUniform.activeVariable;

    flagBitsAsUInt               = 0;
    flagBits.isFragmentInOut     = usedUniform.isFragmentInOut;
    flagBits.texelFetchStaticUse = usedUniform.texelFetchStaticUse;
    flagBits.isArray             = usedUniform.isArray();

    typeInfo = usedUniform.typeInfo;
}

LinkedUniform::~LinkedUniform() {}

BufferVariable::BufferVariable()
    : bufferIndex(-1), blockInfo(sh::kDefaultBlockMemberInfo), topLevelArraySize(-1)
{}

BufferVariable::BufferVariable(GLenum typeIn,
                               GLenum precisionIn,
                               const std::string &nameIn,
                               const std::vector<unsigned int> &arraySizesIn,
                               const int bufferIndexIn,
                               const sh::BlockMemberInfo &blockInfoIn)
    : bufferIndex(bufferIndexIn), blockInfo(blockInfoIn), topLevelArraySize(-1)
{
    type       = typeIn;
    precision  = precisionIn;
    name       = nameIn;
    arraySizes = arraySizesIn;
}

BufferVariable::~BufferVariable() {}

ShaderVariableBuffer::ShaderVariableBuffer() : binding(0), dataSize(0) {}

ShaderVariableBuffer::ShaderVariableBuffer(const ShaderVariableBuffer &other) = default;

ShaderVariableBuffer::~ShaderVariableBuffer() {}

int ShaderVariableBuffer::numActiveVariables() const
{
    return static_cast<int>(memberIndexes.size());
}

InterfaceBlock::InterfaceBlock() : isArray(false), isReadOnly(false), arrayElement(0) {}

InterfaceBlock::InterfaceBlock(const std::string &nameIn,
                               const std::string &mappedNameIn,
                               bool isArrayIn,
                               bool isReadOnlyIn,
                               unsigned int arrayElementIn,
                               unsigned int firstFieldArraySizeIn,
                               int bindingIn)
    : name(nameIn),
      mappedName(mappedNameIn),
      isArray(isArrayIn),
      isReadOnly(isReadOnlyIn),
      arrayElement(arrayElementIn),
      firstFieldArraySize(firstFieldArraySizeIn)
{
    binding = bindingIn;
}

InterfaceBlock::InterfaceBlock(const InterfaceBlock &other) = default;

std::string InterfaceBlock::nameWithArrayIndex() const
{
    std::stringstream fullNameStr;
    fullNameStr << name;
    if (isArray)
    {
        fullNameStr << "[" << arrayElement << "]";
    }

    return fullNameStr.str();
}

std::string InterfaceBlock::mappedNameWithArrayIndex() const
{
    std::stringstream fullNameStr;
    fullNameStr << mappedName;
    if (isArray)
    {
        fullNameStr << "[" << arrayElement << "]";
    }

    return fullNameStr.str();
}
}  // namespace gl

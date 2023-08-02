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

LinkedUniform::LinkedUniform()
{
    mFixedSizeData.type                          = GL_NONE;
    mFixedSizeData.precision                     = 0;
    mFixedSizeData.flagBitsAsUInt                = 0;
    mFixedSizeData.location                      = -1;
    mFixedSizeData.binding                       = -1;
    mFixedSizeData.imageUnitFormat               = GL_NONE;
    mFixedSizeData.offset                        = -1;
    mFixedSizeData.id                            = 0;
    mFixedSizeData.flattenedOffsetInParentArrays = -1;
    typeInfo                                     = nullptr;
    mFixedSizeData.bufferIndex                   = -1;
    mFixedSizeData.blockInfo                     = sh::kDefaultBlockMemberInfo;
    mFixedSizeData.outerArraySizeProduct         = 1;
    mFixedSizeData.outerArrayOffset              = 0;
}

LinkedUniform::LinkedUniform(GLenum typeIn,
                             GLenum precisionIn,
                             const std::string &nameIn,
                             const std::vector<unsigned int> &arraySizesIn,
                             const int bindingIn,
                             const int offsetIn,
                             const int locationIn,
                             const int bufferIndexIn,
                             const sh::BlockMemberInfo &blockInfoIn)
{
    mFixedSizeData.type                          = typeIn;
    mFixedSizeData.precision                     = precisionIn;
    mFixedSizeData.location                      = locationIn;
    mFixedSizeData.binding                       = bindingIn;
    mFixedSizeData.offset                        = offsetIn;
    mFixedSizeData.bufferIndex                   = bufferIndexIn;
    mFixedSizeData.blockInfo                     = blockInfoIn;
    mFixedSizeData.flagBitsAsUInt                = 0;
    mFixedSizeData.id                            = 0;
    mFixedSizeData.flattenedOffsetInParentArrays = -1;
    mFixedSizeData.outerArraySizeProduct         = 1;
    mFixedSizeData.outerArrayOffset              = 0;
    mFixedSizeData.imageUnitFormat               = GL_NONE;

    name       = nameIn;
    typeInfo   = &GetUniformTypeInfo(typeIn);
    arraySizes = arraySizesIn;

    ASSERT(!isArrayOfArrays());
    ASSERT(!isArray() || !isStruct());
}

LinkedUniform::LinkedUniform(const LinkedUniform &other)
{
    *this = other;
}

LinkedUniform::LinkedUniform(const UsedUniform &usedUniform)
{
    mFixedSizeData.type      = usedUniform.type;
    mFixedSizeData.precision = usedUniform.precision;

    mFixedSizeData.flagBits.staticUse           = usedUniform.staticUse;
    mFixedSizeData.flagBits.active              = usedUniform.active;
    mFixedSizeData.flagBits.isStruct            = usedUniform.isStruct();
    mFixedSizeData.flagBits.rasterOrdered       = usedUniform.rasterOrdered;
    mFixedSizeData.flagBits.readonly            = usedUniform.readonly;
    mFixedSizeData.flagBits.writeonly           = usedUniform.writeonly;
    mFixedSizeData.flagBits.isFragmentInOut     = usedUniform.isFragmentInOut;
    mFixedSizeData.flagBits.texelFetchStaticUse = usedUniform.texelFetchStaticUse;

    mFixedSizeData.flattenedOffsetInParentArrays = usedUniform.getFlattenedOffsetInParentArrays();
    mFixedSizeData.location                      = usedUniform.location;
    mFixedSizeData.binding                       = usedUniform.binding;
    mFixedSizeData.imageUnitFormat               = usedUniform.imageUnitFormat;
    mFixedSizeData.offset                        = usedUniform.offset;
    mFixedSizeData.id                            = usedUniform.id;
    mFixedSizeData.bufferIndex                   = usedUniform.bufferIndex;
    mFixedSizeData.blockInfo                     = usedUniform.blockInfo;
    mFixedSizeData.outerArraySizeProduct         = ArraySizeProduct(usedUniform.outerArraySizes);
    mFixedSizeData.outerArrayOffset              = usedUniform.outerArrayOffset;
    mFixedSizeData.activeVariable                = usedUniform.activeVariable;

    name       = usedUniform.name;
    mappedName = usedUniform.mappedName;
    arraySizes = usedUniform.arraySizes;
    typeInfo   = usedUniform.typeInfo;
}

LinkedUniform &LinkedUniform::operator=(const LinkedUniform &other)
{
    mFixedSizeData = other.mFixedSizeData;

    name       = other.name;
    mappedName = other.mappedName;
    arraySizes = other.arraySizes;
    typeInfo   = other.typeInfo;

    return *this;
}

LinkedUniform::~LinkedUniform() {}

void LinkedUniform::save(BinaryOutputStream *stream) const
{
    // mFixedSizeData is a simple structure with fundamental data types, we can just do bulk save
    // for performance.
    stream->writeBytes(reinterpret_cast<const unsigned char *>(&mFixedSizeData),
                       sizeof(mFixedSizeData));

    stream->writeString(name);
    stream->writeString(mappedName);
    stream->writeIntVector(arraySizes);
}

void LinkedUniform::load(BinaryInputStream *stream)
{
    // mFixedSizeData is a simple structure with fundamental data types, we can just do bulk load
    // for performance.
    stream->readBytes(reinterpret_cast<unsigned char *>(&mFixedSizeData), sizeof(mFixedSizeData));

    stream->readString(&name);
    stream->readString(&mappedName);
    stream->readIntVector<unsigned int>(&arraySizes);

    typeInfo = &GetUniformTypeInfo(getType());
}

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

//
// Copyright (c) 2010-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libANGLE/Uniform.h"

#include "common/utilities.h"

#include <cstring>

namespace gl
{

LinkedUniform::LinkedUniform()
    : bufferIndex(-1), blockInfo(sh::BlockMemberInfo::getDefaultBlockInfo())
{
}

LinkedUniform::LinkedUniform(GLenum typeIn,
                             GLenum precisionIn,
                             const std::string &nameIn,
                             unsigned int arraySizeIn,
                             const int bindingIn,
                             const int offsetIn,
                             const int locationIn,
                             const int bufferIndexIn,
                             const sh::BlockMemberInfo &blockInfoIn)
    : bufferIndex(bufferIndexIn), blockInfo(blockInfoIn)
{
    type      = typeIn;
    precision = precisionIn;
    name      = nameIn;
    arraySize = arraySizeIn;
    binding   = bindingIn;
    offset    = offsetIn;
    location  = locationIn;
}

LinkedUniform::LinkedUniform(const sh::Uniform &uniform)
    : sh::Uniform(uniform), bufferIndex(-1), blockInfo(sh::BlockMemberInfo::getDefaultBlockInfo())
{
}

LinkedUniform::LinkedUniform(const LinkedUniform &uniform)
    : sh::Uniform(uniform), bufferIndex(uniform.bufferIndex), blockInfo(uniform.blockInfo)
{
    // This function is not intended to be called during runtime.
    ASSERT(uniform.mLazyData.empty());
}

LinkedUniform &LinkedUniform::operator=(const LinkedUniform &uniform)
{
    // This function is not intended to be called during runtime.
    ASSERT(uniform.mLazyData.empty());

    sh::Uniform::operator=(uniform);
    bufferIndex          = uniform.bufferIndex;
    blockInfo            = uniform.blockInfo;

    return *this;
}

LinkedUniform::~LinkedUniform()
{
}

bool LinkedUniform::isInDefaultBlock() const
{
    return bufferIndex == -1;
}

size_t LinkedUniform::dataSize() const
{
    ASSERT(type != GL_STRUCT_ANGLEX);
    if (mLazyData.empty())
    {
        mLazyData.resize(VariableExternalSize(type) * elementCount());
        ASSERT(!mLazyData.empty());
    }

    return mLazyData.size();
}

const uint8_t *LinkedUniform::data() const
{
    return const_cast<LinkedUniform *>(this)->data();
}

bool LinkedUniform::isSampler() const
{
    return IsSamplerType(type);
}

bool LinkedUniform::isImage() const
{
    return IsImageType(type);
}

bool LinkedUniform::isAtomicCounter() const
{
    return IsAtomicCounterType(type);
}

bool LinkedUniform::isField() const
{
    return name.find('.') != std::string::npos;
}

size_t LinkedUniform::getElementSize() const
{
    return VariableExternalSize(type);
}

size_t LinkedUniform::getElementComponents() const
{
    return VariableComponentCount(type);
}

uint8_t *LinkedUniform::getDataPtrToElement(size_t elementIndex)
{
    ASSERT((!isArray() && elementIndex == 0) || (isArray() && elementIndex < arraySize));
    return data() + (elementIndex > 0 ? (getElementSize() * elementIndex) : 0u);
}

const uint8_t *LinkedUniform::getDataPtrToElement(size_t elementIndex) const
{
    return const_cast<LinkedUniform *>(this)->getDataPtrToElement(elementIndex);
}

ShaderVariableBuffer::ShaderVariableBuffer()
    : binding(0),
      dataSize(0),
      vertexStaticUse(false),
      fragmentStaticUse(false),
      computeStaticUse(false)
{
}

ShaderVariableBuffer::~ShaderVariableBuffer()
{
}

UniformBlock::UniformBlock() : isArray(false), arrayElement(0)
{
}

UniformBlock::UniformBlock(const std::string &nameIn,
                           bool isArrayIn,
                           unsigned int arrayElementIn,
                           int bindingIn)
    : name(nameIn), isArray(isArrayIn), arrayElement(arrayElementIn)
{
    binding = bindingIn;
}

std::string UniformBlock::nameWithArrayIndex() const
{
    std::stringstream fullNameStr;
    fullNameStr << name;
    if (isArray)
    {
        fullNameStr << "[" << arrayElement << "]";
    }

    return fullNameStr.str();
}
}

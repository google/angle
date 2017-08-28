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

template <typename T>
void MarkResourceStaticUse(T *resource, GLenum shaderType, bool used)
{
    switch (shaderType)
    {
        case GL_VERTEX_SHADER:
            resource->vertexStaticUse = used;
            break;

        case GL_FRAGMENT_SHADER:
            resource->fragmentStaticUse = used;
            break;

        case GL_COMPUTE_SHADER:
            resource->computeStaticUse = used;
            break;

        default:
            UNREACHABLE();
    }
}

template void MarkResourceStaticUse(LinkedUniform *resource, GLenum shaderType, bool used);
template void MarkResourceStaticUse(InterfaceBlock *resource, GLenum shaderType, bool used);

LinkedUniform::LinkedUniform()
    : typeInfo(nullptr),
      bufferIndex(-1),
      blockInfo(sh::BlockMemberInfo::getDefaultBlockInfo()),
      vertexStaticUse(false),
      fragmentStaticUse(false),
      computeStaticUse(false)
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
    : typeInfo(&GetUniformTypeInfo(typeIn)),
      bufferIndex(bufferIndexIn),
      blockInfo(blockInfoIn),
      vertexStaticUse(false),
      fragmentStaticUse(false),
      computeStaticUse(false)
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
    : sh::Uniform(uniform),
      typeInfo(&GetUniformTypeInfo(type)),
      bufferIndex(-1),
      blockInfo(sh::BlockMemberInfo::getDefaultBlockInfo()),
      vertexStaticUse(false),
      fragmentStaticUse(false),
      computeStaticUse(false)
{
}

LinkedUniform::LinkedUniform(const LinkedUniform &uniform)
    : sh::Uniform(uniform),
      typeInfo(uniform.typeInfo),
      bufferIndex(uniform.bufferIndex),
      blockInfo(uniform.blockInfo),
      vertexStaticUse(uniform.vertexStaticUse),
      fragmentStaticUse(uniform.fragmentStaticUse),
      computeStaticUse(uniform.computeStaticUse)

{
}

LinkedUniform &LinkedUniform::operator=(const LinkedUniform &uniform)
{
    sh::Uniform::operator=(uniform);
    typeInfo             = uniform.typeInfo;
    bufferIndex          = uniform.bufferIndex;
    blockInfo            = uniform.blockInfo;
    vertexStaticUse      = uniform.vertexStaticUse;
    fragmentStaticUse    = uniform.fragmentStaticUse;
    computeStaticUse     = uniform.computeStaticUse;

    return *this;
}

LinkedUniform::~LinkedUniform()
{
}

bool LinkedUniform::isInDefaultBlock() const
{
    return bufferIndex == -1;
}

bool LinkedUniform::isSampler() const
{
    return typeInfo->isSampler;
}

bool LinkedUniform::isImage() const
{
    return typeInfo->isImageType;
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
    return typeInfo->externalSize;
}

size_t LinkedUniform::getElementComponents() const
{
    return typeInfo->componentCount;
}

ShaderVariableBuffer::ShaderVariableBuffer()
    : binding(0),
      dataSize(0),
      vertexStaticUse(false),
      fragmentStaticUse(false),
      computeStaticUse(false)
{
}

InterfaceBlock::InterfaceBlock() : isArray(false), arrayElement(0)
{
}

InterfaceBlock::InterfaceBlock(const std::string &nameIn,
                               const std::string &mappedNameIn,
                               bool isArrayIn,
                               unsigned int arrayElementIn,
                               int bindingIn)
    : name(nameIn), mappedName(mappedNameIn), isArray(isArrayIn), arrayElement(arrayElementIn)
{
    binding = bindingIn;
}

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
}

//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/Uniform.h"
#include "common/mathutil.h"
#include "common/utilities.h"

namespace sh
{

ShaderVariable::ShaderVariable()
    : type(GL_NONE),
      precision(GL_NONE),
      arraySize(0),
      location(-1)
{
}

ShaderVariable::ShaderVariable(GLenum type, GLenum precision, const char *name, unsigned int arraySize, int location)
    : type(type),
      precision(precision),
      name(name),
      arraySize(arraySize),
      location(location)
{
}

Uniform::Uniform(GLenum type, GLenum precision, const char *name, unsigned int arraySize, unsigned int registerIndex)
{
    this->type = type;
    this->precision = precision;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
}

BlockMemberInfo::BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
    : offset(offset),
      arrayStride(arrayStride),
      matrixStride(matrixStride),
      isRowMajorMatrix(isRowMajorMatrix)
{
}

const BlockMemberInfo BlockMemberInfo::defaultBlockInfo(-1, -1, -1, false);

InterfaceBlock::InterfaceBlock(const char *name, unsigned int arraySize, unsigned int registerIndex)
    : name(name),
      arraySize(arraySize),
      layout(BLOCKLAYOUT_SHARED),
      registerIndex(registerIndex)
{
}

// Use the same layout for packed and shared
void InterfaceBlock::setBlockLayout(BlockLayoutType newLayout)
{
    layout = newLayout;

    const size_t componentSize = 4;
    unsigned int currentOffset = 0;

    blockInfo.clear();
    getBlockLayoutInfo(activeUniforms, &currentOffset);

    dataSize = currentOffset * componentSize;
}

void InterfaceBlock::getBlockLayoutInfo(const sh::ActiveUniforms &fields, unsigned int *currentOffset)
{
    const size_t componentSize = 4;

    // TODO: row major matrices
    bool isRowMajorMatrix = false;

    for (unsigned int fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
        int arrayStride;
        int matrixStride;

        const sh::Uniform &uniform = fields[fieldIndex];

        if (getBlockLayoutInfo(uniform, currentOffset, &arrayStride, &matrixStride))
        {
            const BlockMemberInfo memberInfo(*currentOffset * componentSize, arrayStride * componentSize, matrixStride * componentSize, isRowMajorMatrix);
            blockInfo.push_back(memberInfo);

            if (uniform.arraySize > 0)
            {
                *currentOffset += arrayStride * (uniform.arraySize - 1);
            }

            if (gl::IsMatrixType(uniform.type))
            {
                const int componentGroups = (isRowMajorMatrix ? gl::VariableRowCount(uniform.type) : gl::VariableColumnCount(uniform.type));
                const int numComponents = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
                *currentOffset += matrixStride * (componentGroups - 1);
                *currentOffset += numComponents;
            }
            else
            {
                *currentOffset += gl::UniformComponentCount(uniform.type);
            }
        }
    }
}

bool InterfaceBlock::getBlockLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut)
{
    if (!uniform.fields.empty())
    {
        const unsigned int elementCount = std::max(1u, uniform.arraySize);

        for (unsigned int elementIndex = 0; elementIndex < elementCount; elementIndex++)
        {
            // align struct to register size
            *currentOffset = rx::roundUp(*currentOffset, 4u);
            getBlockLayoutInfo(uniform.fields, currentOffset);
        }

        return false;
    }

    switch (layout)
    {
      case BLOCKLAYOUT_SHARED:
      case BLOCKLAYOUT_PACKED:
        getD3DLayoutInfo(uniform, currentOffset, arrayStrideOut, matrixStrideOut);
        return true;

      case BLOCKLAYOUT_STANDARD:
        getStandardLayoutInfo(uniform, currentOffset, arrayStrideOut, matrixStrideOut);
        return true;

      default:
        UNREACHABLE();
        return false;
    }
}

// Block layout packed according to the default D3D11 register packing rules
// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx
void InterfaceBlock::getD3DLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut)
{
    ASSERT(uniform.fields.empty());

    const unsigned int registerSize = 4;
    const size_t componentSize = 4;

    // TODO: row major matrices
    bool isRowMajorMatrix = false;
    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == componentSize);
    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(uniform.type))
    {
        *currentOffset = rx::roundUp(*currentOffset, 4u);
        matrixStride = registerSize;

        if (uniform.arraySize > 0)
        {
            const int componentGroups = (isRowMajorMatrix ? gl::VariableRowCount(uniform.type) : gl::VariableColumnCount(uniform.type));
            arrayStride = matrixStride * componentGroups;
        }
    }
    else if (uniform.arraySize > 0)
    {
        *currentOffset = rx::roundUp(*currentOffset, registerSize);
        arrayStride = registerSize;
    }
    else
    {
        int numComponents = gl::UniformComponentCount(uniform.type);
        if ((numComponents + (*currentOffset % registerSize)) > registerSize)
        {
            *currentOffset = rx::roundUp(*currentOffset, registerSize);
        }
    }

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

// Block layout according to the std140 block layout
// See "Standard Uniform Block Layout" in Section 2.11.6 of the OpenGL ES 3.0 specification
void InterfaceBlock::getStandardLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut)
{
    ASSERT(uniform.fields.empty());

    const size_t componentSize = 4;

    // TODO: row major matrices
    bool isRowMajorMatrix = false;

    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == componentSize);

    int numComponents = gl::UniformComponentCount(uniform.type);
    size_t baseAlignment = static_cast<size_t>(numComponents == 3 ? 4 : numComponents);
    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(uniform.type))
    {
        numComponents = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
        baseAlignment = rx::roundUp(baseAlignment, 4u);
        matrixStride = baseAlignment;

        if (uniform.arraySize > 0)
        {
            const int componentGroups = (isRowMajorMatrix ? gl::VariableRowCount(uniform.type) : gl::VariableColumnCount(uniform.type));
            arrayStride = matrixStride * componentGroups;
        }
    }
    else if (uniform.arraySize > 0)
    {
        baseAlignment = rx::roundUp(baseAlignment, 4u);
        arrayStride = baseAlignment;
    }

    *currentOffset = rx::roundUp(*currentOffset, baseAlignment);

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

}

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
      registerIndex(registerIndex)
{
}

// Use the same layout for packed and shared
void InterfaceBlock::setSharedBlockLayout()
{
    setPackedBlockLayout();
}

// Block layout packed according to the default D3D11 register packing rules
// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx
void InterfaceBlock::setPackedBlockLayout()
{
    const size_t componentSize = 4;
    const unsigned int registerSize = 4;
    unsigned int currentOffset = 0;

    blockInfo.clear();

    for (unsigned int uniformIndex = 0; uniformIndex < activeUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = activeUniforms[uniformIndex];

        // TODO: structs
        // TODO: row major matrices
        bool isRowMajorMatrix = false;

        // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
        ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == componentSize);

        int arrayStride = 0;
        int matrixStride = 0;

        if (gl::IsMatrixType(uniform.type))
        {
            currentOffset = rx::roundUp(currentOffset, 4u);
            matrixStride = registerSize;

            if (uniform.arraySize > 0)
            {
                const int componentGroups = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
                arrayStride = matrixStride * componentGroups;
            }
        }
        else if (uniform.arraySize > 0)
        {
            currentOffset = rx::roundUp(currentOffset, registerSize);
            arrayStride = registerSize;
        }
        else
        {
            int numComponents = gl::UniformComponentCount(uniform.type);
            if ((numComponents + (currentOffset % registerSize)) >= registerSize)
            {
                currentOffset = rx::roundUp(currentOffset, registerSize);
            }
        }

        BlockMemberInfo memberInfo(currentOffset * componentSize, arrayStride * componentSize, matrixStride * componentSize, isRowMajorMatrix);
        blockInfo.push_back(memberInfo);

        // for arrays/matrices, the next element is in a multiple of register size
        if (uniform.arraySize > 0)
        {
            currentOffset += arrayStride * uniform.arraySize;
        }
        else if (gl::IsMatrixType(uniform.type))
        {
            const int componentGroups = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
            currentOffset += matrixStride * componentGroups;
        }
        else
        {
            currentOffset += gl::UniformComponentCount(uniform.type);
        }
    }

    dataSize = currentOffset * componentSize;
}

void InterfaceBlock::setStandardBlockLayout()
{
    const size_t componentSize = 4;
    unsigned int currentOffset = 0;

    blockInfo.clear();

    for (unsigned int uniformIndex = 0; uniformIndex < activeUniforms.size(); uniformIndex++)
    {
        const sh::Uniform &uniform = activeUniforms[uniformIndex];

        // TODO: structs
        // TODO: row major matrices
        bool isRowMajorMatrix = false;

        // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
        ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == componentSize);

        int numComponents = gl::UniformComponentCount(uniform.type);
        size_t baseAlignment = static_cast<size_t>(numComponents == 3 ? 4 : numComponents);
        int arrayStride = 0;
        int matrixStride = 0;

        if (gl::IsMatrixType(uniform.type))
        {
            numComponents = (isRowMajorMatrix ? gl::VariableRowCount(uniform.type) : gl::VariableColumnCount(uniform.type));
            baseAlignment = rx::roundUp(baseAlignment, 4u);
            matrixStride = baseAlignment;

            if (uniform.arraySize > 0)
            {
                const int componentGroups = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
                arrayStride = matrixStride * componentGroups;
            }
        }
        else if (uniform.arraySize > 0)
        {
            baseAlignment = rx::roundUp(baseAlignment, 4u);
            arrayStride = baseAlignment;
        }

        const unsigned int alignedOffset = rx::roundUp(currentOffset, baseAlignment);

        BlockMemberInfo memberInfo(alignedOffset * componentSize, arrayStride * componentSize, matrixStride * componentSize, isRowMajorMatrix);
        blockInfo.push_back(memberInfo);

        if (uniform.arraySize > 0)
        {
            currentOffset += arrayStride * uniform.arraySize;
            currentOffset = rx::roundUp(currentOffset, baseAlignment);
        }
        else if (gl::IsMatrixType(uniform.type))
        {
            const int componentGroups = (isRowMajorMatrix ? gl::VariableColumnCount(uniform.type) : gl::VariableRowCount(uniform.type));
            currentOffset += matrixStride * componentGroups;
            currentOffset = rx::roundUp(currentOffset, baseAlignment);
        }
        else
        {
            currentOffset += gl::UniformComponentCount(uniform.type);
        }
    }

    dataSize = currentOffset * componentSize;
}

}

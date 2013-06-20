//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/BlockLayoutEncoder.h"
#include "compiler/Uniform.h"
#include "common/mathutil.h"
#include "common/utilities.h"

namespace sh
{

BlockLayoutEncoder::BlockLayoutEncoder(std::vector<BlockMemberInfo> *blockInfoOut)
    : mCurrentOffset(0),
      mBlockInfoOut(blockInfoOut)
{
}

void BlockLayoutEncoder::encodeFields(const std::vector<Uniform> &fields)
{
    for (unsigned int fieldIndex = 0; fieldIndex < fields.size(); fieldIndex++)
    {
        const Uniform &uniform = fields[fieldIndex];

        if (uniform.fields.size() > 0)
        {
            const unsigned int elementCount = std::max(1u, uniform.arraySize);

            for (unsigned int elementIndex = 0; elementIndex < elementCount; elementIndex++)
            {
                enterAggregateType();
                encodeFields(uniform.fields);
                exitAggregateType();
            }
        }
        else
        {
            encodeType(uniform);
        }
    }
}

void BlockLayoutEncoder::encodeType(const Uniform &uniform)
{
    int arrayStride;
    int matrixStride;

    getBlockLayoutInfo(uniform, &arrayStride, &matrixStride);

    const BlockMemberInfo memberInfo(mCurrentOffset * ComponentSize, arrayStride * ComponentSize, matrixStride * ComponentSize, uniform.isRowMajorMatrix);
    mBlockInfoOut->push_back(memberInfo);

    advanceOffset(uniform, arrayStride, matrixStride);
}

void BlockLayoutEncoder::nextRegister()
{
    mCurrentOffset = rx::roundUp(mCurrentOffset, RegisterSize);
}

Std140BlockEncoder::Std140BlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut)
    : BlockLayoutEncoder(blockInfoOut)
{
}

void Std140BlockEncoder::enterAggregateType()
{
    nextRegister();
}

void Std140BlockEncoder::exitAggregateType()
{
    nextRegister();
}

void Std140BlockEncoder::getBlockLayoutInfo(const Uniform &uniform, int *arrayStrideOut, int *matrixStrideOut)
{
    ASSERT(uniform.fields.empty());

    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == ComponentSize);

    int numComponents = gl::UniformComponentCount(uniform.type);
    size_t baseAlignment = 0;
    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(uniform.type))
    {
        baseAlignment = RegisterSize;
        matrixStride = RegisterSize;

        if (uniform.arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(uniform.type, uniform.isRowMajorMatrix);
            arrayStride = RegisterSize * numRegisters;
        }
    }
    else if (uniform.arraySize > 0)
    {
        baseAlignment = RegisterSize;
        arrayStride = RegisterSize;
    }
    else
    {
        const int numComponents = gl::UniformComponentCount(uniform.type);
        baseAlignment = (numComponents == 3 ? 4u : static_cast<size_t>(numComponents));
    }

    mCurrentOffset = rx::roundUp(mCurrentOffset, baseAlignment);

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

void Std140BlockEncoder::advanceOffset(const Uniform &uniform, int arrayStride, int matrixStride)
{
    if (uniform.arraySize > 0)
    {
        mCurrentOffset += arrayStride * uniform.arraySize;
    }
    else if (gl::IsMatrixType(uniform.type))
    {
        ASSERT(matrixStride == RegisterSize);
        const int numRegisters = gl::MatrixRegisterCount(uniform.type, uniform.isRowMajorMatrix);
        mCurrentOffset += RegisterSize * numRegisters;
    }
    else
    {
        mCurrentOffset += gl::UniformComponentCount(uniform.type);
    }
}

}

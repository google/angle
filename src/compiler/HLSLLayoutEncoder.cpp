//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/HLSLLayoutEncoder.h"
#include "compiler/Uniform.h"
#include "common/mathutil.h"
#include "common/utilities.h"

namespace sh
{

HLSLBlockEncoder::HLSLBlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut)
    : BlockLayoutEncoder(blockInfoOut)
{
}

void HLSLBlockEncoder::enterAggregateType()
{
    nextRegister();
}

void HLSLBlockEncoder::exitAggregateType()
{
}

void HLSLBlockEncoder::getBlockLayoutInfo(const sh::Uniform &uniform, int *arrayStrideOut, int *matrixStrideOut)
{
    ASSERT(uniform.fields.empty());

    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(uniform.type)) == ComponentSize);

    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(uniform.type))
    {
        nextRegister();
        matrixStride = RegisterSize;

        if (uniform.arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(uniform.type, uniform.isRowMajorMatrix);
            arrayStride = RegisterSize * numRegisters;
        }
    }
    else if (uniform.arraySize > 0)
    {
        nextRegister();
        arrayStride = RegisterSize;
    }
    else
    {
        int numComponents = gl::UniformComponentCount(uniform.type);
        if ((numComponents + (mCurrentOffset % RegisterSize)) > RegisterSize)
        {
            nextRegister();
        }
    }

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

void HLSLBlockEncoder::advanceOffset(const sh::Uniform &uniform, int arrayStride, int matrixStride)
{
    if (uniform.arraySize > 0)
    {
        mCurrentOffset += arrayStride * (uniform.arraySize - 1);
    }

    if (gl::IsMatrixType(uniform.type))
    {
        ASSERT(matrixStride == RegisterSize);
        const int numRegisters = gl::MatrixRegisterCount(uniform.type, uniform.isRowMajorMatrix);
        const int numComponents = gl::MatrixComponentCount(uniform.type, uniform.isRowMajorMatrix);
        mCurrentOffset += RegisterSize * (numRegisters - 1);
        mCurrentOffset += numComponents;
    }
    else
    {
        mCurrentOffset += gl::UniformComponentCount(uniform.type);
    }
}

}

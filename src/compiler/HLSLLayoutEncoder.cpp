//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/HLSLLayoutEncoder.h"
#include "compiler/ShaderVariable.h"
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

void HLSLBlockEncoder::getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut)
{
    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(type)) == ComponentSize);

    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(type))
    {
        nextRegister();
        matrixStride = RegisterSize;

        if (arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
            arrayStride = RegisterSize * numRegisters;
        }
    }
    else if (arraySize > 0)
    {
        nextRegister();
        arrayStride = RegisterSize;
    }
    else
    {
        int numComponents = gl::UniformComponentCount(type);
        if ((numComponents + (mCurrentOffset % RegisterSize)) > RegisterSize)
        {
            nextRegister();
        }
    }

    *matrixStrideOut = matrixStride;
    *arrayStrideOut = arrayStride;
}

void HLSLBlockEncoder::advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride)
{
    if (arraySize > 0)
    {
        mCurrentOffset += arrayStride * (arraySize - 1);
    }

    if (gl::IsMatrixType(type))
    {
        ASSERT(matrixStride == RegisterSize);
        const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
        const int numComponents = gl::MatrixComponentCount(type, isRowMajorMatrix);
        mCurrentOffset += RegisterSize * (numRegisters - 1);
        mCurrentOffset += numComponents;
    }
    else
    {
        mCurrentOffset += gl::UniformComponentCount(type);
    }
}

}

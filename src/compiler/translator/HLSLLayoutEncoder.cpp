//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/HLSLLayoutEncoder.h"
#include "compiler/translator/ShaderVariable.h"
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
    ASSERT(gl::UniformComponentSize(gl::UniformComponentType(type)) == BytesPerComponent);

    int matrixStride = 0;
    int arrayStride = 0;

    if (gl::IsMatrixType(type))
    {
        nextRegister();
        matrixStride = ComponentsPerRegister;

        if (arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
            arrayStride = ComponentsPerRegister * numRegisters;
        }
    }
    else if (arraySize > 0)
    {
        nextRegister();
        arrayStride = ComponentsPerRegister;
    }
    else
    {
        int numComponents = gl::UniformComponentCount(type);
        if ((numComponents + (mCurrentOffset % ComponentsPerRegister)) > ComponentsPerRegister)
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
        ASSERT(matrixStride == ComponentsPerRegister);
        const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
        const int numComponents = gl::MatrixComponentCount(type, isRowMajorMatrix);
        mCurrentOffset += ComponentsPerRegister * (numRegisters - 1);
        mCurrentOffset += numComponents;
    }
    else
    {
        mCurrentOffset += gl::UniformComponentCount(type);
    }
}

void HLSLVariableGetRegisterInfo(unsigned int baseRegisterIndex, Uniform *variable, HLSLBlockEncoder *encoder, const std::vector<BlockMemberInfo> &blockInfo)
{
    // because this method computes offsets (element indexes) instead of any total sizes,
    // we can ignore the array size of the variable

    if (variable->isStruct())
    {
        encoder->enterAggregateType();

        for (size_t fieldIndex = 0; fieldIndex < variable->fields.size(); fieldIndex++)
        {
            HLSLVariableGetRegisterInfo(baseRegisterIndex, &variable->fields[fieldIndex], encoder, blockInfo);
        }

        encoder->exitAggregateType();
    }
    else
    {
        encoder->encodeType(variable->type, variable->arraySize, false);

        const size_t registerBytes = (encoder->BytesPerComponent * encoder->ComponentsPerRegister);
        variable->registerIndex = baseRegisterIndex + (blockInfo.back().offset / registerBytes);
        variable->elementIndex = (blockInfo.back().offset % registerBytes) / sizeof(float);
    }
}

void HLSLVariableGetRegisterInfo(unsigned int baseRegisterIndex, Uniform *variable)
{
    std::vector<BlockMemberInfo> blockInfo;
    HLSLBlockEncoder encoder(&blockInfo);
    HLSLVariableGetRegisterInfo(baseRegisterIndex, variable, &encoder, blockInfo);
}

template <class ShaderVarType>
void HLSLVariableRegisterCount(const ShaderVarType &variable, HLSLBlockEncoder *encoder)
{
    if (variable.isStruct())
    {
        for (size_t arrayElement = 0; arrayElement < variable.elementCount(); arrayElement++)
        {
            encoder->enterAggregateType();

            for (size_t fieldIndex = 0; fieldIndex < variable.fields.size(); fieldIndex++)
            {
                HLSLVariableRegisterCount(variable.fields[fieldIndex], encoder);
            }

            encoder->exitAggregateType();
        }
    }
    else
    {
        // We operate only on varyings and uniforms, which do not have matrix layout qualifiers
        encoder->encodeType(variable.type, variable.arraySize, false);
    }
}

unsigned int HLSLVariableRegisterCount(const Varying &variable)
{
    HLSLBlockEncoder encoder(NULL);
    HLSLVariableRegisterCount(variable, &encoder);

    const size_t registerBytes = (encoder.BytesPerComponent * encoder.ComponentsPerRegister);
    return static_cast<unsigned int>(rx::roundUp<size_t>(encoder.getBlockSize(), registerBytes) / registerBytes);
}

unsigned int HLSLVariableRegisterCount(const Uniform &variable)
{
    HLSLBlockEncoder encoder(NULL);
    HLSLVariableRegisterCount(variable, &encoder);

    const size_t registerBytes = (encoder.BytesPerComponent * encoder.ComponentsPerRegister);
    return static_cast<unsigned int>(rx::roundUp<size_t>(encoder.getBlockSize(), registerBytes) / registerBytes);
}

}

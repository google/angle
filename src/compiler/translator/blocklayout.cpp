//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// blocklayout.cpp:
//   Implementation for block layout classes and methods.
//

#include "compiler/translator/blocklayout.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace sh
{

namespace
{
bool IsRowMajorLayout(const InterfaceBlockField &var)
{
    return var.isRowMajorLayout;
}

bool IsRowMajorLayout(const ShaderVariable &var)
{
    return false;
}
}  // anonymous namespace

BlockLayoutEncoder::BlockLayoutEncoder() : mCurrentOffset(0)
{
}

BlockMemberInfo BlockLayoutEncoder::encodeType(GLenum type,
                                               unsigned int arraySize,
                                               bool isRowMajorMatrix)
{
    int arrayStride;
    int matrixStride;

    getBlockLayoutInfo(type, arraySize, isRowMajorMatrix, &arrayStride, &matrixStride);

    const BlockMemberInfo memberInfo(static_cast<int>(mCurrentOffset * BytesPerComponent),
                                     static_cast<int>(arrayStride * BytesPerComponent),
                                     static_cast<int>(matrixStride * BytesPerComponent),
                                     isRowMajorMatrix);

    advanceOffset(type, arraySize, isRowMajorMatrix, arrayStride, matrixStride);

    return memberInfo;
}

// static
size_t BlockLayoutEncoder::getBlockRegister(const BlockMemberInfo &info)
{
    return (info.offset / BytesPerComponent) / ComponentsPerRegister;
}

// static
size_t BlockLayoutEncoder::getBlockRegisterElement(const BlockMemberInfo &info)
{
    return (info.offset / BytesPerComponent) % ComponentsPerRegister;
}

void BlockLayoutEncoder::nextRegister()
{
    mCurrentOffset = rx::roundUp<size_t>(mCurrentOffset, ComponentsPerRegister);
}

Std140BlockEncoder::Std140BlockEncoder()
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

void Std140BlockEncoder::getBlockLayoutInfo(GLenum type,
                                            unsigned int arraySize,
                                            bool isRowMajorMatrix,
                                            int *arrayStrideOut,
                                            int *matrixStrideOut)
{
    // We assume we are only dealing with 4 byte components (no doubles or half-words currently)
    ASSERT(gl::VariableComponentSize(gl::VariableComponentType(type)) == BytesPerComponent);

    size_t baseAlignment = 0;
    int matrixStride     = 0;
    int arrayStride      = 0;

    if (gl::IsMatrixType(type))
    {
        baseAlignment = ComponentsPerRegister;
        matrixStride  = ComponentsPerRegister;

        if (arraySize > 0)
        {
            const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
            arrayStride            = ComponentsPerRegister * numRegisters;
        }
    }
    else if (arraySize > 0)
    {
        baseAlignment = ComponentsPerRegister;
        arrayStride   = ComponentsPerRegister;
    }
    else
    {
        const int numComponents = gl::VariableComponentCount(type);
        baseAlignment           = (numComponents == 3 ? 4u : static_cast<size_t>(numComponents));
    }

    mCurrentOffset = rx::roundUp(mCurrentOffset, baseAlignment);

    *matrixStrideOut = matrixStride;
    *arrayStrideOut  = arrayStride;
}

void Std140BlockEncoder::advanceOffset(GLenum type,
                                       unsigned int arraySize,
                                       bool isRowMajorMatrix,
                                       int arrayStride,
                                       int matrixStride)
{
    if (arraySize > 0)
    {
        mCurrentOffset += arrayStride * arraySize;
    }
    else if (gl::IsMatrixType(type))
    {
        ASSERT(matrixStride == ComponentsPerRegister);
        const int numRegisters = gl::MatrixRegisterCount(type, isRowMajorMatrix);
        mCurrentOffset += ComponentsPerRegister * numRegisters;
    }
    else
    {
        mCurrentOffset += gl::VariableComponentCount(type);
    }
}

template <typename VarT>
void GetUniformBlockInfo(const std::vector<VarT> &fields,
                         const std::string &prefix,
                         BlockLayoutEncoder *encoder,
                         bool inRowMajorLayout,
                         BlockLayoutMap *blockLayoutMap)
{
    for (const VarT &field : fields)
    {
        // Skip samplers.
        if (gl::IsSamplerType(field.type))
        {
            continue;
        }

        const std::string &fieldName = (prefix.empty() ? field.name : prefix + "." + field.name);

        if (field.isStruct())
        {
            bool rowMajorLayout = (inRowMajorLayout || IsRowMajorLayout(field));

            for (unsigned int arrayElement = 0; arrayElement < field.elementCount(); arrayElement++)
            {
                encoder->enterAggregateType();

                const std::string uniformElementName =
                    fieldName + (field.isArray() ? ArrayString(arrayElement) : "");
                GetUniformBlockInfo(field.fields, uniformElementName, encoder, rowMajorLayout,
                                    blockLayoutMap);

                encoder->exitAggregateType();
            }
        }
        else
        {
            bool isRowMajorMatrix = (gl::IsMatrixType(field.type) && inRowMajorLayout);
            const BlockMemberInfo &blockMemberInfo =
                encoder->encodeType(field.type, field.arraySize, isRowMajorMatrix);

            (*blockLayoutMap)[fieldName] = blockMemberInfo;
        }
    }
}

template void GetUniformBlockInfo(const std::vector<InterfaceBlockField> &,
                                  const std::string &,
                                  sh::BlockLayoutEncoder *,
                                  bool,
                                  BlockLayoutMap *);

template void GetUniformBlockInfo(const std::vector<Uniform> &,
                                  const std::string &,
                                  sh::BlockLayoutEncoder *,
                                  bool,
                                  BlockLayoutMap *);

template void GetUniformBlockInfo(const std::vector<ShaderVariable> &,
                                  const std::string &,
                                  sh::BlockLayoutEncoder *,
                                  bool,
                                  BlockLayoutMap *);

}  // namespace sh

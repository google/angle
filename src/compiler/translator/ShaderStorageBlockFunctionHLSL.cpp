//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderStorageBlockFunctionHLSL: Wrapper functions for RWByteAddressBuffer Load/Store functions.
//

#include "compiler/translator/ShaderStorageBlockFunctionHLSL.h"

#include "compiler/translator/UtilsHLSL.h"
#include "compiler/translator/blocklayout.h"
#include "compiler/translator/blocklayoutHLSL.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

unsigned int GetMatrixStride(const TType &type)
{
    sh::Std140BlockEncoder std140Encoder;
    sh::HLSLBlockEncoder hlslEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED, false);
    sh::BlockLayoutEncoder *encoder = nullptr;

    if (type.getLayoutQualifier().blockStorage == EbsStd140)
    {
        encoder = &std140Encoder;
    }
    else
    {
        // TODO(jiajia.qin@intel.com): add std430 support. http://anglebug.com/1951
        encoder = &hlslEncoder;
    }
    const bool isRowMajorLayout = (type.getLayoutQualifier().matrixPacking == EmpRowMajor);
    std::vector<unsigned int> arraySizes;
    auto *typeArraySizes = type.getArraySizes();
    if (typeArraySizes != nullptr)
    {
        arraySizes.assign(typeArraySizes->begin(), typeArraySizes->end());
    }
    const BlockMemberInfo &memberInfo =
        encoder->encodeType(GLVariableType(type), arraySizes, isRowMajorLayout);
    return memberInfo.matrixStride;
}

}  // anonymous namespace

// static
void ShaderStorageBlockFunctionHLSL::OutputSSBOLoadFunctionBody(
    TInfoSinkBase &out,
    const ShaderStorageBlockFunction &ssboFunction)
{
    const char *convertString;
    switch (ssboFunction.type.getBasicType())
    {
        case EbtFloat:
            convertString = "asfloat(";
            break;
        case EbtInt:
            convertString = "asint(";
            break;
        case EbtUInt:
            convertString = "asuint(";
            break;
        case EbtBool:
            convertString = "asint(";
            break;
        default:
            UNREACHABLE();
            return;
    }

    out << "    " << ssboFunction.typeString << " result";
    if (ssboFunction.type.isScalar())
    {
        out << " = " << convertString << "buffer.Load(loc));\n";
    }
    else if (ssboFunction.type.isVector())
    {
        out << " = " << convertString << "buffer.Load" << ssboFunction.type.getNominalSize()
            << "(loc));\n";
    }
    else if (ssboFunction.type.isMatrix())
    {
        unsigned int matrixStride = GetMatrixStride(ssboFunction.type);
        out << " = {";
        for (int rowIndex = 0; rowIndex < ssboFunction.type.getRows(); rowIndex++)
        {
            out << "asfloat(buffer.Load" << ssboFunction.type.getCols() << "(loc +"
                << rowIndex * matrixStride << ")), ";
        }

        out << "};\n";
    }
    else
    {
        // TODO(jiajia.qin@intel.com): Process all possible return types. http://anglebug.com/1951
        out << ";\n";
    }

    out << "    return result;\n";
    return;
}

// static
void ShaderStorageBlockFunctionHLSL::OutputSSBOStoreFunctionBody(
    TInfoSinkBase &out,
    const ShaderStorageBlockFunction &ssboFunction)
{
    if (ssboFunction.type.isScalar())
    {
        if (ssboFunction.type.getBasicType() == EbtBool)
        {
            out << "    uint _tmp = uint(value);\n"
                << "    buffer.Store(loc, _tmp);\n";
        }
        else
        {
            out << "    buffer.Store(loc, asuint(value));\n";
        }
    }
    else if (ssboFunction.type.isVector())
    {
        if (ssboFunction.type.getBasicType() == EbtBool)
        {
            out << "    uint" << ssboFunction.type.getNominalSize() << " _tmp = uint"
                << ssboFunction.type.getNominalSize() << "(value);\n";
            out << "    buffer.Store" << ssboFunction.type.getNominalSize() << "(loc, _tmp);\n";
        }
        else
        {
            out << "    buffer.Store" << ssboFunction.type.getNominalSize()
                << "(loc, asuint(value));\n";
        }
    }
    else if (ssboFunction.type.isMatrix())
    {
        unsigned int matrixStride = GetMatrixStride(ssboFunction.type);
        for (int rowIndex = 0; rowIndex < ssboFunction.type.getRows(); rowIndex++)
        {
            out << "    buffer.Store" << ssboFunction.type.getCols() << "(loc +"
                << rowIndex * matrixStride << ", asuint(value[" << rowIndex << "]));\n";
        }
    }
    else
    {
        // TODO(jiajia.qin@intel.com): Process all possible return types. http://anglebug.com/1951
    }
}

bool ShaderStorageBlockFunctionHLSL::ShaderStorageBlockFunction::operator<(
    const ShaderStorageBlockFunction &rhs) const
{
    return std::tie(functionName, typeString, method) <
           std::tie(rhs.functionName, rhs.typeString, rhs.method);
}

TString ShaderStorageBlockFunctionHLSL::registerShaderStorageBlockFunction(const TType &type,
                                                                           SSBOMethod method)
{
    ShaderStorageBlockFunction ssboFunction;
    ssboFunction.typeString = TypeString(type);
    ssboFunction.method     = method;
    ssboFunction.type       = type;

    switch (method)
    {
        case SSBOMethod::LOAD:
            ssboFunction.functionName = ssboFunction.typeString + "_Load";
            break;
        case SSBOMethod::STORE:
            ssboFunction.functionName = ssboFunction.typeString + "_Store";
            break;
        default:
            UNREACHABLE();
    }

    mRegisteredShaderStorageBlockFunctions.insert(ssboFunction);
    return ssboFunction.functionName;
}

void ShaderStorageBlockFunctionHLSL::shaderStorageBlockFunctionHeader(TInfoSinkBase &out)
{
    for (const ShaderStorageBlockFunction &ssboFunction : mRegisteredShaderStorageBlockFunctions)
    {
        if (ssboFunction.method == SSBOMethod::LOAD)
        {
            // Function header
            out << ssboFunction.typeString << " " << ssboFunction.functionName
                << "(RWByteAddressBuffer buffer, uint loc)\n";
            out << "{\n";
            OutputSSBOLoadFunctionBody(out, ssboFunction);
        }
        else
        {
            // Function header
            out << "void " << ssboFunction.functionName << "(RWByteAddressBuffer buffer, uint loc, "
                << ssboFunction.typeString << " value)\n";
            out << "{\n";
            OutputSSBOStoreFunctionBody(out, ssboFunction);
        }

        out << "}\n"
               "\n";
    }
}

}  // namespace sh

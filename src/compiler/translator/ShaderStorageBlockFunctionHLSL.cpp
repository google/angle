//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderStorageBlockFunctionHLSL: Wrapper functions for RWByteAddressBuffer Load/Store functions.
//

#include "compiler/translator/ShaderStorageBlockFunctionHLSL.h"

#include "common/utilities.h"
#include "compiler/translator/UtilsHLSL.h"
#include "compiler/translator/blocklayout.h"
#include "compiler/translator/blocklayoutHLSL.h"
#include "compiler/translator/util.h"

namespace sh
{

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

    size_t bytesPerComponent =
        gl::VariableComponentSize(gl::VariableComponentType(GLVariableType(ssboFunction.type)));
    out << "    " << ssboFunction.typeString << " result";
    if (ssboFunction.type.isScalar())
    {
        size_t offset = ssboFunction.swizzleOffsets[0] * bytesPerComponent;
        out << " = " << convertString << "buffer.Load(loc + " << offset << "));\n ";
    }
    else if (ssboFunction.type.isVector())
    {
        if (ssboFunction.rowMajor || !ssboFunction.isDefaultSwizzle)
        {
            size_t componentStride = bytesPerComponent;
            if (ssboFunction.rowMajor)
            {
                componentStride = ssboFunction.matrixStride;
            }

            out << " = {";
            for (const int offset : ssboFunction.swizzleOffsets)
            {
                size_t offsetInBytes = offset * componentStride;
                out << convertString << "buffer.Load(loc + " << offsetInBytes << ")),";
            }
            out << "};\n";
        }
        else
        {
            out << " = " << convertString << "buffer.Load" << ssboFunction.type.getNominalSize()
                << "(loc));\n";
        }
    }
    else if (ssboFunction.type.isMatrix())
    {
        if (ssboFunction.rowMajor)
        {
            out << ";";
            out << "    float" << ssboFunction.type.getRows() << "x" << ssboFunction.type.getCols()
                << " tmp_ = {";
            for (int rowIndex = 0; rowIndex < ssboFunction.type.getRows(); rowIndex++)
            {
                out << "asfloat(buffer.Load" << ssboFunction.type.getCols() << "(loc + "
                    << rowIndex * ssboFunction.matrixStride << ")), ";
            }
            out << "};\n";
            out << "    result = transpose(tmp_);\n";
        }
        else
        {
            out << " = {";
            for (int columnIndex = 0; columnIndex < ssboFunction.type.getCols(); columnIndex++)
            {
                out << "asfloat(buffer.Load" << ssboFunction.type.getRows() << "(loc + "
                    << columnIndex * ssboFunction.matrixStride << ")), ";
            }
            out << "};\n";
        }
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
    size_t bytesPerComponent =
        gl::VariableComponentSize(gl::VariableComponentType(GLVariableType(ssboFunction.type)));
    if (ssboFunction.type.isScalar())
    {
        size_t offset = ssboFunction.swizzleOffsets[0] * bytesPerComponent;
        if (ssboFunction.type.getBasicType() == EbtBool)
        {
            out << "    buffer.Store(loc + " << offset << ", uint(value));\n";
        }
        else
        {
            out << "    buffer.Store(loc + " << offset << ", asuint(value));\n";
        }
    }
    else if (ssboFunction.type.isVector())
    {
        out << "    uint" << ssboFunction.type.getNominalSize() << " _value;\n";
        if (ssboFunction.type.getBasicType() == EbtBool)
        {
            out << "    _value = uint" << ssboFunction.type.getNominalSize() << "(value);\n";
        }
        else
        {
            out << "    _value = asuint(value);\n";
        }

        if (ssboFunction.rowMajor || !ssboFunction.isDefaultSwizzle)
        {
            size_t componentStride = bytesPerComponent;
            if (ssboFunction.rowMajor)
            {
                componentStride = ssboFunction.matrixStride;
            }
            const TVector<int> &swizzleOffsets = ssboFunction.swizzleOffsets;
            for (int index = 0; index < static_cast<int>(swizzleOffsets.size()); index++)
            {
                size_t offsetInBytes = swizzleOffsets[index] * componentStride;
                out << "buffer.Store(loc + " << offsetInBytes << ", _value[" << index << "]);\n";
            }
        }
        else
        {
            out << "    buffer.Store" << ssboFunction.type.getNominalSize() << "(loc, _value);\n";
        }
    }
    else if (ssboFunction.type.isMatrix())
    {
        if (ssboFunction.rowMajor)
        {
            out << "    float" << ssboFunction.type.getRows() << "x" << ssboFunction.type.getCols()
                << " tmp_ = transpose(value);\n";
            for (int rowIndex = 0; rowIndex < ssboFunction.type.getRows(); rowIndex++)
            {
                out << "    buffer.Store" << ssboFunction.type.getCols() << "(loc + "
                    << rowIndex * ssboFunction.matrixStride << ", asuint(tmp_[" << rowIndex
                    << "]));\n";
            }
        }
        else
        {
            for (int columnIndex = 0; columnIndex < ssboFunction.type.getCols(); columnIndex++)
            {
                out << "    buffer.Store" << ssboFunction.type.getRows() << "(loc + "
                    << columnIndex * ssboFunction.matrixStride << ", asuint(value[" << columnIndex
                    << "]));\n";
            }
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
    return functionName < rhs.functionName;
}

TString ShaderStorageBlockFunctionHLSL::registerShaderStorageBlockFunction(
    const TType &type,
    SSBOMethod method,
    TLayoutBlockStorage storage,
    bool rowMajor,
    int matrixStride,
    TIntermSwizzle *swizzleNode)
{
    ShaderStorageBlockFunction ssboFunction;
    ssboFunction.typeString = TypeString(type);
    ssboFunction.method     = method;
    ssboFunction.type       = type;
    if (swizzleNode != nullptr)
    {
        ssboFunction.swizzleOffsets   = swizzleNode->getSwizzleOffsets();
        ssboFunction.isDefaultSwizzle = false;
    }
    else
    {
        if (ssboFunction.type.getNominalSize() > 1)
        {
            for (int index = 0; index < ssboFunction.type.getNominalSize(); index++)
            {
                ssboFunction.swizzleOffsets.push_back(index);
            }
        }
        else
        {
            ssboFunction.swizzleOffsets.push_back(0);
        }

        ssboFunction.isDefaultSwizzle = true;
    }
    ssboFunction.rowMajor     = rowMajor;
    ssboFunction.matrixStride = matrixStride;
    ssboFunction.functionName =
        TString(getBlockStorageString(storage)) + "_" + ssboFunction.typeString;

    switch (method)
    {
        case SSBOMethod::LOAD:
            ssboFunction.functionName += "_Load";
            break;
        case SSBOMethod::STORE:
            ssboFunction.functionName += "_Store";
            break;
        default:
            UNREACHABLE();
    }

    if (rowMajor)
    {
        ssboFunction.functionName += "_rm_";
    }
    else
    {
        ssboFunction.functionName += "_cm_";
    }

    for (const int offset : ssboFunction.swizzleOffsets)
    {
        switch (offset)
        {
            case 0:
                ssboFunction.functionName += "x";
                break;
            case 1:
                ssboFunction.functionName += "y";
                break;
            case 2:
                ssboFunction.functionName += "z";
                break;
            case 3:
                ssboFunction.functionName += "w";
                break;
            default:
                UNREACHABLE();
        }
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

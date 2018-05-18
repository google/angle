//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderStorageBlockFunctionHLSL: Wrapper functions for RWByteAddressBuffer Load/Store functions.
//

#include "compiler/translator/ShaderStorageBlockFunctionHLSL.h"

#include "compiler/translator/UtilsHLSL.h"

namespace sh
{

// static
void ShaderStorageBlockFunctionHLSL::OutputSSBOLoadFunctionBody(
    TInfoSinkBase &out,
    const ShaderStorageBlockFunction &ssboFunction)
{
    if (ssboFunction.type.isScalar())
    {
        TString convertString;
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
                break;
        }

        out << "    " << ssboFunction.typeString << " result = " << convertString
            << "buffer.Load(loc));\n";
        out << "    return result;\n";
        return;
    }

    // TODO(jiajia.qin@intel.com): Process all possible return types.
    out << "    return 1.0;\n";
}

// static
void ShaderStorageBlockFunctionHLSL::OutputSSBOStoreFunctionBody(
    TInfoSinkBase &out,
    const ShaderStorageBlockFunction &ssboFunction)
{
    if (ssboFunction.type.isScalar())
    {
        out << "    buffer.Store(loc, asuint(value));\n";
    }

    // TODO(jiajia.qin@intel.com): Process all possible return types.
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

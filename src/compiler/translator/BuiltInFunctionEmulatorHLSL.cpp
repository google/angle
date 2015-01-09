//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulatorHLSL.h"
#include "compiler/translator/SymbolTable.h"

BuiltInFunctionEmulatorHLSL::BuiltInFunctionEmulatorHLSL()
    : BuiltInFunctionEmulator()
{
    TType float1(EbtFloat);
    TType float2(EbtFloat, 2);
    TType float3(EbtFloat, 3);
    TType float4(EbtFloat, 4);

    AddEmulatedFunction(EOpAsinh, float1,
        "float webgl_asinh_emu(in float x) {\n"
        "    return log(x + sqrt(pow(x, 2.0) + 1.0));\n"
        "}\n");
    AddEmulatedFunction(EOpAsinh, float2,
        "float2 webgl_asinh_emu(in float2 x) {\n"
        "    return log(x + sqrt(pow(x, 2.0) + 1.0));"
        "}\n");
    AddEmulatedFunction(EOpAsinh, float3,
        "float3 webgl_asinh_emu(in float3 x) {\n"
        "    return log(x + sqrt(pow(x, 2.0) + 1.0));\n"
        "}\n");
    AddEmulatedFunction(EOpAsinh, float4,
        "float4 webgl_asinh_emu(in float4 x) {\n"
        "    return log(x + sqrt(pow(x, 2.0) + 1.0));\n"
        "}\n");

    AddEmulatedFunction(EOpAcosh, float1,
        "float webgl_acosh_emu(in float x) {\n"
        "    return log(x + sqrt(x + 1.0) * sqrt(x - 1.0));\n"
        "}\n");
    AddEmulatedFunction(EOpAcosh, float2,
        "float2 webgl_acosh_emu(in float2 x) {\n"
        "    return log(x + sqrt(x + 1.0) * sqrt(x - 1.0));\n"
        "}\n");
    AddEmulatedFunction(EOpAcosh, float3,
        "float3 webgl_acosh_emu(in float3 x) {\n"
        "    return log(x + sqrt(x + 1.0) * sqrt(x - 1.0));\n"
        "}\n");
    AddEmulatedFunction(EOpAcosh, float4,
        "float4 webgl_acosh_emu(in float4 x) {\n"
        "    return log(x + sqrt(x + 1.0) * sqrt(x - 1.0));\n"
        "}\n");

    AddEmulatedFunction(EOpAtanh, float1,
        "float webgl_atanh_emu(in float x) {\n"
        "    return 0.5 * log((1.0 + x) / (1.0 - x));\n"
        "}\n");
    AddEmulatedFunction(EOpAtanh, float2,
        "float2 webgl_atanh_emu(in float2 x) {\n"
        "    return 0.5 * log((1.0 + x) / (1.0 - x));\n"
        "}\n");
    AddEmulatedFunction(EOpAtanh, float3,
        "float3 webgl_atanh_emu(in float3 x) {\n"
        "    return 0.5 * log((1.0 + x) / (1.0 - x));\n"
        "}\n");
    AddEmulatedFunction(EOpAtanh, float4,
        "float4 webgl_atanh_emu(in float4 x) {\n"
        "    return 0.5 * log((1.0 + x) / (1.0 - x));\n"
        "}\n");
}


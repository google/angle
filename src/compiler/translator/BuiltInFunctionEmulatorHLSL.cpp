//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulatorHLSL.h"
#include "compiler/translator/SymbolTable.h"

namespace
{

const char* kFunctionEmulationSource[] =
{
    "#error no emulation for cos(float)",
    "#error no emulation for cos(vec2)",
    "#error no emulation for cos(vec3)",
    "#error no emulation for cos(vec4)",

    "#error no emulation for distance(float, float)",
    "#error no emulation for distance(vec2, vec2)",
    "#error no emulation for distance(vec3, vec3)",
    "#error no emulation for distance(vec4, vec4)",

    "#error no emulation for dot(float, float)",
    "#error no emulation for dot(vec2, vec2)",
    "#error no emulation for dot(vec3, vec3)",
    "#error no emulation for dot(vec4, vec4)",

    "#error no emulation for length(float)",
    "#error no emulation for length(vec2)",
    "#error no emulation for length(vec3)",
    "#error no emulation for length(vec4)",

    "#error no emulation for normalize(float)",
    "#error no emulation for normalize(vec2)",
    "#error no emulation for normalize(vec3)",
    "#error no emulation for normalize(vec4)",

    "#error no emulation for reflect(float, float)",
    "#error no emulation for reflect(vec2, vec2)",
    "#error no emulation for reflect(vec3, vec3)",
    "#error no emulation for reflect(vec4, vec4)",

    "float webgl_asinh_emu(in float x) { return log(x + sqrt(pow(x, 2.0) + 1.0)); }",
    "float2 webgl_asinh_emu(in float2 x) { return log(x + sqrt(pow(x, 2.0) + 1.0)); }",
    "float3 webgl_asinh_emu(in float3 x) { return log(x + sqrt(pow(x, 2.0) + 1.0)); }",
    "float4 webgl_asinh_emu(in float4 x) { return log(x + sqrt(pow(x, 2.0) + 1.0)); }",

    "float webgl_acosh_emu(in float x) { return log(x + sqrt(x + 1.0) * sqrt(x - 1.0)); }",
    "float2 webgl_acosh_emu(in float2 x) { return log(x + sqrt(x + 1.0) * sqrt(x - 1.0)); }",
    "float3 webgl_acosh_emu(in float3 x) { return log(x + sqrt(x + 1.0) * sqrt(x - 1.0)); }",
    "float4 webgl_acosh_emu(in float4 x) { return log(x + sqrt(x + 1.0) * sqrt(x - 1.0)); }",

    "float webgl_atanh_emu(in float x) { return 0.5 * log((1.0 + x) / (1.0 - x)); }",
    "float2 webgl_atanh_emu(in float2 x) { return 0.5 * log((1.0 + x) / (1.0 - x)); }",
    "float3 webgl_atanh_emu(in float3 x) { return 0.5 * log((1.0 + x) / (1.0 - x)); }",
    "float4 webgl_atanh_emu(in float4 x) { return 0.5 * log((1.0 + x) / (1.0 - x)); }"
};

const bool kFunctionEmulationMask[] =
{
    false, // TFunctionCos1
    false, // TFunctionCos2
    false, // TFunctionCos3
    false, // TFunctionCos4
    false, // TFunctionDistance1_1
    false, // TFunctionDistance2_2
    false, // TFunctionDistance3_3
    false, // TFunctionDistance4_4
    false, // TFunctionDot1_1
    false, // TFunctionDot2_2
    false, // TFunctionDot3_3
    false, // TFunctionDot4_4
    false, // TFunctionLength1
    false, // TFunctionLength2
    false, // TFunctionLength3
    false, // TFunctionLength4
    false, // TFunctionNormalize1
    false, // TFunctionNormalize2
    false, // TFunctionNormalize3
    false, // TFunctionNormalize4
    false, // TFunctionReflect1_1
    false, // TFunctionReflect2_2
    false, // TFunctionReflect3_3
    false, // TFunctionReflect4_4
    true, // TFunctionAsinh1
    true, // TFunctionAsinh2
    true, // TFunctionAsinh3
    true, // TFunctionAsinh4
    true, // TFunctionAcosh1
    true, // TFunctionAcosh2
    true, // TFunctionAcosh3
    true, // TFunctionAcosh4
    true, // TFunctionAtanh1
    true, // TFunctionAtanh2
    true, // TFunctionAtanh3
    true, // TFunctionAtanh4
    false  // TFunctionUnknown
};

}  // anonymous namepsace

BuiltInFunctionEmulatorHLSL::BuiltInFunctionEmulatorHLSL()
    : BuiltInFunctionEmulator()
{
    mFunctionMask = kFunctionEmulationMask;
    mFunctionSource = kFunctionEmulationSource;
}


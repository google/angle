//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/BuiltInFunctionEmulatorGLSL.h"
#include "compiler/translator/SymbolTable.h"

void InitBuiltInFunctionEmulatorForGLSL(BuiltInFunctionEmulator *emu, sh::GLenum shaderType)
{
    // we use macros here instead of function definitions to work around more GLSL
    // compiler bugs, in particular on NVIDIA hardware on Mac OSX. Macros are
    // problematic because if the argument has side-effects they will be repeatedly
    // evaluated. This is unlikely to show up in real shaders, but is something to
    // consider.

    TType *float1 = new TType(EbtFloat);
    TType *float2 = new TType(EbtFloat, 2);
    TType *float3 = new TType(EbtFloat, 3);
    TType *float4 = new TType(EbtFloat, 4);

    if (shaderType == GL_FRAGMENT_SHADER)
    {
        emu->addEmulatedFunction(EOpCos, float1, "webgl_emu_precision float webgl_cos_emu(webgl_emu_precision float a) { return cos(a); }");
        emu->addEmulatedFunction(EOpCos, float2, "webgl_emu_precision vec2 webgl_cos_emu(webgl_emu_precision vec2 a) { return cos(a); }");
        emu->addEmulatedFunction(EOpCos, float3, "webgl_emu_precision vec3 webgl_cos_emu(webgl_emu_precision vec3 a) { return cos(a); }");
        emu->addEmulatedFunction(EOpCos, float4, "webgl_emu_precision vec4 webgl_cos_emu(webgl_emu_precision vec4 a) { return cos(a); }");
    }
    emu->addEmulatedFunction(EOpDistance, float1, float1, "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))");
    emu->addEmulatedFunction(EOpDot, float1, float1, "#define webgl_dot_emu(x, y) ((x) * (y))");
    emu->addEmulatedFunction(EOpLength, float1, "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))");
    emu->addEmulatedFunction(EOpNormalize, float1, "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))");
    emu->addEmulatedFunction(EOpReflect, float1, float1, "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))");
}

// emulate built-in functions missing from OpenGL 4.1
void InitBuiltInFunctionEmulatorForGLSL4_1(BuiltInFunctionEmulator *emu, sh::GLenum shaderType)
{
    TType *float2 = new TType(EbtFloat, 2);
    TType *uint1  = new TType(EbtUInt);

    emu->addEmulatedFunction(EOpPackSnorm2x16, float2,
        "uint webgl_packSnorm2x16_emu(vec2 v){\n"
        "    int x = int(round(clamp(v.x, -1.0, 1.0) * 32767.0));\n"
        "    int y = int(round(clamp(v.y, -1.0, 1.0) * 32767.0));\n"
        "    return uint((y << 16) | (x & 0xffff));\n"
        "}\n");
    emu->addEmulatedFunction(EOpUnpackSnorm2x16, uint1,
        "float webgl_fromSnorm(uint x){\n"
        "    int xi = (int(x) & 0x7fff) - (int(x) & 0x8000);\n"
        "    return clamp(float(xi) / 32767.0, -1.0, 1.0);\n"
        "}\n"
        "vec2 webgl_unpackSnorm2x16_emu(uint u){\n"
        "    uint y = (u >> 16);\n"
        "    uint x = u;\n"
        "    return vec2(webgl_fromSnorm(x), webgl_fromSnorm(y));\n"
        "}\n");
    // Functions uint webgl_f32tof16(float val) and float webgl_f16tof32(uint val) are
    // based on the OpenGL redbook Appendix Session "Floating-Point Formats Used in OpenGL".
    emu->addEmulatedFunction(EOpPackHalf2x16, float2,
        "uint webgl_f32tof16(float val){\n"
        "    uint f32 = floatBitsToInt(val);\n"
        "    uint f16 = 0;\n"
        "    uint sign = (f32 >> 16) & 0x8000u;\n"
        "    int exponent = int((f32 >> 23) & 0xff) - 127;\n"
        "    uint mantissa = f32 & 0x007fffffu;\n"
        "    if (exponent == 128) { /* Infinity or NaN */\n"
        "        // NaN bits that are masked out by 0x3ff get discarded. This can turn some NaNs to infinity, but this is allowed by the spec.\n"
        "        f16 = sign | (0x1F << 10); f16 |= (mantissa & 0x3ff);\n"
        "    }\n"
        "        else if (exponent > 15) { /* Overflow - flush to Infinity */ f16 = sign | (0x1F << 10); }\n"
        "        else if (exponent > -15) { /* Representable value */ exponent += 15; mantissa >>= 13; f16 = sign | exponent << 10 | mantissa; }\n"
        "        else { f16 = sign; }\n"
        "    return f16;\n"
        "}\n"
        "uint webgl_packHalf2x16_emu(vec2 v){\n"
        "    uint x = webgl_f32tof16(v.x);\n"
        "    uint y = webgl_f32tof16(v.y);\n"
        "    return (y << 16) | x;\n"
        "}\n");
    emu->addEmulatedFunction(EOpUnpackHalf2x16, uint1,
        "float webgl_f16tof32(uint val){\n"
        "    uint sign = (val & 0x8000u) << 16;\n"
        "    int exponent = int((val & 0x7c00) >> 10);\n"
        "    uint mantissa = val & 0x03ffu;\n"
        "    float f32 = 0.0;\n"
        "    if(exponent == 0) { if (mantissa != 0) { const float scale = 1.0 / (1 << 24); f32 = scale * mantissa; } }\n"
        "        else if (exponent == 31) { return uintBitsToFloat(sign | 0x7f800000 | mantissa); }\n"
        "        else{\n"
        "             float scale, decimal; exponent -= 15;\n"
        "             if(exponent < 0) { scale = 1.0 / (1 << -exponent); }\n"
        "                 else { scale = 1 << exponent; }\n"
        "             decimal = 1.0 + float(mantissa) / float(1 << 10);\n"
        "             f32 = scale * decimal;\n"
        "        }\n"
        "    if (sign != 0) f32 = -f32;\n"
        "    return f32;\n"
        "}\n"
        "vec2 webgl_unpackHalf2x16_emu(uint u){\n"
        "    uint y = (u >> 16);\n"
        "    uint x = u & 0xffffu;\n"
        "    return vec2(webgl_f16tof32(x), webgl_f16tof32(y));\n"
        "}\n");
}
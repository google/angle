//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulatorGLSL.h"
#include "compiler/translator/SymbolTable.h"

namespace
{

// we use macros here instead of function definitions to work around more GLSL
// compiler bugs, in particular on NVIDIA hardware on Mac OSX. Macros are
// problematic because if the argument has side-effects they will be repeatedly
// evaluated. This is unlikely to show up in real shaders, but is something to
// consider.
const char* kFunctionEmulationVertexSource[] =
{
    "#error no emulation for cos(float)",
    "#error no emulation for cos(vec2)",
    "#error no emulation for cos(vec3)",
    "#error no emulation for cos(vec4)",

    "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))",
    "#error no emulation for distance(vec2, vec2)",
    "#error no emulation for distance(vec3, vec3)",
    "#error no emulation for distance(vec4, vec4)",

    "#define webgl_dot_emu(x, y) ((x) * (y))",
    "#error no emulation for dot(vec2, vec2)",
    "#error no emulation for dot(vec3, vec3)",
    "#error no emulation for dot(vec4, vec4)",

    "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))",
    "#error no emulation for length(vec2)",
    "#error no emulation for length(vec3)",
    "#error no emulation for length(vec4)",

    "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))",
    "#error no emulation for normalize(vec2)",
    "#error no emulation for normalize(vec3)",
    "#error no emulation for normalize(vec4)",

    "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))",
    "#error no emulation for reflect(vec2, vec2)",
    "#error no emulation for reflect(vec3, vec3)",
    "#error no emulation for reflect(vec4, vec4)",

    "#error no emulation for asinh(float)",
    "#error no emulation for asinh(vec2)",
    "#error no emulation for asinh(vec3)",
    "#error no emulation for asinh(vec4)",

    "#error no emulation for acosh(float)",
    "#error no emulation for acosh(vec2)",
    "#error no emulation for acosh(vec3)",
    "#error no emulation for acosh(vec4)",

    "#error no emulation for atanh(float)",
    "#error no emulation for atanh(vec2)",
    "#error no emulation for atanh(vec3)",
    "#error no emulation for atanh(vec4)"
};

const char* kFunctionEmulationFragmentSource[] =
{
    "webgl_emu_precision float webgl_cos_emu(webgl_emu_precision float a) { return cos(a); }",
    "webgl_emu_precision vec2 webgl_cos_emu(webgl_emu_precision vec2 a) { return cos(a); }",
    "webgl_emu_precision vec3 webgl_cos_emu(webgl_emu_precision vec3 a) { return cos(a); }",
    "webgl_emu_precision vec4 webgl_cos_emu(webgl_emu_precision vec4 a) { return cos(a); }",

    "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))",
    "#error no emulation for distance(vec2, vec2)",
    "#error no emulation for distance(vec3, vec3)",
    "#error no emulation for distance(vec4, vec4)",

    "#define webgl_dot_emu(x, y) ((x) * (y))",
    "#error no emulation for dot(vec2, vec2)",
    "#error no emulation for dot(vec3, vec3)",
    "#error no emulation for dot(vec4, vec4)",

    "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))",
    "#error no emulation for length(vec2)",
    "#error no emulation for length(vec3)",
    "#error no emulation for length(vec4)",

    "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))",
    "#error no emulation for normalize(vec2)",
    "#error no emulation for normalize(vec3)",
    "#error no emulation for normalize(vec4)",

    "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))",
    "#error no emulation for reflect(vec2, vec2)",
    "#error no emulation for reflect(vec3, vec3)",
    "#error no emulation for reflect(vec4, vec4)"

    "#error no emulation for asinh(float)",
    "#error no emulation for asinh(vec2)",
    "#error no emulation for asinh(vec3)",
    "#error no emulation for asinh(vec4)",

    "#error no emulation for acosh(float)",
    "#error no emulation for acosh(vec2)",
    "#error no emulation for acosh(vec3)",
    "#error no emulation for acosh(vec4)",

    "#error no emulation for atanh(float)",
    "#error no emulation for atanh(vec2)",
    "#error no emulation for atanh(vec3)",
    "#error no emulation for atanh(vec4)"
};

const bool kFunctionEmulationVertexMask[] =
{
#if defined(__APPLE__)
    // Work around ATI driver bugs in Mac.
    false, // TFunctionCos1
    false, // TFunctionCos2
    false, // TFunctionCos3
    false, // TFunctionCos4
    true,  // TFunctionDistance1_1
    false, // TFunctionDistance2_2
    false, // TFunctionDistance3_3
    false, // TFunctionDistance4_4
    true,  // TFunctionDot1_1
    false, // TFunctionDot2_2
    false, // TFunctionDot3_3
    false, // TFunctionDot4_4
    true,  // TFunctionLength1
    false, // TFunctionLength2
    false, // TFunctionLength3
    false, // TFunctionLength4
    true,  // TFunctionNormalize1
    false, // TFunctionNormalize2
    false, // TFunctionNormalize3
    false, // TFunctionNormalize4
    true,  // TFunctionReflect1_1
    false, // TFunctionReflect2_2
    false, // TFunctionReflect3_3
    false, // TFunctionReflect4_4
#else
    // Work around D3D driver bug in Win.
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
#endif
    false, // TFunctionAsinh1
    false, // TFunctionAsinh2
    false, // TFunctionAsinh3
    false, // TFunctionAsinh4
    false, // TFunctionAcosh1
    false, // TFunctionAcosh2
    false, // TFunctionAcosh3
    false, // TFunctionAcosh4
    false, // TFunctionAtanh1
    false, // TFunctionAtanh2
    false, // TFunctionAtanh3
    false, // TFunctionAtanh4
    false  // TFunctionUnknown
};

const bool kFunctionEmulationFragmentMask[] =
{
#if defined(__APPLE__)
    // Work around ATI driver bugs in Mac.
    true,  // TFunctionCos1
    true,  // TFunctionCos2
    true,  // TFunctionCos3
    true,  // TFunctionCos4
    true,  // TFunctionDistance1_1
    false, // TFunctionDistance2_2
    false, // TFunctionDistance3_3
    false, // TFunctionDistance4_4
    true,  // TFunctionDot1_1
    false, // TFunctionDot2_2
    false, // TFunctionDot3_3
    false, // TFunctionDot4_4
    true,  // TFunctionLength1
    false, // TFunctionLength2
    false, // TFunctionLength3
    false, // TFunctionLength4
    true,  // TFunctionNormalize1
    false, // TFunctionNormalize2
    false, // TFunctionNormalize3
    false, // TFunctionNormalize4
    true,  // TFunctionReflect1_1
    false, // TFunctionReflect2_2
    false, // TFunctionReflect3_3
    false, // TFunctionReflect4_4
#else
    // Work around D3D driver bug in Win.
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
#endif
    false, // TFunctionAsinh1
    false, // TFunctionAsinh2
    false, // TFunctionAsinh3
    false, // TFunctionAsinh4
    false, // TFunctionAcosh1
    false, // TFunctionAcosh2
    false, // TFunctionAcosh3
    false, // TFunctionAcosh4
    false, // TFunctionAtanh1
    false, // TFunctionAtanh2
    false, // TFunctionAtanh3
    false, // TFunctionAtanh4
    false  // TFunctionUnknown
};

}  // anonymous namepsace

BuiltInFunctionEmulatorGLSL::BuiltInFunctionEmulatorGLSL(sh::GLenum shaderType)
    : BuiltInFunctionEmulator()
{
    if (shaderType == GL_FRAGMENT_SHADER) {
        mFunctionMask = kFunctionEmulationFragmentMask;
        mFunctionSource = kFunctionEmulationFragmentSource;
    } else {
        mFunctionMask = kFunctionEmulationVertexMask;
        mFunctionSource = kFunctionEmulationVertexSource;
    }
}

void BuiltInFunctionEmulatorGLSL::OutputEmulatedFunctionHeader(
    TInfoSinkBase& out, bool withPrecision) const
{
    if (withPrecision) {
        out << "#if defined(GL_FRAGMENT_PRECISION_HIGH)\n"
            << "#define webgl_emu_precision highp\n"
            << "#else\n"
            << "#define webgl_emu_precision mediump\n"
            << "#endif\n\n";
    } else {
        out << "#define webgl_emu_precision\n\n";
    }
}

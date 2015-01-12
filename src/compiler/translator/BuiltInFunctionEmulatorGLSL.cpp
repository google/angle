//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulatorGLSL.h"
#include "compiler/translator/SymbolTable.h"

BuiltInFunctionEmulatorGLSL::BuiltInFunctionEmulatorGLSL(sh::GLenum shaderType)
    : BuiltInFunctionEmulator()
{
#if defined(__APPLE__)
    // we use macros here instead of function definitions to work around more GLSL
    // compiler bugs, in particular on NVIDIA hardware on Mac OSX. Macros are
    // problematic because if the argument has side-effects they will be repeatedly
    // evaluated. This is unlikely to show up in real shaders, but is something to
    // consider.

    TType float1(EbtFloat);
    TType float2(EbtFloat, 2);
    TType float3(EbtFloat, 3);
    TType float4(EbtFloat, 4);

    if (shaderType == GL_FRAGMENT_SHADER)
    {
        AddEmulatedFunction(EOpCos, float1, "webgl_emu_precision float webgl_cos_emu(webgl_emu_precision float a) { return cos(a); }");
        AddEmulatedFunction(EOpCos, float2, "webgl_emu_precision vec2 webgl_cos_emu(webgl_emu_precision vec2 a) { return cos(a); }");
        AddEmulatedFunction(EOpCos, float3, "webgl_emu_precision vec3 webgl_cos_emu(webgl_emu_precision vec3 a) { return cos(a); }");
        AddEmulatedFunction(EOpCos, float4, "webgl_emu_precision vec4 webgl_cos_emu(webgl_emu_precision vec4 a) { return cos(a); }");
    }
    AddEmulatedFunction(EOpDistance, float1, float1, "#define webgl_distance_emu(x, y) ((x) >= (y) ? (x) - (y) : (y) - (x))");
    AddEmulatedFunction(EOpDot, float1, float1, "#define webgl_dot_emu(x, y) ((x) * (y))");
    AddEmulatedFunction(EOpLength, float1, "#define webgl_length_emu(x) ((x) >= 0.0 ? (x) : -(x))");
    AddEmulatedFunction(EOpNormalize, float1, "#define webgl_normalize_emu(x) ((x) == 0.0 ? 0.0 : ((x) > 0.0 ? 1.0 : -1.0))");
    AddEmulatedFunction(EOpReflect, float1, float1, "#define webgl_reflect_emu(I, N) ((I) - 2.0 * (N) * (I) * (N))");
#endif
}

void BuiltInFunctionEmulatorGLSL::OutputEmulatedFunctionDefinition(
    TInfoSinkBase& out, bool withPrecision) const
{
    if (IsOutputEmpty())
        return;

    out << "// BEGIN: Generated code for built-in function emulation\n\n";
    if (withPrecision) {
        out << "#if defined(GL_FRAGMENT_PRECISION_HIGH)\n"
            << "#define webgl_emu_precision highp\n"
            << "#else\n"
            << "#define webgl_emu_precision mediump\n"
            << "#endif\n\n";
    } else {
        out << "#define webgl_emu_precision\n\n";
    }
    OutputEmulatedFunctions(out);
    out << "// END: Generated code for built-in function emulation\n\n";
}

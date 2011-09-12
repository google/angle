//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_
#define COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/InfoSink.h"
#include "compiler/intermediate.h"

//
// Built-in function groups.  We only list the ones that might need to be
// emulated in certain os/drivers, assuming they are no more than 32.
//
enum TBuiltInFunctionGroup {
    TFunctionGroupAbs            = 1 << 0,  // NVIDIA Win/Mac
    TFunctionGroupAtan           = 1 << 1,  // NVIDIA Win/Mac
    TFunctionGroupCos            = 1 << 2,  // ATI Mac
    TFunctionGroupMod            = 1 << 3,  // NVIDIA Win/Mac
    TFunctionGroupSign           = 1 << 4,  // NVIDIA Win/Mac
    TFunctionGroupAll            = TFunctionGroupAbs |
                                   TFunctionGroupAtan |
                                   TFunctionGroupCos |
                                   TFunctionGroupMod |
                                   TFunctionGroupSign
};

//
// This class decides which built-in functions need to be replaced with the
// emulated ones.
// It's only a workaround for OpenGL driver bugs, and isn't needed in general.
//
class BuiltInFunctionEmulator {
public:
    BuiltInFunctionEmulator(ShShaderType shaderType);

    // functionGroupMask is a bitmap of TBuiltInFunctionGroup.
    // We only emulate functions that are marked by this mask and are actually
    // called in a given shader.
    // By default the value is TFunctionGroupAll.
    void SetFunctionGroupMask(unsigned int functionGroupMask);

    // Records that a function is called by the shader and might needs to be
    // emulated.  If the function's group is not in mFunctionGroupFilter, this
    // becomes an no-op.
    // Returns true if the function call needs to be replaced with an emulated
    // one.
    bool SetFunctionCalled(TOperator op, const TType& param);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2);

    // Output function emulation definition.  This should be before any other
    // shader source.
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    // "name(" becomes "webgl_name_emu(".
    static TString GetEmulatedFunctionName(const TString& name);

private:
    //
    // Built-in functions.
    //
    enum TBuiltInFunction {
        TFunctionAbs1 = 0,  // float abs(float);
        TFunctionAbs2,  // vec2 abs(vec2);
        TFunctionAbs3,  // vec3 abs(vec3);
        TFunctionAbs4,  // vec4 abs(vec4);

        TFunctionAtan1,  // float atan(float);
        TFunctionAtan2,  // vec2 atan(vec2);
        TFunctionAtan3,  // vec3 atan(vec3);
        TFunctionAtan4,  // vec4 atan(vec4);
        TFunctionAtan1_1,  // float atan(float, float);
        TFunctionAtan2_2,  // vec2 atan(vec2, vec2);
        TFunctionAtan3_3,  // vec3 atan(vec3, vec2);
        TFunctionAtan4_4,  // vec4 atan(vec4, vec2);

        TFunctionCos1,  // float cos(float);
        TFunctionCos2,  // vec2 cos(vec2);
        TFunctionCos3,  // vec3 cos(vec3);
        TFunctionCos4,  // vec4 cos(vec4);

        TFunctionMod1_1,  // float mod(float, float);
        TFunctionMod2_2,  // vec2 mod(vec2, vec2);
        TFunctionMod3_3,  // vec3 mod(vec3, vec3);
        TFunctionMod4_4,  // vec4 mod(vec4, vec4);

        TFunctionSign1,  // float sign(float);
        TFunctionSign2,  // vec2 sign(vec2);
        TFunctionSign3,  // vec3 sign(vec3);
        TFunctionSign4,  // vec4 sign(vec4);
        TFunctionUnknown
    };

    TBuiltInFunction IdentifyFunction(TOperator op, const TType& param);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2);

    bool SetFunctionCalled(TBuiltInFunction function);

    TVector<TBuiltInFunction> mFunctions;
    unsigned int mFunctionGroupMask;  // a bitmap of TBuiltInFunctionGroup.

    const bool* mFunctionMask;  // a boolean flag for each function.
};

#endif  // COMPILIER_BUILT_IN_FUNCTION_EMULATOR_H_

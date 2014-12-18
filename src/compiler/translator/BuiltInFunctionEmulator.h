//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATOR_H_
#define COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATOR_H_

#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"

//
// This class decides which built-in functions need to be replaced with the
// emulated ones.
// It can be used to work around driver bugs or implement functions that are
// not natively implemented on a specific platform.
//
class BuiltInFunctionEmulator
{
  public:
    // Records that a function is called by the shader and might need to be
    // emulated. If the function is not in mFunctionMask, this becomes an no-op.
    // Returns true if the function call needs to be replaced with an emulated
    // one.
    bool SetFunctionCalled(TOperator op, const TType& param);
    bool SetFunctionCalled(
        TOperator op, const TType& param1, const TType& param2);

    // Output function emulation definition. This should be before any other
    // shader source.
    void OutputEmulatedFunctionDefinition(TInfoSinkBase& out, bool withPrecision) const;

    void MarkBuiltInFunctionsForEmulation(TIntermNode* root);

    void Cleanup();

    // "name(" becomes "webgl_name_emu(".
    static TString GetEmulatedFunctionName(const TString& name);

  protected:
    BuiltInFunctionEmulator();

    //
    // Built-in functions.
    //
    enum TBuiltInFunction {
        TFunctionCos1 = 0,  // float cos(float);
        TFunctionCos2,  // vec2 cos(vec2);
        TFunctionCos3,  // vec3 cos(vec3);
        TFunctionCos4,  // vec4 cos(vec4);

        TFunctionDistance1_1,  // float distance(float, float);
        TFunctionDistance2_2,  // vec2 distance(vec2, vec2);
        TFunctionDistance3_3,  // vec3 distance(vec3, vec3);
        TFunctionDistance4_4,  // vec4 distance(vec4, vec4);

        TFunctionDot1_1,  // float dot(float, float);
        TFunctionDot2_2,  // vec2 dot(vec2, vec2);
        TFunctionDot3_3,  // vec3 dot(vec3, vec3);
        TFunctionDot4_4,  // vec4 dot(vec4, vec4);

        TFunctionLength1,  // float length(float);
        TFunctionLength2,  // float length(vec2);
        TFunctionLength3,  // float length(vec3);
        TFunctionLength4,  // float length(vec4);

        TFunctionNormalize1,  // float normalize(float);
        TFunctionNormalize2,  // vec2 normalize(vec2);
        TFunctionNormalize3,  // vec3 normalize(vec3);
        TFunctionNormalize4,  // vec4 normalize(vec4);

        TFunctionReflect1_1,  // float reflect(float, float);
        TFunctionReflect2_2,  // vec2 reflect(vec2, vec2);
        TFunctionReflect3_3,  // vec3 reflect(vec3, vec3);
        TFunctionReflect4_4,  // vec4 reflect(vec4, vec4);

        TFunctionAsinh1,  // float asinh(float);
        TFunctionAsinh2,  // vec2 asinh(vec2);
        TFunctionAsinh3,  // vec3 asinh(vec3);
        TFunctionAsinh4,  // vec4 asinh(vec4);

        TFunctionAcosh1,  // float acosh(float);
        TFunctionAcosh2,  // vec2 acosh(vec2);
        TFunctionAcosh3,  // vec3 acosh(vec3);
        TFunctionAcosh4,  // vec4 acosh(vec4);

        TFunctionAtanh1,  // float atanh(float);
        TFunctionAtanh2,  // vec2 atanh(vec2);
        TFunctionAtanh3,  // vec3 atanh(vec3);
        TFunctionAtanh4,  // vec4 atanh(vec4);

        TFunctionUnknown
    };

    std::vector<TBuiltInFunction> mFunctions;

    const bool* mFunctionMask;  // a boolean flag for each function.
    const char** mFunctionSource;

    // Override this to add source before emulated function definitions.
    virtual void OutputEmulatedFunctionHeader(TInfoSinkBase& out, bool withPrecision) const {}

  private:
    TBuiltInFunction IdentifyFunction(TOperator op, const TType& param);
    TBuiltInFunction IdentifyFunction(
        TOperator op, const TType& param1, const TType& param2);

    bool SetFunctionCalled(TBuiltInFunction function);
};

#endif  // COMPILER_TRANSLATOR_BUILTINFUNCTIONEMULATOR_H_

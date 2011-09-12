//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/BuiltInFunctionEmulator.h"

#include "compiler/SymbolTable.h"

namespace {

const char* kFunctionEmulationSource[] = {
    "float webgl_abs_emu(float a) { float rt = abs(a); if (rt < 0.0) rt = 0.0;  return rt; }",
    "vec2 webgl_abs_emu(vec2 a) { vec2 rt = abs(a); if (rt[0] < 0.0) rt[0] = 0.0;  return rt; }",
    "vec3 webgl_abs_emu(vec3 a) { vec3 rt = abs(a); if (rt[0] < 0.0) rt[0] = 0.0;  return rt; }",
    "vec4 webgl_abs_emu(vec4 a) { vec4 rt = abs(a); if (rt[0] < 0.0) rt[0] = 0.0;  return rt; }",

    "float webgl_atan_emu(float y, float x) { float rt = atan(y, x); if (rt > 2.0) rt = 0.0;  return rt; }",
    "vec2 webgl_atan_emu(vec2 y, vec2 x) { vec2 rt = atan(y, x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",
    "vec3 webgl_atan_emu(vec3 y, vec3 x) { vec3 rt = atan(y, x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",
    "vec4 webgl_atan_emu(vec4 y, vec4 x) { vec4 rt = atan(y, x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",
    "float webgl_atan_emu(float y_over_x) { float rt = atan(y_over_x); if (rt > 2.0) rt = 0.0;  return rt; }",
    "vec2 webgl_atan_emu(vec2 y_over_x) { vec2 rt = atan(y_over_x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",
    "vec3 webgl_atan_emu(vec3 y_over_x) { vec3 rt = atan(y_over_x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",
    "vec4 webgl_atan_emu(vec4 y_over_x) { vec4 rt = atan(y_over_x); if (rt[0] > 2.0) rt[0] = 0.0;  return rt; }",

    "float webgl_cos_emu(float a) { return cos(a); }",
    "vec2 webgl_cos_emu(vec2 a) { return cos(a); }",
    "vec3 webgl_cos_emu(vec3 a) { return cos(a); }",
    "vec4 webgl_cos_emu(vec4 a) { return cos(a); }",

    "float webgl_mod_emu(float x, float y) { float rt = mod(x, y); if (rt > x) rt = 0.0;  return rt; }",
    "vec2 webgl_mod_emu(vec2 x, vec2 y) { vec2 rt = mod(x, y); if (rt[0] > x[0]) rt[0] = 0.0;  return rt; }",
    "vec3 webgl_mod_emu(vec3 x, vec3 y) { vec3 rt = mod(x, y); if (rt[0] > x[0]) rt[0] = 0.0;  return rt; }",
    "vec4 webgl_mod_emu(vec4 x, vec4 y) { vec4 rt = mod(x, y); if (rt[0] > x[0]) rt[0] = 0.0;  return rt; }",

    "float webgl_sign_emu(float a) { float rt = sign(a); if (rt > 1.0) rt = 1.0;  return rt; }",
    "vec2 webgl_sign_emu(vec2 a) { float rt = sign(a); if (rt[0] > 1.0) rt[0] = 1.0;  return rt; }",
    "vec3 webgl_sign_emu(vec3 a) { float rt = sign(a); if (rt[0] > 1.0) rt[0] = 1.0;  return rt; }",
    "vec4 webgl_sign_emu(vec4 a) { float rt = sign(a); if (rt[0] > 1.0) rt[0] = 1.0;  return rt; }",
};

const bool kFunctionEmulationVertexMask[] = {
    true,  // TFunctionAbs1
    false, // TFunctionAbs2
    false, // TFunctionAbs3
    false, // TFunctionAbs4

    true,  // TFunctionAtan1
    false, // TFunctionAtan2
    false, // TFunctionAtan3
    false, // TFunctionAtan4
    false, // TFunctionAtan1_1
    true,  // TFunctionAtan2_2
    true,  // TFunctionAtan3_3
    true,  // TFunctionAtan4_4

    false, // TFunctionCos1
    false, // TFunctionCos2
    false, // TFunctionCos3
    false, // TFunctionCos4

    false, // TFunctionMod1_1
    true,  // TFunctionMod2_2
    true,  // TFunctionMod3_3
    true,  // TFunctionMod4_4

    true,  // TFunctionSign1
    false, // TFunctionSign2
    false, // TFunctionSign3
    false, // TFunctionSign4

    false  // TFunctionUnknown
};

const bool kFunctionEmulationFragmentMask[] = {
    false, // TFunctionAbs1
    false, // TFunctionAbs2
    false, // TFunctionAbs3
    false, // TFunctionAbs4

    false, // TFunctionAtan1
    false, // TFunctionAtan2
    false, // TFunctionAtan3
    false, // TFunctionAtan4
    false, // TFunctionAtan1_1
    false, // TFunctionAtan2_2
    false, // TFunctionAtan3_3
    false, // TFunctionAtan4_4

#if defined(__APPLE__)
    // Work around a ATI driver bug in Mac that causes crashes.
    true,  // TFunctionCos1
    true,  // TFunctionCos2
    true,  // TFunctionCos3
    true,  // TFunctionCos4
#else
    false, // TFunctionCos1
    false, // TFunctionCos2
    false, // TFunctionCos3
    false, // TFunctionCos4
#endif

    false, // TFunctionMod1_1
    false, // TFunctionMod2_2
    false, // TFunctionMod3_3
    false, // TFunctionMod4_4

    false, // TFunctionSign1
    false, // TFunctionSign2
    false, // TFunctionSign3
    false, // TFunctionSign4

    false  // TFunctionUnknown
};

class BuiltInFunctionEmulationMarker : public TIntermTraverser {
public:
    BuiltInFunctionEmulationMarker(BuiltInFunctionEmulator& emulator)
        : mEmulator(emulator)
    {
    }

    virtual bool visitUnary(Visit visit, TIntermUnary* node)
    {
        if (visit == PreVisit) {
            bool needToEmulate = mEmulator.SetFunctionCalled(
                node->getOp(), node->getOperand()->getType());
            if (needToEmulate)
                node->setUseEmulatedFunction();
        }
        return true;
    }

    virtual bool visitAggregate(Visit visit, TIntermAggregate* node)
    {
        if (visit == PreVisit) {
            // Here we handle all the built-in functions instead of the ones we
            // currently identified as problematic.
            switch (node->getOp()) {
                case EOpLessThan:
                case EOpGreaterThan:
                case EOpLessThanEqual:
                case EOpGreaterThanEqual:
                case EOpVectorEqual:
                case EOpVectorNotEqual:
                case EOpMod:
                case EOpPow:
                case EOpAtan:
                case EOpMin:
                case EOpMax:
                case EOpClamp:
                case EOpMix:
                case EOpStep:
                case EOpSmoothStep:
                case EOpDistance:
                case EOpDot:
                case EOpCross:
                case EOpFaceForward:
                case EOpReflect:
                case EOpRefract:
                case EOpMul:
                    break;
                default:
                    return true;
            };
            const TIntermSequence& sequence = node->getSequence();
            // Right now we only handle built-in functions with two parameters.
            if (sequence.size() != 2)
                return true;
            TIntermTyped* param1 = sequence[0]->getAsTyped();
            TIntermTyped* param2 = sequence[1]->getAsTyped();
            if (!param1 || !param2)
                return true;
            bool needToEmulate = mEmulator.SetFunctionCalled(
                node->getOp(), param1->getType(), param2->getType());
            if (needToEmulate)
                node->setUseEmulatedFunction();
        }
        return true;
    }

private:
    BuiltInFunctionEmulator& mEmulator;
};

}  // anonymous namepsace

BuiltInFunctionEmulator::BuiltInFunctionEmulator(ShShaderType shaderType)
    : mFunctionGroupMask(TFunctionGroupAll)
{
    if (shaderType == SH_FRAGMENT_SHADER)
        mFunctionMask = kFunctionEmulationFragmentMask;
    else
        mFunctionMask = kFunctionEmulationVertexMask;
}

void BuiltInFunctionEmulator::SetFunctionGroupMask(
    unsigned int functionGroupMask)
{
    mFunctionGroupMask = functionGroupMask;
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    TOperator op, const TType& param)
{
    TBuiltInFunction function = IdentifyFunction(op, param);
    return SetFunctionCalled(function);
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    TOperator op, const TType& param1, const TType& param2)
{
    TBuiltInFunction function = IdentifyFunction(op, param1, param2);
    return SetFunctionCalled(function);
}

bool BuiltInFunctionEmulator::SetFunctionCalled(
    BuiltInFunctionEmulator::TBuiltInFunction function) {
    if (function == TFunctionUnknown || mFunctionMask[function] == false)
        return false;
    for (size_t i = 0; i < mFunctions.size(); ++i) {
        if (mFunctions[i] == function)
            return true;
    }
    switch (function) {
        case TFunctionAbs1:
        case TFunctionAbs2:
        case TFunctionAbs3:
        case TFunctionAbs4:
            if (mFunctionGroupMask & TFunctionGroupAbs) {
                mFunctions.push_back(function);
                return true;
            }
            break;
        case TFunctionAtan1:
        case TFunctionAtan2:
        case TFunctionAtan3:
        case TFunctionAtan4:
        case TFunctionAtan1_1:
        case TFunctionAtan2_2:
        case TFunctionAtan3_3:
        case TFunctionAtan4_4:
            if (mFunctionGroupMask & TFunctionGroupAtan) {
                mFunctions.push_back(function);
                return true;
            }
            break;
        case TFunctionCos1:
        case TFunctionCos2:
        case TFunctionCos3:
        case TFunctionCos4:
            if (mFunctionGroupMask & TFunctionGroupCos) {
                mFunctions.push_back(function);
                return true;
            }
            break;
        case TFunctionMod1_1:
        case TFunctionMod2_2:
        case TFunctionMod3_3:
        case TFunctionMod4_4:
            if (mFunctionGroupMask & TFunctionGroupMod) {
                mFunctions.push_back(function);
                return true;
            }
            break;
        case TFunctionSign1:
        case TFunctionSign2:
        case TFunctionSign3:
        case TFunctionSign4:
            if (mFunctionGroupMask & TFunctionGroupSign) {
                mFunctions.push_back(function);
                return true;
            }
            break;
        default:
            UNREACHABLE();
            break;
    }
    return false;
}

void BuiltInFunctionEmulator::OutputEmulatedFunctionDefinition(
    TInfoSinkBase& out, bool withPrecision) const
{
    if (mFunctions.size() == 0)
        return;
    out << "// BEGIN: Generated code for built-in function emulation\n\n";
    if (withPrecision) {
        out << "#if defined(GL_FRAGMENT_PRECISION_HIGH) && (GL_FRAGMENT_PRECISION_HIGH == 1)\n"
            << "precision highp float;\n"
            << "#else\n"
            << "precision mediump float;\n"
            << "#endif\n\n";
    }
    for (size_t i = 0; i < mFunctions.size(); ++i) {
        out << kFunctionEmulationSource[mFunctions[i]] << "\n\n";
    }
    out << "// END: Generated code for built-in function emulation\n\n";
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param)
{
    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpAbs:
            function = TFunctionAbs1;
            break;
        case EOpAtan:
            function = TFunctionAtan1;
            break;
        case EOpCos:
            function = TFunctionCos1;
            break;
        case EOpSign:
            function = TFunctionSign1;
            break;
        default:
            break;
    }
    if (function == TFunctionUnknown)
        return TFunctionUnknown;
    if (param.isVector())
        function += param.getNominalSize() - 1;
    return static_cast<TBuiltInFunction>(function);
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param1, const TType& param2)
{
    // Right now for all the emulated functions with two parameters, the two
    // parameters have the same type.
    if (param1.isVector() != param2.isVector() ||
        param1.getNominalSize() != param2.getNominalSize() ||
        param1.getNominalSize() > 4)
        return TFunctionUnknown;

    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpAtan:
            function = TFunctionAtan1_1;
            break;
        case EOpMod:
            function = TFunctionMod1_1;
            break;
        case EOpSign:
            function = TFunctionSign1;
            break;
        default:
            break;
    }
    if (function == TFunctionUnknown)
        return TFunctionUnknown;
    if (param1.isVector())
        function += param1.getNominalSize() - 1;
    return static_cast<TBuiltInFunction>(function);
}

void BuiltInFunctionEmulator::MarkBuiltInFunctionsForEmulation(
    TIntermNode* root)
{
    ASSERT(root);

    BuiltInFunctionEmulationMarker marker(*this);
    root->traverse(&marker);
}

//static
TString BuiltInFunctionEmulator::GetEmulatedFunctionName(
    const TString& name)
{
    ASSERT(name[name.length() - 1] == '(');
    return "webgl_" + name.substr(0, name.length() - 1) + "_emu(";
}


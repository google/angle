//
// Copyright (c) 2002-2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "angle_gl.h"
#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/SymbolTable.h"

namespace
{

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
            const TIntermSequence& sequence = *(node->getSequence());
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

BuiltInFunctionEmulator::BuiltInFunctionEmulator()
{}

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
    mFunctions.push_back(function);
    return true;
}

void BuiltInFunctionEmulator::OutputEmulatedFunctionDefinition(
    TInfoSinkBase& out, bool withPrecision) const
{
    if (mFunctions.size() == 0)
        return;
    out << "// BEGIN: Generated code for built-in function emulation\n\n";
    OutputEmulatedFunctionHeader(out, withPrecision);
    for (size_t i = 0; i < mFunctions.size(); ++i) {
        out << mFunctionSource[mFunctions[i]] << "\n\n";
    }
    out << "// END: Generated code for built-in function emulation\n\n";
}

BuiltInFunctionEmulator::TBuiltInFunction
BuiltInFunctionEmulator::IdentifyFunction(
    TOperator op, const TType& param)
{
    if (param.getNominalSize() > 4 || param.getSecondarySize() > 4)
        return TFunctionUnknown;
    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpCos:
            function = TFunctionCos1;
            break;
        case EOpLength:
            function = TFunctionLength1;
            break;
        case EOpNormalize:
            function = TFunctionNormalize1;
            break;
        case EOpAsinh:
            function = TFunctionAsinh1;
            break;
        case EOpAcosh:
            function = TFunctionAcosh1;
            break;
        case EOpAtanh:
            function = TFunctionAtanh1;
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
    if (param1.getNominalSize()     != param2.getNominalSize()   ||
        param1.getSecondarySize()   != param2.getSecondarySize() ||
        param1.getNominalSize() > 4 || param1.getSecondarySize() > 4)
        return TFunctionUnknown;

    unsigned int function = TFunctionUnknown;
    switch (op) {
        case EOpDistance:
            function = TFunctionDistance1_1;
            break;
        case EOpDot:
            function = TFunctionDot1_1;
            break;
        case EOpReflect:
            function = TFunctionReflect1_1;
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

void BuiltInFunctionEmulator::Cleanup()
{
    mFunctions.clear();
}

//static
TString BuiltInFunctionEmulator::GetEmulatedFunctionName(
    const TString& name)
{
    ASSERT(name[name.length() - 1] == '(');
    return "webgl_" + name.substr(0, name.length() - 1) + "_emu(";
}

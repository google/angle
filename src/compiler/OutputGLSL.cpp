//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/OutputGLSL.h"

#include "common/debug.h"

namespace
{
TString getTypeName(const TType& type)
{
    TInfoSinkBase out;
    if (type.isMatrix())
    {
        out << "mat";
        out << type.getNominalSize();
    }
    else if (type.isVector())
    {
        switch (type.getBasicType())
        {
            case EbtFloat: out << "vec"; break;
            case EbtInt: out << "ivec"; break;
            case EbtBool: out << "bvec"; break;
            default: UNREACHABLE(); break;
        }
        out << type.getNominalSize();
    }
    else
    {
        if (type.getBasicType() == EbtStruct)
            out << type.getTypeName();
        else
            out << type.getBasicString();
    }
    return TString(out.c_str());
}
}  // namespace

TOutputGLSL::TOutputGLSL(TInfoSinkBase& objSink)
    : TIntermTraverser(true, true, true),
      mObjSink(objSink),
      mWriteFullSymbol(false)
{
}

void TOutputGLSL::writeTriplet(Visit visit, const char* preStr, const char* inStr, const char* postStr)
{
    TInfoSinkBase& out = objSink();
    if (visit == PreVisit && preStr)
    {
        out << preStr;
    }
    else if (visit == InVisit && inStr)
    {
        out << inStr;
    }
    else if (visit == PostVisit && postStr)
    {
        out << postStr;
    }
}

void TOutputGLSL::visitSymbol(TIntermSymbol* node)
{
    TInfoSinkBase& out = objSink();
    const TType& type = node->getType();

    if (mWriteFullSymbol)
    {
        TQualifier qualifier = node->getQualifier();
        if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal))
            out << node->getQualifierString() << " ";

        // Declare the struct if we have not done so already.
        if ((type.getBasicType() == EbtStruct) && (mDeclaredStructs.find(type.getTypeName()) == mDeclaredStructs.end()))
        {
            out << "struct " << type.getTypeName() << "{\n";
            const TTypeList* structure = type.getStruct();
            ASSERT(structure != NULL);
            for (size_t i = 0; i < structure->size(); ++i)
            {
                const TType* fieldType = (*structure)[i].type;
                ASSERT(fieldType != NULL);
                out << getTypeName(*fieldType) << " " << fieldType->getFieldName() << ";\n";
            }
            out << "} ";
            mDeclaredStructs.insert(type.getTypeName());
        }
        else
        {
            out << getTypeName(type) << " ";
        }
    }

    out << node->getSymbol();

    if (mWriteFullSymbol && node->getType().isArray())
    {
        out << "[" << node->getType().getArraySize() << "]";
    }
}

void TOutputGLSL::visitConstantUnion(TIntermConstantUnion* node)
{
    TInfoSinkBase& out = objSink();

    const TType& type = node->getType();
    int size = type.getObjectSize();
    bool writeType = (size > 1) || (type.getBasicType() == EbtStruct);
    if (writeType)
        out << getTypeName(type) << "(";
    for (int i = 0; i < size; ++i)
    {
        const constUnion& data = node->getUnionArrayPointer()[i];
        switch (data.getType())
        {
            case EbtFloat: out << data.getFConst(); break;
            case EbtInt: out << data.getIConst(); break;
            case EbtBool: out << data.getBConst(); break;
            default: UNREACHABLE(); break;
        }
        if (i != size - 1)
            out << ", ";
    }
    if (writeType)
        out << ")";
}

bool TOutputGLSL::visitBinary(Visit visit, TIntermBinary* node)
{
    bool visitChildren = true;
    TInfoSinkBase& out = objSink();
    switch (node->getOp())
    {
        case EOpAssign: writeTriplet(visit, NULL, " = ", NULL); break;
        case EOpInitialize:
            if (visit == InVisit) {
                out << " = ";
                mWriteFullSymbol= false;
            }
            break;
        case EOpAddAssign: writeTriplet(visit, NULL, " += ", NULL); break;
        case EOpSubAssign: writeTriplet(visit, NULL, " -= ", NULL); break;
        case EOpDivAssign: writeTriplet(visit, NULL, " /= ", NULL); break;
        case EOpMulAssign: 
        case EOpVectorTimesMatrixAssign:
        case EOpVectorTimesScalarAssign:
        case EOpMatrixTimesScalarAssign:
        case EOpMatrixTimesMatrixAssign:
            writeTriplet(visit, NULL, " *= ", NULL);
            break;

        case EOpIndexDirect:
        case EOpIndexIndirect:
            writeTriplet(visit, NULL, "[", "]");
            break;
        case EOpIndexDirectStruct:
            if (visit == InVisit)
            {
                out << ".";
                // TODO(alokp): ASSERT
                out << node->getType().getFieldName();
                visitChildren = false;
            }
            break;
        case EOpVectorSwizzle:
            if (visit == InVisit)
            {
                out << ".";
                TIntermAggregate* rightChild = node->getRight()->getAsAggregate();
                TIntermSequence& sequence = rightChild->getSequence();
                for (TIntermSequence::iterator sit = sequence.begin(); sit != sequence.end(); ++sit)
                {
                    TIntermConstantUnion* element = (*sit)->getAsConstantUnion();
                    ASSERT(element->getBasicType() == EbtInt);
                    ASSERT(element->getNominalSize() == 1);
                    const constUnion& data = element->getUnionArrayPointer()[0];
                    ASSERT(data.getType() == EbtInt);
                    switch (data.getIConst())
                    {
                        case 0: out << "x"; break;
                        case 1: out << "y"; break;
                        case 2: out << "z"; break;
                        case 3: out << "w"; break;
                        default: UNREACHABLE(); break;
                    }
                }
                visitChildren = false;
            }
            break;

        case EOpAdd: writeTriplet(visit, "(", " + ", ")"); break;
        case EOpSub: writeTriplet(visit, "(", " - ", ")"); break;
        case EOpMul: writeTriplet(visit, "(", " * ", ")"); break;
        case EOpDiv: writeTriplet(visit, "(", " / ", ")"); break;
        case EOpMod: UNIMPLEMENTED(); break;
        case EOpEqual: writeTriplet(visit, "(", " == ", ")"); break;
        case EOpNotEqual: writeTriplet(visit, "(", " != ", ")"); break;
        case EOpLessThan: writeTriplet(visit, "(", " < ", ")"); break;
        case EOpGreaterThan: writeTriplet(visit, "(", " > ", ")"); break;
        case EOpLessThanEqual: writeTriplet(visit, "(", " <= ", ")"); break;
        case EOpGreaterThanEqual: writeTriplet(visit, "(", " >= ", ")"); break;

        // Notice the fall-through.
        case EOpVectorTimesScalar:
        case EOpVectorTimesMatrix:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesScalar:
        case EOpMatrixTimesMatrix:
            writeTriplet(visit, "(", " * ", ")");
            break;

        case EOpLogicalOr: writeTriplet(visit, "(", " || ", ")"); break;
        case EOpLogicalXor: writeTriplet(visit, "(", " ^^ ", ")"); break;
        case EOpLogicalAnd: writeTriplet(visit, "(", " && ", ")"); break;
        default: UNREACHABLE(); break;
    }

    return visitChildren;
}

bool TOutputGLSL::visitUnary(Visit visit, TIntermUnary* node)
{
    TInfoSinkBase& out = objSink();

    switch (node->getOp())
    {
        case EOpNegative: writeTriplet(visit, "(-", NULL, ")"); break;
        case EOpVectorLogicalNot: writeTriplet(visit, "(!", NULL, ")"); break;
        case EOpLogicalNot: writeTriplet(visit, "(!", NULL, ")"); break;

        case EOpPostIncrement: writeTriplet(visit, "(", NULL, "++)"); break;
        case EOpPostDecrement: writeTriplet(visit, "(", NULL, "--)"); break;
        case EOpPreIncrement: writeTriplet(visit, "(++", NULL, ")"); break;
        case EOpPreDecrement: writeTriplet(visit, "(--", NULL, ")"); break;

        case EOpConvIntToBool:
        case EOpConvFloatToBool:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: writeTriplet(visit, "bool(", NULL, ")");  break;
                case 2: writeTriplet(visit, "bvec2(", NULL, ")"); break;
                case 3: writeTriplet(visit, "bvec3(", NULL, ")"); break;
                case 4: writeTriplet(visit, "bvec4(", NULL, ")"); break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvBoolToFloat:
        case EOpConvIntToFloat:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: writeTriplet(visit, "float(", NULL, ")");  break;
                case 2: writeTriplet(visit, "vec2(", NULL, ")"); break;
                case 3: writeTriplet(visit, "vec3(", NULL, ")"); break;
                case 4: writeTriplet(visit, "vec4(", NULL, ")"); break;
                default: UNREACHABLE();
            }
            break;
        case EOpConvFloatToInt:
        case EOpConvBoolToInt:
            switch (node->getOperand()->getType().getNominalSize())
            {
                case 1: writeTriplet(visit, "int(", NULL, ")");  break;
                case 2: writeTriplet(visit, "ivec2(", NULL, ")"); break;
                case 3: writeTriplet(visit, "ivec3(", NULL, ")"); break;
                case 4: writeTriplet(visit, "ivec4(", NULL, ")"); break;
                default: UNREACHABLE();
            }
            break;

        case EOpRadians: writeTriplet(visit, "radians(", NULL, ")"); break;
        case EOpDegrees: writeTriplet(visit, "degrees(", NULL, ")"); break;
        case EOpSin: writeTriplet(visit, "sin(", NULL, ")"); break;
        case EOpCos: writeTriplet(visit, "cos(", NULL, ")"); break;
        case EOpTan: writeTriplet(visit, "tan(", NULL, ")"); break;
        case EOpAsin: writeTriplet(visit, "asin(", NULL, ")"); break;
        case EOpAcos: writeTriplet(visit, "acos(", NULL, ")"); break;
        case EOpAtan: writeTriplet(visit, "atan(", NULL, ")"); break;

        case EOpExp: writeTriplet(visit, "exp(", NULL, ")"); break;
        case EOpLog: writeTriplet(visit, "log(", NULL, ")"); break;
        case EOpExp2: writeTriplet(visit, "exp2(", NULL, ")"); break;
        case EOpLog2: writeTriplet(visit, "log2(", NULL, ")"); break;
        case EOpSqrt: writeTriplet(visit, "sqrt(", NULL, ")"); break;
        case EOpInverseSqrt: writeTriplet(visit, "inversesqrt(", NULL, ")"); break;

        case EOpAbs: writeTriplet(visit, "abs(", NULL, ")"); break;
        case EOpSign: writeTriplet(visit, "sign(", NULL, ")"); break;
        case EOpFloor: writeTriplet(visit, "floor(", NULL, ")"); break;
        case EOpCeil: writeTriplet(visit, "ceil(", NULL, ")"); break;
        case EOpFract: writeTriplet(visit, "fract(", NULL, ")"); break;

        case EOpLength: writeTriplet(visit, "length(", NULL, ")"); break;
        case EOpNormalize: writeTriplet(visit, "normalize(", NULL, ")"); break;

        case EOpAny: writeTriplet(visit, "any(", NULL, ")"); break;
        case EOpAll: writeTriplet(visit, "all(", NULL, ")"); break;

        default: UNREACHABLE(); break;
    }

    return true;
}

bool TOutputGLSL::visitSelection(Visit visit, TIntermSelection* node)
{
    TInfoSinkBase& out = objSink();

    if (node->usesTernaryOperator())
    {
        out << "(";
        node->getCondition()->traverse(this);
        out << ") ? (";
        node->getTrueBlock()->traverse(this);
        out << ") : (";
        node->getFalseBlock()->traverse(this);
        out << ")";
    }
    else
    {
        out << "if (";
        node->getCondition()->traverse(this);
        out << ") {\n";

        incrementDepth();
        if (node->getTrueBlock())
        {
            node->getTrueBlock()->traverse(this);
        }
        out << "}";

        if (node->getFalseBlock())
        {
            out << " else {\n";
            node->getFalseBlock()->traverse(this);
            out << "}";
        }
        decrementDepth();
        out << "\n";
    }
    return false;
}

bool TOutputGLSL::visitAggregate(Visit visit, TIntermAggregate* node)
{
    TInfoSinkBase& out = objSink();
    switch (node->getOp())
    {
        case EOpSequence:
            writeTriplet(visit, NULL, ";\n", ";\n");
            break;
        case EOpPrototype:
            // Function declaration.
            if (visit == PreVisit)
            {
                TString returnType = getTypeName(node->getType());
                out << returnType << " " << node->getName() << "(";
                mWriteFullSymbol = true;
            }
            else if (visit == InVisit)
            {
                // Called in between function arguments.
                out << ", ";
            }
            else if (visit == PostVisit)
            {
                // Called after fucntion arguments.
                out << ")";
                mWriteFullSymbol = false;
            }
            break;
        case EOpFunction:
            // Function definition.
            if (visit == PreVisit)
            {
                TString returnType = getTypeName(node->getType());
                TString functionName = TFunction::unmangleName(node->getName());
                out << returnType << " " << functionName;
            }
            else if (visit == InVisit)
            {
                // Called after traversing function arguments (EOpParameters)
                // but before traversing function body (EOpSequence).
                out << "{\n";
            }
            else if (visit == PostVisit)
            {
                // Called after traversing function body (EOpSequence).
                out << "}\n";
            }
            break;
        case EOpFunctionCall:
            // Function call.
            if (visit == PreVisit)
            {
                TString functionName = TFunction::unmangleName(node->getName());
                out << functionName << "(";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
            }
            break;
        case EOpParameters:
            // Function parameters.
            if (visit == PreVisit)
            {
                out << "(";
                mWriteFullSymbol = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
                mWriteFullSymbol = false;
            }
            break;
        case EOpDeclaration:
            // Variable declaration.
            if (visit == PreVisit)
            {
                mWriteFullSymbol = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
                mWriteFullSymbol = false;
            }
            else
            {
                mWriteFullSymbol = false;
            }
            break;

        case EOpConstructFloat: writeTriplet(visit, "float(", NULL, ")"); break;
        case EOpConstructVec2: writeTriplet(visit, "vec2(", ", ", ")"); break;
        case EOpConstructVec3: writeTriplet(visit, "vec3(", ", ", ")"); break;
        case EOpConstructVec4: writeTriplet(visit, "vec4(", ", ", ")"); break;
        case EOpConstructBool: writeTriplet(visit, "bool(", NULL, ")"); break;
        case EOpConstructBVec2: writeTriplet(visit, "bvec2(", ", ", ")"); break;
        case EOpConstructBVec3: writeTriplet(visit, "bvec3(", ", ", ")"); break;
        case EOpConstructBVec4: writeTriplet(visit, "bvec4(", ", ", ")"); break;
        case EOpConstructInt: writeTriplet(visit, "int(", NULL, ")"); break;
        case EOpConstructIVec2: writeTriplet(visit, "ivec2(", ", ", ")"); break;
        case EOpConstructIVec3: writeTriplet(visit, "ivec3(", ", ", ")"); break;
        case EOpConstructIVec4: writeTriplet(visit, "ivec4(", ", ", ")"); break;
        case EOpConstructMat2: writeTriplet(visit, "mat2(", ", ", ")"); break;
        case EOpConstructMat3: writeTriplet(visit, "mat3(", ", ", ")"); break;
        case EOpConstructMat4: writeTriplet(visit, "mat4(", ", ", ")"); break;
        case EOpConstructStruct:
            if (visit == PreVisit)
            {
                const TType& type = node->getType();
                ASSERT(type.getBasicType() == EbtStruct);
                out << type.getTypeName() << "(";
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
            }
            break;

        case EOpLessThan: writeTriplet(visit, "lessThan(", ", ", ")"); break;
        case EOpGreaterThan: writeTriplet(visit, "greaterThan(", ", ", ")"); break;
        case EOpLessThanEqual: writeTriplet(visit, "lessThanEqual(", ", ", ")"); break;
        case EOpGreaterThanEqual: writeTriplet(visit, "greaterThanEqual(", ", ", ")"); break;
        case EOpVectorEqual: writeTriplet(visit, "equal(", ", ", ")"); break;
        case EOpVectorNotEqual: writeTriplet(visit, "notEqual(", ", ", ")"); break;
        case EOpComma: writeTriplet(visit, NULL, ", ", NULL); break;

        case EOpMod: writeTriplet(visit, "mod(", ", ", ")"); break;
        case EOpPow: writeTriplet(visit, "pow(", ", ", ")"); break;
        case EOpAtan: writeTriplet(visit, "atan(", ", ", ")"); break;
        case EOpMin: writeTriplet(visit, "min(", ", ", ")"); break;
        case EOpMax: writeTriplet(visit, "max(", ", ", ")"); break;
        case EOpClamp: writeTriplet(visit, "clamp(", ", ", ")"); break;
        case EOpMix: writeTriplet(visit, "mix(", ", ", ")"); break;
        case EOpStep: writeTriplet(visit, "step(", ", ", ")"); break;
        case EOpSmoothStep: writeTriplet(visit, "smoothstep(", ", ", ")"); break;

        case EOpDistance: writeTriplet(visit, "distance(", ", ", ")"); break;
        case EOpDot: writeTriplet(visit, "dot(", ", ", ")"); break;
        case EOpCross: writeTriplet(visit, "cross(", ", ", ")"); break;
        case EOpFaceForward: writeTriplet(visit, "faceforward(", ", ", ")"); break;
        case EOpReflect: writeTriplet(visit, "reflect(", ", ", ")"); break;
        case EOpRefract: writeTriplet(visit, "refract(", ", ", ")"); break;
        case EOpMul: writeTriplet(visit, "matrixCompMult(", ", ", ")"); break;

        default: UNREACHABLE(); break;
    }
    return true;
}

bool TOutputGLSL::visitLoop(Visit visit, TIntermLoop* node)
{
    TInfoSinkBase& out = objSink();

    // Loop header.
    if (node->testFirst())  // for loop
    {
        out << "for (";
        if (node->getInit())
            node->getInit()->traverse(this);
        out << "; ";

        ASSERT(node->getTest() != NULL);
        node->getTest()->traverse(this);
        out << "; ";

        if (node->getTerminal())
            node->getTerminal()->traverse(this);
        out << ") {\n";
    }
    else  // do-while loop
    {
        out << "do {\n";
    }

    // Loop body.
    if (node->getBody())
        node->getBody()->traverse(this);

    // Loop footer.
    if (node->testFirst())  // for loop
    {
        out << "}\n";
    }
    else  // do-while loop
    {
        out << "} while (";
        ASSERT(node->getTest() != NULL);
        node->getTest()->traverse(this);
        out << ");\n";
    }

    // No need to visit children. They have been already processed in
    // this function.
    return false;
}

bool TOutputGLSL::visitBranch(Visit visit, TIntermBranch* node)
{
    TInfoSinkBase &out = objSink();

    switch (node->getFlowOp())
    {
        case EOpKill: writeTriplet(visit, "discard", NULL, NULL); break;
        case EOpBreak: writeTriplet(visit, "break", NULL, NULL); break;
        case EOpContinue: writeTriplet(visit, "continue", NULL, NULL); break;
        case EOpReturn: writeTriplet(visit, "return ", NULL, NULL); break;
        default: UNREACHABLE(); break;
    }

    return true;
}

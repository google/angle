//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "OutputGLSL.h"
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

TString getIndentationString(int depth)
{
    TString indentation(depth, ' ');
    return indentation;
}
}  // namespace

TOutputGLSL::TOutputGLSL(TParseContext &context)
    : TIntermTraverser(true, true, true),
      writeFullSymbol(false),
      parseContext(context)
{
}

// Header declares user-defined structs.
void TOutputGLSL::header()
{
    TInfoSinkBase& out = objSink();

    TSymbolTableLevel* symbols = parseContext.symbolTable.getGlobalLevel();
    for (TSymbolTableLevel::const_iterator symbolIter = symbols->begin(); symbolIter != symbols->end(); ++symbolIter)
    {
        const TSymbol* symbol = symbolIter->second;
        if (!symbol->isVariable())
            continue;

        const TVariable* variable = static_cast<const TVariable*>(symbol);
        if (!variable->isUserType())
            continue;

        const TType& type = variable->getType();
        ASSERT(type.getQualifier() == EvqTemporary);
        ASSERT(type.getBasicType() == EbtStruct);

        out << "struct " << variable->getName() << "{\n";
        const TTypeList* structure = type.getStruct();
        ASSERT(structure != NULL);
        incrementDepth();
        for (size_t i = 0; i < structure->size(); ++i) {
            const TType* fieldType = (*structure)[i].type;
            ASSERT(fieldType != NULL);
            out << getIndentationString(depth);
            out << getTypeName(*fieldType) << " " << fieldType->getFieldName() << ";\n";
        }
        decrementDepth();
        out << "};\n";
    }
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
    if (writeFullSymbol)
    {
        TQualifier qualifier = node->getQualifier();
        if ((qualifier != EvqTemporary) && (qualifier != EvqGlobal))
            out << node->getQualifierString() << " ";
 
        out << getTypeName(node->getType()) << " ";
    }
    out << node->getSymbol();
    if (writeFullSymbol && node->getType().isArray())
    {
        out << "[" << node->getType().getArraySize() << "]";
    }
}

void TOutputGLSL::visitConstantUnion(TIntermConstantUnion* node)
{
    TInfoSinkBase& out = objSink();

    TType type = node->getType();
    int size = type.getObjectSize();
    if (size > 1)
        out << getTypeName(type) << "(";
    for (int i = 0; i < size; ++i) {
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
    if (size > 1)
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
                writeFullSymbol= false;
            }
            break;
        case EOpAddAssign: writeTriplet(visit, NULL, " += ", NULL); break;
        case EOpSubAssign: UNIMPLEMENTED(); break;
        case EOpMulAssign: writeTriplet(visit, NULL, " *= ", NULL); break;
        case EOpVectorTimesMatrixAssign: UNIMPLEMENTED(); break;
        case EOpVectorTimesScalarAssign: UNIMPLEMENTED(); break;
        case EOpMatrixTimesScalarAssign: UNIMPLEMENTED(); break;
        case EOpMatrixTimesMatrixAssign: UNIMPLEMENTED(); break;
        case EOpDivAssign: UNIMPLEMENTED(); break;

        case EOpIndexDirect: writeTriplet(visit, NULL, "[", "]"); break;
        case EOpIndexIndirect: UNIMPLEMENTED(); break;
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
        case EOpNotEqual: UNIMPLEMENTED(); break;
        case EOpLessThan: writeTriplet(visit, "(", " < ", ")"); break;
        case EOpGreaterThan: writeTriplet(visit, "(", " > ", ")"); break;
        case EOpLessThanEqual: UNIMPLEMENTED(); break;
        case EOpGreaterThanEqual: UNIMPLEMENTED(); break;

        // Notice the fall-through.
        case EOpVectorTimesScalar:
        case EOpVectorTimesMatrix:
        case EOpMatrixTimesVector:
        case EOpMatrixTimesScalar:
        case EOpMatrixTimesMatrix:
            writeTriplet(visit, "(", " * ", ")");
            break;

        case EOpLogicalOr: writeTriplet(visit, "(", " || ", ")"); break;
        case EOpLogicalXor: UNIMPLEMENTED(); break;
        case EOpLogicalAnd: UNIMPLEMENTED(); break;
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
        case EOpVectorLogicalNot: UNIMPLEMENTED(); break;
        case EOpLogicalNot: UNIMPLEMENTED(); break;

        case EOpPostIncrement: UNIMPLEMENTED(); break;
        case EOpPostDecrement: UNIMPLEMENTED(); break;
        case EOpPreIncrement: UNIMPLEMENTED(); break;
        case EOpPreDecrement: UNIMPLEMENTED(); break;

        case EOpConvIntToBool: UNIMPLEMENTED(); break;
        case EOpConvFloatToBool: UNIMPLEMENTED(); break;
        case EOpConvBoolToFloat: UNIMPLEMENTED(); break;
        case EOpConvIntToFloat: writeTriplet(visit, "float(", NULL, ")"); break;
        case EOpConvFloatToInt: writeTriplet(visit, "int(", NULL, ")"); break;
        case EOpConvBoolToInt: UNIMPLEMENTED(); break;

        case EOpRadians: UNIMPLEMENTED(); break;
        case EOpDegrees: UNIMPLEMENTED(); break;
        case EOpSin: writeTriplet(visit, "sin(", NULL, ")"); break;
        case EOpCos: writeTriplet(visit, "cos(", NULL, ")"); break;
        case EOpTan: UNIMPLEMENTED(); break;
        case EOpAsin: UNIMPLEMENTED(); break;
        case EOpAcos: writeTriplet(visit, "acos(", NULL, ")"); break;
        case EOpAtan: UNIMPLEMENTED(); break;

        case EOpExp: UNIMPLEMENTED(); break;
        case EOpLog: UNIMPLEMENTED(); break;
        case EOpExp2: UNIMPLEMENTED(); break;
        case EOpLog2: UNIMPLEMENTED(); break;
        case EOpSqrt: UNIMPLEMENTED(); break;
        case EOpInverseSqrt: UNIMPLEMENTED(); break;

        case EOpAbs: writeTriplet(visit, "abs(", NULL, ")"); break;
        case EOpSign: UNIMPLEMENTED(); break;
        case EOpFloor: writeTriplet(visit, "floor(", NULL, ")"); break;
        case EOpCeil: UNIMPLEMENTED(); break;
        case EOpFract: UNIMPLEMENTED(); break;

        case EOpLength: UNIMPLEMENTED(); break;
        case EOpNormalize: writeTriplet(visit, "normalize(", NULL, ")"); break;

        case EOpAny: UNIMPLEMENTED(); break;
        case EOpAll: UNIMPLEMENTED(); break;

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
        node->getTrueBlock()->traverse(this);
        out << getIndentationString(depth - 2) << "}";

        if (node->getFalseBlock())
        {
            out << " else {\n";
            node->getFalseBlock()->traverse(this);
            out << getIndentationString(depth - 2) << "}";
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
            if (visit == PreVisit)
            {
                out << getIndentationString(depth);
            }
            else if (visit == InVisit)
            {
                out << ";\n";
                out << getIndentationString(depth - 1);
            }
            else
            {
                out << ";\n";
            }
            break;
        case EOpComma:
            UNIMPLEMENTED();
            return true;
        case EOpFunction:
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
            if (visit == PreVisit)
            {
                out << "(";
                writeFullSymbol = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
            }
            else
            {
                out << ")";
                writeFullSymbol = false;
            }
            break;
        case EOpDeclaration:
            if (visit == PreVisit)
            {
                writeFullSymbol = true;
            }
            else if (visit == InVisit)
            {
                out << ", ";
                writeFullSymbol = false;
            }
            else
            {
                writeFullSymbol = false;
            }
            break;

        case EOpConstructFloat: UNIMPLEMENTED(); break;
	case EOpConstructVec2: writeTriplet(visit, "vec2(", ", ", ")"); break;
	case EOpConstructVec3: writeTriplet(visit, "vec3(", ", ", ")"); break;
	case EOpConstructVec4: writeTriplet(visit, "vec4(", ", ", ")"); break;
	case EOpConstructBool: UNIMPLEMENTED(); break;
	case EOpConstructBVec2: UNIMPLEMENTED(); break;
	case EOpConstructBVec3: UNIMPLEMENTED(); break;
	case EOpConstructBVec4: UNIMPLEMENTED(); break;
	case EOpConstructInt: UNIMPLEMENTED(); break;
	case EOpConstructIVec2: UNIMPLEMENTED(); break;
	case EOpConstructIVec3: UNIMPLEMENTED(); break;
	case EOpConstructIVec4: UNIMPLEMENTED(); break;
	case EOpConstructMat2: UNIMPLEMENTED(); break;
	case EOpConstructMat3: UNIMPLEMENTED(); break;
	case EOpConstructMat4: writeTriplet(visit, "mat4(", ", ", ")"); break;
	case EOpConstructStruct: UNIMPLEMENTED(); break;

	case EOpLessThan: UNIMPLEMENTED(); break;
	case EOpGreaterThan: UNIMPLEMENTED(); break;
	case EOpLessThanEqual: UNIMPLEMENTED(); break;
	case EOpGreaterThanEqual: UNIMPLEMENTED(); break;
	case EOpVectorEqual: UNIMPLEMENTED(); break;
	case EOpVectorNotEqual: UNIMPLEMENTED(); break;

	case EOpMod: writeTriplet(visit, "mod(", ", ", ")"); break;
	case EOpPow: writeTriplet(visit, "pow(", ", ", ")"); break;

	case EOpAtan: UNIMPLEMENTED(); break;

	case EOpMin: writeTriplet(visit, "min(", ", ", ")"); break;
	case EOpMax: writeTriplet(visit, "max(", ", ", ")"); break;
	case EOpClamp: writeTriplet(visit, "clamp(", ", ", ")"); break;
	case EOpMix: writeTriplet(visit, "mix(", ", ", ")"); break;
	case EOpStep: UNIMPLEMENTED(); break;
	case EOpSmoothStep: UNIMPLEMENTED(); break;

	case EOpDistance: UNIMPLEMENTED(); break;
	case EOpDot: writeTriplet(visit, "dot(", ", ", ")"); break;
	case EOpCross: UNIMPLEMENTED(); break;
	case EOpFaceForward: UNIMPLEMENTED(); break;
	case EOpReflect: writeTriplet(visit, "reflect(", ", ", ")"); break;
	case EOpRefract: UNIMPLEMENTED(); break;
	case EOpMul: UNIMPLEMENTED(); break;

        default: UNREACHABLE(); break;
    }
    return true;
}

bool TOutputGLSL::visitLoop(Visit visit, TIntermLoop* node)
{
    UNIMPLEMENTED();
    return true;
}

bool TOutputGLSL::visitBranch(Visit visit, TIntermBranch* node)
{
    TInfoSinkBase &out = objSink();

    switch (node->getFlowOp())
    {
        case EOpKill: UNIMPLEMENTED(); break;
        case EOpBreak: UNIMPLEMENTED(); break;
        case EOpContinue: UNIMPLEMENTED(); break;
        case EOpReturn:
            if (visit == PreVisit)
                out << "return ";
            break;
        default: UNREACHABLE(); break;
    }

    return true;
}

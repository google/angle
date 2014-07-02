//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/VariableInfo.h"
#include "compiler/translator/util.h"
#include "angle_gl.h"

namespace {

TString arrayBrackets(int index)
{
    TStringStream stream;
    stream << "[" << index << "]";
    return stream.str();
}

template <typename VarT>
void getBuiltInVariableInfo(const TType &type,
                            const TString &name,
                            const TString &mappedName,
                            std::vector<VarT> &infoList);

template <typename VarT>
void getUserDefinedVariableInfo(const TType &type,
                                const TString &name,
                                const TString &mappedName,
                                std::vector<VarT> &infoList,
                                ShHashFunction64 hashFunction);

// Returns info for an attribute, uniform, or varying.
template <typename VarT>
void getVariableInfo(const TType &type,
                     const TString &name,
                     const TString &mappedName,
                     std::vector<VarT> &infoList,
                     ShHashFunction64 hashFunction)
{
    if (type.getBasicType() == EbtStruct || type.isInterfaceBlock()) {
        if (type.isArray()) {
            for (int i = 0; i < type.getArraySize(); ++i) {
                TString lname = name + arrayBrackets(i);
                TString lmappedName = mappedName + arrayBrackets(i);
                getUserDefinedVariableInfo(type, lname, lmappedName, infoList, hashFunction);
            }
        } else {
            getUserDefinedVariableInfo(type, name, mappedName, infoList, hashFunction);
        }
    } else {
        getBuiltInVariableInfo(type, name, mappedName, infoList);
    }
}

template <class VarT>
void getBuiltInVariableInfo(const TType &type,
                            const TString &name,
                            const TString &mappedName,
                            std::vector<VarT> &infoList)
{
    ASSERT(type.getBasicType() != EbtStruct);

    VarT varInfo;
    if (type.isArray()) {
        varInfo.name = (name + "[0]").c_str();
        varInfo.mappedName = (mappedName + "[0]").c_str();
        varInfo.arraySize = type.getArraySize();
    } else {
        varInfo.name = name.c_str();
        varInfo.mappedName = mappedName.c_str();
        varInfo.arraySize = 0;
    }
    varInfo.precision = sh::GLVariablePrecision(type);
    varInfo.type = sh::GLVariableType(type);
    infoList.push_back(varInfo);
}

template <class VarT>
void getUserDefinedVariableInfo(const TType &type,
                                const TString &name,
                                const TString &mappedName,
                                std::vector<VarT> &infoList,
                                ShHashFunction64 hashFunction)
{
    ASSERT(type.getBasicType() == EbtStruct || type.isInterfaceBlock());

    const TFieldList& fields = type.isInterfaceBlock() ?
        type.getInterfaceBlock()->fields() :
        type.getStruct()->fields();
    for (size_t i = 0; i < fields.size(); ++i) {
        const TType& fieldType = *(fields[i]->type());
        const TString& fieldName = fields[i]->name();
        getVariableInfo(fieldType,
                        name + "." + fieldName,
                        mappedName + "." + TIntermTraverser::hash(fieldName, hashFunction),
                        infoList,
                        hashFunction);
    }
}

template <class VarT>
VarT* findVariable(const TType &type,
                   const TString &name,
                   std::vector<VarT> *infoList)
{
    // TODO(zmo): optimize this function.
    TString myName = name;
    if (type.isArray())
        myName += "[0]";
    for (size_t ii = 0; ii < infoList->size(); ++ii)
    {
        if ((*infoList)[ii].name.c_str() == myName)
            return &((*infoList)[ii]);
    }
    return NULL;
}

}  // namespace anonymous

CollectVariables::CollectVariables(std::vector<sh::Attribute> *attribs,
                                   std::vector<sh::Uniform> *uniforms,
                                   std::vector<sh::Varying> *varyings,
                                   ShHashFunction64 hashFunction)
    : mAttribs(attribs),
      mUniforms(uniforms),
      mVaryings(varyings),
      mPointCoordAdded(false),
      mFrontFacingAdded(false),
      mFragCoordAdded(false),
      mHashFunction(hashFunction)
{
}

// We want to check whether a uniform/varying is statically used
// because we only count the used ones in packing computing.
// Also, gl_FragCoord, gl_PointCoord, and gl_FrontFacing count
// toward varying counting if they are statically used in a fragment
// shader.
void CollectVariables::visitSymbol(TIntermSymbol* symbol)
{
    ASSERT(symbol != NULL);
    sh::ShaderVariable *var = NULL;
    switch (symbol->getQualifier())
    {
      case EvqVaryingOut:
      case EvqInvariantVaryingOut:
      case EvqVaryingIn:
      case EvqInvariantVaryingIn:
        var = findVariable(symbol->getType(), symbol->getSymbol(), mVaryings);
        break;
      case EvqUniform:
        var = findVariable(symbol->getType(), symbol->getSymbol(), mUniforms);
        break;
      case EvqFragCoord:
        if (!mFragCoordAdded)
        {
            sh::Varying info;
            info.name = "gl_FragCoord";
            info.mappedName = "gl_FragCoord";
            info.type = GL_FLOAT_VEC4;
            info.arraySize = 0;
            info.precision = GL_MEDIUM_FLOAT;  // Use mediump as it doesn't really matter.
            info.staticUse = true;
            mVaryings->push_back(info);
            mFragCoordAdded = true;
        }
        return;
      case EvqFrontFacing:
        if (!mFrontFacingAdded)
        {
            sh::Varying info;
            info.name = "gl_FrontFacing";
            info.mappedName = "gl_FrontFacing";
            info.type = GL_BOOL;
            info.arraySize = 0;
            info.precision = GL_NONE;
            info.staticUse = true;
            mVaryings->push_back(info);
            mFrontFacingAdded = true;
        }
        return;
      case EvqPointCoord:
        if (!mPointCoordAdded)
        {
            sh::Varying info;
            info.name = "gl_PointCoord";
            info.mappedName = "gl_PointCoord";
            info.type = GL_FLOAT_VEC2;
            info.arraySize = 0;
            info.precision = GL_MEDIUM_FLOAT;  // Use mediump as it doesn't really matter.
            info.staticUse = true;
            mVaryings->push_back(info);
            mPointCoordAdded = true;
        }
        return;
      default:
        break;
    }
    if (var)
    {
        var->staticUse = true;
    }
}

template <typename VarT>
void CollectVariables::visitInfoList(const TIntermSequence& sequence, std::vector<VarT> *infoList) const
{
    for (size_t seqIndex = 0; seqIndex < sequence.size(); seqIndex++)
    {
        const TIntermSymbol* variable = sequence[seqIndex]->getAsSymbolNode();
        // The only case in which the sequence will not contain a
        // TIntermSymbol node is initialization. It will contain a
        // TInterBinary node in that case. Since attributes, uniforms,
        // and varyings cannot be initialized in a shader, we must have
        // only TIntermSymbol nodes in the sequence.
        ASSERT(variable != NULL);
        TString processedSymbol;
        if (mHashFunction == NULL)
            processedSymbol = variable->getSymbol();
        else
            processedSymbol = TIntermTraverser::hash(variable->getSymbol(), mHashFunction);
        getVariableInfo(variable->getType(),
            variable->getSymbol(),
            processedSymbol,
            *infoList,
            mHashFunction);
    }
}

bool CollectVariables::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = true;

    switch (node->getOp())
    {
    case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if (qualifier == EvqAttribute || qualifier == EvqVertexIn || qualifier == EvqUniform ||
            qualifier == EvqVaryingIn || qualifier == EvqVaryingOut ||
            qualifier == EvqInvariantVaryingIn || qualifier == EvqInvariantVaryingOut)
        {
            switch (qualifier)
            {
              case EvqAttribute:
              case EvqVertexIn:
                visitInfoList(sequence, mAttribs);
                break;
              case EvqUniform:
                visitInfoList(sequence, mUniforms);
                break;
              default:
                visitInfoList(sequence, mVaryings);
                break;
            }

            if (!sequence.empty())
            {
                visitChildren = false;
            }
        }
        break;
    }
    default: break;
    }

    return visitChildren;
}

//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/VariableInfo.h"

static TString arrayBrackets(int index)
{
    TStringStream stream;
    stream << "[" << index << "]";
    return stream.str();
}

// Returns the data type for an attribute or uniform.
static ShDataType getVariableDataType(const TType& type)
{
    switch (type.getBasicType()) {
      case EbtFloat:
          if (type.isMatrix()) {
              switch (type.getCols())
              {
                case 2:
                  switch (type.getRows())
                  {
                    case 2: return SH_FLOAT_MAT2;
                    case 3: return SH_FLOAT_MAT2x3;
                    case 4: return SH_FLOAT_MAT2x4;
                    default: UNREACHABLE();
                  }
                case 3:
                  switch (type.getRows())
                  {
                    case 2: return SH_FLOAT_MAT3x2;
                    case 3: return SH_FLOAT_MAT3;
                    case 4: return SH_FLOAT_MAT3x4;
                    default: UNREACHABLE();
                  }
                case 4:
                  switch (type.getRows())
                  {
                    case 2: return SH_FLOAT_MAT4x2;
                    case 3: return SH_FLOAT_MAT4x3;
                    case 4: return SH_FLOAT_MAT4;
                    default: UNREACHABLE();
                  }
              }
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_FLOAT_VEC2;
                case 3: return SH_FLOAT_VEC3;
                case 4: return SH_FLOAT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_FLOAT;
          }
      case EbtInt:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_INT_VEC2;
                case 3: return SH_INT_VEC3;
                case 4: return SH_INT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_INT;
          }
      case EbtUInt:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_UNSIGNED_INT_VEC2;
                case 3: return SH_UNSIGNED_INT_VEC3;
                case 4: return SH_UNSIGNED_INT_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_UNSIGNED_INT;
          }
      case EbtBool:
          if (type.isMatrix()) {
              UNREACHABLE();
          } else if (type.isVector()) {
              switch (type.getNominalSize()) {
                case 2: return SH_BOOL_VEC2;
                case 3: return SH_BOOL_VEC3;
                case 4: return SH_BOOL_VEC4;
                default: UNREACHABLE();
              }
          } else {
              return SH_BOOL;
          }
      case EbtSampler2D: return SH_SAMPLER_2D;
      case EbtSampler3D: return SH_SAMPLER_3D;
      case EbtSamplerCube: return SH_SAMPLER_CUBE;
      case EbtSamplerExternalOES: return SH_SAMPLER_EXTERNAL_OES;
      case EbtSampler2DRect: return SH_SAMPLER_2D_RECT_ARB;
      case EbtSampler2DArray: return SH_SAMPLER_2D_ARRAY;
      case EbtISampler2D: return SH_INT_SAMPLER_2D;
      case EbtISampler3D: return SH_INT_SAMPLER_3D;
      case EbtISamplerCube: return SH_INT_SAMPLER_CUBE;
      case EbtISampler2DArray: return SH_INT_SAMPLER_2D_ARRAY;
      case EbtUSampler2D: return SH_UNSIGNED_INT_SAMPLER_2D;
      case EbtUSampler3D: return SH_UNSIGNED_INT_SAMPLER_3D;
      case EbtUSamplerCube: return SH_UNSIGNED_INT_SAMPLER_CUBE;
      case EbtUSampler2DArray: return SH_UNSIGNED_INT_SAMPLER_2D_ARRAY;
      case EbtSampler2DShadow: return SH_SAMPLER_2D_SHADOW;
      case EbtSamplerCubeShadow: return SH_SAMPLER_CUBE_SHADOW;
      case EbtSampler2DArrayShadow: return SH_SAMPLER_2D_ARRAY_SHADOW;
      default: UNREACHABLE();
    }
    return SH_NONE;
}

static void getBuiltInVariableInfo(const TType& type,
                                   const TString& name,
                                   const TString& mappedName,
                                   TVariableInfoList& infoList);
static void getUserDefinedVariableInfo(const TType& type,
                                       const TString& name,
                                       const TString& mappedName,
                                       TVariableInfoList& infoList,
                                       ShHashFunction64 hashFunction);

// Returns info for an attribute or uniform.
static void getVariableInfo(const TType& type,
                            const TString& name,
                            const TString& mappedName,
                            TVariableInfoList& infoList,
                            ShHashFunction64 hashFunction)
{
    if (type.getBasicType() == EbtStruct) {
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

void getBuiltInVariableInfo(const TType& type,
                            const TString& name,
                            const TString& mappedName,
                            TVariableInfoList& infoList)
{
    ASSERT(type.getBasicType() != EbtStruct);

    TVariableInfo varInfo;
    if (type.isArray()) {
        varInfo.name = (name + "[0]").c_str();
        varInfo.mappedName = (mappedName + "[0]").c_str();
        varInfo.size = type.getArraySize();
    } else {
        varInfo.name = name.c_str();
        varInfo.mappedName = mappedName.c_str();
        varInfo.size = 1;
    }
    varInfo.precision = type.getPrecision();
    varInfo.type = getVariableDataType(type);
    infoList.push_back(varInfo);
}

void getUserDefinedVariableInfo(const TType& type,
                                const TString& name,
                                const TString& mappedName,
                                TVariableInfoList& infoList,
                                ShHashFunction64 hashFunction)
{
    ASSERT(type.getBasicType() == EbtStruct || type.isInterfaceBlock());

    const TFieldList& fields = type.getStruct()->fields();
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

TVariableInfo::TVariableInfo()
{
}

TVariableInfo::TVariableInfo(ShDataType type, int size)
    : type(type),
      size(size)
{
}

CollectVariables::CollectVariables(TVariableInfoList& attribs,
                                   TVariableInfoList& uniforms,
                                   TVariableInfoList& varyings,
                                   ShHashFunction64 hashFunction)
    : mAttribs(attribs),
      mUniforms(uniforms),
      mVaryings(varyings),
      mHashFunction(hashFunction)
{
}

// We are only interested in attribute and uniform variable declaration.
void CollectVariables::visitSymbol(TIntermSymbol*)
{
}

void CollectVariables::visitConstantUnion(TIntermConstantUnion*)
{
}

bool CollectVariables::visitBinary(Visit, TIntermBinary*)
{
    return false;
}

bool CollectVariables::visitUnary(Visit, TIntermUnary*)
{
    return false;
}

bool CollectVariables::visitSelection(Visit, TIntermSelection*)
{
    return false;
}

bool CollectVariables::visitAggregate(Visit, TIntermAggregate* node)
{
    bool visitChildren = false;

    switch (node->getOp())
    {
    case EOpSequence:
        // We need to visit sequence children to get to variable declarations.
        visitChildren = true;
        break;
    case EOpDeclaration: {
        const TIntermSequence& sequence = node->getSequence();
        TQualifier qualifier = sequence.front()->getAsTyped()->getQualifier();
        if (qualifier == EvqAttribute || qualifier == EvqVertexIn || qualifier == EvqUniform ||
            qualifier == EvqVaryingIn || qualifier == EvqVaryingOut ||
            qualifier == EvqInvariantVaryingIn || qualifier == EvqInvariantVaryingOut)
        {
            TVariableInfoList *infoList = NULL;

            switch (qualifier)
            {
              case EvqAttribute:
              case EvqVertexIn:
                infoList = &mAttribs;
                break;
              case EvqUniform:
                infoList = &mUniforms;
                break;
              default:
                infoList = &mVaryings;
                break;
            }

            for (TIntermSequence::const_iterator i = sequence.begin();
                 i != sequence.end(); ++i)
            {
                const TIntermSymbol* variable = (*i)->getAsSymbolNode();
                // The only case in which the sequence will not contain a
                // TIntermSymbol node is initialization. It will contain a
                // TIntermBinary node in that case. Since attributes and uniforms
                // cannot be initialized in a shader, we must have only
                // TIntermSymbol nodes in the sequence.
                ASSERT(variable != NULL);
                TString processedSymbol;
                if (mHashFunction == NULL)
                    processedSymbol = variable->getSymbol();
                else
                    processedSymbol = TIntermTraverser::hash(variable->getOriginalSymbol(), mHashFunction);
                getVariableInfo(variable->getType(),
                                variable->getOriginalSymbol(),
                                processedSymbol,
                                *infoList,
                                mHashFunction);
            }
        }
        break;
    }
    default: break;
    }

    return visitChildren;
}

bool CollectVariables::visitLoop(Visit, TIntermLoop*)
{
    return false;
}

bool CollectVariables::visitBranch(Visit, TIntermBranch*)
{
    return false;
}


//
// Copyright 2025 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/wgsl/WGSLProgramPrelude.h"
#include "common/log_utils.h"
#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/util.h"
#include "compiler/translator/wgsl/Utils.h"

namespace sh
{

namespace
{
constexpr ImmutableString kEndParanthesis = ImmutableString(")");

void EmitConstructorList(TInfoSinkBase &sink, const TType &type, ImmutableString scalar)
{
    ASSERT(!type.isArray());
    ASSERT(!type.getStruct());

    sink << "(";
    size_t numScalars = 1;
    if (type.isMatrix())
    {
        numScalars = type.getCols() * type.getRows();
    }
    for (size_t i = 0; i < numScalars; i++)
    {
        if (i != 0)
        {
            sink << ", ";
        }
        sink << scalar;
    }
    sink << ")";
}
}  // namespace

WGSLWrapperFunction WGSLProgramPrelude::preIncrement(const TType &incrementedType)
{
    mPreIncrementedTypes.insert(incrementedType);
    switch (incrementedType.getQualifier())
    {
        case EvqTemporary:
        // NOTE: As of Sept 2025, parameters are immutable in WGSL (and are handled by an AST pass
        // that copies parameters to temporaries). Include these here in case parameters become
        // mutable in the future.
        case EvqParamIn:
        case EvqParamOut:
        case EvqParamInOut:
            return {ImmutableString("preIncFunc(&"), kEndParanthesis};
        default:
            // EvqGlobal and various other shader outputs/builtins are all globals.
            return {ImmutableString("preIncPriv(&"), kEndParanthesis};
    }
}

WGSLWrapperFunction WGSLProgramPrelude::preDecrement(const TType &decrementedType)
{
    mPreDecrementedTypes.insert(decrementedType);
    switch (decrementedType.getQualifier())
    {
        case EvqTemporary:
        // NOTE: As of Sept 2025, parameters are immutable in WGSL (and are handled by an AST pass
        // that copies parameters to temporaries). Include these here in case parameters become
        // mutable in the future.
        case EvqParamIn:
        case EvqParamOut:
        case EvqParamInOut:
            return {ImmutableString("preDecFunc(&"), kEndParanthesis};
        default:
            // EvqGlobal and various other shader outputs/builtins are all globals.
            return {ImmutableString("preDecPriv(&"), kEndParanthesis};
    }
}

WGSLWrapperFunction WGSLProgramPrelude::postIncrement(const TType &incrementedType)
{
    mPostIncrementedTypes.insert(incrementedType);
    switch (incrementedType.getQualifier())
    {
        case EvqTemporary:
        // NOTE: As of Sept 2025, parameters are immutable in WGSL (and are handled by an AST pass
        // that copies parameters to temporaries). Include these here in case parameters become
        // mutable in the future.
        case EvqParamIn:
        case EvqParamOut:
        case EvqParamInOut:
            return {ImmutableString("postIncFunc(&"), kEndParanthesis};
        default:
            // EvqGlobal and various other shader outputs/builtins are all globals.
            return {ImmutableString("postIncPriv(&"), kEndParanthesis};
    }
}

WGSLWrapperFunction WGSLProgramPrelude::postDecrement(const TType &decrementedType)
{
    mPostDecrementedTypes.insert(decrementedType);
    switch (decrementedType.getQualifier())
    {
        case EvqTemporary:
        // NOTE: As of Sept 2025, parameters are immutable in WGSL (and are handled by an AST pass
        // that copies parameters to temporaries). Include these here in case parameters become
        // mutable in the future.
        case EvqParamIn:
        case EvqParamOut:
        case EvqParamInOut:
            return {ImmutableString("postDecFunc(&"), kEndParanthesis};
        default:
            // EvqGlobal and various other shader outputs/builtins are all globals.
            return {ImmutableString("postDecPriv(&"), kEndParanthesis};
    }
}

void WGSLProgramPrelude::outputPrelude(TInfoSinkBase &sink)
{
    auto genPreIncOrDec = [&](ImmutableString addressSpace, const TType &type, ImmutableString op,
                              ImmutableString funcName) {
        TStringStream typeStr;
        WriteWgslType(typeStr, type, {});

        sink << "fn " << funcName << "(x : ptr<" << addressSpace << ", " << typeStr.str()
             << ">) -> " << typeStr.str() << " {\n";
        sink << "  (*x) " << op << " " << typeStr.str();
        EmitConstructorList(sink, type, ImmutableString("1"));
        sink << ";\n";
        sink << "  return *x;\n";
        sink << "}\n";
    };
    for (const TType &type : mPreIncrementedTypes)
    {

        // NOTE: it's easiest just to generate increments and decrements functions for variables
        // that live in either the function-local scope or the module-local scope. TType holds a
        // qualifier, but its operator== and operator< ignore the qualifier. We could keep track of
        // the qualifiers used but that's overkill.
        genPreIncOrDec(ImmutableString("private"), type, ImmutableString("+="),
                       ImmutableString("preIncPriv"));
        genPreIncOrDec(ImmutableString("function"), type, ImmutableString("+="),
                       ImmutableString("preIncFunc"));
    }
    for (const TType &type : mPreDecrementedTypes)
    {

        // NOTE: it's easiest just to generate increments and decrements functions for variables
        // that live in either the function-local scope or the module-local scope. TType holds a
        // qualifier, but its operator== and operator< ignore the qualifier. We could keep track of
        // the qualifiers used but that's overkill.
        genPreIncOrDec(ImmutableString("private"), type, ImmutableString("-="),
                       ImmutableString("preDecPriv"));
        genPreIncOrDec(ImmutableString("function"), type, ImmutableString("-="),
                       ImmutableString("preDecFunc"));
    }

    auto genPostIncOrDec = [&](ImmutableString addressSpace, const TType &type, ImmutableString op,
                               ImmutableString funcName) {
        TStringStream typeStr;
        WriteWgslType(typeStr, type, {});

        sink << "fn " << funcName << "(x : ptr<" << addressSpace << ", " << typeStr.str()
             << ">) -> " << typeStr.str() << " {\n";
        sink << "  var old = *x;\n";
        sink << "  (*x) " << op << " " << typeStr.str();
        EmitConstructorList(sink, type, ImmutableString("1"));
        sink << ";\n";
        sink << "  return old;\n";
        sink << "}\n";
    };
    for (const TType &type : mPostIncrementedTypes)
    {
        // NOTE: it's easiest just to generate increments and decrements functions for variables
        // that live in either the function-local scope or the module-local scope. TType holds a
        // qualifier, but its operator== and operator< ignore the qualifier. We could keep track of
        // the qualifiers used but that's overkill.
        genPostIncOrDec(ImmutableString("private"), type, ImmutableString("+="),
                        ImmutableString("postIncPriv"));
        genPostIncOrDec(ImmutableString("function"), type, ImmutableString("+="),
                        ImmutableString("postIncFunc"));
    }
    for (const TType &type : mPostDecrementedTypes)
    {
        // NOTE: it's easiest just to generate increments and decrements functions for variables
        // that live in either the function-local scope or the module-local scope. TType holds a
        // qualifier, but its operator== and operator< ignore the qualifier. We could keep track of
        // the qualifiers used but that's overkill.
        genPostIncOrDec(ImmutableString("private"), type, ImmutableString("-="),
                        ImmutableString("postDecPriv"));
        genPostIncOrDec(ImmutableString("function"), type, ImmutableString("-="),
                        ImmutableString("postDecFunc"));
    }
}

}  // namespace sh

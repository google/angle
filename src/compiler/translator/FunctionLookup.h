//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FunctionLookup.h: Used for storing function calls that have not yet been resolved during parsing.
//

#ifndef COMPILER_TRANSLATOR_FUNCTIONLOOKUP_H_
#define COMPILER_TRANSLATOR_FUNCTIONLOOKUP_H_

#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/IntermNode.h"

namespace sh
{

// A function look-up.
class TFunctionLookup : angle::NonCopyable
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();

    static TFunctionLookup *CreateConstructor(const TType *type);
    static TFunctionLookup *CreateFunctionCall(const ImmutableString &name);

    const ImmutableString &name() const;
    ImmutableString getMangledName() const;
    static ImmutableString GetMangledName(const char *functionName,
                                          const TIntermSequence &arguments);

    bool isConstructor() const;
    const TType &constructorType() const;

    void setThisNode(TIntermTyped *thisNode);
    TIntermTyped *thisNode() const;

    void addArgument(TIntermTyped *argument);
    TIntermSequence &arguments();

  private:
    TFunctionLookup(const ImmutableString &name, const TType *constructorType);

    const ImmutableString mName;
    const TType *const mConstructorType;
    TIntermTyped *mThisNode;
    TIntermSequence mArguments;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_FUNCTIONLOOKUP_H_

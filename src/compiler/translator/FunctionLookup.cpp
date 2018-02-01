//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FunctionLookup.cpp: Used for storing function calls that have not yet been resolved during
// parsing.
//

#include "compiler/translator/FunctionLookup.h"

namespace sh
{

namespace
{

const char kFunctionMangledNameSeparator = '(';

}  // anonymous namespace

TFunctionLookup::TFunctionLookup(const TString *name, const TType *constructorType)
    : mName(name), mConstructorType(constructorType), mThisNode(nullptr)
{
}

// static
TFunctionLookup *TFunctionLookup::CreateConstructor(const TType *type)
{
    ASSERT(type != nullptr);
    return new TFunctionLookup(nullptr, type);
}

// static
TFunctionLookup *TFunctionLookup::CreateFunctionCall(const TString *name)
{
    ASSERT(name != nullptr);
    return new TFunctionLookup(name, nullptr);
}

const TString &TFunctionLookup::name() const
{
    return *mName;
}

const TString &TFunctionLookup::getMangledName() const
{
    return GetMangledName(*mName, mArguments);
}

const TString &TFunctionLookup::GetMangledName(const TString &functionName,
                                               const TIntermSequence &arguments)
{
    std::string newName = functionName.c_str();
    newName += kFunctionMangledNameSeparator;

    for (TIntermNode *argument : arguments)
    {
        newName += argument->getAsTyped()->getType().getMangledName();
    }
    return *NewPoolTString(newName.c_str());
}

bool TFunctionLookup::isConstructor() const
{
    return mConstructorType != nullptr;
}

const TType &TFunctionLookup::constructorType() const
{
    return *mConstructorType;
}

void TFunctionLookup::setThisNode(TIntermTyped *thisNode)
{
    mThisNode = thisNode;
}

TIntermTyped *TFunctionLookup::thisNode() const
{
    return mThisNode;
}

void TFunctionLookup::addArgument(TIntermTyped *argument)
{
    mArguments.push_back(argument);
}

TIntermSequence &TFunctionLookup::arguments()
{
    return mArguments;
}

}  // namespace sh

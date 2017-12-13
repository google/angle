//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Symbol.cpp: Symbols representing variables, functions, structures and interface blocks.
//

#if defined(_MSC_VER)
#pragma warning(disable : 4718)
#endif

#include "compiler/translator/Symbol.h"

#include "compiler/translator/SymbolTable.h"

namespace sh
{

namespace
{

static const char kFunctionMangledNameSeparator = '(';

}  // anonymous namespace

TSymbol::TSymbol(TSymbolTable *symbolTable,
                 const TString *name,
                 SymbolType symbolType,
                 TExtension extension)
    : mName(name),
      mUniqueId(symbolTable->nextUniqueId()),
      mSymbolType(symbolType),
      mExtension(extension)
{
    ASSERT(mSymbolType == SymbolType::BuiltIn || mExtension == TExtension::UNDEFINED);
    ASSERT(mName != nullptr || mSymbolType == SymbolType::AngleInternal ||
           mSymbolType == SymbolType::NotResolved || mSymbolType == SymbolType::Empty);
}

const TString *TSymbol::name() const
{
    if (mName != nullptr || mSymbolType == SymbolType::Empty)
    {
        return mName;
    }
    ASSERT(mSymbolType == SymbolType::AngleInternal);
    TInfoSinkBase symbolNameOut;
    symbolNameOut << "s" << mUniqueId.get();
    return NewPoolTString(symbolNameOut.c_str());
}

const TString &TSymbol::getMangledName() const
{
    ASSERT(mSymbolType != SymbolType::Empty);
    return *name();
}

TVariable::TVariable(TSymbolTable *symbolTable,
                     const TString *name,
                     const TType &t,
                     SymbolType symbolType,
                     TExtension extension)
    : TSymbol(symbolTable, name, symbolType, extension), type(t), unionArray(nullptr)
{
}

TStructure::TStructure(TSymbolTable *symbolTable,
                       const TString *name,
                       const TFieldList *fields,
                       SymbolType symbolType)
    : TSymbol(symbolTable, name, symbolType), TFieldListCollection(fields)
{
}

void TStructure::createSamplerSymbols(const TString &namePrefix,
                                      const TString &apiNamePrefix,
                                      TVector<TIntermSymbol *> *outputSymbols,
                                      TMap<TIntermSymbol *, TString> *outputSymbolsToAPINames,
                                      TSymbolTable *symbolTable) const
{
    ASSERT(containsSamplers());
    for (const auto *field : *mFields)
    {
        const TType *fieldType = field->type();
        if (IsSampler(fieldType->getBasicType()) || fieldType->isStructureContainingSamplers())
        {
            TString fieldName    = namePrefix + "_" + field->name();
            TString fieldApiName = apiNamePrefix + "." + field->name();
            fieldType->createSamplerSymbols(fieldName, fieldApiName, outputSymbols,
                                            outputSymbolsToAPINames, symbolTable);
        }
    }
}

void TStructure::setName(const TString &name)
{
    TString *mutableName = const_cast<TString *>(mName);
    *mutableName         = name;
}

TInterfaceBlock::TInterfaceBlock(TSymbolTable *symbolTable,
                                 const TString *name,
                                 const TFieldList *fields,
                                 const TLayoutQualifier &layoutQualifier,
                                 SymbolType symbolType,
                                 TExtension extension)
    : TSymbol(symbolTable, name, symbolType, extension),
      TFieldListCollection(fields),
      mBlockStorage(layoutQualifier.blockStorage),
      mBinding(layoutQualifier.binding)
{
    ASSERT(name != nullptr);
}

TFunction::TFunction(TSymbolTable *symbolTable,
                     const TString *name,
                     const TType *retType,
                     SymbolType symbolType,
                     bool knownToNotHaveSideEffects,
                     TOperator tOp,
                     TExtension extension)
    : TSymbol(symbolTable, name, symbolType, extension),
      returnType(retType),
      mangledName(nullptr),
      op(tOp),
      defined(false),
      mHasPrototypeDeclaration(false),
      mKnownToNotHaveSideEffects(knownToNotHaveSideEffects)
{
    // Functions with an empty name are not allowed.
    ASSERT(symbolType != SymbolType::Empty);
    ASSERT(name != nullptr || symbolType == SymbolType::AngleInternal || tOp != EOpNull);
}

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
    clearParameters();
}

void TFunction::clearParameters()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
    parameters.clear();
    mangledName = nullptr;
}

void TFunction::swapParameters(const TFunction &parametersSource)
{
    clearParameters();
    for (auto parameter : parametersSource.parameters)
    {
        addParameter(parameter);
    }
}

const TString *TFunction::buildMangledName() const
{
    std::string newName = name()->c_str();
    newName += kFunctionMangledNameSeparator;

    for (const auto &p : parameters)
    {
        newName += p.type->getMangledName();
    }
    return NewPoolTString(newName.c_str());
}

const TString &TFunction::GetMangledNameFromCall(const TString &functionName,
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

}  // namespace sh

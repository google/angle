//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Symbol.h: Symbols representing variables, functions, structures and interface blocks.
//

#ifndef COMPILER_TRANSLATOR_SYMBOL_H_
#define COMPILER_TRANSLATOR_SYMBOL_H_

#include "common/angleutils.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolUniqueId.h"

namespace sh
{

class TSymbolTable;

enum class SymbolType
{
    BuiltIn,
    UserDefined,
    AngleInternal,
    Empty,  // Meaning symbol without a name.
    NotResolved
};

// Symbol base class. (Can build functions or variables out of these...)
class TSymbol : angle::NonCopyable
{
  public:
    POOL_ALLOCATOR_NEW_DELETE();
    TSymbol(TSymbolTable *symbolTable,
            const TString *name,
            SymbolType symbolType,
            TExtension extension = TExtension::UNDEFINED);

    virtual ~TSymbol()
    {
        // don't delete name, it's from the pool
    }

    const TString *name() const;
    virtual const TString &getMangledName() const;
    virtual bool isFunction() const { return false; }
    virtual bool isVariable() const { return false; }
    virtual bool isStruct() const { return false; }
    const TSymbolUniqueId &uniqueId() const { return mUniqueId; }
    SymbolType symbolType() const { return mSymbolType; }
    TExtension extension() const { return mExtension; }

  protected:
    const TString *const mName;

  private:
    const TSymbolUniqueId mUniqueId;
    const SymbolType mSymbolType;
    const TExtension mExtension;
};

// Variable.
// May store the value of a constant variable of any type (float, int, bool or struct).
class TVariable : public TSymbol
{
  public:
    TVariable(TSymbolTable *symbolTable,
              const TString *name,
              const TType &t,
              SymbolType symbolType,
              TExtension ext = TExtension::UNDEFINED);

    ~TVariable() override {}
    bool isVariable() const override { return true; }
    TType &getType() { return type; }
    const TType &getType() const { return type; }
    void setQualifier(TQualifier qualifier) { type.setQualifier(qualifier); }

    const TConstantUnion *getConstPointer() const { return unionArray; }

    void shareConstPointer(const TConstantUnion *constArray) { unionArray = constArray; }

  private:
    TType type;
    const TConstantUnion *unionArray;
};

// Struct type.
class TStructure : public TSymbol, public TFieldListCollection
{
  public:
    TStructure(TSymbolTable *symbolTable,
               const TString *name,
               const TFieldList *fields,
               SymbolType symbolType);

    bool isStruct() const override { return true; }

    void createSamplerSymbols(const TString &namePrefix,
                              const TString &apiNamePrefix,
                              TVector<TIntermSymbol *> *outputSymbols,
                              TMap<TIntermSymbol *, TString> *outputSymbolsToAPINames,
                              TSymbolTable *symbolTable) const;

    void setAtGlobalScope(bool atGlobalScope) { mAtGlobalScope = atGlobalScope; }
    bool atGlobalScope() const { return mAtGlobalScope; }

  private:
    // TODO(zmo): Find a way to get rid of the const_cast in function
    // setName().  At the moment keep this function private so only
    // friend class RegenerateStructNames may call it.
    friend class RegenerateStructNames;
    void setName(const TString &name);

    bool mAtGlobalScope;
};

// Interface block. Note that this contains the block name, not the instance name. Interface block
// instances are stored as TVariable.
class TInterfaceBlock : public TSymbol, public TFieldListCollection
{
  public:
    TInterfaceBlock(TSymbolTable *symbolTable,
                    const TString *name,
                    const TFieldList *fields,
                    const TLayoutQualifier &layoutQualifier,
                    SymbolType symbolType,
                    TExtension extension = TExtension::UNDEFINED);

    TLayoutBlockStorage blockStorage() const { return mBlockStorage; }
    int blockBinding() const { return mBinding; }

  private:
    TLayoutBlockStorage mBlockStorage;
    int mBinding;

    // Note that we only record matrix packing on a per-field granularity.
};

// Immutable version of TParameter.
struct TConstParameter
{
    TConstParameter() : name(nullptr), type(nullptr) {}
    explicit TConstParameter(const TString *n) : name(n), type(nullptr) {}
    explicit TConstParameter(const TType *t) : name(nullptr), type(t) {}
    TConstParameter(const TString *n, const TType *t) : name(n), type(t) {}

    // Both constructor arguments must be const.
    TConstParameter(TString *n, TType *t)       = delete;
    TConstParameter(const TString *n, TType *t) = delete;
    TConstParameter(TString *n, const TType *t) = delete;

    const TString *const name;
    const TType *const type;
};

// The function sub-class of symbols and the parser will need to
// share this definition of a function parameter.
struct TParameter
{
    // Destructively converts to TConstParameter.
    // This method resets name and type to nullptrs to make sure
    // their content cannot be modified after the call.
    TConstParameter turnToConst()
    {
        const TString *constName = name;
        const TType *constType   = type;
        name                     = nullptr;
        type                     = nullptr;
        return TConstParameter(constName, constType);
    }

    const TString *name;
    TType *type;
};

// The function sub-class of a symbol.
class TFunction : public TSymbol
{
  public:
    TFunction(TSymbolTable *symbolTable,
              const TString *name,
              const TType *retType,
              SymbolType symbolType,
              bool knownToNotHaveSideEffects,
              TOperator tOp        = EOpNull,
              TExtension extension = TExtension::UNDEFINED);

    ~TFunction() override;
    bool isFunction() const override { return true; }

    void addParameter(const TConstParameter &p)
    {
        parameters.push_back(p);
        mangledName = nullptr;
    }

    void swapParameters(const TFunction &parametersSource);

    const TString &getMangledName() const override
    {
        if (mangledName == nullptr)
        {
            mangledName = buildMangledName();
        }
        return *mangledName;
    }

    static const TString &GetMangledNameFromCall(const TString &functionName,
                                                 const TIntermSequence &arguments);

    const TType &getReturnType() const { return *returnType; }

    TOperator getBuiltInOp() const { return op; }

    void setDefined() { defined = true; }
    bool isDefined() { return defined; }
    void setHasPrototypeDeclaration() { mHasPrototypeDeclaration = true; }
    bool hasPrototypeDeclaration() const { return mHasPrototypeDeclaration; }

    size_t getParamCount() const { return parameters.size(); }
    const TConstParameter &getParam(size_t i) const { return parameters[i]; }

    bool isKnownToNotHaveSideEffects() const { return mKnownToNotHaveSideEffects; }

  private:
    void clearParameters();

    const TString *buildMangledName() const;

    typedef TVector<TConstParameter> TParamList;
    TParamList parameters;
    const TType *returnType;
    mutable const TString *mangledName;
    // TODO(oetuaho): Remove op from TFunction once TFunction is not used for looking up builtins or
    // constructors.
    TOperator op;
    bool defined;
    bool mHasPrototypeDeclaration;
    bool mKnownToNotHaveSideEffects;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_SYMBOL_H_

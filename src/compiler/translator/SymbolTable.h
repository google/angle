//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_SYMBOLTABLE_H_
#define COMPILER_TRANSLATOR_SYMBOLTABLE_H_

//
// Symbol table for parsing.  Has these design characteristics:
//
// * Same symbol table can be used to compile many shaders, to preserve
//   effort of creating and loading with the large numbers of built-in
//   symbols.
//
// * Name mangling will be used to give each function a unique name
//   so that symbol table lookups are never ambiguous.  This allows
//   a simpler symbol table structure.
//
// * Pushing and popping of scope, so symbol table will really be a stack
//   of symbol tables.  Searched from the top, with new inserts going into
//   the top.
//
// * Constants:  Compile time constant symbols will keep their values
//   in the symbol table.  The parser can substitute constants at parse
//   time, including doing constant folding and constant propagation.
//
// * No temporaries:  Temporaries made from operations (+, --, .xy, etc.)
//   are tracked in the intermediate representation, not the symbol table.
//

#include <array>
#include <memory>

#include "common/angleutils.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/Symbol.h"

namespace sh
{

// Define ESymbolLevel as int rather than an enum so that we can do arithmetic on it.
typedef int ESymbolLevel;
const int COMMON_BUILTINS    = 0;
const int ESSL1_BUILTINS     = 1;
const int ESSL3_BUILTINS     = 2;
const int ESSL3_1_BUILTINS   = 3;
// GLSL_BUILTINS are desktop GLSL builtins that don't exist in ESSL but are used to implement
// features in ANGLE's GLSL backend. They're not visible to the parser.
const int GLSL_BUILTINS      = 4;
const int LAST_BUILTIN_LEVEL = GLSL_BUILTINS;

struct UnmangledBuiltIn
{
    constexpr UnmangledBuiltIn(TExtension extension) : extension(extension) {}

    TExtension extension;
};

class TSymbolTable : angle::NonCopyable
{
  public:
    TSymbolTable();
    // To start using the symbol table after construction:
    // * initializeBuiltIns() needs to be called.
    // * push() needs to be called to push the global level.

    ~TSymbolTable();

    bool isEmpty() const;
    bool atGlobalLevel() const;

    void push();
    void pop();

    // Declare a non-function symbol at the current scope. Return true in case the declaration was
    // successful, and false if the declaration failed due to redefinition.
    bool declare(TSymbol *symbol);

    // Functions are always declared at global scope.
    void declareUserDefinedFunction(TFunction *function, bool insertUnmangledName);

    // These return the TFunction pointer to keep using to refer to this function.
    const TFunction *markFunctionHasPrototypeDeclaration(const ImmutableString &mangledName,
                                                         bool *hadPrototypeDeclarationOut);
    const TFunction *setFunctionParameterNamesFromDefinition(const TFunction *function,
                                                             bool *wasDefinedOut);

    // find() is guaranteed not to retain a reference to the ImmutableString, so an ImmutableString
    // with a reference to a short-lived char * is fine to pass here.
    const TSymbol *find(const ImmutableString &name, int shaderVersion) const;

    const TSymbol *findGlobal(const ImmutableString &name) const;

    const TSymbol *findBuiltIn(const ImmutableString &name, int shaderVersion) const;

    const TSymbol *findBuiltIn(const ImmutableString &name,
                               int shaderVersion,
                               bool includeGLSLBuiltins) const;

    void setDefaultPrecision(TBasicType type, TPrecision prec);

    // Searches down the precisionStack for a precision qualifier
    // for the specified TBasicType
    TPrecision getDefaultPrecision(TBasicType type) const;

    // This records invariant varyings declared through
    // "invariant varying_name;".
    void addInvariantVarying(const ImmutableString &originalName);

    // If this returns false, the varying could still be invariant
    // if it is set as invariant during the varying variable
    // declaration - this piece of information is stored in the
    // variable's type, not here.
    bool isVaryingInvariant(const ImmutableString &originalName) const;

    void setGlobalInvariant(bool invariant);

    const TSymbolUniqueId nextUniqueId() { return TSymbolUniqueId(this); }

    // Gets the built-in accessible by a shader with the specified version, if any.
    const UnmangledBuiltIn *getUnmangledBuiltInForShaderVersion(const ImmutableString &name,
                                                                int shaderVersion);

    void initializeBuiltIns(sh::GLenum type,
                            ShShaderSpec spec,
                            const ShBuiltInResources &resources);
    void clearCompilationResults();

  private:
    friend class TSymbolUniqueId;
    int nextUniqueIdValue();

    class TSymbolTableBuiltInLevel;
    class TSymbolTableLevel;

    void pushBuiltInLevel();

    // The insert* entry points are used when initializing the symbol table with built-ins.
    void insertVariable(ESymbolLevel level, const ImmutableString &name, const TType *type);
    void insertVariableExt(ESymbolLevel level,
                           TExtension ext,
                           const ImmutableString &name,
                           const TType *type);

    template <TPrecision precision>
    void insertConstInt(ESymbolLevel level, const ImmutableString &name, int value);

    template <TPrecision precision>
    void insertConstIntExt(ESymbolLevel level,
                           TExtension ext,
                           const ImmutableString &name,
                           int value);

    template <TPrecision precision>
    void insertConstIvec3(ESymbolLevel level,
                          const ImmutableString &name,
                          const std::array<int, 3> &values);

    TVariable *insertVariable(ESymbolLevel level,
                              const ImmutableString &name,
                              const TType *type,
                              SymbolType symbolType);

    void insertBuiltIn(ESymbolLevel level, const TSymbol *symbol);

    TFunction *findUserDefinedFunction(const ImmutableString &name) const;

    void initSamplerDefaultPrecision(TBasicType samplerType);

    void initializeBuiltInVariables(sh::GLenum type,
                                    ShShaderSpec spec,
                                    const ShBuiltInResources &resources);
    void markBuiltInInitializationFinished();

    void insertBuiltInFunctions(sh::GLenum shaderType);

    std::vector<std::unique_ptr<TSymbolTableBuiltInLevel>> mBuiltInTable;
    std::vector<std::unique_ptr<TSymbolTableLevel>> mTable;

    // There's one precision stack level for predefined precisions and then one level for each scope
    // in table.
    typedef TMap<TBasicType, TPrecision> PrecisionStackLevel;
    std::vector<std::unique_ptr<PrecisionStackLevel>> mPrecisionStack;

    int mUniqueIdCounter;

    static const int kLastStaticBuiltInId;

    // -1 before built-in init has finished, one past the last built-in id afterwards.
    // TODO(oetuaho): Make this a compile-time constant once the symbol table is initialized at
    // compile time. http://anglebug.com/1432
    int mUserDefinedUniqueIdsStart;

    sh::GLenum mShaderType;
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_SYMBOLTABLE_H_

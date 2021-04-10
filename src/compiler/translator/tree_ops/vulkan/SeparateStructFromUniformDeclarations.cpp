//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// SeparateStructFromUniformDeclarations: Separate struct declarations from uniform declarations.
//

#include "compiler/translator/tree_ops/vulkan/SeparateStructFromUniformDeclarations.h"

#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/ReplaceVariable.h"

namespace sh
{
namespace
{
// This traverser translates embedded uniform structs into a specifier and declaration.
// This makes the declarations easier to move into uniform blocks.
class Traverser : public TIntermTraverser
{
  public:
    explicit Traverser(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable)
    {}

    bool visitDeclaration(Visit visit, TIntermDeclaration *decl) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return false;
        }

        const TIntermSequence &sequence = *(decl->getSequence());
        ASSERT(sequence.size() == 1);
        TIntermTyped *declarator = sequence.front()->getAsTyped();
        const TType &type        = declarator->getType();

        if (type.isStructSpecifier() && type.getQualifier() == EvqUniform)
        {
            doReplacement(decl, declarator, type.getStruct());
        }

        return false;
    }

    void visitSymbol(TIntermSymbol *symbol) override
    {
        const TVariable *variable = &symbol->variable();
        if (mVariableMap.count(variable) > 0)
        {
            queueReplacement(mVariableMap[variable]->deepCopy(), OriginalNode::IS_DROPPED);
        }
    }

  private:
    void doReplacement(TIntermDeclaration *decl,
                       TIntermTyped *declarator,
                       const TStructure *oldStructure)
    {
        // struct <structName> { ... };
        const TStructure *structure = oldStructure;
        if (oldStructure->symbolType() == SymbolType::Empty)
        {
            structure = new TStructure(mSymbolTable, kEmptyImmutableString, &oldStructure->fields(),
                                       SymbolType::AngleInternal);
        }
        TType *namedType = new TType(structure, true);
        namedType->setQualifier(EvqGlobal);

        TVariable *structVariable =
            new TVariable(mSymbolTable, kEmptyImmutableString, namedType, SymbolType::Empty);
        TIntermSymbol *structDeclarator       = new TIntermSymbol(structVariable);
        TIntermDeclaration *structDeclaration = new TIntermDeclaration;
        structDeclaration->appendDeclarator(structDeclarator);

        TIntermSequence newSequence;
        newSequence.push_back(structDeclaration);

        // uniform <structName> <structUniformName>;
        TIntermSymbol *asSymbol = declarator->getAsSymbolNode();
        if (asSymbol && asSymbol->variable().symbolType() != SymbolType::Empty)
        {
            TIntermDeclaration *namedDecl = new TIntermDeclaration;
            TType *uniformType            = new TType(structure, false);
            uniformType->setQualifier(EvqUniform);

            TVariable *newVar        = new TVariable(mSymbolTable, asSymbol->getName(), uniformType,
                                              asSymbol->variable().symbolType());
            TIntermSymbol *newSymbol = new TIntermSymbol(newVar);
            namedDecl->appendDeclarator(newSymbol);

            newSequence.push_back(namedDecl);

            mVariableMap[&asSymbol->variable()] = new TIntermSymbol(newVar);
        }

        mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), decl,
                                        std::move(newSequence));
    }

    VariableReplacementMap mVariableMap;
};
}  // anonymous namespace

bool SeparateStructFromUniformDeclarations(TCompiler *compiler,
                                           TIntermBlock *root,
                                           TSymbolTable *symbolTable)
{
    Traverser separateStructDecls(symbolTable);
    root->traverse(&separateStructDecls);
    return separateStructDecls.updateTree(compiler, root);
}
}  // namespace sh

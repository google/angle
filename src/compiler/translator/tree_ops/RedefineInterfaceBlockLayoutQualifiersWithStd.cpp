//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RedefineInterfaceBlockLayoutQualifiersWithStd: Ensure layout qualifiers are either std140 or
// std430 to comply with Vulkan GLSL.
//

#include "compiler/translator/tree_ops/RedefineInterfaceBlockLayoutQualifiersWithStd.h"

#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{
// Helper to replace the type of a symbol
TIntermSymbol *RedefineLayoutQualifierOfSymbolNode(TIntermSymbol *symbolNode,
                                                   const TLayoutQualifier &newLayoutQualifier,
                                                   TSymbolTable *symbolTable)
{
    const TVariable &oldVariable = symbolNode->variable();

    ASSERT(symbolNode->getType().isInterfaceBlock());

    const TType &oldType                     = symbolNode->getType();
    const TInterfaceBlock *oldInterfaceBlock = oldType.getInterfaceBlock();

    // Create a new type based on the old type, but the memory layout qualifier changed.
    TType *newType = new TType(oldType);
    newType->setLayoutQualifier(newLayoutQualifier);

    // Create a new interface block based on the old one, with the new memory layout qualifier as
    // well.
    TInterfaceBlock *newInterfaceBlock =
        new TInterfaceBlock(symbolTable, oldInterfaceBlock->name(), &oldInterfaceBlock->fields(),
                            newLayoutQualifier, oldInterfaceBlock->symbolType());

    newType->setInterfaceBlock(newInterfaceBlock);

    // Create a new variable with the modified type, to substitute the old variable.
    TVariable *newVariable =
        new TVariable(oldVariable.uniqueId(), oldVariable.name(), oldVariable.symbolType(),
                      oldVariable.extension(), newType);

    return new TIntermSymbol(newVariable);
}

class Traverser : public TIntermTraverser
{
  public:
    explicit Traverser(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable)
    {
        symbolTable->push();
    }

    ~Traverser() override { mSymbolTable->pop(); }

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return false;
        }

        const TIntermSequence &sequence = *(node->getSequence());
        TIntermTyped *declarator        = sequence.front()->getAsTyped();
        const TType &type               = declarator->getType();

        if (type.isInterfaceBlock())
        {
            ASSERT(declarator->getAsSymbolNode());

            TLayoutQualifier layoutQualifier = type.getLayoutQualifier();

            // If the layout qualifier is not explicitly std140 or std430, change it to std140 for
            // uniforms and std430 otherwise.  See the comment in the header for more information.
            if (layoutQualifier.blockStorage != EbsStd140 &&
                layoutQualifier.blockStorage != EbsStd430)
            {
                layoutQualifier.blockStorage =
                    type.getQualifier() == EvqUniform ? EbsStd140 : EbsStd430;

                TIntermSymbol *replacement = RedefineLayoutQualifierOfSymbolNode(
                    declarator->getAsSymbolNode(), layoutQualifier, mSymbolTable);

                queueReplacementWithParent(node, declarator, replacement, OriginalNode::IS_DROPPED);
            }
        }

        return false;
    }
};
}  // anonymous namespace

void RedefineInterfaceBlockLayoutQualifiersWithStd(TIntermBlock *root, TSymbolTable *symbolTable)
{
    Traverser redefineInterfaceBlockLayoutQualifiersWithStd(symbolTable);
    root->traverse(&redefineInterfaceBlockLayoutQualifiersWithStd);
    redefineInterfaceBlockLayoutQualifiersWithStd.updateTree();
}
}  // namespace sh

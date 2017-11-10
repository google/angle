//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RemoveUnreferencedVariables.cpp:
//  Drop variables that are declared but never referenced in the AST. This avoids adding unnecessary
//  initialization code for them.
//

#include "compiler/translator/RemoveUnreferencedVariables.h"

#include "compiler/translator/IntermTraverse.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

namespace
{

class CollectVariableRefCountsTraverser : public TIntermTraverser
{
  public:
    CollectVariableRefCountsTraverser();

    using RefCountMap = std::unordered_map<int, unsigned int>;
    RefCountMap &getSymbolIdRefCounts() { return mSymbolIdRefCounts; }

    void visitSymbol(TIntermSymbol *node) override;

  private:
    RefCountMap mSymbolIdRefCounts;
};

CollectVariableRefCountsTraverser::CollectVariableRefCountsTraverser()
    : TIntermTraverser(true, false, false)
{
}

void CollectVariableRefCountsTraverser::visitSymbol(TIntermSymbol *node)
{
    auto iter = mSymbolIdRefCounts.find(node->getId());
    if (iter == mSymbolIdRefCounts.end())
    {
        mSymbolIdRefCounts[node->getId()] = 1u;
        return;
    }
    ++(iter->second);
}

// Traverser that removes all unreferenced variables on one traversal.
class RemoveUnreferencedVariablesTraverser : public TIntermTraverser
{
  public:
    RemoveUnreferencedVariablesTraverser(
        CollectVariableRefCountsTraverser::RefCountMap *symbolIdRefCounts,
        TSymbolTable *symbolTable);

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override;
    void visitSymbol(TIntermSymbol *node) override;

    // Traverse loop and block nodes in reverse order. Note that this traverser does not track
    // parent block positions, so insertStatementInParentBlock is unusable!
    void traverseBlock(TIntermBlock *block) override;
    void traverseLoop(TIntermLoop *loop) override;

  private:
    void removeDeclaration(TIntermDeclaration *node, TIntermTyped *declarator);

    CollectVariableRefCountsTraverser::RefCountMap *mSymbolIdRefCounts;
    bool mRemoveReferences;
};

RemoveUnreferencedVariablesTraverser::RemoveUnreferencedVariablesTraverser(
    CollectVariableRefCountsTraverser::RefCountMap *symbolIdRefCounts,
    TSymbolTable *symbolTable)
    : TIntermTraverser(true, false, true, symbolTable),
      mSymbolIdRefCounts(symbolIdRefCounts),
      mRemoveReferences(false)
{
}

void RemoveUnreferencedVariablesTraverser::removeDeclaration(TIntermDeclaration *node,
                                                             TIntermTyped *declarator)
{
    if (declarator->getType().isStructSpecifier() && !declarator->getType().isNamelessStruct())
    {
        // We don't count references to struct types, so if this declaration declares a named struct
        // type, we'll keep it. We can still change the declarator though so that it doesn't declare
        // a variable.
        queueReplacementWithParent(
            node, declarator,
            new TIntermSymbol(mSymbolTable->getEmptySymbolId(), TString(""), declarator->getType()),
            OriginalNode::IS_DROPPED);
        return;
    }

    if (getParentNode()->getAsBlock())
    {
        TIntermSequence emptyReplacement;
        mMultiReplacements.push_back(
            NodeReplaceWithMultipleEntry(getParentNode()->getAsBlock(), node, emptyReplacement));
    }
    else
    {
        ASSERT(getParentNode()->getAsLoopNode());
        queueReplacement(nullptr, OriginalNode::IS_DROPPED);
    }
}

bool RemoveUnreferencedVariablesTraverser::visitDeclaration(Visit visit, TIntermDeclaration *node)
{
    if (visit == PreVisit)
    {
        // SeparateDeclarations should have already been run.
        ASSERT(node->getSequence()->size() == 1u);

        TIntermTyped *declarator = node->getSequence()->back()->getAsTyped();
        ASSERT(declarator);

        // We can only remove variables that are not a part of the shader interface.
        TQualifier qualifier = declarator->getQualifier();
        if (qualifier != EvqTemporary && qualifier != EvqGlobal)
        {
            return true;
        }

        bool canRemove            = false;
        TIntermSymbol *symbolNode = declarator->getAsSymbolNode();
        if (symbolNode != nullptr)
        {
            canRemove = (*mSymbolIdRefCounts)[symbolNode->getId()] == 1u;
        }
        TIntermBinary *initNode = declarator->getAsBinaryNode();
        if (initNode != nullptr)
        {
            ASSERT(initNode->getLeft()->getAsSymbolNode());
            int symbolId = initNode->getLeft()->getAsSymbolNode()->getId();
            canRemove =
                (*mSymbolIdRefCounts)[symbolId] == 1u && !initNode->getRight()->hasSideEffects();
        }

        if (canRemove)
        {
            removeDeclaration(node, declarator);
            mRemoveReferences = true;
        }
        return true;
    }
    ASSERT(visit == PostVisit);
    mRemoveReferences = false;
    return true;
}

void RemoveUnreferencedVariablesTraverser::visitSymbol(TIntermSymbol *node)
{
    if (mRemoveReferences)
    {
        ASSERT(mSymbolIdRefCounts->find(node->getId()) != mSymbolIdRefCounts->end());
        --(*mSymbolIdRefCounts)[node->getId()];
    }
}

void RemoveUnreferencedVariablesTraverser::traverseBlock(TIntermBlock *node)
{
    // We traverse blocks in reverse order.  This way reference counts can be decremented when
    // removing initializers, and variables that become unused when initializers are removed can be
    // removed on the same traversal.

    ScopedNodeInTraversalPath addToPath(this, node);

    bool visit = true;

    TIntermSequence *sequence = node->getSequence();

    if (preVisit)
        visit = visitBlock(PreVisit, node);

    if (visit)
    {
        for (auto iter = sequence->rbegin(); iter != sequence->rend(); ++iter)
        {
            (*iter)->traverse(this);
            if (visit && inVisit)
            {
                if ((iter + 1) != sequence->rend())
                    visit = visitBlock(InVisit, node);
            }
        }
    }

    if (visit && postVisit)
        visitBlock(PostVisit, node);
}

void RemoveUnreferencedVariablesTraverser::traverseLoop(TIntermLoop *node)
{
    // We traverse loops in reverse order as well. The loop body gets traversed before the init
    // node.

    ScopedNodeInTraversalPath addToPath(this, node);

    bool visit = true;

    if (preVisit)
        visit = visitLoop(PreVisit, node);

    if (visit)
    {
        // We don't need to traverse loop expressions or conditions since they can't be declarations
        // in the AST (loops which have a declaration in their condition get transformed in the
        // parsing stage).
        ASSERT(node->getExpression() == nullptr ||
               node->getExpression()->getAsDeclarationNode() == nullptr);
        ASSERT(node->getCondition() == nullptr ||
               node->getCondition()->getAsDeclarationNode() == nullptr);

        if (node->getBody())
            node->getBody()->traverse(this);

        if (node->getInit())
            node->getInit()->traverse(this);
    }

    if (visit && postVisit)
        visitLoop(PostVisit, node);
}

}  // namespace

void RemoveUnreferencedVariables(TIntermBlock *root, TSymbolTable *symbolTable)
{
    CollectVariableRefCountsTraverser collector;
    root->traverse(&collector);
    RemoveUnreferencedVariablesTraverser traverser(&collector.getSymbolIdRefCounts(), symbolTable);
    root->traverse(&traverser);
    traverser.updateTree();
}

}  // namespace sh

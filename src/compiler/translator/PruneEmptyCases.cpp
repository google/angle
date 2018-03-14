//
// Copyright (c) 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// PruneEmptyCases.cpp: The PruneEmptyCases function prunes cases that are followed by nothing from
// the AST.

#include "compiler/translator/PruneEmptyCases.h"

#include "compiler/translator/IntermTraverse.h"
#include "compiler/translator/Symbol.h"

namespace sh
{

namespace
{

class PruneEmptyCasesTraverser : private TIntermTraverser
{
  public:
    static void apply(TIntermBlock *root);

  private:
    PruneEmptyCasesTraverser();
    bool visitSwitch(Visit visit, TIntermSwitch *node) override;
};

void PruneEmptyCasesTraverser::apply(TIntermBlock *root)
{
    PruneEmptyCasesTraverser prune;
    root->traverse(&prune);
    prune.updateTree();
}

PruneEmptyCasesTraverser::PruneEmptyCasesTraverser() : TIntermTraverser(true, false, false)
{
}

bool PruneEmptyCasesTraverser::visitSwitch(Visit visit, TIntermSwitch *node)
{
    TIntermBlock *statementList = node->getStatementList();
    TIntermSequence *statements = statementList->getSequence();

    // Iterate block children in reverse order. Cases that are only followed by other cases are
    // pruned.
    size_t i                  = statements->size();
    bool emptySwitchStatement = true;
    while (i > 0)
    {
        --i;
        TIntermNode *statement = statements->at(i);
        if (statement->getAsCaseNode())
        {
            TIntermSequence emptyReplacement;
            mMultiReplacements.push_back(
                NodeReplaceWithMultipleEntry(statementList, statement, emptyReplacement));
        }
        else
        {
            emptySwitchStatement = false;
            break;
        }
    }
    if (emptySwitchStatement)
    {
        TIntermTyped *init = node->getInit();
        if (init->hasSideEffects())
        {
            queueReplacement(init, OriginalNode::IS_DROPPED);
        }
        else
        {
            TIntermSequence emptyReplacement;
            ASSERT(getParentNode()->getAsBlock());
            mMultiReplacements.push_back(NodeReplaceWithMultipleEntry(getParentNode()->getAsBlock(),
                                                                      node, emptyReplacement));
        }
        return false;
    }

    return true;
}

}  // namespace

void PruneEmptyCases(TIntermBlock *root)
{
    PruneEmptyCasesTraverser::apply(root);
}

}  // namespace sh

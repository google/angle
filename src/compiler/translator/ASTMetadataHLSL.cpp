//
// Copyright (c) 2002-2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Analysis of the AST needed for HLSL generation

#include "compiler/translator/ASTMetadataHLSL.h"

#include "compiler/translator/CallDAG.h"
#include "compiler/translator/SymbolTable.h"

namespace
{

// Class used to traverse the AST of a function definition, checking if the
// function uses a gradient, and writing the set of control flow using gradients.
// It assumes that the analysis has already been made for the function's
// callees.
class PullGradient : public TIntermTraverser
{
  public:
    PullGradient(MetadataList *metadataList, size_t index, const CallDAG &dag)
        : TIntermTraverser(true, false, true),
          mMetadataList(metadataList),
          mMetadata(&(*metadataList)[index]),
          mIndex(index),
          mDag(dag)
    {
        ASSERT(index < metadataList->size());
    }

    void traverse(TIntermAggregate *node)
    {
        node->traverse(this);
        ASSERT(mParents.empty());
    }

    // Called when a gradient operation or a call to a function using a gradient is found.
    void onGradient()
    {
        mMetadata->mUsesGradient = true;
        // Mark the latest control flow as using a gradient.
        if (!mParents.empty())
        {
            mMetadata->mControlFlowsContainingGradient.insert(mParents.back());
        }
    }

    void visitControlFlow(Visit visit, TIntermNode *node)
    {
        if (visit == PreVisit)
        {
            mParents.push_back(node);
        }
        else if (visit == PostVisit)
        {
            ASSERT(mParents.back() == node);
            mParents.pop_back();
            // A control flow's using a gradient means its parents are too.
            if (mMetadata->mControlFlowsContainingGradient.count(node)> 0 && !mParents.empty())
            {
                mMetadata->mControlFlowsContainingGradient.insert(mParents.back());
            }
        }
    }

    bool visitLoop(Visit visit, TIntermLoop *loop)
    {
        visitControlFlow(visit, loop);
        return true;
    }

    bool visitSelection(Visit visit, TIntermSelection *selection)
    {
        visitControlFlow(visit, selection);
        return true;
    }

    bool visitUnary(Visit visit, TIntermUnary *node) override
    {
        if (visit == PreVisit)
        {
            switch (node->getOp())
            {
              case EOpDFdx:
              case EOpDFdy:
                onGradient();
              default:
                break;
            }
        }

        return true;
    }

    bool visitAggregate(Visit visit, TIntermAggregate *node) override
    {
        if (visit == PreVisit)
        {
            if (node->getOp() == EOpFunctionCall)
            {
                if (node->isUserDefined())
                {
                    size_t calleeIndex = mDag.findIndex(node);
                    ASSERT(calleeIndex != CallDAG::InvalidIndex && calleeIndex < mIndex);

                    if ((*mMetadataList)[calleeIndex].mUsesGradient) {
                        onGradient();
                    }
                }
                else
                {
                    TString name = TFunction::unmangleName(node->getName());

                    if (name == "texture2D" ||
                        name == "texture2DProj" ||
                        name == "textureCube")
                    {
                        onGradient();
                    }
                }
            }
        }

        return true;
    }

  private:
    MetadataList *mMetadataList;
    ASTMetadataHLSL *mMetadata;
    size_t mIndex;
    const CallDAG &mDag;

    // Contains a stack of the control flow nodes that are parents of the node being
    // currently visited. It is used to mark control flows using a gradient.
    std::vector<TIntermNode*> mParents;
};

}

bool ASTMetadataHLSL::hasGradientInCallGraph(TIntermSelection *node)
{
    return mControlFlowsContainingGradient.count(node) > 0;
}

bool ASTMetadataHLSL::hasGradientInCallGraph(TIntermLoop *node)
{
    return mControlFlowsContainingGradient.count(node) > 0;
}

MetadataList CreateASTMetadataHLSL(TIntermNode *root, const CallDAG &callDag)
{
    MetadataList metadataList(callDag.size());

    // Compute all the information related to when gradient operations are used.
    // We want to know for each function and control flow operation if they have
    // a gradient operation in their call graph (shortened to "using a gradient"
    // in the rest of the file).
    //
    // This computation is logically split in three steps:
    //  1 - For each function compute if it uses a gradient in its body, ignoring
    // calls to other user-defined functions.
    //  2 - For each function determine if it uses a gradient in its call graph,
    // using the result of step 1 and the CallDAG to know its callees.
    //  3 - For each control flow statement of each function, check if it uses a
    // gradient in the function's body, or if it calls a user-defined function that
    // uses a gradient.
    //
    // We take advantage of the call graph being a DAG and instead compute 1, 2 and 3
    // for leaves first, then going down the tree. This is correct because 1 doesn't
    // depend on other functions, and 2 and 3 depend only on callees.
    for (size_t i = 0; i < callDag.size(); i++)
    {
        PullGradient pull(&metadataList, i, callDag);
        pull.traverse(callDag.getRecordFromIndex(i).node);
    }

    return metadataList;
}
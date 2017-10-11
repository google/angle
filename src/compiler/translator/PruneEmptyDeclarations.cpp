//
// Copyright (c) 2002-2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The PruneEmptyDeclarations function prunes unnecessary empty declarations and declarators from
// the AST.

#include "compiler/translator/PruneEmptyDeclarations.h"

#include "compiler/translator/IntermTraverse.h"

namespace sh
{

namespace
{

class PruneEmptyDeclarationsTraverser : private TIntermTraverser
{
  public:
    static void apply(TIntermBlock *root);

  private:
    PruneEmptyDeclarationsTraverser();
    bool visitDeclaration(Visit, TIntermDeclaration *node) override;
};

void PruneEmptyDeclarationsTraverser::apply(TIntermBlock *root)
{
    PruneEmptyDeclarationsTraverser prune;
    root->traverse(&prune);
    prune.updateTree();
}

PruneEmptyDeclarationsTraverser::PruneEmptyDeclarationsTraverser()
    : TIntermTraverser(true, false, false)
{
}

bool PruneEmptyDeclarationsTraverser::visitDeclaration(Visit, TIntermDeclaration *node)
{
    TIntermSequence *sequence = node->getSequence();
    if (sequence->size() >= 1)
    {
        TIntermSymbol *sym = sequence->front()->getAsSymbolNode();
        // Prune declarations without a variable name, unless it's an interface block declaration.
        if (sym != nullptr && sym->getSymbol() == "" && !sym->isInterfaceBlock())
        {
            if (sequence->size() > 1)
            {
                // Generate a replacement that will remove the empty declarator in the beginning of
                // a declarator list. Example of a declaration that will be changed:
                // float, a;
                // will be changed to
                // float a;
                // This applies also to struct declarations.
                TIntermSequence emptyReplacement;
                mMultiReplacements.push_back(
                    NodeReplaceWithMultipleEntry(node, sym, emptyReplacement));
            }
            else if (sym->getBasicType() != EbtStruct)
            {
                // Single struct declarations may just declare the struct type and no variables, so
                // they should not be pruned. If there are entirely empty non-struct declarations,
                // they result in TIntermDeclaration nodes without any children in the parsing
                // stage. This will be handled further down in the code.
                UNREACHABLE();
            }
            else if (sym->getType().getQualifier() != EvqGlobal &&
                     sym->getType().getQualifier() != EvqTemporary)
            {
                // We've hit an empty struct declaration with a qualifier, for example like
                // this:
                // const struct a { int i; };
                // NVIDIA GL driver version 367.27 doesn't accept this kind of declarations, so
                // we convert the declaration to a regular struct declaration. This is okay,
                // since ESSL 1.00 spec section 4.1.8 says about structs that "The optional
                // qualifiers only apply to any declarators, and are not part of the type being
                // defined for name."

                if (mInGlobalScope)
                {
                    sym->getTypePointer()->setQualifier(EvqGlobal);
                }
                else
                {
                    sym->getTypePointer()->setQualifier(EvqTemporary);
                }
            }
        }
    }
    else
    {
        // We have a declaration with no declarators.
        // The declaration may be either inside a block or in a loop init expression.
        TIntermBlock *parentAsBlock = getParentNode()->getAsBlock();
        ASSERT(parentAsBlock != nullptr || getParentNode()->getAsLoopNode() != nullptr);
        if (parentAsBlock)
        {
            TIntermSequence emptyReplacement;
            mMultiReplacements.push_back(
                NodeReplaceWithMultipleEntry(parentAsBlock, node, emptyReplacement));
        }
        else
        {
            queueReplacement(nullptr, OriginalNode::IS_DROPPED);
        }
    }
    return false;
}

}  // namespace

void PruneEmptyDeclarations(TIntermBlock *root)
{
    PruneEmptyDeclarationsTraverser::apply(root);
}

}  // namespace sh

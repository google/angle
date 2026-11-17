//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EmitWhileAsForWithLatch.cpp: Rewrite while(cond) loops as for(mediump uint s = 0u; cond; s++) to
// work around driver shader compiler bugs.
//

#include "compiler/translator/tree_ops/glsl/EmitWhileAsForWithLatch.h"

#include "compiler/translator/Compiler.h"
#include "compiler/translator/IntermRebuild.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/tree_util/IntermNode_util.h"

namespace sh
{

namespace
{

class EmitWhileAsForWithLatchRebuilder final : public TIntermRebuild
{
  public:
    explicit EmitWhileAsForWithLatchRebuilder(TCompiler *compiler)
        : TIntermRebuild(*compiler, /*preVisit=*/false, /*postVisit=*/true)
    {}

    PostResult visitLoopPost(TIntermLoop &node) override
    {
        if (node.getType() != ELoopWhile)
        {
            return node;
        }

        ASSERT(node.getInit() == nullptr);
        ASSERT(node.getCondition() != nullptr);
        ASSERT(node.getExpression() == nullptr);

        // ESSL 1.00 does not support uint.
        const TType *counterType = mCompiler.getShaderVersion() >= 300
                                       ? StaticType::GetTemporary<EbtUInt, EbpMedium>()
                                       : StaticType::GetTemporary<EbtInt, EbpMedium>();

        TVariable *counter = CreateTempVariable(&mSymbolTable, counterType);
        TIntermDeclaration *init =
            CreateTempInitDeclarationNode(counter, CreateZeroNode(*counterType));
        TIntermTyped *increment =
            new TIntermUnary(EOpPostIncrement, CreateTempSymbolNode(counter), nullptr);

        return *new TIntermLoop(ELoopFor, init, node.getCondition(), increment, node.getBody());
    }
};

}  // namespace

bool EmitWhileAsForWithLatch(TCompiler *compiler, TIntermBlock *root)
{
    EmitWhileAsForWithLatchRebuilder rebuilder(compiler);
    return rebuilder.rebuildRoot(*root);
}

}  // namespace sh

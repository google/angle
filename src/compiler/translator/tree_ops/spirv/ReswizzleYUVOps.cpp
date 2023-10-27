//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ReswizzleYUVOps: Adjusts swizzles for YUV channel order difference between
//   GLES and Vulkan
//
//

#include "compiler/translator/tree_ops/spirv/EmulateYUVBuiltIns.h"

#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/RunAtTheEndOfShader.h"

namespace sh
{
namespace
{
// A traverser that adjusts channel order for various yuv ops.
class ReswizzleYUVOpsTraverser : public TIntermTraverser
{
  public:
    ReswizzleYUVOpsTraverser(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable)
    {}

    bool visitAggregate(Visit visit, TIntermAggregate *node) override;
    bool adjustOutput(TCompiler *compiler, TIntermBlock *root, const TIntermSymbol &yuvOutput);

  private:
};

bool ReswizzleYUVOpsTraverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    if (visit != Visit::PreVisit)
    {
        return true;
    }

    if (!BuiltInGroup::IsBuiltIn(node->getOp()))
    {
        return true;
    }

    TOperator op = node->getFunction()->getBuiltInOp();
    if (op != EOpTexture && op != EOpTextureProj && op != EOpTexelFetch)
    {
        return true;
    }

    TIntermSequence *arguments = node->getSequence();
    TType const &samplerType   = (*arguments)[0]->getAsTyped()->getType();
    if (samplerType.getBasicType() != EbtSamplerExternal2DY2YEXT)
    {
        return true;
    }

    // texture(...).gbra
    TIntermTyped *replacement = new TIntermSwizzle(node, {1, 2, 0, 3});

    if (replacement != nullptr)
    {
        queueReplacement(replacement, OriginalNode::BECOMES_CHILD);
        return false;
    }

    return true;
}

bool ReswizzleYUVOpsTraverser::adjustOutput(TCompiler *compiler,
                                            TIntermBlock *root,
                                            const TIntermSymbol &yuvOutput)
{
    TIntermBlock *block = new TIntermBlock;

    // output = output.brga
    TVector<int> swizzle = {2, 0, 1, 3};
    const int size       = yuvOutput.getType().getNominalSize();
    if (size < 4)
    {
        swizzle.resize(size);
    }

    TIntermTyped *assignment = new TIntermBinary(EOpAssign, yuvOutput.deepCopy(),
                                                 new TIntermSwizzle(yuvOutput.deepCopy(), swizzle));
    block->appendStatement(assignment);

    return RunAtTheEndOfShader(compiler, root, block, mSymbolTable);
}
}  // anonymous namespace

bool ReswizzleYUVOps(TCompiler *compiler,
                     TIntermBlock *root,
                     TSymbolTable *symbolTable,
                     const TIntermSymbol *yuvOutput)
{
    ReswizzleYUVOpsTraverser traverser(symbolTable);
    root->traverse(&traverser);

    if (!traverser.updateTree(compiler, root))
    {
        return false;
    }

    if (yuvOutput != nullptr && !traverser.adjustOutput(compiler, root, *yuvOutput))
    {
        return false;
    }
    return true;
}
}  // namespace sh

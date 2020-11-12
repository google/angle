//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Implementation of InterpolateAtOffset viewport transformation.
// See header for more info.

#include "compiler/translator/tree_ops/RewriteDfdy.h"

#include "common/angleutils.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/DriverUniform.h"
#include "compiler/translator/tree_util/FlipRotateSpecConst.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{

namespace
{

class Traverser : public TIntermTraverser
{
  public:
    ANGLE_NO_DISCARD static bool Apply(TCompiler *compiler,
                                       ShCompileOptions compileOptions,
                                       TIntermNode *root,
                                       const TSymbolTable &symbolTable,
                                       int ShaderVersion,
                                       FlipRotateSpecConst *rotationSpecConst);

  private:
    Traverser(TSymbolTable *symbolTable,
              ShCompileOptions compileOptions,
              int shaderVersion,
              FlipRotateSpecConst *rotationSpecConst);
    bool visitAggregate(Visit visit, TIntermAggregate *node) override;

    const TSymbolTable *symbolTable = nullptr;
    const int shaderVersion;
    FlipRotateSpecConst *mRotationSpecConst = nullptr;
    bool mUsePreRotation                    = false;
};

Traverser::Traverser(TSymbolTable *symbolTable,
                     ShCompileOptions compileOptions,
                     int shaderVersion,
                     FlipRotateSpecConst *rotationSpecConst)
    : TIntermTraverser(true, false, false, symbolTable),
      symbolTable(symbolTable),
      shaderVersion(shaderVersion),
      mRotationSpecConst(rotationSpecConst),
      mUsePreRotation((compileOptions & SH_ADD_PRE_ROTATION) != 0)
{}

// static
bool Traverser::Apply(TCompiler *compiler,
                      ShCompileOptions compileOptions,
                      TIntermNode *root,
                      const TSymbolTable &symbolTable,
                      int shaderVersion,
                      FlipRotateSpecConst *rotationSpecConst)
{
    TSymbolTable *pSymbolTable = const_cast<TSymbolTable *>(&symbolTable);
    Traverser traverser(pSymbolTable, compileOptions, shaderVersion, rotationSpecConst);
    root->traverse(&traverser);
    return traverser.updateTree(compiler, root);
}

bool Traverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    // Decide if the node represents the call of texelFetchOffset.
    if (node->getOp() != EOpCallBuiltInFunction)
    {
        return true;
    }

    ASSERT(node->getFunction()->symbolType() == SymbolType::BuiltIn);
    if (node->getFunction()->name() != "interpolateAtOffset")
    {
        return true;
    }

    const TIntermSequence *sequence = node->getSequence();
    ASSERT(sequence->size() == 2u);

    TIntermSequence *interpolateAtOffsetArguments = new TIntermSequence();
    // interpolant node
    interpolateAtOffsetArguments->push_back(sequence->at(0));
    // offset
    TIntermTyped *offsetNode = sequence->at(1)->getAsTyped();
    ASSERT(offsetNode->getType() == *(StaticType::GetBasic<EbtFloat, 2>()));

    // If pre-rotation is enabled apply the transformation else just flip the Y-coordinate
    TIntermTyped *rotatedXY = mUsePreRotation ? mRotationSpecConst->getFragRotationMultiplyFlipXY()
                                              : mRotationSpecConst->getFlipXY();

    TIntermBinary *correctedOffset = new TIntermBinary(EOpMul, offsetNode, rotatedXY);
    correctedOffset->setLine(offsetNode->getLine());
    interpolateAtOffsetArguments->push_back(correctedOffset);

    TIntermTyped *interpolateAtOffsetNode = CreateBuiltInFunctionCallNode(
        "interpolateAtOffset", interpolateAtOffsetArguments, *symbolTable, shaderVersion);
    interpolateAtOffsetNode->setLine(node->getLine());

    // Replace the old node by this new node.
    queueReplacement(interpolateAtOffsetNode, OriginalNode::IS_DROPPED);

    return true;
}

}  // anonymous namespace

bool RewriteInterpolateAtOffset(TCompiler *compiler,
                                ShCompileOptions compileOptions,
                                TIntermNode *root,
                                const TSymbolTable &symbolTable,
                                int shaderVersion,
                                FlipRotateSpecConst *rotationSpecConst)
{
    // interpolateAtOffset is only valid in GLSL 3.0 and later.
    if (shaderVersion < 300)
    {
        return true;
    }

    return Traverser::Apply(compiler, compileOptions, root, symbolTable, shaderVersion,
                            rotationSpecConst);
}

}  // namespace sh

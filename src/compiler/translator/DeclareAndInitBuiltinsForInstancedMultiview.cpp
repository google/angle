//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Applies the necessary AST transformations to support multiview rendering through instancing.
// Check the header file For more information.
//

#include "compiler/translator/DeclareAndInitBuiltinsForInstancedMultiview.h"

#include "compiler/translator/FindMain.h"
#include "compiler/translator/InitializeVariables.h"
#include "compiler/translator/IntermTraverse.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

namespace
{

class ReplaceVariableTraverser : public TIntermTraverser
{
  public:
    ReplaceVariableTraverser(const TString &symbolName, TIntermSymbol *newSymbol)
        : TIntermTraverser(true, false, false), mSymbolName(symbolName), mNewSymbol(newSymbol)
    {
    }

    void visitSymbol(TIntermSymbol *node) override
    {
        TName &name = node->getName();
        if (name.getString() == mSymbolName)
        {
            queueReplacement(mNewSymbol->deepCopy(), OriginalNode::IS_DROPPED);
        }
    }

  private:
    TString mSymbolName;
    TIntermSymbol *mNewSymbol;
};

TIntermSymbol *CreateGLInstanceIDSymbol()
{
    return new TIntermSymbol(0, "gl_InstanceID", TType(EbtInt, EbpHigh, EvqInstanceID));
}

void InitializeViewIDAndInstanceID(TIntermBlock *root,
                                   TIntermTyped *viewIDSymbol,
                                   TIntermTyped *instanceIDSymbol,
                                   unsigned numberOfViews)
{
    // Create a signed numberOfViews node.
    TConstantUnion *numberOfViewsConstant = new TConstantUnion();
    numberOfViewsConstant->setIConst(static_cast<int>(numberOfViews));
    TIntermConstantUnion *numberOfViewsIntSymbol =
        new TIntermConstantUnion(numberOfViewsConstant, TType(EbtInt, EbpHigh, EvqConst));

    // Create a gl_InstanceID / numberOfViews node.
    TIntermBinary *normalizedInstanceID =
        new TIntermBinary(EOpDiv, CreateGLInstanceIDSymbol(), numberOfViewsIntSymbol);

    // Create a InstanceID = gl_InstanceID / numberOfViews node.
    TIntermBinary *instanceIDInitializer =
        new TIntermBinary(EOpAssign, instanceIDSymbol->deepCopy(), normalizedInstanceID);

    // Create a uint(gl_InstanceID) node.
    TIntermSequence *glInstanceIDSymbolCastArguments = new TIntermSequence();
    glInstanceIDSymbolCastArguments->push_back(CreateGLInstanceIDSymbol());
    TIntermAggregate *glInstanceIDAsUint = TIntermAggregate::CreateConstructor(
        TType(EbtUInt, EbpHigh, EvqTemporary), glInstanceIDSymbolCastArguments);

    // Create an unsigned numberOfViews node.
    TConstantUnion *numberOfViewsUnsignedConstant = new TConstantUnion();
    numberOfViewsUnsignedConstant->setUConst(numberOfViews);
    TIntermConstantUnion *numberOfViewsUintSymbol =
        new TIntermConstantUnion(numberOfViewsUnsignedConstant, TType(EbtUInt, EbpHigh, EvqConst));

    // Create a uint(gl_InstanceID) % numberOfViews node.
    TIntermBinary *normalizedViewID =
        new TIntermBinary(EOpIMod, glInstanceIDAsUint, numberOfViewsUintSymbol);

    // Create a ViewID_OVR = uint(gl_InstanceID) % numberOfViews node.
    TIntermBinary *viewIDInitializer =
        new TIntermBinary(EOpAssign, viewIDSymbol->deepCopy(), normalizedViewID);

    // Add initializers at the beginning of main().
    TIntermBlock *mainBody = FindMainBody(root);
    mainBody->getSequence()->insert(mainBody->getSequence()->begin(), instanceIDInitializer);
    mainBody->getSequence()->insert(mainBody->getSequence()->begin(), viewIDInitializer);
}

// Replaces every occurrence of a symbol with the name specified in symbolName with newSymbolNode.
void ReplaceSymbol(TIntermBlock *root, const TString &symbolName, TIntermSymbol *newSymbolNode)
{
    ReplaceVariableTraverser traverser(symbolName, newSymbolNode);
    root->traverse(&traverser);
    traverser.updateTree();
}

void DeclareGlobalVariable(TIntermBlock *root, TIntermTyped *typedNode)
{
    TIntermSequence *globalSequence = root->getSequence();
    TIntermDeclaration *declaration = new TIntermDeclaration();
    declaration->appendDeclarator(typedNode->deepCopy());
    globalSequence->insert(globalSequence->begin(), declaration);
}

}  // namespace

void DeclareAndInitBuiltinsForInstancedMultiview(TIntermBlock *root,
                                                 unsigned numberOfViews,
                                                 GLenum shaderType)
{
    ASSERT(shaderType == GL_VERTEX_SHADER || shaderType == GL_FRAGMENT_SHADER);

    TQualifier viewIDQualifier  = (shaderType == GL_VERTEX_SHADER) ? EvqFlatOut : EvqFlatIn;
    TIntermSymbol *viewIDSymbol = new TIntermSymbol(TSymbolTable::nextUniqueId(), "ViewID_OVR",
                                                    TType(EbtUInt, EbpHigh, viewIDQualifier));
    viewIDSymbol->setInternal(true);

    DeclareGlobalVariable(root, viewIDSymbol);
    ReplaceSymbol(root, "gl_ViewID_OVR", viewIDSymbol);
    if (shaderType == GL_VERTEX_SHADER)
    {
        // Replacing gl_InstanceID with InstanceID should happen before adding the initializers of
        // InstanceID and ViewID.
        TIntermSymbol *instanceIDSymbol = new TIntermSymbol(
            TSymbolTable::nextUniqueId(), "InstanceID", TType(EbtInt, EbpHigh, EvqGlobal));
        instanceIDSymbol->setInternal(true);
        DeclareGlobalVariable(root, instanceIDSymbol);
        ReplaceSymbol(root, "gl_InstanceID", instanceIDSymbol);
        InitializeViewIDAndInstanceID(root, viewIDSymbol, instanceIDSymbol, numberOfViews);
    }
}

}  // namespace sh
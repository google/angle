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
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

namespace
{

class RenameVariableAndMarkAsInternalTraverser : public TIntermTraverser
{
  public:
    RenameVariableAndMarkAsInternalTraverser(const TString &oldName, const TString &newName)
        : TIntermTraverser(true, false, false), mOldName(oldName), mNewName(newName)
    {
    }

    void visitSymbol(TIntermSymbol *node) override
    {
        TName &name = node->getName();
        if (name.getString() == mOldName)
        {
            node->getTypePointer()->setQualifier(EvqTemporary);
            name.setInternal(true);
            name.setString(mNewName);
        }
    }

  private:
    TString mOldName;
    TString mNewName;
};

TIntermSymbol *CreateGLInstanceIDSymbol()
{
    return new TIntermSymbol(0, "gl_InstanceID", TType(EbtInt, EbpHigh, EvqInstanceID));
}

// Adds initializers for InstanceID and ViewID_OVR at the beginning of main().
void InitializeBuiltinsInMain(TIntermBlock *root,
                              TIntermSymbol *instanceIDSymbol,
                              TIntermSymbol *viewIDSymbol,
                              unsigned numberOfViews)
{
    // Create a signed numberOfViews node.
    TConstantUnion *numberOfViewsConstant = new TConstantUnion();
    numberOfViewsConstant->setIConst(numberOfViews);
    TIntermConstantUnion *numberOfViewsIntSymbol =
        new TIntermConstantUnion(numberOfViewsConstant, TType(EbtInt, EbpHigh, EvqConst));

    // Create a gl_InstanceID / numberOfViews node.
    TIntermBinary *normalizedInstanceID =
        new TIntermBinary(EOpDiv, CreateGLInstanceIDSymbol(), numberOfViewsIntSymbol);

    // Create a InstanceID = gl_InstanceID / numberOfViews node.
    TIntermBinary *instanceIDInitializer =
        new TIntermBinary(EOpAssign, instanceIDSymbol, normalizedInstanceID);

    // Create a uint(gl_InstanceID) node.
    TIntermSequence *instanceIDSymbolCastArguments = new TIntermSequence();
    instanceIDSymbolCastArguments->push_back(CreateGLInstanceIDSymbol());
    TIntermAggregate *instanceIDAsUint = TIntermAggregate::CreateConstructor(
        TType(EbtUInt, EbpHigh, EvqTemporary), instanceIDSymbolCastArguments);

    // Create a unsigned numberOfViews node.
    TConstantUnion *numberOfViewsUnsignedConstant = new TConstantUnion();
    numberOfViewsUnsignedConstant->setUConst(numberOfViews);
    TIntermConstantUnion *numberOfViewsUintSymbol =
        new TIntermConstantUnion(numberOfViewsUnsignedConstant, TType(EbtUInt, EbpHigh, EvqConst));

    // Create a uint(gl_InstanceID) % numberOfViews node.
    TIntermBinary *normalizedViewID =
        new TIntermBinary(EOpIMod, instanceIDAsUint, numberOfViewsUintSymbol);

    // Create a ViewID_OVR = uint(gl_InstanceID) % numberOfViews node.
    TIntermBinary *viewIDInitializer = new TIntermBinary(EOpAssign, viewIDSymbol, normalizedViewID);

    // Add nodes to sequence and insert into main.
    TIntermSequence *initializers = new TIntermSequence();
    initializers->push_back(viewIDInitializer);
    initializers->push_back(instanceIDInitializer);

    // Insert init code as a block at the beginning of the main() function.
    TIntermBlock *initGlobalsBlock = new TIntermBlock();
    initGlobalsBlock->getSequence()->swap(*initializers);

    TIntermFunctionDefinition *main = FindMain(root);
    ASSERT(main != nullptr);
    TIntermBlock *mainBody = main->getBody();
    ASSERT(mainBody != nullptr);
    mainBody->getSequence()->insert(mainBody->getSequence()->begin(), initGlobalsBlock);
}

// Adds declarations for ViewID_OVR and InstanceID in global scope at the beginning of
// the shader.
void DeclareBuiltinsInGlobalScope(TIntermBlock *root,
                                  TIntermSymbol *instanceIDSymbol,
                                  TIntermSymbol *viewIDSymbol)
{
    TIntermSequence *globalSequence = root->getSequence();

    TIntermDeclaration *instanceIDDeclaration = new TIntermDeclaration();
    instanceIDDeclaration->appendDeclarator(instanceIDSymbol);

    TIntermDeclaration *viewIDOVRDeclaration = new TIntermDeclaration();
    viewIDOVRDeclaration->appendDeclarator(viewIDSymbol);

    globalSequence->insert(globalSequence->begin(), viewIDOVRDeclaration);
    globalSequence->insert(globalSequence->begin(), instanceIDDeclaration);
}

// Replaces every occurrence of gl_InstanceID with InstanceID, sets the name to internal
// and changes the qualifier from EvqInstanceID to EvqTemporary.
void RenameGLInstanceIDAndMarkAsInternal(TIntermBlock *root)
{
    RenameVariableAndMarkAsInternalTraverser traverser("gl_InstanceID", "InstanceID");
    root->traverse(&traverser);
}

// Replaces every occurrence of gl_ViewID_OVR with ViewID_OVR, sets the name to internal
// and changes the qualifier from EvqViewIDOVR to EvqTemporary.
void RenameGLViewIDAndMarkAsInternal(TIntermBlock *root)
{
    RenameVariableAndMarkAsInternalTraverser traverser("gl_ViewID_OVR", "ViewID_OVR");
    root->traverse(&traverser);
}

}  // namespace

void DeclareAndInitBuiltinsForInstancedMultiview(TIntermBlock *root, unsigned numberOfViews)
{
    TIntermSymbol *instanceIDSymbol = new TIntermSymbol(TSymbolTable::nextUniqueId(), "InstanceID",
                                                        TType(EbtInt, EbpHigh, EvqGlobal));
    instanceIDSymbol->setInternal(true);

    TIntermSymbol *viewIDSymbol = new TIntermSymbol(TSymbolTable::nextUniqueId(), "ViewID_OVR",
                                                    TType(EbtUInt, EbpHigh, EvqGlobal));
    viewIDSymbol->setInternal(true);

    // Renaming the variables should happen before adding the initializers.
    RenameGLInstanceIDAndMarkAsInternal(root);
    DeclareBuiltinsInGlobalScope(root, instanceIDSymbol, viewIDSymbol);
    InitializeBuiltinsInMain(root, instanceIDSymbol->deepCopy()->getAsSymbolNode(),
                             viewIDSymbol->deepCopy()->getAsSymbolNode(), numberOfViews);
    RenameGLViewIDAndMarkAsInternal(root);
}

}  // namespace sh
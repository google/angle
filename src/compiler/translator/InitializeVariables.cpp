//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/InitializeVariables.h"

#include "angle_gl.h"
#include "common/debug.h"
#include "compiler/translator/FindMain.h"
#include "compiler/translator/IntermTraverse.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

bool IsNamelessStruct(const TIntermTyped *node)
{
    return (node->getBasicType() == EbtStruct && node->getType().getStruct()->name() == "");
}

void AddArrayZeroInitSequence(const TIntermTyped *initializedNode,
                              TIntermSequence *initSequenceOut);

TIntermBinary *CreateZeroInitAssignment(const TIntermTyped *initializedNode)
{
    TIntermTyped *zero = TIntermTyped::CreateZero(initializedNode->getType());
    return new TIntermBinary(EOpAssign, initializedNode->deepCopy(), zero);
}

void AddStructZeroInitSequence(const TIntermTyped *initializedNode,
                               TIntermSequence *initSequenceOut)
{
    ASSERT(initializedNode->getBasicType() == EbtStruct);
    TStructure *structType = initializedNode->getType().getStruct();
    for (int i = 0; i < static_cast<int>(structType->fields().size()); ++i)
    {
        TIntermBinary *element = new TIntermBinary(
            EOpIndexDirectStruct, initializedNode->deepCopy(), TIntermTyped::CreateIndexNode(i));
        if (element->isArray())
        {
            AddArrayZeroInitSequence(element, initSequenceOut);
        }
        else if (element->getType().isStructureContainingArrays())
        {
            AddStructZeroInitSequence(element, initSequenceOut);
        }
        else
        {
            // Structs can't be defined inside structs, so the type of a struct field can't be a
            // nameless struct.
            ASSERT(!IsNamelessStruct(element));
            initSequenceOut->push_back(CreateZeroInitAssignment(element));
        }
    }
}

void AddArrayZeroInitSequence(const TIntermTyped *initializedNode, TIntermSequence *initSequenceOut)
{
    ASSERT(initializedNode->isArray());
    // Assign the array elements one by one to keep the AST compatible with ESSL 1.00 which
    // doesn't have array assignment.
    // Note that it is important to have the array init in the right order to workaround
    // http://crbug.com/709317
    for (unsigned int i = 0; i < initializedNode->getArraySize(); ++i)
    {
        TIntermBinary *element = new TIntermBinary(EOpIndexDirect, initializedNode->deepCopy(),
                                                   TIntermTyped::CreateIndexNode(i));
        if (element->getType().isStructureContainingArrays())
        {
            AddStructZeroInitSequence(element, initSequenceOut);
        }
        else
        {
            initSequenceOut->push_back(CreateZeroInitAssignment(element));
        }
    }
}

void InsertInitCode(TIntermSequence *mainBody,
                    const InitVariableList &variables,
                    const TSymbolTable &symbolTable,
                    int shaderVersion,
                    ShShaderSpec shaderSpec,
                    const TExtensionBehavior &extensionBehavior)
{
    for (const auto &var : variables)
    {
        TString name = TString(var.name.c_str());
        size_t pos   = name.find_last_of('[');
        if (pos != TString::npos)
        {
            name = name.substr(0, pos);
        }

        const TVariable *symbolInfo = nullptr;
        if (var.isBuiltIn())
        {
            symbolInfo =
                reinterpret_cast<const TVariable *>(symbolTable.findBuiltIn(name, shaderVersion));
        }
        else
        {
            symbolInfo = reinterpret_cast<const TVariable *>(symbolTable.findGlobal(name));
        }
        ASSERT(symbolInfo != nullptr);

        TType type = symbolInfo->getType();
        if (type.getQualifier() == EvqFragData &&
            (shaderSpec == SH_WEBGL2_SPEC ||
             !IsExtensionEnabled(extensionBehavior, "GL_EXT_draw_buffers")))
        {
            // Adjust the number of attachment indices which can be initialized according to the
            // WebGL2 spec and extension behavior:
            // - WebGL2 spec, Editor's draft May 31, 5.13 GLSL ES
            //   1.00 Fragment Shader Output: "A fragment shader written in The OpenGL ES Shading
            //   Language, Version 1.00, that statically assigns a value to gl_FragData[n] where n
            //   does not equal constant value 0 must fail to compile in the WebGL 2.0 API.".
            //   This excerpt limits the initialization of gl_FragData to only the 0th index.
            // - If GL_EXT_draw_buffers is disabled, only the 0th index of gl_FragData can be
            //   written to.
            type.setArraySize(1u);
        }

        TIntermSymbol *initializedSymbol = new TIntermSymbol(0, name, type);
        TIntermSequence *initCode = CreateInitCode(initializedSymbol);
        mainBody->insert(mainBody->begin(), initCode->begin(), initCode->end());
    }
}

class InitializeLocalsTraverser : public TIntermTraverser
{
  public:
    InitializeLocalsTraverser(int shaderVersion)
        : TIntermTraverser(true, false, false), mShaderVersion(shaderVersion)
    {
    }

  protected:
    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        for (TIntermNode *declarator : *node->getSequence())
        {
            if (!mInGlobalScope && !declarator->getAsBinaryNode())
            {
                TIntermSymbol *symbol = declarator->getAsSymbolNode();
                ASSERT(symbol);
                if (symbol->getSymbol() == "")
                {
                    continue;
                }

                // Arrays may need to be initialized one element at a time, since ESSL 1.00 does not
                // support array constructors or assigning arrays.
                bool arrayConstructorUnavailable =
                    (symbol->isArray() || symbol->getType().isStructureContainingArrays()) &&
                    mShaderVersion == 100;
                // Nameless struct constructors can't be referred to, so they also need to be
                // initialized one element at a time.
                if (arrayConstructorUnavailable || IsNamelessStruct(symbol))
                {
                    // SimplifyLoopConditions should have been run so the parent node of this node
                    // should not be a loop.
                    ASSERT(getParentNode()->getAsLoopNode() == nullptr);
                    // SeparateDeclarations should have already been run, so we don't need to worry
                    // about further declarators in this declaration depending on the effects of
                    // this declarator.
                    ASSERT(node->getSequence()->size() == 1);
                    insertStatementsInParentBlock(TIntermSequence(), *CreateInitCode(symbol));
                }
                else
                {
                    TIntermBinary *init = new TIntermBinary(
                        EOpInitialize, symbol, TIntermTyped::CreateZero(symbol->getType()));
                    queueReplacementWithParent(node, symbol, init, OriginalNode::BECOMES_CHILD);
                }
            }
        }
        return false;
    }

  private:
    int mShaderVersion;
};

}  // namespace anonymous

TIntermSequence *CreateInitCode(const TIntermSymbol *initializedSymbol)
{
    TIntermSequence *initCode = new TIntermSequence();
    if (initializedSymbol->isArray())
    {
        AddArrayZeroInitSequence(initializedSymbol, initCode);
    }
    else if (initializedSymbol->getType().isStructureContainingArrays() ||
             IsNamelessStruct(initializedSymbol))
    {
        AddStructZeroInitSequence(initializedSymbol, initCode);
    }
    else
    {
        initCode->push_back(CreateZeroInitAssignment(initializedSymbol));
    }
    return initCode;
}

void InitializeUninitializedLocals(TIntermBlock *root, int shaderVersion)
{
    InitializeLocalsTraverser traverser(shaderVersion);
    root->traverse(&traverser);
    traverser.updateTree();
}

void InitializeVariables(TIntermBlock *root,
                         const InitVariableList &vars,
                         const TSymbolTable &symbolTable,
                         int shaderVersion,
                         ShShaderSpec shaderSpec,
                         const TExtensionBehavior &extensionBehavior)
{

    TIntermFunctionDefinition *main = FindMain(root);
    ASSERT(main != nullptr);
    TIntermBlock *body = main->getBody();
    InsertInitCode(body->getSequence(), vars, symbolTable, shaderVersion, shaderSpec,
                   extensionBehavior);
}

}  // namespace sh

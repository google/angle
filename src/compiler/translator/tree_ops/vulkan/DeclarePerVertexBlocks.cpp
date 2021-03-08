//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DeclarePerVertexBlocks: Declare gl_PerVertex blocks if not already.
//

#include "compiler/translator/tree_ops/vulkan/DeclarePerVertexBlocks.h"

#include "compiler/translator/Compiler.h"
#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/ReplaceVariable.h"

namespace sh
{
namespace
{
// Traverser that:
//
// 1. Declares the input and output gl_PerVertex types and variables if not already (based on shader
//    type).
// 2. Turns built-in references into indexes into these variables.
class DeclarePerVertexBlocksTraverser : public TIntermTraverser
{
  public:
    DeclarePerVertexBlocksTraverser(TCompiler *compiler, TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable),
          mShaderType(compiler->getShaderType()),
          mResources(compiler->getResources()),
          mPerVertexInVar(nullptr),
          mPerVertexOutVar(nullptr),
          mPerVertexInVarRedeclared(false),
          mPerVertexOutVarRedeclared(false)
    {}

    void visitSymbol(TIntermSymbol *symbol) override
    {
        const TVariable *variable = &symbol->variable();
        const TType *type         = &variable->getType();

        // Replace gl_out if necessary.
        if (mShaderType == GL_TESS_CONTROL_SHADER && type->getQualifier() == EvqPerVertexOut)
        {
            ASSERT(variable->name() == "gl_out");

            // Declare gl_out if not already.
            if (mPerVertexOutVar == nullptr)
            {
                declareDefaultGlOut();
            }

            if (mPerVertexOutVarRedeclared)
            {
                queueReplacement(new TIntermSymbol(mPerVertexOutVar), OriginalNode::IS_DROPPED);
            }

            return;
        }

        // Replace gl_in if necessary.
        if ((mShaderType == GL_TESS_CONTROL_SHADER || mShaderType == GL_TESS_EVALUATION_SHADER ||
             mShaderType == GL_GEOMETRY_SHADER) &&
            type->getQualifier() == EvqPerVertexIn)
        {
            ASSERT(variable->name() == "gl_in");

            // Declare gl_in if not already.
            if (mPerVertexInVar == nullptr)
            {
                declareDefaultGlIn();
            }

            if (mPerVertexInVarRedeclared)
            {
                queueReplacement(new TIntermSymbol(mPerVertexInVar), OriginalNode::IS_DROPPED);
            }

            return;
        }

        // Turn gl_Position, gl_PointSize, gl_ClipDistance and gl_CullDistance into references to
        // the output gl_PerVertex.  Note that the default gl_PerVertex is declared as follows:
        //
        //     out gl_PerVertex
        //     {
        //         vec4 gl_Position;
        //         float gl_PointSize;
        //         float gl_ClipDistance[];
        //         float gl_CullDistance[];
        //     };
        //

        if (variable->symbolType() != SymbolType::BuiltIn)
        {
            ASSERT(variable->name() != "gl_Position" && variable->name() != "gl_PointSize" &&
                   variable->name() != "gl_ClipDistance" && variable->name() != "gl_CullDistance");

            return;
        }

        // If this built-in was already visited, reuse the variable defined for it.
        auto replacement = mVariableMap.find(variable);
        if (replacement != mVariableMap.end())
        {
            queueReplacement(replacement->second->deepCopy(), OriginalNode::IS_DROPPED);
            return;
        }

        int fieldIndex = -1;
        if (type->getQualifier() == EvqPosition)
        {
            ASSERT(variable->name() == "gl_Position");
            fieldIndex = 0;
        }
        if (type->getQualifier() == EvqPointSize)
        {
            ASSERT(variable->name() == "gl_PointSize");
            fieldIndex = 1;
        }
        if (type->getQualifier() == EvqClipDistance)
        {
            ASSERT(variable->name() == "gl_ClipDistance");
            fieldIndex = 2;
        }
        if (type->getQualifier() == EvqCullDistance)
        {
            ASSERT(variable->name() == "gl_CullDistance");
            fieldIndex = 3;
        }

        // Not the built-in we are looking for.
        if (fieldIndex < 0)
        {
            return;
        }

        // Declare the output gl_PerVertex if not already.
        if (mPerVertexOutVar == nullptr)
        {
            declareDefaultGlOut();
        }

        TType *newType = new TType(*type);
        newType->setInterfaceBlockField(mPerVertexOutVar->getType().getInterfaceBlock(),
                                        fieldIndex);

        TVariable *newVariable = new TVariable(mSymbolTable, variable->name(), newType,
                                               variable->symbolType(), variable->extensions());

        TIntermSymbol *newSymbol = new TIntermSymbol(newVariable);
        mVariableMap[variable]   = newSymbol;

        queueReplacement(newSymbol, OriginalNode::IS_DROPPED);
    }

    const TVariable *getRedeclaredPerVertexOutVar()
    {
        return mPerVertexOutVarRedeclared ? mPerVertexOutVar : nullptr;
    }

    const TVariable *getRedeclaredPerVertexInVar()
    {
        return mPerVertexInVarRedeclared ? mPerVertexInVar : nullptr;
    }

  private:
    const TVariable *declarePerVertex(TQualifier qualifier,
                                      uint32_t arraySize,
                                      ImmutableString &variableName)
    {
        TFieldList *fields = new TFieldList;

        const TType *vec4Type  = StaticType::GetBasic<EbtFloat, 4>();
        const TType *floatType = StaticType::GetBasic<EbtFloat, 1>();

        TType *positionType     = new TType(*vec4Type);
        TType *pointSizeType    = new TType(*floatType);
        TType *clipDistanceType = new TType(*floatType);
        TType *cullDistanceType = new TType(*floatType);

        positionType->setQualifier(EvqPosition);
        pointSizeType->setQualifier(EvqPointSize);
        clipDistanceType->setQualifier(EvqClipDistance);
        cullDistanceType->setQualifier(EvqCullDistance);

        clipDistanceType->makeArray(mResources.MaxClipDistances);
        cullDistanceType->makeArray(mResources.MaxCullDistances);

        fields->push_back(new TField(positionType, ImmutableString("gl_Position"), TSourceLoc(),
                                     SymbolType::AngleInternal));
        fields->push_back(new TField(pointSizeType, ImmutableString("gl_PointSize"), TSourceLoc(),
                                     SymbolType::AngleInternal));
        fields->push_back(new TField(clipDistanceType, ImmutableString("gl_ClipDistance"),
                                     TSourceLoc(), SymbolType::AngleInternal));
        fields->push_back(new TField(cullDistanceType, ImmutableString("gl_CullDistance"),
                                     TSourceLoc(), SymbolType::AngleInternal));

        TInterfaceBlock *interfaceBlock =
            new TInterfaceBlock(mSymbolTable, ImmutableString("gl_PerVertex"), fields,
                                TLayoutQualifier::Create(), SymbolType::AngleInternal);

        TType *interfaceBlockType =
            new TType(interfaceBlock, qualifier, TLayoutQualifier::Create());
        if (arraySize > 0)
        {
            interfaceBlockType->makeArray(arraySize);
        }

        TVariable *interfaceBlockVar =
            new TVariable(mSymbolTable, variableName, interfaceBlockType,
                          variableName.empty() ? SymbolType::Empty : SymbolType::AngleInternal);

        return interfaceBlockVar;
    }

    void declareDefaultGlOut()
    {
        ASSERT(!mPerVertexOutVarRedeclared);

        // For tessellation control shaders, gl_out is an array of MaxPatchVertices
        // For other shaders, there's no explicit name or array size

        ImmutableString varName("");
        uint32_t arraySize = 0;
        if (mShaderType == GL_TESS_CONTROL_SHADER)
        {
            varName   = ImmutableString("gl_out");
            arraySize = mResources.MaxPatchVertices;
        }

        mPerVertexOutVar           = declarePerVertex(EvqPerVertexOut, arraySize, varName);
        mPerVertexOutVarRedeclared = true;
    }

    void declareDefaultGlIn()
    {
        ASSERT(!mPerVertexInVarRedeclared);

        // For tessellation shaders, gl_in is an array of MaxPatchVertices.
        // For geometry shaders, gl_in is sized based on the primitive type.

        ImmutableString varName("gl_in");
        uint32_t arraySize = mResources.MaxPatchVertices;
        if (mShaderType == GL_GEOMETRY_SHADER)
        {
            arraySize =
                mSymbolTable->getGlInVariableWithArraySize()->getType().getOutermostArraySize();
        }

        mPerVertexInVar           = declarePerVertex(EvqPerVertexIn, arraySize, varName);
        mPerVertexInVarRedeclared = true;
    }

    GLenum mShaderType;
    const ShBuiltInResources &mResources;

    const TVariable *mPerVertexInVar;
    const TVariable *mPerVertexOutVar;

    bool mPerVertexInVarRedeclared;
    bool mPerVertexOutVarRedeclared;

    // A map of already replaced built-in variables.
    VariableReplacementMap mVariableMap;
};

void AddPerVertexDecl(TIntermBlock *root, const TVariable *variable)
{
    if (variable == nullptr)
    {
        return;
    }

    TIntermDeclaration *decl = new TIntermDeclaration;
    TIntermSymbol *symbol    = new TIntermSymbol(variable);
    decl->appendDeclarator(symbol);

    // Insert the declaration before the first function.
    size_t firstFunctionIndex = FindFirstFunctionDefinitionIndex(root);
    root->insertChildNodes(firstFunctionIndex, {decl});
}
}  // anonymous namespace

bool DeclarePerVertexBlocks(TCompiler *compiler, TIntermBlock *root, TSymbolTable *symbolTable)
{
    if (compiler->getShaderType() == GL_COMPUTE_SHADER ||
        compiler->getShaderType() == GL_FRAGMENT_SHADER)
    {
        return true;
    }

    DeclarePerVertexBlocksTraverser traverser(compiler, symbolTable);
    root->traverse(&traverser);
    if (!traverser.updateTree(compiler, root))
    {
        return false;
    }

    AddPerVertexDecl(root, traverser.getRedeclaredPerVertexOutVar());
    AddPerVertexDecl(root, traverser.getRedeclaredPerVertexInVar());

    return compiler->validateAST(root);
}
}  // namespace sh

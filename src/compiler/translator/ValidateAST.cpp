//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/ValidateAST.h"

#include "compiler/translator/Diagnostics.h"
#include "compiler/translator/Symbol.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/tree_util/SpecializationConstant.h"

namespace sh
{

namespace
{

class ValidateAST : public TIntermTraverser
{
  public:
    static bool validate(TIntermNode *root,
                         TDiagnostics *diagnostics,
                         const ValidateASTOptions &options);

    void visitSymbol(TIntermSymbol *node) override;
    void visitConstantUnion(TIntermConstantUnion *node) override;
    bool visitSwizzle(Visit visit, TIntermSwizzle *node) override;
    bool visitBinary(Visit visit, TIntermBinary *node) override;
    bool visitUnary(Visit visit, TIntermUnary *node) override;
    bool visitTernary(Visit visit, TIntermTernary *node) override;
    bool visitIfElse(Visit visit, TIntermIfElse *node) override;
    bool visitSwitch(Visit visit, TIntermSwitch *node) override;
    bool visitCase(Visit visit, TIntermCase *node) override;
    void visitFunctionPrototype(TIntermFunctionPrototype *node) override;
    bool visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node) override;
    bool visitAggregate(Visit visit, TIntermAggregate *node) override;
    bool visitBlock(Visit visit, TIntermBlock *node) override;
    bool visitGlobalQualifierDeclaration(Visit visit,
                                         TIntermGlobalQualifierDeclaration *node) override;
    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override;
    bool visitLoop(Visit visit, TIntermLoop *node) override;
    bool visitBranch(Visit visit, TIntermBranch *node) override;
    void visitPreprocessorDirective(TIntermPreprocessorDirective *node) override;

  private:
    ValidateAST(TIntermNode *root, TDiagnostics *diagnostics, const ValidateASTOptions &options);

    // Visit as a generic node
    void visitNode(Visit visit, TIntermNode *node);

    void scope(Visit visit);
    bool isVariableDeclared(const TVariable *variable);
    bool variableNeedsDeclaration(const TVariable *variable);

    void expectNonNullChildren(Visit visit, TIntermNode *node, size_t least_count);

    bool validateInternal();

    ValidateASTOptions mOptions;
    TDiagnostics *mDiagnostics;

    // For validateSingleParent:
    std::map<TIntermNode *, TIntermNode *> mParent;
    bool mSingleParentFailed = false;

    // For validateVariableReferences:
    std::vector<std::set<const TVariable *>> mDeclaredVariables;
    std::set<const TInterfaceBlock *> mNamelessInterfaceBlocks;
    bool mVariableReferencesFailed = false;

    // For validateNullNodes:
    bool mNullNodesFailed = false;

    // For validateMultiDeclarations:
    bool mMultiDeclarationsFailed = false;
};

bool ValidateAST::validate(TIntermNode *root,
                           TDiagnostics *diagnostics,
                           const ValidateASTOptions &options)
{
    ValidateAST validate(root, diagnostics, options);
    root->traverse(&validate);
    return validate.validateInternal();
}

ValidateAST::ValidateAST(TIntermNode *root,
                         TDiagnostics *diagnostics,
                         const ValidateASTOptions &options)
    : TIntermTraverser(true, false, true, nullptr), mOptions(options), mDiagnostics(diagnostics)
{
    bool isTreeRoot = root->getAsBlock() && root->getAsBlock()->isTreeRoot();

    // Some validations are not applicable unless run on the entire tree.
    if (!isTreeRoot)
    {
        mOptions.validateVariableReferences = false;
    }

    if (mOptions.validateSingleParent)
    {
        mParent[root] = nullptr;
    }
}

void ValidateAST::visitNode(Visit visit, TIntermNode *node)
{
    if (visit == PreVisit && mOptions.validateSingleParent)
    {
        size_t childCount = node->getChildCount();
        for (size_t i = 0; i < childCount; ++i)
        {
            TIntermNode *child = node->getChildNode(i);
            if (mParent.find(child) != mParent.end())
            {
                // If child is visited twice but through the same parent, the problem is in one of
                // the ancestors.
                if (mParent[child] != node)
                {
                    mDiagnostics->error(node->getLine(), "Found child with two parents",
                                        "<validateSingleParent>");
                    mSingleParentFailed = true;
                }
            }

            mParent[child] = node;
        }
    }
}

void ValidateAST::scope(Visit visit)
{
    if (mOptions.validateVariableReferences)
    {
        if (visit == PreVisit)
        {
            mDeclaredVariables.push_back({});
        }
        else if (visit == PostVisit)
        {
            mDeclaredVariables.pop_back();
        }
    }
}

bool ValidateAST::isVariableDeclared(const TVariable *variable)
{
    ASSERT(mOptions.validateVariableReferences);

    for (const std::set<const TVariable *> &scopeVariables : mDeclaredVariables)
    {
        if (scopeVariables.count(variable) > 0)
        {
            return true;
        }
    }

    return false;
}

bool ValidateAST::variableNeedsDeclaration(const TVariable *variable)
{
    // Don't expect declaration for built-in variables.
    if (variable->name().beginsWith("gl_"))
    {
        return false;
    }

    // Additionally, don't expect declaration for Vulkan specialization constants.  There is no
    // representation for them in the AST.
    if (variable->symbolType() == SymbolType::AngleInternal &&
        SpecConst::IsSpecConstName(variable->name()))
    {
        return false;
    }

    return true;
}

void ValidateAST::expectNonNullChildren(Visit visit, TIntermNode *node, size_t least_count)
{
    if (visit == PreVisit && mOptions.validateNullNodes)
    {
        size_t childCount = node->getChildCount();
        if (childCount < least_count)
        {
            mDiagnostics->error(node->getLine(), "Too few children", "<validateNullNodes>");
            mNullNodesFailed = true;
        }

        for (size_t i = 0; i < childCount; ++i)
        {
            if (node->getChildNode(i) == nullptr)
            {
                mDiagnostics->error(node->getLine(), "Found nullptr child", "<validateNullNodes>");
                mNullNodesFailed = true;
            }
        }
    }
}

void ValidateAST::visitSymbol(TIntermSymbol *node)
{
    visitNode(PreVisit, node);

    const TVariable *variable = &node->variable();
    const TType &type         = node->getType();

    if (mOptions.validateVariableReferences && variableNeedsDeclaration(variable))
    {
        // If it's a reference to a field of a nameless interface block, match it by index and name.
        if (type.getInterfaceBlock() && !type.isInterfaceBlock())
        {
            const TInterfaceBlock *interfaceBlock = type.getInterfaceBlock();
            const TFieldList &fieldList           = interfaceBlock->fields();
            const size_t fieldIndex               = type.getInterfaceBlockFieldIndex();

            if (mNamelessInterfaceBlocks.count(interfaceBlock) == 0)
            {
                mDiagnostics->error(node->getLine(),
                                    "Found reference to undeclared or inconsistenly redeclared "
                                    "nameless interface block <validateVariableReferences>",
                                    node->getName().data());
                mVariableReferencesFailed = true;
            }
            else if (fieldIndex >= fieldList.size() ||
                     node->getName() != fieldList[fieldIndex]->name())
            {
                mDiagnostics->error(node->getLine(),
                                    "Found reference to inconsistenly redeclared nameless "
                                    "interface block field <validateVariableReferences>",
                                    node->getName().data());
                mVariableReferencesFailed = true;
            }
        }
        else
        {
            if (!isVariableDeclared(variable))
            {
                mDiagnostics->error(node->getLine(),
                                    "Found reference to undeclared or inconsistently redeclared "
                                    "variable <validateVariableReferences>",
                                    node->getName().data());
                mVariableReferencesFailed = true;
            }
        }
    }
}

void ValidateAST::visitConstantUnion(TIntermConstantUnion *node)
{
    visitNode(PreVisit, node);
}

bool ValidateAST::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitBinary(Visit visit, TIntermBinary *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitUnary(Visit visit, TIntermUnary *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitTernary(Visit visit, TIntermTernary *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitIfElse(Visit visit, TIntermIfElse *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitSwitch(Visit visit, TIntermSwitch *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitCase(Visit visit, TIntermCase *node)
{
    visitNode(visit, node);
    return true;
}

void ValidateAST::visitFunctionPrototype(TIntermFunctionPrototype *node)
{
    visitNode(PreVisit, node);
}

bool ValidateAST::visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node)
{
    visitNode(visit, node);
    scope(visit);

    if (mOptions.validateVariableReferences && visit == PreVisit)
    {
        const TFunction *function = node->getFunction();

        size_t paramCount = function->getParamCount();
        for (size_t paramIndex = 0; paramIndex < paramCount; ++paramIndex)
        {
            const TVariable *variable = function->getParam(paramIndex);

            if (isVariableDeclared(variable))
            {
                mDiagnostics->error(node->getLine(),
                                    "Found two declarations of the same function argument "
                                    "<validateVariableReferences>",
                                    variable->name().data());
                mVariableReferencesFailed = true;
                break;
            }

            mDeclaredVariables.back().insert(variable);
        }
    }

    return true;
}

bool ValidateAST::visitAggregate(Visit visit, TIntermAggregate *node)
{
    visitNode(visit, node);
    expectNonNullChildren(visit, node, 0);
    return true;
}

bool ValidateAST::visitBlock(Visit visit, TIntermBlock *node)
{
    visitNode(visit, node);
    scope(visit);
    expectNonNullChildren(visit, node, 0);
    return true;
}

bool ValidateAST::visitGlobalQualifierDeclaration(Visit visit,
                                                  TIntermGlobalQualifierDeclaration *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitDeclaration(Visit visit, TIntermDeclaration *node)
{
    visitNode(visit, node);
    expectNonNullChildren(visit, node, 0);

    const TIntermSequence &sequence = *(node->getSequence());

    if (mOptions.validateMultiDeclarations && sequence.size() > 1)
    {
        mMultiDeclarationsFailed = true;
    }

    if (mOptions.validateVariableReferences && visit == PreVisit)
    {
        for (TIntermNode *instance : sequence)
        {
            TIntermSymbol *symbol = instance->getAsSymbolNode();
            if (symbol == nullptr)
            {
                TIntermBinary *init = instance->getAsBinaryNode();
                ASSERT(init && init->getOp() == EOpInitialize);
                symbol = init->getLeft()->getAsSymbolNode();
            }
            ASSERT(symbol);

            const TVariable *variable = &symbol->variable();

            if (isVariableDeclared(variable))
            {
                mDiagnostics->error(
                    node->getLine(),
                    "Found two declarations of the same variable <validateVariableReferences>",
                    variable->name().data());
                mVariableReferencesFailed = true;
                break;
            }

            mDeclaredVariables.back().insert(variable);

            const TInterfaceBlock *interfaceBlock = variable->getType().getInterfaceBlock();

            if (variable->symbolType() == SymbolType::Empty && interfaceBlock != nullptr)
            {
                // Nameless interface blocks can only be declared at the top level.  Their fields
                // are matched by field index, and then verified to match by name.  Conflict in
                // names should have already generated a compile error.
                ASSERT(mDeclaredVariables.size() == 1);
                ASSERT(mNamelessInterfaceBlocks.count(interfaceBlock) == 0);

                mNamelessInterfaceBlocks.insert(interfaceBlock);
            }
        }
    }

    return true;
}

bool ValidateAST::visitLoop(Visit visit, TIntermLoop *node)
{
    visitNode(visit, node);
    return true;
}

bool ValidateAST::visitBranch(Visit visit, TIntermBranch *node)
{
    visitNode(visit, node);
    return true;
}

void ValidateAST::visitPreprocessorDirective(TIntermPreprocessorDirective *node)
{
    visitNode(PreVisit, node);
}

bool ValidateAST::validateInternal()
{
    return !mSingleParentFailed && !mVariableReferencesFailed && !mNullNodesFailed &&
           !mMultiDeclarationsFailed;
}

}  // anonymous namespace

bool ValidateAST(TIntermNode *root, TDiagnostics *diagnostics, const ValidateASTOptions &options)
{
    return ValidateAST::validate(root, diagnostics, options);
}

}  // namespace sh

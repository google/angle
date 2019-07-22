//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteAtomicCounters: Emulate atomic counter buffers with storage buffers.
//

#include "compiler/translator/tree_ops/RewriteAtomicCounters.h"

#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{
constexpr ImmutableString kAtomicCounterTypeName  = ImmutableString("ANGLE_atomic_uint");
constexpr ImmutableString kAtomicCounterBlockName = ImmutableString("ANGLEAtomicCounters");
constexpr ImmutableString kAtomicCounterVarName   = ImmutableString("atomicCounters");
constexpr ImmutableString kAtomicCounterFieldName = ImmutableString("counters");

// DeclareAtomicCountersBuffer adds a storage buffer array that's used with atomic counters.
const TVariable *DeclareAtomicCountersBuffers(TIntermBlock *root, TSymbolTable *symbolTable)
{
    // Define `uint counters[];` as the only field in the interface block.
    TFieldList *fieldList = new TFieldList;
    TType *counterType    = new TType(EbtUInt);
    counterType->makeArray(0);

    TField *countersField =
        new TField(counterType, kAtomicCounterFieldName, TSourceLoc(), SymbolType::AngleInternal);

    fieldList->push_back(countersField);

    TMemoryQualifier coherentMemory = TMemoryQualifier::Create();
    coherentMemory.coherent         = true;

    // There are a maximum of 8 atomic counter buffers per IMPLEMENTATION_MAX_ATOMIC_COUNTER_BUFFERS
    // in libANGLE/Constants.h.
    constexpr uint32_t kMaxAtomicCounterBuffers = 8;

    // Define a storage block "ANGLEAtomicCounters" with instance name "atomicCounters".
    return DeclareInterfaceBlock(root, symbolTable, fieldList, EvqBuffer, coherentMemory,
                                 kMaxAtomicCounterBuffers, kAtomicCounterBlockName,
                                 kAtomicCounterVarName);
}

TIntermConstantUnion *CreateUIntConstant(uint32_t value)
{
    TType *constantType = new TType(*StaticType::GetBasic<EbtUInt, 1>());
    constantType->setQualifier(EvqConst);

    TConstantUnion *constantValue = new TConstantUnion;
    constantValue->setUConst(value);
    return new TIntermConstantUnion(constantValue, *constantType);
}

TIntermTyped *CreateAtomicCounterConstant(TType *atomicCounterType,
                                          uint32_t binding,
                                          uint32_t offset)
{
    ASSERT(atomicCounterType->getBasicType() == EbtStruct);

    TIntermSequence *arguments = new TIntermSequence();
    arguments->push_back(CreateUIntConstant(binding));
    arguments->push_back(CreateUIntConstant(offset));

    return TIntermAggregate::CreateConstructor(*atomicCounterType, arguments);
}

TIntermBinary *CreateAtomicCounterRef(const TVariable *atomicCounters,
                                      const TIntermTyped *bindingOffset,
                                      const TIntermTyped *bufferOffsets)
{
    // The atomic counters storage buffer declaration looks as such:
    //
    // layout(...) buffer ANGLEAtomicCounters
    // {
    //     uint counters[];
    // } atomicCounters[N];
    //
    // Where N is large enough to accommodate atomic counter buffer bindings used in the shader.
    //
    // Given an ANGLEAtomicCounter variable (which is a struct of {binding, offset}), we need to
    // return:
    //
    // atomicCounters[binding].counters[offset]
    //
    // The offset itself is the provided one plus an offset given through uniforms.

    TIntermSymbol *atomicCountersRef = new TIntermSymbol(atomicCounters);

    TIntermConstantUnion *bindingFieldRef  = CreateIndexNode(0);
    TIntermConstantUnion *offsetFieldRef   = CreateIndexNode(1);
    TIntermConstantUnion *countersFieldRef = CreateIndexNode(0);

    // Create references to bindingOffset.binding and bindingOffset.offset.
    TIntermBinary *binding =
        new TIntermBinary(EOpIndexDirectStruct, bindingOffset->deepCopy(), bindingFieldRef);
    TIntermBinary *offset =
        new TIntermBinary(EOpIndexDirectStruct, bindingOffset->deepCopy(), offsetFieldRef);

    // Create reference to atomicCounters[bindingOffset.binding]
    TIntermBinary *countersBlock = new TIntermBinary(EOpIndexDirect, atomicCountersRef, binding);

    // Create reference to atomicCounters[bindingOffset.binding].counters
    TIntermBinary *counters =
        new TIntermBinary(EOpIndexDirectInterfaceBlock, countersBlock, countersFieldRef);

    // Create bufferOffsets[binding / 4].  Each uint in bufferOffsets contains offsets for 4
    // bindings.
    TIntermBinary *bindingDivFour =
        new TIntermBinary(EOpDiv, binding->deepCopy(), CreateUIntConstant(4));
    TIntermBinary *bufferOffsetUint =
        new TIntermBinary(EOpIndexDirect, bufferOffsets->deepCopy(), bindingDivFour);

    // Create (binding % 4) * 8
    TIntermBinary *bindingModFour =
        new TIntermBinary(EOpIMod, binding->deepCopy(), CreateUIntConstant(4));
    TIntermBinary *bufferOffsetShift =
        new TIntermBinary(EOpMul, bindingModFour, CreateUIntConstant(8));

    // Create bufferOffsets[binding / 4] >> ((binding % 4) * 8) & 0xFF
    TIntermBinary *bufferOffsetShifted =
        new TIntermBinary(EOpBitShiftRight, bufferOffsetUint, bufferOffsetShift);
    TIntermBinary *bufferOffset =
        new TIntermBinary(EOpBitwiseAnd, bufferOffsetShifted, CreateUIntConstant(0xFF));

    // return atomicCounters[bindingOffset.binding].counters[bindingOffset.offset + bufferOffset]
    offset = new TIntermBinary(EOpAdd, offset, bufferOffset);
    return new TIntermBinary(EOpIndexDirect, counters, offset);
}

// Traverser that:
//
// 1. Converts the |atomic_uint| types to |{uint,uint}| for binding and offset.
// 2. Substitutes the |uniform atomic_uint| declarations with a global declaration that holds the
//    binding and offset.
// 3. Substitutes |atomicVar[n]| with |buffer[binding].counters[offset + n]|.
class RewriteAtomicCountersTraverser : public TIntermTraverser
{
  public:
    RewriteAtomicCountersTraverser(TSymbolTable *symbolTable,
                                   const TVariable *atomicCounters,
                                   const TIntermTyped *acbBufferOffsets)
        : TIntermTraverser(true, true, true, symbolTable),
          mAtomicCounters(atomicCounters),
          mAcbBufferOffsets(acbBufferOffsets),
          mCurrentAtomicCounterOffset(0),
          mCurrentAtomicCounterBinding(0),
          mCurrentAtomicCounterDecl(nullptr),
          mCurrentAtomicCounterDeclParent(nullptr),
          mAtomicCounterType(nullptr),
          mAtomicCounterTypeConst(nullptr),
          mAtomicCounterTypeDeclaration(nullptr)
    {}

    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override
    {
        const TIntermSequence &sequence = *(node->getSequence());

        TIntermTyped *variable = sequence.front()->getAsTyped();
        const TType &type      = variable->getType();
        bool isAtomicCounter   = type.getQualifier() == EvqUniform && type.isAtomicCounter();

        if (visit == PreVisit || visit == InVisit)
        {
            if (isAtomicCounter)
            {
                mCurrentAtomicCounterDecl       = node;
                mCurrentAtomicCounterDeclParent = getParentNode()->getAsBlock();
                mCurrentAtomicCounterOffset     = type.getLayoutQualifier().offset;
                mCurrentAtomicCounterBinding    = type.getLayoutQualifier().binding;
            }
        }
        else if (visit == PostVisit)
        {
            mCurrentAtomicCounterDecl       = nullptr;
            mCurrentAtomicCounterDeclParent = nullptr;
            mCurrentAtomicCounterOffset     = 0;
            mCurrentAtomicCounterBinding    = 0;
        }
        return true;
    }

    void visitFunctionPrototype(TIntermFunctionPrototype *node) override
    {
        const TFunction *function = node->getFunction();
        // Go over the parameters and replace the atomic arguments with a uint type.  If this is
        // the function definition, keep the replaced variable for future encounters.
        mAtomicCounterFunctionParams.clear();
        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            const TVariable *param = function->getParam(paramIndex);
            TVariable *replacement = convertFunctionParameter(node, param);
            if (replacement)
            {
                mAtomicCounterFunctionParams[param] = replacement;
            }
        }

        if (mAtomicCounterFunctionParams.empty())
        {
            return;
        }

        // Create a new function prototype and replace this with it.
        TFunction *replacementFunction = new TFunction(
            mSymbolTable, function->name(), SymbolType::UserDefined,
            new TType(function->getReturnType()), function->isKnownToNotHaveSideEffects());
        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            const TVariable *param = function->getParam(paramIndex);
            TVariable *replacement = nullptr;
            if (param->getType().isAtomicCounter())
            {
                ASSERT(mAtomicCounterFunctionParams.count(param) != 0);
                replacement = mAtomicCounterFunctionParams[param];
            }
            else
            {
                replacement = new TVariable(mSymbolTable, param->name(),
                                            new TType(param->getType()), SymbolType::UserDefined);
            }
            replacementFunction->addParameter(replacement);
        }

        TIntermFunctionPrototype *replacementPrototype =
            new TIntermFunctionPrototype(replacementFunction);
        queueReplacement(replacementPrototype, OriginalNode::IS_DROPPED);

        mReplacedFunctions[function] = replacementFunction;
    }

    bool visitAggregate(Visit visit, TIntermAggregate *node) override
    {
        if (visit == PreVisit)
        {
            mAtomicCounterFunctionCallArgs.clear();
        }

        if (visit != PostVisit)
        {
            return true;
        }

        if (node->getOp() == EOpCallBuiltInFunction)
        {
            convertBuiltinFunction(node);
        }
        else if (node->getOp() == EOpCallFunctionInAST)
        {
            convertASTFunction(node);
        }

        return true;
    }

    void visitSymbol(TIntermSymbol *symbol) override
    {
        const TVariable *symbolVariable = &symbol->variable();

        if (mCurrentAtomicCounterDecl)
        {
            declareAtomicCounter(symbolVariable);
            return;
        }

        if (!symbol->getType().isAtomicCounter())
        {
            return;
        }

        // The symbol is either referencing a global atomic counter, or is a function parameter.  In
        // either case, it could be an array.  The are the following possibilities:
        //
        //     layout(..) uniform atomic_uint ac;
        //     layout(..) uniform atomic_uint acArray[N];
        //
        //     void func(inout atomic_uint c)
        //     {
        //         otherFunc(c);
        //     }
        //
        //     void funcArray(inout atomic_uint cArray[N])
        //     {
        //         otherFuncArray(cArray);
        //         otherFunc(cArray[n]);
        //     }
        //
        //     void funcGlobal()
        //     {
        //         func(ac);
        //         func(acArray[n]);
        //         funcArray(acArray);
        //         atomicIncrement(ac);
        //         atomicIncrement(acArray[n]);
        //     }
        //
        // This should translate to:
        //
        //     buffer ANGLEAtomicCounters
        //     {
        //         uint counters[];
        //     } atomicCounters;
        //
        //     struct ANGLEAtomicCounter
        //     {
        //         uint binding;
        //         uint offset;
        //     };
        //     const ANGLEAtomicCounter ac = {<binding>, <offset>};
        //     const ANGLEAtomicCounter acArray = {<binding>, <offset>};
        //
        //     void func(inout ANGLEAtomicCounter c)
        //     {
        //         otherFunc(c);
        //     }
        //
        //     void funcArray(inout uint cArray)
        //     {
        //         otherFuncArray(cArray);
        //         otherFunc({cArray.binding, cArray.offset + n});
        //     }
        //
        //     void funcGlobal()
        //     {
        //         func(ac);
        //         func(acArray+n);
        //         funcArray(acArray);
        //         atomicAdd(atomicCounters[ac.binding]counters[ac.offset]);
        //         atomicAdd(atomicCounters[ac.binding]counters[ac.offset+n]);
        //     }
        //
        // In all cases, the argument transformation is stored in |mAtomicCounterFunctionCallArgs|.
        // In the function call's PostVisit, if it's a builtin, the look up in
        // |atomicCounters.counters| is done as well as the builtin function change.  Otherwise,
        // the transformed argument is passed on as is.
        //

        TIntermTyped *bindingOffset = nullptr;
        if (mAtomicCounterBindingOffsets.count(symbolVariable) != 0)
        {
            bindingOffset = new TIntermSymbol(mAtomicCounterBindingOffsets[symbolVariable]);
        }
        else
        {
            ASSERT(mAtomicCounterFunctionParams.count(symbolVariable) != 0);
            bindingOffset = new TIntermSymbol(mAtomicCounterFunctionParams[symbolVariable]);
        }

        TIntermNode *argument = symbol;

        TIntermNode *parent = getParentNode();
        ASSERT(parent);

        TIntermBinary *arrayExpression = parent->getAsBinaryNode();
        if (arrayExpression)
        {
            ASSERT(arrayExpression->getOp() == EOpIndexDirect ||
                   arrayExpression->getOp() == EOpIndexIndirect);

            argument = arrayExpression;

            TIntermTyped *subscript                   = arrayExpression->getRight();
            TIntermConstantUnion *subscriptAsConstant = subscript->getAsConstantUnion();
            const bool subscriptIsZero = subscriptAsConstant && subscriptAsConstant->isZero(0);

            if (!subscriptIsZero)
            {
                // Copy the atomic counter binding/offset constant and modify it by adding the array
                // subscript to its offset field.
                TVariable *modified = CreateTempVariable(mSymbolTable, mAtomicCounterType);
                TIntermDeclaration *modifiedDecl =
                    CreateTempInitDeclarationNode(modified, bindingOffset);

                TIntermSymbol *modifiedSymbol    = new TIntermSymbol(modified);
                TConstantUnion *offsetFieldIndex = new TConstantUnion;
                offsetFieldIndex->setIConst(1);
                TIntermConstantUnion *offsetFieldRef =
                    new TIntermConstantUnion(offsetFieldIndex, *StaticType::GetBasic<EbtUInt>());
                TIntermBinary *offsetField =
                    new TIntermBinary(EOpIndexDirectStruct, modifiedSymbol, offsetFieldRef);

                TIntermBinary *modifiedOffset = new TIntermBinary(
                    EOpAddAssign, offsetField, arrayExpression->getRight()->deepCopy());

                TIntermSequence *modifySequence = new TIntermSequence();
                modifySequence->push_back(modifiedDecl);
                modifySequence->push_back(modifiedOffset);
                insertStatementsInParentBlock(*modifySequence);

                bindingOffset = modifiedSymbol->deepCopy();
            }
        }

        mAtomicCounterFunctionCallArgs[argument] = bindingOffset;
    }

    TIntermDeclaration *getAtomicCounterTypeDeclaration() { return mAtomicCounterTypeDeclaration; }

  private:
    void declareAtomicCounter(const TVariable *symbolVariable)
    {
        // Create a global variable that contains the binding and offset of this atomic counter
        // declaration.
        if (mAtomicCounterType == nullptr)
        {
            declareAtomicCounterType();
        }
        ASSERT(mAtomicCounterTypeConst);

        TVariable *bindingOffset = new TVariable(mSymbolTable, symbolVariable->name(),
                                                 mAtomicCounterTypeConst, SymbolType::UserDefined);

        ASSERT(mCurrentAtomicCounterOffset % 4 == 0);
        TIntermTyped *bindingOffsetInitValue = CreateAtomicCounterConstant(
            mAtomicCounterTypeConst, mCurrentAtomicCounterBinding, mCurrentAtomicCounterOffset / 4);

        TIntermSymbol *bindingOffsetSymbol = new TIntermSymbol(bindingOffset);
        TIntermBinary *bindingOffsetInit =
            new TIntermBinary(EOpInitialize, bindingOffsetSymbol, bindingOffsetInitValue);

        TIntermDeclaration *bindingOffsetDeclaration = new TIntermDeclaration();
        bindingOffsetDeclaration->appendDeclarator(bindingOffsetInit);

        // Replace the atomic_uint declaration with the binding/offset declaration.
        TIntermSequence replacement;
        replacement.push_back(bindingOffsetDeclaration);
        mMultiReplacements.emplace_back(mCurrentAtomicCounterDeclParent, mCurrentAtomicCounterDecl,
                                        replacement);

        // Remember the binding/offset variable.
        mAtomicCounterBindingOffsets[symbolVariable] = bindingOffset;
    }

    void declareAtomicCounterType()
    {
        ASSERT(mAtomicCounterType == nullptr);

        TFieldList *fields = new TFieldList();
        fields->push_back(new TField(new TType(EbtUInt, EbpUndefined, EvqGlobal, 1, 1),
                                     ImmutableString("binding"), TSourceLoc(),
                                     SymbolType::AngleInternal));
        fields->push_back(new TField(new TType(EbtUInt, EbpUndefined, EvqGlobal, 1, 1),
                                     ImmutableString("arrayIndex"), TSourceLoc(),
                                     SymbolType::AngleInternal));
        TStructure *atomicCounterTypeStruct =
            new TStructure(mSymbolTable, kAtomicCounterTypeName, fields, SymbolType::AngleInternal);
        mAtomicCounterType = new TType(atomicCounterTypeStruct, false);

        mAtomicCounterTypeDeclaration = new TIntermDeclaration;
        TVariable *emptyVariable      = new TVariable(mSymbolTable, kEmptyImmutableString,
                                                 mAtomicCounterType, SymbolType::Empty);
        mAtomicCounterTypeDeclaration->appendDeclarator(new TIntermSymbol(emptyVariable));

        // Keep a const variant around as well.
        mAtomicCounterTypeConst = new TType(*mAtomicCounterType);
        mAtomicCounterTypeConst->setQualifier(EvqConst);
    }

    TVariable *convertFunctionParameter(TIntermNode *parent, const TVariable *param)
    {
        if (!param->getType().isAtomicCounter())
        {
            return nullptr;
        }
        if (mAtomicCounterType == nullptr)
        {
            declareAtomicCounterType();
        }

        const TType *paramType = &param->getType();
        TType *newType =
            paramType->getQualifier() == EvqConst ? mAtomicCounterTypeConst : mAtomicCounterType;

        TVariable *replacementVar =
            new TVariable(mSymbolTable, param->name(), newType, SymbolType::UserDefined);

        return replacementVar;
    }

    void convertBuiltinFunction(TIntermAggregate *node)
    {
        // If the function is |memoryBarrierAtomicCounter|, simply replace it with
        // |memoryBarrierBuffer|.
        if (node->getFunction()->name() == "memoryBarrierAtomicCounter")
        {
            TIntermTyped *substituteCall = CreateBuiltInFunctionCallNode(
                "memoryBarrierBuffer", new TIntermSequence, *mSymbolTable, 310);
            queueReplacement(substituteCall, OriginalNode::IS_DROPPED);
            return;
        }

        // If it's an |atomicCounter*| function, replace the function with an |atomic*| equivalent.
        if (!node->getFunction()->isAtomicCounterFunction())
        {
            return;
        }

        const ImmutableString &functionName = node->getFunction()->name();
        TIntermSequence *arguments          = node->getSequence();

        // Note: atomicAdd(0) is used for atomic reads.
        uint32_t valueChange                = 0;
        constexpr char kAtomicAddFunction[] = "atomicAdd";
        bool isDecrement                    = false;

        if (functionName == "atomicCounterIncrement")
        {
            valueChange = 1;
        }
        else if (functionName == "atomicCounterDecrement")
        {
            // uint values are required to wrap around, so 0xFFFFFFFFu is used as -1.
            valueChange = std::numeric_limits<uint32_t>::max();
            static_assert(static_cast<uint32_t>(-1) == std::numeric_limits<uint32_t>::max(),
                          "uint32_t max is not -1");

            isDecrement = true;
        }
        else
        {
            ASSERT(functionName == "atomicCounter");
        }

        const TIntermNode *param = (*arguments)[0];
        ASSERT(mAtomicCounterFunctionCallArgs.count(param) != 0);

        TIntermTyped *bindingOffset = mAtomicCounterFunctionCallArgs[param];

        TIntermSequence *substituteArguments = new TIntermSequence;
        substituteArguments->push_back(
            CreateAtomicCounterRef(mAtomicCounters, bindingOffset, mAcbBufferOffsets));
        substituteArguments->push_back(CreateUIntConstant(valueChange));

        TIntermTyped *substituteCall = CreateBuiltInFunctionCallNode(
            kAtomicAddFunction, substituteArguments, *mSymbolTable, 310);

        // Note that atomicCounterDecrement returns the *new* value instead of the prior value,
        // unlike atomicAdd.  So we need to do a -1 on the result as well.
        if (isDecrement)
        {
            substituteCall = new TIntermBinary(EOpSub, substituteCall, CreateUIntConstant(1));
        }

        queueReplacement(substituteCall, OriginalNode::IS_DROPPED);
    }

    void convertASTFunction(TIntermAggregate *node)
    {
        // See if the function needs replacement at all.
        const TFunction *function = node->getFunction();
        if (mReplacedFunctions.count(function) == 0)
        {
            return;
        }

        // atomic_uint arguments to this call are staged to be replaced at the same time.
        TFunction *substituteFunction        = mReplacedFunctions[function];
        TIntermSequence *substituteArguments = new TIntermSequence;

        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            TIntermNode *param = node->getChildNode(paramIndex);

            TIntermNode *replacement = nullptr;
            if (param->getAsTyped()->getType().isAtomicCounter())
            {
                ASSERT(mAtomicCounterFunctionCallArgs.count(param) != 0);
                replacement = mAtomicCounterFunctionCallArgs[param];
            }
            else
            {
                replacement = param->getAsTyped()->deepCopy();
            }
            substituteArguments->push_back(replacement);
        }

        TIntermTyped *substituteCall =
            TIntermAggregate::CreateFunctionCall(*substituteFunction, substituteArguments);

        queueReplacement(substituteCall, OriginalNode::IS_DROPPED);
    }

    const TVariable *mAtomicCounters;
    const TIntermTyped *mAcbBufferOffsets;

    // A map from the atomic_uint variable to the binding/offset declaration.
    std::unordered_map<const TVariable *, TVariable *> mAtomicCounterBindingOffsets;
    // A map from functions with atomic_uint parameters to one where that's replaced with uint.
    std::unordered_map<const TFunction *, TFunction *> mReplacedFunctions;
    // A map from atomic_uint function parameters to their replacement uint parameter for the
    // current function definition.
    std::unordered_map<const TVariable *, TVariable *> mAtomicCounterFunctionParams;
    // A map from atomic_uint function call arguments to their replacement for the current
    // non-builtin function call.
    std::unordered_map<const TIntermNode *, TIntermTyped *> mAtomicCounterFunctionCallArgs;

    uint32_t mCurrentAtomicCounterOffset;
    uint32_t mCurrentAtomicCounterBinding;
    TIntermDeclaration *mCurrentAtomicCounterDecl;
    TIntermAggregateBase *mCurrentAtomicCounterDeclParent;

    TType *mAtomicCounterType;
    TType *mAtomicCounterTypeConst;

    // Stored to be put at the top of the shader after the pass.
    TIntermDeclaration *mAtomicCounterTypeDeclaration;
};

}  // anonymous namespace

void RewriteAtomicCounters(TIntermBlock *root,
                           TSymbolTable *symbolTable,
                           const TIntermTyped *acbBufferOffsets)
{
    const TVariable *atomicCounters = DeclareAtomicCountersBuffers(root, symbolTable);

    RewriteAtomicCountersTraverser traverser(symbolTable, atomicCounters, acbBufferOffsets);
    root->traverse(&traverser);
    traverser.updateTree();

    TIntermDeclaration *atomicCounterTypeDeclaration = traverser.getAtomicCounterTypeDeclaration();
    if (atomicCounterTypeDeclaration)
    {
        root->getSequence()->insert(root->getSequence()->begin(), atomicCounterTypeDeclaration);
    }
}
}  // namespace sh

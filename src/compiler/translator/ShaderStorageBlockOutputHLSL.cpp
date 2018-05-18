//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ShaderStorageBlockOutputHLSL: A traverser to translate a ssbo_access_chain to an offset of
// RWByteAddressBuffer.
//     //EOpIndexDirectInterfaceBlock
//     ssbo_variable :=
//       | the name of the SSBO
//       | the name of a variable in an SSBO backed interface block

//     // EOpIndexInDirect
//     // EOpIndexDirect
//     ssbo_array_indexing := ssbo_access_chain[expr_no_ssbo]

//     // EOpIndexDirectStruct
//     ssbo_structure_access := ssbo_access_chain.identifier

//     ssbo_access_chain :=
//       | ssbo_variable
//       | ssbo_array_indexing
//       | ssbo_structure_access
//

#include "compiler/translator/ShaderStorageBlockOutputHLSL.h"

#include "compiler/translator/ResourcesHLSL.h"
#include "compiler/translator/blocklayout.h"
#include "compiler/translator/blocklayoutHLSL.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

const TField *GetFieldMemberInShaderStorageBlock(const TInterfaceBlock *interfaceBlock,
                                                 const ImmutableString &variableName)
{
    for (const TField *field : interfaceBlock->fields())
    {
        if (field->name() == variableName)
        {
            return field;
        }
    }
    return nullptr;
}

void SetShaderStorageBlockFieldMemberInfo(const TFieldList &fields, sh::BlockLayoutEncoder *encoder)
{
    for (TField *field : fields)
    {
        const TType &fieldType = *field->type();
        if (fieldType.getStruct())
        {
            // TODO(jiajia.qin@intel.com): Add structure field member support.
        }
        else if (fieldType.isArrayOfArrays())
        {
            // TODO(jiajia.qin@intel.com): Add array of array field member support.
        }
        else
        {
            std::vector<unsigned int> fieldArraySizes;
            if (auto *arraySizes = fieldType.getArraySizes())
            {
                fieldArraySizes.assign(arraySizes->begin(), arraySizes->end());
            }
            const bool isRowMajorLayout =
                (fieldType.getLayoutQualifier().matrixPacking == EmpRowMajor);
            const BlockMemberInfo &memberInfo =
                encoder->encodeType(GLVariableType(fieldType), fieldArraySizes,
                                    isRowMajorLayout && fieldType.isMatrix());
            field->setOffset(memberInfo.offset);
            field->setArrayStride(memberInfo.arrayStride);
        }
    }
}

void SetShaderStorageBlockMembersOffset(const TInterfaceBlock *interfaceBlock)
{
    sh::Std140BlockEncoder std140Encoder;
    sh::HLSLBlockEncoder hlslEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED, false);
    sh::BlockLayoutEncoder *encoder = nullptr;

    if (interfaceBlock->blockStorage() == EbsStd140)
    {
        encoder = &std140Encoder;
    }
    else
    {
        // TODO(jiajia.qin@intel.com): add std430 support.
        encoder = &hlslEncoder;
    }

    SetShaderStorageBlockFieldMemberInfo(interfaceBlock->fields(), encoder);
}

}  // anonymous namespace

ShaderStorageBlockOutputHLSL::ShaderStorageBlockOutputHLSL(OutputHLSL *outputHLSL,
                                                           TSymbolTable *symbolTable,
                                                           ResourcesHLSL *resourcesHLSL)
    : TIntermTraverser(true, true, true, symbolTable),
      mIsLoadFunctionCall(false),
      mOutputHLSL(outputHLSL),
      mResourcesHLSL(resourcesHLSL)
{
    mSSBOFunctionHLSL = new ShaderStorageBlockFunctionHLSL;
}

ShaderStorageBlockOutputHLSL::~ShaderStorageBlockOutputHLSL()
{
    SafeDelete(mSSBOFunctionHLSL);
}

void ShaderStorageBlockOutputHLSL::outputStoreFunctionCallPrefix(TIntermTyped *node)
{
    mIsLoadFunctionCall = false;
    traverseSSBOAccess(node, SSBOMethod::STORE);
}

void ShaderStorageBlockOutputHLSL::outputLoadFunctionCall(TIntermTyped *node)
{
    mIsLoadFunctionCall = true;
    traverseSSBOAccess(node, SSBOMethod::LOAD);
}

void ShaderStorageBlockOutputHLSL::traverseSSBOAccess(TIntermTyped *node, SSBOMethod method)
{
    const TString &functionName =
        mSSBOFunctionHLSL->registerShaderStorageBlockFunction(node->getType(), method);
    TInfoSinkBase &out = mOutputHLSL->getInfoSink();
    out << functionName;
    out << "(";
    node->traverse(this);
}

void ShaderStorageBlockOutputHLSL::writeShaderStorageBlocksHeader(TInfoSinkBase &out) const
{
    out << mResourcesHLSL->shaderStorageBlocksHeader(mReferencedShaderStorageBlocks);
    mSSBOFunctionHLSL->shaderStorageBlockFunctionHeader(out);
}

// Check if the current node is the end of the sssbo access chain. If true, we should output ')' for
// Load method.
bool ShaderStorageBlockOutputHLSL::isEndOfSSBOAccessChain()
{
    TIntermNode *parent = getParentNode();
    if (parent)
    {
        TIntermBinary *parentBinary = parent->getAsBinaryNode();
        if (parentBinary != nullptr)
        {
            switch (parentBinary->getOp())
            {
                case EOpIndexDirectStruct:
                case EOpIndexDirect:
                case EOpIndexIndirect:
                {
                    return false;
                }
                default:
                    return true;
            }
        }

        const TIntermSwizzle *parentSwizzle = parent->getAsSwizzleNode();
        if (parentSwizzle)
        {
            return false;
        }
    }
    return true;
}

void ShaderStorageBlockOutputHLSL::visitSymbol(TIntermSymbol *node)
{
    TInfoSinkBase &out        = mOutputHLSL->getInfoSink();
    const TVariable &variable = node->variable();
    TQualifier qualifier      = variable.getType().getQualifier();

    if (qualifier == EvqBuffer)
    {
        const TType &variableType             = variable.getType();
        const TInterfaceBlock *interfaceBlock = variableType.getInterfaceBlock();
        ASSERT(interfaceBlock);
        if (mReferencedShaderStorageBlocks.count(interfaceBlock->uniqueId().get()) == 0)
        {
            const TVariable *instanceVariable = nullptr;
            if (variableType.isInterfaceBlock())
            {
                instanceVariable = &variable;
            }
            mReferencedShaderStorageBlocks[interfaceBlock->uniqueId().get()] =
                new TReferencedBlock(interfaceBlock, instanceVariable);
            SetShaderStorageBlockMembersOffset(interfaceBlock);
        }
        if (variableType.isInterfaceBlock())
        {
            out << DecorateVariableIfNeeded(variable);
        }
        else
        {
            out << Decorate(interfaceBlock->name());
            out << ", ";

            const TField *field =
                GetFieldMemberInShaderStorageBlock(interfaceBlock, variable.name());
            writeDotOperatorOutput(out, field);
        }
    }
    else
    {
        return mOutputHLSL->visitSymbol(node);
    }
}

void ShaderStorageBlockOutputHLSL::visitConstantUnion(TIntermConstantUnion *node)
{
    TInfoSinkBase &out = mOutputHLSL->getInfoSink();
    mOutputHLSL->writeConstantUnion(out, node->getType(), node->getConstantValue());
}

bool ShaderStorageBlockOutputHLSL::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    TInfoSinkBase &out = mOutputHLSL->getInfoSink();
    if (visit == PostVisit)
    {
        // TODO(jiajia.qin@intel.com): add swizzle process.
        if (mIsLoadFunctionCall)
        {
            out << ")";
        }
    }
    return true;
}

bool ShaderStorageBlockOutputHLSL::visitBinary(Visit visit, TIntermBinary *node)
{
    TInfoSinkBase &out = mOutputHLSL->getInfoSink();

    switch (node->getOp())
    {
        case EOpIndexDirect:
        {
            const TType &leftType = node->getLeft()->getType();
            if (leftType.isInterfaceBlock())
            {
                if (visit == PreVisit)
                {
                    ASSERT(leftType.getQualifier() == EvqBuffer);
                    TIntermSymbol *instanceArraySymbol    = node->getLeft()->getAsSymbolNode();
                    const TInterfaceBlock *interfaceBlock = leftType.getInterfaceBlock();

                    if (mReferencedShaderStorageBlocks.count(interfaceBlock->uniqueId().get()) == 0)
                    {
                        mReferencedShaderStorageBlocks[interfaceBlock->uniqueId().get()] =
                            new TReferencedBlock(interfaceBlock, &instanceArraySymbol->variable());
                        SetShaderStorageBlockMembersOffset(interfaceBlock);
                    }

                    const int arrayIndex = node->getRight()->getAsConstantUnion()->getIConst(0);
                    out << mResourcesHLSL->InterfaceBlockInstanceString(
                        instanceArraySymbol->getName(), arrayIndex);
                    return false;
                }
            }
            else
            {
                writeEOpIndexDirectOrIndirectOutput(out, visit, node);
            }
            break;
        }
        case EOpIndexIndirect:
            // We do not currently support indirect references to interface blocks
            ASSERT(node->getLeft()->getBasicType() != EbtInterfaceBlock);
            writeEOpIndexDirectOrIndirectOutput(out, visit, node);
            break;
        case EOpIndexDirectStruct:
            if (visit == InVisit)
            {
                ASSERT(IsInShaderStorageBlock(node->getLeft()));
                const TStructure *structure       = node->getLeft()->getType().getStruct();
                const TIntermConstantUnion *index = node->getRight()->getAsConstantUnion();
                const TField *field               = structure->fields()[index->getIConst(0)];
                writeDotOperatorOutput(out, field);
                return false;
            }
            break;
        case EOpIndexDirectInterfaceBlock:
            if (visit == InVisit)
            {
                ASSERT(IsInShaderStorageBlock(node->getLeft()));
                out << ", ";
                const TInterfaceBlock *interfaceBlock =
                    node->getLeft()->getType().getInterfaceBlock();
                const TIntermConstantUnion *index = node->getRight()->getAsConstantUnion();
                const TField *field               = interfaceBlock->fields()[index->getIConst(0)];
                writeDotOperatorOutput(out, field);
                return false;
            }
            break;
        default:
            // It may have other operators in EOpIndexIndirect. Such as buffer.attribs[(y * gridSize
            // + x) * 6u + 0u]
            return mOutputHLSL->visitBinary(visit, node);
    }

    return true;
}

void ShaderStorageBlockOutputHLSL::writeEOpIndexDirectOrIndirectOutput(TInfoSinkBase &out,
                                                                       Visit visit,
                                                                       TIntermBinary *node)
{
    ASSERT(IsInShaderStorageBlock(node->getLeft()));
    if (visit == InVisit)
    {
        const TType &type = node->getLeft()->getType();
        if (node->getType().isVector() && type.isMatrix())
        {
            int matrixStride =
                BlockLayoutEncoder::ComponentsPerRegister * BlockLayoutEncoder::BytesPerComponent;
            out << " + " << str(matrixStride);
        }
        else if (node->getType().isScalar() && !type.isArray())
        {
            int scalarStride = BlockLayoutEncoder::BytesPerComponent;
            out << " + " << str(scalarStride);
        }

        out << " * ";
    }
    else if (visit == PostVisit && mIsLoadFunctionCall && isEndOfSSBOAccessChain())
    {
        out << ")";
    }
}

void ShaderStorageBlockOutputHLSL::writeDotOperatorOutput(TInfoSinkBase &out, const TField *field)
{
    out << str(field->getOffset());

    const TType &fieldType = *field->type();
    if (fieldType.isArray() && !isEndOfSSBOAccessChain())
    {
        out << " + ";
        out << field->getArrayStride();
    }
    if (mIsLoadFunctionCall && isEndOfSSBOAccessChain())
    {
        out << ")";
    }
}

}  // namespace sh

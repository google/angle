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

void GetShaderStorageBlockFieldMemberInfo(const TFieldList &fields,
                                          sh::BlockLayoutEncoder *encoder,
                                          TLayoutBlockStorage storage,
                                          bool rowMajor,
                                          bool isSSBOFieldMember,
                                          BlockMemberInfoMap *blockInfoOut);

size_t GetBlockFieldMemberInfoAndReturnBlockSize(const TFieldList &fields,
                                                 TLayoutBlockStorage storage,
                                                 bool rowMajor,
                                                 BlockMemberInfoMap *blockInfoOut)
{
    sh::Std140BlockEncoder std140Encoder;
    sh::HLSLBlockEncoder hlslEncoder(sh::HLSLBlockEncoder::ENCODE_PACKED, false);
    sh::BlockLayoutEncoder *structureEncoder = nullptr;

    if (storage == EbsStd140)
    {
        structureEncoder = &std140Encoder;
    }
    else
    {
        // TODO(jiajia.qin@intel.com): add std430 support.
        structureEncoder = &hlslEncoder;
    }

    GetShaderStorageBlockFieldMemberInfo(fields, structureEncoder, storage, rowMajor, false,
                                         blockInfoOut);
    structureEncoder->exitAggregateType();
    return structureEncoder->getBlockSize();
}

void GetShaderStorageBlockFieldMemberInfo(const TFieldList &fields,
                                          sh::BlockLayoutEncoder *encoder,
                                          TLayoutBlockStorage storage,
                                          bool rowMajor,
                                          bool isSSBOFieldMember,
                                          BlockMemberInfoMap *blockInfoOut)
{
    for (const TField *field : fields)
    {
        const TType &fieldType = *field->type();
        bool isRowMajorLayout  = rowMajor;
        if (isSSBOFieldMember)
        {
            isRowMajorLayout = (fieldType.getLayoutQualifier().matrixPacking == EmpRowMajor);
        }
        if (fieldType.getStruct())
        {
            encoder->enterAggregateType();
            // This is to set structure member offset and array stride using a new encoder to ensure
            // that the first field member offset in structure is always zero.
            size_t structureStride = GetBlockFieldMemberInfoAndReturnBlockSize(
                fieldType.getStruct()->fields(), storage, isRowMajorLayout, blockInfoOut);
            const BlockMemberInfo memberInfo(static_cast<int>(encoder->getBlockSize()),
                                             static_cast<int>(structureStride), 0, false);
            (*blockInfoOut)[field] = memberInfo;

            // Below if-else is in order to get correct offset for the field members after structure
            // field.
            if (fieldType.isArray())
            {
                size_t size = fieldType.getArraySizeProduct() * structureStride;
                encoder->increaseCurrentOffset(size);
            }
            else
            {
                encoder->increaseCurrentOffset(structureStride);
            }
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
            const BlockMemberInfo &memberInfo =
                encoder->encodeType(GLVariableType(fieldType), fieldArraySizes,
                                    isRowMajorLayout && fieldType.isMatrix());
            (*blockInfoOut)[field] = memberInfo;
        }
    }
}

void GetShaderStorageBlockMembersInfo(const TInterfaceBlock *interfaceBlock,
                                      BlockMemberInfoMap *blockInfoOut)
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

    GetShaderStorageBlockFieldMemberInfo(interfaceBlock->fields(), encoder,
                                         interfaceBlock->blockStorage(), false, true, blockInfoOut);
}

bool IsInArrayOfArraysChain(TIntermTyped *node)
{
    if (node->getType().isArrayOfArrays())
        return true;
    TIntermBinary *binaryNode = node->getAsBinaryNode();
    if (binaryNode)
    {
        if (binaryNode->getLeft()->getType().isArrayOfArrays())
            return true;
    }

    return false;
}

}  // anonymous namespace

ShaderStorageBlockOutputHLSL::ShaderStorageBlockOutputHLSL(OutputHLSL *outputHLSL,
                                                           TSymbolTable *symbolTable,
                                                           ResourcesHLSL *resourcesHLSL)
    : TIntermTraverser(true, true, true, symbolTable),
      mMatrixStride(0),
      mRowMajor(false),
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
            GetShaderStorageBlockMembersInfo(interfaceBlock, &mBlockMemberInfoMap);
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
    mOutputHLSL->visitConstantUnion(node);
}

bool ShaderStorageBlockOutputHLSL::visitAggregate(Visit visit, TIntermAggregate *node)
{
    return mOutputHLSL->visitAggregate(visit, node);
}

bool ShaderStorageBlockOutputHLSL::visitTernary(Visit visit, TIntermTernary *node)
{
    return mOutputHLSL->visitTernary(visit, node);
}

bool ShaderStorageBlockOutputHLSL::visitUnary(Visit visit, TIntermUnary *node)
{
    return mOutputHLSL->visitUnary(visit, node);
}

bool ShaderStorageBlockOutputHLSL::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    if (visit == PostVisit)
    {
        if (!IsInShaderStorageBlock(node))
        {
            return mOutputHLSL->visitSwizzle(visit, node);
        }

        TInfoSinkBase &out = mOutputHLSL->getInfoSink();
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
            if (!IsInShaderStorageBlock(node->getLeft()))
            {
                return mOutputHLSL->visitBinary(visit, node);
            }

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
                        GetShaderStorageBlockMembersInfo(interfaceBlock, &mBlockMemberInfoMap);
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
        {
            if (!IsInShaderStorageBlock(node->getLeft()))
            {
                return mOutputHLSL->visitBinary(visit, node);
            }

            // We do not currently support indirect references to interface blocks
            ASSERT(node->getLeft()->getBasicType() != EbtInterfaceBlock);
            writeEOpIndexDirectOrIndirectOutput(out, visit, node);
            break;
        }
        case EOpIndexDirectStruct:
        {
            if (!IsInShaderStorageBlock(node->getLeft()))
            {
                return mOutputHLSL->visitBinary(visit, node);
            }

            if (visit == InVisit)
            {
                ASSERT(IsInShaderStorageBlock(node->getLeft()));
                const TStructure *structure       = node->getLeft()->getType().getStruct();
                const TIntermConstantUnion *index = node->getRight()->getAsConstantUnion();
                const TField *field               = structure->fields()[index->getIConst(0)];
                out << " + ";
                writeDotOperatorOutput(out, field);
                return false;
            }
            break;
        }
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
        // For array of arrays, we calculate the offset using the formula below:
        // elementStride * (a3 * a2 * a1 * i0 + a3 * a2 * i1 + a3 * i2 + i3)
        // Note: assume that there are 4 dimensions.
        //       a0, a1, a2, a3 is the size of the array in each dimension. (S s[a0][a1][a2][a3])
        //       i0, i1, i2, i3 is the index of the array in each dimension. (s[i0][i1][i2][i3])
        if (IsInArrayOfArraysChain(node->getLeft()))
        {
            if (type.isArrayOfArrays())
            {
                const TVector<unsigned int> &arraySizes = *type.getArraySizes();
                // Don't need to concern the tail comma which will be used to multiply the index.
                for (unsigned int i = 0; i < (arraySizes.size() - 1); i++)
                {
                    out << arraySizes[i];
                    out << " * ";
                }
            }
        }
        else
        {
            if (node->getType().isVector() && type.isMatrix())
            {
                if (mRowMajor)
                {
                    out << " + " << str(BlockLayoutEncoder::BytesPerComponent);
                }
                else
                {
                    out << " + " << str(mMatrixStride);
                }
            }
            else if (node->getType().isScalar() && !type.isArray())
            {
                if (mRowMajor)
                {
                    out << " + " << str(mMatrixStride);
                }
                else
                {
                    out << " + " << str(BlockLayoutEncoder::BytesPerComponent);
                }
            }

            out << " * ";
        }
    }
    else if (visit == PostVisit)
    {
        // This is used to output the '+' in the array of arrays formula in above.
        if (node->getType().isArray() && !isEndOfSSBOAccessChain())
        {
            out << " + ";
        }
        // This corresponds to '(' in writeDotOperatorOutput when fieldType.isArrayOfArrays() is
        // true.
        if (IsInArrayOfArraysChain(node->getLeft()) && !node->getType().isArray())
        {
            out << ")";
        }
        if (mIsLoadFunctionCall && isEndOfSSBOAccessChain())
        {
            out << ")";
        }
    }
}

void ShaderStorageBlockOutputHLSL::writeDotOperatorOutput(TInfoSinkBase &out, const TField *field)
{
    auto fieldInfoIter = mBlockMemberInfoMap.find(field);
    ASSERT(fieldInfoIter != mBlockMemberInfoMap.end());
    const BlockMemberInfo &memberInfo = fieldInfoIter->second;
    mMatrixStride                     = memberInfo.matrixStride;
    mRowMajor                         = memberInfo.isRowMajorMatrix;
    out << memberInfo.offset;

    const TType &fieldType = *field->type();
    if (fieldType.isArray() && !isEndOfSSBOAccessChain())
    {
        out << " + ";
        out << memberInfo.arrayStride;
        if (fieldType.isArrayOfArrays())
        {
            out << " * (";
        }
    }
    if (mIsLoadFunctionCall && isEndOfSSBOAccessChain())
    {
        out << ")";
    }
}

}  // namespace sh

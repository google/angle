//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RewriteStructSamplers: Extract structs from samplers.
//

#include "compiler/translator/tree_ops/RewriteStructSamplers.h"

#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

namespace sh
{
namespace
{
class Traverser final : public TIntermTraverser
{
  public:
    explicit Traverser(TSymbolTable *symbolTable)
        : TIntermTraverser(true, false, false, symbolTable), mRemovedUniformsCount(0)
    {
    }

    int removedUniformsCount() const { return mRemovedUniformsCount; }

    bool visitDeclaration(Visit visit, TIntermDeclaration *decl) override
    {
        ASSERT(visit == PreVisit);

        if (!mInGlobalScope)
        {
            return true;
        }

        const TIntermSequence &sequence = *(decl->getSequence());
        TIntermTyped *declarator        = sequence.front()->getAsTyped();
        const TType &type               = declarator->getType();

        if (type.isStructureContainingSamplers())
        {
            TIntermSequence *newSequence = new TIntermSequence;

            if (type.isStructSpecifier())
            {
                stripStructSpecifierSamplers(type.getStruct(), newSequence);
            }
            else
            {
                TIntermSymbol *asSymbol = declarator->getAsSymbolNode();
                ASSERT(asSymbol);
                const TVariable &variable = asSymbol->variable();
                ASSERT(variable.symbolType() != SymbolType::Empty);
                extractStructSamplerUniforms(decl, variable, type.getStruct(), newSequence);
            }

            mMultiReplacements.emplace_back(getParentNode()->getAsBlock(), decl, *newSequence);
        }

        return true;
    }

    bool visitBinary(Visit visit, TIntermBinary *node) override
    {
        if (node->getOp() == EOpIndexDirectStruct && node->getType().isSampler())
        {
            std::string stringBuilder;

            TIntermTyped *currentNode = node;
            while (currentNode->getAsBinaryNode())
            {
                TIntermBinary *asBinary = currentNode->getAsBinaryNode();

                switch (asBinary->getOp())
                {
                    case EOpIndexDirect:
                    {
                        const int index = asBinary->getRight()->getAsConstantUnion()->getIConst(0);
                        const std::string strInt = Str(index);
                        stringBuilder.insert(0, strInt);
                        stringBuilder.insert(0, "_");
                        break;
                    }
                    case EOpIndexDirectStruct:
                    {
                        stringBuilder.insert(0, asBinary->getIndexStructFieldName().data());
                        stringBuilder.insert(0, "_");
                        break;
                    }

                    default:
                        UNREACHABLE();
                        break;
                }

                currentNode = asBinary->getLeft();
            }

            const ImmutableString &variableName = currentNode->getAsSymbolNode()->variable().name();
            stringBuilder.insert(0, variableName.data());

            ImmutableString newName(stringBuilder);

            TVariable *samplerReplacement = mExtractedSamplers[newName];
            ASSERT(samplerReplacement);

            TIntermSymbol *replacement = new TIntermSymbol(samplerReplacement);

            queueReplacement(replacement, OriginalNode::IS_DROPPED);
            return true;
        }

        return true;
    }

  private:
    void stripStructSpecifierSamplers(const TStructure *structure, TIntermSequence *newSequence)
    {
        TFieldList *newFieldList = new TFieldList;
        ASSERT(structure->containsSamplers());

        for (const TField *field : structure->fields())
        {
            const TType &fieldType = *field->type();
            if (!fieldType.isSampler() && !isRemovedStructType(fieldType))
            {
                TType *newType = new TType(fieldType);
                TField *newField =
                    new TField(newType, field->name(), field->line(), field->symbolType());
                newFieldList->push_back(newField);
            }
        }

        // Prune empty structs.
        if (newFieldList->empty())
        {
            mRemovedStructs.insert(structure->name());
            return;
        }

        TStructure *newStruct =
            new TStructure(mSymbolTable, structure->name(), newFieldList, structure->symbolType());
        TType *newStructType = new TType(newStruct, true);
        TVariable *newStructVar =
            new TVariable(mSymbolTable, kEmptyImmutableString, newStructType, SymbolType::Empty);
        TIntermSymbol *newStructRef = new TIntermSymbol(newStructVar);

        TIntermDeclaration *structDecl = new TIntermDeclaration;
        structDecl->appendDeclarator(newStructRef);

        newSequence->push_back(structDecl);
    }

    bool isRemovedStructType(const TType &type) const
    {
        const TStructure *structure = type.getStruct();
        return (structure && (mRemovedStructs.count(structure->name()) > 0));
    }

    void extractStructSamplerUniforms(TIntermDeclaration *oldDeclaration,
                                      const TVariable &variable,
                                      const TStructure *structure,
                                      TIntermSequence *newSequence)
    {
        ASSERT(structure->containsSamplers());

        size_t nonSamplerCount = 0;

        for (const TField *field : structure->fields())
        {
            nonSamplerCount +=
                extractFieldSamplers(variable.name(), field, variable.getType(), newSequence);
        }

        if (nonSamplerCount > 0)
        {
            // Keep the old declaration around if it has other members.
            newSequence->push_back(oldDeclaration);
        }
        else
        {
            mRemovedUniformsCount++;
        }
    }

    size_t extractFieldSamplers(const ImmutableString &prefix,
                                const TField *field,
                                const TType &containingType,
                                TIntermSequence *newSequence)
    {
        if (containingType.isArray())
        {
            size_t nonSamplerCount = 0;

            // Name the samplers internally as varName_<index>_fieldName
            const TVector<unsigned int> &arraySizes = *containingType.getArraySizes();
            for (unsigned int arrayElement = 0; arrayElement < arraySizes[0]; ++arrayElement)
            {
                ImmutableStringBuilder stringBuilder(prefix.length() + 10);
                stringBuilder << prefix << "_";
                stringBuilder.appendHex(arrayElement);
                nonSamplerCount = extractFieldSamplersImpl(stringBuilder, field, newSequence);
            }

            return nonSamplerCount;
        }

        return extractFieldSamplersImpl(prefix, field, newSequence);
    }

    size_t extractFieldSamplersImpl(const ImmutableString &prefix,
                                    const TField *field,
                                    TIntermSequence *newSequence)
    {
        size_t nonSamplerCount = 0;

        const TType &fieldType = *field->type();
        if (fieldType.isSampler() || fieldType.isStructureContainingSamplers())
        {
            ImmutableStringBuilder stringBuilder(prefix.length() + field->name().length() + 1);
            stringBuilder << prefix << "_" << field->name();
            ImmutableString newPrefix(stringBuilder);

            if (fieldType.isSampler())
            {
                extractSampler(newPrefix, fieldType, newSequence);
            }
            else
            {
                const TStructure *structure = fieldType.getStruct();
                for (const TField *nestedField : structure->fields())
                {
                    nonSamplerCount +=
                        extractFieldSamplers(newPrefix, nestedField, fieldType, newSequence);
                }
            }
        }
        else
        {
            nonSamplerCount++;
        }

        return nonSamplerCount;
    }

    void extractSampler(const ImmutableString &newName,
                        const TType &fieldType,
                        TIntermSequence *newSequence)
    {
        TType *newType = new TType(fieldType);
        newType->setQualifier(EvqUniform);
        TVariable *newVariable =
            new TVariable(mSymbolTable, newName, newType, SymbolType::AngleInternal);
        TIntermSymbol *newRef = new TIntermSymbol(newVariable);

        TIntermDeclaration *samplerDecl = new TIntermDeclaration;
        samplerDecl->appendDeclarator(newRef);

        newSequence->push_back(samplerDecl);

        mExtractedSamplers[newName] = newVariable;
    }

    int mRemovedUniformsCount;
    std::map<ImmutableString, TVariable *> mExtractedSamplers;
    std::set<ImmutableString> mRemovedStructs;
};
}  // anonymous namespace

int RewriteStructSamplers(TIntermBlock *root, TSymbolTable *symbolTable)
{
    Traverser rewriteStructSamplers(symbolTable);
    root->traverse(&rewriteStructSamplers);
    rewriteStructSamplers.updateTree();

    return rewriteStructSamplers.removedUniformsCount();
}
}  // namespace sh

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// UseInterfaceBlockFields.cpp: insert statements to reference all members in InterfaceBlock list at
// the beginning of main. This is to work around a Mac driver that treats unused standard/shared
// uniform blocks as inactive.

#include "compiler/translator/UseInterfaceBlockFields.h"

#include "compiler/translator/FindMain.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/IntermNode_util.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

void AddFieldUseStatements(const ShaderVariable &var,
                           TIntermSequence *sequence,
                           const TSymbolTable &symbolTable)
{
    TString name = TString(var.name.c_str());
    if (var.isArray())
    {
        size_t pos = name.find_last_of('[');
        if (pos != TString::npos)
        {
            name = name.substr(0, pos);
        }
    }
    const TType *type;
    TType basicType;
    if (var.isStruct())
    {
        TVariable *structInfo = reinterpret_cast<TVariable *>(symbolTable.findGlobal(name));
        ASSERT(structInfo);
        const TType &structType = structInfo->getType();
        type                    = &structType;
    }
    else
    {
        basicType = sh::GetShaderVariableBasicType(var);
        type      = &basicType;
    }
    ASSERT(type);

    TIntermSymbol *symbol = new TIntermSymbol(0, name, *type);
    if (var.isArray())
    {
        for (unsigned int i = 0; i < var.arraySize; ++i)
        {
            TIntermBinary *element = new TIntermBinary(EOpIndexDirect, symbol, CreateIndexNode(i));
            sequence->insert(sequence->begin(), element);
        }
    }
    else
    {
        sequence->insert(sequence->begin(), symbol);
    }
}

void InsertUseCode(TIntermSequence *sequence,
                   const InterfaceBlockList &blocks,
                   const TSymbolTable &symbolTable)
{
    for (const auto &block : blocks)
    {
        if (block.instanceName.empty())
        {
            for (const auto &var : block.fields)
            {
                AddFieldUseStatements(var, sequence, symbolTable);
            }
        }
        else if (block.arraySize > 0)
        {
            TString name      = TString(block.instanceName.c_str());
            TVariable *ubInfo = reinterpret_cast<TVariable *>(symbolTable.findGlobal(name));
            ASSERT(ubInfo);
            TIntermSymbol *arraySymbol = new TIntermSymbol(0, name, ubInfo->getType());
            for (unsigned int i = 0; i < block.arraySize; ++i)
            {
                TIntermBinary *instanceSymbol =
                    new TIntermBinary(EOpIndexDirect, arraySymbol, CreateIndexNode(i));
                for (unsigned int j = 0; j < block.fields.size(); ++j)
                {
                    TIntermBinary *element = new TIntermBinary(EOpIndexDirectInterfaceBlock,
                                                               instanceSymbol, CreateIndexNode(j));
                    sequence->insert(sequence->begin(), element);
                }
            }
        }
        else
        {
            TString name      = TString(block.instanceName.c_str());
            TVariable *ubInfo = reinterpret_cast<TVariable *>(symbolTable.findGlobal(name));
            ASSERT(ubInfo);
            TIntermSymbol *blockSymbol = new TIntermSymbol(0, name, ubInfo->getType());
            for (unsigned int i = 0; i < block.fields.size(); ++i)
            {
                TIntermBinary *element = new TIntermBinary(EOpIndexDirectInterfaceBlock,
                                                           blockSymbol, CreateIndexNode(i));

                sequence->insert(sequence->begin(), element);
            }
        }
    }
}

}  // namespace anonymous

void UseInterfaceBlockFields(TIntermBlock *root,
                             const InterfaceBlockList &blocks,
                             const TSymbolTable &symbolTable)
{
    TIntermFunctionDefinition *main = FindMain(root);
    TIntermBlock *mainBody          = main->getBody();
    ASSERT(mainBody);
    InsertUseCode(mainBody->getSequence(), blocks, symbolTable);
}

}  // namespace sh

//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/InitializeVariables.h"

#include "angle_gl.h"
#include "common/debug.h"
#include "compiler/translator/FindMain.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{

void InsertInitCode(TIntermSequence *sequence,
                    const InitVariableList &variables,
                    const TSymbolTable &symbolTable)
{
    for (const auto &var : variables)
    {
        TString name = TString(var.name.c_str());

        if (var.isArray())
        {
            // Assign the array elements one by one to keep the AST compatible with ESSL 1.00 which
            // doesn't have array assignment.
            size_t pos = name.find_last_of('[');
            if (pos != TString::npos)
            {
                name = name.substr(0, pos);
            }
            TType elementType = sh::GetShaderVariableBasicType(var);
            TType arrayType   = elementType;
            arrayType.setArraySize(var.elementCount());

            // Workaround for http://crbug.com/709317
            //   This loop is reversed to initialize elements in increasing
            // order [0 1 2 ...]. Otherwise, they're initialized in
            // decreasing order [... 2 1 0], due to
            // `sequence->insert(sequence->begin(), ...)` below.
            for (unsigned int i = var.arraySize; i > 0; --i)
            {
                unsigned int index = i - 1;
                TIntermSymbol *arraySymbol = new TIntermSymbol(0, name, arrayType);
                TIntermBinary *element     = new TIntermBinary(EOpIndexDirect, arraySymbol,
                                                           TIntermTyped::CreateIndexNode(index));

                TIntermTyped *zero        = TIntermTyped::CreateZero(elementType);
                TIntermBinary *assignment = new TIntermBinary(EOpAssign, element, zero);

                sequence->insert(sequence->begin(), assignment);
            }
        }
        else if (var.isStruct())
        {
            TVariable *structInfo = reinterpret_cast<TVariable *>(symbolTable.findGlobal(name));
            ASSERT(structInfo);

            TIntermSymbol *symbol = new TIntermSymbol(0, name, structInfo->getType());
            TIntermTyped *zero    = TIntermTyped::CreateZero(structInfo->getType());

            TIntermBinary *assign = new TIntermBinary(EOpAssign, symbol, zero);
            sequence->insert(sequence->begin(), assign);
        }
        else
        {
            TType type            = sh::GetShaderVariableBasicType(var);
            TIntermSymbol *symbol = new TIntermSymbol(0, name, type);
            TIntermTyped *zero    = TIntermTyped::CreateZero(type);

            TIntermBinary *assign = new TIntermBinary(EOpAssign, symbol, zero);
            sequence->insert(sequence->begin(), assign);
        }
    }
}

}  // namespace anonymous

void InitializeVariables(TIntermBlock *root,
                         const InitVariableList &vars,
                         const TSymbolTable &symbolTable)
{
    TIntermFunctionDefinition *main = FindMain(root);
    ASSERT(main != nullptr);
    TIntermBlock *body = main->getBody();
    InsertInitCode(body->getSequence(), vars, symbolTable);
}

}  // namespace sh

//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// IntermNode_util.h: High-level utilities for creating AST nodes and node hierarchies. Mostly meant
// to be used in AST transforms.

#ifndef COMPILER_TRANSLATOR_INTERMNODEUTIL_H_
#define COMPILER_TRANSLATOR_INTERMNODEUTIL_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{

TIntermFunctionPrototype *CreateInternalFunctionPrototypeNode(const TType &returnType,
                                                              const char *name,
                                                              const TSymbolUniqueId &functionId);
TIntermFunctionDefinition *CreateInternalFunctionDefinitionNode(const TType &returnType,
                                                                const char *name,
                                                                TIntermBlock *functionBody,
                                                                const TSymbolUniqueId &functionId);
TIntermAggregate *CreateInternalFunctionCallNode(const TType &returnType,
                                                 const char *name,
                                                 const TSymbolUniqueId &functionId,
                                                 TIntermSequence *arguments);

TIntermTyped *CreateZeroNode(const TType &type);
TIntermConstantUnion *CreateIndexNode(int index);
TIntermConstantUnion *CreateBoolNode(bool value);

// If the input node is nullptr, return nullptr.
// If the input node is a block node, return it.
// If the input node is not a block node, put it inside a block node and return that.
TIntermBlock *EnsureBlock(TIntermNode *node);

TIntermSymbol *ReferenceGlobalVariable(const TString &name, const TSymbolTable &symbolTable);
TIntermSymbol *ReferenceBuiltInVariable(const TString &name,
                                        const TSymbolTable &symbolTable,
                                        int shaderVersion);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_INTERMNODEUTIL_H_
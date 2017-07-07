//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Build the intermediate representation.
//

#include <float.h>
#include <limits.h>
#include <algorithm>

#include "compiler/translator/Intermediate.h"
#include "compiler/translator/SymbolTable.h"

namespace sh
{

////////////////////////////////////////////////////////////////////////////
//
// First set of functions are to help build the intermediate representation.
// These functions are not member functions of the nodes.
// They are called from parser productions.
//
/////////////////////////////////////////////////////////////////////////////

// If the input node is nullptr, return nullptr.
// If the input node is a block node, return it.
// If the input node is not a block node, put it inside a block node and return that.
TIntermBlock *TIntermediate::EnsureBlock(TIntermNode *node)
{
    if (node == nullptr)
        return nullptr;
    TIntermBlock *blockNode = node->getAsBlock();
    if (blockNode != nullptr)
        return blockNode;

    blockNode = new TIntermBlock();
    blockNode->setLine(node->getLine());
    blockNode->appendStatement(node);
    return blockNode;
}

//
// Constant terminal nodes.  Has a union that contains bool, float or int constants
//
// Returns the constant union node created.
//

TIntermConstantUnion *TIntermediate::addConstantUnion(const TConstantUnion *constantUnion,
                                                      const TType &type,
                                                      const TSourceLoc &line)
{
    TIntermConstantUnion *node = new TIntermConstantUnion(constantUnion, type);
    node->setLine(line);

    return node;
}

}  // namespace sh

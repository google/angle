//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#pragma warning(disable: 4718)

#include "compiler/depgraph/DependencyGraph.h"
#include "compiler/depgraph/DependencyGraphBuilder.h"

TDependencyGraph::TDependencyGraph(TIntermNode* intermNode)
{
    TDependencyGraphBuilder::build(intermNode, this);
}

TDependencyGraph::~TDependencyGraph()
{
    for (TGraphNodeVector::const_iterator iter = mAllNodes.begin(); iter != mAllNodes.end(); ++iter)
    {
        TGraphNode* node = *iter;
        delete node;
    }
}

TGraphSymbol* TDependencyGraph::getGlobalSymbolByName(const TString& name) const
{
    TSymbolNameMap::const_iterator iter = mGlobalSymbolMap.find(name);
    if (iter == mGlobalSymbolMap.end())
        return NULL;

    TSymbolNamePair pair = *iter;
    TGraphSymbol* symbol = pair.second;
    return symbol;
}

TGraphArgument* TDependencyGraph::createArgument(TIntermAggregate* intermFunctionCall,
                                                 int argumentNumber)
{
    TGraphArgument* argument = new TGraphArgument(intermFunctionCall, argumentNumber);
    mAllNodes.push_back(argument);
    return argument;
}

TGraphFunctionCall* TDependencyGraph::createFunctionCall(TIntermAggregate* intermFunctionCall)
{
    TGraphFunctionCall* functionCall = new TGraphFunctionCall(intermFunctionCall);
    mAllNodes.push_back(functionCall);
    if (functionCall->getIntermFunctionCall()->isUserDefined())
        mUserDefinedFunctionCalls.push_back(functionCall);
    return functionCall;
}

TGraphSymbol* TDependencyGraph::getOrCreateSymbol(TIntermSymbol* intermSymbol, bool isGlobalSymbol)
{
    TSymbolIdMap::const_iterator iter = mSymbolIdMap.find(intermSymbol->getId());

    TGraphSymbol* symbol = NULL;

    if (iter != mSymbolIdMap.end()) {
        TSymbolIdPair pair = *iter;
        symbol = pair.second;
    } else {
        symbol = new TGraphSymbol(intermSymbol);
        mAllNodes.push_back(symbol);

        TSymbolIdPair pair(intermSymbol->getId(), symbol);
        mSymbolIdMap.insert(pair);

        if (isGlobalSymbol) {
            // We map all symbols in the global scope by name, so traversers of the graph can
            // quickly start searches at global symbols with specific names.
            TSymbolNamePair pair(intermSymbol->getSymbol(), symbol);
            mGlobalSymbolMap.insert(pair);
        }
    }

    return symbol;
}

TGraphSelection* TDependencyGraph::createSelection(TIntermSelection* intermSelection)
{
    TGraphSelection* selection = new TGraphSelection(intermSelection);
    mAllNodes.push_back(selection);
    return selection;
}

TGraphLoop* TDependencyGraph::createLoop(TIntermLoop* intermLoop)
{
    TGraphLoop* loop = new TGraphLoop(intermLoop);
    mAllNodes.push_back(loop);
    return loop;
}

TGraphLogicalOp* TDependencyGraph::createLogicalOp(TIntermBinary* intermLogicalOp)
{
    TGraphLogicalOp* logicalOp = new TGraphLogicalOp(intermLogicalOp);
    mAllNodes.push_back(logicalOp);
    return logicalOp;
}

const char* TGraphLogicalOp::getOpString() const
{
    const char* opString = NULL;
    switch (getIntermLogicalOp()->getOp()) {
        case EOpLogicalAnd: opString = "and"; break;
        case EOpLogicalOr: opString = "or"; break;
        default: opString = "unknown"; break;
    }
    return opString;
}

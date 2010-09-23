//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "GLSLANG/ShaderLang.h"
#include "compiler/intermediate.h"

// Provides information about a variable.
// It is currently being used to store info about active attribs and uniforms.
struct TVariableInfo {
    TPersistString name;
    EShDataType type;
    int size;
};
typedef std::vector<TVariableInfo> TVariableInfoList;

// Traverses intermediate tree to collect all attributes and uniforms.
class CollectAttribsUniforms : public TIntermTraverser {
public:
    CollectAttribsUniforms(TVariableInfoList& attribs,
                           TVariableInfoList& uniforms);

    virtual void visitSymbol(TIntermSymbol*);
    virtual void visitConstantUnion(TIntermConstantUnion*);
    virtual bool visitBinary(Visit visit, TIntermBinary*);
    virtual bool visitUnary(Visit visit, TIntermUnary*);
    virtual bool visitSelection(Visit visit, TIntermSelection*);
    virtual bool visitAggregate(Visit visit, TIntermAggregate*);
    virtual bool visitLoop(Visit visit, TIntermLoop*);
    virtual bool visitBranch(Visit visit, TIntermBranch*);

private:
    TVariableInfoList& mAttribs;
    TVariableInfoList& mUniforms;
};


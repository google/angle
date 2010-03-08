//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Common.h"
#include "ShHandle.h"

//
// Here is where real machine specific high-level data would be defined.
//
class TGenericCompiler : public TCompiler {
public:
    TGenericCompiler(EShLanguage l, int dOptions) : TCompiler(l, infoSink), debugOptions(dOptions) { }
    virtual bool compile(TIntermNode* root);
    TInfoSink infoSink;
    int debugOptions;
};

//
// This function must be provided to create the actual
// compile object used by higher level code.  It returns
// a subclass of TCompiler.
//
TCompiler* ConstructCompiler(EShLanguage language, int debugOptions)
{
    return new TGenericCompiler(language, debugOptions);
}

//
// Delete the compiler made by ConstructCompiler
//
void DeleteCompiler(TCompiler* compiler)
{
    delete compiler;
}

//
//  Generate code from the given parse tree
//
bool TGenericCompiler::compile(TIntermNode *root)
{
    haveValidObjectCode = true;

    return haveValidObjectCode;
}

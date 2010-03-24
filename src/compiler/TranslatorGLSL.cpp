//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "TranslatorGLSL.h"
#include "OutputGLSL.h"

TranslatorGLSL::TranslatorGLSL(EShLanguage l, int dOptions)
        : TCompiler(l),
          debugOptions(dOptions) {
}

bool TranslatorGLSL::compile(TIntermNode* root) {
    TParseContext& parseContext = *GetGlobalParseContext();
    TOutputGLSL outputGLSL(parseContext);
    outputGLSL.header();
    parseContext.treeRoot->traverse(&outputGLSL);

    return true;
}

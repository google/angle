//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "TranslatorHLSL.h"
#include "OutputHLSL.h"

TranslatorHLSL::TranslatorHLSL(EShLanguage language, int debugOptions)
    : TCompiler(language), debugOptions(debugOptions)
{
}

bool TranslatorHLSL::compile(TIntermNode *root)
{
    TParseContext& parseContext = *GetGlobalParseContext();
    sh::OutputHLSL outputHLSL(parseContext);

    outputHLSL.header();
    parseContext.treeRoot->traverse(&outputHLSL);
    outputHLSL.footer();
    
    return true;
}

//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Preprocessor.h"

#include "Token.h"

namespace pp
{

Preprocessor::Preprocessor(Diagnostics* diagnostics,
                           DirectiveHandler* directiveHandler) :
    mTokenizer(diagnostics),
    mDirectiveParser(&mTokenizer, &mMacroSet, diagnostics, directiveHandler),
    mMacroExpander(&mDirectiveParser, &mMacroSet, diagnostics)
{
}

bool Preprocessor::init(int count,
                        const char* const string[],
                        const int length[])
{
    return mTokenizer.init(count, string, length);
}

void Preprocessor::lex(Token* token)
{
    mMacroExpander.lex(token);
}

}  // namespace pp


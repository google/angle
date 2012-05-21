//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "MacroExpander.h"

namespace pp
{

MacroExpander::MacroExpander(Lexer* lexer,
                             MacroSet* macroSet,
                             Diagnostics* diagnostics) :
    mLexer(lexer),
    mMacroSet(macroSet),
    mDiagnostics(diagnostics)
{
}

void MacroExpander::lex(Token* token)
{
    // TODO(alokp): Implement me.
    mLexer->lex(token);
}

}  // namespace pp


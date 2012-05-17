//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_
#define COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_

#include "Lexer.h"
#include "pp_utils.h"

namespace pp
{

class Diagnostics;

class MacroExpander : public Lexer
{
  public:
    MacroExpander(Lexer* lexer, Diagnostics* diagnostics);

    virtual void lex(Token* token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(MacroExpander);

    Lexer* mLexer;
    Diagnostics* mDiagnostics;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_MACRO_EXPANDER_H_


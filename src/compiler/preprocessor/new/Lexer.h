//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_LEXER_H_
#define COMPILER_PREPROCESSOR_LEXER_H_

#include <memory>

#include "Input.h"
#include "pp_utils.h"

namespace pp
{

struct Token;

class Lexer
{
  public:
    struct Context
    {
        std::auto_ptr<Input> input;
        // The location where yytext points to. Token location should track
        // scanLoc instead of Input::mReadLoc because they may not be the same
        // if text is buffered up in the lexer input buffer.
        Input::Location scanLoc;
    };

    Lexer();
    ~Lexer();

    bool init(int count, const char* const string[], const int length[]);
    int lex(Token* token);

  private:
    PP_DISALLOW_COPY_AND_ASSIGN(Lexer);
    bool initLexer();
    void destroyLexer();

    void* mHandle;  // Lexer handle.
    Context mContext;  // Lexer extra.
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_LEXER_H_


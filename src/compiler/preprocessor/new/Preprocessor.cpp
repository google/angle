//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Preprocessor.h"

#include <sstream>

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
    static const int kGLSLVersion = 100;

    // Add standard pre-defined macros.
    predefineMacro("__LINE__", 0);
    predefineMacro("__FILE__", 0);
    predefineMacro("__VERSION__", kGLSLVersion);
    predefineMacro("GL_ES", 1);

    return mTokenizer.init(count, string, length);
}

void Preprocessor::predefineMacro(const std::string& name, int value)
{
    std::stringstream stream;
    stream << value;

    Token token;
    token.type = Token::CONST_INT;
    token.value = stream.str();

    Macro macro;
    macro.predefined = true;
    macro.type = Macro::kTypeObj;
    macro.name = name;
    macro.replacements.push_back(token);

    mMacroSet[name] = macro;
}

void Preprocessor::lex(Token* token)
{
    mMacroExpander.lex(token);
}

}  // namespace pp


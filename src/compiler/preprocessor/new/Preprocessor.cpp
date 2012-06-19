//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Preprocessor.h"

#include <sstream>

#include "DirectiveParser.h"
#include "Macro.h"
#include "MacroExpander.h"
#include "Token.h"
#include "Tokenizer.h"

namespace pp
{

struct PreprocessorImpl
{
    MacroSet macroSet;
    Tokenizer tokenizer;
    DirectiveParser directiveParser;
    MacroExpander macroExpander;

    PreprocessorImpl(Diagnostics* diagnostics,
                     DirectiveHandler* directiveHandler) :
        tokenizer(diagnostics),
        directiveParser(&tokenizer, &macroSet, diagnostics, directiveHandler),
        macroExpander(&directiveParser, &macroSet, diagnostics)
    {
    }
};

Preprocessor::Preprocessor(Diagnostics* diagnostics,
                           DirectiveHandler* directiveHandler)
{
    mImpl = new PreprocessorImpl(diagnostics, directiveHandler);
}

Preprocessor::~Preprocessor()
{
    delete mImpl;
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

    return mImpl->tokenizer.init(count, string, length);
}

void Preprocessor::predefineMacro(const char* name, int value)
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

    mImpl->macroSet[name] = macro;
}

void Preprocessor::lex(Token* token)
{
    mImpl->macroExpander.lex(token);
}

}  // namespace pp


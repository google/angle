//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "DirectiveParser.h"

#include <cassert>
#include <sstream>

#include "Diagnostics.h"
#include "DirectiveHandler.h"
#include "ExpressionParser.h"
#include "MacroExpander.h"
#include "Token.h"
#include "Tokenizer.h"

namespace {
static const std::string kDirectiveDefine("define");
static const std::string kDirectiveUndef("undef");
static const std::string kDirectiveIf("if");
static const std::string kDirectiveIfdef("ifdef");
static const std::string kDirectiveIfndef("ifndef");
static const std::string kDirectiveElse("else");
static const std::string kDirectiveElif("elif");
static const std::string kDirectiveEndif("endif");
static const std::string kDirectiveError("error");
static const std::string kDirectivePragma("pragma");
static const std::string kDirectiveExtension("extension");
static const std::string kDirectiveVersion("version");
static const std::string kDirectiveLine("line");
}  // namespace

static bool isMacroNameReserved(const std::string& name)
{
    // Names prefixed with "GL_" are reserved.
    if (name.substr(0, 3) == "GL_")
        return true;

    // Names containing two consecutive underscores are reserved.
    if (name.find("__") != std::string::npos)
        return true;

    return false;
}

namespace pp
{

class DefinedParser : public Lexer
{
  public:
    DefinedParser(Lexer* lexer) : mLexer(lexer) { }

  protected:
    virtual void lex(Token* token)
    {
        // TODO(alokp): Implement me.
        mLexer->lex(token);
    }

  private:
    Lexer* mLexer;
};

DirectiveParser::DirectiveParser(Tokenizer* tokenizer,
                                 MacroSet* macroSet,
                                 Diagnostics* diagnostics,
                                 DirectiveHandler* directiveHandler) :
    mTokenizer(tokenizer),
    mMacroSet(macroSet),
    mDiagnostics(diagnostics),
    mDirectiveHandler(directiveHandler)
{
}

void DirectiveParser::lex(Token* token)
{
    do
    {
        mTokenizer->lex(token);
        if (token->type == '#') parseDirective(token);
    } while (token->type == '\n');
}

void DirectiveParser::parseDirective(Token* token)
{
    assert(token->type == '#');

    mTokenizer->lex(token);
    if (token->type == Token::IDENTIFIER)
    {
        if (token->value == kDirectiveDefine)
            parseDefine(token);
        else if (token->value == kDirectiveUndef)
            parseUndef(token);
        else if (token->value == kDirectiveIf)
            parseIf(token);
        else if (token->value == kDirectiveIfdef)
            parseIfdef(token);
        else if (token->value == kDirectiveIfndef)
            parseIfndef(token);
        else if (token->value == kDirectiveElse)
            parseElse(token);
        else if (token->value == kDirectiveElif)
            parseElif(token);
        else if (token->value == kDirectiveEndif)
            parseEndif(token);
        else if (token->value == kDirectiveError)
            parseError(token);
        else if (token->value == kDirectivePragma)
            parsePragma(token);
        else if (token->value == kDirectiveExtension)
            parseExtension(token);
        else if (token->value == kDirectiveVersion)
            parseVersion(token);
        else if (token->value == kDirectiveLine)
            parseLine(token);
    }

    if ((token->type != '\n') && (token->type != 0))
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                             token->location,
                             token->value);

    while (token->type != '\n')
    {
        if (token->type == 0) {
            mDiagnostics->report(Diagnostics::EOF_IN_DIRECTIVE,
                                 token->location,
                                 token->value);
            break;
        }
        mTokenizer->lex(token);
    }
}

void DirectiveParser::parseDefine(Token* token)
{
    assert(token->value == kDirectiveDefine);

    mTokenizer->lex(token);
    if (token->type != Token::IDENTIFIER)
    {
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                             token->location,
                             token->value);
        return;
    }
    if (isMacroNameReserved(token->value))
    {
        mDiagnostics->report(Diagnostics::MACRO_NAME_RESERVED,
                             token->location,
                             token->value);
        return;
    }

    Macro macro;
    macro.type = Macro::kTypeObj;
    macro.name = token->value;

    mTokenizer->lex(token);
    if (token->type == '(' && !token->hasLeadingSpace())
    {
        // Function-like macro. Collect arguments.
        macro.type = Macro::kTypeFunc;
        do {
            mTokenizer->lex(token);
            if (token->type != Token::IDENTIFIER)
                break;
            macro.parameters.push_back(token->value);

            mTokenizer->lex(token);  // Get comma.
        } while (token->type == ',');

        if (token->type != ')')
        {
            mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                                 token->location,
                                 token->value);
            return;
        }
    }

    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        // Reset the token location because it is unnecessary in replacement
        // list. Resetting it also allows us to reuse Token::equals() to
        // compare macros.
        token->location = SourceLocation();
        macro.replacements.push_back(*token);
        mTokenizer->lex(token);
    }

    // Check for macro redefinition.
    MacroSet::const_iterator iter = mMacroSet->find(macro.name);
    if (iter != mMacroSet->end() && !macro.equals(iter->second))
    {
        mDiagnostics->report(Diagnostics::MACRO_REDEFINED,
                             token->location,
                             macro.name);
        return;
    }
    mMacroSet->insert(std::make_pair(macro.name, macro));
}

void DirectiveParser::parseUndef(Token* token)
{
    assert(token->value == kDirectiveUndef);

    mTokenizer->lex(token);
    if (token->type != Token::IDENTIFIER)
    {
        mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                             token->location,
                             token->value);
        return;
    }

    MacroSet::const_iterator iter = mMacroSet->find(token->value);
    if (iter != mMacroSet->end())
        mMacroSet->erase(iter);

    mTokenizer->lex(token);
}

void DirectiveParser::parseIf(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveIf);

    DefinedParser definedParser(mTokenizer);
    MacroExpander macroExpander(&definedParser, mMacroSet, mDiagnostics);
    ExpressionParser expressionParser(&macroExpander, mDiagnostics);
    macroExpander.lex(token);

    int expression = 0;
    if (!expressionParser.parse(token, &expression))
    {
        // TODO(alokp): Report diagnostic.
        return;
    }

    // We have a valid #if directive. Handle it.
    // TODO(alokp): Push conditional block.
}

void DirectiveParser::parseIfdef(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveIfdef);
    mTokenizer->lex(token);
}

void DirectiveParser::parseIfndef(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveIfndef);
    mTokenizer->lex(token);
}

void DirectiveParser::parseElse(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveElse);
    mTokenizer->lex(token);
}

void DirectiveParser::parseElif(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveElif);
    mTokenizer->lex(token);
}

void DirectiveParser::parseEndif(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveEndif);
    mTokenizer->lex(token);
}

void DirectiveParser::parseError(Token* token)
{
    assert(token->value == kDirectiveError);

    std::stringstream stream;
    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        stream << *token;
        mTokenizer->lex(token);
    }
    mDirectiveHandler->handleError(token->location, stream.str());
}

// Parses pragma of form: #pragma name[(value)].
void DirectiveParser::parsePragma(Token* token)
{
    assert(token->value == kDirectivePragma);

    enum State
    {
        PRAGMA_NAME,
        LEFT_PAREN,
        PRAGMA_VALUE,
        RIGHT_PAREN
    };

    bool valid = true;
    std::string name, value;
    int state = PRAGMA_NAME;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch(state++)
        {
          case PRAGMA_NAME:
            name = token->value;
            valid = valid && (token->type == Token::IDENTIFIER);
            break;
          case LEFT_PAREN:
            valid = valid && (token->type == '(');
            break;
          case PRAGMA_VALUE:
            value = token->value;
            valid = valid && (token->type == Token::IDENTIFIER);
            break;
          case RIGHT_PAREN:
            valid = valid && (token->type == ')');
            break;
          default:
            valid = false;
            break;
        }
        mTokenizer->lex(token);
    }

    valid = valid && ((state == PRAGMA_NAME) ||     // Empty pragma.
                      (state == LEFT_PAREN) ||      // Without value.
                      (state == RIGHT_PAREN + 1));  // With value.
    if (!valid)
    {
        mDiagnostics->report(Diagnostics::UNRECOGNIZED_PRAGMA,
                             token->location, name);
    }
    else if (state > PRAGMA_NAME)  // Do not notify for empty pragma.
    {
        mDirectiveHandler->handlePragma(token->location, name, value);
    }
}

void DirectiveParser::parseExtension(Token* token)
{
    assert(token->value == kDirectiveExtension);

    enum State
    {
        EXT_NAME,
        COLON,
        EXT_BEHAVIOR
    };

    bool valid = true;
    std::string name, behavior;
    int state = EXT_NAME;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch (state++)
        {
          case EXT_NAME:
            if (valid && (token->type != Token::IDENTIFIER))
            {
                mDiagnostics->report(Diagnostics::INVALID_EXTENSION_NAME,
                                     token->location, token->value);
                valid = false;
            }
            if (valid) name = token->value;
            break;
          case COLON:
            if (valid && (token->type != ':'))
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                                     token->location, token->value);
                valid = false;
            }
            break;
          case EXT_BEHAVIOR:
            if (valid && (token->type != Token::IDENTIFIER))
            {
                mDiagnostics->report(Diagnostics::INVALID_EXTENSION_BEHAVIOR,
                                     token->location, token->value);
                valid = false;
            }
            if (valid) behavior = token->value;
            break;
          default:
            if (valid)
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                                     token->location, token->value);
                valid = false;
            }
            break;
        }
        mTokenizer->lex(token);
    }
    if (valid && (state != EXT_BEHAVIOR + 1))
    {
        mDiagnostics->report(Diagnostics::INVALID_EXTENSION_DIRECTIVE,
                             token->location, token->value);
        valid = false;
    }
    if (valid)
        mDirectiveHandler->handleExtension(token->location, name, behavior);
}

void DirectiveParser::parseVersion(Token* token)
{
    assert(token->value == kDirectiveVersion);

    enum State
    {
        VERSION_NUMBER
    };

    bool valid = true;
    int version = 0;
    int state = VERSION_NUMBER;

    mTokenizer->lex(token);
    while ((token->type != '\n') && (token->type != Token::LAST))
    {
        switch (state++)
        {
          case VERSION_NUMBER:
            if (valid && (token->type != Token::CONST_INT))
            {
                mDiagnostics->report(Diagnostics::INVALID_VERSION_NUMBER,
                                     token->location, token->value);
                valid = false;
            }
            if (valid) version = atoi(token->value.c_str());
            break;
          default:
            if (valid)
            {
                mDiagnostics->report(Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE,
                                     token->location, token->value);
                valid = false;
            }
            break;
        }
        mTokenizer->lex(token);
    }
    if (valid && (state != VERSION_NUMBER + 1))
    {
        mDiagnostics->report(Diagnostics::INVALID_VERSION_DIRECTIVE,
                             token->location, token->value);
        valid = false;
    }
    if (valid)
        mDirectiveHandler->handleVersion(token->location, version);
}

void DirectiveParser::parseLine(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveLine);
    MacroExpander macroExpander(mTokenizer, mMacroSet, mDiagnostics);
    macroExpander.lex(token);
}

}  // namespace pp

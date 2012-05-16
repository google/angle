//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "DirectiveParser.h"

#include <cassert>

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
    if (token->type == pp::Token::IDENTIFIER)
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
        else
            token->type = pp::Token::INVALID_DIRECTIVE;
    }

    while (token->type != '\n')
    {
        if (token->type == 0) {
            //token->type = pp::Token::EOF_IN_DIRECTIVE;
            break;
        }
        //token->type = pp::Token::INVALID_DIRECTIVE;
        mTokenizer->lex(token);
    }
}

void DirectiveParser::parseDefine(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveDefine);
    mTokenizer->lex(token);
}

void DirectiveParser::parseUndef(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveUndef);
    mTokenizer->lex(token);
}

void DirectiveParser::parseIf(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveIf);

    DefinedParser definedParser(mTokenizer);
    MacroExpander macroExpander(&definedParser);
    ExpressionParser expressionParser(&macroExpander);
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
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveError);
    mTokenizer->lex(token);
}

void DirectiveParser::parsePragma(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectivePragma);
    mTokenizer->lex(token);
}

void DirectiveParser::parseExtension(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveExtension);
    mTokenizer->lex(token);
}

void DirectiveParser::parseVersion(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveVersion);
    mTokenizer->lex(token);
}

void DirectiveParser::parseLine(Token* token)
{
    // TODO(alokp): Implement me.
    assert(token->value == kDirectiveLine);
    MacroExpander macroExpander(mTokenizer);
    macroExpander.lex(token);
}

}  // namespace pp

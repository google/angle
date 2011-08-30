//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Context.h"

#include <algorithm>
#include <sstream>

#include "compiler/debug.h"
#include "stl_utils.h"
#include "token_type.h"

namespace pp
{

Context::Context()
    : mLexer(NULL),
      mInput(NULL),
      mOutput(NULL)
{
}

Context::~Context()
{
    destroyLexer();
}

bool Context::init()
{
    return initLexer();

    // TODO(alokp): Define built-in macros here so that we do not need to
    // define them everytime in process().
}

bool Context::process(int count,
                      const char* const string[],
                      const int length[],
                      TokenVector* output)
{
    ASSERT((count >=0) && (string != NULL) && (output != NULL));

    // Setup.
    mInput = new Input(count, string, length);
    mOutput = output;
    defineBuiltInMacro("GL_ES", 1);

    // Parse.
    bool success = parse();

    // Cleanup.
    reset();
    return success;
}

bool Context::defineMacro(pp::Token::Location location,
                          pp::Macro::Type type,
                          std::string* identifier,
                          pp::Macro::ParameterVector* parameters,
                          pp::TokenVector* replacements)
{
    // TODO(alokp): Check for reserved macro names and duplicate macros.
    mMacros[*identifier] = new Macro(type, identifier, parameters, replacements);
    return true;
}

bool Context::undefineMacro(const std::string* identifier)
{
    MacroSet::iterator iter = mMacros.find(*identifier);
    if (iter == mMacros.end())
    {
        // TODO(alokp): Report error.
        return false;
    }
    mMacros.erase(iter);
    return true;
}

bool Context::isMacroDefined(const std::string* identifier)
{
    return mMacros.find(*identifier) != mMacros.end();
}

// Reset to initialized state.
void Context::reset()
{
    std::for_each(mMacros.begin(), mMacros.end(), DeleteSecond());
    mMacros.clear();

    delete mInput;
    mInput = NULL;

    mOutput = NULL;
}

void Context::defineBuiltInMacro(const std::string& identifier, int value)
{
    std::ostringstream stream;
    stream << value;
    Token* token = new Token(0, INT_CONSTANT, new std::string(stream.str()));
    TokenVector* replacements = new pp::TokenVector(1, token);

    mMacros[identifier] = new Macro(Macro::kTypeObj,
                                    new std::string(identifier),
                                    NULL,
                                    replacements);
}

}  // namespace pp


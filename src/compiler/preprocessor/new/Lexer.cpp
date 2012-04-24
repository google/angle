//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Lexer.h"

#include <cassert>

namespace pp
{

Lexer::Lexer() : mHandle(0)
{
}

Lexer::~Lexer()
{
    destroyLexer();

    // Make sure the lexer and associated buffer are deleted.
    assert(mHandle == 0);
    assert(mInput.buffer == 0);
}

bool Lexer::init(int count, const char* const string[], const int length[])
{
    assert((count >= 0) && string);

    mInput.count = count;
    mInput.string = string;
    mInput.length = length;

    return initLexer();
}

}  // namespace pp


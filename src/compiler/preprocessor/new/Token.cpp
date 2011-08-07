//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Token.h"

static const int kLocationLineSize = 16;  // in bits.
static const int kLocationLineMask = (1 << kLocationLineSize) - 1;

namespace pp
{

Token::Token() : mLocation(-1), mType(-1), mValue(0)
{
}

Token::Token(Location l, int t) : mLocation(l), mType(t)
{
}

Token::Token(Location l, int t, const std::string& s) : mLocation(l), mType(t), mValue(s)
{
}

Token::Location Token::encodeLocation(int line, int file)
{
    return (file << kLocationLineSize) | (line & kLocationLineMask);
}

void Token::decodeLocation(Location loc, int* line, int* file)
{
    if (file) *file = loc >> kLocationLineSize;
    if (line) *line = loc & kLocationLineMask;
}

}  // namespace pp


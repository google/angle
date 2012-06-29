//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "Token.h"

#include <cassert>
#include <sstream>

template<typename IntType>
static bool atoi_t(const std::string& str, IntType* value)
{
    std::ios::fmtflags base = std::ios::dec;
    if ((str.size() >= 2) && (str[0] == '0') && (tolower(str[1]) == 'x'))
    {
        base = std::ios::hex;
    }
    else if ((str.size() >= 1) && (str[0] == '0'))
    {
        base = std::ios::oct;
    }

    std::istringstream stream(str);
    stream.setf(base, std::ios::basefield);
    stream >> (*value);
    return !stream.fail();
}

namespace pp
{

void Token::reset()
{
    type = 0;
    flags = 0;
    location = SourceLocation();
    text.clear();
}

bool Token::equals(const Token& other) const
{
    return (type == other.type) &&
           (flags == other.flags) &&
           (location == other.location) &&
           (text == other.text);
}

void Token::setAtStartOfLine(bool start)
{
    if (start)
        flags |= AT_START_OF_LINE;
    else
        flags &= ~AT_START_OF_LINE;
}

void Token::setHasLeadingSpace(bool space)
{
    if (space)
        flags |= HAS_LEADING_SPACE;
    else
        flags &= ~HAS_LEADING_SPACE;
}

void Token::setExpansionDisabled(bool disable)
{
    if (disable)
        flags |= EXPANSION_DISABLED;
    else
        flags &= ~EXPANSION_DISABLED;
}

bool Token::iValue(int* value) const
{
    assert(type == CONST_INT);
    return atoi_t(text, value);
}

bool Token::uValue(unsigned int* value) const
{
    assert(type == CONST_INT);
    return atoi_t(text, value);
}

bool Token::fValue(float* value) const
{
    assert(type == CONST_FLOAT);

    std::istringstream stream(text);
    stream.imbue(std::locale("C"));
    stream >> (*value);
    return !stream.fail();
}

std::ostream& operator<<(std::ostream& out, const Token& token)
{
    if (token.hasLeadingSpace())
        out << " ";

    out << token.text;
    return out;
}

}  // namespace pp

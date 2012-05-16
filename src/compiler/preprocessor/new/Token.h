//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_TOKEN_H_
#define COMPILER_PREPROCESSOR_TOKEN_H_

#include <ostream>
#include <string>

namespace pp
{

struct Token
{
    enum Type
    {
        // Token IDs for error conditions are negative.
        INTERNAL_ERROR = -1,
        OUT_OF_MEMORY = -2,
        INVALID_CHARACTER = -3,
        INVALID_NUMBER = -4,
        INVALID_DIRECTIVE = -5,
        INVALID_EXPRESSION = -6,
        DIVISION_BY_ZERO = -7,
        EOF_IN_COMMENT = -8,
        EOF_IN_DIRECTIVE = -9,
        UNEXPECTED_TOKEN_IN_DIRECTIVE = -10,

        // Indicates EOF.
        LAST = 0,

        IDENTIFIER = 258,

        CONST_INT,
        CONST_FLOAT,

        OP_INC,
        OP_DEC,
        OP_LEFT,
        OP_RIGHT,
        OP_LE,
        OP_GE,
        OP_EQ,
        OP_NE,
        OP_AND,
        OP_XOR,
        OP_OR,
        OP_ADD_ASSIGN,
        OP_SUB_ASSIGN,
        OP_MUL_ASSIGN,
        OP_DIV_ASSIGN,
        OP_MOD_ASSIGN,
        OP_LEFT_ASSIGN,
        OP_RIGHT_ASSIGN,
        OP_AND_ASSIGN,
        OP_XOR_ASSIGN,
        OP_OR_ASSIGN
    };
    enum Flags
    {
        HAS_LEADING_SPACE = 1 << 0
    };
    struct Location
    {
        Location() : file(0), line(0) { }
        bool equals(const Location& other) const
        {
            return (file == other.file) && (line == other.line);
        }

        int file;
        int line;
    };

    Token() : type(0), flags(0) { }

    void reset()
    {
        type = 0;
        flags = 0;
        location = Location();
        value.clear();
    }

    bool equals(const Token& other) const
    {
        return (type == other.type) &&
               (flags == other.flags) &&
               (location.equals(other.location)) &&
               (value == other.value);
    }

    bool hasLeadingSpace() const { return (flags & HAS_LEADING_SPACE) != 0; }
    void setHasLeadingSpace(bool space)
    {
        if (space)
            flags |= HAS_LEADING_SPACE;
        else
            flags &= ~HAS_LEADING_SPACE;
    }

    int type;
    int flags;
    Location location;
    std::string value;
};

extern std::ostream& operator<<(std::ostream& out, const Token& token);

}  // namepsace pp
#endif  // COMPILER_PREPROCESSOR_TOKEN_H_

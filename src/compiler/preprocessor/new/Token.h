//
// Copyright (c) 2011 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_TOKEN_H_
#define COMPILER_PREPROCESSOR_TOKEN_H_

#include <string>

namespace pp
{

struct Token
{
    enum Type
    {
        IDENTIFIER = 258,

        CONST_INT,
        CONST_FLOAT,

        OP_LEFT_SHIFT,
        OP_RIGHT_SHIFT,
        OP_LESS_EQUAL,
        OP_GREATER_EQUAL,
        OP_EQUAL,
        OP_NOT_EQUAL,
        OP_AND_AND,
        OP_OR_OR
    };
    struct Location
    {
        int line;
        int string;
    };

    int type;
    Location location;
    std::string value;
};

extern std::ostream& operator<<(std::ostream& out, const Token& token);

}  // namepsace pp
#endif  // COMPILER_PREPROCESSOR_TOKEN_H_

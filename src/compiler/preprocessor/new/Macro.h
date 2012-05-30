//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_MACRO_H_
#define COMPILER_PREPROCESSOR_MACRO_H_

#include <map>
#include <string>
#include <vector>

namespace pp
{

struct Token;

struct Macro
{
    enum Type
    {
        kTypeObj,
        kTypeFunc
    };

    bool equals(const Macro& other) const;

    Type type;
    std::string name;
    std::vector<std::string> parameters;
    std::vector<Token> replacements;
};

typedef std::map<std::string, Macro> MacroSet;

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_MACRO_H_

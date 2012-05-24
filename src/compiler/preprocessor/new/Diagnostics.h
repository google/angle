//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_PREPROCESSOR_DIAGNOSTICS_H_
#define COMPILER_PREPROCESSOR_DIAGNOSTICS_H_

#include <string>

namespace pp
{

struct SourceLocation;

// Base class for reporting diagnostic messages.
// Derived classes are responsible for formatting and printing the messages.
class Diagnostics
{
  public:
    enum ID
    {
        ERROR_BEGIN,
        INTERNAL_ERROR,
        OUT_OF_MEMORY,
        INVALID_CHARACTER,
        INVALID_NUMBER,
        INVALID_EXPRESSION,
        DIVISION_BY_ZERO,
        EOF_IN_COMMENT,
        EOF_IN_DIRECTIVE,
        UNEXPECTED_TOKEN_IN_DIRECTIVE,
        MACRO_NAME_RESERVED,
        MACRO_REDEFINED,
        ERROR_END,

        WARNING_BEGIN,
        UNRECOGNIZED_PRAGMA,
        WARNING_END
    };

    virtual ~Diagnostics();

    void report(ID id, const SourceLocation& loc, const std::string& text);

  protected:
    enum Severity
    {
        ERROR,
        WARNING
    };
    Severity severity(ID id);

    virtual void print(ID id,
                       const SourceLocation& loc,
                       const std::string& text) = 0;
};

}  // namespace pp
#endif  // COMPILER_PREPROCESSOR_DIAGNOSTICS_H_

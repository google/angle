//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gtest/gtest.h"
#include "MockDiagnostics.h"
#include "Preprocessor.h"
#include "Token.h"

static void PreprocessAndVerifyLocation(int count,
                                        const char* const string[],
                                        const int length[],
                                        const pp::SourceLocation& location)
{
    MockDiagnostics diagnostics;
    pp::Preprocessor preprocessor(&diagnostics);
    ASSERT_TRUE(preprocessor.init(count, string, length));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("foo", token.value);

    EXPECT_EQ(location.file, token.location.file);
    EXPECT_EQ(location.line, token.location.line);
}

TEST(LocationTest, String0_Line1)
{
    const char* str = "foo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("String0_Line1");
    PreprocessAndVerifyLocation(1, &str, 0, loc);
}

TEST(LocationTest, String0_Line2)
{
    const char* str = "\nfoo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 2;

    SCOPED_TRACE("String0_Line2");
    PreprocessAndVerifyLocation(1, &str, 0, loc);
}

TEST(LocationTest, String1_Line1)
{
    const char* const str[] = {"\n\n", "foo"};
    pp::SourceLocation loc;
    loc.file = 1;
    loc.line = 1;

    SCOPED_TRACE("String1_Line1");
    PreprocessAndVerifyLocation(2, str, 0, loc);
}

TEST(LocationTest, String1_Line2)
{
    const char* const str[] = {"\n\n", "\nfoo"};
    pp::SourceLocation loc;
    loc.file = 1;
    loc.line = 2;

    SCOPED_TRACE("String1_Line2");
    PreprocessAndVerifyLocation(2, str, 0, loc);
}

TEST(LocationTest, NewlineInsideCommentCounted)
{
    const char* str = "/*\n\n*/foo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 3;

    SCOPED_TRACE("NewlineInsideCommentCounted");
    PreprocessAndVerifyLocation(1, &str, 0, loc);
}

TEST(LocationTest, ErrorLocationAfterComment)
{
    const char* str = "/*\n\n*/@";

    MockDiagnostics diagnostics;
    pp::Preprocessor preprocessor(&diagnostics);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    pp::Diagnostics::ID id(pp::Diagnostics::INVALID_CHARACTER);
    pp::SourceLocation loc(0, 3);
    EXPECT_CALL(diagnostics, print(id, loc, "@"));

    pp::Token token;
    preprocessor.lex(&token);
}

// The location of a token straddling two or more strings is that of the
// first character of the token.

TEST(LocationTest, TokenStraddlingTwoStrings)
{
    const char* const str[] = {"f", "oo"};
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("TokenStraddlingTwoStrings");
    PreprocessAndVerifyLocation(2, str, 0, loc);
}

TEST(LocationTest, TokenStraddlingThreeStrings)
{
    const char* const str[] = {"f", "o", "o"};
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("TokenStraddlingThreeStrings");
    PreprocessAndVerifyLocation(3, str, 0, loc);
}

// TODO(alokp): Add tests for #line directives.

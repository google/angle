//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "PreprocessorTest.h"
#include "Token.h"

class LocationTest : public PreprocessorTest
{
protected:
    void preprocess(int count,
                    const char* const string[],
                    const int length[],
                    const pp::SourceLocation& location)
    {
        ASSERT_TRUE(mPreprocessor.init(count, string, length));

        pp::Token token;
        mPreprocessor.lex(&token);
        EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
        EXPECT_EQ("foo", token.value);

        EXPECT_EQ(location.file, token.location.file);
        EXPECT_EQ(location.line, token.location.line);
    }
};

TEST_F(LocationTest, String0_Line1)
{
    const char* str = "foo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("String0_Line1");
    preprocess(1, &str, NULL, loc);
}

TEST_F(LocationTest, String0_Line2)
{
    const char* str = "\nfoo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 2;

    SCOPED_TRACE("String0_Line2");
    preprocess(1, &str, NULL, loc);
}

TEST_F(LocationTest, String1_Line1)
{
    const char* const str[] = {"\n\n", "foo"};
    pp::SourceLocation loc;
    loc.file = 1;
    loc.line = 1;

    SCOPED_TRACE("String1_Line1");
    preprocess(2, str, NULL, loc);
}

TEST_F(LocationTest, String1_Line2)
{
    const char* const str[] = {"\n\n", "\nfoo"};
    pp::SourceLocation loc;
    loc.file = 1;
    loc.line = 2;

    SCOPED_TRACE("String1_Line2");
    preprocess(2, str, NULL, loc);
}

TEST_F(LocationTest, NewlineInsideCommentCounted)
{
    const char* str = "/*\n\n*/foo";
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 3;

    SCOPED_TRACE("NewlineInsideCommentCounted");
    preprocess(1, &str, NULL, loc);
}

TEST_F(LocationTest, ErrorLocationAfterComment)
{
    const char* str = "/*\n\n*/@";

    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::INVALID_CHARACTER,
                                    pp::SourceLocation(0, 3),
                                    "@"));

    pp::Token token;
    mPreprocessor.lex(&token);
}

// The location of a token straddling two or more strings is that of the
// first character of the token.

TEST_F(LocationTest, TokenStraddlingTwoStrings)
{
    const char* const str[] = {"f", "oo"};
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("TokenStraddlingTwoStrings");
    preprocess(2, str, NULL, loc);
}

TEST_F(LocationTest, TokenStraddlingThreeStrings)
{
    const char* const str[] = {"f", "o", "o"};
    pp::SourceLocation loc;
    loc.file = 0;
    loc.line = 1;

    SCOPED_TRACE("TokenStraddlingThreeStrings");
    preprocess(3, str, NULL, loc);
}

TEST_F(LocationTest, EndOfFileWithoutNewline)
{
    const char* const str[] = {"foo"};
    ASSERT_TRUE(mPreprocessor.init(1, str, NULL));

    pp::Token token;
    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("foo", token.value);
    EXPECT_EQ(0, token.location.file);
    EXPECT_EQ(1, token.location.line);

    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::LAST, token.type);
    EXPECT_EQ(0, token.location.file);
    EXPECT_EQ(1, token.location.line);
}

TEST_F(LocationTest, EndOfFileAfterNewline)
{
    const char* const str[] = {"foo\n"};
    ASSERT_TRUE(mPreprocessor.init(1, str, NULL));

    pp::Token token;
    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("foo", token.value);
    EXPECT_EQ(0, token.location.file);
    EXPECT_EQ(1, token.location.line);

    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::LAST, token.type);
    EXPECT_EQ(0, token.location.file);
    EXPECT_EQ(2, token.location.line);
}

TEST_F(LocationTest, EndOfFileAfterEmptyString)
{
    const char* const str[] = {"foo\n", "\n", ""};
    ASSERT_TRUE(mPreprocessor.init(3, str, NULL));

    pp::Token token;
    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("foo", token.value);
    EXPECT_EQ(0, token.location.file);
    EXPECT_EQ(1, token.location.line);

    mPreprocessor.lex(&token);
    EXPECT_EQ(pp::Token::LAST, token.type);
    EXPECT_EQ(2, token.location.file);
    EXPECT_EQ(1, token.location.line);
}

// TODO(alokp): Add tests for #line directives.

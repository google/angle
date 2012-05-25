//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "PreprocessorTest.h"
#include "Token.h"

class ErrorTest : public PreprocessorTest
{
  protected:
    void preprocess(const char* str)
    {
        ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

        pp::Token token;
        mPreprocessor.lex(&token);
        EXPECT_EQ(pp::Token::LAST, token.type);
        EXPECT_EQ("", token.value);
    }
};

TEST_F(ErrorTest, Empty)
{
    const char* str = "#error\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler, handleError(pp::SourceLocation(0, 1), ""));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str);
}

TEST_F(ErrorTest, OneTokenMessage)
{
    const char* str = "#error foo\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo"));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str);
}

TEST_F(ErrorTest, TwoTokenMessage)
{
    const char* str = "#error foo bar\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo bar"));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str);
}

TEST_F(ErrorTest, Comments)
{
    const char* str = "/*foo*/"
                      "#"
                      "/*foo*/"
                      "error"
                      "/*foo*/"
                      "foo"
                      "/*foo*/"
                      "bar"
                      "/*foo*/"
                      "//foo"
                      "\n";

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo bar"));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    preprocess(str);
}

TEST_F(ErrorTest, MissingNewline)
{
    const char* str = "#error foo";

    using testing::_;
    // Directive successfully parsed.
    EXPECT_CALL(mDirectiveHandler,
                handleError(pp::SourceLocation(0, 1), " foo"));
    // Error reported about EOF.
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::EOF_IN_DIRECTIVE, _, _));

    preprocess(str);
}

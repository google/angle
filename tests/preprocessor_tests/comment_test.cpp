//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gtest/gtest.h"

#include "MockDiagnostics.h"
#include "MockDirectiveHandler.h"
#include "Preprocessor.h"
#include "Token.h"

class CommentTest : public testing::TestWithParam<const char*>
{
};

TEST_P(CommentTest, CommentIgnored)
{
    const char* str = GetParam();

    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::LAST, token.type);
}

INSTANTIATE_TEST_CASE_P(LineComment, CommentTest,
                        testing::Values("//foo\n", // With newline.
                                        "//foo",   // Without newline.
                                        "//**/",   // Nested block comment.
                                        "////",    // Nested line comment.
                                        "//\""));  // Invalid character.  

INSTANTIATE_TEST_CASE_P(BlockComment, CommentTest,
                        testing::Values("/*foo*/",
                                        "/*foo\n*/", // With newline.
                                        "/*//*/",    // Nested line comment.
                                        "/*/**/",    // Nested block comment.
                                        "/***/",     // With lone '*'.
                                        "/*\"*/"));  // Invalid character.

TEST(BlockComment, CommentReplacedWithSpace)
{
    const char* str = "/*foo*/bar";

    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_EQ("bar", token.value);
    EXPECT_TRUE(token.hasLeadingSpace());
}

TEST(BlockComment, UnterminatedComment)
{
    const char* str = "/*foo";

    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    using testing::_;
    EXPECT_CALL(diagnostics, print(pp::Diagnostics::EOF_IN_COMMENT, _, _));

    pp::Token token;
    preprocessor.lex(&token);
}

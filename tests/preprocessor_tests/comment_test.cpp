//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gtest/gtest.h"
#include "Preprocessor.h"
#include "Token.h"

class CommentTest : public testing::TestWithParam<const char*>
{
};

TEST_P(CommentTest, CommentIgnored)
{
    const char* str = GetParam();

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &str, 0));
    EXPECT_EQ(pp::Token::LAST, preprocessor.lex(&token));
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

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &str, 0));
    EXPECT_EQ(pp::Token::IDENTIFIER, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ("bar", token.value.c_str());
    EXPECT_TRUE(token.hasLeadingSpace());
}

TEST(BlockComment, UnterminatedComment)
{
    const char* str = "/*foo";

    pp::Token token;
    pp::Preprocessor preprocessor;
    ASSERT_TRUE(preprocessor.init(1, &str, 0));
    EXPECT_EQ(pp::Token::EOF_IN_COMMENT, preprocessor.lex(&token));
    EXPECT_EQ(pp::Token::EOF_IN_COMMENT, token.type);
}

//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include <algorithm>

#include "gtest/gtest.h"
#include "Preprocessor.h"
#include "Token.h"

#if GTEST_HAS_PARAM_TEST

class CharTest : public testing::TestWithParam<int>
{
};

static const char kPunctuators[] = {
    '.', '+', '-', '/', '*', '%', '<', '>', '[', ']', '(', ')', '{', '}',
    '^', '|', '&', '~', '=', '!', ':', ';', ',', '?'};
static const int kNumPunctuators =
    sizeof(kPunctuators) / sizeof(kPunctuators[0]);

static const char kWhitespaces[] = {' ', '\t', '\v', '\f', '\n', '\r'};
static const int kNumWhitespaces =
    sizeof(kWhitespaces) / sizeof(kWhitespaces[0]);

TEST_P(CharTest, Identified)
{
    std::string str(1, GetParam());
    const char* cstr = str.c_str();
    int length = 1;

    pp::Preprocessor preprocessor;
    // Note that we pass the length param as well because the invalid
    // string may contain the null character.
    ASSERT_TRUE(preprocessor.init(1, &cstr, &length));

    pp::Token token;
    int ret = preprocessor.lex(&token);

    // Handle identifier.
    if ((cstr[0] == '_') ||
        ((cstr[0] >= 'a') && (cstr[0] <= 'z')) ||
        ((cstr[0] >= 'A') && (cstr[0] <= 'Z')))
    {
        EXPECT_EQ(pp::Token::IDENTIFIER, ret);
        EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
        EXPECT_EQ(cstr[0], token.value[0]);
        return;
    }

    // Handle numbers.
    if (cstr[0] >= '0' && cstr[0] <= '9')
    {
        EXPECT_EQ(pp::Token::CONST_INT, ret);
        EXPECT_EQ(pp::Token::CONST_INT, token.type);
        EXPECT_EQ(cstr[0], token.value[0]);
        return;
    }

    // Handle punctuators.
    const char* lastIter = kPunctuators + kNumPunctuators;
    const char* iter = std::find(kPunctuators, lastIter, cstr[0]);
    if (iter != lastIter)
    {
        EXPECT_EQ(cstr[0], ret);
        EXPECT_EQ(cstr[0], token.type);
        EXPECT_TRUE(token.value.empty());
        return;
    }

    // Handle whitespace.
    lastIter = kWhitespaces + kNumWhitespaces;
    iter = std::find(kWhitespaces, lastIter, cstr[0]);
    if (iter != lastIter)
    {
        // Whitespace is ignored.
        EXPECT_EQ(pp::Token::LAST, ret);
        EXPECT_EQ(pp::Token::LAST, token.type);
        EXPECT_TRUE(token.value.empty());
        return;
    }

    // Handle number sign.
    if (cstr[0] == '#')
    {
        // Lone '#' is ignored.
        EXPECT_EQ(pp::Token::LAST, ret);
        EXPECT_EQ(pp::Token::LAST, token.type);
        EXPECT_TRUE(token.value.empty());
        return;
    }

    // Everything else is invalid.
    EXPECT_EQ(pp::Token::INVALID_CHARACTER, ret);
    EXPECT_EQ(pp::Token::INVALID_CHARACTER, token.type);
    EXPECT_EQ(cstr[0], token.value[0]);
};

// Note +1 for the max-value in range. It is there because the max-value
// not included in the range.
INSTANTIATE_TEST_CASE_P(AllCharacters, CharTest,
                        testing::Range(-127, 127 + 1));

#endif  // GTEST_HAS_PARAM_TEST

//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gtest/gtest.h"

#include "MockDiagnostics.h"
#include "Preprocessor.h"
#include "Token.h"

static void PreprocessAndVerifyIdentifier(const char* str)
{
    MockDiagnostics diagnostics;
    pp::Preprocessor preprocessor(&diagnostics);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::IDENTIFIER, token.type);
    EXPECT_STREQ(str, token.value.c_str());
}

#if GTEST_HAS_COMBINE

typedef std::tr1::tuple<char, char> IdentifierParams;
class IdentifierTest : public testing::TestWithParam<IdentifierParams>
{
};

// This test covers identifier names of form [_a-zA-Z][_a-zA-Z0-9]?.
TEST_P(IdentifierTest, IdentifierIdentified)
{
    std::string str(1, std::tr1::get<0>(GetParam()));
    char c = std::tr1::get<1>(GetParam());
    if (c != '\0') str.push_back(c);

    PreprocessAndVerifyIdentifier(str.c_str());
}

#define CLOSED_RANGE(x, y) testing::Range(x, static_cast<char>((y) + 1))

// Test string: '_'
INSTANTIATE_TEST_CASE_P(SingleLetter_Underscore,
                        IdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         testing::Values('\0')));

// Test string: [a-z]
INSTANTIATE_TEST_CASE_P(SingleLetter_a_z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         testing::Values('\0')));

// Test string: [A-Z]
INSTANTIATE_TEST_CASE_P(SingleLetter_A_Z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         testing::Values('\0')));

// Test string: "__"
INSTANTIATE_TEST_CASE_P(DoubleLetter_Underscore_Underscore,
                        IdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         testing::Values('_')));

// Test string: "_"[a-z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_Underscore_a_z,
                        IdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('a', 'z')));

// Test string: "_"[A-Z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_Underscore_A_Z,
                        IdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('A', 'Z')));

// Test string: "_"[0-9]
INSTANTIATE_TEST_CASE_P(DoubleLetter_Underscore_0_9,
                        IdentifierTest,
                        testing::Combine(testing::Values('_'),
                                         CLOSED_RANGE('0', '9')));

// Test string: [a-z]"_"
INSTANTIATE_TEST_CASE_P(DoubleLetter_a_z_Underscore,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         testing::Values('_')));

// Test string: [a-z][a-z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_a_z_a_z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('a', 'z')));

// Test string: [a-z][A-Z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_a_z_A_Z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('A', 'Z')));

// Test string: [a-z][0-9]
INSTANTIATE_TEST_CASE_P(DoubleLetter_a_z_0_9,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('a', 'z'),
                                         CLOSED_RANGE('0', '9')));

// Test string: [A-Z]"_"
INSTANTIATE_TEST_CASE_P(DoubleLetter_A_Z_Underscore,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         testing::Values('_')));

// Test string: [A-Z][a-z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_A_Z_a_z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('a', 'z')));

// Test string: [A-Z][A-Z]
INSTANTIATE_TEST_CASE_P(DoubleLetter_A_Z_A_Z,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('A', 'Z')));

// Test string: [A-Z][0-9]
INSTANTIATE_TEST_CASE_P(DoubleLetter_A_Z_0_9,
                        IdentifierTest,
                        testing::Combine(CLOSED_RANGE('A', 'Z'),
                                         CLOSED_RANGE('0', '9')));

#endif  // GTEST_HAS_COMBINE

// The tests above cover one-letter and various combinations of two-letter
// identifier names. This test covers all characters in a single string.
TEST(IdentifierTestAllCharacters, IdentifierIdentified)
{
    std::string str;
    for (int c = 'a'; c <= 'z'; ++c)
        str.push_back(c);

    str.push_back('_');

    for (int c = 'A'; c <= 'Z'; ++c)
        str.push_back(c);

    str.push_back('_');

    for (int c = '0'; c <= '9'; ++c)
        str.push_back(c);

    PreprocessAndVerifyIdentifier(str.c_str());
}

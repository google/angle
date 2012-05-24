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

#if GTEST_HAS_PARAM_TEST

class InvalidNumberTest : public testing::TestWithParam<const char*>
{
};

TEST_P(InvalidNumberTest, InvalidNumberIdentified)
{
    const char* str = GetParam();

    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    using testing::_;
    EXPECT_CALL(diagnostics, print(pp::Diagnostics::INVALID_NUMBER, _, str));

    pp::Token token;
    preprocessor.lex(&token);
}

INSTANTIATE_TEST_CASE_P(InvalidIntegers, InvalidNumberTest,
                        testing::Values("1a", "08", "0xG"));


INSTANTIATE_TEST_CASE_P(InvalidFloats, InvalidNumberTest,
                        testing::Values("1eg", "0.a", "0.1.2", ".0a", ".0.1"));

#endif  // GTEST_HAS_PARAM_TEST

#if GTEST_HAS_COMBINE

typedef std::tr1::tuple<const char*, char> IntegerParams;
class IntegerTest : public testing::TestWithParam<IntegerParams>
{
};

TEST_P(IntegerTest, IntegerIdentified)
{
    std::string str(std::tr1::get<0>(GetParam()));  // prefix.
    str.push_back(std::tr1::get<1>(GetParam()));  // digit.
    const char* cstr = str.c_str();

    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &cstr, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::CONST_INT, token.type);
    EXPECT_EQ(str, token.value);
}

#define CLOSED_RANGE(x, y) testing::Range(x, static_cast<char>((y) + 1))

INSTANTIATE_TEST_CASE_P(DecimalInteger,
                        IntegerTest,
                        testing::Combine(testing::Values(""),
                                         CLOSED_RANGE('0', '9')));

INSTANTIATE_TEST_CASE_P(OctalInteger,
                        IntegerTest,
                        testing::Combine(testing::Values("0"),
                                         CLOSED_RANGE('0', '7')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_0_9,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         CLOSED_RANGE('0', '9')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_a_f,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         CLOSED_RANGE('a', 'f')));

INSTANTIATE_TEST_CASE_P(HexadecimalInteger_A_F,
                        IntegerTest,
                        testing::Combine(testing::Values("0x"),
                                         CLOSED_RANGE('A', 'F')));

static void PreprocessAndVerifyFloat(const char* str)
{
    MockDiagnostics diagnostics;
    MockDirectiveHandler directiveHandler;
    pp::Preprocessor preprocessor(&diagnostics, &directiveHandler);
    ASSERT_TRUE(preprocessor.init(1, &str, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(pp::Token::CONST_FLOAT, token.type);
    EXPECT_EQ(str, token.value);
}

typedef std::tr1::tuple<char, char, const char*, char> FloatScientificParams;
class FloatScientificTest : public testing::TestWithParam<FloatScientificParams>
{
};

// This test covers floating point numbers of form [0-9][eE][+-]?[0-9].
TEST_P(FloatScientificTest, FloatIdentified)
{
    std::string str;
    str.push_back(std::tr1::get<0>(GetParam()));  // significand [0-9].
    str.push_back(std::tr1::get<1>(GetParam()));  // separator [eE].
    str.append(std::tr1::get<2>(GetParam()));  // sign [" " "+" "-"].
    str.push_back(std::tr1::get<3>(GetParam()));  // exponent [0-9].

    SCOPED_TRACE("FloatScientificTest");
    PreprocessAndVerifyFloat(str.c_str());
}

INSTANTIATE_TEST_CASE_P(FloatScientific,
                        FloatScientificTest,
                        testing::Combine(CLOSED_RANGE('0', '9'),
                                         testing::Values('e', 'E'),
                                         testing::Values("", "+", "-"),
                                         CLOSED_RANGE('0', '9')));

typedef std::tr1::tuple<char, char> FloatFractionParams;
class FloatFractionTest : public testing::TestWithParam<FloatFractionParams>
{
};

// This test covers floating point numbers of form [0-9]"." and [0-9]?"."[0-9].
TEST_P(FloatFractionTest, FloatIdentified)
{
    std::string str;

    char significand = std::tr1::get<0>(GetParam());
    if (significand != '\0')
        str.push_back(significand);

    str.push_back('.');

    char fraction = std::tr1::get<1>(GetParam());
    if (fraction != '\0')
        str.push_back(fraction);

    SCOPED_TRACE("FloatFractionTest");
    PreprocessAndVerifyFloat(str.c_str());
}

INSTANTIATE_TEST_CASE_P(FloatFraction_X_X,
                        FloatFractionTest,
                        testing::Combine(CLOSED_RANGE('0', '9'),
                                         CLOSED_RANGE('0', '9')));

INSTANTIATE_TEST_CASE_P(FloatFraction_0_X,
                        FloatFractionTest,
                        testing::Combine(testing::Values('\0'),
                                         CLOSED_RANGE('0', '9')));

INSTANTIATE_TEST_CASE_P(FloatFraction_X_0,
                        FloatFractionTest,
                        testing::Combine(CLOSED_RANGE('0', '9'),
                                         testing::Values('\0')));

#endif  // GTEST_HAS_COMBINE

// In the tests above we have tested individual parts of a float separately.
// This test has all parts of a float.

TEST(FloatFractionScientificTest, FloatIdentified)
{
    SCOPED_TRACE("FloatFractionScientificTest");
    PreprocessAndVerifyFloat("0.1e+2");
}

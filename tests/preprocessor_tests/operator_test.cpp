//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gtest/gtest.h"

#include "MockDiagnostics.h"
#include "Preprocessor.h"
#include "Token.h"

#if GTEST_HAS_PARAM_TEST

struct OperatorTestParam
{
    const char* str;
    int op;
};

class OperatorTest : public testing::TestWithParam<OperatorTestParam>
{
};

TEST_P(OperatorTest, Identified)
{
    OperatorTestParam param = GetParam();

    MockDiagnostics diagnostics;
    pp::Preprocessor preprocessor(&diagnostics);
    ASSERT_TRUE(preprocessor.init(1, &param.str, 0));

    pp::Token token;
    preprocessor.lex(&token);
    EXPECT_EQ(param.op, token.type);
    EXPECT_EQ(param.str, token.value);
}

static const OperatorTestParam kOperators[] = {
    {"(", '('},
    {")", ')'},
    {"[", '['},
    {"]", ']'},
    {".", '.'},
    {"+", '+'},
    {"-", '-'},
    {"~", '~'},
    {"!", '!'},
    {"*", '*'},
    {"/", '/'},
    {"%", '%'},
    {"<", '<'},
    {">", '>'},
    {"&", '&'},
    {"^", '^'},
    {"|", '|'},
    {"?", '?'},
    {":", ':'},
    {"=", '='},
    {",", ','},
    {"++",  pp::Token::OP_INC},
    {"--",  pp::Token::OP_DEC},
    {"<<",  pp::Token::OP_LEFT},
    {">>",  pp::Token::OP_RIGHT},
    {"<=",  pp::Token::OP_LE},
    {">=",  pp::Token::OP_GE},
    {"==",  pp::Token::OP_EQ},
    {"!=",  pp::Token::OP_NE},
    {"&&",  pp::Token::OP_AND},
    {"^^",  pp::Token::OP_XOR},
    {"||",  pp::Token::OP_OR},
    {"+=",  pp::Token::OP_ADD_ASSIGN},
    {"-=",  pp::Token::OP_SUB_ASSIGN},
    {"*=",  pp::Token::OP_MUL_ASSIGN},
    {"/=",  pp::Token::OP_DIV_ASSIGN},
    {"%=",  pp::Token::OP_MOD_ASSIGN},
    {"<<=", pp::Token::OP_LEFT_ASSIGN},
    {">>=", pp::Token::OP_RIGHT_ASSIGN},
    {"&=",  pp::Token::OP_AND_ASSIGN},
    {"^=",  pp::Token::OP_XOR_ASSIGN},
    {"|=",  pp::Token::OP_OR_ASSIGN}
};

INSTANTIATE_TEST_CASE_P(All, OperatorTest,
                        testing::ValuesIn(kOperators));

#endif  // GTEST_HAS_PARAM_TEST

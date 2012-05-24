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

class VersionTest : public testing::Test
{
protected:
    VersionTest() : mPreprocessor(&mDiagnostics, &mDirectiveHandler) { }

    void lex()
    {
        pp::Token token;
        mPreprocessor.lex(&token);
        EXPECT_EQ(pp::Token::LAST, token.type);
        EXPECT_EQ("", token.value);
    }

    MockDiagnostics mDiagnostics;
    MockDirectiveHandler mDirectiveHandler;
    pp::Preprocessor mPreprocessor;
};

TEST_F(VersionTest, Valid)
{
    const char* str = "#version 200\n";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleVersion(pp::SourceLocation(0, 1), 200));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    lex();
}

TEST_F(VersionTest, CommentsIgnored)
{
    const char* str = "/*foo*/"
                      "#"
                      "/*foo*/"
                      "version"
                      "/*foo*/"
                      "200"
                      "/*foo*/"
                      "//foo"
                      "\n";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleVersion(pp::SourceLocation(0, 1), 200));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    lex();
}

TEST_F(VersionTest, MissingNewline)
{
    const char* str = "#version 200";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    // Directive successfully parsed.
    EXPECT_CALL(mDirectiveHandler,
                handleVersion(pp::SourceLocation(0, 1), 200));
    // Error reported about EOF.
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::EOF_IN_DIRECTIVE, _, _));

    lex();
}

struct VersionTestParam
{
    const char* str;
    pp::Diagnostics::ID id;
};

class InvalidVersionTest : public VersionTest,
                           public testing::WithParamInterface<VersionTestParam>
{
};

TEST_P(InvalidVersionTest, Identified)
{
    VersionTestParam param = GetParam();
    ASSERT_TRUE(mPreprocessor.init(1, &param.str, NULL));

    using testing::_;
    // No handleVersion call.
    EXPECT_CALL(mDirectiveHandler, handleVersion(_, _)).Times(0);
    // Invalid version directive call.
    EXPECT_CALL(mDiagnostics, print(param.id, pp::SourceLocation(0, 1), _));

    lex();
}

static const VersionTestParam kParams[] = {
    {"#version\n", pp::Diagnostics::INVALID_VERSION_DIRECTIVE},
    {"#version foo\n", pp::Diagnostics::INVALID_VERSION_NUMBER},
    {"#version 100 foo\n", pp::Diagnostics::UNEXPECTED_TOKEN_IN_DIRECTIVE}
};

INSTANTIATE_TEST_CASE_P(All, InvalidVersionTest, testing::ValuesIn(kParams));

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

class ExtensionTest : public testing::Test
{
protected:
    ExtensionTest() : mPreprocessor(&mDiagnostics, &mDirectiveHandler) { }

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

TEST_F(ExtensionTest, Valid)
{
    const char* str = "#extension foo : bar\n";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleExtension(pp::SourceLocation(0, 1), "foo", "bar"));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    lex();
}

TEST_F(ExtensionTest, Comments)
{
    const char* str = "/*foo*/"
                      "#"
                      "/*foo*/"
                      "extension"
                      "/*foo*/"
                      "foo"
                      "/*foo*/"
                      ":"
                      "/*foo*/"
                      "bar"
                      "/*foo*/"
                      "//foo"
                      "\n";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    EXPECT_CALL(mDirectiveHandler,
                handleExtension(pp::SourceLocation(0, 1), "foo", "bar"));
    // No error or warning.
    EXPECT_CALL(mDiagnostics, print(_, _, _)).Times(0);

    lex();
}

TEST_F(ExtensionTest, MissingNewline)
{
    const char* str = "#extension foo : bar";
    ASSERT_TRUE(mPreprocessor.init(1, &str, NULL));

    using testing::_;
    // Directive successfully parsed.
    EXPECT_CALL(mDirectiveHandler,
                handleExtension(pp::SourceLocation(0, 1), "foo", "bar"));
    // Error reported about EOF.
    EXPECT_CALL(mDiagnostics, print(pp::Diagnostics::EOF_IN_DIRECTIVE, _, _));

    lex();
}

struct ExtensionTestParam
{
    const char* str;
    pp::Diagnostics::ID id;
};

using testing::WithParamInterface;
class InvalidExtensionTest : public ExtensionTest,
                             public WithParamInterface<ExtensionTestParam>
{
};

TEST_P(InvalidExtensionTest, Identified)
{
    ExtensionTestParam param = GetParam();
    ASSERT_TRUE(mPreprocessor.init(1, &param.str, NULL));

    using testing::_;
    // No handleExtension call.
    EXPECT_CALL(mDirectiveHandler, handleExtension(_, _, _)).Times(0);
    // Invalid extension directive call.
    EXPECT_CALL(mDiagnostics, print(param.id, pp::SourceLocation(0, 1), _));

    lex();
}

static const ExtensionTestParam kParams[] = {
    {"#extension\n", pp::Diagnostics::INVALID_EXTENSION_DIRECTIVE},
    {"#extension 1\n", pp::Diagnostics::INVALID_EXTENSION_NAME},
    {"#extension foo bar\n", pp::Diagnostics::UNEXPECTED_TOKEN},
    {"#extension foo : \n", pp::Diagnostics::INVALID_EXTENSION_DIRECTIVE},
    {"#extension foo : 1\n", pp::Diagnostics::INVALID_EXTENSION_BEHAVIOR},
    {"#extension foo : bar baz\n", pp::Diagnostics::UNEXPECTED_TOKEN}
};
INSTANTIATE_TEST_CASE_P(All, InvalidExtensionTest, testing::ValuesIn(kParams));

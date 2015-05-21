//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// string_utils_unittests:
//   Unit tests for the string utils.
//

#include "string_utils.h"

#include <gtest/gtest.h>

using namespace angle;

namespace
{

// Basic functionality tests for SplitString
TEST(StringUtilsTest, SplitStringBasic)
{
    std::string testString("AxBxCxxxDExxFGHx");
    std::vector<std::string> tokens;
    SplitString(testString, 'x', &tokens);

    ASSERT_EQ(5u, tokens.size());
    EXPECT_EQ("A", tokens[0]);
    EXPECT_EQ("B", tokens[1]);
    EXPECT_EQ("C", tokens[2]);
    EXPECT_EQ("DE", tokens[3]);
    EXPECT_EQ("FGH", tokens[4]);
}

// Basic functionality tests for SplitStringAlongWhitespace
TEST(StringUtilsTest, SplitStringAlongWhitespaceBasic)
{
    std::string testString("A B\nC\r\tDE\v\fFGH \t\r\n");
    std::vector<std::string> tokens;
    SplitStringAlongWhitespace(testString, &tokens);

    ASSERT_EQ(5u, tokens.size());
    EXPECT_EQ("A", tokens[0]);
    EXPECT_EQ("B", tokens[1]);
    EXPECT_EQ("C", tokens[2]);
    EXPECT_EQ("DE", tokens[3]);
    EXPECT_EQ("FGH", tokens[4]);
}

// Basic functionality tests for HexStringToUInt
TEST(StringUtilsTest, HexStringToUIntBasic)
{
    std::string testStringA("0xBADF00D");
    unsigned int uintValue;
    ASSERT_TRUE(HexStringToUInt(testStringA, &uintValue));
    EXPECT_EQ(0xBADF00Du, uintValue);

    std::string testStringB("0xBADFOOD");
    EXPECT_FALSE(HexStringToUInt(testStringB, &uintValue));

    std::string testStringC("BADF00D");
    EXPECT_FALSE(HexStringToUInt(testStringC, &uintValue));
}

// Note: ReadFileToString is harder to test

}

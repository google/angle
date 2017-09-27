//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// utilities_unittest.cpp: Unit tests for ANGLE's GL utility functions

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/utilities.h"

namespace
{

// Test parsing valid single array indices
TEST(ParseResourceName, ArrayIndex)
{
    std::vector<unsigned int> indices;
    EXPECT_EQ("foo", gl::ParseResourceName("foo[123]", &indices));
    ASSERT_EQ(1u, indices.size());
    EXPECT_EQ(123u, indices[0]);

    EXPECT_EQ("bar", gl::ParseResourceName("bar[0]", &indices));
    ASSERT_EQ(1u, indices.size());
    EXPECT_EQ(0u, indices[0]);
}

// Parsing a negative array index should result in INVALID_INDEX.
TEST(ParseResourceName, NegativeArrayIndex)
{
    std::vector<unsigned int> indices;
    EXPECT_EQ("foo", gl::ParseResourceName("foo[-1]", &indices));
    ASSERT_EQ(1u, indices.size());
    EXPECT_EQ(GL_INVALID_INDEX, indices.back());
}

// Parsing no array indices should result in an empty array.
TEST(ParseResourceName, NoArrayIndex)
{
    std::vector<unsigned int> indices;
    EXPECT_EQ("foo", gl::ParseResourceName("foo", &indices));
    EXPECT_TRUE(indices.empty());
}

// The ParseResourceName function should work when a nullptr is passed as the indices output vector.
TEST(ParseResourceName, NULLArrayIndices)
{
    EXPECT_EQ("foo", gl::ParseResourceName("foo[10]", nullptr));
}

// Parsing multiple array indices should result in outermost array indices being last in the vector.
TEST(ParseResourceName, MultipleArrayIndices)
{
    std::vector<unsigned int> indices;
    EXPECT_EQ("foo", gl::ParseResourceName("foo[12][34][56]", &indices));
    ASSERT_EQ(3u, indices.size());
    // Indices are sorted with outermost array index last.
    EXPECT_EQ(56u, indices[0]);
    EXPECT_EQ(34u, indices[1]);
    EXPECT_EQ(12u, indices[2]);
}

// Trailing whitespace should not be accepted by ParseResourceName.
TEST(ParseResourceName, TrailingWhitespace)
{
    std::vector<unsigned int> indices;
    EXPECT_EQ("foo ", gl::ParseResourceName("foo ", &indices));
    EXPECT_TRUE(indices.empty());

    EXPECT_EQ("foo[10] ", gl::ParseResourceName("foo[10] ", &indices));
    EXPECT_TRUE(indices.empty());

    EXPECT_EQ("foo[10][20] ", gl::ParseResourceName("foo[10][20] ", &indices));
    EXPECT_TRUE(indices.empty());
}

}

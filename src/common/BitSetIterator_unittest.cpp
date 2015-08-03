//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BitSetIteratorTest:
//   Test the IterableBitSet class.
//

#include <gtest/gtest.h>

#include "common/BitSetIterator.h"

using namespace angle;

namespace
{
class BitSetIteratorTest : public testing::Test
{
  protected:
    std::bitset<40> mStateBits;
};

TEST_F(BitSetIteratorTest, Iterator)
{
    std::set<unsigned long> originalValues;
    originalValues.insert(2);
    originalValues.insert(6);
    originalValues.insert(8);
    originalValues.insert(35);

    for (unsigned long value : originalValues)
    {
        mStateBits.set(value);
    }

    std::set<unsigned long> readValues;
    for (unsigned long bit : IterateBitSet(mStateBits))
    {
        EXPECT_EQ(1u, originalValues.count(bit));
        EXPECT_EQ(0u, readValues.count(bit));
        readValues.insert(bit);
    }

    EXPECT_EQ(originalValues.size(), readValues.size());
}

}  // anonymous namespace

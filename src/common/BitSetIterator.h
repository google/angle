//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BitSetIterator:
//   A helper class to quickly bitscan bitsets for set bits.
//

#ifndef COMMON_BITSETITERATOR_H_
#define COMMON_BITSETITERATOR_H_

#include <stdint.h>

#include <bitset>

#include "common/angleutils.h"
#include "common/platform.h"

namespace angle
{
template <size_t N>
class BitSetIterator final
{
  public:
    BitSetIterator(const std::bitset<N> &bitset);
    BitSetIterator(const BitSetIterator &other);

    class Iterator final
    {
      public:
        Iterator(const std::bitset<N> *bits);
        Iterator &operator++();

        bool operator!=(const Iterator &other) const { return mBits != other.mBits; }
        size_t operator*() const { return mCurrentBit; }
        size_t getNextBit() const;

      private:
        unsigned long long mBits;
        size_t mCurrentBit;
    };

    Iterator begin() const { return Iterator(&mBits); }
    Iterator end() const { return Iterator(nullptr); }

  private:
    const std::bitset<N> &mBits;
};

template <size_t N>
BitSetIterator<N>::BitSetIterator(const std::bitset<N> &bitset)
    : mBits(bitset)
{
    static_assert(N <= 64, "IterableBitSet only supports up to 64 bits.");
}

template <size_t N>
BitSetIterator<N>::BitSetIterator(const BitSetIterator &other)
    : mBits(other.mBits)
{
}

template <size_t N>
BitSetIterator<N>::Iterator::Iterator(const std::bitset<N> *bits)
    : mBits(bits ? bits->to_ullong() : 0ull), mCurrentBit(bits ? getNextBit() : 0ull)
{
}

template <size_t N>
typename BitSetIterator<N>::Iterator &BitSetIterator<N>::Iterator::operator++()
{
    mBits &= ~(1ull << mCurrentBit);
    mCurrentBit = getNextBit();
    return *this;
}

template <size_t N>
size_t BitSetIterator<N>::Iterator::getNextBit() const
{
#if defined(_WIN64)
    static_assert(sizeof(uint64_t) == sizeof(unsigned long long), "Incompatible sizes");

    unsigned long firstBitIndex = 0ul;
    unsigned char ret           = _BitScanForward64(&firstBitIndex, mBits);

    if (ret != 0)
        return static_cast<size_t>(firstBitIndex);
#elif defined(_WIN32)
    static_assert(sizeof(size_t) == sizeof(unsigned long), "Incompatible sizes");

    unsigned long firstBitIndex     = 0ul;
    const unsigned long BitsPerWord = 32ul;

    for (unsigned long offset = 0; offset <= BitsPerWord; offset += BitsPerWord)
    {
        unsigned long wordBits = mBits >> offset;
        unsigned char ret      = _BitScanForward(&firstBitIndex, wordBits);

        if (ret != 0)
        {
            return static_cast<size_t>(firstBitIndex + offset);
        }
    }
#elif defined(ANGLE_PLATFORM_LINUX) || defined(ANGLE_PLATFORM_APPLE)
    if (mBits != 0)
    {
        return __builtin_ctzll(mBits);
    }
#else
#error Please implement bit-scan-forward for your platform!
#endif
    return 0;
}

// Helper to avoid needing to specify the template parameter size
template <size_t N>
BitSetIterator<N> IterateBitSet(const std::bitset<N> &bitset)
{
    return BitSetIterator<N>(bitset);
}

}  // angle

#endif // COMMON_BITSETITERATOR_H_

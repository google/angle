//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// bitset_utils:
//   Bitset-related helper classes, such as a fast iterator to scan for set bits.
//

#ifndef COMMON_BITSETITERATOR_H_
#define COMMON_BITSETITERATOR_H_

#include <stdint.h>

#include <bitset>

#include "common/angleutils.h"
#include "common/debug.h"
#include "common/mathutil.h"
#include "common/platform.h"

namespace angle
{
template <typename BitsT, typename ParamT>
constexpr static BitsT Bit(ParamT x)
{
    // It's undefined behavior if the shift size is equal to or larger than the width of the type.
    ASSERT(static_cast<size_t>(x) < sizeof(BitsT) * 8);

    return (static_cast<BitsT>(1) << static_cast<size_t>(x));
}

template <size_t N, typename BitsT, typename ParamT = std::size_t>
class BitSetT final
{
  public:
    class Reference final
    {
      public:
        ~Reference() {}
        Reference &operator=(bool x)
        {
            mParent->set(mBit, x);
            return *this;
        }
        explicit operator bool() const { return mParent->test(mBit); }

      private:
        friend class BitSetT;

        Reference(BitSetT *parent, ParamT bit) : mParent(parent), mBit(bit) {}

        BitSetT *mParent;
        ParamT mBit;
    };

    class Iterator final
    {
      public:
        Iterator(const BitSetT &bits);
        Iterator &operator++();

        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;
        ParamT operator*() const;

        // These helper functions allow mutating an iterator in-flight.
        // They only operate on later bits to ensure we don't iterate the same bit twice.
        void resetLaterBit(std::size_t index)
        {
            ASSERT(index > mCurrentBit);
            mBitsCopy.reset(index);
        }

        void setLaterBit(std::size_t index)
        {
            ASSERT(index > mCurrentBit);
            mBitsCopy.set(index);
        }

      private:
        std::size_t getNextBit();

        BitSetT mBitsCopy;
        std::size_t mCurrentBit;
    };

    using value_type = BitsT;

    constexpr BitSetT();
    constexpr explicit BitSetT(BitsT value);
    constexpr explicit BitSetT(std::initializer_list<ParamT> init);

    constexpr BitSetT(const BitSetT &other);
    constexpr BitSetT &operator=(const BitSetT &other);

    constexpr bool operator==(const BitSetT &other) const;
    constexpr bool operator!=(const BitSetT &other) const;

    constexpr bool operator[](ParamT pos) const;
    Reference operator[](ParamT pos) { return Reference(this, pos); }

    constexpr bool test(ParamT pos) const;

    constexpr bool all() const;
    constexpr bool any() const;
    constexpr bool none() const;
    constexpr std::size_t count() const;

    constexpr std::size_t size() const { return N; }

    constexpr BitSetT &operator&=(const BitSetT &other);
    constexpr BitSetT &operator|=(const BitSetT &other);
    constexpr BitSetT &operator^=(const BitSetT &other);
    constexpr BitSetT operator~() const;

    constexpr BitSetT &operator&=(BitsT value);
    constexpr BitSetT &operator|=(BitsT value);
    constexpr BitSetT &operator^=(BitsT value);

    constexpr BitSetT operator<<(std::size_t pos) const;
    constexpr BitSetT &operator<<=(std::size_t pos);
    constexpr BitSetT operator>>(std::size_t pos) const;
    constexpr BitSetT &operator>>=(std::size_t pos);

    constexpr BitSetT &set();
    constexpr BitSetT &set(ParamT pos, bool value = true);

    constexpr BitSetT &reset();
    constexpr BitSetT &reset(ParamT pos);

    constexpr BitSetT &flip();
    constexpr BitSetT &flip(ParamT pos);

    constexpr unsigned long to_ulong() const { return static_cast<unsigned long>(mBits); }
    constexpr BitsT bits() const { return mBits; }

    Iterator begin() const { return Iterator(*this); }
    Iterator end() const { return Iterator(BitSetT()); }

    constexpr static BitSetT Zero() { return BitSetT(); }

    constexpr ParamT first() const;
    constexpr ParamT last() const;

    // Produces a mask of ones up to the "x"th bit.
    constexpr static BitsT Mask(std::size_t x)
    {
        return ((Bit<BitsT>(static_cast<ParamT>(x - 1)) - 1) << 1) + 1;
    }

  private:
    BitsT mBits;
};

template <size_t N>
class IterableBitSet : public std::bitset<N>
{
  public:
    constexpr IterableBitSet() {}
    constexpr IterableBitSet(const std::bitset<N> &implicitBitSet) : std::bitset<N>(implicitBitSet)
    {}

    class Iterator final
    {
      public:
        Iterator(const std::bitset<N> &bits);
        Iterator &operator++();

        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;
        unsigned long operator*() const { return mCurrentBit; }

        // These helper functions allow mutating an iterator in-flight.
        // They only operate on later bits to ensure we don't iterate the same bit twice.
        void resetLaterBit(std::size_t index)
        {
            ASSERT(index > mCurrentBit);
            mBits.reset(index - mOffset);
        }

        void setLaterBit(std::size_t index)
        {
            ASSERT(index > mCurrentBit);
            mBits.set(index - mOffset);
        }

      private:
        unsigned long getNextBit();

        static constexpr size_t BitsPerWord = sizeof(uint32_t) * 8;
        std::bitset<N> mBits;
        unsigned long mCurrentBit;
        unsigned long mOffset;
    };

    Iterator begin() const { return Iterator(*this); }
    Iterator end() const { return Iterator(std::bitset<N>(0)); }
};

template <size_t N>
IterableBitSet<N>::Iterator::Iterator(const std::bitset<N> &bitset)
    : mBits(bitset), mCurrentBit(0), mOffset(0)
{
    if (mBits.any())
    {
        mCurrentBit = getNextBit();
    }
    else
    {
        mOffset = static_cast<unsigned long>(rx::roundUpPow2(N, BitsPerWord));
    }
}

template <size_t N>
ANGLE_INLINE typename IterableBitSet<N>::Iterator &IterableBitSet<N>::Iterator::operator++()
{
    ASSERT(mBits.any());
    mBits.set(mCurrentBit - mOffset, 0);
    mCurrentBit = getNextBit();
    return *this;
}

template <size_t N>
bool IterableBitSet<N>::Iterator::operator==(const Iterator &other) const
{
    return mOffset == other.mOffset && mBits == other.mBits;
}

template <size_t N>
bool IterableBitSet<N>::Iterator::operator!=(const Iterator &other) const
{
    return !(*this == other);
}

template <size_t N>
unsigned long IterableBitSet<N>::Iterator::getNextBit()
{
    // TODO(jmadill): Use 64-bit scan when possible.
    static constexpr std::bitset<N> wordMask(std::numeric_limits<uint32_t>::max());

    while (mOffset < N)
    {
        uint32_t wordBits = static_cast<uint32_t>((mBits & wordMask).to_ulong());
        if (wordBits != 0)
        {
            return gl::ScanForward(wordBits) + mOffset;
        }

        mBits >>= BitsPerWord;
        mOffset += BitsPerWord;
    }
    return 0;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT() : mBits(0)
{
    static_assert(N > 0, "Bitset type cannot support zero bits.");
    static_assert(N <= sizeof(BitsT) * 8, "Bitset type cannot support a size this large.");
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT(BitsT value) : mBits(value & Mask(N))
{}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT(std::initializer_list<ParamT> init) : mBits(0)
{
    for (ParamT element : init)
    {
        mBits |= Bit<BitsT>(element) & Mask(N);
    }
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT>::BitSetT(const BitSetT &other) : mBits(other.mBits)
{}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator=(const BitSetT &other)
{
    mBits = other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator==(const BitSetT &other) const
{
    return mBits == other.mBits;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator!=(const BitSetT &other) const
{
    return mBits != other.mBits;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::operator[](ParamT pos) const
{
    return test(pos);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::test(ParamT pos) const
{
    return (mBits & Bit<BitsT>(pos)) != 0;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::all() const
{
    ASSERT(mBits == (mBits & Mask(N)));
    return mBits == Mask(N);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::any() const
{
    ASSERT(mBits == (mBits & Mask(N)));
    return (mBits != 0);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr bool BitSetT<N, BitsT, ParamT>::none() const
{
    ASSERT(mBits == (mBits & Mask(N)));
    return (mBits == 0);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr std::size_t BitSetT<N, BitsT, ParamT>::count() const
{
    return gl::BitCount(mBits);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator&=(const BitSetT &other)
{
    mBits &= other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator|=(const BitSetT &other)
{
    mBits |= other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator^=(const BitSetT &other)
{
    mBits = mBits ^ other.mBits;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator~() const
{
    return BitSetT<N, BitsT, ParamT>(~mBits & Mask(N));
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator&=(BitsT value)
{
    mBits &= value;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator|=(BitsT value)
{
    mBits |= value & Mask(N);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator^=(BitsT value)
{
    mBits ^= value & Mask(N);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator<<(std::size_t pos) const
{
    return BitSetT<N, BitsT, ParamT>((mBits << pos) & Mask(N));
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator<<=(std::size_t pos)
{
    mBits = (mBits << pos & Mask(N));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> BitSetT<N, BitsT, ParamT>::operator>>(std::size_t pos) const
{
    return BitSetT<N, BitsT, ParamT>(mBits >> pos);
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::operator>>=(std::size_t pos)
{
    mBits = ((mBits >> pos) & Mask(N));
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::set()
{
    ASSERT(mBits == (mBits & Mask(N)));
    mBits = Mask(N);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::set(ParamT pos, bool value)
{
    ASSERT(mBits == (mBits & Mask(N)));
    if (value)
    {
        mBits |= Bit<BitsT>(pos) & Mask(N);
    }
    else
    {
        reset(pos);
    }
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::reset()
{
    ASSERT(mBits == (mBits & Mask(N)));
    mBits = 0;
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::reset(ParamT pos)
{
    ASSERT(mBits == (mBits & Mask(N)));
    mBits &= ~Bit<BitsT>(pos);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::flip()
{
    ASSERT(mBits == (mBits & Mask(N)));
    mBits ^= Mask(N);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr BitSetT<N, BitsT, ParamT> &BitSetT<N, BitsT, ParamT>::flip(ParamT pos)
{
    ASSERT(mBits == (mBits & Mask(N)));
    mBits ^= Bit<BitsT>(pos) & Mask(N);
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
constexpr ParamT BitSetT<N, BitsT, ParamT>::first() const
{
    ASSERT(!none());
    return static_cast<ParamT>(gl::ScanForward(mBits));
}

template <size_t N, typename BitsT, typename ParamT>
constexpr ParamT BitSetT<N, BitsT, ParamT>::last() const
{
    ASSERT(!none());
    return static_cast<ParamT>(gl::ScanReverse(mBits));
}

template <size_t N, typename BitsT, typename ParamT>
BitSetT<N, BitsT, ParamT>::Iterator::Iterator(const BitSetT &bits) : mBitsCopy(bits), mCurrentBit(0)
{
    if (bits.any())
    {
        mCurrentBit = getNextBit();
    }
}

template <size_t N, typename BitsT, typename ParamT>
ANGLE_INLINE typename BitSetT<N, BitsT, ParamT>::Iterator &
BitSetT<N, BitsT, ParamT>::Iterator::operator++()
{
    ASSERT(mBitsCopy.any());
    mBitsCopy.reset(static_cast<ParamT>(mCurrentBit));
    mCurrentBit = getNextBit();
    return *this;
}

template <size_t N, typename BitsT, typename ParamT>
bool BitSetT<N, BitsT, ParamT>::Iterator::operator==(const Iterator &other) const
{
    return mBitsCopy == other.mBitsCopy;
}

template <size_t N, typename BitsT, typename ParamT>
bool BitSetT<N, BitsT, ParamT>::Iterator::operator!=(const Iterator &other) const
{
    return !(*this == other);
}

template <size_t N, typename BitsT, typename ParamT>
ParamT BitSetT<N, BitsT, ParamT>::Iterator::operator*() const
{
    return static_cast<ParamT>(mCurrentBit);
}

template <size_t N, typename BitsT, typename ParamT>
std::size_t BitSetT<N, BitsT, ParamT>::Iterator::getNextBit()
{
    if (mBitsCopy.none())
    {
        return 0;
    }

    return gl::ScanForward(mBitsCopy.mBits);
}

template <size_t N>
using BitSet8 = BitSetT<N, uint8_t>;

template <size_t N>
using BitSet16 = BitSetT<N, uint16_t>;

template <size_t N>
using BitSet32 = BitSetT<N, uint32_t>;

template <size_t N>
using BitSet64 = BitSetT<N, uint64_t>;

namespace priv
{

template <size_t N, typename T>
using EnableIfBitsFit = typename std::enable_if<N <= sizeof(T) * 8>::type;

template <size_t N, typename Enable = void>
struct GetBitSet
{
    using Type = IterableBitSet<N>;
};

// Prefer 64-bit bitsets on 64-bit CPUs. They seem faster than 32-bit.
#if defined(ANGLE_IS_64_BIT_CPU)
template <size_t N>
struct GetBitSet<N, EnableIfBitsFit<N, uint64_t>>
{
    using Type = BitSet64<N>;
};
constexpr std::size_t kDefaultBitSetSize = 64;
using BaseBitSetType                     = BitSet64<kDefaultBitSetSize>;
#else
template <size_t N>
struct GetBitSet<N, EnableIfBitsFit<N, uint32_t>>
{
    using Type = BitSet32<N>;
};
constexpr std::size_t kDefaultBitSetSize = 32;
using BaseBitSetType                     = BitSet32<kDefaultBitSetSize>;
#endif  // defined(ANGLE_IS_64_BIT_CPU)

}  // namespace priv

template <size_t N>
using BitSet = typename priv::GetBitSet<N>::Type;

template <std::size_t N>
class BitSetArray final
{
  private:
    static constexpr std::size_t kDefaultBitSetSizeMinusOne = priv::kDefaultBitSetSize - 1;
    static constexpr std::size_t kShiftForDivision =
        static_cast<std::size_t>(rx::Log2(static_cast<unsigned int>(priv::kDefaultBitSetSize)));
    static constexpr std::size_t kArraySize =
        ((N + kDefaultBitSetSizeMinusOne) >> kShiftForDivision);
    constexpr static std::size_t kLastElementCount = (N & kDefaultBitSetSizeMinusOne);
    constexpr static std::size_t kLastElementMask  = priv::BaseBitSetType::Mask(
        kLastElementCount == 0 ? priv::kDefaultBitSetSize : kLastElementCount);

    using BaseBitSet = priv::BaseBitSetType;
    std::array<BaseBitSet, kArraySize> mBaseBitSetArray;

  public:
    BitSetArray();
    BitSetArray(const BitSetArray<N> &other);

    class Reference final
    {
      public:
        ~Reference() {}
        Reference &operator=(bool x)
        {
            mParent.set(mPosition, x);
            return *this;
        }
        explicit operator bool() const { return mParent.test(mPosition); }

      private:
        friend class BitSetArray;

        Reference(BitSetArray &parent, std::size_t pos) : mParent(parent), mPosition(pos) {}

        BitSetArray &mParent;
        std::size_t mPosition;
    };
    class Iterator final
    {
      public:
        Iterator(const BitSetArray<N> &bitSetArray, std::size_t index);
        Iterator &operator++();
        bool operator==(const Iterator &other) const;
        bool operator!=(const Iterator &other) const;
        size_t operator*() const;

      private:
        const BitSetArray &mParent;
        size_t mIndex;
        typename BaseBitSet::Iterator mCurrentIterator;
    };

    constexpr std::size_t size() const { return N; }
    Iterator begin() const { return Iterator(*this, 0); }
    Iterator end() const { return Iterator(*this, kArraySize); }
    unsigned long to_ulong() const
    {
        // TODO(anglebug.com/5628): Handle serializing more than kDefaultBitSetSize
        for (std::size_t index = 1; index < kArraySize; index++)
        {
            ASSERT(mBaseBitSetArray[index].none());
        }
        return static_cast<unsigned long>(mBaseBitSetArray[0].to_ulong());
    }

    BitSetArray &operator=(const BitSetArray &other);
    bool operator[](std::size_t pos) const;
    Reference operator[](std::size_t pos)
    {
        ASSERT(pos < size());
        return Reference(*this, pos);
    }

    BitSetArray &set(std::size_t pos, bool value = true);
    BitSetArray &reset();
    BitSetArray &reset(std::size_t pos);
    bool test(std::size_t pos) const;
    bool all() const;
    bool any() const;
    bool none() const;
    std::size_t count() const;
    bool intersects(const BitSetArray &other) const;
    BitSetArray<N> &flip();
};

template <std::size_t N>
BitSetArray<N>::BitSetArray()
{
    static_assert(N > priv::kDefaultBitSetSize, "BitSetArray type can't support requested size.");
    reset();
}

template <size_t N>
BitSetArray<N>::BitSetArray(const BitSetArray<N> &other)
{
    for (std::size_t index = 0; index < kArraySize; index++)
    {
        mBaseBitSetArray[index] = other.mBaseBitSetArray[index];
    }
}

template <size_t N>
BitSetArray<N>::Iterator::Iterator(const BitSetArray<N> &bitSetArray, std::size_t index)
    : mParent(bitSetArray), mIndex(index), mCurrentIterator(mParent.mBaseBitSetArray[0].begin())
{
    while (mIndex < mParent.kArraySize)
    {
        if (mParent.mBaseBitSetArray[mIndex].any())
        {
            break;
        }
        mIndex++;
    }

    if (mIndex < mParent.kArraySize)
    {
        mCurrentIterator = mParent.mBaseBitSetArray[mIndex].begin();
    }
    else
    {
        mCurrentIterator = mParent.mBaseBitSetArray[mParent.kArraySize - 1].end();
    }
}

template <std::size_t N>
typename BitSetArray<N>::Iterator &BitSetArray<N>::Iterator::operator++()
{
    ++mCurrentIterator;
    if (mCurrentIterator == mParent.mBaseBitSetArray[mIndex].end())
    {
        mIndex++;
        if (mIndex < mParent.kArraySize)
        {
            mCurrentIterator = mParent.mBaseBitSetArray[mIndex].begin();
        }
    }
    return *this;
}

template <std::size_t N>
bool BitSetArray<N>::Iterator::operator==(const BitSetArray<N>::Iterator &other) const
{
    return mCurrentIterator == other.mCurrentIterator;
}

template <std::size_t N>
bool BitSetArray<N>::Iterator::operator!=(const BitSetArray<N>::Iterator &other) const
{
    return mCurrentIterator != other.mCurrentIterator;
}

template <std::size_t N>
std::size_t BitSetArray<N>::Iterator::operator*() const
{
    return (mIndex * priv::kDefaultBitSetSize) + *mCurrentIterator;
}

template <std::size_t N>
BitSetArray<N> &BitSetArray<N>::operator=(const BitSetArray<N> &other)
{
    for (std::size_t index = 0; index < kArraySize; index++)
    {
        mBaseBitSetArray[index] = other.mBaseBitSetArray[index];
    }
    return *this;
}

template <std::size_t N>
bool BitSetArray<N>::operator[](std::size_t pos) const
{
    ASSERT(pos < size());
    return test(pos);
}

template <std::size_t N>
BitSetArray<N> &BitSetArray<N>::set(std::size_t pos, bool value)
{
    ASSERT(pos < size());
    // Get the index and offset, then set the bit
    size_t index  = pos >> kShiftForDivision;
    size_t offset = pos & kDefaultBitSetSizeMinusOne;
    mBaseBitSetArray[index].set(offset, value);
    return *this;
}

template <std::size_t N>
BitSetArray<N> &BitSetArray<N>::reset()
{
    for (BaseBitSet &baseBitSet : mBaseBitSetArray)
    {
        baseBitSet.reset();
    }
    return *this;
}

template <std::size_t N>
BitSetArray<N> &BitSetArray<N>::reset(std::size_t pos)
{
    ASSERT(pos < size());
    return set(pos, false);
}

template <std::size_t N>
bool BitSetArray<N>::test(std::size_t pos) const
{
    ASSERT(pos < size());
    // Get the index and offset, then test the bit
    size_t index  = pos >> kShiftForDivision;
    size_t offset = pos & kDefaultBitSetSizeMinusOne;
    return mBaseBitSetArray[index].test(offset);
}

template <std::size_t N>
bool BitSetArray<N>::all() const
{
    static priv::BaseBitSetType kLastElementBitSet = priv::BaseBitSetType(kLastElementMask);

    for (std::size_t index = 0; index < kArraySize - 1; index++)
    {
        if (!mBaseBitSetArray[index].all())
        {
            return false;
        }
    }

    // The last element in mBaseBitSetArray may need special handling
    return mBaseBitSetArray[kArraySize - 1] == kLastElementBitSet;
}

template <std::size_t N>
bool BitSetArray<N>::any() const
{
    for (const BaseBitSet &baseBitSet : mBaseBitSetArray)
    {
        if (baseBitSet.any())
        {
            return true;
        }
    }
    return false;
}

template <std::size_t N>
bool BitSetArray<N>::none() const
{
    for (const BaseBitSet &baseBitSet : mBaseBitSetArray)
    {
        if (!baseBitSet.none())
        {
            return false;
        }
    }
    return true;
}

template <std::size_t N>
std::size_t BitSetArray<N>::count() const
{
    size_t count = 0;
    for (const BaseBitSet &baseBitSet : mBaseBitSetArray)
    {
        count += baseBitSet.count();
    }
    return count;
}

template <std::size_t N>
bool BitSetArray<N>::intersects(const BitSetArray<N> &other) const
{
    for (std::size_t index = 0; index < kArraySize; index++)
    {
        if (mBaseBitSetArray[index].bits() & other.mBaseBitSetArray[index].bits())
        {
            return true;
        }
    }
    return false;
}

template <std::size_t N>
BitSetArray<N> &BitSetArray<N>::flip()
{
    for (BaseBitSet &baseBitSet : mBaseBitSetArray)
    {
        baseBitSet.flip();
    }

    // The last element in mBaseBitSetArray may need special handling
    mBaseBitSetArray[kArraySize - 1] &= kLastElementMask;
    return *this;
}
}  // namespace angle

template <size_t N, typename BitsT, typename ParamT>
inline constexpr angle::BitSetT<N, BitsT, ParamT> operator&(
    const angle::BitSetT<N, BitsT, ParamT> &lhs,
    const angle::BitSetT<N, BitsT, ParamT> &rhs)
{
    angle::BitSetT<N, BitsT, ParamT> result(lhs);
    result &= rhs.bits();
    return result;
}

template <size_t N, typename BitsT, typename ParamT>
inline constexpr angle::BitSetT<N, BitsT, ParamT> operator|(
    const angle::BitSetT<N, BitsT, ParamT> &lhs,
    const angle::BitSetT<N, BitsT, ParamT> &rhs)
{
    angle::BitSetT<N, BitsT, ParamT> result(lhs);
    result |= rhs.bits();
    return result;
}

template <size_t N, typename BitsT, typename ParamT>
inline constexpr angle::BitSetT<N, BitsT, ParamT> operator^(
    const angle::BitSetT<N, BitsT, ParamT> &lhs,
    const angle::BitSetT<N, BitsT, ParamT> &rhs)
{
    angle::BitSetT<N, BitsT, ParamT> result(lhs);
    result ^= rhs.bits();
    return result;
}

#endif  // COMMON_BITSETITERATOR_H_

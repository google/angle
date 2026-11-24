//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Decompress_unittest.cpp: Unit tests for the |(Compress/Decompress)*Blob| functions.

#include <gtest/gtest.h>
#include "common/span.h"
#include "common/unsafe_buffers.h"

#include "libANGLE/angletypes.h"

namespace angle
{
namespace
{
enum class Format
{
    Zlib,
    Zstd,
};

bool Compress(Format format, const std::vector<uint8_t> &data, MemoryBuffer *compressedOut)
{
    switch (format)
    {
        case Format::Zlib:
            return CompressBlob(data.size(), data.data(), compressedOut);
#if defined(ANGLE_HAS_ZSTD)
        case Format::Zstd:
            return CompressBlobZstd(data.size(), data.data(), compressedOut);
#endif
        default:
            UNREACHABLE();
            return false;
    }
}

bool Decompress(Format format,
                const MemoryBuffer &compressed,
                size_t compressedSize,
                size_t maxUncompressedDataSize,
                MemoryBuffer *uncompressedOut)
{
    switch (format)
    {
        case Format::Zlib:
            return DecompressBlob(compressed.data(), compressedSize, maxUncompressedDataSize,
                                  uncompressedOut);
#if defined(ANGLE_HAS_ZSTD)
        case Format::Zstd:
            return DecompressBlobZstd(compressed.data(), compressedSize, maxUncompressedDataSize,
                                      uncompressedOut);
#endif
        default:
            UNREACHABLE();
            return false;
    }
}

class DecompressTest : public ::testing::TestWithParam<Format>
{
  protected:
    void SetUp() override
    {
        constexpr size_t kTestDataSize = 100'000;

        mTestData.resize(kTestDataSize);
        for (size_t i = 0; i < kTestDataSize; ++i)
        {
            mTestData[i] = static_cast<uint8_t>(i);
        }

        ASSERT_TRUE(Compress(GetParam(), mTestData, &mCompressedData));
    }

    void setCompressedDataLastDWord(uint32_t value)
    {
        ASSERT(IsLittleEndian());
        ASSERT(mCompressedData.size() > sizeof(value));
        ANGLE_UNSAFE_TODO(memcpy(mCompressedData.data() + mCompressedData.size() - sizeof(value),
                                 &value, sizeof(value)));
    }

    bool decompress(size_t compressedSize, size_t maxUncompressedDataSize)
    {
        return Decompress(GetParam(), mCompressedData, compressedSize, maxUncompressedDataSize,
                          &mUncompressedData);
    }

    bool checkUncompressedData()
    {
        return (mTestData.size() == mUncompressedData.size()) &&
               (ANGLE_UNSAFE_TODO(
                    memcmp(mTestData.data(), mUncompressedData.data(), mTestData.size())) == 0);
    }

    std::vector<uint8_t> mTestData;
    MemoryBuffer mCompressedData;
    MemoryBuffer mUncompressedData;
};

// Tests that decompressing full data has no errors.
TEST_P(DecompressTest, FullData)
{
    EXPECT_TRUE(decompress(mCompressedData.size(), mTestData.size()));
    EXPECT_TRUE(checkUncompressedData());
}

// Tests expected failure if |maxUncompressedDataSize| is less than actual uncompressed size.
TEST_P(DecompressTest, InsufficientMaxUncompressedDataSize)
{
    EXPECT_FALSE(decompress(mCompressedData.size(), mTestData.size() - 1));
}

// Tests expected failure if try to decompress partial compressed data.
TEST_P(DecompressTest, UnexpectedPartialData)
{
    // Use this to avoid |maxUncompressedDataSize| affecting the test.
    constexpr size_t kMaxUncompressedDataSize = std::numeric_limits<size_t>::max();

    EXPECT_FALSE(decompress(mCompressedData.size() - 1, kMaxUncompressedDataSize));
}

// Tests expected failure if try to decompress corrupted data.
TEST_P(DecompressTest, CorruptedData)
{
    // Corrupt the compressed data.
    const size_t corruptIndex = mCompressedData.size() / 2;
    mCompressedData[corruptIndex] ^= 255;

    EXPECT_FALSE(decompress(mCompressedData.size(), mTestData.size()));
}

// Tests expected failures if try to decompress data with the corrupted last dword.
TEST_P(DecompressTest, CorruptedLastDWord)
{
    // This is zlib's trailer.  zstd ends with its checksum, which CorruptedData covers.
    if (GetParam() != Format::Zlib)
    {
        GTEST_SKIP();
    }

    // Last dword stores decompressed data size and not the actual compressed data. This dword must
    // match the decompressed size. Decompress should fail if it is not the case.

    // Use this to avoid |maxUncompressedDataSize| affecting the test.
    constexpr size_t kMaxUncompressedDataSize = std::numeric_limits<size_t>::max();

    // Try to decompress with decreased size in the last dword.
    setCompressedDataLastDWord(static_cast<uint32_t>(mTestData.size() - 1));
    EXPECT_FALSE(decompress(mCompressedData.size(), kMaxUncompressedDataSize));

    // Try to decompress with increased size in the last dword.
    setCompressedDataLastDWord(static_cast<uint32_t>(mTestData.size() + 1));
    EXPECT_FALSE(decompress(mCompressedData.size(), kMaxUncompressedDataSize));

    // Try to decompress with the last dword set to 0.
    setCompressedDataLastDWord(0);
    EXPECT_FALSE(decompress(mCompressedData.size(), kMaxUncompressedDataSize));

    // Decompress with the last dword set to the correct size should succeed.
    setCompressedDataLastDWord(static_cast<uint32_t>(mTestData.size()));
    EXPECT_TRUE(decompress(mCompressedData.size(), kMaxUncompressedDataSize));
    EXPECT_TRUE(checkUncompressedData());
}

std::string FormatName(const ::testing::TestParamInfo<Format> &info)
{
    switch (info.param)
    {
        case Format::Zlib:
            return "Zlib";
        case Format::Zstd:
            return "Zstd";
        default:
            UNREACHABLE();
            return "";
    }
}

#if defined(ANGLE_HAS_ZSTD)
const auto kFormats = ::testing::Values(Format::Zlib, Format::Zstd);
#else
const auto kFormats = ::testing::Values(Format::Zlib);
#endif

INSTANTIATE_TEST_SUITE_P(Formats, DecompressTest, kFormats, FormatName);

class StreamingDecompressTest : public ::testing::Test
{
  protected:
    void SetUp() override
    {
        constexpr size_t kTestDataSize = 100'000;

        mTestData.resize(kTestDataSize);
        for (size_t i = 0; i < kTestDataSize; ++i)
        {
            mTestData[i] = static_cast<uint8_t>((i * 131 + (i >> 8)) & 0xFF);
        }

        ASSERT_TRUE(CompressBlob(mTestData.size(), mTestData.data(), &mCompressedData));
    }

    bool decompressInChunks(Span<const uint8_t> data,
                            size_t chunkSize,
                            size_t maxUncompressedDataSize,
                            bool interleaveEmptyChunks = false)
    {
        StreamingBlobDecompressor decompressor;
        if (!decompressor.init(maxUncompressedDataSize))
        {
            return false;
        }

        size_t offset = 0;
        while (offset < data.size())
        {
            if (interleaveEmptyChunks)
            {
                if (!decompressor.decompressChunk(data.subspan(offset, 0)))
                {
                    return false;
                }
            }
            const size_t len = std::min(chunkSize, data.size() - offset);
            if (!decompressor.decompressChunk(data.subspan(offset, len)))
            {
                return false;
            }
            offset += len;
        }

        if (interleaveEmptyChunks && !decompressor.decompressChunk({}))
        {
            return false;
        }

        return decompressor.finish(&mUncompressedData);
    }

    bool checkUncompressedData() const
    {
        return Span<const uint8_t>(mTestData) == mUncompressedData.span();
    }

    std::vector<uint8_t> mTestData;
    MemoryBuffer mCompressedData;
    MemoryBuffer mUncompressedData;
};

// Tests streaming decompression across single-chunk and multi-chunk boundaries, including 1-byte
// chunks that split the gzip header and trailer, with exact and oversized output capacities.
TEST_F(StreamingDecompressTest, ChunkedDecompression)
{
    const size_t chunkSizes[] = {mCompressedData.size(), 1000, 64, 7, 1};
    for (size_t chunkSize : chunkSizes)
    {
        EXPECT_TRUE(decompressInChunks(mCompressedData.span(), chunkSize, mTestData.size(),
                                       /*interleaveEmptyChunks=*/true))
            << "chunkSize=" << chunkSize;
        EXPECT_TRUE(checkUncompressedData()) << "chunkSize=" << chunkSize;

        EXPECT_TRUE(decompressInChunks(mCompressedData.span(), chunkSize, mTestData.size() + 1024))
            << "chunkSize=" << chunkSize;
        EXPECT_TRUE(checkUncompressedData()) << "chunkSize=" << chunkSize;
    }
}

// Tests that streaming decompression fails if maxUncompressedDataSize is too small.
TEST_F(StreamingDecompressTest, InsufficientMaxUncompressedDataSize)
{
    EXPECT_FALSE(decompressInChunks(mCompressedData.span(), 64, mTestData.size() - 1));
    EXPECT_FALSE(
        decompressInChunks(mCompressedData.span(), mCompressedData.size(), mTestData.size() - 1));
    EXPECT_FALSE(decompressInChunks(mCompressedData.span(), 64, 0));
}

// Tests that streaming decompression fails on truncated data.
TEST_F(StreamingDecompressTest, UnexpectedPartialData)
{
    const Span<const uint8_t> truncated = mCompressedData.span().first(mCompressedData.size() - 1);
    EXPECT_FALSE(decompressInChunks(truncated, 64, mTestData.size()));
    EXPECT_FALSE(decompressInChunks(truncated, 1, mTestData.size()));
}

// Tests that streaming decompression fails on corrupted data, CRC32 trailer, and ISIZE trailer.
TEST_F(StreamingDecompressTest, CorruptedDataAndTrailer)
{
    std::vector<uint8_t> corrupted(mCompressedData.size());
    Span(corrupted).copy_from(mCompressedData.span());
    corrupted[corrupted.size() / 2] ^= 0xFF;
    EXPECT_FALSE(decompressInChunks(corrupted, 64, mTestData.size()));

    // Corrupt the gzip CRC32 trailer (4 bytes preceding ISIZE).
    Span(corrupted).copy_from(mCompressedData.span());
    corrupted[corrupted.size() - 5] ^= 0xFF;
    EXPECT_FALSE(decompressInChunks(corrupted, 7, mTestData.size()));

    // Corrupt the last dword (gzip ISIZE trailer).
    Span(corrupted).copy_from(mCompressedData.span());
    const uint32_t badSize = static_cast<uint32_t>(mTestData.size() - 1);
    Span(corrupted).last(sizeof(badSize)).copy_from(byte_span_from_ref(badSize));
    EXPECT_FALSE(decompressInChunks(corrupted, 7, mTestData.size()));
}

// Tests that trailing bytes after a complete gzip stream are rejected.
TEST_F(StreamingDecompressTest, TrailingDataRejected)
{
    std::vector<uint8_t> withTrailing(mCompressedData.size() + 1, 0x42);
    Span(withTrailing).copy_prefix_from(mCompressedData.span());

    // Trailing byte in the same chunk.
    EXPECT_FALSE(decompressInChunks(withTrailing, withTrailing.size(), mTestData.size()));

    // Trailing byte in a separate chunk after Z_STREAM_END.
    StreamingBlobDecompressor decompressor;
    ASSERT_TRUE(decompressor.init(mTestData.size()));
    ASSERT_TRUE(decompressor.decompressChunk(mCompressedData.span()));
    const uint8_t extraByte = 0x42;
    EXPECT_FALSE(decompressor.decompressChunk(byte_span_from_ref(extraByte)));
}

// Tests that a valid 0-byte gzip stream is rejected by finish() and leaves output unchanged.
TEST_F(StreamingDecompressTest, EmptyGzipStreamRejected)
{
    MemoryBuffer emptyCompressed;
    ASSERT_TRUE(CompressBlob(0, nullptr, &emptyCompressed));
    ASSERT_GT(emptyCompressed.size(), 0u);

    MemoryBuffer output;
    ASSERT_TRUE(output.resize(4));
    output.fill(0xAB);

    StreamingBlobDecompressor decompressor;
    ASSERT_TRUE(decompressor.init(64));
    ASSERT_TRUE(decompressor.decompressChunk(emptyCompressed.span()));
    EXPECT_FALSE(decompressor.finish(&output));

    const uint8_t expected[4] = {0xAB, 0xAB, 0xAB, 0xAB};
    EXPECT_EQ(output.span(), Span(expected));
}

// Tests that finish() failure on an incomplete stream leaves the output buffer unchanged.
TEST_F(StreamingDecompressTest, OutputUnchangedOnFinishFailure)
{
    MemoryBuffer output;
    ASSERT_TRUE(output.resize(4));
    output.fill(0xCD);

    StreamingBlobDecompressor decompressor;
    ASSERT_TRUE(decompressor.init(mTestData.size()));
    ASSERT_TRUE(decompressor.decompressChunk(mCompressedData.span().first(16)));
    EXPECT_FALSE(decompressor.finish(&output));

    const uint8_t expected[4] = {0xCD, 0xCD, 0xCD, 0xCD};
    EXPECT_EQ(output.span(), Span(expected));
}

// Tests that init() resets prior state and allows reusing a decompressor after finish or failure.
TEST_F(StreamingDecompressTest, ReuseViaInit)
{
    StreamingBlobDecompressor decompressor;

    // Complete a first decompression.
    ASSERT_TRUE(decompressor.init(mTestData.size()));
    ASSERT_TRUE(decompressor.decompressChunk(mCompressedData.span()));
    ASSERT_TRUE(decompressor.finish(&mUncompressedData));
    EXPECT_TRUE(checkUncompressedData());

    // Fail mid-stream, then re-initialize and succeed.
    std::vector<uint8_t> corrupted(mCompressedData.size());
    Span(corrupted).copy_from(mCompressedData.span());
    corrupted[corrupted.size() / 2] ^= 0xFF;
    ASSERT_TRUE(decompressor.init(mTestData.size()));
    EXPECT_FALSE(decompressor.decompressChunk(corrupted));

    mUncompressedData.clear();
    ASSERT_TRUE(decompressor.init(mTestData.size()));
    ASSERT_TRUE(decompressor.decompressChunk(mCompressedData.span()));
    ASSERT_TRUE(decompressor.finish(&mUncompressedData));
    EXPECT_TRUE(checkUncompressedData());
}

}  // anonymous namespace
}  // namespace angle

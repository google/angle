//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_pipeline_binary_cache_unittest.cpp:
//   Unit tests for how PipelineBinariesCache stores one pipeline's binaries: the entry format,
//   malformed entries, and blob keys.  They use made up keys and binaries, and need no device.
//

#include <gtest/gtest.h>

#include "libANGLE/renderer/vulkan/vk_pipeline_binary_cache.h"

namespace rx
{
namespace vk
{
namespace
{
// Made up driver keys, distinct for each |value|.
VkPipelineBinaryKeyKHR MakeTestKey(uint32_t value, uint32_t keySize)
{
    VkPipelineBinaryKeyKHR key = {};
    key.sType                  = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
    key.keySize                = keySize;
    uint32_t index             = 0;
    for (uint8_t &byte : angle::Span(key.key).first(keySize))
    {
        byte = static_cast<uint8_t>((value >> (8 * (index % 4))) + index);
        ++index;
    }
    return key;
}

// Global, pipeline and binary keys of different sizes, so that a size lost in storage shows up.
// Round trips also use keys of the maximum size.
VkPipelineBinaryKeyKHR MakeTestGlobalKey(uint32_t value)
{
    return MakeTestKey(value, 16);
}

VkPipelineBinaryKeyKHR MakeTestPipelineKey(uint32_t value)
{
    return MakeTestKey(value, 20);
}

VkPipelineBinaryKeyKHR MakeTestBinaryKey(uint32_t value)
{
    return MakeTestKey(value, 28);
}

VkPipelineBinaryKeyKHR MakeTestMaxSizeKey(uint32_t value)
{
    return MakeTestKey(value, VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR);
}

// Made up binary data, distinct for each |value|.  It comes in runs, so that it compresses well.
std::vector<uint8_t> MakeTestBinary(uint32_t value, size_t size)
{
    std::vector<uint8_t> binary(size);
    for (size_t index = 0; index < size; ++index)
    {
        binary[index] = static_cast<uint8_t>(value * 31 + index / 64);
    }
    return binary;
}

// A set of one binary, for |pipelineKey|.
PipelineBinarySet MakeTestBinarySet(const VkPipelineBinaryKeyKHR &pipelineKey)
{
    PipelineBinarySet binarySet;
    binarySet.pipelineKey = pipelineKey;
    binarySet.binaryKeys.push_back(MakeTestBinaryKey(1));
    binarySet.binaries.push_back(MakeTestBinary(1, 256));
    return binarySet;
}

void ExpectSameBinarySet(const PipelineBinarySet &expected, const PipelineBinarySet &actual)
{
    EXPECT_TRUE(PipelineBinaryKeysEqual(actual.pipelineKey, expected.pipelineKey));
    ASSERT_EQ(actual.binaryKeys.size(), expected.binaryKeys.size());
    ASSERT_EQ(actual.binaries.size(), expected.binaries.size());
    for (size_t index = 0; index < expected.binaries.size(); ++index)
    {
        EXPECT_TRUE(PipelineBinaryKeysEqual(actual.binaryKeys[index], expected.binaryKeys[index]))
            << "binary key " << index;
        // Not EXPECT_EQ, which would print every byte of a binary that differs.
        EXPECT_EQ(actual.binaries[index].size(), expected.binaries[index].size())
            << "binary " << index;
        EXPECT_TRUE(actual.binaries[index] == expected.binaries[index]) << "binary " << index;
    }
}

// Writes |value| over the uint32 at |offset| of |serialized|.
void OverwriteUint32(std::vector<uint8_t> *serialized, size_t offset, uint32_t value)
{
    angle::Span(*serialized)
        .subspan(offset, sizeof(value))
        .copy_from(angle::byte_span_from_ref(value));
}

// Where a serialized set's format version, global key size and pipeline key size are.  Keys are
// stored as their size, then the whole key array.
constexpr size_t kFormatVersionOffset = 0;
constexpr size_t kGlobalKeySizeOffset = kFormatVersionOffset + sizeof(uint32_t);
constexpr size_t kPipelineKeySizeOffset =
    kGlobalKeySizeOffset + sizeof(uint32_t) + VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR;

// An entry cut short is rejected rather than read past its end: one emptied, and one missing its
// last byte.
TEST(PipelineBinariesCacheTest, CorruptBinaryFileIsRejected)
{
    const std::vector<uint8_t> rawData(256, 0x42);
    angle::MemoryBuffer entry;
    ASSERT_TRUE(PipelineBinariesCache::CompressEntry(rawData, &entry));

    angle::MemoryBuffer payload;
    ASSERT_TRUE(PipelineBinariesCache::DecompressEntry(entry.span(), &payload));
    EXPECT_TRUE(payload.span() == angle::Span<const uint8_t>(rawData));

    EXPECT_FALSE(PipelineBinariesCache::DecompressEntry({}, &payload));
    EXPECT_FALSE(PipelineBinariesCache::DecompressEntry(entry.first(entry.size() - 1), &payload));
}

// An entry with garbage written over the codec's magic number is rejected.
TEST(PipelineBinariesCacheTest, GarbageBinaryFileIsRejected)
{
    const std::vector<uint8_t> rawData(256, 0x42);
    angle::MemoryBuffer entry;
    ASSERT_TRUE(PipelineBinariesCache::CompressEntry(rawData, &entry));

    angle::MemoryBuffer payload;
    ASSERT_TRUE(PipelineBinariesCache::DecompressEntry(entry.span(), &payload));

    entry.span()[0] = 'G';
    entry.span()[1] = 'A';
    EXPECT_FALSE(PipelineBinariesCache::DecompressEntry(entry.span(), &payload));
}

// CompressEntry() refuses a payload larger than the platform's blob cache stores, and one that
// doesn't compress and so would make an entry over the limit.
TEST(PipelineBinariesCacheTest, OversizedPayloadIsNotCompressed)
{
    constexpr size_t kLimit = PipelineBinariesCache::kMaxBlobSize;
    angle::MemoryBuffer entry;

    // Compresses to almost nothing, so only the payload's own size is over.
    EXPECT_FALSE(
        PipelineBinariesCache::CompressEntry(std::vector<uint8_t>(kLimit + 1, 0x42), &entry));
    EXPECT_TRUE(PipelineBinariesCache::CompressEntry(std::vector<uint8_t>(kLimit, 0x42), &entry));
    EXPECT_LE(entry.size(), kLimit);

    // At the limit but incompressible, so the entry is a little larger than the payload.
    std::vector<uint8_t> noise(kLimit);
    uint32_t state = 1;
    for (uint8_t &byte : noise)
    {
        state = state * 1664525u + 1013904223u;
        byte  = static_cast<uint8_t>(state >> 24);
    }
    EXPECT_FALSE(PipelineBinariesCache::CompressEntry(noise, &entry));
    EXPECT_EQ(entry.size(), 0u);
}

// A set comes back exactly as it was stored: the pipeline key, and each binary with its key, in
// order.  Keys of the maximum size come back too.
TEST(PipelineBinariesCacheTest, BinarySetRoundTrips)
{
    // Two binaries, as a driver may make for a pipeline with more than one stage, one with a key
    // of the maximum size.
    PipelineBinarySet twoBinaries;
    twoBinaries.pipelineKey = MakeTestPipelineKey(3);
    twoBinaries.binaryKeys.push_back(MakeTestBinaryKey(30));
    twoBinaries.binaries.push_back(MakeTestBinary(30, 1000));
    twoBinaries.binaryKeys.push_back(MakeTestMaxSizeKey(31));
    twoBinaries.binaries.push_back(MakeTestBinary(31, 1001));

    // Some drivers give all the binaries of a pipeline the same binary key.
    PipelineBinarySet sameKeyBinaries;
    sameKeyBinaries.pipelineKey = MakeTestPipelineKey(9);
    for (uint32_t index = 0; index < 5; ++index)
    {
        sameKeyBinaries.binaryKeys.push_back(MakeTestBinaryKey(9));
        sameKeyBinaries.binaries.push_back(MakeTestBinary(90 + index, 500));
    }

    // A pipeline key of the maximum size.
    PipelineBinarySet maxSizePipelineKey;
    maxSizePipelineKey.pipelineKey = MakeTestMaxSizeKey(7);
    maxSizePipelineKey.binaryKeys.push_back(MakeTestBinaryKey(70));
    maxSizePipelineKey.binaries.push_back(MakeTestBinary(70, 700));

    // Each set is stored under a global key of the maximum size too.
    for (const VkPipelineBinaryKeyKHR &globalKey :
         {MakeTestGlobalKey(100), MakeTestMaxSizeKey(100)})
    {
        for (const PipelineBinarySet *binarySet :
             {&twoBinaries, &sameKeyBinaries, &maxSizePipelineKey})
        {
            std::vector<uint8_t> serialized;
            PipelineBinariesCache::SerializeBinarySet(globalKey, *binarySet, &serialized);
            angle::MemoryBuffer entry;
            ASSERT_TRUE(PipelineBinariesCache::CompressEntry(serialized, &entry));
            angle::MemoryBuffer payload;
            ASSERT_TRUE(PipelineBinariesCache::DecompressEntry(entry.span(), &payload));
            PipelineBinarySet loaded;
            ASSERT_TRUE(PipelineBinariesCache::DeserializeBinarySet(
                payload.span(), globalKey, binarySet->pipelineKey, &loaded));
            ExpectSameBinarySet(*binarySet, loaded);
        }
    }
}

// A set that does not parse is rejected rather than handed to the driver: cut short, with a
// trailing byte, with no binaries, or with a key larger than the driver's maximum.
TEST(PipelineBinariesCacheTest, MalformedBinarySetIsRejected)
{
    const VkPipelineBinaryKeyKHR globalKey   = MakeTestGlobalKey(100);
    const VkPipelineBinaryKeyKHR pipelineKey = MakeTestPipelineKey(1);
    std::vector<uint8_t> serialized;
    PipelineBinariesCache::SerializeBinarySet(globalKey, MakeTestBinarySet(pipelineKey),
                                              &serialized);

    PipelineBinarySet loaded;
    ASSERT_TRUE(
        PipelineBinariesCache::DeserializeBinarySet(serialized, globalKey, pipelineKey, &loaded));

    std::vector<uint8_t> truncated(serialized.begin(), serialized.end() - 1);
    EXPECT_FALSE(
        PipelineBinariesCache::DeserializeBinarySet(truncated, globalKey, pipelineKey, &loaded));

    std::vector<uint8_t> trailing = serialized;
    trailing.push_back(0);
    EXPECT_FALSE(
        PipelineBinariesCache::DeserializeBinarySet(trailing, globalKey, pipelineKey, &loaded));

    PipelineBinarySet empty;
    empty.pipelineKey = pipelineKey;
    std::vector<uint8_t> noBinaries;
    PipelineBinariesCache::SerializeBinarySet(globalKey, empty, &noBinaries);
    EXPECT_FALSE(
        PipelineBinariesCache::DeserializeBinarySet(noBinaries, globalKey, pipelineKey, &loaded));

    for (size_t keySizeOffset : {kGlobalKeySizeOffset, kPipelineKeySizeOffset})
    {
        std::vector<uint8_t> oversizedKey = serialized;
        OverwriteUint32(&oversizedKey, keySizeOffset, VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR + 1);
        EXPECT_FALSE(PipelineBinariesCache::DeserializeBinarySet(oversizedKey, globalKey,
                                                                 pipelineKey, &loaded))
            << "key size at " << keySizeOffset;
    }
}

// An entry is only used for what it was stored for: its format version, global key and pipeline
// key are checked, so one found under a colliding blob key is rejected.
TEST(PipelineBinariesCacheTest, BinarySetForOtherKeysIsRejected)
{
    const VkPipelineBinaryKeyKHR globalKey   = MakeTestGlobalKey(100);
    const VkPipelineBinaryKeyKHR pipelineKey = MakeTestPipelineKey(1);
    std::vector<uint8_t> serialized;
    PipelineBinariesCache::SerializeBinarySet(globalKey, MakeTestBinarySet(pipelineKey),
                                              &serialized);

    PipelineBinarySet loaded;
    ASSERT_TRUE(
        PipelineBinariesCache::DeserializeBinarySet(serialized, globalKey, pipelineKey, &loaded));

    EXPECT_FALSE(PipelineBinariesCache::DeserializeBinarySet(serialized, MakeTestGlobalKey(101),
                                                             pipelineKey, &loaded));
    EXPECT_FALSE(PipelineBinariesCache::DeserializeBinarySet(serialized, globalKey,
                                                             MakeTestPipelineKey(2), &loaded));

    // Another format version.
    uint32_t formatVersion = 0;
    angle::byte_span_from_ref(formatVersion)
        .copy_from(angle::Span(serialized).subspan(kFormatVersionOffset, sizeof(formatVersion)));
    std::vector<uint8_t> otherFormat = serialized;
    OverwriteUint32(&otherFormat, kFormatVersionOffset, formatVersion + 1);
    EXPECT_FALSE(
        PipelineBinariesCache::DeserializeBinarySet(otherFormat, globalKey, pipelineKey, &loaded));
}

// Binaries are stored under a key of the pipeline key and the driver's global key: other
// pipelines, and other drivers, look elsewhere.
TEST(PipelineBinariesCacheTest, BlobKeyDependsOnPipelineAndGlobalKeys)
{
    PipelineBinariesCache cache;

    cache.setGlobalKeyForTesting(MakeTestGlobalKey(100));
    const angle::BlobCacheKey key1 = cache.getBlobKey(MakeTestPipelineKey(1));
    EXPECT_EQ(cache.getBlobKey(MakeTestPipelineKey(1)), key1);
    EXPECT_NE(cache.getBlobKey(MakeTestPipelineKey(2)), key1);

    cache.setGlobalKeyForTesting(MakeTestGlobalKey(101));
    EXPECT_NE(cache.getBlobKey(MakeTestPipelineKey(1)), key1);

    // Back to the first driver, the first key again.
    cache.setGlobalKeyForTesting(MakeTestGlobalKey(100));
    EXPECT_EQ(cache.getBlobKey(MakeTestPipelineKey(1)), key1);
}
}  // anonymous namespace
}  // namespace vk
}  // namespace rx

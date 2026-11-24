//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BlobCache_unittest.h: Unit tests for the blob cache.

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <vector>

#include "common/span.h"
#include "common/unsafe_buffers.h"
#include "libANGLE/BlobCache.h"

namespace egl
{

// Note: this is fairly similar to SizedMRUCache_unittest, and makes sure the
// BlobCache usage of SizedMRUCache is not broken.

using BlobPut = angle::MemoryBuffer;
using Blob    = BlobCache::Value;
using Key     = BlobCache::Key;

template <typename T>
void MakeSequence(T &seq, uint8_t start)
{
    for (size_t i = 0; i < seq.size(); ++i)
    {
        seq[i] = static_cast<uint8_t>(i + start);
    }
}

BlobPut MakeBlob(size_t size, uint8_t start = 0)
{
    BlobPut blob;
    EXPECT_TRUE(blob.resize(size));
    MakeSequence(blob, start);
    return blob;
}

Key MakeKey(uint8_t start = 0)
{
    Key key;
    MakeSequence(key, start);
    return key;
}

// Test a cache with a value that takes up maximum size.
TEST(BlobCacheTest, MaxSizedValue)
{
    constexpr size_t kSize = 32;
    BlobCache blobCache(kSize);
    angle::ScratchBuffer scratchBuffer;

    blobCache.populate(MakeKey(0), MakeBlob(kSize));
    EXPECT_EQ(32u, blobCache.size());
    EXPECT_FALSE(blobCache.empty());

    blobCache.populate(MakeKey(1), MakeBlob(kSize));
    EXPECT_EQ(32u, blobCache.size());
    EXPECT_FALSE(blobCache.empty());

    Blob blob;
    EXPECT_FALSE(blobCache.get(nullptr, &scratchBuffer, MakeKey(0), &blob));

    blobCache.clear();
    EXPECT_TRUE(blobCache.empty());
}

// Test a cache with many small values, that it can handle unlimited inserts.
TEST(BlobCacheTest, ManySmallValues)
{
    constexpr size_t kSize = 32;
    BlobCache blobCache(kSize);
    angle::ScratchBuffer scratchBuffer;

    for (size_t value = 0; value < kSize; ++value)
    {
        blobCache.populate(MakeKey(value), MakeBlob(1, value));

        Blob qvalue;
        EXPECT_TRUE(blobCache.get(nullptr, &scratchBuffer, MakeKey(value), &qvalue));
        if (qvalue.size() > 0)
        {
            EXPECT_EQ(value, qvalue[0]);
        }
    }

    EXPECT_EQ(32u, blobCache.size());
    EXPECT_FALSE(blobCache.empty());

    // Putting one element evicts the first element.
    blobCache.populate(MakeKey(kSize), MakeBlob(1, kSize));

    Blob qvalue;
    EXPECT_FALSE(blobCache.get(nullptr, &scratchBuffer, MakeKey(0), &qvalue));

    // Putting one large element cleans out the whole stack.
    blobCache.populate(MakeKey(kSize + 1), MakeBlob(kSize, kSize + 1));
    EXPECT_EQ(32u, blobCache.size());
    EXPECT_FALSE(blobCache.empty());

    for (size_t value = 0; value <= kSize; ++value)
    {
        EXPECT_FALSE(blobCache.get(nullptr, &scratchBuffer, MakeKey(value), &qvalue));
    }
    EXPECT_TRUE(blobCache.get(nullptr, &scratchBuffer, MakeKey(kSize + 1), &qvalue));
    if (qvalue.size() > 0)
    {
        EXPECT_EQ(kSize + 1, qvalue[0]);
    }

    // Put a bunch of items in the cache sequentially.
    for (size_t value = 0; value < kSize * 10; ++value)
    {
        blobCache.populate(MakeKey(value), MakeBlob(1, value));
    }

    EXPECT_EQ(32u, blobCache.size());
}

// Tests putting an oversize element.
TEST(BlobCacheTest, OversizeValue)
{
    constexpr size_t kSize = 32;
    BlobCache blobCache(kSize);
    angle::ScratchBuffer scratchBuffer;

    blobCache.populate(MakeKey(5), MakeBlob(100));

    Blob qvalue;
    EXPECT_FALSE(blobCache.get(nullptr, &scratchBuffer, MakeKey(5), &qvalue));
}

TEST(BlobCacheTest, Resize)
{
    constexpr size_t kSize = 32;
    BlobCache blobCache(kSize);

    blobCache.populate(MakeKey(0), MakeBlob(16));
    EXPECT_EQ(16u, blobCache.size());

    EXPECT_EQ(16u, blobCache.resize(64));
    EXPECT_EQ(0u, blobCache.size());
    EXPECT_EQ(64u, blobCache.maxSize());
}

// Tests querying cache entries by index: size-only, out-of-range, buffer-too-small, and full copy.
TEST(BlobCacheTest, GetAt)
{
    constexpr size_t kSize = 64;
    BlobCache blobCache(kSize);

    size_t valueSize = 0;
    EXPECT_EQ(BlobCache::GetAtResult::NotFound,
              blobCache.getAt(0, nullptr, std::nullopt, &valueSize));

    const Key key0 = MakeKey(10);
    blobCache.populate(key0, MakeBlob(16, 42));

    // Size-only query.
    EXPECT_EQ(BlobCache::GetAtResult::Success,
              blobCache.getAt(0, nullptr, std::nullopt, &valueSize));
    EXPECT_EQ(16u, valueSize);

    // Out-of-range query.
    EXPECT_EQ(BlobCache::GetAtResult::NotFound,
              blobCache.getAt(1, nullptr, std::nullopt, &valueSize));

    // Buffer too small should fail without overwriting output key/value, but still report the
    // required size.
    Key queriedKey = MakeKey(0);
    std::vector<uint8_t> smallBuf(8, 0xAB);
    valueSize = 0;
    EXPECT_EQ(BlobCache::GetAtResult::BufferTooSmall,
              blobCache.getAt(0, &queriedKey, angle::Span(smallBuf), &valueSize));
    EXPECT_EQ(MakeKey(0), queriedKey);
    EXPECT_EQ(std::vector<uint8_t>(8, 0xAB), smallBuf);
    EXPECT_EQ(16u, valueSize);

    // Full query.
    std::vector<uint8_t> buf(16, 0);
    EXPECT_EQ(BlobCache::GetAtResult::Success,
              blobCache.getAt(0, &queriedKey, angle::Span(buf), &valueSize));
    EXPECT_EQ(16u, valueSize);
    EXPECT_EQ(key0, queriedKey);
    for (uint8_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(i + 42), buf[i]);
    }
}

// Verifies that data returned by get() retains shared ownership of the cached buffer, so it
// remains valid after the cache is resized or the entry is evicted.
TEST(BlobCacheTest, GetRetainsOwnership)
{
    constexpr size_t kSize = 64;
    BlobCache blobCache(kSize);

    const Key key0 = MakeKey(10);
    blobCache.populate(key0, MakeBlob(16, 42));

    // Returned view from get() shares ownership without copying into scratchBuffer and stays valid
    // after cache resize/eviction.
    angle::ScratchBuffer scratchBuffer;
    Blob qvalue;
    Blob qvalue2;
    ASSERT_TRUE(blobCache.get(nullptr, &scratchBuffer, key0, &qvalue));
    ASSERT_TRUE(blobCache.get(nullptr, &scratchBuffer, key0, &qvalue2));
    EXPECT_EQ(qvalue.data(), qvalue2.data());
    EXPECT_EQ(0u, scratchBuffer.getMemoryBuffer()->capacity());
    blobCache.resize(kSize);
    EXPECT_EQ(0u, blobCache.entryCount());
    EXPECT_EQ(16u, qvalue.size());
    for (uint8_t i = 0; i < 16; ++i)
    {
        EXPECT_EQ(static_cast<uint8_t>(i + 42), qvalue[i]);
    }

    // Empty blobs return a null/zero-sized view and stay safe across eviction.
    const Key emptyKey = MakeKey(20);
    blobCache.populate(emptyKey, MakeBlob(0));
    Blob emptyValue;
    ASSERT_TRUE(blobCache.get(nullptr, &scratchBuffer, emptyKey, &emptyValue));
    EXPECT_EQ(nullptr, emptyValue.data());
    EXPECT_EQ(0u, emptyValue.size());
    EXPECT_EQ(0u, scratchBuffer.getMemoryBuffer()->capacity());

    // Move-assigning a value transfers ownership and leaves the source as an empty view.
    Blob moveAssigned;
    moveAssigned = std::move(qvalue);
    EXPECT_EQ(nullptr, qvalue.data());
    EXPECT_EQ(0u, qvalue.size());
    EXPECT_EQ(16u, moveAssigned.size());
    EXPECT_EQ(static_cast<uint8_t>(42), moveAssigned[0]);
}

// Tests BlobCache::getSize() on both internal cache entries and application callbacks, verifying
// that the callback path issues only a single null-buffer size query with no data copy.
TEST(BlobCacheTest, GetSize)
{
    constexpr size_t kSize = 64;
    BlobCache blobCache(kSize);

    const Key key0     = MakeKey(10);
    const Key emptyKey = MakeKey(20);
    const Key missKey  = MakeKey(30);

    size_t blobSize = 123;
    EXPECT_FALSE(blobCache.getSize(nullptr, key0, &blobSize));

    blobCache.populate(key0, MakeBlob(24, 7));
    blobCache.populate(emptyKey, MakeBlob(0));

    EXPECT_TRUE(blobCache.getSize(nullptr, key0, &blobSize));
    EXPECT_EQ(24u, blobSize);

    EXPECT_TRUE(blobCache.getSize(nullptr, emptyKey, &blobSize));
    EXPECT_EQ(0u, blobSize);

    EXPECT_FALSE(blobCache.getSize(nullptr, missKey, &blobSize));

    blobCache.remove(key0);
    EXPECT_FALSE(blobCache.getSize(nullptr, key0, &blobSize));

    // Application callback path: getSize() must issue exactly one callback with (nullptr, 0).
    static Key sExpectedKey;
    static size_t sGetCallCount;
    static bool sNonNullBufferSeen;
    sExpectedKey       = key0;
    sGetCallCount      = 0;
    sNonNullBufferSeen = false;

    auto setFunc = [](const void *, EGLsizeiANDROID, const void *, EGLsizeiANDROID) {};
    auto getFunc = [](const void *key, EGLsizeiANDROID keySize, void *value,
                      EGLsizeiANDROID valueSize) -> EGLsizeiANDROID {
        ++sGetCallCount;
        if (value != nullptr || valueSize != 0)
        {
            sNonNullBufferSeen = true;
        }
        // SAFETY: EGLGetBlobFuncANDROID passes |keySize| bytes at |key|.
        if (keySize == static_cast<EGLsizeiANDROID>(sExpectedKey.size()) &&
            ANGLE_UNSAFE_BUFFERS(
                angle::Span(static_cast<const uint8_t *>(key), static_cast<size_t>(keySize))) ==
                angle::Span(sExpectedKey))
        {
            return 4096;
        }
        return 0;
    };

    blobCache.setBlobCacheFuncs(setFunc, getFunc);

    blobSize = 0;
    EXPECT_TRUE(blobCache.getSize(nullptr, key0, &blobSize));
    EXPECT_EQ(4096u, blobSize);
    EXPECT_EQ(1u, sGetCallCount);
    EXPECT_FALSE(sNonNullBufferSeen);

    sGetCallCount = 0;
    EXPECT_FALSE(blobCache.getSize(nullptr, missKey, &blobSize));
    EXPECT_EQ(1u, sGetCallCount);
    EXPECT_FALSE(sNonNullBufferSeen);
}

// Tests thread safety of BlobCache under concurrent populate, get, getSize, getAt, compressAndPut,
// getAndDecompress, trim, remove, resize, and size queries from multiple threads.
TEST(BlobCacheTest, ConcurrentAccess)
{
    constexpr size_t kCacheSize  = 16 * 1024;
    constexpr int kNumThreads    = 8;
    constexpr int kNumIterations = 200;

    BlobCache blobCache(kCacheSize);
    std::atomic<bool> start(false);
    std::vector<std::thread> threads;

    for (int t = 0; t < kNumThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            angle::ScratchBuffer scratchBuffer;
            std::vector<uint8_t> queryBuf(512);
            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (int i = 0; i < kNumIterations; ++i)
            {
                uint8_t seed      = static_cast<uint8_t>((t * 31 + i) & 0xFF);
                Key rawKey        = MakeKey(seed);
                rawKey[0]         = 0;
                Key compressedKey = MakeKey(seed);
                compressedKey[0]  = 1;

                switch (i % 6)
                {
                    case 0:
                    {
                        blobCache.populate(rawKey, MakeBlob(256, seed));
                        break;
                    }
                    case 1:
                    {
                        size_t blobSize = 0;
                        if (blobCache.getSize(nullptr, rawKey, &blobSize))
                        {
                            EXPECT_EQ(256u, blobSize);
                        }
                        Blob value;
                        if (blobCache.get(nullptr, &scratchBuffer, rawKey, &value))
                        {
                            EXPECT_EQ(256u, value.size());
                            if (value.size() > 0)
                            {
                                EXPECT_EQ(seed, value[0]);
                            }
                        }
                        break;
                    }
                    case 2:
                    {
                        size_t compressedSize = 0;
                        EXPECT_TRUE(blobCache.compressAndPut(nullptr, compressedKey,
                                                             MakeBlob(128, seed), &compressedSize));
                        angle::MemoryBuffer uncompressed;
                        auto res = blobCache.getAndDecompress(nullptr, &scratchBuffer,
                                                              compressedKey, 1024, &uncompressed);
                        // Concurrent eviction may legitimately produce NotFound, but every stored
                        // entry is a valid compressed blob, so decompression must never fail.
                        EXPECT_NE(BlobCache::GetAndDecompressResult::DecompressFailure, res);
                        if (res == BlobCache::GetAndDecompressResult::Success)
                        {
                            EXPECT_EQ(128u, uncompressed.size());
                            EXPECT_EQ(seed, uncompressed[0]);
                        }
                        break;
                    }
                    case 3:
                    {
                        blobCache.trim(kCacheSize / 2);
                        blobCache.remove(rawKey);
                        blobCache.remove(compressedKey);
                        break;
                    }
                    case 4:
                    {
                        if (t == 0 && (i % 30 == 4))
                        {
                            blobCache.resize(kCacheSize);
                        }
                        else
                        {
                            (void)blobCache.size();
                            (void)blobCache.maxSize();
                        }
                        break;
                    }
                    case 5:
                    {
                        Key queriedKey;
                        size_t valueSize = 0;
                        size_t count     = blobCache.entryCount();
                        if (count > 0)
                        {
                            BlobCache::GetAtResult res = blobCache.getAt(
                                i % count, &queriedKey, angle::Span(queryBuf), &valueSize);
                            if (res == BlobCache::GetAtResult::Success && valueSize > 0)
                            {
                                EXPECT_LE(valueSize, queryBuf.size());
                            }
                        }
                        break;
                    }
                }
            }
        });
    }

    start.store(true, std::memory_order_release);
    for (std::thread &thread : threads)
    {
        thread.join();
    }
}

}  // namespace egl

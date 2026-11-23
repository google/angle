//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BlobCache: Stores keyed blobs in memory to support EGL_ANDROID_blob_cache.
// Can be used in conjunction with the platform layer to warm up the cache from
// disk.  MemoryProgramCache uses this to handle caching of compiled programs.

#include "libANGLE/BlobCache.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/histogram_macros.h"
#include "platform/PlatformMethods.h"

namespace egl
{
BlobCache::BlobCache(size_t maxCacheSizeBytes)
    : mBlobCache(maxCacheSizeBytes), mSetBlobFunc(nullptr), mGetBlobFunc(nullptr)
{}

BlobCache::~BlobCache() {}

void BlobCache::put(const gl::Context *context,
                    const BlobCache::Key &key,
                    angle::MemoryBuffer &&value)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    if (areApplicationCallbacksSetLocked(context))
    {
        putApplicationLocked(context, key, value);
    }
    else
    {
        // Cache it inside blob cache only if caching inside the application is not possible.
        populateLocked(key, std::move(value), CacheSource::Memory);
    }
}

bool BlobCache::compressAndPut(const gl::Context *context,
                               const BlobCache::Key &key,
                               angle::MemoryBuffer &&uncompressedValue,
                               size_t *compressedSize)
{
    angle::MemoryBuffer compressedValue;
    if (!angle::CompressBlob(uncompressedValue.size(), uncompressedValue.data(), &compressedValue))
    {
        return false;
    }
    if (compressedSize != nullptr)
        *compressedSize = compressedValue.size();
    put(context, key, std::move(compressedValue));
    return true;
}

void BlobCache::putApplication(const gl::Context *context,
                               const BlobCache::Key &key,
                               const angle::MemoryBuffer &value)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    putApplicationLocked(context, key, value);
}

void BlobCache::putApplicationLocked(const gl::Context *context,
                                     const BlobCache::Key &key,
                                     const angle::MemoryBuffer &value)
{
    if (context && context->areBlobCacheFuncsSet())
    {
        const gl::BlobCacheCallbacks &contextCallbacks =
            context->getState().getBlobCacheCallbacks();
        contextCallbacks.setFunction(key.data(), key.size(), value.data(), value.size(),
                                     contextCallbacks.userParam);
    }
    else if (areBlobCacheFuncsSetLocked())
    {
        mSetBlobFunc(key.data(), key.size(), value.data(), value.size());
    }
}

void BlobCache::populate(const BlobCache::Key &key, angle::MemoryBuffer &&value, CacheSource source)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    populateLocked(key, std::move(value), source);
}

void BlobCache::populateLocked(const BlobCache::Key &key,
                               angle::MemoryBuffer &&value,
                               CacheSource source)
{
    const size_t entrySize = value.size();
    if (entrySize > mBlobCache.maxSize())
    {
        return;
    }

    auto sharedValue = std::make_shared<const angle::MemoryBuffer>(std::move(value));
    mBlobCache.put(key, CacheEntry(std::move(sharedValue), source), entrySize);
}

bool BlobCache::get(const gl::Context *context,
                    angle::ScratchBuffer *scratchBuffer,
                    const BlobCache::Key &key,
                    BlobCache::Value *valueOut)
{
    ASSERT(scratchBuffer != nullptr);
    ASSERT(valueOut != nullptr);

    BlobCache::Value resultValue;
    {
        std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
        bool found = false;
        if (areApplicationCallbacksSetLocked(context))
        {
            // Look into the application's cache, if there is such a cache.
            found = getApplicationLocked(context, scratchBuffer, key, &resultValue);
        }
        else
        {
            // Otherwise we are doing caching internally, so try to find it there.
            found = getInternalLocked(key, &resultValue);
        }
        if (!found)
        {
            return false;
        }
    }

    // Assign *valueOut after releasing mBlobCacheMutex so that if *valueOut previously held the
    // last reference to an evicted buffer, deallocation happens outside the lock.
    *valueOut = std::move(resultValue);
    return true;
}

bool BlobCache::getApplicationLocked(const gl::Context *context,
                                     angle::ScratchBuffer *scratchBuffer,
                                     const BlobCache::Key &key,
                                     BlobCache::Value *valueOut)
{
    EGLsizeiANDROID valueSize = callBlobGetCallback(context, key.data(), key.size(), nullptr, 0);
    if (valueSize <= 0)
    {
        return false;
    }

    angle::MemoryBuffer *scratchMemory;
    bool result = scratchBuffer->get(valueSize, &scratchMemory);
    if (!result)
    {
        ERR() << "Failed to allocate memory for binary blob";
        return false;
    }

    EGLsizeiANDROID originalValueSize = valueSize;
    valueSize =
        callBlobGetCallback(context, key.data(), key.size(), scratchMemory->data(), valueSize);

    // Make sure the key/value pair still exists/is unchanged after the second call
    // (modifications to the application cache by another thread are a possibility)
    if (valueSize != originalValueSize)
    {
        // This warning serves to find issues with the application cache, none of which are
        // currently known to be thread-safe.  If such a use ever arises, this WARN can be
        // removed.
        WARN() << "Binary blob no longer available in cache (removed by a thread?)";
        return false;
    }

    *valueOut = BlobCache::Value(scratchMemory->data(), valueSize);
    return true;
}

bool BlobCache::getInternalLocked(const BlobCache::Key &key, BlobCache::Value *valueOut)
{
    const CacheEntry *entry;
    if (!mBlobCache.get(key, &entry))
    {
        return false;
    }

    // Retain shared ownership of the immutable cached buffer so the returned view remains valid
    // without copying even if another thread concurrently evicts entries from mBlobCache.
    *valueOut = entry->first->empty() ? BlobCache::Value() : BlobCache::Value(entry->first);
    return true;
}

bool BlobCache::getAt(size_t index, const BlobCache::Key **keyOut, BlobCache::Value *valueOut)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    const CacheEntry *valueBuf;
    bool result = mBlobCache.getAt(index, keyOut, &valueBuf);
    if (result)
    {
        *valueOut = BlobCache::Value(valueBuf->first);
    }
    return result;
}

BlobCache::GetAndDecompressResult BlobCache::getAndDecompress(
    const gl::Context *context,
    angle::ScratchBuffer *scratchBuffer,
    const BlobCache::Key &key,
    size_t maxUncompressedDataSize,
    angle::MemoryBuffer *uncompressedValueOut)
{
    ASSERT(uncompressedValueOut);

    Value compressedValue;
    if (!get(context, scratchBuffer, key, &compressedValue))
    {
        return GetAndDecompressResult::NotFound;
    }

    // |compressedValue| either points into the caller-owned |scratchBuffer| or retains shared
    // ownership of the immutable cache entry, so decompression can run without holding
    // mBlobCacheMutex.
    if (!angle::DecompressBlob(compressedValue.data(), compressedValue.size(),
                               maxUncompressedDataSize, uncompressedValueOut))
    {
        return GetAndDecompressResult::DecompressFailure;
    }

    return GetAndDecompressResult::Success;
}

void BlobCache::remove(const BlobCache::Key &key)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    mBlobCache.eraseByKey(key);
}

void BlobCache::setBlobCacheFuncs(EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get)
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    mSetBlobFunc = set;
    mGetBlobFunc = get;
}

bool BlobCache::areBlobCacheFuncsSetLocked() const
{
    // Either none or both of the callbacks should be set.
    ASSERT((mSetBlobFunc != nullptr) == (mGetBlobFunc != nullptr));

    return mSetBlobFunc != nullptr && mGetBlobFunc != nullptr;
}

bool BlobCache::areBlobCacheFuncsSet() const
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    return areBlobCacheFuncsSetLocked();
}

bool BlobCache::areApplicationCallbacksSetLocked(const gl::Context *context) const
{
    return (context && context->areBlobCacheFuncsSet()) || areBlobCacheFuncsSetLocked();
}

bool BlobCache::isCachingEnabled(const gl::Context *context) const
{
    std::scoped_lock<angle::SimpleMutex> lock(mBlobCacheMutex);
    return areApplicationCallbacksSetLocked(context) || mBlobCache.maxSize() > 0;
}

size_t BlobCache::callBlobGetCallback(const gl::Context *context,
                                      const void *key,
                                      size_t keySize,
                                      void *value,
                                      size_t valueSize)
{
    if (context && context->areBlobCacheFuncsSet())
    {
        const gl::BlobCacheCallbacks &contextCallbacks =
            context->getState().getBlobCacheCallbacks();
        return contextCallbacks.getFunction(key, keySize, value, valueSize,
                                            contextCallbacks.userParam);
    }
    else
    {
        ASSERT(mGetBlobFunc);
        return mGetBlobFunc(key, keySize, value, valueSize);
    }
}

}  // namespace egl

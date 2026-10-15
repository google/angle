//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_pipeline_binary_cache.h:
//    Defines the PipelineBinariesCache class and helper structures for caching
//    Vulkan pipeline binaries.
//

#ifndef LIBANGLE_RENDERER_VULKAN_VK_PIPELINE_BINARY_CACHE_H_
#define LIBANGLE_RENDERER_VULKAN_VK_PIPELINE_BINARY_CACHE_H_

#include <atomic>
#include <vector>

#include "common/MemoryBuffer.h"
#include "common/angleutils.h"
#include "common/span.h"
#include "common/vulkan/vk_headers.h"
#include "libANGLE/Error.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

namespace rx
{
class Renderer;

namespace vk
{

inline bool PipelineBinaryKeysEqual(const VkPipelineBinaryKeyKHR &a,
                                    const VkPipelineBinaryKeyKHR &b)
{
    return angle::Span(a.key).first(a.keySize) == angle::Span(b.key).first(b.keySize);
}

// One pipeline's binaries: the key the driver gave the pipeline, and the key and bytes of each
// binary it was built from.  How many binaries a pipeline has varies by driver.
struct PipelineBinarySet
{
    VkPipelineBinaryKeyKHR pipelineKey = {};
    std::vector<VkPipelineBinaryKeyKHR> binaryKeys;
    std::vector<std::vector<uint8_t>> binaries;
};

// Creates Vulkan pipelines through VK_KHR_pipeline_binary, storing the binaries of each pipeline it
// compiles in the platform blob cache, one entry per pipeline key, and creating pipelines whose
// key has an entry from the stored binaries instead of compiling them.
class PipelineBinariesCache : angle::NonCopyable
{
  public:
    PipelineBinariesCache();
    ~PipelineBinariesCache();

    // The largest entry, and the largest payload one may hold.
    static constexpr size_t kMaxBlobSize = 8 * 1024 * 1024;

    // Compresses |payload| into an entry in |entryOut|.  Returns false, leaving |entryOut| empty,
    // on failure.
    static bool CompressEntry(angle::Span<const uint8_t> payload, angle::MemoryBuffer *entryOut);
    // Decompresses an entry written by CompressEntry() into |payloadOut|.  Returns false if it is
    // damaged, or written with a codec this build lacks.
    static bool DecompressEntry(angle::Span<const uint8_t> entry, angle::MemoryBuffer *payloadOut);

    // Serializes |binarySet|, captured with the driver's |globalKey|, as stored in a blob cache
    // entry, before CompressEntry().
    static void SerializeBinarySet(const VkPipelineBinaryKeyKHR &globalKey,
                                   const PipelineBinarySet &binarySet,
                                   std::vector<uint8_t> *serializedOut);
    // Reads back what SerializeBinarySet() wrote.  Returns false if it is malformed, or was stored
    // in another format or for another global or pipeline key.
    static bool DeserializeBinarySet(angle::Span<const uint8_t> serialized,
                                     const VkPipelineBinaryKeyKHR &globalKey,
                                     const VkPipelineBinaryKeyKHR &pipelineKey,
                                     PipelineBinarySet *binarySetOut);

    angle::Result init(ErrorContext *context);

    // Creates a pipeline from stored binaries if its key has an entry, and otherwise compiles it
    // and stores its binaries.  |createInfo| has a VkPipelineCreateFlags2CreateInfoKHR chained,
    // which the flag that asks the driver for binaries is set in.
    VkResult createGraphicsPipeline(ErrorContext *context,
                                    VkGraphicsPipelineCreateInfo *createInfo,
                                    Pipeline *pipelineOut);
    VkResult createComputePipeline(ErrorContext *context,
                                   VkComputePipelineCreateInfo *createInfo,
                                   Pipeline *pipelineOut);

    // The blob cache key the binaries for |pipelineKey| are stored under.
    angle::BlobCacheKey getBlobKey(const VkPipelineBinaryKeyKHR &pipelineKey) const;

    bool isInitialized() const { return mInitialized; }
    size_t getCacheHits() const;
    size_t getCacheMisses() const;
    void resetStats();

    // Testing helpers.  Not to be called while pipelines are being created.
    void setGlobalKeyForTesting(const VkPipelineBinaryKeyKHR &key);
    VkPipelineBinaryKeyKHR getGlobalKeyForTesting() const;

  private:
    template <typename CreateInfoT>
    VkResult createPipelineWithBinaries(ErrorContext *context,
                                        CreateInfoT *createInfo,
                                        Pipeline *pipelineOut);

    // Sets mGlobalKey, and hashes what every blob key starts with into mBlobKeyPrefix.
    void setGlobalKey(const VkPipelineBinaryKeyKHR &key);

    // Reads the binaries stored for |pipelineKey|.  Returns false if there are none, or they are
    // damaged.
    bool loadBinaries(ErrorContext *context,
                      const VkPipelineBinaryKeyKHR &pipelineKey,
                      PipelineBinarySet *binarySetOut) const;
    // Stores |binarySet| under its pipeline key.
    void storeBinaries(ErrorContext *context, const PipelineBinarySet &binarySet) const;

    // Written by init(), before any pipeline is created, and so read without a lock.
    //
    // The driver's global pipeline binary key, part of every blob key.
    VkPipelineBinaryKeyKHR mGlobalKey;
    // The hash of what every blob key starts with, the format version and the global key.
    angle::BlobCacheKey mBlobKeyPrefix;
    bool mInitialized;

    // Pipelines are created, and counted, on any thread.
    std::atomic<size_t> mCacheHits;
    std::atomic<size_t> mCacheMisses;
};

}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_PIPELINE_BINARY_CACHE_H_

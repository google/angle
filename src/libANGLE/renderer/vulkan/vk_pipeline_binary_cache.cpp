//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_pipeline_binary_cache.cpp:
//    Implements the PipelineBinariesCache class and helper structures.
//

#include "libANGLE/renderer/vulkan/vk_pipeline_binary_cache.h"

#include <array>

#include "common/BinaryStream.h"
#include "common/FixedVector.h"
#include "common/MemoryBuffer.h"
#include "common/PackedEnums.h"
#include "common/hash_utils.h"
#include "common/span.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"
#include "libANGLE/trace.h"

namespace rx
{
namespace vk
{

namespace
{
// Bumped when the stored format changes.  It is part of every blob key, so an entry in another
// format is never looked up, and is stored in every entry, so one found under a colliding blob key
// is rejected.
constexpr uint32_t kFormatVersion = 1;

// Drivers produce a handful of binaries per pipeline.  An entry with far more is damaged.
constexpr uint32_t kMaxBinariesPerSet = 64;

// zstd where the build has it, and gzip in an embedder's build without it.
bool CompressPayload(angle::Span<const uint8_t> payload, angle::MemoryBuffer *compressedOut)
{
#if defined(ANGLE_HAS_ZSTD)
    return angle::CompressBlobZstd(payload.size(), payload.data(), compressedOut);
#else
    return angle::CompressBlob(payload.size(), payload.data(), compressedOut);
#endif
}

bool StartsWith(angle::Span<const uint8_t> data, angle::Span<const uint8_t> prefix)
{
    return data.size() >= prefix.size() && data.first(prefix.size()) == prefix;
}

void SerializeKey(const VkPipelineBinaryKeyKHR &key, gl::BinaryOutputStream *stream)
{
    stream->writeInt<uint32_t>(key.keySize);
    stream->writeBytes(angle::Span<const uint8_t>(key.key));
}

bool DeserializeKey(gl::BinaryInputStream *stream, VkPipelineBinaryKeyKHR *keyOut)
{
    *keyOut         = {};
    keyOut->sType   = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
    keyOut->keySize = stream->readInt<uint32_t>();
    stream->readBytes(angle::Span<uint8_t>(keyOut->key));
    return !stream->error() && keyOut->keySize <= VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR;
}

// Reads the key and bytes of each of |binaries| into |binarySetOut|.
bool GetBinaryData(VkDevice device,
                   const std::vector<VkPipelineBinaryKHR> &binaries,
                   PipelineBinarySet *binarySetOut)
{
    // The driver writes each binary's key into these, which need their sType set
    VkPipelineBinaryKeyKHR emptyKey = {};
    emptyKey.sType                  = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
    binarySetOut->binaryKeys.assign(binaries.size(), emptyKey);
    binarySetOut->binaries.resize(binaries.size());

    for (size_t i = 0; i < binaries.size(); ++i)
    {
        VkPipelineBinaryDataInfoKHR dataInfo = {};
        dataInfo.sType                       = VK_STRUCTURE_TYPE_PIPELINE_BINARY_DATA_INFO_KHR;
        dataInfo.pipelineBinary              = binaries[i];

        // First call with null output to get the size
        size_t dataSize = 0;
        VkResult result = VK_CALL(vkGetPipelineBinaryDataKHR, device, &dataInfo,
                                  &binarySetOut->binaryKeys[i], &dataSize, nullptr);
        if (result != VK_SUCCESS)
        {
            WARN() << "vkGetPipelineBinaryDataKHR failed to get a binary's size, VkResult: "
                   << result;
            return false;
        }
        if (dataSize == 0)
        {
            WARN() << "vkGetPipelineBinaryDataKHR gave an empty binary";
            return false;
        }

        // Call again with the output location to retrieve data
        binarySetOut->binaries[i].resize(dataSize);
        result = VK_CALL(vkGetPipelineBinaryDataKHR, device, &dataInfo,
                         &binarySetOut->binaryKeys[i], &dataSize, binarySetOut->binaries[i].data());
        if (result != VK_SUCCESS)
        {
            WARN() << "vkGetPipelineBinaryDataKHR failed to get a binary's data, VkResult: "
                   << result;
            return false;
        }
    }
    return true;
}

// Reads the binaries out of a pipeline created with VK_PIPELINE_CREATE_2_CAPTURE_DATA_BIT_KHR.
// Returns false if the driver fails.  A pipeline the driver gives no binaries for leaves
// |binarySetOut| empty.
bool CaptureBinaries(VkDevice device, const Pipeline &pipeline, PipelineBinarySet *binarySetOut)
{
    VkPipelineBinaryCreateInfoKHR createInfo = {};
    createInfo.sType                         = VK_STRUCTURE_TYPE_PIPELINE_BINARY_CREATE_INFO_KHR;
    createInfo.pipeline                      = pipeline.getHandle();

    // First call create with a pipeline and null binary pointer to get back the count
    VkPipelineBinaryHandlesInfoKHR handlesInfo = {};
    handlesInfo.sType                          = VK_STRUCTURE_TYPE_PIPELINE_BINARY_HANDLES_INFO_KHR;
    VkResult result =
        VK_CALL(vkCreatePipelineBinariesKHR, device, &createInfo, nullptr, &handlesInfo);
    if (result != VK_SUCCESS)
    {
        WARN() << "vkCreatePipelineBinariesKHR failed to count a pipeline's binaries, VkResult: "
               << result;
        return false;
    }
    if (handlesInfo.pipelineBinaryCount == 0)
    {
        return true;
    }

    // Now call create again using count, and pointing to storage to populate
    std::vector<VkPipelineBinaryKHR> binaries(handlesInfo.pipelineBinaryCount, VK_NULL_HANDLE);
    handlesInfo.pPipelineBinaries = binaries.data();
    result = VK_CALL(vkCreatePipelineBinariesKHR, device, &createInfo, nullptr, &handlesInfo);

    bool captured = false;
    if (result == VK_SUCCESS)
    {
        captured = GetBinaryData(device, binaries, binarySetOut);
    }
    else
    {
        WARN() << "vkCreatePipelineBinariesKHR failed to capture a pipeline's binaries, VkResult: "
               << result;
    }

    // Clean up the handles
    for (VkPipelineBinaryKHR handle : binaries)
    {
        VK_CALL(vkDestroyPipelineBinaryKHR, device, handle, nullptr);
    }
    return captured;
}

// Returns the structure of type |sType| in |createInfo|'s pNext chain, or nullptr if it has none.
template <typename CreateInfoT>
VkBaseOutStructure *FindInPNextChain(CreateInfoT *createInfo, VkStructureType sType)
{
    VkBaseOutStructure *next = reinterpret_cast<VkBaseOutStructure *>(createInfo)->pNext;
    while (next != nullptr && next->sType != sType)
    {
        next = next->pNext;
    }
    return next;
}

// Whether |key| is all zeros.
bool IsPlaceholderKey(const VkPipelineBinaryKeyKHR &key)
{
    for (uint8_t byte : angle::Span(key.key).first(key.keySize))
    {
        if (byte != 0)
        {
            return false;
        }
    }
    return true;
}

// Whether |createInfo| links a pipeline from graphics pipeline libraries.
template <typename CreateInfoT>
bool IsLinkedFromLibraries(CreateInfoT *createInfo)
{
    return FindInPNextChain(createInfo, VK_STRUCTURE_TYPE_PIPELINE_LIBRARY_CREATE_INFO_KHR) !=
           nullptr;
}
}  // namespace

// Fails when the payload or the entry would be larger than kMaxBlobSize, the largest value
// Android's blob cache stores.
//
// static
bool PipelineBinariesCache::CompressEntry(angle::Span<const uint8_t> payload,
                                          angle::MemoryBuffer *entryOut)
{
    if (payload.empty() || payload.size() > kMaxBlobSize)
    {
        WARN() << "Vulkan pipeline binary not stored: " << payload.size()
               << " bytes is outside the 1 to " << kMaxBlobSize << " byte range";
        return false;
    }

    if (!CompressPayload(payload, entryOut))
    {
        // The compression helpers say why if they fail.
        return false;
    }

    // A payload that does not compress can come out a little larger than it went in.
    if (entryOut->size() > kMaxBlobSize)
    {
        WARN() << "Vulkan pipeline binary not stored: " << payload.size() << " bytes compressed to "
               << entryOut->size() << ", past the " << kMaxBlobSize << " byte limit";
        entryOut->clear();
        return false;
    }

    return true;
}

// Tells the codecs apart by the magic number each format starts with: gzip's (RFC 1952), which
// angle::CompressBlob() writes, and zstd's (RFC 8878).  Both verify a checksum as they decompress,
// and check the size the entry records against kMaxBlobSize before allocating.
//
// static
bool PipelineBinariesCache::DecompressEntry(angle::Span<const uint8_t> entry,
                                            angle::MemoryBuffer *payloadOut)
{
    constexpr std::array<uint8_t, 2> kGzipMagic = {0x1F, 0x8B};
    constexpr std::array<uint8_t, 4> kZstdMagic = {0x28, 0xB5, 0x2F, 0xFD};

    // The helpers say why if they fail.
    if (StartsWith(entry, kGzipMagic))
    {
        return angle::DecompressBlob(entry.data(), entry.size(), kMaxBlobSize, payloadOut);
    }
    if (StartsWith(entry, kZstdMagic))
    {
#if defined(ANGLE_HAS_ZSTD)
        return angle::DecompressBlobZstd(entry.data(), entry.size(), kMaxBlobSize, payloadOut);
#else
        // Written by a build with zstd.  Not damage, so a miss that is not worth a warning.
        return false;
#endif
    }

    WARN() << "Vulkan pipeline binary entry is in neither the gzip nor the zstd format";
    return false;
}

// A pipeline's binaries, serialized, before CompressEntry() compresses them:
//
//   uint32_t formatVersion
//   uint32_t globalKeySize;
//   uint8_t  globalKey[VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR]
//   uint32_t pipelineKeySize;
//   uint8_t  pipelineKey[VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR]
//   uint32_t binaryCount
//   binaryCount x { uint32_t keySize; uint8_t key[VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR];
//                   uint32_t dataSize; uint8_t data[dataSize] }
//
// The format version and the keys are what the blob key is made of.  They are stored too, so that
// an entry found under a colliding blob key is rejected.
//
// static
void PipelineBinariesCache::SerializeBinarySet(const VkPipelineBinaryKeyKHR &globalKey,
                                               const PipelineBinarySet &binarySet,
                                               std::vector<uint8_t> *serializedOut)
{
    ASSERT(binarySet.binaryKeys.size() == binarySet.binaries.size());
    gl::BinaryOutputStream stream;
    stream.writeInt<uint32_t>(kFormatVersion);
    SerializeKey(globalKey, &stream);
    SerializeKey(binarySet.pipelineKey, &stream);
    stream.writeInt<uint32_t>(static_cast<uint32_t>(binarySet.binaries.size()));
    for (size_t index = 0; index < binarySet.binaries.size(); ++index)
    {
        SerializeKey(binarySet.binaryKeys[index], &stream);
        stream.writeInt<uint32_t>(static_cast<uint32_t>(binarySet.binaries[index].size()));
        stream.writeBytes(binarySet.binaries[index]);
    }
    *serializedOut = stream.takeData();
}

// static
bool PipelineBinariesCache::DeserializeBinarySet(angle::Span<const uint8_t> serialized,
                                                 const VkPipelineBinaryKeyKHR &globalKey,
                                                 const VkPipelineBinaryKeyKHR &pipelineKey,
                                                 PipelineBinarySet *binarySetOut)
{
    gl::BinaryInputStream stream(serialized);
    const uint32_t formatVersion = stream.readInt<uint32_t>();
    if (stream.error() || formatVersion != kFormatVersion)
    {
        WARN() << "Stored Vulkan pipeline binaries are in another format";
        return false;
    }

    VkPipelineBinaryKeyKHR storedGlobalKey = {};
    if (!DeserializeKey(&stream, &storedGlobalKey) ||
        !PipelineBinaryKeysEqual(storedGlobalKey, globalKey))
    {
        WARN() << "Stored Vulkan pipeline binaries are for another global key";
        return false;
    }

    if (!DeserializeKey(&stream, &binarySetOut->pipelineKey) ||
        !PipelineBinaryKeysEqual(binarySetOut->pipelineKey, pipelineKey))
    {
        WARN() << "Stored Vulkan pipeline binaries are for another pipeline key";
        return false;
    }
    const uint32_t binaryCount = stream.readInt<uint32_t>();
    if (stream.error() || binaryCount == 0)
    {
        WARN() << "Error reading binary count in PipelineBinariesCache";
        return false;
    }
    if (binaryCount > kMaxBinariesPerSet)
    {
        WARN() << "kMaxBinariesPerSet exceeded in PipelineBinariesCache";
        return false;
    }

    binarySetOut->binaryKeys.resize(binaryCount);
    binarySetOut->binaries.resize(binaryCount);
    for (uint32_t index = 0; index < binaryCount; ++index)
    {
        if (!DeserializeKey(&stream, &binarySetOut->binaryKeys[index]))
        {
            WARN() << "Failed to deserialize binary key in PipelineBinariesCache";
            return false;
        }
        const uint32_t dataSize = stream.readInt<uint32_t>();
        if (stream.error() || dataSize == 0 || dataSize > stream.remainingSpan().size())
        {
            WARN() << "Error querying binary data size in PipelineBinariesCache";
            return false;
        }
        binarySetOut->binaries[index].resize(dataSize);
        stream.readBytes(binarySetOut->binaries[index]);
    }

    if (stream.error() || !stream.endOfStream())
    {
        WARN() << "Error reading binary data in PipelineBinariesCache";
        return false;
    }

    return true;
}

// Holds nothing in memory but the driver's global key and hit and miss counts.  Binaries live only
// in the blob cache, one entry per pipeline key.
PipelineBinariesCache::PipelineBinariesCache()
    : mGlobalKey{}, mBlobKeyPrefix{}, mInitialized(false), mCacheHits(0), mCacheMisses(0)
{
    mGlobalKey.sType = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
}

PipelineBinariesCache::~PipelineBinariesCache() = default;

// Queries the driver's global pipeline binary key.  Called when the Renderer is initialized, before
// any pipeline is created.
angle::Result PipelineBinariesCache::init(ErrorContext *context)
{
    // Binaries are stored through the platform's blob cache.
    ASSERT(context->getRenderer()->getGlobalOps() != nullptr);

    VkPipelineBinaryKeyKHR globalKey = {};
    globalKey.sType                  = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
    ANGLE_VK_TRY(context, VK_CALL(vkGetPipelineKeyKHR, context->getDevice(), nullptr, &globalKey));
    ANGLE_VK_CHECK(context, globalKey.keySize <= VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR,
                   VK_ERROR_INITIALIZATION_FAILED);
    setGlobalKey(globalKey);

    mInitialized = true;
    return angle::Result::Continue;
}

void PipelineBinariesCache::setGlobalKeyForTesting(const VkPipelineBinaryKeyKHR &key)
{
    setGlobalKey(key);
}

VkPipelineBinaryKeyKHR PipelineBinariesCache::getGlobalKeyForTesting() const
{
    return mGlobalKey;
}

size_t PipelineBinariesCache::getCacheHits() const
{
    return mCacheHits;
}

size_t PipelineBinariesCache::getCacheMisses() const
{
    return mCacheMisses;
}

void PipelineBinariesCache::resetStats()
{
    mCacheHits   = 0;
    mCacheMisses = 0;
}

// The only writer of mGlobalKey.  Every blob key starts with the format version and the global key,
// so they are hashed here, once.
//
// The global key is part of the blob key because a driver update invalidates the binaries: the new
// driver simply looks under other keys.  The ANGLE version is not: the binaries are the driver's,
// and an ANGLE build that creates a pipeline differently gets another pipeline key from the driver
// anyway.
void PipelineBinariesCache::setGlobalKey(const VkPipelineBinaryKeyKHR &key)
{
    mGlobalKey = key;

    static constexpr char kKeyPrefix[] = "angle_pipeline_binaries";
    const angle::Span<const uint8_t> globalKey =
        angle::Span(mGlobalKey.key).first(mGlobalKey.keySize);

    angle::BlobCacheHasher hasher;
    hasher.Init();
    hasher.Update(kKeyPrefix, sizeof(kKeyPrefix) - 1);
    angle::UpdateHashWithValue(hasher, kFormatVersion);
    angle::UpdateHashWithValue(hasher, mGlobalKey.keySize);
    hasher.Update(globalKey.data(), globalKey.size());
    hasher.Final();

    // SAFETY: The hasher's digest is kBlobCacheKeyLength bytes
    angle::Span(mBlobKeyPrefix)
        .copy_from(ANGLE_UNSAFE_BUFFERS(
            angle::Span<const uint8_t>(hasher.Digest(), mBlobKeyPrefix.size())));
}

angle::BlobCacheKey PipelineBinariesCache::getBlobKey(
    const VkPipelineBinaryKeyKHR &pipelineKey) const
{
    const angle::Span<const uint8_t> key = angle::Span(pipelineKey.key).first(pipelineKey.keySize);

    angle::BlobCacheHasher hasher;
    hasher.Init();
    hasher.Update(mBlobKeyPrefix.data(), mBlobKeyPrefix.size());
    angle::UpdateHashWithValue(hasher, pipelineKey.keySize);
    hasher.Update(key.data(), key.size());
    hasher.Final();

    angle::BlobCacheKey blobKey;
    // SAFETY: The hasher's digest is kBlobCacheKeyLength bytes
    angle::Span(blobKey).copy_from(
        ANGLE_UNSAFE_BUFFERS(angle::Span<const uint8_t>(hasher.Digest(), blobKey.size())));
    return blobKey;
}

bool PipelineBinariesCache::loadBinaries(ErrorContext *context,
                                         const VkPipelineBinaryKeyKHR &pipelineKey,
                                         PipelineBinarySet *binarySetOut) const
{
    ANGLE_TRACE_EVENT0("gpu.angle", "PipelineBinariesCache::loadBinaries");

    // See if we've got an entry for this key
    angle::BlobCacheValue value;
    if (!context->getRenderer()->getGlobalOps()->getBlob(getBlobKey(pipelineKey), &value))
    {
        return false;
    }

    // If we do, decompress the entry
    angle::MemoryBuffer serialized;
    if (!DecompressEntry(angle::Span<const uint8_t>(value), &serialized))
    {
        // The failure reason has already been issued
        return false;
    }

    // Extract the stored keys and binaries for this pipeline.  Any failure reason is issued.
    return DeserializeBinarySet(serialized.span(), mGlobalKey, pipelineKey, binarySetOut);
}

void PipelineBinariesCache::storeBinaries(ErrorContext *context,
                                          const PipelineBinarySet &binarySet) const
{
    ANGLE_TRACE_EVENT0("gpu.angle", "PipelineBinariesCache::storeBinaries");

    std::vector<uint8_t> serialized;
    SerializeBinarySet(mGlobalKey, binarySet, &serialized);
    angle::MemoryBuffer value;
    if (!CompressEntry(serialized, &value))
    {
        // Failure reason was already logged
        return;
    }

    context->getRenderer()->getGlobalOps()->putBlob(getBlobKey(binarySet.pipelineKey), value);
}

// Lets createPipelineWithBinaries() create either a graphics or a compute pipeline.
template <typename CreateInfoT>
struct PipelineHelperTraits;

template <>
struct PipelineHelperTraits<VkGraphicsPipelineCreateInfo>
{
    // At most one stage per graphics shader type
    using ShaderStages =
        angle::FixedVector<VkPipelineShaderStageCreateInfo, gl::kGraphicsShaderCount>;

    static VkResult initPipeline(VkDevice device,
                                 const VkGraphicsPipelineCreateInfo &createInfo,
                                 Pipeline *pipeline)
    {
        return pipeline->initGraphics(device, createInfo, nullptr);
    }

    // Points |createInfo| at copies of its stages, kept in |stagesOut|, without shader modules.
    static void OmitShaderModules(VkGraphicsPipelineCreateInfo *createInfo, ShaderStages *stagesOut)
    {
        // SAFETY: pStages holds stageCount stages.
        const angle::Span<const VkPipelineShaderStageCreateInfo> stages =
            ANGLE_UNSAFE_BUFFERS(angle::Span<const VkPipelineShaderStageCreateInfo>(
                createInfo->pStages, createInfo->stageCount));
        for (const VkPipelineShaderStageCreateInfo &stage : stages)
        {
            stagesOut->push_back(stage);
            stagesOut->back().module = VK_NULL_HANDLE;
        }
        createInfo->pStages = stagesOut->data();
    }
};

template <>
struct PipelineHelperTraits<VkComputePipelineCreateInfo>
{
    // Intentionally empty.  A compute pipeline has only one stage, which is part of its create
    // info, so there are no stages to copy.
    struct ShaderStages
    {};

    static VkResult initPipeline(VkDevice device,
                                 const VkComputePipelineCreateInfo &createInfo,
                                 Pipeline *pipeline)
    {
        return pipeline->initCompute(device, createInfo, nullptr);
    }

    // Clears the module of |createInfo|'s only stage.  |stagesOut| is unused, as there are no
    // stages to copy.
    static void OmitShaderModules(VkComputePipelineCreateInfo *createInfo, ShaderStages *stagesOut)
    {
        createInfo->stage.module = VK_NULL_HANDLE;
    }
};

// Creates a pipeline from stored binaries.
template <typename CreateInfoT>
VkResult CreatePipelineFromBinaries(VkDevice device,
                                    const CreateInfoT &createInfo,
                                    const PipelineBinarySet &binarySet,
                                    Pipeline *pipelineOut)
{
    ASSERT(!binarySet.binaries.empty());
    ASSERT(binarySet.binaryKeys.size() == binarySet.binaries.size());
    const uint32_t binaryCount = static_cast<uint32_t>(binarySet.binaries.size());

    std::vector<VkPipelineBinaryDataKHR> binaryData(binaryCount);
    for (uint32_t i = 0; i < binaryCount; ++i)
    {
        binaryData[i].dataSize = binarySet.binaries[i].size();
        binaryData[i].pData    = const_cast<uint8_t *>(binarySet.binaries[i].data());
    }

    // Populate the structs needed to create pipeline binary objects from binary data
    VkPipelineBinaryKeysAndDataKHR keysAndData = {};
    keysAndData.binaryCount                    = binaryCount;
    keysAndData.pPipelineBinaryKeys            = binarySet.binaryKeys.data();
    keysAndData.pPipelineBinaryData            = binaryData.data();

    VkPipelineBinaryCreateInfoKHR binaryCreateInfo = {};
    binaryCreateInfo.sType            = VK_STRUCTURE_TYPE_PIPELINE_BINARY_CREATE_INFO_KHR;
    binaryCreateInfo.pKeysAndDataInfo = &keysAndData;

    // Create the array the driver returns the pipeline binary handles in
    std::vector<VkPipelineBinaryKHR> binaries(binaryCount, VK_NULL_HANDLE);
    VkPipelineBinaryHandlesInfoKHR handlesInfo = {};
    handlesInfo.sType                          = VK_STRUCTURE_TYPE_PIPELINE_BINARY_HANDLES_INFO_KHR;
    handlesInfo.pipelineBinaryCount            = binaryCount;
    handlesInfo.pPipelineBinaries              = binaries.data();

    VkResult result =
        VK_CALL(vkCreatePipelineBinariesKHR, device, &binaryCreateInfo, nullptr, &handlesInfo);
    if (result == VK_SUCCESS)
    {
        // Use the pipeline binary handles to create the pipeline
        VkPipelineBinaryInfoKHR binaryInfo = {};
        binaryInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_BINARY_INFO_KHR;
        binaryInfo.binaryCount             = binaryCount;
        binaryInfo.pPipelineBinaries       = binaries.data();

        // The driver ignores the shader modules when given binaries.  Leaving them out makes sure
        // it can't compile them instead.
        CreateInfoT localCreateInfo = createInfo;
        typename PipelineHelperTraits<CreateInfoT>::ShaderStages stages;
        PipelineHelperTraits<CreateInfoT>::OmitShaderModules(&localCreateInfo, &stages);
        AddToPNextChain(&localCreateInfo, &binaryInfo);
        result =
            PipelineHelperTraits<CreateInfoT>::initPipeline(device, localCreateInfo, pipelineOut);
    }

    // Clean up the pipeline binaries
    for (VkPipelineBinaryKHR handle : binaries)
    {
        VK_CALL(vkDestroyPipelineBinaryKHR, device, handle, nullptr);
    }

    return result;
}

// Creates a graphics or compute pipeline.  If the blob cache has binaries for the pipeline's key,
// the pipeline is created from them.  Otherwise it is compiled, and its binaries are captured and
// stored under its key.
//
// Graphics pipeline libraries are stored like any other pipeline, under their own keys.  So are the
// pipelines linked from them, with cachePipelineBinariesForLinkedPipelines.
template <typename CreateInfoT>
VkResult PipelineBinariesCache::createPipelineWithBinaries(ErrorContext *context,
                                                           CreateInfoT *createInfo,
                                                           Pipeline *pipelineOut)
{
    VkDevice device = context->getDevice();

    // Unless the driver's linked pipelines are worth storing, they are linked from their libraries,
    // which were created from stored binaries
    if (!context->getFeatures().cachePipelineBinariesForLinkedPipelines.enabled &&
        IsLinkedFromLibraries(createInfo))
    {
        return PipelineHelperTraits<CreateInfoT>::initPipeline(device, *createInfo, pipelineOut);
    }

    VkPipelineCreateInfoKHR pipelineCreateInfo = {};
    pipelineCreateInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_CREATE_INFO_KHR;
    pipelineCreateInfo.pNext                   = createInfo;

    // Retrieve a key for this pipeline based on its createInfo
    VkPipelineBinaryKeyKHR pipelineKey = {};
    pipelineKey.sType                  = VK_STRUCTURE_TYPE_PIPELINE_BINARY_KEY_KHR;
    VkResult result = VK_CALL(vkGetPipelineKeyKHR, device, &pipelineCreateInfo, &pipelineKey);
    if (result != VK_SUCCESS)
    {
        return result;
    }
    // A key larger than the spec allows is a driver bug.  It concerns one pipeline, which is
    // created without being stored.
    if (pipelineKey.keySize > VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR)
    {
        WARN() << "vkGetPipelineKeyKHR gave a " << pipelineKey.keySize
               << " byte pipeline key, larger than the maximum of "
               << VK_MAX_PIPELINE_BINARY_KEY_SIZE_KHR;
        return PipelineHelperTraits<CreateInfoT>::initPipeline(device, *createInfo, pipelineOut);
    }

    // An all zero key is a placeholder some drivers give, shared by pipelines that differ, so
    // nothing is stored under it
    if (IsPlaceholderKey(pipelineKey))
    {
        return PipelineHelperTraits<CreateInfoT>::initPipeline(device, *createInfo, pipelineOut);
    }

    // Look up any binaries for this pipeline key
    PipelineBinarySet stored;
    if (loadBinaries(context, pipelineKey, &stored))
    {
        // We have binaries for this pipeline, try to use them
        result = CreatePipelineFromBinaries(device, *createInfo, stored, pipelineOut);
        if (result == VK_SUCCESS)
        {
            ++mCacheHits;
            return VK_SUCCESS;
        }
        WARN() << "Failed to create a pipeline from stored binaries, VkResult: " << result;
    }

    // Create the pipeline while telling the driver we want binaries back from it.  The capture bit
    // only exists in VkPipelineCreateFlags2CreateInfoKHR, which the caller chained for it.  The
    // caller doesn't use the create info again, so the bit is left set.
    VkPipelineCreateFlags2CreateInfoKHR *createFlags2 =
        reinterpret_cast<VkPipelineCreateFlags2CreateInfoKHR *>(FindInPNextChain(
            createInfo, VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO_KHR));
    ASSERT(createFlags2 != nullptr);
    createFlags2->flags |= VK_PIPELINE_CREATE_2_CAPTURE_DATA_BIT_KHR;
    result = PipelineHelperTraits<CreateInfoT>::initPipeline(device, *createInfo, pipelineOut);
    if (result != VK_SUCCESS)
    {
        return result;
    }

    // Retrieve the binaries for this pipeline
    PipelineBinarySet captured;
    const bool captureSucceeded = CaptureBinaries(device, *pipelineOut, &captured);
    ++mCacheMisses;

    if (captureSucceeded && !captured.binaries.empty())
    {
        // Persist them to disk
        captured.pipelineKey = pipelineKey;
        storeBinaries(context, captured);
    }

    // Clean up
    VkReleaseCapturedPipelineDataInfoKHR releaseInfo = {};
    releaseInfo.sType    = VK_STRUCTURE_TYPE_RELEASE_CAPTURED_PIPELINE_DATA_INFO_KHR;
    releaseInfo.pipeline = pipelineOut->getHandle();
    VK_CALL(vkReleaseCapturedPipelineDataKHR, device, &releaseInfo, nullptr);

    return VK_SUCCESS;
}

VkResult PipelineBinariesCache::createGraphicsPipeline(ErrorContext *context,
                                                       VkGraphicsPipelineCreateInfo *createInfo,
                                                       Pipeline *pipelineOut)
{
    return createPipelineWithBinaries(context, createInfo, pipelineOut);
}

VkResult PipelineBinariesCache::createComputePipeline(ErrorContext *context,
                                                      VkComputePipelineCreateInfo *createInfo,
                                                      Pipeline *pipelineOut)
{
    return createPipelineWithBinaries(context, createInfo, pipelineOut);
}

}  // namespace vk
}  // namespace rx

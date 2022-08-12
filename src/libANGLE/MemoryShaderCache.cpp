//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// MemoryShaderCache: Stores compiled shader in memory so they don't
//   always have to be re-compiled. Can be used in conjunction with the platform
//   layer to warm up the cache from disk.

#include "libANGLE/MemoryShaderCache.h"

#include <GLSLANG/ShaderVars.h>
#include <anglebase/sha1.h>

#include "common/angle_version_info.h"
#include "common/utilities.h"
#include "libANGLE/BinaryStream.h"
#include "libANGLE/Context.h"
#include "libANGLE/Debug.h"
#include "libANGLE/Uniform.h"
#include "libANGLE/histogram_macros.h"
#include "libANGLE/renderer/ShaderImpl.h"
#include "platform/PlatformMethods.h"

namespace gl
{

namespace
{
class HashStream final : angle::NonCopyable
{
  public:
    std::string str() { return mStringStream.str(); }

    template <typename T>
    HashStream &operator<<(T value)
    {
        mStringStream << value << kSeparator;
        return *this;
    }

  private:
    static constexpr char kSeparator = ':';
    std::ostringstream mStringStream;
};

HashStream &operator<<(HashStream &stream, const Shader *shader)
{
    if (shader)
    {
        stream << shader->getType() << shader->getSourceString().length()
               << shader->getSourceString().c_str();
    }
    return stream;
}

void ComputeHash(const Context *context, const Shader *shader, egl::BlobCache::Key *hashOut)
{
    // Compute the shader hash. Start with the shader hashes and resource strings.
    HashStream hashStream;
    hashStream << shader;

    // Add some ANGLE metadata and Context properties, such as version and back-end.
    hashStream << angle::GetANGLECommitHash() << context->getClientMajorVersion()
               << context->getClientMinorVersion() << context->getString(GL_RENDERER);

    // Shaders must be recompiled if these extensions have been toggled, so we include them in the
    // key.
    const std::string shaderTextureLodEXTTag =
        (context->getExtensions().shaderTextureLodEXT) ? "EXT_shader_texture_lod" : "";
    const std::string fragDepthEXTTag =
        (context->getExtensions().fragDepthEXT) ? "EXT_frag_depth" : "";
    const std::string oesStarndardDerivativesTag =
        (context->getExtensions().standardDerivativesOES) ? "OES_standard_derivatives" : "";
    hashStream << shaderTextureLodEXTTag << fragDepthEXTTag << oesStarndardDerivativesTag;

    // Call the secure SHA hashing function.
    const std::string &shaderKey = hashStream.str();
    angle::base::SHA1HashBytes(reinterpret_cast<const unsigned char *>(shaderKey.c_str()),
                               shaderKey.length(), hashOut->data());
}
}  // namespace

MemoryShaderCache::MemoryShaderCache(egl::BlobCache &blobCache) : mBlobCache(blobCache) {}

MemoryShaderCache::~MemoryShaderCache() {}

angle::Result MemoryShaderCache::getShader(const Context *context,
                                           Shader *shader,
                                           egl::BlobCache::Key *hashOut)
{
    // If caching is effectively disabled, don't bother calculating the hash.
    if (!mBlobCache.isCachingEnabled())
    {
        return angle::Result::Incomplete;
    }

    ComputeHash(context, shader, hashOut);

    angle::MemoryBuffer uncompressedData;
    switch (mBlobCache.getAndDecompress(context->getScratchBuffer(), *hashOut, &uncompressedData))
    {
        case egl::BlobCache::GetAndDecompressResult::DecompressFailure:
            ANGLE_PERF_WARNING(context->getState().getDebug(), GL_DEBUG_SEVERITY_LOW,
                               "Error decompressing shader binary data from cache.");
            return angle::Result::Incomplete;

        case egl::BlobCache::GetAndDecompressResult::NotFound:
            return angle::Result::Incomplete;

        case egl::BlobCache::GetAndDecompressResult::GetSuccess:
            angle::Result result = shader->loadBinary(context, uncompressedData.data(),
                                                      static_cast<int>(uncompressedData.size()));

            {
                std::scoped_lock<std::mutex> lock(mHistogramMutex);
                ANGLE_HISTOGRAM_BOOLEAN("GPU.ANGLE.ShaderCache.LoadBinarySuccess",
                                        result == angle::Result::Continue);
            }
            ANGLE_TRY(result);

            if (result == angle::Result::Continue)
                return angle::Result::Continue;

            // Cache load failed, evict.
            ANGLE_PERF_WARNING(context->getState().getDebug(), GL_DEBUG_SEVERITY_LOW,
                               "Failed to load shader binary from cache.");
            mBlobCache.remove(*hashOut);
            return angle::Result::Incomplete;
    }

    UNREACHABLE();
    return angle::Result::Incomplete;
}

angle::Result MemoryShaderCache::putShader(const Context *context,
                                           const egl::BlobCache::Key &shaderHash,
                                           const Shader *shader)
{
    // If caching is effectively disabled, don't bother serializing the shader.
    if (!mBlobCache.isCachingEnabled())
    {
        return angle::Result::Incomplete;
    }

    angle::MemoryBuffer serializedShader;
    ANGLE_TRY(shader->serialize(nullptr, &serializedShader));

    size_t compressedSize;
    if (!mBlobCache.compressAndPut(shaderHash, std::move(serializedShader), &compressedSize))
    {
        ANGLE_PERF_WARNING(context->getState().getDebug(), GL_DEBUG_SEVERITY_LOW,
                           "Error compressing shader binary data for insertion into cache.");
        return angle::Result::Incomplete;
    }

    {
        std::scoped_lock<std::mutex> lock(mHistogramMutex);
        ANGLE_HISTOGRAM_COUNTS("GPU.ANGLE.ShaderCache.ShaderBinarySizeBytes",
                               static_cast<int>(compressedSize));
    }

    return angle::Result::Continue;
}

void MemoryShaderCache::clear()
{
    mBlobCache.clear();
}

size_t MemoryShaderCache::maxSize() const
{
    return mBlobCache.maxSize();
};

}  // namespace gl

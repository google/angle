//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_format_utils:
//   Helper for Vulkan format code.

#include "libANGLE/renderer/vulkan/vk_format_utils.h"

#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/load_functions_table.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_caps_utils.h"

namespace rx
{
namespace
{
void AddSampleCounts(VkSampleCountFlags sampleCounts, gl::SupportedSampleSet *outSet)
{
    // The possible bits are VK_SAMPLE_COUNT_n_BIT = n, with n = 1 << b.  At the time of this
    // writing, b is in [0, 6], however, we test all 32 bits in case the enum is extended.
    for (unsigned int i = 0; i < 32; ++i)
    {
        if ((sampleCounts & (1 << i)) != 0)
        {
            outSet->insert(1 << i);
        }
    }
}

void FillTextureFormatCaps(RendererVk *renderer, VkFormat format, gl::TextureCaps *outTextureCaps)
{
    const VkPhysicalDeviceLimits &physicalDeviceLimits =
        renderer->getPhysicalDeviceProperties().limits;
    bool hasColorAttachmentFeatureBit =
        renderer->hasTextureFormatFeatureBits(format, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    bool hasDepthAttachmentFeatureBit = renderer->hasTextureFormatFeatureBits(
        format, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    outTextureCaps->texturable =
        renderer->hasTextureFormatFeatureBits(format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT);
    outTextureCaps->filterable = renderer->hasTextureFormatFeatureBits(
        format, VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT);
    outTextureCaps->textureAttachment =
        hasColorAttachmentFeatureBit || hasDepthAttachmentFeatureBit;
    outTextureCaps->renderbuffer = outTextureCaps->textureAttachment;

    if (outTextureCaps->renderbuffer)
    {
        if (hasColorAttachmentFeatureBit)
        {
            AddSampleCounts(physicalDeviceLimits.framebufferColorSampleCounts,
                            &outTextureCaps->sampleCounts);
        }
        if (hasDepthAttachmentFeatureBit)
        {
            AddSampleCounts(physicalDeviceLimits.framebufferDepthSampleCounts,
                            &outTextureCaps->sampleCounts);
            AddSampleCounts(physicalDeviceLimits.framebufferStencilSampleCounts,
                            &outTextureCaps->sampleCounts);
        }
    }
}

bool HasFullTextureFormatSupport(RendererVk *renderer, VkFormat vkFormat)
{
    constexpr uint32_t kBitsColor = VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
                                    VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
                                    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    constexpr uint32_t kBitsDepth = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return renderer->hasTextureFormatFeatureBits(vkFormat, kBitsColor) ||
           renderer->hasTextureFormatFeatureBits(vkFormat, kBitsDepth);
}

bool HasFullBufferFormatSupport(RendererVk *renderer, VkFormat vkFormat)
{
    return renderer->hasBufferFormatFeatureBits(vkFormat, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT);
}

using SupportTest = bool (*)(RendererVk *renderer, VkFormat vkFormat);

template <class FormatInitInfo>
int FindSupportedFormat(RendererVk *renderer,
                        const FormatInitInfo *info,
                        int numInfo,
                        SupportTest hasSupport)
{
    ASSERT(numInfo > 0);
    const int last = numInfo - 1;

    for (int i = 0; i < last; ++i)
    {
        ASSERT(info[i].format != angle::FormatID::NONE);
        if (hasSupport(renderer, info[i].vkFormat))
            return i;
    }

    // List must contain a supported item.  We failed on all the others so the last one must be it.
    ASSERT(info[last].format != angle::FormatID::NONE);
    ASSERT(hasSupport(renderer, info[last].vkFormat));
    return last;
}

}  // anonymous namespace

namespace vk
{

// Format implementation.
Format::Format()
    : angleFormatID(angle::FormatID::NONE),
      internalFormat(GL_NONE),
      textureFormatID(angle::FormatID::NONE),
      vkTextureFormat(VK_FORMAT_UNDEFINED),
      bufferFormatID(angle::FormatID::NONE),
      vkBufferFormat(VK_FORMAT_UNDEFINED),
      textureInitializerFunction(nullptr),
      textureLoadFunctions(),
      vertexLoadRequiresConversion(false),
      vkBufferFormatIsPacked(false)
{}

void Format::initTextureFallback(RendererVk *renderer,
                                 const TextureFormatInitInfo *info,
                                 int numInfo)
{
    size_t skip = renderer->getFeatures().forceFallbackFormat ? 1 : 0;
    int i = FindSupportedFormat(renderer, info + skip, numInfo - skip, HasFullTextureFormatSupport);
    i += skip;

    textureFormatID            = info[i].format;
    vkTextureFormat            = info[i].vkFormat;
    textureInitializerFunction = info[i].initializer;
}

void Format::initBufferFallback(RendererVk *renderer, const BufferFormatInitInfo *info, int numInfo)
{
    int i          = FindSupportedFormat(renderer, info, numInfo, HasFullBufferFormatSupport);
    bufferFormatID = info[i].format;
    vkBufferFormat = info[i].vkFormat;
    vkBufferFormatIsPacked       = info[i].vkFormatIsPacked;
    vertexLoadFunction           = info[i].vertexLoadFunction;
    vertexLoadRequiresConversion = info[i].vertexLoadRequiresConversion;
}

const angle::Format &Format::textureFormat() const
{
    return angle::Format::Get(textureFormatID);
}

const angle::Format &Format::bufferFormat() const
{
    return angle::Format::Get(bufferFormatID);
}

const angle::Format &Format::angleFormat() const
{
    return angle::Format::Get(angleFormatID);
}

bool operator==(const Format &lhs, const Format &rhs)
{
    return &lhs == &rhs;
}

bool operator!=(const Format &lhs, const Format &rhs)
{
    return &lhs != &rhs;
}

// FormatTable implementation.
FormatTable::FormatTable() {}

FormatTable::~FormatTable() {}

void FormatTable::initialize(RendererVk *renderer,
                             gl::TextureCapsMap *outTextureCapsMap,
                             std::vector<GLenum> *outCompressedTextureFormats)
{
    for (size_t formatIndex = 0; formatIndex < angle::kNumANGLEFormats; ++formatIndex)
    {
        vk::Format &format               = mFormatData[formatIndex];
        const auto formatID              = static_cast<angle::FormatID>(formatIndex);
        const angle::Format &angleFormat = angle::Format::Get(formatID);

        format.initialize(renderer, angleFormat);
        const GLenum internalFormat = format.internalFormat;
        format.textureLoadFunctions = GetLoadFunctionsMap(internalFormat, format.textureFormatID);
        format.angleFormatID        = formatID;

        if (!format.valid())
        {
            continue;
        }

        gl::TextureCaps textureCaps;
        FillTextureFormatCaps(renderer, format.vkTextureFormat, &textureCaps);
        outTextureCapsMap->set(formatID, textureCaps);

        if (angleFormat.isBlock)
        {
            outCompressedTextureFormats->push_back(internalFormat);
        }
    }
}

const Format &FormatTable::operator[](GLenum internalFormat) const
{
    angle::FormatID formatID = angle::Format::InternalFormatToID(internalFormat);
    return mFormatData[static_cast<size_t>(formatID)];
}

const Format &FormatTable::operator[](angle::FormatID formatID) const
{
    return mFormatData[static_cast<size_t>(formatID)];
}

}  // namespace vk

size_t GetVertexInputAlignment(const vk::Format &format)
{
    const angle::Format &bufferFormat = format.bufferFormat();
    size_t pixelBytes                 = bufferFormat.pixelBytes;
    return format.vkBufferFormatIsPacked ? pixelBytes : (pixelBytes / bufferFormat.channelCount());
}
}  // namespace rx

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
#include "libANGLE/renderer/vulkan/vk_caps_utils.h"

namespace rx
{
namespace
{
constexpr VkFormatFeatureFlags kNecessaryBitsFullSupportDepthStencil =
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
    VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
constexpr VkFormatFeatureFlags kNecessaryBitsFullSupportColor =
    VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
    VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;

bool HasFormatFeatureBits(const VkFormatFeatureFlags featureBits,
                          const VkFormatProperties &formatProperties)
{
    return IsMaskFlagSet(formatProperties.optimalTilingFeatures, featureBits);
}

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

void FillTextureFormatCaps(const VkPhysicalDeviceLimits &physicalDeviceLimits,
                           const VkFormatProperties &formatProperties,
                           gl::TextureCaps *outTextureCaps)
{
    outTextureCaps->texturable =
        HasFormatFeatureBits(VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT, formatProperties);
    outTextureCaps->filterable =
        HasFormatFeatureBits(VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, formatProperties);
    outTextureCaps->textureAttachment =
        HasFormatFeatureBits(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, formatProperties) ||
        HasFormatFeatureBits(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, formatProperties);
    outTextureCaps->renderbuffer = outTextureCaps->textureAttachment;

    if (outTextureCaps->renderbuffer)
    {
        if (HasFormatFeatureBits(VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT, formatProperties))
        {
            AddSampleCounts(physicalDeviceLimits.framebufferColorSampleCounts,
                            &outTextureCaps->sampleCounts);
        }
        if (HasFormatFeatureBits(VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT, formatProperties))
        {
            AddSampleCounts(physicalDeviceLimits.framebufferDepthSampleCounts,
                            &outTextureCaps->sampleCounts);
            AddSampleCounts(physicalDeviceLimits.framebufferStencilSampleCounts,
                            &outTextureCaps->sampleCounts);
        }
    }
}

bool HasFullTextureFormatSupport(VkPhysicalDevice physicalDevice, VkFormat vkFormat)
{
    VkFormatProperties formatProperties;
    vk::GetFormatProperties(physicalDevice, vkFormat, &formatProperties);

    constexpr uint32_t kBitsColor =
        (VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT |
         VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
    constexpr uint32_t kBitsDepth = (VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    return HasFormatFeatureBits(kBitsColor, formatProperties) ||
           HasFormatFeatureBits(kBitsDepth, formatProperties);
}

bool HasFullBufferFormatSupport(VkPhysicalDevice physicalDevice, VkFormat vkFormat)
{
    VkFormatProperties formatProperties;
    vk::GetFormatProperties(physicalDevice, vkFormat, &formatProperties);
    return formatProperties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
}

using SupportTest = bool (*)(VkPhysicalDevice physicalDevice, VkFormat vkFormat);

template <class FormatInitInfo>
int FindSupportedFormat(VkPhysicalDevice physicalDevice,
                        const FormatInitInfo *info,
                        int numInfo,
                        SupportTest hasSupport)
{
    ASSERT(numInfo > 0);
    const int last = numInfo - 1;

    for (int i = 0; i < last; ++i)
    {
        ASSERT(info[i].format != angle::FormatID::NONE);
        if (hasSupport(physicalDevice, info[i].vkFormat))
            return i;
    }

    // List must contain a supported item.  We failed on all the others so the last one must be it.
    ASSERT(info[last].format != angle::FormatID::NONE);
    ASSERT(hasSupport(physicalDevice, info[last].vkFormat));
    return last;
}

}  // anonymous namespace

namespace vk
{

void GetFormatProperties(VkPhysicalDevice physicalDevice,
                         VkFormat vkFormat,
                         VkFormatProperties *propertiesOut)
{
    // Try filling out the info from our hard coded format data, if we can't find the
    // information we need, we'll make the call to Vulkan.
    const VkFormatProperties &formatProperties = vk::GetMandatoryFormatSupport(vkFormat);

    // Once we filled what we could with the mandatory texture caps, we verify if
    // all the bits we need to satify all our checks are present, and if so we can
    // skip the device call.
    if (!IsMaskFlagSet(formatProperties.optimalTilingFeatures, kNecessaryBitsFullSupportColor) &&
        !IsMaskFlagSet(formatProperties.optimalTilingFeatures,
                       kNecessaryBitsFullSupportDepthStencil))
    {
        vkGetPhysicalDeviceFormatProperties(physicalDevice, vkFormat, propertiesOut);
    }
    else
    {
        *propertiesOut = formatProperties;
    }
}

// Format implementation.
Format::Format()
    : angleFormatID(angle::FormatID::NONE),
      internalFormat(GL_NONE),
      textureFormatID(angle::FormatID::NONE),
      vkTextureFormat(VK_FORMAT_UNDEFINED),
      bufferFormatID(angle::FormatID::NONE),
      vkBufferFormat(VK_FORMAT_UNDEFINED),
      vkBufferFormatIsPacked(false),
      textureInitializerFunction(nullptr),
      textureLoadFunctions()
{
}

void Format::initTextureFallback(VkPhysicalDevice physicalDevice,
                                 const TextureFormatInitInfo *info,
                                 int numInfo,
                                 const angle::FeaturesVk &featuresVk)
{
    size_t skip = featuresVk.forceFallbackFormat ? 1 : 0;
    int i       = FindSupportedFormat(physicalDevice, info + skip, numInfo - skip,
                                HasFullTextureFormatSupport);
    i += skip;

    textureFormatID            = info[i].format;
    vkTextureFormat            = info[i].vkFormat;
    textureInitializerFunction = info[i].initializer;
}

void Format::initBufferFallback(VkPhysicalDevice physicalDevice,
                                const BufferFormatInitInfo *info,
                                int numInfo)
{
    int i          = FindSupportedFormat(physicalDevice, info, numInfo, HasFullBufferFormatSupport);
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
FormatTable::FormatTable()
{
}

FormatTable::~FormatTable()
{
}

void FormatTable::initialize(VkPhysicalDevice physicalDevice,
                             const VkPhysicalDeviceProperties &physicalDeviceProperties,
                             const angle::FeaturesVk &featuresVk,
                             gl::TextureCapsMap *outTextureCapsMap,
                             std::vector<GLenum> *outCompressedTextureFormats)
{
    for (size_t formatIndex = 0; formatIndex < angle::kNumANGLEFormats; ++formatIndex)
    {
        const auto formatID              = static_cast<angle::FormatID>(formatIndex);
        const angle::Format &angleFormat = angle::Format::Get(formatID);
        mFormatData[formatIndex].initialize(physicalDevice, angleFormat, featuresVk);
        const GLenum internalFormat = mFormatData[formatIndex].internalFormat;
        mFormatData[formatIndex].textureLoadFunctions =
            GetLoadFunctionsMap(internalFormat, mFormatData[formatIndex].textureFormatID);
        mFormatData[formatIndex].angleFormatID = formatID;

        if (!mFormatData[formatIndex].valid())
        {
            continue;
        }

        const VkFormat vkFormat = mFormatData[formatIndex].vkTextureFormat;

        // Try filling out the info from our hard coded format data, if we can't find the
        // information we need, we'll make the call to Vulkan.
        VkFormatProperties formatProperties;
        GetFormatProperties(physicalDevice, vkFormat, &formatProperties);
        gl::TextureCaps textureCaps;
        FillTextureFormatCaps(physicalDeviceProperties.limits, formatProperties, &textureCaps);
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
    size_t pixelBytes = bufferFormat.pixelBytes;
    return format.vkBufferFormatIsPacked ? pixelBytes : (pixelBytes / bufferFormat.channelCount());
}
}  // namespace rx

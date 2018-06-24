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

void FillTextureFormatCaps(const VkFormatProperties &formatProperties,
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
    : angleFormatID(angle::Format::ID::NONE),
      internalFormat(GL_NONE),
      textureFormatID(angle::Format::ID::NONE),
      vkTextureFormat(VK_FORMAT_UNDEFINED),
      bufferFormatID(angle::Format::ID::NONE),
      vkBufferFormat(VK_FORMAT_UNDEFINED),
      textureInitializerFunction(nullptr),
      textureLoadFunctions()
{
}

void Format::initTextureFallback(VkPhysicalDevice physicalDevice,
                                 angle::Format::ID format,
                                 VkFormat vkFormat,
                                 InitializeTextureDataFunction initializer,
                                 angle::Format::ID fallbackFormat,
                                 VkFormat fallbackVkFormat,
                                 InitializeTextureDataFunction fallbackInitializer)
{
    ASSERT(format != angle::Format::ID::NONE);
    ASSERT(fallbackFormat != angle::Format::ID::NONE);

    if (HasFullTextureFormatSupport(physicalDevice, vkFormat))
    {
        textureFormatID            = format;
        vkTextureFormat            = vkFormat;
        textureInitializerFunction = initializer;
    }
    else
    {
        textureFormatID            = fallbackFormat;
        vkTextureFormat            = fallbackVkFormat;
        textureInitializerFunction = fallbackInitializer;
        ASSERT(HasFullTextureFormatSupport(physicalDevice, vkTextureFormat));
    }
}

void Format::initBufferFallback(VkPhysicalDevice physicalDevice,
                                angle::Format::ID format,
                                VkFormat vkFormat,
                                angle::Format::ID fallbackFormat,
                                VkFormat fallbackVkFormat)
{
    ASSERT(format != angle::Format::ID::NONE);
    ASSERT(fallbackFormat != angle::Format::ID::NONE);
    UNIMPLEMENTED();
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
                             gl::TextureCapsMap *outTextureCapsMap,
                             std::vector<GLenum> *outCompressedTextureFormats)
{
    for (size_t formatIndex = 0; formatIndex < angle::kNumANGLEFormats; ++formatIndex)
    {
        const auto formatID              = static_cast<angle::Format::ID>(formatIndex);
        const angle::Format &angleFormat = angle::Format::Get(formatID);
        mFormatData[formatIndex].initialize(physicalDevice, angleFormat);
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
        FillTextureFormatCaps(formatProperties, &textureCaps);
        outTextureCapsMap->set(formatID, textureCaps);

        if (angleFormat.isBlock)
        {
            outCompressedTextureFormats->push_back(internalFormat);
        }
    }
}

const Format &FormatTable::operator[](GLenum internalFormat) const
{
    angle::Format::ID formatID = angle::Format::InternalFormatToID(internalFormat);
    return mFormatData[static_cast<size_t>(formatID)];
}

const Format &FormatTable::operator[](angle::Format::ID formatID) const
{
    return mFormatData[static_cast<size_t>(formatID)];
}

}  // namespace vk

}  // namespace rx

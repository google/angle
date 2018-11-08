//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_format_utils:
//   Helper for Vulkan format code.

#ifndef LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_
#define LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

#include <vulkan/vulkan.h>

#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/Format.h"
#include "libANGLE/renderer/copyvertex.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "platform/FeaturesVk.h"

#include <array>

namespace gl
{
class TextureCapsMap;
}  // namespace gl

namespace rx
{

namespace vk
{

void GetFormatProperties(VkPhysicalDevice physicalDevice,
                         VkFormat vkFormat,
                         VkFormatProperties *propertiesOut);

struct TextureFormatInitInfo final
{
    angle::FormatID format;
    VkFormat vkFormat;
    InitializeTextureDataFunction initializer;
};

struct BufferFormatInitInfo final
{
    angle::FormatID format;
    VkFormat vkFormat;
    bool vkFormatIsPacked;
    VertexCopyFunction vertexLoadFunction;
    bool vertexLoadRequiresConversion;
};

struct Format final : private angle::NonCopyable
{
    Format();

    bool valid() const { return internalFormat != 0; }

    // This is an auto-generated method in vk_format_table_autogen.cpp.
    void initialize(VkPhysicalDevice physicalDevice,
                    const angle::Format &angleFormat,
                    const angle::FeaturesVk &featuresVk);

    void initTextureFallback(VkPhysicalDevice physicalDevice,
                             const TextureFormatInitInfo *info,
                             int numInfo,
                             const angle::FeaturesVk &featuresVk);
    void initBufferFallback(VkPhysicalDevice physicalDevice,
                            const BufferFormatInitInfo *info,
                            int numInfo);

    const angle::Format &angleFormat() const;
    const angle::Format &textureFormat() const;
    const angle::Format &bufferFormat() const;

    angle::FormatID angleFormatID;
    GLenum internalFormat;
    angle::FormatID textureFormatID;
    VkFormat vkTextureFormat;
    angle::FormatID bufferFormatID;
    VkFormat vkBufferFormat;
    bool vkBufferFormatIsPacked;
    InitializeTextureDataFunction textureInitializerFunction;
    LoadFunctionMap textureLoadFunctions;
    VertexCopyFunction vertexLoadFunction;
    bool vertexLoadRequiresConversion;
};

bool operator==(const Format &lhs, const Format &rhs);
bool operator!=(const Format &lhs, const Format &rhs);

class FormatTable final : angle::NonCopyable
{
  public:
    FormatTable();
    ~FormatTable();

    // Also initializes the TextureCapsMap and the compressedTextureCaps in the Caps instance.
    void initialize(VkPhysicalDevice physicalDevice,
                    const VkPhysicalDeviceProperties &physicalDeviceProperties,
                    const angle::FeaturesVk &featuresVk,
                    gl::TextureCapsMap *outTextureCapsMap,
                    std::vector<GLenum> *outCompressedTextureFormats);

    const Format &operator[](GLenum internalFormat) const;
    const Format &operator[](angle::FormatID formatID) const;

  private:
    // The table data is indexed by angle::FormatID.
    std::array<Format, angle::kNumANGLEFormats> mFormatData;
};

// This will return a reference to a VkFormatProperties with the feature flags supported
// if the format is a mandatory format described in section 31.3.3. Required Format Support
// of the Vulkan spec. If the vkFormat isn't mandatory, it will return a VkFormatProperties
// initialized to 0.
const VkFormatProperties &GetMandatoryFormatSupport(VkFormat vkFormat);

}  // namespace vk

// Returns the alignment for a buffer to be used with the vertex input stage in Vulkan. This
// calculation is listed in the Vulkan spec at the end of the section 'Vertex Input Description'.
size_t GetVertexInputAlignment(const vk::Format &format);
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

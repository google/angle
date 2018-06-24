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

struct Format final : private angle::NonCopyable
{
    Format();

    bool valid() const { return internalFormat != 0; }

    // This is an auto-generated method in vk_format_table_autogen.cpp.
    void initialize(VkPhysicalDevice physicalDevice, const angle::Format &angleFormat);

    void initTextureFallback(VkPhysicalDevice physicalDevice,
                             angle::Format::ID format,
                             VkFormat vkFormat,
                             InitializeTextureDataFunction initializer,
                             angle::Format::ID fallbackFormat,
                             VkFormat fallbackVkFormat,
                             InitializeTextureDataFunction fallbackInitializer);

    void initBufferFallback(VkPhysicalDevice physicalDevice,
                            angle::Format::ID format,
                            VkFormat vkFormat,
                            VertexCopyFunction function,
                            bool functionConverts,
                            angle::Format::ID fallbackFormat,
                            VkFormat fallbackVkFormat,
                            VertexCopyFunction fallbackFunction);

    const angle::Format &textureFormat() const;
    const angle::Format &bufferFormat() const;
    const angle::Format &angleFormat() const;

    angle::Format::ID angleFormatID;
    GLenum internalFormat;
    angle::Format::ID textureFormatID;
    VkFormat vkTextureFormat;
    angle::Format::ID bufferFormatID;
    VkFormat vkBufferFormat;
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
                    gl::TextureCapsMap *outTextureCapsMap,
                    std::vector<GLenum> *outCompressedTextureFormats);

    const Format &operator[](GLenum internalFormat) const;
    const Format &operator[](angle::Format::ID formatID) const;

  private:
    // The table data is indexed by angle::Format::ID.
    std::array<Format, angle::kNumANGLEFormats> mFormatData;
};

// This will return a reference to a VkFormatProperties with the feature flags supported
// if the format is a mandatory format described in section 31.3.3. Required Format Support
// of the Vulkan spec. If the vkFormat isn't mandatory, it will return a VkFormatProperties
// initialized to 0.
const VkFormatProperties &GetMandatoryFormatSupport(VkFormat vkFormat);

}  // namespace vk

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

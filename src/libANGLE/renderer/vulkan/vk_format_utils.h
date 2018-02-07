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

struct Format final : private angle::NonCopyable
{
    Format();

    bool valid() const { return internalFormat != 0; }

    // This is an auto-generated method in vk_format_table_autogen.cpp.
    void initialize(VkPhysicalDevice physicalDevice, const angle::Format &angleFormat);

    const angle::Format &textureFormat() const;
    const angle::Format &bufferFormat() const;

    GLenum internalFormat;
    angle::Format::ID textureFormatID;
    VkFormat vkTextureFormat;
    angle::Format::ID bufferFormatID;
    VkFormat vkBufferFormat;
    InitializeTextureDataFunction dataInitializerFunction;
    LoadFunctionMap loadFunctions;
};

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

  private:
    // The table data is indexed by angle::Format::ID.
    std::array<Format, angle::kNumANGLEFormats> mFormatData;
};

// TODO(jmadill): This is temporary. Figure out how to handle format conversions.
VkFormat GetNativeVertexFormat(gl::VertexFormatType vertexFormat);

}  // namespace vk

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_format_utils:
//   Helper for Vulkan format code.

#ifndef LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_
#define LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

#include "common/vulkan/vk_headers.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/Format.h"
#include "libANGLE/renderer/copyvertex.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "platform/FeaturesVk.h"

#include <array>

namespace gl
{
struct SwizzleState;
class TextureCapsMap;
}  // namespace gl

namespace rx
{
class RendererVk;
class ContextVk;

namespace vk
{
// VkFormat values in range [0, kNumVkFormats) are used as indices in various tables.
constexpr uint32_t kNumVkFormats = 185;

struct ImageFormatInitInfo final
{
    angle::FormatID format;
    InitializeTextureDataFunction initializer;
};

struct BufferFormatInitInfo final
{
    angle::FormatID format;
    bool vkFormatIsPacked;
    VertexCopyFunction vertexLoadFunction;
    bool vertexLoadRequiresConversion;
};

VkFormat GetVkFormatFromFormatID(angle::FormatID formatID);
angle::FormatID GetFormatIDFromVkFormat(VkFormat vkFormat);

// Describes a Vulkan format. For more information on formats in the Vulkan back-end please see
// https://chromium.googlesource.com/angle/angle/+/master/src/libANGLE/renderer/vulkan/doc/FormatTablesAndEmulation.md
struct Format final : private angle::NonCopyable
{
    Format();

    bool valid() const { return intendedGLFormat != 0; }

    // The intended format is the front-end format. For Textures this usually correponds to a
    // GLenum in the headers. Buffer formats don't always have a corresponding GLenum type.
    // Some Surface formats and unsized types also don't have a corresponding GLenum.
    const angle::Format &intendedFormat() const { return angle::Format::Get(intendedFormatID); }

    // The actual Image format is used to implement the front-end format for Texture/Renderbuffers.
    const angle::Format &actualImageFormat() const
    {
        return angle::Format::Get(actualImageFormatID);
    }

    VkFormat actualImageVkFormat() const { return GetVkFormatFromFormatID(actualImageFormatID); }

    // The actual Buffer format is used to implement the front-end format for Buffers.  This format
    // is used by vertex buffers as well as texture buffers.  Note that all formats required for
    // GL_EXT_texture_buffer have mandatory support for vertex buffers in Vulkan, so they won't be
    // using an emulated format.
    const angle::Format &actualBufferFormat(bool compressed) const
    {
        return angle::Format::Get(compressed ? actualCompressedBufferFormatID
                                             : actualBufferFormatID);
    }

    VkFormat actualBufferVkFormat(bool compressed) const
    {
        return GetVkFormatFromFormatID(compressed ? actualCompressedBufferFormatID
                                                  : actualBufferFormatID);
    }

    VertexCopyFunction getVertexLoadFunction(bool compressed) const
    {
        return compressed ? compressedVertexLoadFunction : vertexLoadFunction;
    }

    bool getVertexLoadRequiresConversion(bool compressed) const
    {
        return compressed ? compressedVertexLoadRequiresConversion : vertexLoadRequiresConversion;
    }

    // |intendedGLFormat| always correponds to a valid GLenum type. For types that don't have a
    // corresponding GLenum we do our best to specify a GLenum that is "close".
    const gl::InternalFormat &getInternalFormatInfo(GLenum type) const
    {
        return gl::GetInternalFormatInfo(intendedGLFormat, type);
    }

    // Returns buffer alignment for image-copy operations (to or from a buffer).
    size_t getImageCopyBufferAlignment() const;
    size_t getValidImageCopyBufferAlignment() const;

    // Returns true if the image format has more channels than the ANGLE format.
    bool hasEmulatedImageChannels() const;

    // Returns true if the image has a different image format than intended.
    bool hasEmulatedImageFormat() const { return actualImageFormatID != intendedFormatID; }

    // This is an auto-generated method in vk_format_table_autogen.cpp.
    void initialize(RendererVk *renderer, const angle::Format &angleFormat);

    // These are used in the format table init.
    void initImageFallback(RendererVk *renderer, const ImageFormatInitInfo *info, int numInfo);
    void initBufferFallback(RendererVk *renderer,
                            const BufferFormatInitInfo *fallbackInfo,
                            int numInfo,
                            int compressedStartIndex);

    angle::FormatID intendedFormatID;
    GLenum intendedGLFormat;
    angle::FormatID actualImageFormatID;
    angle::FormatID actualBufferFormatID;
    angle::FormatID actualCompressedBufferFormatID;

    InitializeTextureDataFunction imageInitializerFunction;
    LoadFunctionMap textureLoadFunctions;
    LoadTextureBorderFunctionMap textureBorderLoadFunctions;
    VertexCopyFunction vertexLoadFunction;
    VertexCopyFunction compressedVertexLoadFunction;

    bool vertexLoadRequiresConversion;
    bool compressedVertexLoadRequiresConversion;
    bool vkBufferFormatIsPacked;
    bool vkCompressedBufferFormatIsPacked;
    bool vkFormatIsInt;
    bool vkFormatIsUnsigned;
};

bool operator==(const Format &lhs, const Format &rhs);
bool operator!=(const Format &lhs, const Format &rhs);

class FormatTable final : angle::NonCopyable
{
  public:
    FormatTable();
    ~FormatTable();

    // Also initializes the TextureCapsMap and the compressedTextureCaps in the Caps instance.
    void initialize(RendererVk *renderer,
                    gl::TextureCapsMap *outTextureCapsMap,
                    std::vector<GLenum> *outCompressedTextureFormats);

    ANGLE_INLINE const Format &operator[](GLenum internalFormat) const
    {
        angle::FormatID formatID = angle::Format::InternalFormatToID(internalFormat);
        return mFormatData[static_cast<size_t>(formatID)];
    }

    ANGLE_INLINE const Format &operator[](angle::FormatID formatID) const
    {
        return mFormatData[static_cast<size_t>(formatID)];
    }

  private:
    // The table data is indexed by angle::FormatID.
    std::array<Format, angle::kNumANGLEFormats> mFormatData;
};

// This will return a reference to a VkFormatProperties with the feature flags supported
// if the format is a mandatory format described in section 31.3.3. Required Format Support
// of the Vulkan spec. If the vkFormat isn't mandatory, it will return a VkFormatProperties
// initialized to 0.
const VkFormatProperties &GetMandatoryFormatSupport(angle::FormatID formatID);

VkImageUsageFlags GetMaximalImageUsageFlags(RendererVk *renderer, angle::FormatID formatID);

}  // namespace vk

// Checks if a Vulkan format supports all the features needed to use it as a GL texture format.
bool HasFullTextureFormatSupport(RendererVk *renderer, angle::FormatID formatID);
// Checks if a Vulkan format supports all the features except rendering.
bool HasNonRenderableTextureFormatSupport(RendererVk *renderer, angle::FormatID formatID);

// Returns the alignment for a buffer to be used with the vertex input stage in Vulkan. This
// calculation is listed in the Vulkan spec at the end of the section 'Vertex Input Description'.
size_t GetVertexInputAlignment(const vk::Format &format, bool compressed);

// Get the swizzle state based on format's requirements and emulations.
gl::SwizzleState GetFormatSwizzle(const ContextVk *contextVk,
                                  const vk::Format &format,
                                  const bool sized);

// Apply application's swizzle to the swizzle implied by format as received from GetFormatSwizzle.
gl::SwizzleState ApplySwizzle(const gl::SwizzleState &formatSwizzle,
                              const gl::SwizzleState &toApply);

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_VK_FORMAT_UTILS_H_

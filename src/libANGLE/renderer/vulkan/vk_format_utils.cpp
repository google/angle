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
#include "vk_caps_utils.h"

namespace rx
{

namespace vk
{

Format::Format()
    : internalFormat(GL_NONE),
      textureFormatID(angle::Format::ID::NONE),
      vkTextureFormat(VK_FORMAT_UNDEFINED),
      bufferFormatID(angle::Format::ID::NONE),
      vkBufferFormat(VK_FORMAT_UNDEFINED),
      dataInitializerFunction(nullptr),
      loadFunctions()
{
}

const angle::Format &Format::textureFormat() const
{
    return angle::Format::Get(textureFormatID);
}

const angle::Format &Format::bufferFormat() const
{
    return angle::Format::Get(bufferFormatID);
}

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
        const angle::Format::ID formatID = static_cast<angle::Format::ID>(formatIndex);
        const angle::Format &angleFormat = angle::Format::Get(formatID);
        mFormatData[formatIndex].initialize(physicalDevice, angleFormat);
        const GLenum internalFormat = mFormatData[formatIndex].internalFormat;
        mFormatData[formatIndex].loadFunctions =
            GetLoadFunctionsMap(internalFormat, mFormatData[formatIndex].textureFormatID);

        if (!mFormatData[formatIndex].valid())
        {
            // TODO(lucferron): Implement support for more OpenGL Texture formats
            // http://anglebug.com/2358
            continue;
        }
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(
            physicalDevice, mFormatData[formatIndex].vkTextureFormat, &formatProperties);

        const gl::TextureCaps textureCaps = GenerateTextureFormatCaps(formatProperties);
        outTextureCapsMap->set(formatID, textureCaps);

        // TODO(lucferron): Optimize this by including compressed bool in the FormatID
        // http://anglebug.com/2358
        if (gl::GetSizedInternalFormatInfo(internalFormat).compressed)
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

// TODO(jmadill): This is temporary. Figure out how to handle format conversions.
VkFormat GetNativeVertexFormat(gl::VertexFormatType vertexFormat)
{
    switch (vertexFormat)
    {
        case gl::VERTEX_FORMAT_INVALID:
            UNREACHABLE();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SBYTE1:
            return VK_FORMAT_R8_SINT;
        case gl::VERTEX_FORMAT_SBYTE1_NORM:
            return VK_FORMAT_R8_SNORM;
        case gl::VERTEX_FORMAT_SBYTE2:
            return VK_FORMAT_R8G8_SINT;
        case gl::VERTEX_FORMAT_SBYTE2_NORM:
            return VK_FORMAT_R8G8_SNORM;
        case gl::VERTEX_FORMAT_SBYTE3:
            return VK_FORMAT_R8G8B8_SINT;
        case gl::VERTEX_FORMAT_SBYTE3_NORM:
            return VK_FORMAT_R8G8B8_SNORM;
        case gl::VERTEX_FORMAT_SBYTE4:
            return VK_FORMAT_R8G8B8A8_SINT;
        case gl::VERTEX_FORMAT_SBYTE4_NORM:
            return VK_FORMAT_R8G8B8A8_SNORM;
        case gl::VERTEX_FORMAT_UBYTE1:
            return VK_FORMAT_R8_UINT;
        case gl::VERTEX_FORMAT_UBYTE1_NORM:
            return VK_FORMAT_R8_UNORM;
        case gl::VERTEX_FORMAT_UBYTE2:
            return VK_FORMAT_R8G8_UINT;
        case gl::VERTEX_FORMAT_UBYTE2_NORM:
            return VK_FORMAT_R8G8_UNORM;
        case gl::VERTEX_FORMAT_UBYTE3:
            return VK_FORMAT_R8G8B8_UINT;
        case gl::VERTEX_FORMAT_UBYTE3_NORM:
            return VK_FORMAT_R8G8B8_UNORM;
        case gl::VERTEX_FORMAT_UBYTE4:
            return VK_FORMAT_R8G8B8A8_UINT;
        case gl::VERTEX_FORMAT_UBYTE4_NORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case gl::VERTEX_FORMAT_SSHORT1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT1_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT2_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT3_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT4_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT1_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT2_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT3_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT4_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT1_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT2_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT3_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT4_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT1_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT2_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT3_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT4_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SBYTE1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SBYTE2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SBYTE3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SBYTE4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UBYTE1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UBYTE2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UBYTE3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UBYTE4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SSHORT4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_USHORT4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT1_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT2_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT3_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT4_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_FIXED1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_FIXED2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_FIXED3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_FIXED4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_HALF1:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_HALF2:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_HALF3:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_HALF4:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_FLOAT1:
            return VK_FORMAT_R32_SFLOAT;
        case gl::VERTEX_FORMAT_FLOAT2:
            return VK_FORMAT_R32G32_SFLOAT;
        case gl::VERTEX_FORMAT_FLOAT3:
            return VK_FORMAT_R32G32B32_SFLOAT;
        case gl::VERTEX_FORMAT_FLOAT4:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case gl::VERTEX_FORMAT_SINT210:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT210:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT210_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT210_NORM:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_SINT210_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        case gl::VERTEX_FORMAT_UINT210_INT:
            UNIMPLEMENTED();
            return VK_FORMAT_UNDEFINED;
        default:
            UNREACHABLE();
            return VK_FORMAT_UNDEFINED;
    }
}

}  // namespace vk

}  // namespace rx

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DispatchUtilsVk.h:
//    Defines the DispatchUtilsVk class, a helper for various internal dispatch utilities such as
//    buffer clear and copy, texture mip map generation, etc.
//
//    - Buffer clear: Implemented, but no current users
//    - Buffer copy:
//      * Used by VertexArrayVk::updateIndexTranslation() to convert a ubyte index array to ushort
//    - Convert vertex attribute:
//      * Used by VertexArrayVk::convertVertexBuffer() to convert vertex attributes from unsupported
//        formats to their fallbacks.
//    - Mipmap generation: Not yet implemented
//

#ifndef LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_
#define LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_

#include "libANGLE/renderer/vulkan/vk_cache_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders_autogen.h"

namespace rx
{
class DispatchUtilsVk : angle::NonCopyable
{
  public:
    DispatchUtilsVk();
    ~DispatchUtilsVk();

    void destroy(VkDevice device);

    struct ClearParameters
    {
        VkClearColorValue clearValue;
        size_t offset;
        size_t size;
    };

    struct CopyParameters
    {
        size_t destOffset;
        size_t srcOffset;
        size_t size;
    };

    struct ConvertVertexParameters
    {
        size_t vertexCount;
        const angle::Format *srcFormat;
        const angle::Format *destFormat;
        size_t srcStride;
        size_t srcOffset;
        size_t destOffset;
    };

    angle::Result clearBuffer(vk::Context *context,
                              vk::BufferHelper *dest,
                              const ClearParameters &params);
    angle::Result copyBuffer(vk::Context *context,
                             vk::BufferHelper *dest,
                             vk::BufferHelper *src,
                             const CopyParameters &params);
    angle::Result convertVertexBuffer(vk::Context *context,
                                      vk::BufferHelper *dest,
                                      vk::BufferHelper *src,
                                      const ConvertVertexParameters &params);

  private:
    struct BufferUtilsShaderParams
    {
        // Structure matching PushConstants in BufferUtils.comp
        uint32_t destOffset          = 0;
        uint32_t size                = 0;
        uint32_t srcOffset           = 0;
        uint32_t padding             = 0;
        VkClearColorValue clearValue = {};
    };

    struct ConvertVertexShaderParams
    {
        // Structure matching PushConstants in ConvertVertex.comp
        uint32_t outputCount    = 0;
        uint32_t componentCount = 0;
        uint32_t srcOffset      = 0;
        uint32_t destOffset     = 0;
        uint32_t Ns             = 0;
        uint32_t Bs             = 0;
        uint32_t Ss             = 0;
        uint32_t Es             = 0;
        uint32_t Nd             = 0;
        uint32_t Bd             = 0;
        uint32_t Sd             = 0;
        uint32_t Ed             = 0;
    };

    // Functions implemented by the class:
    enum class Function
    {
        BufferClear         = 0,
        BufferCopy          = 1,
        ConvertVertexBuffer = 2,

        InvalidEnum = 3,
        EnumCount   = 3,
    };

    // Common function that creates the pipeline for the specified function, binds it and prepares
    // the dispatch call. The possible values of `flags` comes from
    // vk::InternalShader::* defined in vk_internal_shaders_autogen.h
    angle::Result setupProgramCommon(vk::Context *context,
                                     Function function,
                                     vk::RefCounted<vk::ShaderAndSerial> *shader,
                                     vk::ShaderProgramHelper *program,
                                     const VkDescriptorSet descriptorSet,
                                     const void *pushConstants,
                                     size_t pushConstantsSize,
                                     vk::CommandBuffer *commandBuffer);

    using GetShader = angle::Result (vk::ShaderLibrary::*)(vk::Context *,
                                                           uint32_t,
                                                           vk::RefCounted<vk::ShaderAndSerial> **);

    template <GetShader getShader, Function function, typename ShaderParams>
    angle::Result setupProgram(vk::Context *context,
                               vk::ShaderProgramHelper *program,
                               uint32_t flags,
                               const VkDescriptorSet &descriptorSet,
                               const ShaderParams &params,
                               vk::CommandBuffer *commandBuffer);

    // Initializes descriptor set layout, pipeline layout and descriptor pool corresponding to given
    // function, if not already initialized.  Uses setSizes to create the layout.  For example, if
    // this array has two entries {STORAGE_TEXEL_BUFFER, 1} and {UNIFORM_TEXEL_BUFFER, 3}, then the
    // created set layout would be binding 0 for storage texel buffer and bindings 1 through 3 for
    // uniform texel buffer.  All resources are put in set 0.
    angle::Result ensureResourcesInitialized(vk::Context *context,
                                             Function function,
                                             VkDescriptorPoolSize *setSizes,
                                             size_t setSizesCount,
                                             size_t pushConstantsSize);

    // Initializers corresponding to functions, calling into ensureResourcesInitialized with the
    // appropriate parameters.
    angle::Result ensureBufferClearInitialized(vk::Context *context);
    angle::Result ensureBufferCopyInitialized(vk::Context *context);
    angle::Result ensureConvertVertexInitialized(vk::Context *context);

    angle::PackedEnumMap<Function, vk::DescriptorSetLayoutPointerArray> mDescriptorSetLayouts;
    angle::PackedEnumMap<Function, vk::BindingPointer<vk::PipelineLayout>> mPipelineLayouts;
    angle::PackedEnumMap<Function, vk::DynamicDescriptorPool> mDescriptorPools;

    vk::ShaderProgramHelper
        mBufferUtilsPrograms[vk::InternalShader::BufferUtils_comp::kFlagsMask |
                             vk::InternalShader::BufferUtils_comp::kFunctionMask |
                             vk::InternalShader::BufferUtils_comp::kFormatMask];
    vk::ShaderProgramHelper
        mConvertVertexPrograms[vk::InternalShader::ConvertVertex_comp::kFlagsMask |
                               vk::InternalShader::ConvertVertex_comp::kConversionMask];
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_

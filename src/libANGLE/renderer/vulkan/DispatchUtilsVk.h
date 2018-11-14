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
//    - Mipmap generation: Not yet implemented
//

#ifndef LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_
#define LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_

#include "libANGLE/renderer/vulkan/vk_cache_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders_autogen.h"

namespace rx
{

class BufferVk;
class RendererVk;

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

    angle::Result clearBuffer(vk::Context *context,
                              vk::BufferHelper *dest,
                              const ClearParameters &params);
    angle::Result copyBuffer(vk::Context *context,
                             vk::BufferHelper *dest,
                             vk::BufferHelper *src,
                             const CopyParameters &params);

  private:
    struct ShaderParams
    {
        // Structure matching PushConstants in BufferUtils.comp
        uint32_t destOffset          = 0;
        uint32_t size                = 0;
        uint32_t srcOffset           = 0;
        uint32_t padding             = 0;
        VkClearColorValue clearValue = {};
    };

    // Common function that creates the pipeline for the specified function, binds it and prepares
    // the dispatch call. The possible values of `function` comes from
    // vk::InternalShader::BufferUtils_comp defined in vk_internal_shaders_autogen.h
    angle::Result setupProgram(vk::Context *context,
                               uint32_t function,
                               const VkDescriptorSet &descriptorSet,
                               const ShaderParams &params,
                               vk::CommandBuffer *commandBuffer);

    // Functions implemented by the class:
    enum class Function
    {
        BufferClear = 0,
        BufferCopy  = 1,

        InvalidEnum = 2,
        EnumCount   = 2,
    };

    // Initializes descriptor set layout, pipeline layout and descriptor pool corresponding to given
    // function, if not already initialized.  Uses setSizes to create the layout.  For example, if
    // this array has two entries {STORAGE_TEXEL_BUFFER, 1} and {UNIFORM_TEXEL_BUFFER, 3}, then the
    // created set layout would be binding 0 for storage texel buffer and bindings 1 through 3 for
    // uniform texel buffer.  All resources are put in set 0.
    angle::Result ensureResourcesInitialized(vk::Context *context,
                                             Function function,
                                             VkDescriptorPoolSize *setSizes,
                                             size_t setSizesCount);

    // Initializers corresponding to functions, calling into ensureResourcesInitialized with the
    // appropriate parameters.
    angle::Result ensureBufferClearInitialized(vk::Context *context);
    angle::Result ensureBufferCopyInitialized(vk::Context *context);

    angle::PackedEnumMap<Function, vk::DescriptorSetLayoutPointerArray> mDescriptorSetLayouts;
    angle::PackedEnumMap<Function, vk::BindingPointer<vk::PipelineLayout>> mPipelineLayouts;
    angle::PackedEnumMap<Function, vk::DynamicDescriptorPool> mDescriptorPools;

    vk::ShaderProgramHelper mPrograms[vk::InternalShader::BufferUtils_comp::kFlagsMask |
                                      vk::InternalShader::BufferUtils_comp::kFunctionMask |
                                      vk::InternalShader::BufferUtils_comp::kFormatMask];
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_DISPATCHUTILSVK_H_

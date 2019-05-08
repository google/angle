//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FeaturesVk.h: Optional features for the Vulkan renderer.
//

#ifndef ANGLE_PLATFORM_FEATURESVK_H_
#define ANGLE_PLATFORM_FEATURESVK_H_

#include "platform/Feature.h"

namespace angle
{

struct FeaturesVk : angle::FeatureSetBase
{
    FeaturesVk();
    ~FeaturesVk();

    // Line segment rasterization must follow OpenGL rules. This means using an algorithm similar
    // to Bresenham's. Vulkan uses a different algorithm. This feature enables the use of pixel
    // shader patching to implement OpenGL basic line rasterization rules. This feature will
    // normally always be enabled. Exposing it as an option enables performance testing.
    angle::Feature basicGLLineRasterization = {
        "basic_gl_line_rasterization", angle::FeatureCategory::VulkanFeatures,
        "Enable the use of pixel shader patching to implement OpenGL basic line "
        "rasterization rules",
        &members};

    // Flips the viewport to render upside-down. This has the effect to render the same way as
    // OpenGL. If this feature gets enabled, we enable the KHR_MAINTENANCE_1 extension to allow
    // negative viewports. We inverse rendering to the backbuffer by reversing the height of the
    // viewport and increasing Y by the height. So if the viewport was (0,0,width,height), it
    // becomes (0, height, width, -height). Unfortunately, when we start doing this, we also need
    // to adjust a lot of places since the rendering now happens upside-down. Affected places so
    // far:
    // -readPixels
    // -copyTexImage
    // -framebuffer blit
    // -generating mipmaps
    // -Point sprites tests
    // -texStorage
    angle::Feature flipViewportY = {"flip_viewport_y", angle::FeatureCategory::VulkanFeatures,
                                    "Flips the viewport to render upside-down", &members};

    // Add an extra copy region when using vkCmdCopyBuffer as the Windows Intel driver seems
    // to have a bug where the last region is ignored.
    angle::Feature extraCopyBufferRegion = {
        "extra_copy_buffer_region", angle::FeatureCategory::VulkanWorkarounds,
        "Windows Intel driver seems to have a bug where the last copy region in "
        "vkCmdCopyBuffer is ignored",
        &members};

    // This flag is added for the sole purpose of end2end tests, to test the correctness
    // of various algorithms when a fallback format is used, such as using a packed format to
    // emulate a depth- or stencil-only format.
    angle::Feature forceFallbackFormat = {
        "force_fallback_format", angle::FeatureCategory::VulkanWorkarounds,
        "Force a fallback format for angle_end2end_tests", &members};

    // On some NVIDIA drivers the point size range reported from the API is inconsistent with the
    // actual behavior. Clamp the point size to the value from the API to fix this.
    // Tracked in http://anglebug.com/2970.
    angle::Feature clampPointSize = {
        "clamp_point_size", angle::FeatureCategory::VulkanWorkarounds,
        "On some NVIDIA drivers the point size range reported from the API is "
        "inconsistent with the actual behavior",
        &members, "http://anglebug.com/2970"};

    // On some android devices, the memory barrier between the compute shader that converts vertex
    // attributes and the vertex shader that reads from it is ineffective.  Only known workaround is
    // to perform a flush after the conversion.  http://anglebug.com/3016
    angle::Feature flushAfterVertexConversion = {
        "flush_after_vertex_conversion", angle::FeatureCategory::VulkanWorkarounds,
        "On some android devices, the memory barrier between the compute shader that converts "
        "vertex attributes and the vertex shader that reads from it is ineffective",
        &members, "http://anglebug.com/3016"};

    // Whether the VkDevice supports the VK_KHR_incremental_present extension, on which the
    // EGL_KHR_swap_buffers_with_damage extension can be layered.
    angle::Feature supportsIncrementalPresent = {
        "supports_incremental_present", angle::FeatureCategory::VulkanFeatures,
        "VkDevice supports the VK_KHR_incremental_present extension", &members};

    // Whether texture copies on cube map targets should be done on GPU.  This is a workaround for
    // Intel drivers on windows that have an issue with creating single-layer views on cube map
    // textures.
    angle::Feature forceCpuPathForCubeMapCopy = {
        "force_cpu_path_for_cube_map_copy", angle::FeatureCategory::VulkanWorkarounds,
        "Some Intel Windows drivers have an issue with creating single-layer "
        "views on cube map textures",
        &members};

    // Whether the VkDevice supports the VK_ANDROID_external_memory_android_hardware_buffer
    // extension, on which the EGL_ANDROID_image_native_buffer extension can be layered.
    angle::Feature supportsAndroidHardwareBuffer = {
        "supports_android_hardware_buffer", angle::FeatureCategory::VulkanFeatures,
        "VkDevice supports the VK_ANDROID_external_memory_android_hardware_buffer extension",
        &members};

    // Whether the VkDevice supports the VK_KHR_external_memory_fd extension, on which the
    // GL_EXT_memory_object_fd extension can be layered.
    angle::Feature supportsExternalMemoryFd = {
        "supports_external_memory_fd", angle::FeatureCategory::VulkanFeatures,
        "VkDevice supports the VK_KHR_external_memory_fd extension", &members};

    // Whether the VkDevice supports the VK_KHR_external_semaphore_fd extension, on which the
    // GL_EXT_semaphore_fd extension can be layered.
    angle::Feature supportsExternalSemaphoreFd = {
        "supports_external_semaphore_fd", angle::FeatureCategory::VulkanFeatures,
        "VkDevice supports the VK_KHR_external_semaphore_fd extension", &members};

    // VK_PRESENT_MODE_FIFO_KHR causes random timeouts on Linux Intel. http://anglebug.com/3153
    angle::Feature disableFifoPresentMode = {
        "disable_fifo_present_mode", angle::FeatureCategory::VulkanWorkarounds,
        "On Linux Intel, VK_PRESENT_MODE_FIFO_KHR causes random timeouts", &members,
        "http://anglebug.com/3153"};

    // On Qualcomm, a bug is preventing us from using loadOp=Clear with inline commands in the
    // render pass.  http://anglebug.com/2361
    angle::Feature restartRenderPassAfterLoadOpClear = {
        "restart_render_pass_after_load_op_clear", angle::FeatureCategory::VulkanWorkarounds,
        "On Qualcomm, a bug is preventing us from using loadOp=Clear with inline "
        "commands in the render pass",
        &members, "http://anglebug.com/2361"};

    // On Qualcomm, gaps in bound descriptor set indices causes the post-gap sets to misbehave.
    // For example, binding only descriptor set 3 results in zero being read from a uniform buffer
    // object within that set.  This flag results in empty descriptor sets being bound for any
    // unused descriptor set to work around this issue.  http://anglebug.com/2727
    angle::Feature bindEmptyForUnusedDescriptorSets = {
        "bind_empty_for_unused_descriptor_sets", angle::FeatureCategory::VulkanWorkarounds,
        "On Qualcomm,gaps in bound descriptor set indices causes the post-gap sets to misbehave",
        &members, "http://anglebug.com/2727"};

    // When the scissor is (0,0,0,0) on Windows Intel, the driver acts as if the scissor was
    // disabled.  Work-around this by setting the scissor to just outside of the render area
    // (e.g. (renderArea.x, renderArea.y, 1, 1)). http://anglebug.com/3153
    angle::Feature forceNonZeroScissor = {
        "force_non_zero_scissor", angle::FeatureCategory::VulkanWorkarounds,
        "On Windows Intel, when the scissor is (0,0,0,0), the driver acts as if the "
        "scissor was disabled",
        &members, "http://anglebug.com/3153"};
};

inline FeaturesVk::FeaturesVk() = default;
inline FeaturesVk::~FeaturesVk() = default;

}  // namespace angle

#endif  // ANGLE_PLATFORM_FEATURESVK_H_

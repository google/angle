//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_helpers:
//   Helper utilitiy classes that manage Vulkan resources.

#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/driver_utils.h"

#include "common/utilities.h"
#include "image_util/loadimage.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/android/vk_android_utils.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"
#include "libANGLE/trace.h"

namespace rx
{
namespace vk
{
namespace
{
// ANGLE_robust_resource_initialization requires color textures to be initialized to zero.
constexpr VkClearColorValue kRobustInitColorValue = {{0, 0, 0, 0}};
// When emulating a texture, we want the emulated channels to be 0, with alpha 1.
constexpr VkClearColorValue kEmulatedInitColorValue = {{0, 0, 0, 1.0f}};
// ANGLE_robust_resource_initialization requires depth to be initialized to 1 and stencil to 0.
// We are fine with these values for emulated depth/stencil textures too.
constexpr VkClearDepthStencilValue kRobustInitDepthStencilValue = {1.0f, 0};

constexpr VkImageAspectFlags kDepthStencilAspects =
    VK_IMAGE_ASPECT_STENCIL_BIT | VK_IMAGE_ASPECT_DEPTH_BIT;

constexpr VkBufferUsageFlags kLineLoopDynamicBufferUsage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                           VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                           VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
constexpr int kLineLoopDynamicBufferInitialSize = 1024 * 1024;
constexpr VkBufferUsageFlags kLineLoopDynamicIndirectBufferUsage =
    VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
    VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
constexpr int kLineLoopDynamicIndirectBufferInitialSize = sizeof(VkDrawIndirectCommand) * 16;

constexpr angle::PackedEnumMap<PipelineStage, VkPipelineStageFlagBits> kPipelineStageFlagBitMap = {
    {PipelineStage::TopOfPipe, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT},
    {PipelineStage::DrawIndirect, VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT},
    {PipelineStage::VertexInput, VK_PIPELINE_STAGE_VERTEX_INPUT_BIT},
    {PipelineStage::VertexShader, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT},
    {PipelineStage::GeometryShader, VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT},
    {PipelineStage::TransformFeedback, VK_PIPELINE_STAGE_TRANSFORM_FEEDBACK_BIT_EXT},
    {PipelineStage::EarlyFragmentTest, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT},
    {PipelineStage::FragmentShader, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT},
    {PipelineStage::LateFragmentTest, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT},
    {PipelineStage::ColorAttachmentOutput, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT},
    {PipelineStage::ComputeShader, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT},
    {PipelineStage::Transfer, VK_PIPELINE_STAGE_TRANSFER_BIT},
    {PipelineStage::BottomOfPipe, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT},
    {PipelineStage::Host, VK_PIPELINE_STAGE_HOST_BIT}};

constexpr gl::ShaderMap<PipelineStage> kPipelineStageShaderMap = {
    {gl::ShaderType::Vertex, PipelineStage::VertexShader},
    {gl::ShaderType::Fragment, PipelineStage::FragmentShader},
    {gl::ShaderType::Geometry, PipelineStage::GeometryShader},
    {gl::ShaderType::Compute, PipelineStage::ComputeShader},
};

constexpr size_t kDefaultPoolAllocatorPageSize = 16 * 1024;

struct ImageMemoryBarrierData
{
    char name[44];

    // The Vk layout corresponding to the ImageLayout key.
    VkImageLayout layout;

    // The stage in which the image is used (or Bottom/Top if not using any specific stage).  Unless
    // Bottom/Top (Bottom used for transition to and Top used for transition from), the two values
    // should match.
    VkPipelineStageFlags dstStageMask;
    VkPipelineStageFlags srcStageMask;
    // Access mask when transitioning into this layout.
    VkAccessFlags dstAccessMask;
    // Access mask when transitioning out from this layout.  Note that source access mask never
    // needs a READ bit, as WAR hazards don't need memory barriers (just execution barriers).
    VkAccessFlags srcAccessMask;
    // Read or write.
    ResourceAccess type;
    // CommandBufferHelper tracks an array of PipelineBarriers. This indicates which array element
    // this should be merged into. Right now we track individual barrier for every PipelineStage. If
    // layout has a single stage mask bit, we use that stage as index. If layout has multiple stage
    // mask bits, we pick the lowest stage as the index since it is the first stage that needs
    // barrier.
    PipelineStage barrierIndex;
};

constexpr VkPipelineStageFlags kPreFragmentStageFlags =
    VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT |
    VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT | VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT;

constexpr VkPipelineStageFlags kAllShadersPipelineStageFlags =
    kPreFragmentStageFlags | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

constexpr VkPipelineStageFlags kAllDepthStencilPipelineStageFlags =
    VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

// clang-format off
constexpr angle::PackedEnumMap<ImageLayout, ImageMemoryBarrierData> kImageMemoryBarrierData = {
    {
        ImageLayout::Undefined,
        ImageMemoryBarrierData{
            "Undefined",
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // Transition to: we don't expect to transition into Undefined.
            0,
            // Transition from: there's no data in the image to care about.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::InvalidEnum,
        },
    },
    {
        ImageLayout::ColorAttachment,
        ImageMemoryBarrierData{
            "ColorAttachment",
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::ColorAttachmentOutput,
        },
    },
    {
        ImageLayout::ColorAttachmentAndFragmentShaderRead,
        ImageMemoryBarrierData{
            "ColorAttachmentAndFragmentShaderRead",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::FragmentShader,
        },
    },
    {
        ImageLayout::ColorAttachmentAndAllShadersRead,
        ImageMemoryBarrierData{
            "ColorAttachmentAndAllShadersRead",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | kAllShadersPipelineStageFlags,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | kAllShadersPipelineStageFlags,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::DSAttachmentWriteAndFragmentShaderRead,
        ImageMemoryBarrierData{
            "DSAttachmentWriteAndFragmentShaderRead",
            VK_IMAGE_LAYOUT_GENERAL,
            kAllDepthStencilPipelineStageFlags | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            kAllDepthStencilPipelineStageFlags | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::FragmentShader,
        },
    },
    {
        ImageLayout::DSAttachmentWriteAndAllShadersRead,
        ImageMemoryBarrierData{
            "DSAttachmentWriteAndAllShadersRead",
            VK_IMAGE_LAYOUT_GENERAL,
            kAllDepthStencilPipelineStageFlags | kAllShadersPipelineStageFlags,
            kAllDepthStencilPipelineStageFlags | kAllShadersPipelineStageFlags,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_SHADER_READ_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::DSAttachmentReadAndFragmentShaderRead,
            ImageMemoryBarrierData{
            "DSAttachmentReadAndFragmentShaderRead",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | kAllDepthStencilPipelineStageFlags,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | kAllDepthStencilPipelineStageFlags,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::EarlyFragmentTest,
        },
    },
    {
        ImageLayout::DSAttachmentReadAndAllShadersRead,
            ImageMemoryBarrierData{
            "DSAttachmentReadAndAllShadersRead",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            kAllShadersPipelineStageFlags | kAllDepthStencilPipelineStageFlags,
            kAllShadersPipelineStageFlags | kAllDepthStencilPipelineStageFlags,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::DepthStencilAttachmentReadOnly,
            ImageMemoryBarrierData{
            "DepthStencilAttachmentReadOnly",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
            kAllDepthStencilPipelineStageFlags,
            kAllDepthStencilPipelineStageFlags,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::EarlyFragmentTest,
        },
    },
    {
        ImageLayout::DepthStencilAttachment,
        ImageMemoryBarrierData{
            "DepthStencilAttachment",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            kAllDepthStencilPipelineStageFlags,
            kAllDepthStencilPipelineStageFlags,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::EarlyFragmentTest,
        },
    },
    {
        ImageLayout::DepthStencilResolveAttachment,
        ImageMemoryBarrierData{
            "DepthStencilResolveAttachment",
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            // Note: depth/stencil resolve uses color output stage and mask!
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::ColorAttachmentOutput,
        },
    },
    {
        ImageLayout::Present,
        ImageMemoryBarrierData{
            "Present",
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            // transition to: vkQueuePresentKHR automatically performs the appropriate memory barriers:
            //
            // > Any writes to memory backing the images referenced by the pImageIndices and
            // > pSwapchains members of pPresentInfo, that are available before vkQueuePresentKHR
            // > is executed, are automatically made visible to the read access performed by the
            // > presentation engine.
            0,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::BottomOfPipe,
        },
    },
    {
        ImageLayout::ExternalPreInitialized,
        ImageMemoryBarrierData{
            "ExternalPreInitialized",
            VK_IMAGE_LAYOUT_PREINITIALIZED,
            VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_HOST_BIT | VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            // Transition to: we don't expect to transition into PreInitialized.
            0,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_MEMORY_WRITE_BIT,
            ResourceAccess::ReadOnly,
            PipelineStage::InvalidEnum,
        },
    },
    {
        ImageLayout::ExternalShadersReadOnly,
        ImageMemoryBarrierData{
            "ExternalShadersReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::TopOfPipe,
        },
    },
    {
        ImageLayout::ExternalShadersWrite,
        ImageMemoryBarrierData{
            "ExternalShadersWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::TopOfPipe,
        },
    },
    {
        ImageLayout::TransferSrc,
        ImageMemoryBarrierData{
            "TransferSrc",
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_TRANSFER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::Transfer,
        },
    },
    {
        ImageLayout::TransferDst,
        ImageMemoryBarrierData{
            "TransferDst",
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            // Transition to: all writes must happen after barrier.
            VK_ACCESS_TRANSFER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_TRANSFER_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::Transfer,
        },
    },
    {
        ImageLayout::VertexShaderReadOnly,
        ImageMemoryBarrierData{
            "VertexShaderReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::VertexShaderWrite,
        ImageMemoryBarrierData{
            "VertexShaderWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::PreFragmentShadersReadOnly,
        ImageMemoryBarrierData{
            "PreFragmentShadersReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            kPreFragmentStageFlags,
            kPreFragmentStageFlags,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::PreFragmentShadersWrite,
        ImageMemoryBarrierData{
            "PreFragmentShadersWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            kPreFragmentStageFlags,
            kPreFragmentStageFlags,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::FragmentShaderReadOnly,
        ImageMemoryBarrierData{
            "FragmentShaderReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::FragmentShader,
        },
    },
    {
        ImageLayout::FragmentShaderWrite,
        ImageMemoryBarrierData{
            "FragmentShaderWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::FragmentShader,
        },
    },
    {
        ImageLayout::ComputeShaderReadOnly,
        ImageMemoryBarrierData{
            "ComputeShaderReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            PipelineStage::ComputeShader,
        },
    },
    {
        ImageLayout::ComputeShaderWrite,
        ImageMemoryBarrierData{
            "ComputeShaderWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            PipelineStage::ComputeShader,
        },
    },
    {
        ImageLayout::AllGraphicsShadersReadOnly,
        ImageMemoryBarrierData{
            "AllGraphicsShadersReadOnly",
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            kAllShadersPipelineStageFlags,
            kAllShadersPipelineStageFlags,
            // Transition to: all reads must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT,
            // Transition from: RAR and WAR don't need memory barrier.
            0,
            ResourceAccess::ReadOnly,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
    {
        ImageLayout::AllGraphicsShadersWrite,
        ImageMemoryBarrierData{
            "AllGraphicsShadersWrite",
            VK_IMAGE_LAYOUT_GENERAL,
            kAllShadersPipelineStageFlags,
            kAllShadersPipelineStageFlags,
            // Transition to: all reads and writes must happen after barrier.
            VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
            // Transition from: all writes must finish before barrier.
            VK_ACCESS_SHADER_WRITE_BIT,
            ResourceAccess::Write,
            // In case of multiple destination stages, We barrier the earliest stage
            PipelineStage::VertexShader,
        },
    },
};
// clang-format on

VkPipelineStageFlags GetImageLayoutSrcStageMask(Context *context,
                                                const ImageMemoryBarrierData &transition)
{
    return transition.srcStageMask & context->getRenderer()->getSupportedVulkanPipelineStageMask();
}

VkPipelineStageFlags GetImageLayoutDstStageMask(Context *context,
                                                const ImageMemoryBarrierData &transition)
{
    return transition.dstStageMask & context->getRenderer()->getSupportedVulkanPipelineStageMask();
}

VkImageCreateFlags GetImageCreateFlags(gl::TextureType textureType)
{
    switch (textureType)
    {
        case gl::TextureType::CubeMap:
        case gl::TextureType::CubeMapArray:
            return VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;

        case gl::TextureType::_3D:
            return VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;

        default:
            return 0;
    }
}

void HandlePrimitiveRestart(ContextVk *contextVk,
                            gl::DrawElementsType glIndexType,
                            GLsizei indexCount,
                            const uint8_t *srcPtr,
                            uint8_t *outPtr)
{
    switch (glIndexType)
    {
        case gl::DrawElementsType::UnsignedByte:
            if (contextVk->getFeatures().supportsIndexTypeUint8.enabled)
            {
                CopyLineLoopIndicesWithRestart<uint8_t, uint8_t>(indexCount, srcPtr, outPtr);
            }
            else
            {
                CopyLineLoopIndicesWithRestart<uint8_t, uint16_t>(indexCount, srcPtr, outPtr);
            }
            break;
        case gl::DrawElementsType::UnsignedShort:
            CopyLineLoopIndicesWithRestart<uint16_t, uint16_t>(indexCount, srcPtr, outPtr);
            break;
        case gl::DrawElementsType::UnsignedInt:
            CopyLineLoopIndicesWithRestart<uint32_t, uint32_t>(indexCount, srcPtr, outPtr);
            break;
        default:
            UNREACHABLE();
    }
}

bool HasBothDepthAndStencilAspects(VkImageAspectFlags aspectFlags)
{
    return IsMaskFlagSet(aspectFlags, kDepthStencilAspects);
}

uint8_t GetContentDefinedLayerRangeBits(uint32_t layerStart,
                                        uint32_t layerCount,
                                        uint32_t maxLayerCount)
{
    uint8_t layerRangeBits = layerCount >= maxLayerCount ? static_cast<uint8_t>(~0u)
                                                         : angle::BitMask<uint8_t>(layerCount);
    layerRangeBits <<= layerStart;

    return layerRangeBits;
}

uint32_t GetImageLayerCountForView(const ImageHelper &image)
{
    // Depth > 1 means this is a 3D texture and depth is our layer count
    return image.getExtents().depth > 1 ? image.getExtents().depth : image.getLayerCount();
}

void ReleaseImageViews(ImageViewVector *imageViewVector, std::vector<GarbageObject> *garbage)
{
    for (ImageView &imageView : *imageViewVector)
    {
        if (imageView.valid())
        {
            garbage->emplace_back(GetGarbage(&imageView));
        }
    }
    imageViewVector->clear();
}

void DestroyImageViews(ImageViewVector *imageViewVector, VkDevice device)
{
    for (ImageView &imageView : *imageViewVector)
    {
        imageView.destroy(device);
    }
    imageViewVector->clear();
}

ImageView *GetLevelImageView(ImageViewVector *imageViews, LevelIndex levelVk, uint32_t levelCount)
{
    // Lazily allocate the storage for image views. We allocate the full level count because we
    // don't want to trigger any std::vector reallocations. Reallocations could invalidate our
    // view pointers.
    if (imageViews->empty())
    {
        imageViews->resize(levelCount);
    }
    ASSERT(imageViews->size() > levelVk.get());

    return &(*imageViews)[levelVk.get()];
}

ImageView *GetLevelLayerImageView(LayerLevelImageViewVector *imageViews,
                                  LevelIndex levelVk,
                                  uint32_t layer,
                                  uint32_t levelCount,
                                  uint32_t layerCount)
{
    // Lazily allocate the storage for image views. We allocate the full layer count because we
    // don't want to trigger any std::vector reallocations. Reallocations could invalidate our
    // view pointers.
    if (imageViews->empty())
    {
        imageViews->resize(layerCount);
    }
    ASSERT(imageViews->size() > layer);

    return GetLevelImageView(&(*imageViews)[layer], levelVk, levelCount);
}

// Special rules apply to VkBufferImageCopy with depth/stencil. The components are tightly packed
// into a depth or stencil section of the destination buffer. See the spec:
// https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkBufferImageCopy.html
const angle::Format &GetDepthStencilImageToBufferFormat(const angle::Format &imageFormat,
                                                        VkImageAspectFlagBits copyAspect)
{
    if (copyAspect == VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        ASSERT(imageFormat.id == angle::FormatID::D24_UNORM_S8_UINT ||
               imageFormat.id == angle::FormatID::D32_FLOAT_S8X24_UINT ||
               imageFormat.id == angle::FormatID::S8_UINT);
        return angle::Format::Get(angle::FormatID::S8_UINT);
    }

    ASSERT(copyAspect == VK_IMAGE_ASPECT_DEPTH_BIT);

    switch (imageFormat.id)
    {
        case angle::FormatID::D16_UNORM:
            return imageFormat;
        case angle::FormatID::D24_UNORM_X8_UINT:
            return imageFormat;
        case angle::FormatID::D24_UNORM_S8_UINT:
            return angle::Format::Get(angle::FormatID::D24_UNORM_X8_UINT);
        case angle::FormatID::D32_FLOAT:
            return imageFormat;
        case angle::FormatID::D32_FLOAT_S8X24_UINT:
            return angle::Format::Get(angle::FormatID::D32_FLOAT);
        default:
            UNREACHABLE();
            return imageFormat;
    }
}

VkClearValue GetRobustResourceClearValue(const Format &format)
{
    VkClearValue clearValue = {};
    if (format.intendedFormat().hasDepthOrStencilBits())
    {
        clearValue.depthStencil = kRobustInitDepthStencilValue;
    }
    else
    {
        clearValue.color =
            format.hasEmulatedImageChannels() ? kEmulatedInitColorValue : kRobustInitColorValue;
    }
    return clearValue;
}

#if !defined(ANGLE_PLATFORM_MACOS) && !defined(ANGLE_PLATFORM_ANDROID)
bool IsExternalQueueFamily(uint32_t queueFamilyIndex)
{
    return queueFamilyIndex == VK_QUEUE_FAMILY_EXTERNAL ||
           queueFamilyIndex == VK_QUEUE_FAMILY_FOREIGN_EXT;
}
#endif

bool IsShaderReadOnlyLayout(const ImageMemoryBarrierData &imageLayout)
{
    return imageLayout.layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

bool IsAnySubresourceContentDefined(const gl::TexLevelArray<angle::BitSet8<8>> &contentDefined)
{
    for (const angle::BitSet8<8> &levelContentDefined : contentDefined)
    {
        if (levelContentDefined.any())
        {
            return true;
        }
    }
    return false;
}

void ExtendRenderPassInvalidateArea(const gl::Rectangle &invalidateArea, gl::Rectangle *out)
{
    if (out->empty())
    {
        *out = invalidateArea;
    }
    else
    {
        gl::ExtendRectangle(*out, invalidateArea, out);
    }
}

bool CanCopyWithTransferForCopyImage(RendererVk *renderer,
                                     const Format &srcFormat,
                                     VkImageTiling srcTilingMode,
                                     const Format &destFormat,
                                     VkImageTiling destTilingMode)
{
    // Neither source nor destination formats can be emulated for copy image through transfer,
    // unless they are emualted with the same format!
    bool isFormatCompatible =
        (!srcFormat.hasEmulatedImageFormat() && !destFormat.hasEmulatedImageFormat()) ||
        srcFormat.actualImageFormatID == destFormat.actualImageFormatID;

    // If neither formats are emulated, GL validation ensures that pixelBytes is the same for both.
    ASSERT(!isFormatCompatible ||
           srcFormat.actualImageFormat().pixelBytes == destFormat.actualImageFormat().pixelBytes);

    return isFormatCompatible &&
           CanCopyWithTransfer(renderer, srcFormat, srcTilingMode, destFormat, destTilingMode);
}

bool CanCopyWithTransformForReadPixels(const PackPixelsParams &packPixelsParams,
                                       const vk::Format *imageFormat,
                                       const angle::Format *readFormat)
{
    // Don't allow copies from emulated formats for simplicity.
    const bool isEmulatedFormat = imageFormat->hasEmulatedImageFormat();

    // Only allow copies to PBOs with identical format.
    const bool isSameFormatCopy = *readFormat == *packPixelsParams.destFormat;

    // Disallow any transformation.
    const bool needsTransformation =
        packPixelsParams.rotation != SurfaceRotation::Identity || packPixelsParams.reverseRowOrder;

    // Disallow copies when the output pitch cannot be correctly specified in Vulkan.
    const bool isPitchMultipleOfTexelSize =
        packPixelsParams.outputPitch % readFormat->pixelBytes == 0;

    return !isEmulatedFormat && isSameFormatCopy && !needsTransformation &&
           isPitchMultipleOfTexelSize;
}

void ReleaseBufferListToRenderer(RendererVk *renderer, BufferHelperPointerVector *buffers)
{
    for (std::unique_ptr<BufferHelper> &toFree : *buffers)
    {
        toFree->release(renderer);
    }
    buffers->clear();
}

void DestroyBufferList(RendererVk *renderer, BufferHelperPointerVector *buffers)
{
    for (std::unique_ptr<BufferHelper> &toDestroy : *buffers)
    {
        toDestroy->destroy(renderer);
    }
    buffers->clear();
}

bool ShouldReleaseFreeBuffer(const vk::BufferHelper &buffer,
                             size_t dynamicBufferSize,
                             DynamicBufferPolicy policy,
                             size_t freeListSize)
{
    constexpr size_t kLimitedFreeListMaxSize = 1;

    // If the dynamic buffer was resized we cannot reuse the retained buffer.  Additionally,
    // only reuse the buffer if specifically requested.
    const bool sizeMismatch    = buffer.getSize() != dynamicBufferSize;
    const bool releaseByPolicy = policy == DynamicBufferPolicy::OneShotUse ||
                                 (policy == DynamicBufferPolicy::SporadicTextureUpload &&
                                  freeListSize >= kLimitedFreeListMaxSize);

    return sizeMismatch || releaseByPolicy;
}
}  // anonymous namespace

// This is an arbitrary max. We can change this later if necessary.
uint32_t DynamicDescriptorPool::mMaxSetsPerPool           = 16;
uint32_t DynamicDescriptorPool::mMaxSetsPerPoolMultiplier = 2;

VkImageLayout ConvertImageLayoutToVkImageLayout(ImageLayout imageLayout)
{
    return kImageMemoryBarrierData[imageLayout].layout;
}

bool FormatHasNecessaryFeature(RendererVk *renderer,
                               angle::FormatID formatID,
                               VkImageTiling tilingMode,
                               VkFormatFeatureFlags featureBits)
{
    return (tilingMode == VK_IMAGE_TILING_OPTIMAL)
               ? renderer->hasImageFormatFeatureBits(formatID, featureBits)
               : renderer->hasLinearImageFormatFeatureBits(formatID, featureBits);
}

bool CanCopyWithTransfer(RendererVk *renderer,
                         const Format &srcFormat,
                         VkImageTiling srcTilingMode,
                         const Format &destFormat,
                         VkImageTiling destTilingMode)
{
    // Checks that the formats in the copy transfer have the appropriate tiling and transfer bits
    bool isTilingCompatible           = srcTilingMode == destTilingMode;
    bool srcFormatHasNecessaryFeature = FormatHasNecessaryFeature(
        renderer, srcFormat.actualImageFormatID, srcTilingMode, VK_FORMAT_FEATURE_TRANSFER_SRC_BIT);
    bool dstFormatHasNecessaryFeature =
        FormatHasNecessaryFeature(renderer, destFormat.actualImageFormatID, destTilingMode,
                                  VK_FORMAT_FEATURE_TRANSFER_DST_BIT);

    return isTilingCompatible && srcFormatHasNecessaryFeature && dstFormatHasNecessaryFeature;
}

// PackedClearValuesArray implementation
PackedClearValuesArray::PackedClearValuesArray() : mValues{} {}
PackedClearValuesArray::~PackedClearValuesArray() = default;

PackedClearValuesArray::PackedClearValuesArray(const PackedClearValuesArray &other) = default;
PackedClearValuesArray &PackedClearValuesArray::operator=(const PackedClearValuesArray &rhs) =
    default;

void PackedClearValuesArray::store(PackedAttachmentIndex index,
                                   VkImageAspectFlags aspectFlags,
                                   const VkClearValue &clearValue)
{
    ASSERT(aspectFlags != 0);
    if (aspectFlags != VK_IMAGE_ASPECT_STENCIL_BIT)
    {
        storeNoDepthStencil(index, clearValue);
    }
}

void PackedClearValuesArray::storeNoDepthStencil(PackedAttachmentIndex index,
                                                 const VkClearValue &clearValue)
{
    mValues[index.get()] = clearValue;
}

// CommandBufferHelper implementation.
CommandBufferHelper::CommandBufferHelper()
    : mPipelineBarriers(),
      mPipelineBarrierMask(),
      mCounter(0),
      mClearValues{},
      mRenderPassStarted(false),
      mTransformFeedbackCounterBuffers{},
      mValidTransformFeedbackBufferCount(0),
      mRebindTransformFeedbackBuffers(false),
      mIsTransformFeedbackActiveUnpaused(false),
      mIsRenderPassCommandBuffer(false),
      mHasShaderStorageOutput(false),
      mHasGLMemoryBarrierIssued(false),
      mDepthAccess(ResourceAccess::Unused),
      mStencilAccess(ResourceAccess::Unused),
      mDepthCmdSizeInvalidated(kInfiniteCmdSize),
      mDepthCmdSizeDisabled(kInfiniteCmdSize),
      mStencilCmdSizeInvalidated(kInfiniteCmdSize),
      mStencilCmdSizeDisabled(kInfiniteCmdSize),
      mDepthStencilAttachmentIndex(kAttachmentIndexInvalid),
      mDepthStencilImage(nullptr),
      mDepthStencilResolveImage(nullptr),
      mDepthStencilLevelIndex(0),
      mDepthStencilLayerIndex(0),
      mDepthStencilLayerCount(0),
      mColorImagesCount(0),
      mImageOptimizeForPresent(nullptr)
{}

CommandBufferHelper::~CommandBufferHelper()
{
    mFramebuffer.setHandle(VK_NULL_HANDLE);
}

void CommandBufferHelper::initialize(bool isRenderPassCommandBuffer)
{
    ASSERT(mUsedBuffers.empty());
    constexpr size_t kInitialBufferCount = 128;
    mUsedBuffers.ensureCapacity(kInitialBufferCount);

    mAllocator.initialize(kDefaultPoolAllocatorPageSize, 1);
    // Push a scope into the pool allocator so we can easily free and re-init on reset()
    mAllocator.push();
    mCommandBuffer.initialize(&mAllocator);
    mIsRenderPassCommandBuffer = isRenderPassCommandBuffer;
}

void CommandBufferHelper::reset()
{
    mAllocator.pop();
    mAllocator.push();
    mCommandBuffer.reset();
    mUsedBuffers.clear();

    if (mIsRenderPassCommandBuffer)
    {
        mRenderPassStarted                 = false;
        mValidTransformFeedbackBufferCount = 0;
        mRebindTransformFeedbackBuffers    = false;
        mHasShaderStorageOutput            = false;
        mHasGLMemoryBarrierIssued          = false;
        mDepthAccess                       = ResourceAccess::Unused;
        mStencilAccess                     = ResourceAccess::Unused;
        mDepthCmdSizeInvalidated           = kInfiniteCmdSize;
        mDepthCmdSizeDisabled              = kInfiniteCmdSize;
        mStencilCmdSizeInvalidated         = kInfiniteCmdSize;
        mStencilCmdSizeDisabled            = kInfiniteCmdSize;
        mColorImagesCount                  = PackedAttachmentCount(0);
        mDepthStencilAttachmentIndex       = kAttachmentIndexInvalid;
        mDepthInvalidateArea               = gl::Rectangle();
        mStencilInvalidateArea             = gl::Rectangle();
        mRenderPassUsedImages.clear();
        mDepthStencilImage        = nullptr;
        mDepthStencilResolveImage = nullptr;
        mColorImages.reset();
        mColorResolveImages.reset();
        mImageOptimizeForPresent = nullptr;
    }
    // This state should never change for non-renderPass command buffer
    ASSERT(mRenderPassStarted == false);
    ASSERT(mValidTransformFeedbackBufferCount == 0);
    ASSERT(!mRebindTransformFeedbackBuffers);
    ASSERT(!mIsTransformFeedbackActiveUnpaused);
    ASSERT(mRenderPassUsedImages.empty());
}

bool CommandBufferHelper::usesBuffer(const BufferHelper &buffer) const
{
    return mUsedBuffers.contains(buffer.getBufferSerial().getValue());
}

bool CommandBufferHelper::usesBufferForWrite(const BufferHelper &buffer) const
{
    BufferAccess access;
    if (!mUsedBuffers.get(buffer.getBufferSerial().getValue(), &access))
    {
        return false;
    }
    return access == BufferAccess::Write;
}

void CommandBufferHelper::bufferRead(ContextVk *contextVk,
                                     VkAccessFlags readAccessType,
                                     PipelineStage readStage,
                                     BufferHelper *buffer)
{
    buffer->retain(&contextVk->getResourceUseList());
    VkPipelineStageFlagBits stageBits = kPipelineStageFlagBitMap[readStage];
    if (buffer->recordReadBarrier(readAccessType, stageBits, &mPipelineBarriers[readStage]))
    {
        mPipelineBarrierMask.set(readStage);
    }

    ASSERT(!usesBufferForWrite(*buffer));
    if (!mUsedBuffers.contains(buffer->getBufferSerial().getValue()))
    {
        mUsedBuffers.insert(buffer->getBufferSerial().getValue(), BufferAccess::Read);
    }
}

void CommandBufferHelper::bufferWrite(ContextVk *contextVk,
                                      VkAccessFlags writeAccessType,
                                      PipelineStage writeStage,
                                      AliasingMode aliasingMode,
                                      BufferHelper *buffer)
{
    buffer->retain(&contextVk->getResourceUseList());
    VkPipelineStageFlagBits stageBits = kPipelineStageFlagBitMap[writeStage];
    if (buffer->recordWriteBarrier(writeAccessType, stageBits, &mPipelineBarriers[writeStage]))
    {
        mPipelineBarrierMask.set(writeStage);
    }

    // Storage buffers are special. They can alias one another in a shader.
    // We support aliasing by not tracking storage buffers. This works well with the GL API
    // because storage buffers are required to be externally synchronized.
    // Compute / XFB emulation buffers are not allowed to alias.
    if (aliasingMode == AliasingMode::Disallowed)
    {
        ASSERT(!usesBuffer(*buffer));
        mUsedBuffers.insert(buffer->getBufferSerial().getValue(), BufferAccess::Write);
    }

    // Make sure host-visible buffer writes result in a barrier inserted at the end of the frame to
    // make the results visible to the host.  The buffer may be mapped by the application in the
    // future.
    if (buffer->isHostVisible())
    {
        contextVk->onHostVisibleBufferWrite();
    }
}

void CommandBufferHelper::imageRead(ContextVk *contextVk,
                                    VkImageAspectFlags aspectFlags,
                                    ImageLayout imageLayout,
                                    ImageHelper *image)
{
    image->retain(&contextVk->getResourceUseList());

    if (image->isReadBarrierNecessary(imageLayout))
    {
        updateImageLayoutAndBarrier(contextVk, image, aspectFlags, imageLayout);
    }

    if (mIsRenderPassCommandBuffer)
    {
        // As noted in the header we don't support multiple read layouts for Images.
        // We allow duplicate uses in the RP to accomodate for normal GL sampler usage.
        if (!usesImageInRenderPass(*image))
        {
            mRenderPassUsedImages.insert(image->getImageSerial().getValue());
        }
    }
}

void CommandBufferHelper::imageWrite(ContextVk *contextVk,
                                     gl::LevelIndex level,
                                     uint32_t layerStart,
                                     uint32_t layerCount,
                                     VkImageAspectFlags aspectFlags,
                                     ImageLayout imageLayout,
                                     AliasingMode aliasingMode,
                                     ImageHelper *image)
{
    image->retain(&contextVk->getResourceUseList());
    image->onWrite(level, 1, layerStart, layerCount, aspectFlags);
    // Write always requires a barrier
    updateImageLayoutAndBarrier(contextVk, image, aspectFlags, imageLayout);

    if (mIsRenderPassCommandBuffer)
    {
        // When used as a storage image we allow for aliased writes.
        if (aliasingMode == AliasingMode::Disallowed)
        {
            ASSERT(!usesImageInRenderPass(*image));
        }
        if (!usesImageInRenderPass(*image))
        {
            mRenderPassUsedImages.insert(image->getImageSerial().getValue());
        }
    }
}

void CommandBufferHelper::colorImagesDraw(ResourceUseList *resourceUseList,
                                          ImageHelper *image,
                                          ImageHelper *resolveImage,
                                          PackedAttachmentIndex packedAttachmentIndex)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(packedAttachmentIndex < mColorImagesCount);

    image->retain(resourceUseList);
    if (!usesImageInRenderPass(*image))
    {
        // This is possible due to different layers of the same texture being attached to different
        // attachments
        mRenderPassUsedImages.insert(image->getImageSerial().getValue());
    }
    ASSERT(mColorImages[packedAttachmentIndex] == nullptr);
    mColorImages[packedAttachmentIndex] = image;
    image->setRenderPassUsageFlag(RenderPassUsage::RenderTargetAttachment);

    if (resolveImage)
    {
        resolveImage->retain(resourceUseList);
        if (!usesImageInRenderPass(*resolveImage))
        {
            mRenderPassUsedImages.insert(resolveImage->getImageSerial().getValue());
        }
        ASSERT(mColorResolveImages[packedAttachmentIndex] == nullptr);
        mColorResolveImages[packedAttachmentIndex] = resolveImage;
        resolveImage->setRenderPassUsageFlag(RenderPassUsage::RenderTargetAttachment);
    }
}

void CommandBufferHelper::depthStencilImagesDraw(ResourceUseList *resourceUseList,
                                                 gl::LevelIndex level,
                                                 uint32_t layerStart,
                                                 uint32_t layerCount,
                                                 ImageHelper *image,
                                                 ImageHelper *resolveImage)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(!usesImageInRenderPass(*image));
    ASSERT(!resolveImage || !usesImageInRenderPass(*resolveImage));

    // Because depthStencil buffer's read/write property can change while we build renderpass, we
    // defer the image layout changes until endRenderPass time or when images going away so that we
    // only insert layout change barrier once.
    image->retain(resourceUseList);
    mRenderPassUsedImages.insert(image->getImageSerial().getValue());
    mDepthStencilImage      = image;
    mDepthStencilLevelIndex = level;
    mDepthStencilLayerIndex = layerStart;
    mDepthStencilLayerCount = layerCount;
    image->setRenderPassUsageFlag(RenderPassUsage::RenderTargetAttachment);

    if (resolveImage)
    {
        // Note that the resolve depth/stencil image has the same level/layer index as the
        // depth/stencil image as currently it can only ever come from
        // multisampled-render-to-texture renderbuffers.
        resolveImage->retain(resourceUseList);
        mRenderPassUsedImages.insert(resolveImage->getImageSerial().getValue());
        mDepthStencilResolveImage = resolveImage;
        resolveImage->setRenderPassUsageFlag(RenderPassUsage::RenderTargetAttachment);
    }
}

void CommandBufferHelper::onDepthAccess(ResourceAccess access)
{
    // Update the access for optimizing this render pass's loadOp
    UpdateAccess(&mDepthAccess, access);

    // Update the invalidate state for optimizing this render pass's storeOp
    if (onDepthStencilAccess(access, &mDepthCmdSizeInvalidated, &mDepthCmdSizeDisabled))
    {
        // The attachment is no longer invalid, so restore its content.
        restoreDepthContent();
    }
}

void CommandBufferHelper::onStencilAccess(ResourceAccess access)
{
    // Update the access for optimizing this render pass's loadOp
    UpdateAccess(&mStencilAccess, access);

    // Update the invalidate state for optimizing this render pass's stencilStoreOp
    if (onDepthStencilAccess(access, &mStencilCmdSizeInvalidated, &mStencilCmdSizeDisabled))
    {
        // The attachment is no longer invalid, so restore its content.
        restoreStencilContent();
    }
}

bool CommandBufferHelper::onDepthStencilAccess(ResourceAccess access,
                                               uint32_t *cmdCountInvalidated,
                                               uint32_t *cmdCountDisabled)
{
    if (*cmdCountInvalidated == kInfiniteCmdSize)
    {
        // If never invalidated or no longer invalidated, return early.
        return false;
    }
    if (access == ResourceAccess::Write)
    {
        // Drawing to this attachment is being enabled.  Assume that drawing will immediately occur
        // after this attachment is enabled, and that means that the attachment will no longer be
        // invalidated.
        *cmdCountInvalidated = kInfiniteCmdSize;
        *cmdCountDisabled    = kInfiniteCmdSize;
        // Return true to indicate that the store op should remain STORE and that mContentDefined
        // should be set to true;
        return true;
    }
    else
    {
        // Drawing to this attachment is being disabled.
        if (hasWriteAfterInvalidate(*cmdCountInvalidated, *cmdCountDisabled))
        {
            // The attachment was previously drawn while enabled, and so is no longer invalidated.
            *cmdCountInvalidated = kInfiniteCmdSize;
            *cmdCountDisabled    = kInfiniteCmdSize;
            // Return true to indicate that the store op should remain STORE and that
            // mContentDefined should be set to true;
            return true;
        }
        else
        {
            // Get the latest CmdSize at the start of being disabled.  At the end of the render
            // pass, cmdCountDisabled is <= the actual command buffer size, and so it's compared
            // with cmdCountInvalidated.  If the same, the attachment is still invalidated.
            *cmdCountDisabled = mCommandBuffer.getCommandSize();
            return false;
        }
    }
}

void CommandBufferHelper::updateStartedRenderPassWithDepthMode(bool readOnlyDepthStencilMode)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(mRenderPassStarted);

    if (mDepthStencilImage)
    {
        if (readOnlyDepthStencilMode)
        {
            mDepthStencilImage->setRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment);
        }
        else
        {
            mDepthStencilImage->clearRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment);
        }
    }

    if (mDepthStencilResolveImage)
    {
        if (readOnlyDepthStencilMode)
        {
            mDepthStencilResolveImage->setRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment);
        }
        else
        {
            mDepthStencilResolveImage->clearRenderPassUsageFlag(
                RenderPassUsage::ReadOnlyAttachment);
        }
    }
}

void CommandBufferHelper::restoreDepthContent()
{
    // Note that the image may have been deleted since the render pass has started.
    if (mDepthStencilImage)
    {
        ASSERT(mDepthStencilImage->valid());
        mDepthStencilImage->restoreSubresourceContent(
            mDepthStencilLevelIndex, mDepthStencilLayerIndex, mDepthStencilLayerCount);
        mDepthInvalidateArea = gl::Rectangle();
    }
}

void CommandBufferHelper::restoreStencilContent()
{
    // Note that the image may have been deleted since the render pass has started.
    if (mDepthStencilImage)
    {
        ASSERT(mDepthStencilImage->valid());
        mDepthStencilImage->restoreSubresourceStencilContent(
            mDepthStencilLevelIndex, mDepthStencilLayerIndex, mDepthStencilLayerCount);
        mStencilInvalidateArea = gl::Rectangle();
    }
}

void CommandBufferHelper::executeBarriers(const angle::FeaturesVk &features,
                                          PrimaryCommandBuffer *primary)
{
    // make a local copy for faster access
    PipelineStagesMask mask = mPipelineBarrierMask;
    if (mask.none())
    {
        return;
    }

    if (features.preferAggregateBarrierCalls.enabled)
    {
        PipelineStagesMask::Iterator iter = mask.begin();
        PipelineBarrier &barrier          = mPipelineBarriers[*iter];
        for (++iter; iter != mask.end(); ++iter)
        {
            barrier.merge(&mPipelineBarriers[*iter]);
        }
        barrier.execute(primary);
    }
    else
    {
        for (PipelineStage pipelineStage : mask)
        {
            PipelineBarrier &barrier = mPipelineBarriers[pipelineStage];
            barrier.execute(primary);
        }
    }
    mPipelineBarrierMask.reset();
}

void CommandBufferHelper::updateImageLayoutAndBarrier(Context *context,
                                                      ImageHelper *image,
                                                      VkImageAspectFlags aspectFlags,
                                                      ImageLayout imageLayout)
{
    PipelineStage barrierIndex = kImageMemoryBarrierData[imageLayout].barrierIndex;
    ASSERT(barrierIndex != PipelineStage::InvalidEnum);
    PipelineBarrier *barrier = &mPipelineBarriers[barrierIndex];
    if (image->updateLayoutAndBarrier(context, aspectFlags, imageLayout, barrier))
    {
        mPipelineBarrierMask.set(barrierIndex);
    }
}

void CommandBufferHelper::finalizeColorImageLayout(Context *context,
                                                   ImageHelper *image,
                                                   PackedAttachmentIndex packedAttachmentIndex,
                                                   bool isResolveImage)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(packedAttachmentIndex < mColorImagesCount);
    ASSERT(image != nullptr);

    // Do layout change.
    ImageLayout imageLayout;
    if (image->usedByCurrentRenderPassAsAttachmentAndSampler())
    {
        // texture code already picked layout and inserted barrier
        imageLayout = image->getCurrentImageLayout();
        ASSERT(imageLayout == ImageLayout::ColorAttachmentAndFragmentShaderRead ||
               imageLayout == ImageLayout::ColorAttachmentAndAllShadersRead);
    }
    else
    {
        imageLayout = ImageLayout::ColorAttachment;
        updateImageLayoutAndBarrier(context, image, VK_IMAGE_ASPECT_COLOR_BIT, imageLayout);
    }

    if (!isResolveImage)
    {
        mAttachmentOps.setLayouts(packedAttachmentIndex, imageLayout, imageLayout);
    }

    if (mImageOptimizeForPresent == image)
    {
        ASSERT(packedAttachmentIndex == kAttachmentIndexZero);
        // Use finalLayout instead of extra barrier for layout change to present
        mImageOptimizeForPresent->setCurrentImageLayout(vk::ImageLayout::Present);
        // TODO(syoussefi):  We currently don't store the layout of the resolve attachments, so once
        // multisampled backbuffers are optimized to use resolve attachments, this information needs
        // to be stored somewhere.  http://anglebug.com/4836
        SetBitField(mAttachmentOps[packedAttachmentIndex].finalLayout,
                    mImageOptimizeForPresent->getCurrentImageLayout());
        mImageOptimizeForPresent = nullptr;
    }

    image->resetRenderPassUsageFlags();
}

void CommandBufferHelper::finalizeDepthStencilImageLayout(Context *context)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(mDepthStencilImage);

    // Do depth stencil layout change.
    ImageLayout imageLayout;
    bool barrierRequired;

    if (mDepthStencilImage->usedByCurrentRenderPassAsAttachmentAndSampler())
    {
        // texture code already picked layout and inserted barrier
        imageLayout = mDepthStencilImage->getCurrentImageLayout();
        if (mDepthStencilImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment))
        {
            ASSERT(imageLayout == ImageLayout::DSAttachmentReadAndFragmentShaderRead ||
                   imageLayout == ImageLayout::DSAttachmentReadAndAllShadersRead);
            barrierRequired = mDepthStencilImage->isReadBarrierNecessary(imageLayout);
        }
        else
        {
            ASSERT(imageLayout == ImageLayout::DSAttachmentWriteAndFragmentShaderRead ||
                   imageLayout == ImageLayout::DSAttachmentWriteAndAllShadersRead);
            barrierRequired = true;
        }
    }
    else if (mDepthStencilImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment))
    {
        imageLayout     = ImageLayout::DepthStencilAttachmentReadOnly;
        barrierRequired = mDepthStencilImage->isReadBarrierNecessary(imageLayout);
    }
    else
    {
        // Write always requires a barrier
        imageLayout     = ImageLayout::DepthStencilAttachment;
        barrierRequired = true;
    }

    mAttachmentOps.setLayouts(mDepthStencilAttachmentIndex, imageLayout, imageLayout);

    if (barrierRequired)
    {
        const angle::Format &format = mDepthStencilImage->getFormat().actualImageFormat();
        ASSERT(format.hasDepthOrStencilBits());
        VkImageAspectFlags aspectFlags = GetDepthStencilAspectFlags(format);
        updateImageLayoutAndBarrier(context, mDepthStencilImage, aspectFlags, imageLayout);
    }
}

void CommandBufferHelper::finalizeDepthStencilResolveImageLayout(Context *context)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(mDepthStencilImage);
    ASSERT(!mDepthStencilResolveImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment));

    ImageLayout imageLayout     = ImageLayout::DepthStencilResolveAttachment;
    const angle::Format &format = mDepthStencilResolveImage->getFormat().actualImageFormat();
    ASSERT(format.hasDepthOrStencilBits());
    VkImageAspectFlags aspectFlags = GetDepthStencilAspectFlags(format);

    updateImageLayoutAndBarrier(context, mDepthStencilResolveImage, aspectFlags, imageLayout);

    if (!mDepthStencilResolveImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment))
    {
        ASSERT(mDepthStencilAttachmentIndex != kAttachmentIndexInvalid);
        const PackedAttachmentOpsDesc &dsOps = mAttachmentOps[mDepthStencilAttachmentIndex];

        // If the image is being written to, mark its contents defined.
        VkImageAspectFlags definedAspects = 0;
        if (!dsOps.isInvalidated)
        {
            definedAspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (!dsOps.isStencilInvalidated)
        {
            definedAspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        if (definedAspects != 0)
        {
            mDepthStencilResolveImage->onWrite(mDepthStencilLevelIndex, 1, mDepthStencilLayerIndex,
                                               mDepthStencilLayerCount, definedAspects);
        }
    }

    mDepthStencilResolveImage->resetRenderPassUsageFlags();
}

void CommandBufferHelper::finalizeImageLayout(Context *context, const ImageHelper *image)
{
    ASSERT(mIsRenderPassCommandBuffer);

    if (image->hasRenderPassUsageFlag(RenderPassUsage::RenderTargetAttachment))
    {
        for (PackedAttachmentIndex index = kAttachmentIndexZero; index < mColorImagesCount; ++index)
        {
            if (mColorImages[index] == image)
            {
                finalizeColorImageLayout(context, mColorImages[index], index, false);
                mColorImages[index] = nullptr;
            }
            else if (mColorResolveImages[index] == image)
            {
                finalizeColorImageLayout(context, mColorResolveImages[index], index, true);
                mColorResolveImages[index] = nullptr;
            }
        }
    }

    if (mDepthStencilImage == image)
    {
        finalizeDepthStencilImageLayoutAndLoadStore(context);
        mDepthStencilImage = nullptr;
    }

    if (mDepthStencilResolveImage == image)
    {
        finalizeDepthStencilResolveImageLayout(context);
        mDepthStencilResolveImage = nullptr;
    }
}

void CommandBufferHelper::finalizeDepthStencilLoadStore(Context *context)
{
    ASSERT(mDepthStencilAttachmentIndex != kAttachmentIndexInvalid);

    PackedAttachmentOpsDesc &dsOps = mAttachmentOps[mDepthStencilAttachmentIndex];

    // This has to be called after layout been finalized
    ASSERT(dsOps.initialLayout != static_cast<uint16_t>(ImageLayout::Undefined));

    // Ensure we don't write to a read-only RenderPass. (ReadOnly -> !Write)
    ASSERT(!mDepthStencilImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment) ||
           (mDepthAccess != ResourceAccess::Write && mStencilAccess != ResourceAccess::Write));

    // If the attachment is invalidated, skip the store op.  If we are not loading or clearing the
    // attachment and the attachment has not been used, auto-invalidate it.
    const bool depthNotLoaded = dsOps.loadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE &&
                                !mRenderPassDesc.hasDepthUnresolveAttachment();
    if (isInvalidated(mDepthCmdSizeInvalidated, mDepthCmdSizeDisabled) ||
        (depthNotLoaded && mDepthAccess != ResourceAccess::Write))
    {
        dsOps.storeOp       = RenderPassStoreOp::DontCare;
        dsOps.isInvalidated = true;
    }
    else if (hasWriteAfterInvalidate(mDepthCmdSizeInvalidated, mDepthCmdSizeDisabled))
    {
        // The depth attachment was invalidated, but is now valid.  Let the image know the contents
        // are now defined so a future render pass would use loadOp=LOAD.
        restoreDepthContent();
    }
    const bool stencilNotLoaded = dsOps.stencilLoadOp == VK_ATTACHMENT_LOAD_OP_DONT_CARE &&
                                  !mRenderPassDesc.hasStencilUnresolveAttachment();
    if (isInvalidated(mStencilCmdSizeInvalidated, mStencilCmdSizeDisabled) ||
        (stencilNotLoaded && mStencilAccess != ResourceAccess::Write))
    {
        dsOps.stencilStoreOp       = RenderPassStoreOp::DontCare;
        dsOps.isStencilInvalidated = true;
    }
    else if (hasWriteAfterInvalidate(mStencilCmdSizeInvalidated, mStencilCmdSizeDisabled))
    {
        // The stencil attachment was invalidated, but is now valid.  Let the image know the
        // contents are now defined so a future render pass would use loadOp=LOAD.
        restoreStencilContent();
    }

    // For read only depth stencil, we can use StoreOpNone if available. DONT_CARE is still
    // preferred, so do this after finish the DONT_CARE handling.
    if (mDepthStencilImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment) &&
        context->getRenderer()->getFeatures().supportsRenderPassStoreOpNoneQCOM.enabled)
    {
        if (dsOps.storeOp == RenderPassStoreOp::Store)
        {
            dsOps.storeOp = RenderPassStoreOp::NoneQCOM;
        }
        if (dsOps.stencilStoreOp == RenderPassStoreOp::Store)
        {
            dsOps.stencilStoreOp = RenderPassStoreOp::NoneQCOM;
        }
    }

    // If we are loading or clearing the attachment, but the attachment has not been used, and the
    // data has also not been stored back into attachment, then just skip the load/clear op.
    if (mDepthAccess == ResourceAccess::Unused && dsOps.storeOp == VK_ATTACHMENT_STORE_OP_DONT_CARE)
    {
        dsOps.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    if (mStencilAccess == ResourceAccess::Unused &&
        dsOps.stencilStoreOp == RenderPassStoreOp::DontCare)
    {
        dsOps.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    }

    // This has to be done after storeOp has been finalized.
    if (!mDepthStencilImage->hasRenderPassUsageFlag(RenderPassUsage::ReadOnlyAttachment))
    {
        // If the image is being written to, mark its contents defined.
        VkImageAspectFlags definedAspects = 0;
        if (dsOps.storeOp == VK_ATTACHMENT_STORE_OP_STORE)
        {
            definedAspects |= VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (dsOps.stencilStoreOp == VK_ATTACHMENT_STORE_OP_STORE)
        {
            definedAspects |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
        if (definedAspects != 0)
        {
            mDepthStencilImage->onWrite(mDepthStencilLevelIndex, 1, mDepthStencilLayerIndex,
                                        mDepthStencilLayerCount, definedAspects);
        }
    }
}

void CommandBufferHelper::finalizeDepthStencilImageLayoutAndLoadStore(Context *context)
{
    finalizeDepthStencilImageLayout(context);
    finalizeDepthStencilLoadStore(context);
    mDepthStencilImage->resetRenderPassUsageFlags();
}

void CommandBufferHelper::beginRenderPass(const Framebuffer &framebuffer,
                                          const gl::Rectangle &renderArea,
                                          const RenderPassDesc &renderPassDesc,
                                          const AttachmentOpsArray &renderPassAttachmentOps,
                                          const vk::PackedAttachmentCount colorAttachmentCount,
                                          const PackedAttachmentIndex depthStencilAttachmentIndex,
                                          const PackedClearValuesArray &clearValues,
                                          CommandBuffer **commandBufferOut)
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(empty());

    mRenderPassDesc              = renderPassDesc;
    mAttachmentOps               = renderPassAttachmentOps;
    mDepthStencilAttachmentIndex = depthStencilAttachmentIndex;
    mColorImagesCount            = colorAttachmentCount;
    mFramebuffer.setHandle(framebuffer.getHandle());
    mRenderArea       = renderArea;
    mClearValues      = clearValues;
    *commandBufferOut = &mCommandBuffer;

    mRenderPassStarted = true;
    mCounter++;
}

void CommandBufferHelper::endRenderPass(ContextVk *contextVk)
{
    for (PackedAttachmentIndex index = kAttachmentIndexZero; index < mColorImagesCount; ++index)
    {
        if (mColorImages[index])
        {
            finalizeColorImageLayout(contextVk, mColorImages[index], index, false);
        }
        if (mColorResolveImages[index])
        {
            finalizeColorImageLayout(contextVk, mColorResolveImages[index], index, true);
        }
    }

    if (mDepthStencilAttachmentIndex == kAttachmentIndexInvalid)
    {
        return;
    }

    // Do depth stencil layout change and load store optimization.
    if (mDepthStencilImage)
    {
        finalizeDepthStencilImageLayoutAndLoadStore(contextVk);
    }
    if (mDepthStencilResolveImage)
    {
        finalizeDepthStencilResolveImageLayout(contextVk);
    }
}

void CommandBufferHelper::beginTransformFeedback(size_t validBufferCount,
                                                 const VkBuffer *counterBuffers,
                                                 bool rebindBuffers)
{
    ASSERT(mIsRenderPassCommandBuffer);
    mValidTransformFeedbackBufferCount = static_cast<uint32_t>(validBufferCount);
    mRebindTransformFeedbackBuffers    = rebindBuffers;

    for (size_t index = 0; index < validBufferCount; index++)
    {
        mTransformFeedbackCounterBuffers[index] = counterBuffers[index];
    }
}

void CommandBufferHelper::endTransformFeedback()
{
    ASSERT(mIsRenderPassCommandBuffer);
    pauseTransformFeedback();
    mValidTransformFeedbackBufferCount = 0;
}

void CommandBufferHelper::invalidateRenderPassColorAttachment(PackedAttachmentIndex attachmentIndex)
{
    ASSERT(mIsRenderPassCommandBuffer);
    SetBitField(mAttachmentOps[attachmentIndex].storeOp, RenderPassStoreOp::DontCare);
    mAttachmentOps[attachmentIndex].isInvalidated = true;
}

void CommandBufferHelper::invalidateRenderPassDepthAttachment(const gl::DepthStencilState &dsState,
                                                              const gl::Rectangle &invalidateArea)
{
    ASSERT(mIsRenderPassCommandBuffer);
    // Keep track of the size of commands in the command buffer.  If the size grows in the
    // future, that implies that drawing occured since invalidated.
    mDepthCmdSizeInvalidated = mCommandBuffer.getCommandSize();

    // Also track the size if the attachment is currently disabled.
    const bool isDepthWriteEnabled = dsState.depthTest && dsState.depthMask;
    mDepthCmdSizeDisabled = isDepthWriteEnabled ? kInfiniteCmdSize : mDepthCmdSizeInvalidated;

    // Set/extend the invalidate area.
    ExtendRenderPassInvalidateArea(invalidateArea, &mDepthInvalidateArea);
}

void CommandBufferHelper::invalidateRenderPassStencilAttachment(
    const gl::DepthStencilState &dsState,
    const gl::Rectangle &invalidateArea)
{
    ASSERT(mIsRenderPassCommandBuffer);
    // Keep track of the size of commands in the command buffer.  If the size grows in the
    // future, that implies that drawing occured since invalidated.
    mStencilCmdSizeInvalidated = mCommandBuffer.getCommandSize();

    // Also track the size if the attachment is currently disabled.
    const bool isStencilWriteEnabled =
        dsState.stencilTest && (!dsState.isStencilNoOp() || !dsState.isStencilBackNoOp());
    mStencilCmdSizeDisabled = isStencilWriteEnabled ? kInfiniteCmdSize : mStencilCmdSizeInvalidated;

    // Set/extend the invalidate area.
    ExtendRenderPassInvalidateArea(invalidateArea, &mStencilInvalidateArea);
}

angle::Result CommandBufferHelper::flushToPrimary(const angle::FeaturesVk &features,
                                                  PrimaryCommandBuffer *primary,
                                                  const RenderPass *renderPass)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandBufferHelper::flushToPrimary");
    ASSERT(!empty());

    // Commands that are added to primary before beginRenderPass command
    executeBarriers(features, primary);

    if (mIsRenderPassCommandBuffer)
    {
        ASSERT(renderPass != nullptr);

        VkRenderPassBeginInfo beginInfo    = {};
        beginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.renderPass               = renderPass->getHandle();
        beginInfo.framebuffer              = mFramebuffer.getHandle();
        beginInfo.renderArea.offset.x      = static_cast<uint32_t>(mRenderArea.x);
        beginInfo.renderArea.offset.y      = static_cast<uint32_t>(mRenderArea.y);
        beginInfo.renderArea.extent.width  = static_cast<uint32_t>(mRenderArea.width);
        beginInfo.renderArea.extent.height = static_cast<uint32_t>(mRenderArea.height);
        beginInfo.clearValueCount = static_cast<uint32_t>(mRenderPassDesc.attachmentCount());
        beginInfo.pClearValues    = mClearValues.data();

        // Run commands inside the RenderPass.
        primary->beginRenderPass(beginInfo, VK_SUBPASS_CONTENTS_INLINE);
        mCommandBuffer.executeCommands(primary->getHandle());
        primary->endRenderPass();
    }
    else
    {
        mCommandBuffer.executeCommands(primary->getHandle());
    }

    // Restart the command buffer.
    reset();

    return angle::Result::Continue;
}

void CommandBufferHelper::updateRenderPassForResolve(ContextVk *contextVk,
                                                     Framebuffer *newFramebuffer,
                                                     const RenderPassDesc &renderPassDesc)
{
    ASSERT(newFramebuffer);
    mFramebuffer.setHandle(newFramebuffer->getHandle());
    mRenderPassDesc = renderPassDesc;
}

// Helper functions used below
char GetLoadOpShorthand(uint32_t loadOp)
{
    switch (loadOp)
    {
        case VK_ATTACHMENT_LOAD_OP_CLEAR:
            return 'C';
        case VK_ATTACHMENT_LOAD_OP_LOAD:
            return 'L';
        default:
            return 'D';
    }
}

char GetStoreOpShorthand(RenderPassStoreOp storeOp)
{
    switch (storeOp)
    {
        case RenderPassStoreOp::Store:
            return 'S';
        case RenderPassStoreOp::NoneQCOM:
            return 'N';
        default:
            return 'D';
    }
}

void CommandBufferHelper::addCommandDiagnostics(ContextVk *contextVk)
{
    std::ostringstream out;

    out << "Memory Barrier: ";
    for (PipelineBarrier &barrier : mPipelineBarriers)
    {
        if (!barrier.isEmpty())
        {
            barrier.addDiagnosticsString(out);
        }
    }
    out << "\\l";

    if (mIsRenderPassCommandBuffer)
    {
        size_t attachmentCount             = mRenderPassDesc.attachmentCount();
        size_t depthStencilAttachmentCount = mRenderPassDesc.hasDepthStencilAttachment() ? 1 : 0;
        size_t colorAttachmentCount        = attachmentCount - depthStencilAttachmentCount;

        PackedAttachmentIndex attachmentIndexVk(0);
        std::string loadOps, storeOps;

        if (colorAttachmentCount > 0)
        {
            loadOps += " Color: ";
            storeOps += " Color: ";

            for (size_t i = 0; i < colorAttachmentCount; ++i)
            {
                loadOps += GetLoadOpShorthand(mAttachmentOps[attachmentIndexVk].loadOp);
                storeOps += GetStoreOpShorthand(
                    static_cast<RenderPassStoreOp>(mAttachmentOps[attachmentIndexVk].storeOp));
                ++attachmentIndexVk;
            }
        }

        if (depthStencilAttachmentCount > 0)
        {
            ASSERT(depthStencilAttachmentCount == 1);

            loadOps += " Depth/Stencil: ";
            storeOps += " Depth/Stencil: ";

            loadOps += GetLoadOpShorthand(mAttachmentOps[attachmentIndexVk].loadOp);
            loadOps += GetLoadOpShorthand(mAttachmentOps[attachmentIndexVk].stencilLoadOp);

            storeOps += GetStoreOpShorthand(
                static_cast<RenderPassStoreOp>(mAttachmentOps[attachmentIndexVk].storeOp));
            storeOps += GetStoreOpShorthand(
                static_cast<RenderPassStoreOp>(mAttachmentOps[attachmentIndexVk].stencilStoreOp));
        }

        if (attachmentCount > 0)
        {
            out << "LoadOp:  " << loadOps << "\\l";
            out << "StoreOp: " << storeOps << "\\l";
        }
    }
    out << mCommandBuffer.dumpCommands("\\l");
    contextVk->addCommandBufferDiagnostics(out.str());
}

void CommandBufferHelper::resumeTransformFeedback()
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(isTransformFeedbackStarted());

    uint32_t numCounterBuffers =
        mRebindTransformFeedbackBuffers ? 0 : mValidTransformFeedbackBufferCount;

    mRebindTransformFeedbackBuffers    = false;
    mIsTransformFeedbackActiveUnpaused = true;

    mCommandBuffer.beginTransformFeedback(0, numCounterBuffers,
                                          mTransformFeedbackCounterBuffers.data(), nullptr);
}

void CommandBufferHelper::pauseTransformFeedback()
{
    ASSERT(mIsRenderPassCommandBuffer);
    ASSERT(isTransformFeedbackStarted() && isTransformFeedbackActiveUnpaused());
    mIsTransformFeedbackActiveUnpaused = false;
    mCommandBuffer.endTransformFeedback(0, mValidTransformFeedbackBufferCount,
                                        mTransformFeedbackCounterBuffers.data(), nullptr);
}

void CommandBufferHelper::updateRenderPassColorClear(PackedAttachmentIndex colorIndexVk,
                                                     const VkClearValue &clearValue)
{
    mAttachmentOps.setClearOp(colorIndexVk);
    mClearValues.store(colorIndexVk, VK_IMAGE_ASPECT_COLOR_BIT, clearValue);
}

void CommandBufferHelper::updateRenderPassDepthStencilClear(VkImageAspectFlags aspectFlags,
                                                            const VkClearValue &clearValue)
{
    // Don't overwrite prior clear values for individual aspects.
    VkClearValue combinedClearValue = mClearValues[mDepthStencilAttachmentIndex];

    if ((aspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) != 0)
    {
        mAttachmentOps.setClearOp(mDepthStencilAttachmentIndex);
        combinedClearValue.depthStencil.depth = clearValue.depthStencil.depth;
    }

    if ((aspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT) != 0)
    {
        mAttachmentOps.setClearStencilOp(mDepthStencilAttachmentIndex);
        combinedClearValue.depthStencil.stencil = clearValue.depthStencil.stencil;
    }

    // Bypass special D/S handling. This clear values array stores values packed.
    mClearValues.storeNoDepthStencil(mDepthStencilAttachmentIndex, combinedClearValue);
}

void CommandBufferHelper::growRenderArea(ContextVk *contextVk, const gl::Rectangle &newRenderArea)
{
    ASSERT(mIsRenderPassCommandBuffer);

    // The render area is grown such that it covers both the previous and the new render areas.
    gl::GetEnclosingRectangle(mRenderArea, newRenderArea, &mRenderArea);

    // Remove invalidates that are no longer applicable.
    if (!mDepthInvalidateArea.empty() && !mDepthInvalidateArea.encloses(mRenderArea))
    {
        ANGLE_PERF_WARNING(
            contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
            "InvalidateSubFramebuffer for depth discarded due to increased scissor region");
        mDepthInvalidateArea     = gl::Rectangle();
        mDepthCmdSizeInvalidated = kInfiniteCmdSize;
    }
    if (!mStencilInvalidateArea.empty() && !mStencilInvalidateArea.encloses(mRenderArea))
    {
        ANGLE_PERF_WARNING(
            contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
            "InvalidateSubFramebuffer for stencil discarded due to increased scissor region");
        mStencilInvalidateArea     = gl::Rectangle();
        mStencilCmdSizeInvalidated = kInfiniteCmdSize;
    }
}

// DynamicBuffer implementation.
DynamicBuffer::DynamicBuffer()
    : mUsage(0),
      mHostVisible(false),
      mPolicy(DynamicBufferPolicy::OneShotUse),
      mInitialSize(0),
      mNextAllocationOffset(0),
      mLastFlushOrInvalidateOffset(0),
      mSize(0),
      mAlignment(0),
      mMemoryPropertyFlags(0)
{}

DynamicBuffer::DynamicBuffer(DynamicBuffer &&other)
    : mUsage(other.mUsage),
      mHostVisible(other.mHostVisible),
      mPolicy(other.mPolicy),
      mInitialSize(other.mInitialSize),
      mBuffer(std::move(other.mBuffer)),
      mNextAllocationOffset(other.mNextAllocationOffset),
      mLastFlushOrInvalidateOffset(other.mLastFlushOrInvalidateOffset),
      mSize(other.mSize),
      mAlignment(other.mAlignment),
      mMemoryPropertyFlags(other.mMemoryPropertyFlags),
      mInFlightBuffers(std::move(other.mInFlightBuffers)),
      mBufferFreeList(std::move(other.mBufferFreeList))
{}

void DynamicBuffer::init(RendererVk *renderer,
                         VkBufferUsageFlags usage,
                         size_t alignment,
                         size_t initialSize,
                         bool hostVisible,
                         DynamicBufferPolicy policy)
{
    VkMemoryPropertyFlags memoryPropertyFlags =
        (hostVisible) ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    initWithFlags(renderer, usage, alignment, initialSize, memoryPropertyFlags, policy);
}

void DynamicBuffer::initWithFlags(RendererVk *renderer,
                                  VkBufferUsageFlags usage,
                                  size_t alignment,
                                  size_t initialSize,
                                  VkMemoryPropertyFlags memoryPropertyFlags,
                                  DynamicBufferPolicy policy)
{
    mUsage               = usage;
    mHostVisible         = ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0);
    mMemoryPropertyFlags = memoryPropertyFlags;
    mPolicy              = policy;

    // Check that we haven't overriden the initial size of the buffer in setMinimumSizeForTesting.
    if (mInitialSize == 0)
    {
        mInitialSize = initialSize;
        mSize        = 0;
    }

    // Workaround for the mock ICD not supporting allocations greater than 0x1000.
    // Could be removed if https://github.com/KhronosGroup/Vulkan-Tools/issues/84 is fixed.
    if (renderer->isMockICDEnabled())
    {
        mSize = std::min<size_t>(mSize, 0x1000);
    }

    requireAlignment(renderer, alignment);
}

DynamicBuffer::~DynamicBuffer()
{
    ASSERT(mBuffer == nullptr);
    ASSERT(mInFlightBuffers.empty());
    ASSERT(mBufferFreeList.empty());
}

angle::Result DynamicBuffer::allocateNewBuffer(ContextVk *contextVk)
{
    // Gather statistics
    const gl::OverlayType *overlay = contextVk->getOverlay();
    if (overlay->isEnabled())
    {
        gl::RunningGraphWidget *dynamicBufferAllocations =
            overlay->getRunningGraphWidget(gl::WidgetId::VulkanDynamicBufferAllocations);
        dynamicBufferAllocations->add(1);
    }

    // Allocate the buffer
    ASSERT(!mBuffer);
    mBuffer = std::make_unique<BufferHelper>();

    VkBufferCreateInfo createInfo    = {};
    createInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.flags                 = 0;
    createInfo.size                  = mSize;
    createInfo.usage                 = mUsage;
    createInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;
    createInfo.pQueueFamilyIndices   = nullptr;

    return mBuffer->init(contextVk, createInfo, mMemoryPropertyFlags);
}

bool DynamicBuffer::allocateFromCurrentBuffer(size_t sizeInBytes,
                                              uint8_t **ptrOut,
                                              VkDeviceSize *offsetOut)
{
    ASSERT(ptrOut);
    ASSERT(offsetOut);
    size_t sizeToAllocate                                      = roundUp(sizeInBytes, mAlignment);
    angle::base::CheckedNumeric<size_t> checkedNextWriteOffset = mNextAllocationOffset;
    checkedNextWriteOffset += sizeToAllocate;

    if (!checkedNextWriteOffset.IsValid() || checkedNextWriteOffset.ValueOrDie() >= mSize)
    {
        return false;
    }

    ASSERT(mBuffer != nullptr);
    ASSERT(mHostVisible);
    ASSERT(mBuffer->getMappedMemory());

    *ptrOut    = mBuffer->getMappedMemory() + mNextAllocationOffset;
    *offsetOut = static_cast<VkDeviceSize>(mNextAllocationOffset);

    mNextAllocationOffset += static_cast<uint32_t>(sizeToAllocate);
    return true;
}

angle::Result DynamicBuffer::allocateWithAlignment(ContextVk *contextVk,
                                                   size_t sizeInBytes,
                                                   size_t alignment,
                                                   uint8_t **ptrOut,
                                                   VkBuffer *bufferOut,
                                                   VkDeviceSize *offsetOut,
                                                   bool *newBufferAllocatedOut)
{
    mNextAllocationOffset =
        roundUp<uint32_t>(mNextAllocationOffset, static_cast<uint32_t>(alignment));
    size_t sizeToAllocate = roundUp(sizeInBytes, mAlignment);

    angle::base::CheckedNumeric<size_t> checkedNextWriteOffset = mNextAllocationOffset;
    checkedNextWriteOffset += sizeToAllocate;

    if (!checkedNextWriteOffset.IsValid() || checkedNextWriteOffset.ValueOrDie() >= mSize)
    {
        if (mBuffer)
        {
            // Make sure the buffer is not released externally.
            ASSERT(mBuffer->valid());

            ANGLE_TRY(flush(contextVk));

            mInFlightBuffers.push_back(std::move(mBuffer));
            ASSERT(!mBuffer);
        }

        const size_t sizeIgnoringHistory = std::max(mInitialSize, sizeToAllocate);
        if (sizeToAllocate > mSize || sizeIgnoringHistory < mSize / 4)
        {
            mSize = sizeIgnoringHistory;

            // Clear the free list since the free buffers are now either too small or too big.
            ReleaseBufferListToRenderer(contextVk->getRenderer(), &mBufferFreeList);
        }

        // The front of the free list should be the oldest. Thus if it is in use the rest of the
        // free list should be in use as well.
        if (mBufferFreeList.empty() ||
            mBufferFreeList.front()->isCurrentlyInUse(contextVk->getLastCompletedQueueSerial()))
        {
            ANGLE_TRY(allocateNewBuffer(contextVk));
        }
        else
        {
            mBuffer = std::move(mBufferFreeList.front());
            mBufferFreeList.erase(mBufferFreeList.begin());
        }

        ASSERT(mBuffer->getSize() == mSize);

        mNextAllocationOffset        = 0;
        mLastFlushOrInvalidateOffset = 0;

        if (newBufferAllocatedOut != nullptr)
        {
            *newBufferAllocatedOut = true;
        }
    }
    else if (newBufferAllocatedOut != nullptr)
    {
        *newBufferAllocatedOut = false;
    }

    ASSERT(mBuffer != nullptr);

    if (bufferOut != nullptr)
    {
        *bufferOut = mBuffer->getBuffer().getHandle();
    }

    // Optionally map() the buffer if possible
    if (ptrOut)
    {
        ASSERT(mHostVisible);
        uint8_t *mappedMemory;
        ANGLE_TRY(mBuffer->map(contextVk, &mappedMemory));
        *ptrOut = mappedMemory + mNextAllocationOffset;
    }

    if (offsetOut != nullptr)
    {
        *offsetOut = static_cast<VkDeviceSize>(mNextAllocationOffset);
    }

    mNextAllocationOffset += static_cast<uint32_t>(sizeToAllocate);
    return angle::Result::Continue;
}

angle::Result DynamicBuffer::flush(ContextVk *contextVk)
{
    if (mHostVisible && (mNextAllocationOffset > mLastFlushOrInvalidateOffset))
    {
        ASSERT(mBuffer != nullptr);
        ANGLE_TRY(mBuffer->flush(contextVk->getRenderer(), mLastFlushOrInvalidateOffset,
                                 mNextAllocationOffset - mLastFlushOrInvalidateOffset));
        mLastFlushOrInvalidateOffset = mNextAllocationOffset;
    }
    return angle::Result::Continue;
}

angle::Result DynamicBuffer::invalidate(ContextVk *contextVk)
{
    if (mHostVisible && (mNextAllocationOffset > mLastFlushOrInvalidateOffset))
    {
        ASSERT(mBuffer != nullptr);
        ANGLE_TRY(mBuffer->invalidate(contextVk->getRenderer(), mLastFlushOrInvalidateOffset,
                                      mNextAllocationOffset - mLastFlushOrInvalidateOffset));
        mLastFlushOrInvalidateOffset = mNextAllocationOffset;
    }
    return angle::Result::Continue;
}

void DynamicBuffer::release(RendererVk *renderer)
{
    reset();

    ReleaseBufferListToRenderer(renderer, &mInFlightBuffers);
    ReleaseBufferListToRenderer(renderer, &mBufferFreeList);

    if (mBuffer)
    {
        mBuffer->release(renderer);
        mBuffer.reset(nullptr);
    }
}

void DynamicBuffer::releaseInFlightBuffersToResourceUseList(ContextVk *contextVk)
{
    ResourceUseList *resourceUseList = &contextVk->getResourceUseList();
    for (std::unique_ptr<BufferHelper> &bufferHelper : mInFlightBuffers)
    {
        bufferHelper->retain(resourceUseList);

        if (ShouldReleaseFreeBuffer(*bufferHelper, mSize, mPolicy, mBufferFreeList.size()))
        {
            bufferHelper->release(contextVk->getRenderer());
        }
        else
        {
            bufferHelper->unmap(contextVk->getRenderer());
            mBufferFreeList.push_back(std::move(bufferHelper));
        }
    }
    mInFlightBuffers.clear();
}

void DynamicBuffer::releaseInFlightBuffers(ContextVk *contextVk)
{
    for (std::unique_ptr<BufferHelper> &toRelease : mInFlightBuffers)
    {
        if (ShouldReleaseFreeBuffer(*toRelease, mSize, mPolicy, mBufferFreeList.size()))
        {
            toRelease->release(contextVk->getRenderer());
        }
        else
        {
            toRelease->unmap(contextVk->getRenderer());
            mBufferFreeList.push_back(std::move(toRelease));
        }
    }

    mInFlightBuffers.clear();
}

void DynamicBuffer::destroy(RendererVk *renderer)
{
    reset();

    DestroyBufferList(renderer, &mInFlightBuffers);
    DestroyBufferList(renderer, &mBufferFreeList);

    if (mBuffer)
    {
        mBuffer->unmap(renderer);
        mBuffer->destroy(renderer);
        mBuffer.reset(nullptr);
    }
}

void DynamicBuffer::requireAlignment(RendererVk *renderer, size_t alignment)
{
    ASSERT(alignment > 0);

    size_t prevAlignment = mAlignment;

    // If alignment was never set, initialize it with the atom size limit.
    if (prevAlignment == 0)
    {
        prevAlignment =
            static_cast<size_t>(renderer->getPhysicalDeviceProperties().limits.nonCoherentAtomSize);
        ASSERT(gl::isPow2(prevAlignment));
    }

    // We need lcm(prevAlignment, alignment).  Usually, one divides the other so std::max() could be
    // used instead.  Only known case where this assumption breaks is for 3-component types with
    // 16- or 32-bit channels, so that's special-cased to avoid a full-fledged lcm implementation.

    if (gl::isPow2(prevAlignment * alignment))
    {
        ASSERT(alignment % prevAlignment == 0 || prevAlignment % alignment == 0);

        alignment = std::max(prevAlignment, alignment);
    }
    else
    {
        ASSERT(prevAlignment % 3 != 0 || gl::isPow2(prevAlignment / 3));
        ASSERT(alignment % 3 != 0 || gl::isPow2(alignment / 3));

        prevAlignment = prevAlignment % 3 == 0 ? prevAlignment / 3 : prevAlignment;
        alignment     = alignment % 3 == 0 ? alignment / 3 : alignment;

        alignment = std::max(prevAlignment, alignment) * 3;
    }

    // If alignment has changed, make sure the next allocation is done at an aligned offset.
    if (alignment != mAlignment)
    {
        mNextAllocationOffset = roundUp(mNextAllocationOffset, static_cast<uint32_t>(alignment));
    }

    mAlignment = alignment;
}

void DynamicBuffer::setMinimumSizeForTesting(size_t minSize)
{
    // This will really only have an effect next time we call allocate.
    mInitialSize = minSize;

    // Forces a new allocation on the next allocate.
    mSize = 0;
}

void DynamicBuffer::reset()
{
    mSize                        = 0;
    mNextAllocationOffset        = 0;
    mLastFlushOrInvalidateOffset = 0;
}

// DynamicShadowBuffer implementation.
DynamicShadowBuffer::DynamicShadowBuffer() : mInitialSize(0), mSize(0) {}

DynamicShadowBuffer::DynamicShadowBuffer(DynamicShadowBuffer &&other)
    : mInitialSize(other.mInitialSize), mSize(other.mSize), mBuffer(std::move(other.mBuffer))
{}

void DynamicShadowBuffer::init(size_t initialSize)
{
    mInitialSize = initialSize;
}

DynamicShadowBuffer::~DynamicShadowBuffer()
{
    ASSERT(mBuffer.empty());
}

angle::Result DynamicShadowBuffer::allocate(size_t sizeInBytes)
{
    bool result = true;

    // Delete the current buffer, if any
    if (!mBuffer.empty())
    {
        result &= mBuffer.resize(0);
    }

    // Cache the new size
    mSize = std::max(mInitialSize, sizeInBytes);

    // Allocate the buffer
    result &= mBuffer.resize(mSize);

    // If allocation failed, release the buffer and return error.
    if (!result)
    {
        release();
        return angle::Result::Stop;
    }

    return angle::Result::Continue;
}

void DynamicShadowBuffer::release()
{
    reset();

    if (!mBuffer.empty())
    {
        (void)mBuffer.resize(0);
    }
}

void DynamicShadowBuffer::destroy(VkDevice device)
{
    release();
}

void DynamicShadowBuffer::reset()
{
    mSize = 0;
}

// DescriptorPoolHelper implementation.
DescriptorPoolHelper::DescriptorPoolHelper() : mFreeDescriptorSets(0) {}

DescriptorPoolHelper::~DescriptorPoolHelper() = default;

bool DescriptorPoolHelper::hasCapacity(uint32_t descriptorSetCount) const
{
    return mFreeDescriptorSets >= descriptorSetCount;
}

angle::Result DescriptorPoolHelper::init(ContextVk *contextVk,
                                         const std::vector<VkDescriptorPoolSize> &poolSizesIn,
                                         uint32_t maxSets)
{
    if (mDescriptorPool.valid())
    {
        ASSERT(!isCurrentlyInUse(contextVk->getLastCompletedQueueSerial()));
        mDescriptorPool.destroy(contextVk->getDevice());
    }

    // Make a copy of the pool sizes, so we can grow them to satisfy the specified maxSets.
    std::vector<VkDescriptorPoolSize> poolSizes = poolSizesIn;

    for (VkDescriptorPoolSize &poolSize : poolSizes)
    {
        poolSize.descriptorCount *= maxSets;
    }

    VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
    descriptorPoolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.flags                      = 0;
    descriptorPoolInfo.maxSets                    = maxSets;
    descriptorPoolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
    descriptorPoolInfo.pPoolSizes                 = poolSizes.data();

    mFreeDescriptorSets = maxSets;

    ANGLE_VK_TRY(contextVk, mDescriptorPool.init(contextVk->getDevice(), descriptorPoolInfo));

    return angle::Result::Continue;
}

void DescriptorPoolHelper::destroy(VkDevice device)
{
    mDescriptorPool.destroy(device);
}

void DescriptorPoolHelper::release(ContextVk *contextVk)
{
    contextVk->addGarbage(&mDescriptorPool);
}

angle::Result DescriptorPoolHelper::allocateSets(ContextVk *contextVk,
                                                 const VkDescriptorSetLayout *descriptorSetLayout,
                                                 uint32_t descriptorSetCount,
                                                 VkDescriptorSet *descriptorSetsOut)
{
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool              = mDescriptorPool.getHandle();
    allocInfo.descriptorSetCount          = descriptorSetCount;
    allocInfo.pSetLayouts                 = descriptorSetLayout;

    ASSERT(mFreeDescriptorSets >= descriptorSetCount);
    mFreeDescriptorSets -= descriptorSetCount;

    ANGLE_VK_TRY(contextVk, mDescriptorPool.allocateDescriptorSets(contextVk->getDevice(),
                                                                   allocInfo, descriptorSetsOut));

    // The pool is still in use every time a new descriptor set is allocated from it.
    retain(&contextVk->getResourceUseList());

    return angle::Result::Continue;
}

// DynamicDescriptorPool implementation.
DynamicDescriptorPool::DynamicDescriptorPool()
    : mCurrentPoolIndex(0), mCachedDescriptorSetLayout(VK_NULL_HANDLE)
{}

DynamicDescriptorPool::~DynamicDescriptorPool() = default;

angle::Result DynamicDescriptorPool::init(ContextVk *contextVk,
                                          const VkDescriptorPoolSize *setSizes,
                                          size_t setSizeCount,
                                          VkDescriptorSetLayout descriptorSetLayout)
{
    ASSERT(setSizes);
    ASSERT(setSizeCount);
    ASSERT(mCurrentPoolIndex == 0);
    ASSERT(mDescriptorPools.empty() ||
           (mDescriptorPools.size() == 1 &&
            mDescriptorPools[mCurrentPoolIndex]->get().hasCapacity(mMaxSetsPerPool)));
    ASSERT(mCachedDescriptorSetLayout == VK_NULL_HANDLE);

    mPoolSizes.assign(setSizes, setSizes + setSizeCount);
    mCachedDescriptorSetLayout = descriptorSetLayout;

    mDescriptorPools.push_back(new RefCountedDescriptorPoolHelper());
    mCurrentPoolIndex = mDescriptorPools.size() - 1;
    return mDescriptorPools[mCurrentPoolIndex]->get().init(contextVk, mPoolSizes, mMaxSetsPerPool);
}

void DynamicDescriptorPool::destroy(VkDevice device)
{
    for (RefCountedDescriptorPoolHelper *pool : mDescriptorPools)
    {
        ASSERT(!pool->isReferenced());
        pool->get().destroy(device);
        delete pool;
    }

    mDescriptorPools.clear();
    mCurrentPoolIndex          = 0;
    mCachedDescriptorSetLayout = VK_NULL_HANDLE;
}

void DynamicDescriptorPool::release(ContextVk *contextVk)
{
    for (RefCountedDescriptorPoolHelper *pool : mDescriptorPools)
    {
        ASSERT(!pool->isReferenced());
        pool->get().release(contextVk);
        delete pool;
    }

    mDescriptorPools.clear();
    mCurrentPoolIndex          = 0;
    mCachedDescriptorSetLayout = VK_NULL_HANDLE;
}

angle::Result DynamicDescriptorPool::allocateSetsAndGetInfo(
    ContextVk *contextVk,
    const VkDescriptorSetLayout *descriptorSetLayout,
    uint32_t descriptorSetCount,
    RefCountedDescriptorPoolBinding *bindingOut,
    VkDescriptorSet *descriptorSetsOut,
    bool *newPoolAllocatedOut)
{
    ASSERT(!mDescriptorPools.empty());
    ASSERT(*descriptorSetLayout == mCachedDescriptorSetLayout);

    *newPoolAllocatedOut = false;

    if (!bindingOut->valid() || !bindingOut->get().hasCapacity(descriptorSetCount))
    {
        if (!mDescriptorPools[mCurrentPoolIndex]->get().hasCapacity(descriptorSetCount))
        {
            ANGLE_TRY(allocateNewPool(contextVk));
            *newPoolAllocatedOut = true;
        }

        bindingOut->set(mDescriptorPools[mCurrentPoolIndex]);
    }

    return bindingOut->get().allocateSets(contextVk, descriptorSetLayout, descriptorSetCount,
                                          descriptorSetsOut);
}

angle::Result DynamicDescriptorPool::allocateNewPool(ContextVk *contextVk)
{
    bool found = false;

    Serial lastCompletedSerial = contextVk->getLastCompletedQueueSerial();
    for (size_t poolIndex = 0; poolIndex < mDescriptorPools.size(); ++poolIndex)
    {
        if (!mDescriptorPools[poolIndex]->isReferenced() &&
            !mDescriptorPools[poolIndex]->get().isCurrentlyInUse(lastCompletedSerial))
        {
            mCurrentPoolIndex = poolIndex;
            found             = true;
            break;
        }
    }

    if (!found)
    {
        mDescriptorPools.push_back(new RefCountedDescriptorPoolHelper());
        mCurrentPoolIndex = mDescriptorPools.size() - 1;

        static constexpr size_t kMaxPools = 99999;
        ANGLE_VK_CHECK(contextVk, mDescriptorPools.size() < kMaxPools, VK_ERROR_TOO_MANY_OBJECTS);
    }

    // This pool is getting hot, so grow its max size to try and prevent allocating another pool in
    // the future.
    if (mMaxSetsPerPool < kMaxSetsPerPoolMax)
    {
        mMaxSetsPerPool *= mMaxSetsPerPoolMultiplier;
    }

    return mDescriptorPools[mCurrentPoolIndex]->get().init(contextVk, mPoolSizes, mMaxSetsPerPool);
}

// For testing only!
uint32_t DynamicDescriptorPool::GetMaxSetsPerPoolForTesting()
{
    return mMaxSetsPerPool;
}

// For testing only!
void DynamicDescriptorPool::SetMaxSetsPerPoolForTesting(uint32_t maxSetsPerPool)
{
    mMaxSetsPerPool = maxSetsPerPool;
}

// For testing only!
uint32_t DynamicDescriptorPool::GetMaxSetsPerPoolMultiplierForTesting()
{
    return mMaxSetsPerPoolMultiplier;
}

// For testing only!
void DynamicDescriptorPool::SetMaxSetsPerPoolMultiplierForTesting(uint32_t maxSetsPerPoolMultiplier)
{
    mMaxSetsPerPoolMultiplier = maxSetsPerPoolMultiplier;
}

// DynamicallyGrowingPool implementation
template <typename Pool>
DynamicallyGrowingPool<Pool>::DynamicallyGrowingPool()
    : mPoolSize(0), mCurrentPool(0), mCurrentFreeEntry(0)
{}

template <typename Pool>
DynamicallyGrowingPool<Pool>::~DynamicallyGrowingPool() = default;

template <typename Pool>
angle::Result DynamicallyGrowingPool<Pool>::initEntryPool(Context *contextVk, uint32_t poolSize)
{
    ASSERT(mPools.empty() && mPoolStats.empty());
    mPoolSize = poolSize;
    return angle::Result::Continue;
}

template <typename Pool>
void DynamicallyGrowingPool<Pool>::destroyEntryPool()
{
    mPools.clear();
    mPoolStats.clear();
}

template <typename Pool>
bool DynamicallyGrowingPool<Pool>::findFreeEntryPool(ContextVk *contextVk)
{
    Serial lastCompletedQueueSerial = contextVk->getLastCompletedQueueSerial();
    for (size_t i = 0; i < mPools.size(); ++i)
    {
        if (mPoolStats[i].freedCount == mPoolSize &&
            mPoolStats[i].serial <= lastCompletedQueueSerial)
        {
            mCurrentPool      = i;
            mCurrentFreeEntry = 0;

            mPoolStats[i].freedCount = 0;

            return true;
        }
    }

    return false;
}

template <typename Pool>
angle::Result DynamicallyGrowingPool<Pool>::allocateNewEntryPool(ContextVk *contextVk, Pool &&pool)
{
    mPools.push_back(std::move(pool));

    PoolStats poolStats = {0, Serial()};
    mPoolStats.push_back(poolStats);

    mCurrentPool      = mPools.size() - 1;
    mCurrentFreeEntry = 0;

    return angle::Result::Continue;
}

template <typename Pool>
void DynamicallyGrowingPool<Pool>::onEntryFreed(ContextVk *contextVk, size_t poolIndex)
{
    ASSERT(poolIndex < mPoolStats.size() && mPoolStats[poolIndex].freedCount < mPoolSize);

    // Take note of the current serial to avoid reallocating a query in the same pool
    mPoolStats[poolIndex].serial = contextVk->getCurrentQueueSerial();
    ++mPoolStats[poolIndex].freedCount;
}

// DynamicQueryPool implementation
DynamicQueryPool::DynamicQueryPool() = default;

DynamicQueryPool::~DynamicQueryPool() = default;

angle::Result DynamicQueryPool::init(ContextVk *contextVk, VkQueryType type, uint32_t poolSize)
{
    ANGLE_TRY(initEntryPool(contextVk, poolSize));

    mQueryType = type;
    ANGLE_TRY(allocateNewPool(contextVk));

    return angle::Result::Continue;
}

void DynamicQueryPool::destroy(VkDevice device)
{
    for (QueryPool &queryPool : mPools)
    {
        queryPool.destroy(device);
    }

    destroyEntryPool();
}

angle::Result DynamicQueryPool::allocateQuery(ContextVk *contextVk,
                                              QueryHelper *queryOut,
                                              uint32_t queryCount)
{
    ASSERT(!queryOut->valid());

    if (mCurrentFreeEntry + queryCount > mPoolSize)
    {
        // No more queries left in this pool, create another one.
        ANGLE_TRY(allocateNewPool(contextVk));
    }

    uint32_t queryIndex = mCurrentFreeEntry;
    mCurrentFreeEntry += queryCount;

    queryOut->init(this, mCurrentPool, queryIndex, queryCount);

    return angle::Result::Continue;
}

void DynamicQueryPool::freeQuery(ContextVk *contextVk, QueryHelper *query)
{
    if (query->valid())
    {
        size_t poolIndex = query->mQueryPoolIndex;
        ASSERT(getQueryPool(poolIndex).valid());

        onEntryFreed(contextVk, poolIndex);

        query->deinit();
    }
}

angle::Result DynamicQueryPool::allocateNewPool(ContextVk *contextVk)
{
    if (findFreeEntryPool(contextVk))
    {
        return angle::Result::Continue;
    }

    VkQueryPoolCreateInfo queryPoolInfo = {};
    queryPoolInfo.sType                 = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
    queryPoolInfo.flags                 = 0;
    queryPoolInfo.queryType             = mQueryType;
    queryPoolInfo.queryCount            = mPoolSize;
    queryPoolInfo.pipelineStatistics    = 0;

    if (mQueryType == VK_QUERY_TYPE_PIPELINE_STATISTICS)
    {
        queryPoolInfo.pipelineStatistics = VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT;
    }

    QueryPool queryPool;

    ANGLE_VK_TRY(contextVk, queryPool.init(contextVk->getDevice(), queryPoolInfo));

    return allocateNewEntryPool(contextVk, std::move(queryPool));
}

// QueryResult implementation
void QueryResult::setResults(uint64_t *results, uint32_t queryCount)
{
    ASSERT(mResults[0] == 0 && mResults[1] == 0);

    // Accumulate the query results.  For multiview, where multiple query indices are used to return
    // the results, it's undefined how the results are distributed between indices, but the sum is
    // guaranteed to be the desired result.
    for (uint32_t query = 0; query < queryCount; ++query)
    {
        for (uint32_t perQueryIndex = 0; perQueryIndex < mIntsPerResult; ++perQueryIndex)
        {
            mResults[perQueryIndex] += results[query * mIntsPerResult + perQueryIndex];
        }
    }
}

// QueryHelper implementation
QueryHelper::QueryHelper()
    : mDynamicQueryPool(nullptr), mQueryPoolIndex(0), mQuery(0), mQueryCount(0)
{}

QueryHelper::~QueryHelper() {}

// Move constructor
QueryHelper::QueryHelper(QueryHelper &&rhs)
    : Resource(std::move(rhs)),
      mDynamicQueryPool(rhs.mDynamicQueryPool),
      mQueryPoolIndex(rhs.mQueryPoolIndex),
      mQuery(rhs.mQuery),
      mQueryCount(rhs.mQueryCount)
{
    rhs.mDynamicQueryPool = nullptr;
    rhs.mQueryPoolIndex   = 0;
    rhs.mQuery            = 0;
    rhs.mQueryCount       = 0;
}

QueryHelper &QueryHelper::operator=(QueryHelper &&rhs)
{
    std::swap(mDynamicQueryPool, rhs.mDynamicQueryPool);
    std::swap(mQueryPoolIndex, rhs.mQueryPoolIndex);
    std::swap(mQuery, rhs.mQuery);
    std::swap(mQueryCount, rhs.mQueryCount);
    return *this;
}

void QueryHelper::init(const DynamicQueryPool *dynamicQueryPool,
                       const size_t queryPoolIndex,
                       uint32_t query,
                       uint32_t queryCount)
{
    mDynamicQueryPool = dynamicQueryPool;
    mQueryPoolIndex   = queryPoolIndex;
    mQuery            = query;
    mQueryCount       = queryCount;

    ASSERT(mQueryCount <= gl::IMPLEMENTATION_ANGLE_MULTIVIEW_MAX_VIEWS);
}

void QueryHelper::deinit()
{
    mDynamicQueryPool = nullptr;
    mQueryPoolIndex   = 0;
    mQuery            = 0;
    mQueryCount       = 0;
    mUse.release();
    mUse.init();
}

void QueryHelper::beginQueryImpl(ContextVk *contextVk,
                                 CommandBuffer *resetCommandBuffer,
                                 CommandBuffer *commandBuffer)
{
    const QueryPool &queryPool = getQueryPool();
    resetCommandBuffer->resetQueryPool(queryPool, mQuery, mQueryCount);
    commandBuffer->beginQuery(queryPool, mQuery, 0);
}

void QueryHelper::endQueryImpl(ContextVk *contextVk, CommandBuffer *commandBuffer)
{
    commandBuffer->endQuery(getQueryPool(), mQuery);

    // Query results are available after endQuery, retain this query so that we get its serial
    // updated which is used to indicate that query results are (or will be) available.
    retain(&contextVk->getResourceUseList());
}

angle::Result QueryHelper::beginQuery(ContextVk *contextVk)
{
    if (contextVk->hasStartedRenderPass())
    {
        ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());
    }

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer({}, &commandBuffer));

    ANGLE_TRY(contextVk->handleGraphicsEventLog(rx::GraphicsEventCmdBuf::InOutsideCmdBufQueryCmd));

    beginQueryImpl(contextVk, commandBuffer, commandBuffer);

    return angle::Result::Continue;
}

angle::Result QueryHelper::endQuery(ContextVk *contextVk)
{
    if (contextVk->hasStartedRenderPass())
    {
        ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());
    }

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer({}, &commandBuffer));

    ANGLE_TRY(contextVk->handleGraphicsEventLog(rx::GraphicsEventCmdBuf::InOutsideCmdBufQueryCmd));

    endQueryImpl(contextVk, commandBuffer);

    return angle::Result::Continue;
}

angle::Result QueryHelper::beginRenderPassQuery(ContextVk *contextVk)
{
    CommandBuffer *outsideRenderPassCommandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer({}, &outsideRenderPassCommandBuffer));

    CommandBuffer *renderPassCommandBuffer =
        &contextVk->getStartedRenderPassCommands().getCommandBuffer();

    beginQueryImpl(contextVk, outsideRenderPassCommandBuffer, renderPassCommandBuffer);

    return angle::Result::Continue;
}

void QueryHelper::endRenderPassQuery(ContextVk *contextVk)
{
    endQueryImpl(contextVk, &contextVk->getStartedRenderPassCommands().getCommandBuffer());
}

angle::Result QueryHelper::flushAndWriteTimestamp(ContextVk *contextVk)
{
    if (contextVk->hasStartedRenderPass())
    {
        ANGLE_TRY(contextVk->flushCommandsAndEndRenderPass());
    }

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer({}, &commandBuffer));
    writeTimestamp(contextVk, commandBuffer);
    return angle::Result::Continue;
}

void QueryHelper::writeTimestampToPrimary(ContextVk *contextVk, PrimaryCommandBuffer *primary)
{
    // Note that commands may not be flushed at this point.

    const QueryPool &queryPool = getQueryPool();
    primary->resetQueryPool(queryPool, mQuery, mQueryCount);
    primary->writeTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, mQuery);
}

void QueryHelper::writeTimestamp(ContextVk *contextVk, CommandBuffer *commandBuffer)
{
    const QueryPool &queryPool = getQueryPool();
    commandBuffer->resetQueryPool(queryPool, mQuery, mQueryCount);
    commandBuffer->writeTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, queryPool, mQuery);
    // timestamp results are available immediately, retain this query so that we get its serial
    // updated which is used to indicate that query results are (or will be) available.
    retain(&contextVk->getResourceUseList());
}

bool QueryHelper::hasSubmittedCommands() const
{
    return mUse.getSerial().valid();
}

angle::Result QueryHelper::getUint64ResultNonBlocking(ContextVk *contextVk,
                                                      QueryResult *resultOut,
                                                      bool *availableOut)
{
    ASSERT(valid());
    VkResult result;

    // Ensure that we only wait if we have inserted a query in command buffer. Otherwise you will
    // wait forever and trigger GPU timeout.
    if (hasSubmittedCommands())
    {
        constexpr VkQueryResultFlags kFlags = VK_QUERY_RESULT_64_BIT;
        result                              = getResultImpl(contextVk, kFlags, resultOut);
    }
    else
    {
        result     = VK_SUCCESS;
        *resultOut = 0;
    }

    if (result == VK_NOT_READY)
    {
        *availableOut = false;
        return angle::Result::Continue;
    }
    else
    {
        ANGLE_VK_TRY(contextVk, result);
        *availableOut = true;
    }
    return angle::Result::Continue;
}

angle::Result QueryHelper::getUint64Result(ContextVk *contextVk, QueryResult *resultOut)
{
    ASSERT(valid());
    if (hasSubmittedCommands())
    {
        constexpr VkQueryResultFlags kFlags = VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT;
        ANGLE_VK_TRY(contextVk, getResultImpl(contextVk, kFlags, resultOut));
    }
    else
    {
        *resultOut = 0;
    }
    return angle::Result::Continue;
}

VkResult QueryHelper::getResultImpl(ContextVk *contextVk,
                                    const VkQueryResultFlags flags,
                                    QueryResult *resultOut)
{
    std::array<uint64_t, 2 * gl::IMPLEMENTATION_ANGLE_MULTIVIEW_MAX_VIEWS> results;

    VkDevice device = contextVk->getDevice();
    VkResult result = getQueryPool().getResults(device, mQuery, mQueryCount, sizeof(results),
                                                results.data(), sizeof(uint64_t), flags);

    if (result == VK_SUCCESS)
    {
        resultOut->setResults(results.data(), mQueryCount);
    }

    return result;
}

// DynamicSemaphorePool implementation
DynamicSemaphorePool::DynamicSemaphorePool() = default;

DynamicSemaphorePool::~DynamicSemaphorePool() = default;

angle::Result DynamicSemaphorePool::init(ContextVk *contextVk, uint32_t poolSize)
{
    ANGLE_TRY(initEntryPool(contextVk, poolSize));
    ANGLE_TRY(allocateNewPool(contextVk));
    return angle::Result::Continue;
}

void DynamicSemaphorePool::destroy(VkDevice device)
{
    for (auto &semaphorePool : mPools)
    {
        for (Semaphore &semaphore : semaphorePool)
        {
            semaphore.destroy(device);
        }
    }

    destroyEntryPool();
}

angle::Result DynamicSemaphorePool::allocateSemaphore(ContextVk *contextVk,
                                                      SemaphoreHelper *semaphoreOut)
{
    ASSERT(!semaphoreOut->getSemaphore());

    if (mCurrentFreeEntry >= mPoolSize)
    {
        // No more queries left in this pool, create another one.
        ANGLE_TRY(allocateNewPool(contextVk));
    }

    semaphoreOut->init(mCurrentPool, &mPools[mCurrentPool][mCurrentFreeEntry++]);

    return angle::Result::Continue;
}

void DynamicSemaphorePool::freeSemaphore(ContextVk *contextVk, SemaphoreHelper *semaphore)
{
    if (semaphore->getSemaphore())
    {
        onEntryFreed(contextVk, semaphore->getSemaphorePoolIndex());
        semaphore->deinit();
    }
}

angle::Result DynamicSemaphorePool::allocateNewPool(ContextVk *contextVk)
{
    if (findFreeEntryPool(contextVk))
    {
        return angle::Result::Continue;
    }

    std::vector<Semaphore> newPool(mPoolSize);

    for (Semaphore &semaphore : newPool)
    {
        ANGLE_VK_TRY(contextVk, semaphore.init(contextVk->getDevice()));
    }

    // This code is safe as long as the growth of the outer vector in vector<vector<T>> is done by
    // moving the inner vectors, making sure references to the inner vector remain intact.
    Semaphore *assertMove = mPools.size() > 0 ? mPools[0].data() : nullptr;

    ANGLE_TRY(allocateNewEntryPool(contextVk, std::move(newPool)));

    ASSERT(assertMove == nullptr || assertMove == mPools[0].data());

    return angle::Result::Continue;
}

// SemaphoreHelper implementation
SemaphoreHelper::SemaphoreHelper() : mSemaphorePoolIndex(0), mSemaphore(0) {}

SemaphoreHelper::~SemaphoreHelper() {}

SemaphoreHelper::SemaphoreHelper(SemaphoreHelper &&other)
    : mSemaphorePoolIndex(other.mSemaphorePoolIndex), mSemaphore(other.mSemaphore)
{
    other.mSemaphore = nullptr;
}

SemaphoreHelper &SemaphoreHelper::operator=(SemaphoreHelper &&other)
{
    std::swap(mSemaphorePoolIndex, other.mSemaphorePoolIndex);
    std::swap(mSemaphore, other.mSemaphore);
    return *this;
}

void SemaphoreHelper::init(const size_t semaphorePoolIndex, const Semaphore *semaphore)
{
    mSemaphorePoolIndex = semaphorePoolIndex;
    mSemaphore          = semaphore;
}

void SemaphoreHelper::deinit()
{
    mSemaphorePoolIndex = 0;
    mSemaphore          = nullptr;
}

// LineLoopHelper implementation.
LineLoopHelper::LineLoopHelper(RendererVk *renderer)
{
    // We need to use an alignment of the maximum size we're going to allocate, which is
    // VK_INDEX_TYPE_UINT32. When we switch from a drawElement to a drawArray call, the allocations
    // can vary in size. According to the Vulkan spec, when calling vkCmdBindIndexBuffer: 'The
    // sum of offset and the address of the range of VkDeviceMemory object that is backing buffer,
    // must be a multiple of the type indicated by indexType'.
    mDynamicIndexBuffer.init(renderer, kLineLoopDynamicBufferUsage, sizeof(uint32_t),
                             kLineLoopDynamicBufferInitialSize, true,
                             DynamicBufferPolicy::OneShotUse);
    mDynamicIndirectBuffer.init(renderer, kLineLoopDynamicIndirectBufferUsage, sizeof(uint32_t),
                                kLineLoopDynamicIndirectBufferInitialSize, true,
                                DynamicBufferPolicy::OneShotUse);
}

LineLoopHelper::~LineLoopHelper() = default;

angle::Result LineLoopHelper::getIndexBufferForDrawArrays(ContextVk *contextVk,
                                                          uint32_t clampedVertexCount,
                                                          GLint firstVertex,
                                                          BufferHelper **bufferOut,
                                                          VkDeviceSize *offsetOut)
{
    uint32_t *indices    = nullptr;
    size_t allocateBytes = sizeof(uint32_t) * (static_cast<size_t>(clampedVertexCount) + 1);

    mDynamicIndexBuffer.releaseInFlightBuffers(contextVk);
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), nullptr,
                                           offsetOut, nullptr));
    *bufferOut = mDynamicIndexBuffer.getCurrentBuffer();

    // Note: there could be an overflow in this addition.
    uint32_t unsignedFirstVertex = static_cast<uint32_t>(firstVertex);
    uint32_t vertexCount         = (clampedVertexCount + unsignedFirstVertex);
    for (uint32_t vertexIndex = unsignedFirstVertex; vertexIndex < vertexCount; vertexIndex++)
    {
        *indices++ = vertexIndex;
    }
    *indices = unsignedFirstVertex;

    // Since we are not using the VK_MEMORY_PROPERTY_HOST_COHERENT_BIT flag when creating the
    // device memory in the StreamingBuffer, we always need to make sure we flush it after
    // writing.
    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));

    return angle::Result::Continue;
}

angle::Result LineLoopHelper::getIndexBufferForElementArrayBuffer(ContextVk *contextVk,
                                                                  BufferVk *elementArrayBufferVk,
                                                                  gl::DrawElementsType glIndexType,
                                                                  int indexCount,
                                                                  intptr_t elementArrayOffset,
                                                                  BufferHelper **bufferOut,
                                                                  VkDeviceSize *bufferOffsetOut,
                                                                  uint32_t *indexCountOut)
{
    if (glIndexType == gl::DrawElementsType::UnsignedByte ||
        contextVk->getState().isPrimitiveRestartEnabled())
    {
        ANGLE_TRACE_EVENT0("gpu.angle", "LineLoopHelper::getIndexBufferForElementArrayBuffer");

        void *srcDataMapping = nullptr;
        ANGLE_TRY(elementArrayBufferVk->mapImpl(contextVk, &srcDataMapping));
        ANGLE_TRY(streamIndices(contextVk, glIndexType, indexCount,
                                static_cast<const uint8_t *>(srcDataMapping) + elementArrayOffset,
                                bufferOut, bufferOffsetOut, indexCountOut));
        ANGLE_TRY(elementArrayBufferVk->unmapImpl(contextVk));
        return angle::Result::Continue;
    }

    *indexCountOut = indexCount + 1;

    uint32_t *indices    = nullptr;
    size_t unitSize      = contextVk->getVkIndexTypeSize(glIndexType);
    size_t allocateBytes = unitSize * (indexCount + 1) + 1;

    mDynamicIndexBuffer.releaseInFlightBuffers(contextVk);
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), nullptr,
                                           bufferOffsetOut, nullptr));
    *bufferOut = mDynamicIndexBuffer.getCurrentBuffer();

    VkDeviceSize sourceBufferOffset = 0;
    BufferHelper *sourceBuffer = &elementArrayBufferVk->getBufferAndOffset(&sourceBufferOffset);

    VkDeviceSize sourceOffset = static_cast<VkDeviceSize>(elementArrayOffset) + sourceBufferOffset;
    uint64_t unitCount        = static_cast<VkDeviceSize>(indexCount);
    angle::FixedVector<VkBufferCopy, 3> copies = {
        {sourceOffset, *bufferOffsetOut, unitCount * unitSize},
        {sourceOffset, *bufferOffsetOut + unitCount * unitSize, unitSize},
    };
    if (contextVk->getRenderer()->getFeatures().extraCopyBufferRegion.enabled)
        copies.push_back({sourceOffset, *bufferOffsetOut + (unitCount + 1) * unitSize, 1});

    vk::CommandBufferAccess access;
    access.onBufferTransferWrite(*bufferOut);
    access.onBufferTransferRead(sourceBuffer);

    vk::CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    commandBuffer->copyBuffer(sourceBuffer->getBuffer(), (*bufferOut)->getBuffer(),
                              static_cast<uint32_t>(copies.size()), copies.data());

    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));
    return angle::Result::Continue;
}

angle::Result LineLoopHelper::streamIndices(ContextVk *contextVk,
                                            gl::DrawElementsType glIndexType,
                                            GLsizei indexCount,
                                            const uint8_t *srcPtr,
                                            BufferHelper **bufferOut,
                                            VkDeviceSize *bufferOffsetOut,
                                            uint32_t *indexCountOut)
{
    size_t unitSize = contextVk->getVkIndexTypeSize(glIndexType);

    uint8_t *indices = nullptr;

    uint32_t numOutIndices = indexCount + 1;
    if (contextVk->getState().isPrimitiveRestartEnabled())
    {
        numOutIndices = GetLineLoopWithRestartIndexCount(glIndexType, indexCount, srcPtr);
    }
    *indexCountOut       = numOutIndices;
    size_t allocateBytes = unitSize * numOutIndices;
    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes,
                                           reinterpret_cast<uint8_t **>(&indices), nullptr,
                                           bufferOffsetOut, nullptr));
    *bufferOut = mDynamicIndexBuffer.getCurrentBuffer();

    if (contextVk->getState().isPrimitiveRestartEnabled())
    {
        HandlePrimitiveRestart(contextVk, glIndexType, indexCount, srcPtr, indices);
    }
    else
    {
        if (contextVk->shouldConvertUint8VkIndexType(glIndexType))
        {
            // If vulkan doesn't support uint8 index types, we need to emulate it.
            VkIndexType indexType = contextVk->getVkIndexType(glIndexType);
            ASSERT(indexType == VK_INDEX_TYPE_UINT16);
            uint16_t *indicesDst = reinterpret_cast<uint16_t *>(indices);
            for (int i = 0; i < indexCount; i++)
            {
                indicesDst[i] = srcPtr[i];
            }

            indicesDst[indexCount] = srcPtr[0];
        }
        else
        {
            memcpy(indices, srcPtr, unitSize * indexCount);
            memcpy(indices + unitSize * indexCount, srcPtr, unitSize);
        }
    }

    ANGLE_TRY(mDynamicIndexBuffer.flush(contextVk));
    return angle::Result::Continue;
}

angle::Result LineLoopHelper::streamIndicesIndirect(ContextVk *contextVk,
                                                    gl::DrawElementsType glIndexType,
                                                    BufferHelper *indexBuffer,
                                                    VkDeviceSize indexBufferOffset,
                                                    BufferHelper *indirectBuffer,
                                                    VkDeviceSize indirectBufferOffset,
                                                    BufferHelper **indexBufferOut,
                                                    VkDeviceSize *indexBufferOffsetOut,
                                                    BufferHelper **indirectBufferOut,
                                                    VkDeviceSize *indirectBufferOffsetOut)
{
    size_t unitSize      = contextVk->getVkIndexTypeSize(glIndexType);
    size_t allocateBytes = static_cast<size_t>(indexBuffer->getSize() + unitSize);

    if (contextVk->getState().isPrimitiveRestartEnabled())
    {
        // If primitive restart, new index buffer is 135% the size of the original index buffer. The
        // smallest lineloop with primitive restart is 3 indices (point 1, point 2 and restart
        // value) when converted to linelist becomes 4 vertices. Expansion of 4/3. Any larger
        // lineloops would have less overhead and require less extra space. Any incomplete
        // primitives can be dropped or left incomplete and thus not increase the size of the
        // destination index buffer. Since we don't know the number of indices being used we'll use
        // the size of the index buffer as allocated as the index count.
        size_t numInputIndices    = static_cast<size_t>(indexBuffer->getSize() / unitSize);
        size_t numNewInputIndices = ((numInputIndices * 4) / 3) + 1;
        allocateBytes             = static_cast<size_t>(numNewInputIndices * unitSize);
    }

    mDynamicIndexBuffer.releaseInFlightBuffers(contextVk);
    mDynamicIndirectBuffer.releaseInFlightBuffers(contextVk);

    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes, nullptr, nullptr,
                                           indexBufferOffsetOut, nullptr));
    *indexBufferOut = mDynamicIndexBuffer.getCurrentBuffer();

    ANGLE_TRY(mDynamicIndirectBuffer.allocate(contextVk, sizeof(VkDrawIndexedIndirectCommand),
                                              nullptr, nullptr, indirectBufferOffsetOut, nullptr));
    *indirectBufferOut = mDynamicIndirectBuffer.getCurrentBuffer();

    BufferHelper *destIndexBuffer    = mDynamicIndexBuffer.getCurrentBuffer();
    BufferHelper *destIndirectBuffer = mDynamicIndirectBuffer.getCurrentBuffer();

    // Copy relevant section of the source into destination at allocated offset.  Note that the
    // offset returned by allocate() above is in bytes. As is the indices offset pointer.
    UtilsVk::ConvertLineLoopIndexIndirectParameters params = {};
    params.indirectBufferOffset    = static_cast<uint32_t>(indirectBufferOffset);
    params.dstIndirectBufferOffset = static_cast<uint32_t>(*indirectBufferOffsetOut);
    params.srcIndexBufferOffset    = static_cast<uint32_t>(indexBufferOffset);
    params.dstIndexBufferOffset    = static_cast<uint32_t>(*indexBufferOffsetOut);
    params.indicesBitsWidth        = static_cast<uint32_t>(unitSize * 8);

    ANGLE_TRY(contextVk->getUtils().convertLineLoopIndexIndirectBuffer(
        contextVk, indirectBuffer, destIndirectBuffer, destIndexBuffer, indexBuffer, params));

    return angle::Result::Continue;
}

angle::Result LineLoopHelper::streamArrayIndirect(ContextVk *contextVk,
                                                  size_t vertexCount,
                                                  BufferHelper *arrayIndirectBuffer,
                                                  VkDeviceSize arrayIndirectBufferOffset,
                                                  BufferHelper **indexBufferOut,
                                                  VkDeviceSize *indexBufferOffsetOut,
                                                  BufferHelper **indexIndirectBufferOut,
                                                  VkDeviceSize *indexIndirectBufferOffsetOut)
{
    auto unitSize        = sizeof(uint32_t);
    size_t allocateBytes = static_cast<size_t>((vertexCount + 1) * unitSize);

    mDynamicIndexBuffer.releaseInFlightBuffers(contextVk);
    mDynamicIndirectBuffer.releaseInFlightBuffers(contextVk);

    ANGLE_TRY(mDynamicIndexBuffer.allocate(contextVk, allocateBytes, nullptr, nullptr,
                                           indexBufferOffsetOut, nullptr));
    *indexBufferOut = mDynamicIndexBuffer.getCurrentBuffer();

    ANGLE_TRY(mDynamicIndirectBuffer.allocate(contextVk, sizeof(VkDrawIndexedIndirectCommand),
                                              nullptr, nullptr, indexIndirectBufferOffsetOut,
                                              nullptr));
    *indexIndirectBufferOut = mDynamicIndirectBuffer.getCurrentBuffer();

    BufferHelper *destIndexBuffer    = mDynamicIndexBuffer.getCurrentBuffer();
    BufferHelper *destIndirectBuffer = mDynamicIndirectBuffer.getCurrentBuffer();

    // Copy relevant section of the source into destination at allocated offset.  Note that the
    // offset returned by allocate() above is in bytes. As is the indices offset pointer.
    UtilsVk::ConvertLineLoopArrayIndirectParameters params = {};
    params.indirectBufferOffset    = static_cast<uint32_t>(arrayIndirectBufferOffset);
    params.dstIndirectBufferOffset = static_cast<uint32_t>(*indexIndirectBufferOffsetOut);
    params.dstIndexBufferOffset    = static_cast<uint32_t>(*indexBufferOffsetOut);

    ANGLE_TRY(contextVk->getUtils().convertLineLoopArrayIndirectBuffer(
        contextVk, arrayIndirectBuffer, destIndirectBuffer, destIndexBuffer, params));

    return angle::Result::Continue;
}

void LineLoopHelper::release(ContextVk *contextVk)
{
    mDynamicIndexBuffer.release(contextVk->getRenderer());
    mDynamicIndirectBuffer.release(contextVk->getRenderer());
}

void LineLoopHelper::destroy(RendererVk *renderer)
{
    mDynamicIndexBuffer.destroy(renderer);
    mDynamicIndirectBuffer.destroy(renderer);
}

// static
void LineLoopHelper::Draw(uint32_t count, uint32_t baseVertex, CommandBuffer *commandBuffer)
{
    // Our first index is always 0 because that's how we set it up in createIndexBuffer*.
    commandBuffer->drawIndexedBaseVertex(count, baseVertex);
}

PipelineStage GetPipelineStage(gl::ShaderType stage)
{
    return kPipelineStageShaderMap[stage];
}

// PipelineBarrier implementation.
void PipelineBarrier::addDiagnosticsString(std::ostringstream &out) const
{
    if (mMemoryBarrierSrcAccess != 0 || mMemoryBarrierDstAccess != 0)
    {
        out << "Src: 0x" << std::hex << mMemoryBarrierSrcAccess << " &rarr; Dst: 0x" << std::hex
            << mMemoryBarrierDstAccess << std::endl;
    }
}

// BufferHelper implementation.
BufferHelper::BufferHelper()
    : mMemoryPropertyFlags{},
      mSize(0),
      mCurrentQueueFamilyIndex(std::numeric_limits<uint32_t>::max()),
      mCurrentWriteAccess(0),
      mCurrentReadAccess(0),
      mCurrentWriteStages(0),
      mCurrentReadStages(0),
      mSerial()
{}

BufferMemory::BufferMemory() : mClientBuffer(nullptr), mMappedMemory(nullptr) {}

BufferMemory::~BufferMemory() = default;

angle::Result BufferMemory::initExternal(GLeglClientBufferEXT clientBuffer)
{
    ASSERT(clientBuffer != nullptr);
    mClientBuffer = clientBuffer;
    return angle::Result::Continue;
}

angle::Result BufferMemory::init()
{
    ASSERT(mClientBuffer == nullptr);
    return angle::Result::Continue;
}

void BufferMemory::unmap(RendererVk *renderer)
{
    if (mMappedMemory != nullptr)
    {
        if (isExternalBuffer())
        {
            mExternalMemory.unmap(renderer->getDevice());
        }
        else
        {
            mAllocation.unmap(renderer->getAllocator());
        }

        mMappedMemory = nullptr;
    }
}

void BufferMemory::destroy(RendererVk *renderer)
{
    if (isExternalBuffer())
    {
        mExternalMemory.destroy(renderer->getDevice());
        ReleaseAndroidExternalMemory(renderer, mClientBuffer);
    }
    else
    {
        mAllocation.destroy(renderer->getAllocator());
    }
}

void BufferMemory::flush(RendererVk *renderer,
                         VkMemoryMapFlags memoryPropertyFlags,
                         VkDeviceSize offset,
                         VkDeviceSize size)
{
    if (isExternalBuffer())
    {
        // if the memory type is not host coherent, we perform an explicit flush
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) == 0)
        {
            VkMappedMemoryRange mappedRange = {};
            mappedRange.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            mappedRange.memory              = mExternalMemory.getHandle();
            mappedRange.offset              = offset;
            mappedRange.size                = size;
            mExternalMemory.flush(renderer->getDevice(), mappedRange);
        }
    }
    else
    {
        mAllocation.flush(renderer->getAllocator(), offset, size);
    }
}

void BufferMemory::invalidate(RendererVk *renderer,
                              VkMemoryMapFlags memoryPropertyFlags,
                              VkDeviceSize offset,
                              VkDeviceSize size)
{
    if (isExternalBuffer())
    {
        // if the memory type is not device coherent, we perform an explicit invalidate
        if ((memoryPropertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD) == 0)
        {
            VkMappedMemoryRange memoryRanges = {};
            memoryRanges.sType               = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
            memoryRanges.memory              = mExternalMemory.getHandle();
            memoryRanges.offset              = offset;
            memoryRanges.size                = size;
            mExternalMemory.invalidate(renderer->getDevice(), memoryRanges);
        }
    }
    else
    {
        mAllocation.invalidate(renderer->getAllocator(), offset, size);
    }
}

angle::Result BufferMemory::mapImpl(ContextVk *contextVk, VkDeviceSize size)
{
    if (isExternalBuffer())
    {
        ANGLE_VK_TRY(contextVk, mExternalMemory.map(contextVk->getRenderer()->getDevice(), 0, size,
                                                    0, &mMappedMemory));
    }
    else
    {
        ANGLE_VK_TRY(contextVk,
                     mAllocation.map(contextVk->getRenderer()->getAllocator(), &mMappedMemory));
    }

    return angle::Result::Continue;
}

BufferHelper::~BufferHelper() = default;

angle::Result BufferHelper::init(ContextVk *contextVk,
                                 const VkBufferCreateInfo &requestedCreateInfo,
                                 VkMemoryPropertyFlags memoryPropertyFlags)
{
    RendererVk *renderer = contextVk->getRenderer();

    mSerial = renderer->getResourceSerialFactory().generateBufferSerial();
    mSize   = requestedCreateInfo.size;

    VkBufferCreateInfo modifiedCreateInfo;
    const VkBufferCreateInfo *createInfo = &requestedCreateInfo;

    if (renderer->getFeatures().padBuffersToMaxVertexAttribStride.enabled)
    {
        const VkDeviceSize maxVertexAttribStride = renderer->getMaxVertexAttribStride();
        ASSERT(maxVertexAttribStride);
        modifiedCreateInfo = requestedCreateInfo;
        modifiedCreateInfo.size += maxVertexAttribStride;
        createInfo = &modifiedCreateInfo;
    }

    VkMemoryPropertyFlags requiredFlags =
        (memoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    VkMemoryPropertyFlags preferredFlags =
        (memoryPropertyFlags & (~VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT));

    const Allocator &allocator = renderer->getAllocator();
    bool persistentlyMapped    = renderer->getFeatures().persistentlyMappedBuffers.enabled;

    // Check that the allocation is not too large.
    uint32_t memoryTypeIndex = 0;
    ANGLE_VK_TRY(contextVk, allocator.findMemoryTypeIndexForBufferInfo(
                                *createInfo, requiredFlags, preferredFlags, persistentlyMapped,
                                &memoryTypeIndex));

    VkDeviceSize heapSize =
        renderer->getMemoryProperties().getHeapSizeForMemoryType(memoryTypeIndex);

    ANGLE_VK_CHECK(contextVk, createInfo->size <= heapSize, VK_ERROR_OUT_OF_DEVICE_MEMORY);

    ANGLE_VK_TRY(contextVk, allocator.createBuffer(*createInfo, requiredFlags, preferredFlags,
                                                   persistentlyMapped, &memoryTypeIndex, &mBuffer,
                                                   mMemory.getMemoryObject()));
    allocator.getMemoryTypeProperties(memoryTypeIndex, &mMemoryPropertyFlags);
    mCurrentQueueFamilyIndex = renderer->getQueueFamilyIndex();

    if (renderer->getFeatures().allocateNonZeroMemory.enabled)
    {
        // This memory can't be mapped, so the buffer must be marked as a transfer destination so we
        // can use a staging resource to initialize it to a non-zero value. If the memory is
        // mappable we do the initialization in AllocateBufferMemory.
        if ((mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0 &&
            (requestedCreateInfo.usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0)
        {
            ANGLE_TRY(initializeNonZeroMemory(contextVk, createInfo->size));
        }
        else if ((mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0)
        {
            // Can map the memory.
            // Pick an arbitrary value to initialize non-zero memory for sanitization.
            constexpr int kNonZeroInitValue = 55;
            ANGLE_TRY(InitMappableAllocation(contextVk, allocator, mMemory.getMemoryObject(), mSize,
                                             kNonZeroInitValue, mMemoryPropertyFlags));
        }
    }

    ANGLE_TRY(mMemory.init());

    return angle::Result::Continue;
}

angle::Result BufferHelper::initExternal(ContextVk *contextVk,
                                         VkMemoryPropertyFlags memoryProperties,
                                         const VkBufferCreateInfo &requestedCreateInfo,
                                         GLeglClientBufferEXT clientBuffer)
{
    ASSERT(IsAndroid());

    RendererVk *renderer = contextVk->getRenderer();

    mSerial = renderer->getResourceSerialFactory().generateBufferSerial();
    mSize   = requestedCreateInfo.size;

    VkBufferCreateInfo modifiedCreateInfo             = requestedCreateInfo;
    VkExternalMemoryBufferCreateInfo externCreateInfo = {};
    externCreateInfo.sType = VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO;
    externCreateInfo.handleTypes =
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID;
    externCreateInfo.pNext   = nullptr;
    modifiedCreateInfo.pNext = &externCreateInfo;

    ANGLE_VK_TRY(contextVk, mBuffer.init(renderer->getDevice(), modifiedCreateInfo));

    ANGLE_TRY(InitAndroidExternalMemory(contextVk, clientBuffer, memoryProperties, &mBuffer,
                                        &mMemoryPropertyFlags, mMemory.getExternalMemoryObject()));

    ANGLE_TRY(mMemory.initExternal(clientBuffer));

    mCurrentQueueFamilyIndex = renderer->getQueueFamilyIndex();

    return angle::Result::Continue;
}

angle::Result BufferHelper::initializeNonZeroMemory(Context *context, VkDeviceSize size)
{
    // Staging buffer memory is non-zero-initialized in 'init'.
    StagingBuffer stagingBuffer;
    ANGLE_TRY(stagingBuffer.init(context, size, StagingUsage::Both));

    RendererVk *renderer = context->getRenderer();

    PrimaryCommandBuffer commandBuffer;
    ANGLE_TRY(renderer->getCommandBufferOneOff(context, false, &commandBuffer));

    // Queue a DMA copy.
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset    = 0;
    copyRegion.dstOffset    = 0;
    copyRegion.size         = size;

    commandBuffer.copyBuffer(stagingBuffer.getBuffer(), mBuffer, 1, &copyRegion);

    ANGLE_VK_TRY(context, commandBuffer.end());

    Serial serial;
    ANGLE_TRY(renderer->queueSubmitOneOff(context, std::move(commandBuffer), false,
                                          egl::ContextPriority::Medium, nullptr,
                                          vk::SubmitPolicy::AllowDeferred, &serial));

    stagingBuffer.collectGarbage(renderer, serial);
    mUse.updateSerialOneOff(serial);

    return angle::Result::Continue;
}

void BufferHelper::destroy(RendererVk *renderer)
{
    VkDevice device = renderer->getDevice();
    unmap(renderer);
    mSize = 0;

    mBuffer.destroy(device);
    mMemory.destroy(renderer);
}

void BufferHelper::release(RendererVk *renderer)
{
    unmap(renderer);
    mSize = 0;

    renderer->collectGarbageAndReinit(&mUse, &mBuffer, mMemory.getExternalMemoryObject(),
                                      mMemory.getMemoryObject());
}

angle::Result BufferHelper::copyFromBuffer(ContextVk *contextVk,
                                           BufferHelper *srcBuffer,
                                           uint32_t regionCount,
                                           const VkBufferCopy *copyRegions)
{
    // Check for self-dependency.
    vk::CommandBufferAccess access;
    if (srcBuffer->getBufferSerial() == getBufferSerial())
    {
        access.onBufferSelfCopy(this);
    }
    else
    {
        access.onBufferTransferRead(srcBuffer);
        access.onBufferTransferWrite(this);
    }

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    commandBuffer->copyBuffer(srcBuffer->getBuffer(), mBuffer, regionCount, copyRegions);

    return angle::Result::Continue;
}

void BufferHelper::unmap(RendererVk *renderer)
{
    mMemory.unmap(renderer);
}

angle::Result BufferHelper::flush(RendererVk *renderer, VkDeviceSize offset, VkDeviceSize size)
{
    bool hostVisible  = mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    bool hostCoherent = mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (hostVisible && !hostCoherent)
    {
        mMemory.flush(renderer, mMemoryPropertyFlags, offset, size);
    }
    return angle::Result::Continue;
}

angle::Result BufferHelper::invalidate(RendererVk *renderer, VkDeviceSize offset, VkDeviceSize size)
{
    bool hostVisible  = mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    bool hostCoherent = mMemoryPropertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    if (hostVisible && !hostCoherent)
    {
        mMemory.invalidate(renderer, mMemoryPropertyFlags, offset, size);
    }
    return angle::Result::Continue;
}

void BufferHelper::changeQueue(uint32_t newQueueFamilyIndex, CommandBuffer *commandBuffer)
{
    VkBufferMemoryBarrier bufferMemoryBarrier = {};
    bufferMemoryBarrier.sType                 = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    bufferMemoryBarrier.srcAccessMask         = 0;
    bufferMemoryBarrier.dstAccessMask         = 0;
    bufferMemoryBarrier.srcQueueFamilyIndex   = mCurrentQueueFamilyIndex;
    bufferMemoryBarrier.dstQueueFamilyIndex   = newQueueFamilyIndex;
    bufferMemoryBarrier.buffer                = mBuffer.getHandle();
    bufferMemoryBarrier.offset                = 0;
    bufferMemoryBarrier.size                  = VK_WHOLE_SIZE;

    commandBuffer->bufferBarrier(VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                 VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, &bufferMemoryBarrier);

    mCurrentQueueFamilyIndex = newQueueFamilyIndex;
}

void BufferHelper::acquireFromExternal(ContextVk *contextVk,
                                       uint32_t externalQueueFamilyIndex,
                                       uint32_t rendererQueueFamilyIndex,
                                       CommandBuffer *commandBuffer)
{
    mCurrentQueueFamilyIndex = externalQueueFamilyIndex;

    changeQueue(rendererQueueFamilyIndex, commandBuffer);
}

void BufferHelper::releaseToExternal(ContextVk *contextVk,
                                     uint32_t rendererQueueFamilyIndex,
                                     uint32_t externalQueueFamilyIndex,
                                     CommandBuffer *commandBuffer)
{
    ASSERT(mCurrentQueueFamilyIndex == rendererQueueFamilyIndex);

    changeQueue(externalQueueFamilyIndex, commandBuffer);
}

bool BufferHelper::isReleasedToExternal() const
{
#if !defined(ANGLE_PLATFORM_MACOS) && !defined(ANGLE_PLATFORM_ANDROID)
    return IsExternalQueueFamily(mCurrentQueueFamilyIndex);
#else
    // TODO(anglebug.com/4635): Implement external memory barriers on Mac/Android.
    return false;
#endif
}

bool BufferHelper::recordReadBarrier(VkAccessFlags readAccessType,
                                     VkPipelineStageFlags readStage,
                                     PipelineBarrier *barrier)
{
    bool barrierModified = false;
    // If there was a prior write and we are making a read that is either a new access type or from
    // a new stage, we need a barrier
    if (mCurrentWriteAccess != 0 && (((mCurrentReadAccess & readAccessType) != readAccessType) ||
                                     ((mCurrentReadStages & readStage) != readStage)))
    {
        barrier->mergeMemoryBarrier(mCurrentWriteStages, readStage, mCurrentWriteAccess,
                                    readAccessType);
        barrierModified = true;
    }

    // Accumulate new read usage.
    mCurrentReadAccess |= readAccessType;
    mCurrentReadStages |= readStage;
    return barrierModified;
}

bool BufferHelper::recordWriteBarrier(VkAccessFlags writeAccessType,
                                      VkPipelineStageFlags writeStage,
                                      PipelineBarrier *barrier)
{
    bool barrierModified = false;
    // We don't need to check mCurrentReadStages here since if it is not zero, mCurrentReadAccess
    // must not be zero as well. stage is finer grain than accessType.
    ASSERT((!mCurrentReadStages && !mCurrentReadAccess) ||
           (mCurrentReadStages && mCurrentReadAccess));
    if (mCurrentReadAccess != 0 || mCurrentWriteAccess != 0)
    {
        barrier->mergeMemoryBarrier(mCurrentWriteStages | mCurrentReadStages, writeStage,
                                    mCurrentWriteAccess, writeAccessType);
        barrierModified = true;
    }

    // Reset usages on the new write.
    mCurrentWriteAccess = writeAccessType;
    mCurrentReadAccess  = 0;
    mCurrentWriteStages = writeStage;
    mCurrentReadStages  = 0;
    return barrierModified;
}

// ImageHelper implementation.
ImageHelper::ImageHelper()
{
    resetCachedProperties();
}

ImageHelper::ImageHelper(ImageHelper &&other)
    : Resource(std::move(other)),
      mImage(std::move(other.mImage)),
      mDeviceMemory(std::move(other.mDeviceMemory)),
      mImageType(other.mImageType),
      mTilingMode(other.mTilingMode),
      mCreateFlags(other.mCreateFlags),
      mUsage(other.mUsage),
      mExtents(other.mExtents),
      mRotatedAspectRatio(other.mRotatedAspectRatio),
      mFormat(other.mFormat),
      mSamples(other.mSamples),
      mImageSerial(other.mImageSerial),
      mCurrentLayout(other.mCurrentLayout),
      mCurrentQueueFamilyIndex(other.mCurrentQueueFamilyIndex),
      mLastNonShaderReadOnlyLayout(other.mLastNonShaderReadOnlyLayout),
      mCurrentShaderReadStageMask(other.mCurrentShaderReadStageMask),
      mYuvConversionSampler(std::move(other.mYuvConversionSampler)),
      mExternalFormat(other.mExternalFormat),
      mFirstAllocatedLevel(other.mFirstAllocatedLevel),
      mLayerCount(other.mLayerCount),
      mLevelCount(other.mLevelCount),
      mStagingBuffer(std::move(other.mStagingBuffer)),
      mSubresourceUpdates(std::move(other.mSubresourceUpdates)),
      mCurrentSingleClearValue(std::move(other.mCurrentSingleClearValue)),
      mContentDefined(std::move(other.mContentDefined)),
      mStencilContentDefined(std::move(other.mStencilContentDefined))
{
    ASSERT(this != &other);
    other.resetCachedProperties();
}

ImageHelper::~ImageHelper()
{
    ASSERT(!valid());
}

void ImageHelper::resetCachedProperties()
{
    mImageType                   = VK_IMAGE_TYPE_2D;
    mTilingMode                  = VK_IMAGE_TILING_OPTIMAL;
    mCreateFlags                 = kVkImageCreateFlagsNone;
    mUsage                       = 0;
    mExtents                     = {};
    mRotatedAspectRatio          = false;
    mFormat                      = nullptr;
    mSamples                     = 1;
    mImageSerial                 = kInvalidImageSerial;
    mCurrentLayout               = ImageLayout::Undefined;
    mCurrentQueueFamilyIndex     = std::numeric_limits<uint32_t>::max();
    mLastNonShaderReadOnlyLayout = ImageLayout::Undefined;
    mCurrentShaderReadStageMask  = 0;
    mFirstAllocatedLevel         = gl::LevelIndex(0);
    mLayerCount                  = 0;
    mLevelCount                  = 0;
    mExternalFormat              = 0;
    mCurrentSingleClearValue.reset();
    mRenderPassUsageFlags.reset();

    setEntireContentUndefined();
}

void ImageHelper::setEntireContentDefined()
{
    for (LevelContentDefinedMask &levelContentDefined : mContentDefined)
    {
        levelContentDefined.set();
    }
    for (LevelContentDefinedMask &levelContentDefined : mStencilContentDefined)
    {
        levelContentDefined.set();
    }
}

void ImageHelper::setEntireContentUndefined()
{
    for (LevelContentDefinedMask &levelContentDefined : mContentDefined)
    {
        levelContentDefined.reset();
    }
    for (LevelContentDefinedMask &levelContentDefined : mStencilContentDefined)
    {
        levelContentDefined.reset();
    }
}

void ImageHelper::setContentDefined(LevelIndex levelStart,
                                    uint32_t levelCount,
                                    uint32_t layerStart,
                                    uint32_t layerCount,
                                    VkImageAspectFlags aspectFlags)
{
    // Mark the range as defined.  Layers above 8 are discarded, and are always assumed to have
    // defined contents.
    if (layerStart >= kMaxContentDefinedLayerCount)
    {
        return;
    }

    uint8_t layerRangeBits =
        GetContentDefinedLayerRangeBits(layerStart, layerCount, kMaxContentDefinedLayerCount);

    for (uint32_t levelOffset = 0; levelOffset < levelCount; ++levelOffset)
    {
        LevelIndex level = levelStart + levelOffset;

        if ((aspectFlags & ~VK_IMAGE_ASPECT_STENCIL_BIT) != 0)
        {
            getLevelContentDefined(level) |= layerRangeBits;
        }
        if ((aspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT) != 0)
        {
            getLevelStencilContentDefined(level) |= layerRangeBits;
        }
    }
}

ImageHelper::LevelContentDefinedMask &ImageHelper::getLevelContentDefined(LevelIndex level)
{
    return mContentDefined[level.get()];
}

ImageHelper::LevelContentDefinedMask &ImageHelper::getLevelStencilContentDefined(LevelIndex level)
{
    return mStencilContentDefined[level.get()];
}

const ImageHelper::LevelContentDefinedMask &ImageHelper::getLevelContentDefined(
    LevelIndex level) const
{
    return mContentDefined[level.get()];
}

const ImageHelper::LevelContentDefinedMask &ImageHelper::getLevelStencilContentDefined(
    LevelIndex level) const
{
    return mStencilContentDefined[level.get()];
}

void ImageHelper::initStagingBuffer(RendererVk *renderer,
                                    size_t imageCopyBufferAlignment,
                                    VkBufferUsageFlags usageFlags,
                                    size_t initialSize)
{
    mStagingBuffer.init(renderer, usageFlags, imageCopyBufferAlignment, initialSize, true,
                        DynamicBufferPolicy::OneShotUse);
}

angle::Result ImageHelper::init(Context *context,
                                gl::TextureType textureType,
                                const VkExtent3D &extents,
                                const Format &format,
                                GLint samples,
                                VkImageUsageFlags usage,
                                gl::LevelIndex firstLevel,
                                uint32_t mipLevels,
                                uint32_t layerCount,
                                bool isRobustResourceInitEnabled,
                                bool hasProtectedContent)
{
    return initExternal(context, textureType, extents, format, samples, usage,
                        kVkImageCreateFlagsNone, ImageLayout::Undefined, nullptr, firstLevel,
                        mipLevels, layerCount, isRobustResourceInitEnabled, nullptr,
                        hasProtectedContent);
}

angle::Result ImageHelper::initMSAASwapchain(Context *context,
                                             gl::TextureType textureType,
                                             const VkExtent3D &extents,
                                             bool rotatedAspectRatio,
                                             const Format &format,
                                             GLint samples,
                                             VkImageUsageFlags usage,
                                             gl::LevelIndex firstLevel,
                                             uint32_t mipLevels,
                                             uint32_t layerCount,
                                             bool isRobustResourceInitEnabled,
                                             bool hasProtectedContent)
{
    ANGLE_TRY(initExternal(context, textureType, extents, format, samples, usage,
                           kVkImageCreateFlagsNone, ImageLayout::Undefined, nullptr, firstLevel,
                           mipLevels, layerCount, isRobustResourceInitEnabled, nullptr,
                           hasProtectedContent));
    if (rotatedAspectRatio)
    {
        std::swap(mExtents.width, mExtents.height);
    }
    mRotatedAspectRatio = rotatedAspectRatio;
    return angle::Result::Continue;
}

angle::Result ImageHelper::initExternal(Context *context,
                                        gl::TextureType textureType,
                                        const VkExtent3D &extents,
                                        const Format &format,
                                        GLint samples,
                                        VkImageUsageFlags usage,
                                        VkImageCreateFlags additionalCreateFlags,
                                        ImageLayout initialLayout,
                                        const void *externalImageCreateInfo,
                                        gl::LevelIndex firstLevel,
                                        uint32_t mipLevels,
                                        uint32_t layerCount,
                                        bool isRobustResourceInitEnabled,
                                        bool *imageFormatListEnabledOut,
                                        bool hasProtectedContent)
{
    ASSERT(!valid());
    ASSERT(!IsAnySubresourceContentDefined(mContentDefined));
    ASSERT(!IsAnySubresourceContentDefined(mStencilContentDefined));

    mImageType           = gl_vk::GetImageType(textureType);
    mExtents             = extents;
    mRotatedAspectRatio  = false;
    mFormat              = &format;
    mSamples             = std::max(samples, 1);
    mImageSerial         = context->getRenderer()->getResourceSerialFactory().generateImageSerial();
    mFirstAllocatedLevel = firstLevel;
    mLevelCount          = mipLevels;
    mLayerCount          = layerCount;
    mCreateFlags         = GetImageCreateFlags(textureType) | additionalCreateFlags;
    mUsage               = usage;

    // Validate that mLayerCount is compatible with the texture type
    ASSERT(textureType != gl::TextureType::_3D || mLayerCount == 1);
    ASSERT(textureType != gl::TextureType::_2DArray || mExtents.depth == 1);
    ASSERT(textureType != gl::TextureType::External || mLayerCount == 1);
    ASSERT(textureType != gl::TextureType::Rectangle || mLayerCount == 1);
    ASSERT(textureType != gl::TextureType::CubeMap || mLayerCount == gl::kCubeFaceCount);

    // With the introduction of sRGB related GLES extensions any sample/render target could be
    // respecified causing it to be interpreted in a different colorspace. Create the VkImage
    // accordingly.
    bool imageFormatListEnabled                        = false;
    RendererVk *rendererVk                             = context->getRenderer();
    VkImageFormatListCreateInfoKHR imageFormatListInfo = {};
    angle::FormatID imageFormat                        = format.actualImageFormatID;
    angle::FormatID additionalFormat                   = format.actualImageFormat().isSRGB
                                           ? ConvertToLinear(imageFormat)
                                           : ConvertToSRGB(imageFormat);
    constexpr uint32_t kImageListFormatCount = 2;
    VkFormat imageListFormats[kImageListFormatCount];
    imageListFormats[0] = vk::GetVkFormatFromFormatID(imageFormat);
    imageListFormats[1] = vk::GetVkFormatFromFormatID(additionalFormat);

    if (rendererVk->getFeatures().supportsImageFormatList.enabled &&
        rendererVk->haveSameFormatFeatureBits(imageFormat, additionalFormat))
    {
        imageFormatListEnabled = true;

        // Add VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT to VkImage create flag
        mCreateFlags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

        // There is just 1 additional format we might use to create a VkImageView for this
        // VkImage
        imageFormatListInfo.sType           = VK_STRUCTURE_TYPE_IMAGE_FORMAT_LIST_CREATE_INFO_KHR;
        imageFormatListInfo.pNext           = externalImageCreateInfo;
        imageFormatListInfo.viewFormatCount = kImageListFormatCount;
        imageFormatListInfo.pViewFormats    = imageListFormats;
    }

    if (imageFormatListEnabledOut)
    {
        *imageFormatListEnabledOut = imageFormatListEnabled;
    }

    mYuvConversionSampler.reset();
    mExternalFormat = 0;
    if (format.actualImageFormat().isYUV)
    {
        // The Vulkan spec states: If sampler is used and the VkFormat of the image is a
        // multi-planar format, the image must have been created with
        // VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT
        mCreateFlags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT;

        // The Vulkan spec states: The potential format features of the sampler YCBCR conversion
        // must support VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT or
        // VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT
        constexpr VkFormatFeatureFlags kChromaSubSampleFeatureBits =
            VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT |
            VK_FORMAT_FEATURE_MIDPOINT_CHROMA_SAMPLES_BIT;

        VkFormatFeatureFlags supportedChromaSubSampleFeatureBits =
            rendererVk->getImageFormatFeatureBits(format.actualImageFormatID,
                                                  kChromaSubSampleFeatureBits);

        VkChromaLocation supportedLocation = ((supportedChromaSubSampleFeatureBits &
                                               VK_FORMAT_FEATURE_COSITED_CHROMA_SAMPLES_BIT) != 0)
                                                 ? VK_CHROMA_LOCATION_COSITED_EVEN
                                                 : VK_CHROMA_LOCATION_MIDPOINT;

        // Create the VkSamplerYcbcrConversion to associate with image views and samplers
        VkSamplerYcbcrConversionCreateInfo yuvConversionInfo = {};
        yuvConversionInfo.sType         = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO;
        yuvConversionInfo.format        = format.actualImageVkFormat();
        yuvConversionInfo.xChromaOffset = supportedLocation;
        yuvConversionInfo.yChromaOffset = supportedLocation;
        yuvConversionInfo.ycbcrModel    = VK_SAMPLER_YCBCR_MODEL_CONVERSION_YCBCR_601;
        yuvConversionInfo.ycbcrRange    = VK_SAMPLER_YCBCR_RANGE_ITU_NARROW;
        yuvConversionInfo.chromaFilter  = VK_FILTER_NEAREST;

        ANGLE_TRY(rendererVk->getYuvConversionCache().getYuvConversion(
            context, format.actualImageVkFormat(), false, yuvConversionInfo,
            &mYuvConversionSampler));
    }

    if (hasProtectedContent)
    {
        mCreateFlags |= VK_IMAGE_CREATE_PROTECTED_BIT;
    }

    VkImageCreateInfo imageInfo = {};
    imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.pNext     = (imageFormatListEnabled) ? &imageFormatListInfo : externalImageCreateInfo;
    imageInfo.flags     = mCreateFlags;
    imageInfo.imageType = mImageType;
    imageInfo.format    = format.actualImageVkFormat();
    imageInfo.extent    = mExtents;
    imageInfo.mipLevels = mLevelCount;
    imageInfo.arrayLayers           = mLayerCount;
    imageInfo.samples               = gl_vk::GetSamples(mSamples);
    imageInfo.tiling                = mTilingMode;
    imageInfo.usage                 = mUsage;
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = ConvertImageLayoutToVkImageLayout(initialLayout);

    mCurrentLayout = initialLayout;

    ANGLE_VK_TRY(context, mImage.init(context->getDevice(), imageInfo));

    stageClearIfEmulatedFormat(isRobustResourceInitEnabled);

    if (initialLayout != ImageLayout::Undefined)
    {
        setEntireContentDefined();
    }

    return angle::Result::Continue;
}

void ImageHelper::releaseImage(RendererVk *renderer)
{
    renderer->collectGarbageAndReinit(&mUse, &mImage, &mDeviceMemory);
    mImageSerial = kInvalidImageSerial;

    setEntireContentUndefined();
}

void ImageHelper::releaseImageFromShareContexts(RendererVk *renderer, ContextVk *contextVk)
{
    if (contextVk && mImageSerial.valid())
    {
        ContextVkSet &shareContextSet = *contextVk->getShareGroupVk()->getContexts();
        for (ContextVk *ctx : shareContextSet)
        {
            ctx->finalizeImageLayout(this);
        }
    }

    releaseImage(renderer);
}

void ImageHelper::releaseStagingBuffer(RendererVk *renderer)
{
    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    // Remove updates that never made it to the texture.
    for (std::vector<SubresourceUpdate> &levelUpdates : mSubresourceUpdates)
    {
        for (SubresourceUpdate &update : levelUpdates)
        {
            update.release(renderer);
        }
    }

    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    mStagingBuffer.release(renderer);
    mSubresourceUpdates.clear();
    mCurrentSingleClearValue.reset();
}

void ImageHelper::resetImageWeakReference()
{
    mImage.reset();
    mImageSerial        = kInvalidImageSerial;
    mRotatedAspectRatio = false;
}

angle::Result ImageHelper::initializeNonZeroMemory(Context *context,
                                                   bool hasProtectedContent,
                                                   VkDeviceSize size)
{
    const angle::Format &angleFormat = mFormat->actualImageFormat();
    bool isCompressedFormat          = angleFormat.isBlock;

    if (angleFormat.isYUV)
    {
        // VUID-vkCmdClearColorImage-image-01545
        // vkCmdClearColorImage(): format must not be one of the formats requiring sampler YCBCR
        // conversion for VK_IMAGE_ASPECT_COLOR_BIT image views
        return angle::Result::Continue;
    }

    RendererVk *renderer = context->getRenderer();

    PrimaryCommandBuffer commandBuffer;
    ANGLE_TRY(renderer->getCommandBufferOneOff(context, hasProtectedContent, &commandBuffer));

    // Queue a DMA copy.
    barrierImpl(context, getAspectFlags(), ImageLayout::TransferDst, mCurrentQueueFamilyIndex,
                &commandBuffer);

    StagingBuffer stagingBuffer;

    if (isCompressedFormat)
    {
        // If format is compressed, set its contents through buffer copies.

        // The staging buffer memory is non-zero-initialized in 'init'.
        ANGLE_TRY(stagingBuffer.init(context, size, StagingUsage::Write));

        for (LevelIndex level(0); level < LevelIndex(mLevelCount); ++level)
        {
            VkBufferImageCopy copyRegion = {};

            gl_vk::GetExtent(getLevelExtents(level), &copyRegion.imageExtent);
            copyRegion.imageSubresource.aspectMask = getAspectFlags();
            copyRegion.imageSubresource.layerCount = mLayerCount;

            // If image has depth and stencil, copy to each individually per Vulkan spec.
            bool hasBothDepthAndStencil = isCombinedDepthStencilFormat();
            if (hasBothDepthAndStencil)
            {
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            commandBuffer.copyBufferToImage(stagingBuffer.getBuffer().getHandle(), mImage,
                                            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

            if (hasBothDepthAndStencil)
            {
                copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;

                commandBuffer.copyBufferToImage(stagingBuffer.getBuffer().getHandle(), mImage,
                                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1,
                                                &copyRegion);
            }
        }
    }
    else
    {
        // Otherwise issue clear commands.
        VkImageSubresourceRange subresource = {};
        subresource.aspectMask              = getAspectFlags();
        subresource.baseMipLevel            = 0;
        subresource.levelCount              = mLevelCount;
        subresource.baseArrayLayer          = 0;
        subresource.layerCount              = mLayerCount;

        // Arbitrary value to initialize the memory with.  Note: the given uint value, reinterpreted
        // as float is about 0.7.
        constexpr uint32_t kInitValue   = 0x3F345678;
        constexpr float kInitValueFloat = 0.12345f;

        if ((subresource.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != 0)
        {
            VkClearColorValue clearValue;
            clearValue.uint32[0] = kInitValue;
            clearValue.uint32[1] = kInitValue;
            clearValue.uint32[2] = kInitValue;
            clearValue.uint32[3] = kInitValue;

            commandBuffer.clearColorImage(mImage, getCurrentLayout(), clearValue, 1, &subresource);
        }
        else
        {
            VkClearDepthStencilValue clearValue;
            clearValue.depth   = kInitValueFloat;
            clearValue.stencil = kInitValue;

            commandBuffer.clearDepthStencilImage(mImage, getCurrentLayout(), clearValue, 1,
                                                 &subresource);
        }
    }

    ANGLE_VK_TRY(context, commandBuffer.end());

    Serial serial;
    ANGLE_TRY(renderer->queueSubmitOneOff(context, std::move(commandBuffer), hasProtectedContent,
                                          egl::ContextPriority::Medium, nullptr,
                                          vk::SubmitPolicy::AllowDeferred, &serial));

    if (isCompressedFormat)
    {
        stagingBuffer.collectGarbage(renderer, serial);
    }
    mUse.updateSerialOneOff(serial);

    return angle::Result::Continue;
}

angle::Result ImageHelper::initMemory(Context *context,
                                      bool hasProtectedContent,
                                      const MemoryProperties &memoryProperties,
                                      VkMemoryPropertyFlags flags)
{
    // TODO(jmadill): Memory sub-allocation. http://anglebug.com/2162
    VkDeviceSize size;
    if (hasProtectedContent)
    {
        flags |= VK_MEMORY_PROPERTY_PROTECTED_BIT;
    }
    ANGLE_TRY(AllocateImageMemory(context, flags, &flags, nullptr, &mImage, &mDeviceMemory, &size));
    mCurrentQueueFamilyIndex = context->getRenderer()->getQueueFamilyIndex();

    RendererVk *renderer = context->getRenderer();
    if (renderer->getFeatures().allocateNonZeroMemory.enabled)
    {
        // Can't map the memory. Use a staging resource.
        if ((flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0)
        {
            ANGLE_TRY(initializeNonZeroMemory(context, hasProtectedContent, size));
        }
    }

    return angle::Result::Continue;
}

angle::Result ImageHelper::initExternalMemory(
    Context *context,
    const MemoryProperties &memoryProperties,
    const VkMemoryRequirements &memoryRequirements,
    const VkSamplerYcbcrConversionCreateInfo *samplerYcbcrConversionCreateInfo,
    const void *extraAllocationInfo,
    uint32_t currentQueueFamilyIndex,
    VkMemoryPropertyFlags flags)
{
    // TODO(jmadill): Memory sub-allocation. http://anglebug.com/2162
    ANGLE_TRY(AllocateImageMemoryWithRequirements(context, flags, memoryRequirements,
                                                  extraAllocationInfo, &mImage, &mDeviceMemory));
    mCurrentQueueFamilyIndex = currentQueueFamilyIndex;

#ifdef VK_USE_PLATFORM_ANDROID_KHR
    if (samplerYcbcrConversionCreateInfo)
    {
        const VkExternalFormatANDROID *vkExternalFormat =
            reinterpret_cast<const VkExternalFormatANDROID *>(
                samplerYcbcrConversionCreateInfo->pNext);
        ASSERT(vkExternalFormat->sType == VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID);
        mExternalFormat = vkExternalFormat->externalFormat;

        ANGLE_TRY(context->getRenderer()->getYuvConversionCache().getYuvConversion(
            context, mExternalFormat, true, *samplerYcbcrConversionCreateInfo,
            &mYuvConversionSampler));
    }
#endif
    return angle::Result::Continue;
}

angle::Result ImageHelper::initImageView(Context *context,
                                         gl::TextureType textureType,
                                         VkImageAspectFlags aspectMask,
                                         const gl::SwizzleState &swizzleMap,
                                         ImageView *imageViewOut,
                                         LevelIndex baseMipLevelVk,
                                         uint32_t levelCount)
{
    return initLayerImageView(context, textureType, aspectMask, swizzleMap, imageViewOut,
                              baseMipLevelVk, levelCount, 0, mLayerCount,
                              gl::SrgbWriteControlMode::Default);
}

angle::Result ImageHelper::initLayerImageView(Context *context,
                                              gl::TextureType textureType,
                                              VkImageAspectFlags aspectMask,
                                              const gl::SwizzleState &swizzleMap,
                                              ImageView *imageViewOut,
                                              LevelIndex baseMipLevelVk,
                                              uint32_t levelCount,
                                              uint32_t baseArrayLayer,
                                              uint32_t layerCount,
                                              gl::SrgbWriteControlMode mode) const
{
    angle::FormatID imageFormat = mFormat->actualImageFormatID;

    // If we are initializing an imageview for use with EXT_srgb_write_control, we need to override
    // the format to its linear counterpart. Formats that cannot be reinterpreted are exempt from
    // this requirement.
    if (mode == gl::SrgbWriteControlMode::Linear)
    {
        angle::FormatID linearFormat = ConvertToLinear(imageFormat);
        if (linearFormat != angle::FormatID::NONE)
        {
            imageFormat = linearFormat;
        }
    }

    return initLayerImageViewImpl(
        context, textureType, aspectMask, swizzleMap, imageViewOut, baseMipLevelVk, levelCount,
        baseArrayLayer, layerCount,
        context->getRenderer()->getFormat(imageFormat).actualImageVkFormat(), nullptr);
}

angle::Result ImageHelper::initLayerImageViewWithFormat(Context *context,
                                                        gl::TextureType textureType,
                                                        const Format &format,
                                                        VkImageAspectFlags aspectMask,
                                                        const gl::SwizzleState &swizzleMap,
                                                        ImageView *imageViewOut,
                                                        LevelIndex baseMipLevelVk,
                                                        uint32_t levelCount,
                                                        uint32_t baseArrayLayer,
                                                        uint32_t layerCount) const
{
    return initLayerImageViewImpl(context, textureType, aspectMask, swizzleMap, imageViewOut,
                                  baseMipLevelVk, levelCount, baseArrayLayer, layerCount,
                                  format.actualImageVkFormat(), nullptr);
}

angle::Result ImageHelper::initLayerImageViewImpl(
    Context *context,
    gl::TextureType textureType,
    VkImageAspectFlags aspectMask,
    const gl::SwizzleState &swizzleMap,
    ImageView *imageViewOut,
    LevelIndex baseMipLevelVk,
    uint32_t levelCount,
    uint32_t baseArrayLayer,
    uint32_t layerCount,
    VkFormat imageFormat,
    const VkImageViewUsageCreateInfo *imageViewUsageCreateInfo) const
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.flags                 = 0;
    viewInfo.image                 = mImage.getHandle();
    viewInfo.viewType              = gl_vk::GetImageViewType(textureType);
    viewInfo.format                = imageFormat;

    if (swizzleMap.swizzleRequired() && !mYuvConversionSampler.valid())
    {
        viewInfo.components.r = gl_vk::GetSwizzle(swizzleMap.swizzleRed);
        viewInfo.components.g = gl_vk::GetSwizzle(swizzleMap.swizzleGreen);
        viewInfo.components.b = gl_vk::GetSwizzle(swizzleMap.swizzleBlue);
        viewInfo.components.a = gl_vk::GetSwizzle(swizzleMap.swizzleAlpha);
    }
    else
    {
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    }
    viewInfo.subresourceRange.aspectMask     = aspectMask;
    viewInfo.subresourceRange.baseMipLevel   = baseMipLevelVk.get();
    viewInfo.subresourceRange.levelCount     = levelCount;
    viewInfo.subresourceRange.baseArrayLayer = baseArrayLayer;
    viewInfo.subresourceRange.layerCount     = layerCount;

    viewInfo.pNext = imageViewUsageCreateInfo;

    VkSamplerYcbcrConversionInfo yuvConversionInfo = {};
    if (mYuvConversionSampler.valid())
    {
        ASSERT((context->getRenderer()->getFeatures().supportsYUVSamplerConversion.enabled));
        yuvConversionInfo.sType      = VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO;
        yuvConversionInfo.pNext      = nullptr;
        yuvConversionInfo.conversion = mYuvConversionSampler.get().getHandle();
        AddToPNextChain(&viewInfo, &yuvConversionInfo);

        // VUID-VkImageViewCreateInfo-image-02399
        // If image has an external format, format must be VK_FORMAT_UNDEFINED
        if (mExternalFormat)
        {
            viewInfo.format = VK_FORMAT_UNDEFINED;
        }
    }
    ANGLE_VK_TRY(context, imageViewOut->init(context->getDevice(), viewInfo));
    return angle::Result::Continue;
}

angle::Result ImageHelper::initReinterpretedLayerImageView(Context *context,
                                                           gl::TextureType textureType,
                                                           VkImageAspectFlags aspectMask,
                                                           const gl::SwizzleState &swizzleMap,
                                                           ImageView *imageViewOut,
                                                           LevelIndex baseMipLevelVk,
                                                           uint32_t levelCount,
                                                           uint32_t baseArrayLayer,
                                                           uint32_t layerCount,
                                                           VkImageUsageFlags imageUsageFlags,
                                                           angle::FormatID imageViewFormat) const
{
    VkImageViewUsageCreateInfo imageViewUsageCreateInfo = {};
    imageViewUsageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO;
    imageViewUsageCreateInfo.usage =
        imageUsageFlags & GetMaximalImageUsageFlags(context->getRenderer(), imageViewFormat);

    return initLayerImageViewImpl(context, textureType, aspectMask, swizzleMap, imageViewOut,
                                  baseMipLevelVk, levelCount, baseArrayLayer, layerCount,
                                  vk::GetVkFormatFromFormatID(imageViewFormat),
                                  &imageViewUsageCreateInfo);
}

void ImageHelper::destroy(RendererVk *renderer)
{
    VkDevice device = renderer->getDevice();

    mImage.destroy(device);
    mDeviceMemory.destroy(device);
    mStagingBuffer.destroy(renderer);
    mCurrentLayout = ImageLayout::Undefined;
    mImageType     = VK_IMAGE_TYPE_2D;
    mLayerCount    = 0;
    mLevelCount    = 0;

    setEntireContentUndefined();
}

void ImageHelper::init2DWeakReference(Context *context,
                                      VkImage handle,
                                      const gl::Extents &glExtents,
                                      bool rotatedAspectRatio,
                                      const Format &format,
                                      GLint samples,
                                      bool isRobustResourceInitEnabled)
{
    ASSERT(!valid());
    ASSERT(!IsAnySubresourceContentDefined(mContentDefined));
    ASSERT(!IsAnySubresourceContentDefined(mStencilContentDefined));

    gl_vk::GetExtent(glExtents, &mExtents);
    mRotatedAspectRatio = rotatedAspectRatio;
    mFormat             = &format;
    mSamples            = std::max(samples, 1);
    mImageSerial        = context->getRenderer()->getResourceSerialFactory().generateImageSerial();
    mCurrentLayout      = ImageLayout::Undefined;
    mLayerCount         = 1;
    mLevelCount         = 1;

    mImage.setHandle(handle);

    stageClearIfEmulatedFormat(isRobustResourceInitEnabled);
}

angle::Result ImageHelper::init2DStaging(Context *context,
                                         bool hasProtectedContent,
                                         const MemoryProperties &memoryProperties,
                                         const gl::Extents &glExtents,
                                         const Format &format,
                                         VkImageUsageFlags usage,
                                         uint32_t layerCount)
{
    gl_vk::GetExtent(glExtents, &mExtents);

    return initStaging(context, hasProtectedContent, memoryProperties, VK_IMAGE_TYPE_2D, mExtents,
                       format, 1, usage, 1, layerCount);
}

angle::Result ImageHelper::initStaging(Context *context,
                                       bool hasProtectedContent,
                                       const MemoryProperties &memoryProperties,
                                       VkImageType imageType,
                                       const VkExtent3D &extents,
                                       const Format &format,
                                       GLint samples,
                                       VkImageUsageFlags usage,
                                       uint32_t mipLevels,
                                       uint32_t layerCount)
{
    ASSERT(!valid());
    ASSERT(!IsAnySubresourceContentDefined(mContentDefined));
    ASSERT(!IsAnySubresourceContentDefined(mStencilContentDefined));

    mImageType          = imageType;
    mExtents            = extents;
    mRotatedAspectRatio = false;
    mFormat             = &format;
    mSamples            = std::max(samples, 1);
    mImageSerial        = context->getRenderer()->getResourceSerialFactory().generateImageSerial();
    mLayerCount         = layerCount;
    mLevelCount         = mipLevels;
    mUsage              = usage;

    // Validate that mLayerCount is compatible with the image type
    ASSERT(imageType != VK_IMAGE_TYPE_3D || mLayerCount == 1);
    ASSERT(imageType != VK_IMAGE_TYPE_2D || mExtents.depth == 1);

    mCurrentLayout = ImageLayout::Undefined;

    VkImageCreateInfo imageInfo     = {};
    imageInfo.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.flags                 = hasProtectedContent ? VK_IMAGE_CREATE_PROTECTED_BIT : 0;
    imageInfo.imageType             = mImageType;
    imageInfo.format                = format.actualImageVkFormat();
    imageInfo.extent                = mExtents;
    imageInfo.mipLevels             = mLevelCount;
    imageInfo.arrayLayers           = mLayerCount;
    imageInfo.samples               = gl_vk::GetSamples(mSamples);
    imageInfo.tiling                = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.usage                 = usage;
    imageInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.queueFamilyIndexCount = 0;
    imageInfo.pQueueFamilyIndices   = nullptr;
    imageInfo.initialLayout         = getCurrentLayout();

    ANGLE_VK_TRY(context, mImage.init(context->getDevice(), imageInfo));

    // Allocate and bind device-local memory.
    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (hasProtectedContent)
    {
        memoryPropertyFlags |= VK_MEMORY_PROPERTY_PROTECTED_BIT;
    }
    ANGLE_TRY(initMemory(context, hasProtectedContent, memoryProperties, memoryPropertyFlags));

    return angle::Result::Continue;
}

angle::Result ImageHelper::initImplicitMultisampledRenderToTexture(
    Context *context,
    bool hasProtectedContent,
    const MemoryProperties &memoryProperties,
    gl::TextureType textureType,
    GLint samples,
    const ImageHelper &resolveImage,
    bool isRobustResourceInitEnabled)
{
    ASSERT(!valid());
    ASSERT(samples > 1);
    ASSERT(!IsAnySubresourceContentDefined(mContentDefined));
    ASSERT(!IsAnySubresourceContentDefined(mStencilContentDefined));

    // The image is used as either color or depth/stencil attachment.  Additionally, its memory is
    // lazily allocated as the contents are discarded at the end of the renderpass and with tiling
    // GPUs no actual backing memory is required.
    //
    // Note that the Vulkan image is created with or without VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT
    // based on whether the memory that will be used to create the image would have
    // VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT.  TRANSIENT is provided if there is any memory that
    // supports LAZILY_ALLOCATED.  However, based on actual image requirements, such a memory may
    // not be suitable for the image.  We don't support such a case, which will result in the
    // |initMemory| call below failing.
    const bool hasLazilyAllocatedMemory = memoryProperties.hasLazilyAllocatedMemory();

    const VkImageUsageFlags kMultisampledUsageFlags =
        (hasLazilyAllocatedMemory ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 0) |
        (resolveImage.getAspectFlags() == VK_IMAGE_ASPECT_COLOR_BIT
             ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
             : VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);
    const VkImageCreateFlags kMultisampledCreateFlags =
        hasProtectedContent ? VK_IMAGE_CREATE_PROTECTED_BIT : 0;

    ANGLE_TRY(initExternal(
        context, textureType, resolveImage.getExtents(), resolveImage.getFormat(), samples,
        kMultisampledUsageFlags, kMultisampledCreateFlags, ImageLayout::Undefined, nullptr,
        resolveImage.getFirstAllocatedLevel(), resolveImage.getLevelCount(),
        resolveImage.getLayerCount(), isRobustResourceInitEnabled, nullptr, hasProtectedContent));

    const VkMemoryPropertyFlags kMultisampledMemoryFlags =
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
        (hasLazilyAllocatedMemory ? VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT : 0) |
        (hasProtectedContent ? VK_MEMORY_PROPERTY_PROTECTED_BIT : 0);

    // If this ever fails, this code should be modified to retry creating the image without the
    // TRANSIENT flag.
    ANGLE_TRY(initMemory(context, hasProtectedContent, memoryProperties, kMultisampledMemoryFlags));

    // Remove the emulated format clear from the multisampled image if any.  There is one already
    // staged on the resolve image if needed.
    removeStagedUpdates(context, getFirstAllocatedLevel(), getLastAllocatedLevel());

    return angle::Result::Continue;
}

VkImageAspectFlags ImageHelper::getAspectFlags() const
{
    return GetFormatAspectFlags(mFormat->actualImageFormat());
}

bool ImageHelper::isCombinedDepthStencilFormat() const
{
    return (getAspectFlags() & kDepthStencilAspects) == kDepthStencilAspects;
}

VkImageLayout ImageHelper::getCurrentLayout() const
{
    return ConvertImageLayoutToVkImageLayout(mCurrentLayout);
}

gl::Extents ImageHelper::getLevelExtents(LevelIndex levelVk) const
{
    // Level 0 should be the size of the extents, after that every time you increase a level
    // you shrink the extents by half.
    uint32_t width  = std::max(mExtents.width >> levelVk.get(), 1u);
    uint32_t height = std::max(mExtents.height >> levelVk.get(), 1u);
    uint32_t depth  = std::max(mExtents.depth >> levelVk.get(), 1u);

    return gl::Extents(width, height, depth);
}

gl::Extents ImageHelper::getLevelExtents2D(LevelIndex levelVk) const
{
    gl::Extents extents = getLevelExtents(levelVk);
    extents.depth       = 1;
    return extents;
}

const VkExtent3D ImageHelper::getRotatedExtents() const
{
    VkExtent3D extents = mExtents;
    if (mRotatedAspectRatio)
    {
        std::swap(extents.width, extents.height);
    }
    return extents;
}

gl::Extents ImageHelper::getRotatedLevelExtents2D(LevelIndex levelVk) const
{
    gl::Extents extents = getLevelExtents2D(levelVk);
    if (mRotatedAspectRatio)
    {
        std::swap(extents.width, extents.height);
    }
    return extents;
}

bool ImageHelper::isDepthOrStencil() const
{
    return mFormat->actualImageFormat().hasDepthOrStencilBits();
}

void ImageHelper::setRenderPassUsageFlag(RenderPassUsage flag)
{
    mRenderPassUsageFlags.set(flag);
}

void ImageHelper::clearRenderPassUsageFlag(RenderPassUsage flag)
{
    mRenderPassUsageFlags.reset(flag);
}

void ImageHelper::resetRenderPassUsageFlags()
{
    mRenderPassUsageFlags.reset();
}

bool ImageHelper::hasRenderPassUsageFlag(RenderPassUsage flag) const
{
    return mRenderPassUsageFlags.test(flag);
}

bool ImageHelper::usedByCurrentRenderPassAsAttachmentAndSampler() const
{
    return mRenderPassUsageFlags[RenderPassUsage::RenderTargetAttachment] &&
           mRenderPassUsageFlags[RenderPassUsage::TextureSampler];
}

bool ImageHelper::isReadBarrierNecessary(ImageLayout newLayout) const
{
    // If transitioning to a different layout, we need always need a barrier.
    if (mCurrentLayout != newLayout)
    {
        return true;
    }

    // RAR (read-after-read) is not a hazard and doesn't require a barrier.
    //
    // RAW (read-after-write) hazards always require a memory barrier.  This can only happen if the
    // layout (same as new layout) is writable which in turn is only possible if the image is
    // simultaneously bound for shader write (i.e. the layout is GENERAL).
    const ImageMemoryBarrierData &layoutData = kImageMemoryBarrierData[mCurrentLayout];
    return layoutData.type == ResourceAccess::Write;
}

void ImageHelper::changeLayoutAndQueue(Context *context,
                                       VkImageAspectFlags aspectMask,
                                       ImageLayout newLayout,
                                       uint32_t newQueueFamilyIndex,
                                       CommandBuffer *commandBuffer)
{
    ASSERT(isQueueChangeNeccesary(newQueueFamilyIndex));
    barrierImpl(context, aspectMask, newLayout, newQueueFamilyIndex, commandBuffer);
}

void ImageHelper::acquireFromExternal(ContextVk *contextVk,
                                      uint32_t externalQueueFamilyIndex,
                                      uint32_t rendererQueueFamilyIndex,
                                      ImageLayout currentLayout,
                                      CommandBuffer *commandBuffer)
{
    // The image must be newly allocated or have been released to the external
    // queue. If this is not the case, it's an application bug, so ASSERT might
    // eventually need to change to a warning.
    ASSERT(mCurrentLayout == ImageLayout::Undefined ||
           mCurrentQueueFamilyIndex == externalQueueFamilyIndex);

    mCurrentLayout           = currentLayout;
    mCurrentQueueFamilyIndex = externalQueueFamilyIndex;

    changeLayoutAndQueue(contextVk, getAspectFlags(), mCurrentLayout, rendererQueueFamilyIndex,
                         commandBuffer);

    // It is unknown how the external has modified the image, so assume every subresource has
    // defined content.  That is unless the layout is Undefined.
    if (currentLayout == ImageLayout::Undefined)
    {
        setEntireContentUndefined();
    }
    else
    {
        setEntireContentDefined();
    }
}

void ImageHelper::releaseToExternal(ContextVk *contextVk,
                                    uint32_t rendererQueueFamilyIndex,
                                    uint32_t externalQueueFamilyIndex,
                                    ImageLayout desiredLayout,
                                    CommandBuffer *commandBuffer)
{
    ASSERT(mCurrentQueueFamilyIndex == rendererQueueFamilyIndex);

    changeLayoutAndQueue(contextVk, getAspectFlags(), desiredLayout, externalQueueFamilyIndex,
                         commandBuffer);
}

bool ImageHelper::isReleasedToExternal() const
{
#if !defined(ANGLE_PLATFORM_MACOS) && !defined(ANGLE_PLATFORM_ANDROID)
    return IsExternalQueueFamily(mCurrentQueueFamilyIndex);
#else
    // TODO(anglebug.com/4635): Implement external memory barriers on Mac/Android.
    return false;
#endif
}

void ImageHelper::setFirstAllocatedLevel(gl::LevelIndex firstLevel)
{
    ASSERT(!valid());
    mFirstAllocatedLevel = firstLevel;
}

LevelIndex ImageHelper::toVkLevel(gl::LevelIndex levelIndexGL) const
{
    return gl_vk::GetLevelIndex(levelIndexGL, mFirstAllocatedLevel);
}

gl::LevelIndex ImageHelper::toGLLevel(LevelIndex levelIndexVk) const
{
    return vk_gl::GetLevelIndex(levelIndexVk, mFirstAllocatedLevel);
}

ANGLE_INLINE void ImageHelper::initImageMemoryBarrierStruct(
    VkImageAspectFlags aspectMask,
    ImageLayout newLayout,
    uint32_t newQueueFamilyIndex,
    VkImageMemoryBarrier *imageMemoryBarrier) const
{
    const ImageMemoryBarrierData &transitionFrom = kImageMemoryBarrierData[mCurrentLayout];
    const ImageMemoryBarrierData &transitionTo   = kImageMemoryBarrierData[newLayout];

    imageMemoryBarrier->sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier->srcAccessMask       = transitionFrom.srcAccessMask;
    imageMemoryBarrier->dstAccessMask       = transitionTo.dstAccessMask;
    imageMemoryBarrier->oldLayout           = transitionFrom.layout;
    imageMemoryBarrier->newLayout           = transitionTo.layout;
    imageMemoryBarrier->srcQueueFamilyIndex = mCurrentQueueFamilyIndex;
    imageMemoryBarrier->dstQueueFamilyIndex = newQueueFamilyIndex;
    imageMemoryBarrier->image               = mImage.getHandle();

    // Transition the whole resource.
    imageMemoryBarrier->subresourceRange.aspectMask     = aspectMask;
    imageMemoryBarrier->subresourceRange.baseMipLevel   = 0;
    imageMemoryBarrier->subresourceRange.levelCount     = mLevelCount;
    imageMemoryBarrier->subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier->subresourceRange.layerCount     = mLayerCount;
}

// Generalized to accept both "primary" and "secondary" command buffers.
template <typename CommandBufferT>
void ImageHelper::barrierImpl(Context *context,
                              VkImageAspectFlags aspectMask,
                              ImageLayout newLayout,
                              uint32_t newQueueFamilyIndex,
                              CommandBufferT *commandBuffer)
{
    const ImageMemoryBarrierData &transitionFrom = kImageMemoryBarrierData[mCurrentLayout];
    const ImageMemoryBarrierData &transitionTo   = kImageMemoryBarrierData[newLayout];

    VkImageMemoryBarrier imageMemoryBarrier = {};
    initImageMemoryBarrierStruct(aspectMask, newLayout, newQueueFamilyIndex, &imageMemoryBarrier);

    // There might be other shaderRead operations there other than the current layout.
    VkPipelineStageFlags srcStageMask = GetImageLayoutSrcStageMask(context, transitionFrom);
    if (mCurrentShaderReadStageMask)
    {
        srcStageMask |= mCurrentShaderReadStageMask;
        mCurrentShaderReadStageMask  = 0;
        mLastNonShaderReadOnlyLayout = ImageLayout::Undefined;
    }
    commandBuffer->imageBarrier(srcStageMask, GetImageLayoutDstStageMask(context, transitionTo),
                                imageMemoryBarrier);

    mCurrentLayout           = newLayout;
    mCurrentQueueFamilyIndex = newQueueFamilyIndex;
}

template void ImageHelper::barrierImpl<priv::SecondaryCommandBuffer>(
    Context *context,
    VkImageAspectFlags aspectMask,
    ImageLayout newLayout,
    uint32_t newQueueFamilyIndex,
    priv::SecondaryCommandBuffer *commandBuffer);

bool ImageHelper::updateLayoutAndBarrier(Context *context,
                                         VkImageAspectFlags aspectMask,
                                         ImageLayout newLayout,
                                         PipelineBarrier *barrier)
{
    bool barrierModified = false;
    if (newLayout == mCurrentLayout)
    {
        const ImageMemoryBarrierData &layoutData = kImageMemoryBarrierData[mCurrentLayout];
        // RAR is not a hazard and doesn't require a barrier, especially as the image layout hasn't
        // changed.  The following asserts that such a barrier is not attempted.
        ASSERT(layoutData.type == ResourceAccess::Write);
        // No layout change, only memory barrier is required
        barrier->mergeMemoryBarrier(GetImageLayoutSrcStageMask(context, layoutData),
                                    GetImageLayoutDstStageMask(context, layoutData),
                                    layoutData.srcAccessMask, layoutData.dstAccessMask);
        barrierModified = true;
    }
    else
    {
        const ImageMemoryBarrierData &transitionFrom = kImageMemoryBarrierData[mCurrentLayout];
        const ImageMemoryBarrierData &transitionTo   = kImageMemoryBarrierData[newLayout];
        VkPipelineStageFlags srcStageMask = GetImageLayoutSrcStageMask(context, transitionFrom);
        VkPipelineStageFlags dstStageMask = GetImageLayoutDstStageMask(context, transitionTo);

        if (IsShaderReadOnlyLayout(transitionTo) && IsShaderReadOnlyLayout(transitionFrom))
        {
            // If we are switching between different shader stage reads, then there is no actual
            // layout change or access type change. We only need a barrier if we are making a read
            // that is from a new stage. Also note that we barrier against previous non-shaderRead
            // layout. We do not barrier between one shaderRead and another shaderRead.
            bool isNewReadStage = (mCurrentShaderReadStageMask & dstStageMask) != dstStageMask;
            if (isNewReadStage)
            {
                const ImageMemoryBarrierData &layoutData =
                    kImageMemoryBarrierData[mLastNonShaderReadOnlyLayout];
                barrier->mergeMemoryBarrier(GetImageLayoutSrcStageMask(context, layoutData),
                                            dstStageMask, layoutData.srcAccessMask,
                                            transitionTo.dstAccessMask);
                barrierModified = true;
                // Accumulate new read stage.
                mCurrentShaderReadStageMask |= dstStageMask;
            }
        }
        else
        {
            VkImageMemoryBarrier imageMemoryBarrier = {};
            initImageMemoryBarrierStruct(aspectMask, newLayout, mCurrentQueueFamilyIndex,
                                         &imageMemoryBarrier);
            // if we transition from shaderReadOnly, we must add in stashed shader stage masks since
            // there might be outstanding shader reads from stages other than current layout. We do
            // not insert barrier between one shaderRead to another shaderRead
            if (mCurrentShaderReadStageMask)
            {
                srcStageMask |= mCurrentShaderReadStageMask;
                mCurrentShaderReadStageMask  = 0;
                mLastNonShaderReadOnlyLayout = ImageLayout::Undefined;
            }
            barrier->mergeImageBarrier(srcStageMask, dstStageMask, imageMemoryBarrier);
            barrierModified = true;

            // If we are transition into shaderRead layout, remember the last
            // non-shaderRead layout here.
            if (IsShaderReadOnlyLayout(transitionTo))
            {
                ASSERT(!IsShaderReadOnlyLayout(transitionFrom));
                mLastNonShaderReadOnlyLayout = mCurrentLayout;
                mCurrentShaderReadStageMask  = dstStageMask;
            }
        }
        mCurrentLayout = newLayout;
    }
    return barrierModified;
}

void ImageHelper::clearColor(const VkClearColorValue &color,
                             LevelIndex baseMipLevelVk,
                             uint32_t levelCount,
                             uint32_t baseArrayLayer,
                             uint32_t layerCount,
                             CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    ASSERT(mCurrentLayout == ImageLayout::TransferDst);

    VkImageSubresourceRange range = {};
    range.aspectMask              = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel            = baseMipLevelVk.get();
    range.levelCount              = levelCount;
    range.baseArrayLayer          = baseArrayLayer;
    range.layerCount              = layerCount;

    if (mImageType == VK_IMAGE_TYPE_3D)
    {
        ASSERT(baseArrayLayer == 0);
        ASSERT(layerCount == 1 ||
               layerCount == static_cast<uint32_t>(getLevelExtents(baseMipLevelVk).depth));
        range.layerCount = 1;
    }

    commandBuffer->clearColorImage(mImage, getCurrentLayout(), color, 1, &range);
}

void ImageHelper::clearDepthStencil(VkImageAspectFlags clearAspectFlags,
                                    const VkClearDepthStencilValue &depthStencil,
                                    LevelIndex baseMipLevelVk,
                                    uint32_t levelCount,
                                    uint32_t baseArrayLayer,
                                    uint32_t layerCount,
                                    CommandBuffer *commandBuffer)
{
    ASSERT(valid());

    ASSERT(mCurrentLayout == ImageLayout::TransferDst);

    VkImageSubresourceRange range = {};
    range.aspectMask              = clearAspectFlags;
    range.baseMipLevel            = baseMipLevelVk.get();
    range.levelCount              = levelCount;
    range.baseArrayLayer          = baseArrayLayer;
    range.layerCount              = layerCount;

    if (mImageType == VK_IMAGE_TYPE_3D)
    {
        ASSERT(baseArrayLayer == 0);
        ASSERT(layerCount == 1 ||
               layerCount == static_cast<uint32_t>(getLevelExtents(baseMipLevelVk).depth));
        range.layerCount = 1;
    }

    commandBuffer->clearDepthStencilImage(mImage, getCurrentLayout(), depthStencil, 1, &range);
}

void ImageHelper::clear(VkImageAspectFlags aspectFlags,
                        const VkClearValue &value,
                        LevelIndex mipLevel,
                        uint32_t baseArrayLayer,
                        uint32_t layerCount,
                        CommandBuffer *commandBuffer)
{
    const angle::Format &angleFormat = mFormat->actualImageFormat();
    bool isDepthStencil              = angleFormat.depthBits > 0 || angleFormat.stencilBits > 0;

    if (isDepthStencil)
    {
        clearDepthStencil(aspectFlags, value.depthStencil, mipLevel, 1, baseArrayLayer, layerCount,
                          commandBuffer);
    }
    else
    {
        ASSERT(!angleFormat.isBlock);

        clearColor(value.color, mipLevel, 1, baseArrayLayer, layerCount, commandBuffer);
    }
}

// static
void ImageHelper::Copy(ImageHelper *srcImage,
                       ImageHelper *dstImage,
                       const gl::Offset &srcOffset,
                       const gl::Offset &dstOffset,
                       const gl::Extents &copySize,
                       const VkImageSubresourceLayers &srcSubresource,
                       const VkImageSubresourceLayers &dstSubresource,
                       CommandBuffer *commandBuffer)
{
    ASSERT(commandBuffer->valid() && srcImage->valid() && dstImage->valid());

    ASSERT(srcImage->getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    ASSERT(dstImage->getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkImageCopy region    = {};
    region.srcSubresource = srcSubresource;
    region.srcOffset.x    = srcOffset.x;
    region.srcOffset.y    = srcOffset.y;
    region.srcOffset.z    = srcOffset.z;
    region.dstSubresource = dstSubresource;
    region.dstOffset.x    = dstOffset.x;
    region.dstOffset.y    = dstOffset.y;
    region.dstOffset.z    = dstOffset.z;
    region.extent.width   = copySize.width;
    region.extent.height  = copySize.height;
    region.extent.depth   = copySize.depth;

    commandBuffer->copyImage(srcImage->getImage(), srcImage->getCurrentLayout(),
                             dstImage->getImage(), dstImage->getCurrentLayout(), 1, &region);
}

// static
angle::Result ImageHelper::CopyImageSubData(const gl::Context *context,
                                            ImageHelper *srcImage,
                                            GLint srcLevel,
                                            GLint srcX,
                                            GLint srcY,
                                            GLint srcZ,
                                            ImageHelper *dstImage,
                                            GLint dstLevel,
                                            GLint dstX,
                                            GLint dstY,
                                            GLint dstZ,
                                            GLsizei srcWidth,
                                            GLsizei srcHeight,
                                            GLsizei srcDepth)
{
    ContextVk *contextVk = GetImpl(context);

    const Format &sourceVkFormat = srcImage->getFormat();
    VkImageTiling srcTilingMode  = srcImage->getTilingMode();
    const Format &destVkFormat   = dstImage->getFormat();
    VkImageTiling destTilingMode = dstImage->getTilingMode();

    const gl::LevelIndex srcLevelGL = gl::LevelIndex(srcLevel);
    const gl::LevelIndex dstLevelGL = gl::LevelIndex(dstLevel);

    if (CanCopyWithTransferForCopyImage(contextVk->getRenderer(), sourceVkFormat, srcTilingMode,
                                        destVkFormat, destTilingMode))
    {
        bool isSrc3D = srcImage->getType() == VK_IMAGE_TYPE_3D;
        bool isDst3D = dstImage->getType() == VK_IMAGE_TYPE_3D;

        srcImage->retain(&contextVk->getResourceUseList());
        dstImage->retain(&contextVk->getResourceUseList());

        VkImageCopy region = {};

        region.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.srcSubresource.mipLevel       = srcImage->toVkLevel(srcLevelGL).get();
        region.srcSubresource.baseArrayLayer = isSrc3D ? 0 : srcZ;
        region.srcSubresource.layerCount     = isSrc3D ? 1 : srcDepth;

        region.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        region.dstSubresource.mipLevel       = dstImage->toVkLevel(dstLevelGL).get();
        region.dstSubresource.baseArrayLayer = isDst3D ? 0 : dstZ;
        region.dstSubresource.layerCount     = isDst3D ? 1 : srcDepth;

        region.srcOffset.x   = srcX;
        region.srcOffset.y   = srcY;
        region.srcOffset.z   = isSrc3D ? srcZ : 0;
        region.dstOffset.x   = dstX;
        region.dstOffset.y   = dstY;
        region.dstOffset.z   = isDst3D ? dstZ : 0;
        region.extent.width  = srcWidth;
        region.extent.height = srcHeight;
        region.extent.depth  = (isSrc3D || isDst3D) ? srcDepth : 1;

        CommandBufferAccess access;
        access.onImageTransferRead(VK_IMAGE_ASPECT_COLOR_BIT, srcImage);
        access.onImageTransferWrite(dstLevelGL, 1, region.dstSubresource.baseArrayLayer,
                                    region.dstSubresource.layerCount, VK_IMAGE_ASPECT_COLOR_BIT,
                                    dstImage);

        CommandBuffer *commandBuffer;
        ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

        ASSERT(srcImage->valid() && dstImage->valid());
        ASSERT(srcImage->getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        ASSERT(dstImage->getCurrentLayout() == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        commandBuffer->copyImage(srcImage->getImage(), srcImage->getCurrentLayout(),
                                 dstImage->getImage(), dstImage->getCurrentLayout(), 1, &region);
    }
    else if (!sourceVkFormat.intendedFormat().isBlock && !destVkFormat.intendedFormat().isBlock)
    {
        // The source and destination image formats may be using a fallback in the case of RGB
        // images.  A compute shader is used in such a case to perform the copy.
        UtilsVk &utilsVk = contextVk->getUtils();

        UtilsVk::CopyImageBitsParameters params;
        params.srcOffset[0]   = srcX;
        params.srcOffset[1]   = srcY;
        params.srcOffset[2]   = srcZ;
        params.srcLevel       = srcLevelGL;
        params.dstOffset[0]   = dstX;
        params.dstOffset[1]   = dstY;
        params.dstOffset[2]   = dstZ;
        params.dstLevel       = dstLevelGL;
        params.copyExtents[0] = srcWidth;
        params.copyExtents[1] = srcHeight;
        params.copyExtents[2] = srcDepth;

        ANGLE_TRY(utilsVk.copyImageBits(contextVk, dstImage, srcImage, params));
    }
    else
    {
        // No support for emulated compressed formats.
        UNIMPLEMENTED();
        ANGLE_VK_CHECK(contextVk, false, VK_ERROR_FEATURE_NOT_PRESENT);
    }

    return angle::Result::Continue;
}

angle::Result ImageHelper::generateMipmapsWithBlit(ContextVk *contextVk,
                                                   LevelIndex baseLevel,
                                                   LevelIndex maxLevel)
{
    CommandBufferAccess access;
    gl::LevelIndex baseLevelGL = toGLLevel(baseLevel);
    access.onImageTransferWrite(baseLevelGL + 1, maxLevel.get(), 0, mLayerCount,
                                VK_IMAGE_ASPECT_COLOR_BIT, this);

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    // We are able to use blitImage since the image format we are using supports it.
    int32_t mipWidth  = mExtents.width;
    int32_t mipHeight = mExtents.height;
    int32_t mipDepth  = mExtents.depth;

    // Manually manage the image memory barrier because it uses a lot more parameters than our
    // usual one.
    VkImageMemoryBarrier barrier            = {};
    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image                           = mImage.getHandle();
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = mLayerCount;
    barrier.subresourceRange.levelCount     = 1;

    const VkFilter filter = gl_vk::GetFilter(CalculateGenerateMipmapFilter(contextVk, getFormat()));

    for (LevelIndex mipLevel(1); mipLevel <= LevelIndex(mLevelCount); ++mipLevel)
    {
        int32_t nextMipWidth  = std::max<int32_t>(1, mipWidth >> 1);
        int32_t nextMipHeight = std::max<int32_t>(1, mipHeight >> 1);
        int32_t nextMipDepth  = std::max<int32_t>(1, mipDepth >> 1);

        if (mipLevel > baseLevel && mipLevel <= maxLevel)
        {
            barrier.subresourceRange.baseMipLevel = mipLevel.get() - 1;
            barrier.oldLayout                     = getCurrentLayout();
            barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

            // We can do it for all layers at once.
            commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                        VK_PIPELINE_STAGE_TRANSFER_BIT, barrier);
            VkImageBlit blit                   = {};
            blit.srcOffsets[0]                 = {0, 0, 0};
            blit.srcOffsets[1]                 = {mipWidth, mipHeight, mipDepth};
            blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel       = mipLevel.get() - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount     = mLayerCount;
            blit.dstOffsets[0]                 = {0, 0, 0};
            blit.dstOffsets[1]                 = {nextMipWidth, nextMipHeight, nextMipDepth};
            blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel       = mipLevel.get();
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount     = mLayerCount;

            commandBuffer->blitImage(mImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, mImage,
                                     VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);
        }
        mipWidth  = nextMipWidth;
        mipHeight = nextMipHeight;
        mipDepth  = nextMipDepth;
    }

    // Transition all mip level to the same layout so we can declare our whole image layout to one
    // ImageLayout. FragmentShaderReadOnly is picked here since this is the most reasonable usage
    // after glGenerateMipmap call.
    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    if (baseLevel.get() > 0)
    {
        // [0:baseLevel-1] from TRANSFER_DST to SHADER_READ
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount   = baseLevel.get();
        commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier);
    }
    // [maxLevel:mLevelCount-1] from TRANSFER_DST to SHADER_READ
    ASSERT(mLevelCount > maxLevel.get());
    barrier.subresourceRange.baseMipLevel = maxLevel.get();
    barrier.subresourceRange.levelCount   = mLevelCount - maxLevel.get();
    commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier);
    // [baseLevel:maxLevel-1] from TRANSFER_SRC to SHADER_READ
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.subresourceRange.baseMipLevel = baseLevel.get();
    barrier.subresourceRange.levelCount   = maxLevel.get() - baseLevel.get();
    commandBuffer->imageBarrier(VK_PIPELINE_STAGE_TRANSFER_BIT,
                                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, barrier);

    // This is just changing the internal state of the image helper so that the next call
    // to changeLayout will use this layout as the "oldLayout" argument.
    // mLastNonShaderReadOnlyLayout is used to ensure previous write are made visible to reads,
    // since the only write here is transfer, hence mLastNonShaderReadOnlyLayout is set to
    // ImageLayout::TransferDst.
    mLastNonShaderReadOnlyLayout = ImageLayout::TransferDst;
    mCurrentShaderReadStageMask  = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    mCurrentLayout               = ImageLayout::FragmentShaderReadOnly;

    return angle::Result::Continue;
}

void ImageHelper::resolve(ImageHelper *dest,
                          const VkImageResolve &region,
                          CommandBuffer *commandBuffer)
{
    ASSERT(mCurrentLayout == ImageLayout::TransferSrc);
    commandBuffer->resolveImage(getImage(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dest->getImage(),
                                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

void ImageHelper::removeSingleSubresourceStagedUpdates(ContextVk *contextVk,
                                                       gl::LevelIndex levelIndexGL,
                                                       uint32_t layerIndex,
                                                       uint32_t layerCount)
{
    mCurrentSingleClearValue.reset();

    // Find any staged updates for this index and remove them from the pending list.
    std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(levelIndexGL);
    if (levelUpdates == nullptr)
    {
        return;
    }

    for (size_t index = 0; index < levelUpdates->size();)
    {
        auto update = levelUpdates->begin() + index;
        if (update->isUpdateToLayers(layerIndex, layerCount))
        {
            update->release(contextVk->getRenderer());
            levelUpdates->erase(update);
        }
        else
        {
            index++;
        }
    }
}

void ImageHelper::removeStagedUpdates(Context *context,
                                      gl::LevelIndex levelGLStart,
                                      gl::LevelIndex levelGLEnd)
{
    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    // Remove all updates to levels [start, end].
    for (gl::LevelIndex level = levelGLStart; level <= levelGLEnd; ++level)
    {
        std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(level);
        if (levelUpdates == nullptr)
        {
            ASSERT(static_cast<size_t>(level.get()) >= mSubresourceUpdates.size());
            return;
        }

        for (SubresourceUpdate &update : *levelUpdates)
        {
            update.release(context->getRenderer());
        }

        levelUpdates->clear();
    }

    ASSERT(validateSubresourceUpdateImageRefsConsistent());
}

angle::Result ImageHelper::stageSubresourceUpdateImpl(ContextVk *contextVk,
                                                      const gl::ImageIndex &index,
                                                      const gl::Extents &glExtents,
                                                      const gl::Offset &offset,
                                                      const gl::InternalFormat &formatInfo,
                                                      const gl::PixelUnpackState &unpack,
                                                      DynamicBuffer *stagingBufferOverride,
                                                      GLenum type,
                                                      const uint8_t *pixels,
                                                      const Format &vkFormat,
                                                      const GLuint inputRowPitch,
                                                      const GLuint inputDepthPitch,
                                                      const GLuint inputSkipBytes)
{
    const angle::Format &storageFormat = vkFormat.actualImageFormat();

    size_t outputRowPitch;
    size_t outputDepthPitch;
    size_t stencilAllocationSize = 0;
    uint32_t bufferRowLength;
    uint32_t bufferImageHeight;
    size_t allocationSize;

    LoadImageFunctionInfo loadFunctionInfo = vkFormat.textureLoadFunctions(type);
    LoadImageFunction stencilLoadFunction  = nullptr;

    if (storageFormat.isBlock)
    {
        const gl::InternalFormat &storageFormatInfo = vkFormat.getInternalFormatInfo(type);
        GLuint rowPitch;
        GLuint depthPitch;
        GLuint totalSize;

        ANGLE_VK_CHECK_MATH(contextVk, storageFormatInfo.computeCompressedImageSize(
                                           gl::Extents(glExtents.width, 1, 1), &rowPitch));
        ANGLE_VK_CHECK_MATH(contextVk,
                            storageFormatInfo.computeCompressedImageSize(
                                gl::Extents(glExtents.width, glExtents.height, 1), &depthPitch));

        ANGLE_VK_CHECK_MATH(contextVk,
                            storageFormatInfo.computeCompressedImageSize(glExtents, &totalSize));

        outputRowPitch   = rowPitch;
        outputDepthPitch = depthPitch;
        allocationSize   = totalSize;

        ANGLE_VK_CHECK_MATH(
            contextVk, storageFormatInfo.computeBufferRowLength(glExtents.width, &bufferRowLength));
        ANGLE_VK_CHECK_MATH(contextVk, storageFormatInfo.computeBufferImageHeight(
                                           glExtents.height, &bufferImageHeight));
    }
    else
    {
        ASSERT(storageFormat.pixelBytes != 0);

        if (storageFormat.id == angle::FormatID::D24_UNORM_S8_UINT)
        {
            stencilLoadFunction = angle::LoadX24S8ToS8;
        }
        if (storageFormat.id == angle::FormatID::D32_FLOAT_S8X24_UINT)
        {
            // If depth is D32FLOAT_S8, we must pack D32F tightly (no stencil) for CopyBufferToImage
            outputRowPitch = sizeof(float) * glExtents.width;

            // The generic load functions don't handle tightly packing D32FS8 to D32F & S8 so call
            // special case load functions.
            switch (type)
            {
                case GL_UNSIGNED_INT:
                    loadFunctionInfo.loadFunction = angle::LoadD32ToD32F;
                    stencilLoadFunction           = nullptr;
                    break;
                case GL_DEPTH32F_STENCIL8:
                case GL_FLOAT_32_UNSIGNED_INT_24_8_REV:
                    loadFunctionInfo.loadFunction = angle::LoadD32FS8X24ToD32F;
                    stencilLoadFunction           = angle::LoadX32S8ToS8;
                    break;
                case GL_UNSIGNED_INT_24_8_OES:
                    loadFunctionInfo.loadFunction = angle::LoadD24S8ToD32F;
                    stencilLoadFunction           = angle::LoadX24S8ToS8;
                    break;
                default:
                    UNREACHABLE();
            }
        }
        else
        {
            outputRowPitch = storageFormat.pixelBytes * glExtents.width;
        }
        outputDepthPitch = outputRowPitch * glExtents.height;

        bufferRowLength   = glExtents.width;
        bufferImageHeight = glExtents.height;

        allocationSize = outputDepthPitch * glExtents.depth;

        // Note: because the LoadImageFunctionInfo functions are limited to copying a single
        // component, we have to special case packed depth/stencil use and send the stencil as a
        // separate chunk.
        if (storageFormat.depthBits > 0 && storageFormat.stencilBits > 0 &&
            formatInfo.depthBits > 0 && formatInfo.stencilBits > 0)
        {
            // Note: Stencil is always one byte
            stencilAllocationSize = glExtents.width * glExtents.height * glExtents.depth;
            allocationSize += stencilAllocationSize;
        }
    }

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer    = nullptr;
    VkDeviceSize stagingOffset = 0;
    // If caller has provided a staging buffer, use it.
    DynamicBuffer *stagingBuffer = stagingBufferOverride ? stagingBufferOverride : &mStagingBuffer;
    size_t alignment             = mStagingBuffer.getAlignment();
    ANGLE_TRY(stagingBuffer->allocateWithAlignment(contextVk, allocationSize, alignment,
                                                   &stagingPointer, &bufferHandle, &stagingOffset,
                                                   nullptr));
    BufferHelper *currentBuffer = stagingBuffer->getCurrentBuffer();

    const uint8_t *source = pixels + static_cast<ptrdiff_t>(inputSkipBytes);

    loadFunctionInfo.loadFunction(glExtents.width, glExtents.height, glExtents.depth, source,
                                  inputRowPitch, inputDepthPitch, stagingPointer, outputRowPitch,
                                  outputDepthPitch);

    // YUV formats need special handling.
    if (vkFormat.actualImageFormat().isYUV)
    {
        gl::YuvFormatInfo yuvInfo(formatInfo.internalFormat, glExtents);

        constexpr VkImageAspectFlagBits kPlaneAspectFlags[3] = {
            VK_IMAGE_ASPECT_PLANE_0_BIT, VK_IMAGE_ASPECT_PLANE_1_BIT, VK_IMAGE_ASPECT_PLANE_2_BIT};

        // We only support mip level 0 and layerCount of 1 for YUV formats.
        ASSERT(index.getLevelIndex() == 0);
        ASSERT(index.getLayerCount() == 1);

        for (uint32_t plane = 0; plane < yuvInfo.planeCount; plane++)
        {
            VkBufferImageCopy copy           = {};
            copy.bufferOffset                = stagingOffset + yuvInfo.planeOffset[plane];
            copy.bufferRowLength             = 0;
            copy.bufferImageHeight           = 0;
            copy.imageSubresource.mipLevel   = 0;
            copy.imageSubresource.layerCount = 1;
            gl_vk::GetOffset(offset, &copy.imageOffset);
            gl_vk::GetExtent(yuvInfo.planeExtent[plane], &copy.imageExtent);
            copy.imageSubresource.baseArrayLayer = 0;
            copy.imageSubresource.aspectMask     = kPlaneAspectFlags[plane];
            appendSubresourceUpdate(gl::LevelIndex(0), SubresourceUpdate(currentBuffer, copy));
        }

        return angle::Result::Continue;
    }

    VkBufferImageCopy copy         = {};
    VkImageAspectFlags aspectFlags = GetFormatAspectFlags(vkFormat.actualImageFormat());

    copy.bufferOffset      = stagingOffset;
    copy.bufferRowLength   = bufferRowLength;
    copy.bufferImageHeight = bufferImageHeight;

    gl::LevelIndex updateLevelGL(index.getLevelIndex());
    copy.imageSubresource.mipLevel   = updateLevelGL.get();
    copy.imageSubresource.layerCount = index.getLayerCount();

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(glExtents, &copy.imageExtent);

    if (gl::IsArrayTextureType(index.getType()))
    {
        copy.imageSubresource.baseArrayLayer = offset.z;
        copy.imageOffset.z                   = 0;
        copy.imageExtent.depth               = 1;
    }
    else
    {
        copy.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
    }

    if (stencilAllocationSize > 0)
    {
        // Note: Stencil is always one byte
        ASSERT((aspectFlags & VK_IMAGE_ASPECT_STENCIL_BIT) != 0);

        // Skip over depth data.
        stagingPointer += outputDepthPitch * glExtents.depth;
        stagingOffset += outputDepthPitch * glExtents.depth;

        // recompute pitch for stencil data
        outputRowPitch   = glExtents.width;
        outputDepthPitch = outputRowPitch * glExtents.height;

        ASSERT(stencilLoadFunction != nullptr);
        stencilLoadFunction(glExtents.width, glExtents.height, glExtents.depth, source,
                            inputRowPitch, inputDepthPitch, stagingPointer, outputRowPitch,
                            outputDepthPitch);

        VkBufferImageCopy stencilCopy = {};

        stencilCopy.bufferOffset                    = stagingOffset;
        stencilCopy.bufferRowLength                 = bufferRowLength;
        stencilCopy.bufferImageHeight               = bufferImageHeight;
        stencilCopy.imageSubresource.mipLevel       = copy.imageSubresource.mipLevel;
        stencilCopy.imageSubresource.baseArrayLayer = copy.imageSubresource.baseArrayLayer;
        stencilCopy.imageSubresource.layerCount     = copy.imageSubresource.layerCount;
        stencilCopy.imageOffset                     = copy.imageOffset;
        stencilCopy.imageExtent                     = copy.imageExtent;
        stencilCopy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_STENCIL_BIT;
        appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(currentBuffer, stencilCopy));

        aspectFlags &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    if (HasBothDepthAndStencilAspects(aspectFlags))
    {
        // We still have both depth and stencil aspect bits set. That means we have a destination
        // buffer that is packed depth stencil and that the application is only loading one aspect.
        // Figure out which aspect the user is touching and remove the unused aspect bit.
        if (formatInfo.stencilBits > 0)
        {
            aspectFlags &= ~VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            aspectFlags &= ~VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    if (aspectFlags)
    {
        copy.imageSubresource.aspectMask = aspectFlags;
        appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(currentBuffer, copy));
    }

    return angle::Result::Continue;
}

angle::Result ImageHelper::CalculateBufferInfo(ContextVk *contextVk,
                                               const gl::Extents &glExtents,
                                               const gl::InternalFormat &formatInfo,
                                               const gl::PixelUnpackState &unpack,
                                               GLenum type,
                                               bool is3D,
                                               GLuint *inputRowPitch,
                                               GLuint *inputDepthPitch,
                                               GLuint *inputSkipBytes)
{
    // YUV formats need special handling.
    if (gl::IsYuvFormat(formatInfo.internalFormat))
    {
        gl::YuvFormatInfo yuvInfo(formatInfo.internalFormat, glExtents);

        // row pitch = Y plane row pitch
        *inputRowPitch = yuvInfo.planePitch[0];
        // depth pitch = Y plane size + chroma plane size
        *inputDepthPitch = yuvInfo.planeSize[0] + yuvInfo.planeSize[1] + yuvInfo.planeSize[2];
        *inputSkipBytes  = 0;

        return angle::Result::Continue;
    }

    ANGLE_VK_CHECK_MATH(contextVk,
                        formatInfo.computeRowPitch(type, glExtents.width, unpack.alignment,
                                                   unpack.rowLength, inputRowPitch));

    ANGLE_VK_CHECK_MATH(contextVk,
                        formatInfo.computeDepthPitch(glExtents.height, unpack.imageHeight,
                                                     *inputRowPitch, inputDepthPitch));

    ANGLE_VK_CHECK_MATH(
        contextVk, formatInfo.computeSkipBytes(type, *inputRowPitch, *inputDepthPitch, unpack, is3D,
                                               inputSkipBytes));

    return angle::Result::Continue;
}

bool ImageHelper::hasImmutableSampler() const
{
    return mExternalFormat != 0 || mFormat->actualImageFormat().isYUV;
}

void ImageHelper::onWrite(gl::LevelIndex levelStart,
                          uint32_t levelCount,
                          uint32_t layerStart,
                          uint32_t layerCount,
                          VkImageAspectFlags aspectFlags)
{
    mCurrentSingleClearValue.reset();

    // Mark contents of the given subresource as defined.
    setContentDefined(toVkLevel(levelStart), levelCount, layerStart, layerCount, aspectFlags);
}

bool ImageHelper::hasSubresourceDefinedContent(gl::LevelIndex level,
                                               uint32_t layerIndex,
                                               uint32_t layerCount) const
{
    if (layerIndex >= kMaxContentDefinedLayerCount)
    {
        return true;
    }

    uint8_t layerRangeBits =
        GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
    return (getLevelContentDefined(toVkLevel(level)) & LevelContentDefinedMask(layerRangeBits))
        .any();
}

bool ImageHelper::hasSubresourceDefinedStencilContent(gl::LevelIndex level,
                                                      uint32_t layerIndex,
                                                      uint32_t layerCount) const
{
    if (layerIndex >= kMaxContentDefinedLayerCount)
    {
        return true;
    }

    uint8_t layerRangeBits =
        GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
    return (getLevelStencilContentDefined(toVkLevel(level)) &
            LevelContentDefinedMask(layerRangeBits))
        .any();
}

void ImageHelper::invalidateSubresourceContent(ContextVk *contextVk,
                                               gl::LevelIndex level,
                                               uint32_t layerIndex,
                                               uint32_t layerCount)
{
    if (layerIndex < kMaxContentDefinedLayerCount)
    {
        uint8_t layerRangeBits =
            GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
        getLevelContentDefined(toVkLevel(level)) &= static_cast<uint8_t>(~layerRangeBits);
    }
    else
    {
        ANGLE_PERF_WARNING(
            contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
            "glInvalidateFramebuffer (color or depth) ineffective on attachments with layer >= 8");
    }
}

void ImageHelper::invalidateSubresourceStencilContent(ContextVk *contextVk,
                                                      gl::LevelIndex level,
                                                      uint32_t layerIndex,
                                                      uint32_t layerCount)
{
    if (layerIndex < kMaxContentDefinedLayerCount)
    {
        uint8_t layerRangeBits =
            GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
        getLevelStencilContentDefined(toVkLevel(level)) &= static_cast<uint8_t>(~layerRangeBits);
    }
    else
    {
        ANGLE_PERF_WARNING(
            contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
            "glInvalidateFramebuffer (stencil) ineffective on attachments with layer >= 8");
    }
}

void ImageHelper::restoreSubresourceContent(gl::LevelIndex level,
                                            uint32_t layerIndex,
                                            uint32_t layerCount)
{
    if (layerIndex < kMaxContentDefinedLayerCount)
    {
        uint8_t layerRangeBits =
            GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
        getLevelContentDefined(toVkLevel(level)) |= layerRangeBits;
    }
}

void ImageHelper::restoreSubresourceStencilContent(gl::LevelIndex level,
                                                   uint32_t layerIndex,
                                                   uint32_t layerCount)
{
    if (layerIndex < kMaxContentDefinedLayerCount)
    {
        uint8_t layerRangeBits =
            GetContentDefinedLayerRangeBits(layerIndex, layerCount, kMaxContentDefinedLayerCount);
        getLevelStencilContentDefined(toVkLevel(level)) |= layerRangeBits;
    }
}

angle::Result ImageHelper::stageSubresourceUpdate(ContextVk *contextVk,
                                                  const gl::ImageIndex &index,
                                                  const gl::Extents &glExtents,
                                                  const gl::Offset &offset,
                                                  const gl::InternalFormat &formatInfo,
                                                  const gl::PixelUnpackState &unpack,
                                                  DynamicBuffer *stagingBufferOverride,
                                                  GLenum type,
                                                  const uint8_t *pixels,
                                                  const Format &vkFormat)
{
    GLuint inputRowPitch   = 0;
    GLuint inputDepthPitch = 0;
    GLuint inputSkipBytes  = 0;
    ANGLE_TRY(CalculateBufferInfo(contextVk, glExtents, formatInfo, unpack, type, index.usesTex3D(),
                                  &inputRowPitch, &inputDepthPitch, &inputSkipBytes));

    ANGLE_TRY(stageSubresourceUpdateImpl(contextVk, index, glExtents, offset, formatInfo, unpack,
                                         stagingBufferOverride, type, pixels, vkFormat,
                                         inputRowPitch, inputDepthPitch, inputSkipBytes));

    return angle::Result::Continue;
}

angle::Result ImageHelper::stageSubresourceUpdateAndGetData(ContextVk *contextVk,
                                                            size_t allocationSize,
                                                            const gl::ImageIndex &imageIndex,
                                                            const gl::Extents &glExtents,
                                                            const gl::Offset &offset,
                                                            uint8_t **destData,
                                                            DynamicBuffer *stagingBufferOverride)
{
    VkBuffer bufferHandle;
    VkDeviceSize stagingOffset = 0;

    DynamicBuffer *stagingBuffer = stagingBufferOverride ? stagingBufferOverride : &mStagingBuffer;
    size_t alignment             = mStagingBuffer.getAlignment();
    ANGLE_TRY(stagingBuffer->allocateWithAlignment(contextVk, allocationSize, alignment, destData,
                                                   &bufferHandle, &stagingOffset, nullptr));

    gl::LevelIndex updateLevelGL(imageIndex.getLevelIndex());

    VkBufferImageCopy copy               = {};
    copy.bufferOffset                    = stagingOffset;
    copy.bufferRowLength                 = glExtents.width;
    copy.bufferImageHeight               = glExtents.height;
    copy.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel       = updateLevelGL.get();
    copy.imageSubresource.baseArrayLayer = imageIndex.hasLayer() ? imageIndex.getLayerIndex() : 0;
    copy.imageSubresource.layerCount     = imageIndex.getLayerCount();

    // Note: Only support color now
    ASSERT((mFormat == nullptr) || (getAspectFlags() == VK_IMAGE_ASPECT_COLOR_BIT));

    gl_vk::GetOffset(offset, &copy.imageOffset);
    gl_vk::GetExtent(glExtents, &copy.imageExtent);

    appendSubresourceUpdate(updateLevelGL,
                            SubresourceUpdate(stagingBuffer->getCurrentBuffer(), copy));

    return angle::Result::Continue;
}

angle::Result ImageHelper::stageSubresourceUpdateFromFramebuffer(
    const gl::Context *context,
    const gl::ImageIndex &index,
    const gl::Rectangle &sourceArea,
    const gl::Offset &dstOffset,
    const gl::Extents &dstExtent,
    const gl::InternalFormat &formatInfo,
    FramebufferVk *framebufferVk,
    DynamicBuffer *stagingBufferOverride)
{
    ContextVk *contextVk = GetImpl(context);

    // If the extents and offset is outside the source image, we need to clip.
    gl::Rectangle clippedRectangle;
    const gl::Extents readExtents = framebufferVk->getReadImageExtents();
    if (!ClipRectangle(sourceArea, gl::Rectangle(0, 0, readExtents.width, readExtents.height),
                       &clippedRectangle))
    {
        // Empty source area, nothing to do.
        return angle::Result::Continue;
    }

    bool isViewportFlipEnabled = contextVk->isViewportFlipEnabledForDrawFBO();
    if (isViewportFlipEnabled)
    {
        clippedRectangle.y = readExtents.height - clippedRectangle.y - clippedRectangle.height;
    }

    // 1- obtain a buffer handle to copy to
    RendererVk *renderer = contextVk->getRenderer();

    const Format &vkFormat             = renderer->getFormat(formatInfo.sizedInternalFormat);
    const angle::Format &storageFormat = vkFormat.actualImageFormat();
    LoadImageFunctionInfo loadFunction = vkFormat.textureLoadFunctions(formatInfo.type);

    size_t outputRowPitch   = storageFormat.pixelBytes * clippedRectangle.width;
    size_t outputDepthPitch = outputRowPitch * clippedRectangle.height;

    VkBuffer bufferHandle = VK_NULL_HANDLE;

    uint8_t *stagingPointer    = nullptr;
    VkDeviceSize stagingOffset = 0;

    // The destination is only one layer deep.
    size_t allocationSize        = outputDepthPitch;
    DynamicBuffer *stagingBuffer = stagingBufferOverride ? stagingBufferOverride : &mStagingBuffer;
    size_t alignment             = mStagingBuffer.getAlignment();
    ANGLE_TRY(stagingBuffer->allocateWithAlignment(contextVk, allocationSize, alignment,
                                                   &stagingPointer, &bufferHandle, &stagingOffset,
                                                   nullptr));
    BufferHelper *currentBuffer = stagingBuffer->getCurrentBuffer();

    const angle::Format &copyFormat =
        GetFormatFromFormatType(formatInfo.internalFormat, formatInfo.type);
    PackPixelsParams params(clippedRectangle, copyFormat, static_cast<GLuint>(outputRowPitch),
                            isViewportFlipEnabled, nullptr, 0);

    RenderTargetVk *readRenderTarget = framebufferVk->getColorReadRenderTarget();

    // 2- copy the source image region to the pixel buffer using a cpu readback
    if (loadFunction.requiresConversion)
    {
        // When a conversion is required, we need to use the loadFunction to read from a temporary
        // buffer instead so its an even slower path.
        size_t bufferSize =
            storageFormat.pixelBytes * clippedRectangle.width * clippedRectangle.height;
        angle::MemoryBuffer *memoryBuffer = nullptr;
        ANGLE_VK_CHECK_ALLOC(contextVk, context->getScratchBuffer(bufferSize, &memoryBuffer));

        // Read into the scratch buffer
        ANGLE_TRY(framebufferVk->readPixelsImpl(contextVk, clippedRectangle, params,
                                                VK_IMAGE_ASPECT_COLOR_BIT, readRenderTarget,
                                                memoryBuffer->data()));

        // Load from scratch buffer to our pixel buffer
        loadFunction.loadFunction(clippedRectangle.width, clippedRectangle.height, 1,
                                  memoryBuffer->data(), outputRowPitch, 0, stagingPointer,
                                  outputRowPitch, 0);
    }
    else
    {
        // We read directly from the framebuffer into our pixel buffer.
        ANGLE_TRY(framebufferVk->readPixelsImpl(contextVk, clippedRectangle, params,
                                                VK_IMAGE_ASPECT_COLOR_BIT, readRenderTarget,
                                                stagingPointer));
    }

    gl::LevelIndex updateLevelGL(index.getLevelIndex());

    // 3- enqueue the destination image subresource update
    VkBufferImageCopy copyToImage               = {};
    copyToImage.bufferOffset                    = static_cast<VkDeviceSize>(stagingOffset);
    copyToImage.bufferRowLength                 = 0;  // Tightly packed data can be specified as 0.
    copyToImage.bufferImageHeight               = clippedRectangle.height;
    copyToImage.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToImage.imageSubresource.mipLevel       = updateLevelGL.get();
    copyToImage.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
    copyToImage.imageSubresource.layerCount     = index.getLayerCount();
    gl_vk::GetOffset(dstOffset, &copyToImage.imageOffset);
    gl_vk::GetExtent(dstExtent, &copyToImage.imageExtent);

    // 3- enqueue the destination image subresource update
    appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(currentBuffer, copyToImage));
    return angle::Result::Continue;
}

void ImageHelper::stageSubresourceUpdateFromImage(RefCounted<ImageHelper> *image,
                                                  const gl::ImageIndex &index,
                                                  LevelIndex srcMipLevel,
                                                  const gl::Offset &destOffset,
                                                  const gl::Extents &glExtents,
                                                  const VkImageType imageType)
{
    gl::LevelIndex updateLevelGL(index.getLevelIndex());

    VkImageCopy copyToImage               = {};
    copyToImage.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToImage.srcSubresource.mipLevel   = srcMipLevel.get();
    copyToImage.srcSubresource.layerCount = index.getLayerCount();
    copyToImage.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copyToImage.dstSubresource.mipLevel   = updateLevelGL.get();

    if (imageType == VK_IMAGE_TYPE_3D)
    {
        // These values must be set explicitly to follow the Vulkan spec:
        // https://www.khronos.org/registry/vulkan/specs/1.1-extensions/man/html/VkImageCopy.html
        // If either of the calling command's srcImage or dstImage parameters are of VkImageType
        // VK_IMAGE_TYPE_3D, the baseArrayLayer and layerCount members of the corresponding
        // subresource must be 0 and 1, respectively
        copyToImage.dstSubresource.baseArrayLayer = 0;
        copyToImage.dstSubresource.layerCount     = 1;
        // Preserve the assumption that destOffset.z == "dstSubresource.baseArrayLayer"
        ASSERT(destOffset.z == (index.hasLayer() ? index.getLayerIndex() : 0));
    }
    else
    {
        copyToImage.dstSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
        copyToImage.dstSubresource.layerCount     = index.getLayerCount();
    }

    gl_vk::GetOffset(destOffset, &copyToImage.dstOffset);
    gl_vk::GetExtent(glExtents, &copyToImage.extent);

    appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(image, copyToImage));
}

void ImageHelper::stageSubresourceUpdatesFromAllImageLevels(RefCounted<ImageHelper> *image,
                                                            gl::LevelIndex baseLevel)
{
    for (LevelIndex levelVk(0); levelVk < LevelIndex(image->get().getLevelCount()); ++levelVk)
    {
        const gl::LevelIndex levelGL = vk_gl::GetLevelIndex(levelVk, baseLevel);
        const gl::ImageIndex index =
            gl::ImageIndex::Make2DArrayRange(levelGL.get(), 0, image->get().getLayerCount());

        stageSubresourceUpdateFromImage(image, index, levelVk, gl::kOffsetZero,
                                        image->get().getLevelExtents(levelVk),
                                        image->get().getType());
    }
}

void ImageHelper::stageClear(const gl::ImageIndex &index,
                             VkImageAspectFlags aspectFlags,
                             const VkClearValue &clearValue)
{
    gl::LevelIndex updateLevelGL(index.getLevelIndex());
    appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(aspectFlags, clearValue, index));
}

void ImageHelper::stageRobustResourceClear(const gl::ImageIndex &index)
{
    const VkImageAspectFlags aspectFlags = getAspectFlags();

    ASSERT(mFormat);
    VkClearValue clearValue = GetRobustResourceClearValue(*mFormat);

    gl::LevelIndex updateLevelGL(index.getLevelIndex());
    appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(aspectFlags, clearValue, index));
}

angle::Result ImageHelper::stageRobustResourceClearWithFormat(ContextVk *contextVk,
                                                              const gl::ImageIndex &index,
                                                              const gl::Extents &glExtents,
                                                              const Format &format)
{
    const angle::Format &imageFormat     = format.actualImageFormat();
    const VkImageAspectFlags aspectFlags = GetFormatAspectFlags(imageFormat);

    // Robust clears must only be staged if we do not have any prior data for this subresource.
    ASSERT(!hasStagedUpdatesForSubresource(gl::LevelIndex(index.getLevelIndex()),
                                           index.getLayerIndex(), index.getLayerCount()));

    VkClearValue clearValue = GetRobustResourceClearValue(format);

    gl::LevelIndex updateLevelGL(index.getLevelIndex());

    if (imageFormat.isBlock)
    {
        // This only supports doing an initial clear to 0, not clearing to a specific encoded RGBA
        // value
        ASSERT((clearValue.color.int32[0] == 0) && (clearValue.color.int32[1] == 0) &&
               (clearValue.color.int32[2] == 0) && (clearValue.color.int32[3] == 0));

        const gl::InternalFormat &formatInfo =
            gl::GetSizedInternalFormatInfo(imageFormat.glInternalFormat);
        GLuint totalSize;
        ANGLE_VK_CHECK_MATH(contextVk,
                            formatInfo.computeCompressedImageSize(glExtents, &totalSize));

        VkBuffer bufferHandle      = VK_NULL_HANDLE;
        uint8_t *stagingPointer    = nullptr;
        VkDeviceSize stagingOffset = 0;
        ANGLE_TRY(mStagingBuffer.allocate(contextVk, totalSize, &stagingPointer, &bufferHandle,
                                          &stagingOffset, nullptr));
        memset(stagingPointer, 0, totalSize);

        VkBufferImageCopy copyRegion               = {};
        copyRegion.imageExtent.width               = glExtents.width;
        copyRegion.imageExtent.height              = glExtents.height;
        copyRegion.imageExtent.depth               = glExtents.depth;
        copyRegion.imageSubresource.mipLevel       = updateLevelGL.get();
        copyRegion.imageSubresource.aspectMask     = aspectFlags;
        copyRegion.imageSubresource.baseArrayLayer = index.hasLayer() ? index.getLayerIndex() : 0;
        copyRegion.imageSubresource.layerCount     = index.getLayerCount();

        appendSubresourceUpdate(updateLevelGL,
                                SubresourceUpdate(mStagingBuffer.getCurrentBuffer(), copyRegion));
    }
    else
    {
        appendSubresourceUpdate(updateLevelGL, SubresourceUpdate(aspectFlags, clearValue, index));
    }

    return angle::Result::Continue;
}

void ImageHelper::stageClearIfEmulatedFormat(bool isRobustResourceInitEnabled)
{
    // Skip staging extra clears if robust resource init is enabled.
    if (!mFormat->hasEmulatedImageChannels() || isRobustResourceInitEnabled)
    {
        return;
    }

    VkClearValue clearValue = {};
    if (mFormat->intendedFormat().hasDepthOrStencilBits())
    {
        clearValue.depthStencil = kRobustInitDepthStencilValue;
    }
    else
    {
        clearValue.color = kEmulatedInitColorValue;
    }

    const VkImageAspectFlags aspectFlags = getAspectFlags();

    // If the image has an emulated channel and robust resource init is not enabled, always clear
    // it. These channels will be masked out in future writes, and shouldn't contain uninitialized
    // values.
    for (LevelIndex level(0); level < LevelIndex(mLevelCount); ++level)
    {
        gl::LevelIndex updateLevelGL = toGLLevel(level);
        gl::ImageIndex index =
            gl::ImageIndex::Make2DArrayRange(updateLevelGL.get(), 0, mLayerCount);
        prependSubresourceUpdate(updateLevelGL, SubresourceUpdate(aspectFlags, clearValue, index));
    }
}

void ImageHelper::stageSelfAsSubresourceUpdates(ContextVk *contextVk,
                                                uint32_t levelCount,
                                                gl::TexLevelMask skipLevelsMask)

{
    // Nothing to do if every level must be skipped
    if ((~skipLevelsMask & gl::TexLevelMask(angle::BitMask<uint32_t>(levelCount))).none())
    {
        return;
    }

    // Because we are cloning this object to another object, we must finalize the layout if it is
    // being used by current renderpass as attachment. Otherwise we are copying the incorrect layout
    // since it is determined at endRenderPass time.
    contextVk->finalizeImageLayout(this);

    std::unique_ptr<RefCounted<ImageHelper>> prevImage =
        std::make_unique<RefCounted<ImageHelper>>();

    // Move the necessary information for staged update to work, and keep the rest as part of this
    // object.

    // Vulkan objects
    prevImage->get().mImage        = std::move(mImage);
    prevImage->get().mDeviceMemory = std::move(mDeviceMemory);

    // Barrier information.  Note: mLevelCount is set to levelCount so that only the necessary
    // levels are transitioned when flushing the update.
    prevImage->get().mFormat                      = mFormat;
    prevImage->get().mCurrentLayout               = mCurrentLayout;
    prevImage->get().mCurrentQueueFamilyIndex     = mCurrentQueueFamilyIndex;
    prevImage->get().mLastNonShaderReadOnlyLayout = mLastNonShaderReadOnlyLayout;
    prevImage->get().mCurrentShaderReadStageMask  = mCurrentShaderReadStageMask;
    prevImage->get().mLevelCount                  = levelCount;
    prevImage->get().mLayerCount                  = mLayerCount;
    prevImage->get().mImageSerial                 = mImageSerial;

    // Reset information for current (invalid) image.
    mCurrentLayout               = ImageLayout::Undefined;
    mCurrentQueueFamilyIndex     = std::numeric_limits<uint32_t>::max();
    mLastNonShaderReadOnlyLayout = ImageLayout::Undefined;
    mCurrentShaderReadStageMask  = 0;
    mImageSerial                 = kInvalidImageSerial;

    setEntireContentUndefined();

    // Stage updates from the previous image.
    for (LevelIndex levelVk(0); levelVk < LevelIndex(levelCount); ++levelVk)
    {
        if (skipLevelsMask.test(levelVk.get()))
        {
            continue;
        }

        const gl::ImageIndex index =
            gl::ImageIndex::Make2DArrayRange(toGLLevel(levelVk).get(), 0, mLayerCount);

        stageSubresourceUpdateFromImage(prevImage.get(), index, levelVk, gl::kOffsetZero,
                                        getLevelExtents(levelVk), mImageType);
    }

    ASSERT(levelCount > 0);
    prevImage.release();
}

angle::Result ImageHelper::flushSingleSubresourceStagedUpdates(ContextVk *contextVk,
                                                               gl::LevelIndex levelGL,
                                                               uint32_t layer,
                                                               uint32_t layerCount,
                                                               ClearValuesArray *deferredClears,
                                                               uint32_t deferredClearIndex)
{
    std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(levelGL);
    if (levelUpdates == nullptr || levelUpdates->empty())
    {
        return angle::Result::Continue;
    }

    LevelIndex levelVk = toVkLevel(levelGL);

    // Handle deferred clears. Search the updates list for a matching clear index.
    if (deferredClears)
    {
        Optional<size_t> foundClear;

        for (size_t updateIndex = 0; updateIndex < levelUpdates->size(); ++updateIndex)
        {
            SubresourceUpdate &update = (*levelUpdates)[updateIndex];

            if (update.isUpdateToLayers(layer, layerCount))
            {
                // On any data update, exit out. We'll need to do a full upload.
                const bool isClear              = update.updateSource == UpdateSource::Clear;
                const uint32_t updateLayerCount = isClear ? update.data.clear.layerCount : 0;
                const uint32_t imageLayerCount =
                    mImageType == VK_IMAGE_TYPE_3D ? getLevelExtents(levelVk).depth : mLayerCount;

                if (!isClear || (updateLayerCount != layerCount &&
                                 !(update.data.clear.layerCount == VK_REMAINING_ARRAY_LAYERS &&
                                   imageLayerCount == layerCount)))
                {
                    foundClear.reset();
                    break;
                }

                // Otherwise track the latest clear update index.
                foundClear = updateIndex;
            }
        }

        // If we have a valid index we defer the clear using the clear reference.
        if (foundClear.valid())
        {
            size_t foundIndex         = foundClear.value();
            const ClearUpdate &update = (*levelUpdates)[foundIndex].data.clear;

            // Note that this set command handles combined or separate depth/stencil clears.
            deferredClears->store(deferredClearIndex, update.aspectFlags, update.value);

            // Do not call onWrite as it removes mCurrentSingleClearValue, but instead call
            // setContentDefined directly.
            setContentDefined(toVkLevel(levelGL), 1, layer, layerCount, update.aspectFlags);

            // We process the updates again to erase any clears for this level.
            removeSingleSubresourceStagedUpdates(contextVk, levelGL, layer, layerCount);
            return angle::Result::Continue;
        }

        // Otherwise we proceed with a normal update.
    }

    return flushStagedUpdates(contextVk, levelGL, levelGL + 1, layer, layer + layerCount, {});
}

angle::Result ImageHelper::flushStagedUpdates(ContextVk *contextVk,
                                              gl::LevelIndex levelGLStart,
                                              gl::LevelIndex levelGLEnd,
                                              uint32_t layerStart,
                                              uint32_t layerEnd,
                                              gl::TexLevelMask skipLevelsMask)
{
    if (!hasStagedUpdatesInLevels(levelGLStart, levelGLEnd))
    {
        return angle::Result::Continue;
    }

    removeSupersededUpdates(contextVk, skipLevelsMask);

    // If a clear is requested and we know it was previously cleared with the same value, we drop
    // the clear.
    if (mCurrentSingleClearValue.valid())
    {
        std::vector<SubresourceUpdate> *levelUpdates =
            getLevelUpdates(gl::LevelIndex(mCurrentSingleClearValue.value().levelIndex));
        if (levelUpdates && levelUpdates->size() == 1)
        {
            SubresourceUpdate &update = (*levelUpdates)[0];
            if (update.updateSource == UpdateSource::Clear &&
                mCurrentSingleClearValue.value() == update.data.clear)
            {
                ANGLE_PERF_WARNING(contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
                                   "Repeated Clear on framebuffer attachment dropped");
                update.release(contextVk->getRenderer());
                levelUpdates->clear();
                return angle::Result::Continue;
            }
        }
    }

    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    ANGLE_TRY(mStagingBuffer.flush(contextVk));

    const VkImageAspectFlags aspectFlags = GetFormatAspectFlags(mFormat->actualImageFormat());

    // For each level, upload layers that don't conflict in parallel.  The layer is hashed to
    // `layer % 64` and used to track whether that subresource is currently in transfer.  If so, a
    // barrier is inserted.  If mLayerCount > 64, there will be a few unnecessary barriers.
    //
    // Note: when a barrier is necessary when uploading updates to a level, we could instead move to
    // the next level and continue uploads in parallel.  Once all levels need a barrier, a single
    // barrier can be issued and we could continue with the rest of the updates from the first
    // level.
    constexpr uint32_t kMaxParallelSubresourceUpload = 64;

    // Start in TransferDst.  Don't yet mark any subresource as having defined contents; that is
    // done with fine granularity as updates are applied.  This is achieved by specifying a layer
    // that is outside the tracking range.
    CommandBufferAccess access;
    access.onImageTransferWrite(levelGLStart, 1, kMaxContentDefinedLayerCount, 0, aspectFlags,
                                this);

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    for (gl::LevelIndex updateMipLevelGL = levelGLStart; updateMipLevelGL < levelGLEnd;
         ++updateMipLevelGL)
    {
        std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(updateMipLevelGL);
        if (levelUpdates == nullptr)
        {
            ASSERT(static_cast<size_t>(updateMipLevelGL.get()) >= mSubresourceUpdates.size());
            break;
        }

        std::vector<SubresourceUpdate> updatesToKeep;

        // Hash map of uploads in progress.  See comment on kMaxParallelSubresourceUpload.
        uint64_t subresourceUploadsInProgress = 0;

        for (SubresourceUpdate &update : *levelUpdates)
        {
            ASSERT(update.updateSource == UpdateSource::Clear ||
                   (update.updateSource == UpdateSource::Buffer &&
                    update.data.buffer.bufferHelper != nullptr) ||
                   (update.updateSource == UpdateSource::Image && update.image != nullptr &&
                    update.image->isReferenced() && update.image->get().valid()));

            uint32_t updateBaseLayer, updateLayerCount;
            update.getDestSubresource(mLayerCount, &updateBaseLayer, &updateLayerCount);

            // If the update layers don't intersect the requested layers, skip the update.
            const bool areUpdateLayersOutsideRange =
                updateBaseLayer + updateLayerCount <= layerStart || updateBaseLayer >= layerEnd;

            const LevelIndex updateMipLevelVk = toVkLevel(updateMipLevelGL);

            // Additionally, if updates to this level are specifically asked to be skipped, skip
            // them. This can happen when recreating an image that has been partially incompatibly
            // redefined, in which case only updates to the levels that haven't been redefined
            // should be flushed.
            if (areUpdateLayersOutsideRange || skipLevelsMask.test(updateMipLevelVk.get()))
            {
                updatesToKeep.emplace_back(std::move(update));
                continue;
            }

            // The updates were holding gl::LevelIndex values so that they would not need
            // modification when the base level of the texture changes.  Now that the update is
            // about to take effect, we need to change miplevel to LevelIndex.
            if (update.updateSource == UpdateSource::Clear)
            {
                update.data.clear.levelIndex = updateMipLevelVk.get();
            }
            else if (update.updateSource == UpdateSource::Buffer)
            {
                update.data.buffer.copyRegion.imageSubresource.mipLevel = updateMipLevelVk.get();
            }
            else if (update.updateSource == UpdateSource::Image)
            {
                update.data.image.copyRegion.dstSubresource.mipLevel = updateMipLevelVk.get();
            }

            if (updateLayerCount >= kMaxParallelSubresourceUpload)
            {
                // If there are more subresources than bits we can track, always insert a barrier.
                recordWriteBarrier(contextVk, aspectFlags, ImageLayout::TransferDst, commandBuffer);
                subresourceUploadsInProgress = std::numeric_limits<uint64_t>::max();
            }
            else
            {
                const uint64_t subresourceHashRange = angle::BitMask<uint64_t>(updateLayerCount);
                const uint32_t subresourceHashOffset =
                    updateBaseLayer % kMaxParallelSubresourceUpload;
                const uint64_t subresourceHash =
                    ANGLE_ROTL64(subresourceHashRange, subresourceHashOffset);

                if ((subresourceUploadsInProgress & subresourceHash) != 0)
                {
                    // If there's overlap in subresource upload, issue a barrier.
                    recordWriteBarrier(contextVk, aspectFlags, ImageLayout::TransferDst,
                                       commandBuffer);
                    subresourceUploadsInProgress = 0;
                }
                subresourceUploadsInProgress |= subresourceHash;
            }

            if (update.updateSource == UpdateSource::Clear)
            {
                clear(update.data.clear.aspectFlags, update.data.clear.value, updateMipLevelVk,
                      updateBaseLayer, updateLayerCount, commandBuffer);
                // Remember the latest operation is a clear call
                mCurrentSingleClearValue = update.data.clear;

                // Do not call onWrite as it removes mCurrentSingleClearValue, but instead call
                // setContentDefined directly.
                setContentDefined(updateMipLevelVk, 1, updateBaseLayer, updateLayerCount,
                                  update.data.clear.aspectFlags);
            }
            else if (update.updateSource == UpdateSource::Buffer)
            {
                BufferUpdate &bufferUpdate = update.data.buffer;

                BufferHelper *currentBuffer = bufferUpdate.bufferHelper;
                ASSERT(currentBuffer && currentBuffer->valid());

                CommandBufferAccess bufferAccess;
                bufferAccess.onBufferTransferRead(currentBuffer);
                ANGLE_TRY(
                    contextVk->getOutsideRenderPassCommandBuffer(bufferAccess, &commandBuffer));

                commandBuffer->copyBufferToImage(currentBuffer->getBuffer().getHandle(), mImage,
                                                 getCurrentLayout(), 1,
                                                 &update.data.buffer.copyRegion);
                onWrite(updateMipLevelGL, 1, updateBaseLayer, updateLayerCount,
                        update.data.buffer.copyRegion.imageSubresource.aspectMask);
            }
            else
            {
                CommandBufferAccess imageAccess;
                imageAccess.onImageTransferRead(aspectFlags, &update.image->get());
                ANGLE_TRY(
                    contextVk->getOutsideRenderPassCommandBuffer(imageAccess, &commandBuffer));

                commandBuffer->copyImage(update.image->get().getImage(),
                                         update.image->get().getCurrentLayout(), mImage,
                                         getCurrentLayout(), 1, &update.data.image.copyRegion);
                onWrite(updateMipLevelGL, 1, updateBaseLayer, updateLayerCount,
                        update.data.image.copyRegion.dstSubresource.aspectMask);
            }

            update.release(contextVk->getRenderer());
        }

        // Only remove the updates that were actually applied to the image.
        *levelUpdates = std::move(updatesToKeep);
    }

    // Compact mSubresourceUpdates, then check if there are any updates left.
    size_t compactSize;
    for (compactSize = mSubresourceUpdates.size(); compactSize > 0; --compactSize)
    {
        if (!mSubresourceUpdates[compactSize - 1].empty())
        {
            break;
        }
    }
    mSubresourceUpdates.resize(compactSize);

    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    // If no updates left, release the staging buffers to save memory.
    if (mSubresourceUpdates.empty())
    {
        mStagingBuffer.releaseInFlightBuffers(contextVk);
        mStagingBuffer.release(contextVk->getRenderer());
    }

    return angle::Result::Continue;
}

angle::Result ImageHelper::flushAllStagedUpdates(ContextVk *contextVk)
{
    return flushStagedUpdates(contextVk, mFirstAllocatedLevel, mFirstAllocatedLevel + mLevelCount,
                              0, mLayerCount, {});
}

bool ImageHelper::hasStagedUpdatesForSubresource(gl::LevelIndex levelGL,
                                                 uint32_t layer,
                                                 uint32_t layerCount) const
{
    // Check to see if any updates are staged for the given level and layer

    const std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(levelGL);
    if (levelUpdates == nullptr || levelUpdates->empty())
    {
        return false;
    }

    for (const SubresourceUpdate &update : *levelUpdates)
    {
        uint32_t updateBaseLayer, updateLayerCount;
        update.getDestSubresource(mLayerCount, &updateBaseLayer, &updateLayerCount);

        const uint32_t updateLayerEnd = updateBaseLayer + updateLayerCount;
        const uint32_t layerEnd       = layer + layerCount;

        if ((layer >= updateBaseLayer && layer < updateLayerEnd) ||
            (layerEnd > updateBaseLayer && layerEnd <= updateLayerEnd))
        {
            // The layers intersect with the update range
            return true;
        }
    }

    return false;
}

gl::LevelIndex ImageHelper::getLastAllocatedLevel() const
{
    return mFirstAllocatedLevel + mLevelCount - 1;
}

bool ImageHelper::hasStagedUpdatesInAllocatedLevels() const
{
    return hasStagedUpdatesInLevels(mFirstAllocatedLevel, getLastAllocatedLevel() + 1);
}

bool ImageHelper::hasStagedUpdatesInLevels(gl::LevelIndex levelStart, gl::LevelIndex levelEnd) const
{
    for (gl::LevelIndex level = levelStart; level < levelEnd; ++level)
    {
        const std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(level);
        if (levelUpdates == nullptr)
        {
            ASSERT(static_cast<size_t>(level.get()) >= mSubresourceUpdates.size());
            return false;
        }

        if (!levelUpdates->empty())
        {
            return true;
        }
    }
    return false;
}

bool ImageHelper::validateSubresourceUpdateImageRefConsistent(RefCounted<ImageHelper> *image) const
{
    if (image == nullptr)
    {
        return true;
    }

    uint32_t refs = 0;

    for (const std::vector<SubresourceUpdate> &levelUpdates : mSubresourceUpdates)
    {
        for (const SubresourceUpdate &update : levelUpdates)
        {
            if (update.updateSource == UpdateSource::Image && update.image == image)
            {
                ++refs;
            }
        }
    }

    return image->isRefCountAsExpected(refs);
}

bool ImageHelper::validateSubresourceUpdateImageRefsConsistent() const
{
    for (const std::vector<SubresourceUpdate> &levelUpdates : mSubresourceUpdates)
    {
        for (const SubresourceUpdate &update : levelUpdates)
        {
            if (update.updateSource == UpdateSource::Image &&
                !validateSubresourceUpdateImageRefConsistent(update.image))
            {
                return false;
            }
        }
    }

    return true;
}

void ImageHelper::removeSupersededUpdates(ContextVk *contextVk, gl::TexLevelMask skipLevelsMask)
{
    if (mLayerCount > 64)
    {
        // Not implemented for images with more than 64 layers.  A 64-bit mask is used for
        // efficiency, hence the limit.
        return;
    }

    ASSERT(validateSubresourceUpdateImageRefsConsistent());

    RendererVk *renderer = contextVk->getRenderer();

    // Go over updates in reverse order, and mark the layers they completely overwrite.  If an
    // update is encountered whose layers are all already marked, that update is superseded by
    // future updates, so it can be dropped.  This tracking is done per level.  If the aspect being
    // written to is color/depth or stencil, index 0 or 1 is used respectively.  This is so
    // that if a depth write for example covers the whole subresource, a stencil write to that same
    // subresource is not dropped.
    constexpr size_t kIndexColorOrDepth = 0;
    constexpr size_t kIndexStencil      = 1;
    uint64_t supersededLayers[2]        = {};

    gl::Extents levelExtents = {};

    // Note: this lambda only needs |this|, but = is specified because clang warns about kIndex* not
    // needing capture, while MSVC fails to compile without capturing them.
    auto markLayersAndDropSuperseded = [=, &supersededLayers,
                                        &levelExtents](SubresourceUpdate &update) {
        uint32_t updateBaseLayer, updateLayerCount;
        update.getDestSubresource(mLayerCount, &updateBaseLayer, &updateLayerCount);

        const VkImageAspectFlags aspectMask = update.getDestAspectFlags();
        const bool hasColorOrDepth =
            (aspectMask & (VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_PLANE_0_BIT |
                           VK_IMAGE_ASPECT_PLANE_1_BIT | VK_IMAGE_ASPECT_PLANE_2_BIT |
                           VK_IMAGE_ASPECT_DEPTH_BIT)) != 0;
        const bool hasStencil = (aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;

        // Test if the update is to layers that are all superseded.  In that case, drop the update.
        ASSERT(updateLayerCount <= 64);
        uint64_t updateLayersMask = updateLayerCount >= 64
                                        ? ~static_cast<uint64_t>(0)
                                        : angle::BitMask<uint64_t>(updateLayerCount);
        updateLayersMask <<= updateBaseLayer;

        const bool isColorOrDepthSuperseded =
            !hasColorOrDepth ||
            (supersededLayers[kIndexColorOrDepth] & updateLayersMask) == updateLayersMask;
        const bool isStencilSuperseded =
            !hasStencil || (supersededLayers[kIndexStencil] & updateLayersMask) == updateLayersMask;

        if (isColorOrDepthSuperseded && isStencilSuperseded)
        {
            ANGLE_PERF_WARNING(contextVk->getDebug(), GL_DEBUG_SEVERITY_LOW,
                               "Dropped image update that is superseded by an overlapping one");

            update.release(renderer);
            return true;
        }

        // Get the area this update affects.  Note that clear updates always clear the whole
        // subresource.
        gl::Box updateBox(gl::kOffsetZero, levelExtents);

        if (update.updateSource == UpdateSource::Buffer)
        {
            updateBox = gl::Box(update.data.buffer.copyRegion.imageOffset,
                                update.data.buffer.copyRegion.imageExtent);
        }
        else if (update.updateSource == UpdateSource::Image)
        {
            updateBox = gl::Box(update.data.image.copyRegion.dstOffset,
                                update.data.image.copyRegion.extent);
        }

        // Only if the update is to the whole subresource, mark its layers.
        if (updateBox.coversSameExtent(levelExtents))
        {
            if (hasColorOrDepth)
            {
                supersededLayers[kIndexColorOrDepth] |= updateLayersMask;
            }
            if (hasStencil)
            {
                supersededLayers[kIndexStencil] |= updateLayersMask;
            }
        }

        return false;
    };

    for (LevelIndex levelVk(0); levelVk < LevelIndex(mLevelCount); ++levelVk)
    {
        gl::LevelIndex levelGL                       = toGLLevel(levelVk);
        std::vector<SubresourceUpdate> *levelUpdates = getLevelUpdates(levelGL);
        if (levelUpdates == nullptr)
        {
            ASSERT(static_cast<size_t>(levelGL.get()) >= mSubresourceUpdates.size());
            break;
        }

        // If level is skipped (because incompatibly redefined), don't remove any of its updates.
        if (skipLevelsMask.test(levelVk.get()))
        {
            continue;
        }

        levelExtents                         = getLevelExtents(levelVk);
        supersededLayers[kIndexColorOrDepth] = 0;
        supersededLayers[kIndexStencil]      = 0;

        levelUpdates->erase(levelUpdates->rend().base(),
                            std::remove_if(levelUpdates->rbegin(), levelUpdates->rend(),
                                           markLayersAndDropSuperseded)
                                .base());
    }

    ASSERT(validateSubresourceUpdateImageRefsConsistent());
}

angle::Result ImageHelper::copyImageDataToBuffer(ContextVk *contextVk,
                                                 gl::LevelIndex sourceLevelGL,
                                                 uint32_t layerCount,
                                                 uint32_t baseLayer,
                                                 const gl::Box &sourceArea,
                                                 BufferHelper **bufferOut,
                                                 size_t *bufferSize,
                                                 StagingBufferOffsetArray *bufferOffsetsOut,
                                                 uint8_t **outDataPtr)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "ImageHelper::copyImageDataToBuffer");

    const angle::Format &imageFormat = mFormat->actualImageFormat();

    // Two VK formats (one depth-only, one combined depth/stencil) use an extra byte for depth.
    // From https://www.khronos.org/registry/vulkan/specs/1.1/html/vkspec.html#VkBufferImageCopy:
    //  data copied to or from the depth aspect of a VK_FORMAT_X8_D24_UNORM_PACK32 or
    //  VK_FORMAT_D24_UNORM_S8_UINT format is packed with one 32-bit word per texel...
    // So make sure if we hit the depth/stencil format that we have 5 bytes per pixel (4 for depth
    //  data, 1 for stencil). NOTE that depth-only VK_FORMAT_X8_D24_UNORM_PACK32 already has 4 bytes
    //  per pixel which is sufficient to contain its depth aspect (no stencil aspect).
    uint32_t pixelBytes         = imageFormat.pixelBytes;
    uint32_t depthBytesPerPixel = imageFormat.depthBits >> 3;
    if (mFormat->actualImageVkFormat() == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        pixelBytes         = 5;
        depthBytesPerPixel = 4;
    }

    *bufferSize = sourceArea.width * sourceArea.height * sourceArea.depth * pixelBytes * layerCount;

    const VkImageAspectFlags aspectFlags = getAspectFlags();

    // Allocate staging buffer data from context
    VkBuffer bufferHandle;
    size_t alignment = mStagingBuffer.getAlignment();
    ANGLE_TRY(mStagingBuffer.allocateWithAlignment(contextVk, *bufferSize, alignment, outDataPtr,
                                                   &bufferHandle, &(*bufferOffsetsOut)[0],
                                                   nullptr));
    *bufferOut = mStagingBuffer.getCurrentBuffer();

    LevelIndex sourceLevelVk = toVkLevel(sourceLevelGL);

    VkBufferImageCopy regions[2] = {};
    // Default to non-combined DS case
    regions[0].bufferOffset                    = (*bufferOffsetsOut)[0];
    regions[0].bufferRowLength                 = 0;
    regions[0].bufferImageHeight               = 0;
    regions[0].imageExtent.width               = sourceArea.width;
    regions[0].imageExtent.height              = sourceArea.height;
    regions[0].imageExtent.depth               = sourceArea.depth;
    regions[0].imageOffset.x                   = sourceArea.x;
    regions[0].imageOffset.y                   = sourceArea.y;
    regions[0].imageOffset.z                   = sourceArea.z;
    regions[0].imageSubresource.aspectMask     = aspectFlags;
    regions[0].imageSubresource.baseArrayLayer = baseLayer;
    regions[0].imageSubresource.layerCount     = layerCount;
    regions[0].imageSubresource.mipLevel       = sourceLevelVk.get();

    if (isCombinedDepthStencilFormat())
    {
        // For combined DS image we'll copy depth and stencil aspects separately
        // Depth aspect comes first in buffer and can use most settings from above
        regions[0].imageSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        // Get depth data size since stencil data immediately follows depth data in buffer
        const VkDeviceSize depthSize = depthBytesPerPixel * sourceArea.width * sourceArea.height *
                                       sourceArea.depth * layerCount;

        // Double-check that we allocated enough buffer space (always 1 byte per stencil)
        ASSERT(*bufferSize >= (depthSize + (sourceArea.width * sourceArea.height *
                                            sourceArea.depth * layerCount)));

        // Copy stencil data into buffer immediately following the depth data
        const VkDeviceSize stencilOffset       = (*bufferOffsetsOut)[0] + depthSize;
        (*bufferOffsetsOut)[1]                 = stencilOffset;
        regions[1]                             = regions[0];
        regions[1].bufferOffset                = stencilOffset;
        regions[1].imageSubresource.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    CommandBufferAccess access;
    access.onBufferTransferWrite(*bufferOut);
    access.onImageTransferRead(aspectFlags, this);

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    commandBuffer->copyImageToBuffer(mImage, getCurrentLayout(), bufferHandle, 1, regions);

    return angle::Result::Continue;
}

// static
angle::Result ImageHelper::GetReadPixelsParams(ContextVk *contextVk,
                                               const gl::PixelPackState &packState,
                                               gl::Buffer *packBuffer,
                                               GLenum format,
                                               GLenum type,
                                               const gl::Rectangle &area,
                                               const gl::Rectangle &clippedArea,
                                               PackPixelsParams *paramsOut,
                                               GLuint *skipBytesOut)
{
    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(format, type);

    GLuint outputPitch = 0;
    ANGLE_VK_CHECK_MATH(contextVk,
                        sizedFormatInfo.computeRowPitch(type, area.width, packState.alignment,
                                                        packState.rowLength, &outputPitch));
    ANGLE_VK_CHECK_MATH(contextVk, sizedFormatInfo.computeSkipBytes(type, outputPitch, 0, packState,
                                                                    false, skipBytesOut));

    *skipBytesOut += (clippedArea.x - area.x) * sizedFormatInfo.pixelBytes +
                     (clippedArea.y - area.y) * outputPitch;

    const angle::Format &angleFormat = GetFormatFromFormatType(format, type);

    *paramsOut = PackPixelsParams(clippedArea, angleFormat, outputPitch, packState.reverseRowOrder,
                                  packBuffer, 0);
    return angle::Result::Continue;
}

angle::Result ImageHelper::readPixelsForGetImage(ContextVk *contextVk,
                                                 const gl::PixelPackState &packState,
                                                 gl::Buffer *packBuffer,
                                                 gl::LevelIndex levelGL,
                                                 uint32_t layer,
                                                 GLenum format,
                                                 GLenum type,
                                                 void *pixels)
{
    const angle::Format &angleFormat = GetFormatFromFormatType(format, type);

    VkImageAspectFlagBits aspectFlags = {};
    if (angleFormat.redBits > 0 || angleFormat.blueBits > 0 || angleFormat.greenBits > 0 ||
        angleFormat.alphaBits > 0 || angleFormat.luminanceBits > 0)
    {
        aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    else
    {
        if (angleFormat.depthBits > 0)
        {
            if (angleFormat.stencilBits != 0)
            {
                // TODO (anglebug.com/4688) Support combined depth stencil for GetTexImage
                WARN() << "Unable to pull combined depth/stencil for GetTexImage";
                return angle::Result::Continue;
            }
            aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        if (angleFormat.stencilBits > 0)
        {
            aspectFlags = VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }

    ASSERT(aspectFlags != 0);

    PackPixelsParams params;
    GLuint outputSkipBytes = 0;

    const LevelIndex levelVk     = toVkLevel(levelGL);
    const gl::Extents mipExtents = getLevelExtents(levelVk);
    gl::Rectangle area(0, 0, mipExtents.width, mipExtents.height);

    ANGLE_TRY(GetReadPixelsParams(contextVk, packState, packBuffer, format, type, area, area,
                                  &params, &outputSkipBytes));

    // Use a temporary staging buffer. Could be optimized.
    RendererScoped<DynamicBuffer> stagingBuffer(contextVk->getRenderer());
    stagingBuffer.get().init(contextVk->getRenderer(), VK_BUFFER_USAGE_TRANSFER_DST_BIT, 1,
                             kStagingBufferSize, true, DynamicBufferPolicy::OneShotUse);

    if (mExtents.depth > 1)
    {
        // Depth > 1 means this is a 3D texture and we need to copy all layers
        for (layer = 0; layer < static_cast<uint32_t>(mipExtents.depth); layer++)
        {
            ANGLE_TRY(readPixels(contextVk, area, params, aspectFlags, levelGL, layer,
                                 static_cast<uint8_t *>(pixels) + outputSkipBytes,
                                 &stagingBuffer.get()));

            outputSkipBytes += mipExtents.width * mipExtents.height *
                               gl::GetInternalFormatInfo(format, type).pixelBytes;
        }
    }
    else
    {
        ANGLE_TRY(readPixels(contextVk, area, params, aspectFlags, levelGL, layer,
                             static_cast<uint8_t *>(pixels) + outputSkipBytes,
                             &stagingBuffer.get()));
    }

    return angle::Result::Continue;
}

angle::Result ImageHelper::readPixels(ContextVk *contextVk,
                                      const gl::Rectangle &area,
                                      const PackPixelsParams &packPixelsParams,
                                      VkImageAspectFlagBits copyAspectFlags,
                                      gl::LevelIndex levelGL,
                                      uint32_t layer,
                                      void *pixels,
                                      DynamicBuffer *stagingBuffer)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "ImageHelper::readPixels");

    RendererVk *renderer = contextVk->getRenderer();

    // If the source image is multisampled, we need to resolve it into a temporary image before
    // performing a readback.
    bool isMultisampled = mSamples > 1;
    RendererScoped<ImageHelper> resolvedImage(contextVk->getRenderer());

    ImageHelper *src = this;

    ASSERT(!hasStagedUpdatesForSubresource(levelGL, layer, 1));

    if (isMultisampled)
    {
        ANGLE_TRY(resolvedImage.get().init2DStaging(
            contextVk, contextVk->hasProtectedContent(), renderer->getMemoryProperties(),
            gl::Extents(area.width, area.height, 1), *mFormat,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT, 1));
        resolvedImage.get().retain(&contextVk->getResourceUseList());
    }

    VkImageAspectFlags layoutChangeAspectFlags = src->getAspectFlags();

    // Note that although we're reading from the image, we need to update the layout below.
    CommandBufferAccess access;
    access.onImageTransferRead(layoutChangeAspectFlags, this);
    if (isMultisampled)
    {
        access.onImageTransferWrite(gl::LevelIndex(0), 1, 0, 1, layoutChangeAspectFlags,
                                    &resolvedImage.get());
    }

    CommandBuffer *commandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(access, &commandBuffer));

    const angle::Format *readFormat = &mFormat->actualImageFormat();

    if (copyAspectFlags != VK_IMAGE_ASPECT_COLOR_BIT)
    {
        readFormat = &GetDepthStencilImageToBufferFormat(*readFormat, copyAspectFlags);
    }

    VkOffset3D srcOffset = {area.x, area.y, 0};

    VkImageSubresourceLayers srcSubresource = {};
    srcSubresource.aspectMask               = copyAspectFlags;
    srcSubresource.mipLevel                 = toVkLevel(levelGL).get();
    srcSubresource.baseArrayLayer           = layer;
    srcSubresource.layerCount               = 1;

    VkExtent3D srcExtent = {static_cast<uint32_t>(area.width), static_cast<uint32_t>(area.height),
                            1};

    if (mExtents.depth > 1)
    {
        // Depth > 1 means this is a 3D texture and we need special handling
        srcOffset.z                   = layer;
        srcSubresource.baseArrayLayer = 0;
    }

    if (isMultisampled)
    {
        // Note: resolve only works on color images (not depth/stencil).
        ASSERT(copyAspectFlags == VK_IMAGE_ASPECT_COLOR_BIT);

        VkImageResolve resolveRegion                = {};
        resolveRegion.srcSubresource                = srcSubresource;
        resolveRegion.srcOffset                     = srcOffset;
        resolveRegion.dstSubresource.aspectMask     = copyAspectFlags;
        resolveRegion.dstSubresource.mipLevel       = 0;
        resolveRegion.dstSubresource.baseArrayLayer = 0;
        resolveRegion.dstSubresource.layerCount     = 1;
        resolveRegion.dstOffset                     = {};
        resolveRegion.extent                        = srcExtent;

        resolve(&resolvedImage.get(), resolveRegion, commandBuffer);

        CommandBufferAccess readAccess;
        readAccess.onImageTransferRead(layoutChangeAspectFlags, &resolvedImage.get());
        ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(readAccess, &commandBuffer));

        // Make the resolved image the target of buffer copy.
        src                           = &resolvedImage.get();
        srcOffset                     = {0, 0, 0};
        srcSubresource.baseArrayLayer = 0;
        srcSubresource.layerCount     = 1;
        srcSubresource.mipLevel       = 0;
    }

    // If PBO and if possible, copy directly on the GPU.
    if (packPixelsParams.packBuffer &&
        CanCopyWithTransformForReadPixels(packPixelsParams, mFormat, readFormat))
    {
        VkDeviceSize packBufferOffset = 0;
        BufferHelper &packBuffer =
            GetImpl(packPixelsParams.packBuffer)->getBufferAndOffset(&packBufferOffset);

        CommandBufferAccess copyAccess;
        copyAccess.onBufferTransferWrite(&packBuffer);
        copyAccess.onImageTransferRead(copyAspectFlags, src);

        CommandBuffer *copyCommandBuffer;
        ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(copyAccess, &copyCommandBuffer));

        ASSERT(packPixelsParams.outputPitch % readFormat->pixelBytes == 0);

        VkBufferImageCopy region = {};
        region.bufferImageHeight = srcExtent.height;
        region.bufferOffset =
            packBufferOffset + packPixelsParams.offset + reinterpret_cast<ptrdiff_t>(pixels);
        region.bufferRowLength  = packPixelsParams.outputPitch / readFormat->pixelBytes;
        region.imageExtent      = srcExtent;
        region.imageOffset      = srcOffset;
        region.imageSubresource = srcSubresource;

        copyCommandBuffer->copyImageToBuffer(src->getImage(), src->getCurrentLayout(),
                                             packBuffer.getBuffer().getHandle(), 1, &region);
        return angle::Result::Continue;
    }

    VkBuffer bufferHandle      = VK_NULL_HANDLE;
    uint8_t *readPixelBuffer   = nullptr;
    VkDeviceSize stagingOffset = 0;
    size_t allocationSize      = readFormat->pixelBytes * area.width * area.height;

    ANGLE_TRY(stagingBuffer->allocate(contextVk, allocationSize, &readPixelBuffer, &bufferHandle,
                                      &stagingOffset, nullptr));

    VkBufferImageCopy region = {};
    region.bufferImageHeight = srcExtent.height;
    region.bufferOffset      = stagingOffset;
    region.bufferRowLength   = srcExtent.width;
    region.imageExtent       = srcExtent;
    region.imageOffset       = srcOffset;
    region.imageSubresource  = srcSubresource;

    CommandBufferAccess readbackAccess;
    readbackAccess.onBufferTransferWrite(stagingBuffer->getCurrentBuffer());

    CommandBuffer *readbackCommandBuffer;
    ANGLE_TRY(contextVk->getOutsideRenderPassCommandBuffer(readbackAccess, &readbackCommandBuffer));

    readbackCommandBuffer->copyImageToBuffer(src->getImage(), src->getCurrentLayout(), bufferHandle,
                                             1, &region);

    ANGLE_PERF_WARNING(contextVk->getDebug(), GL_DEBUG_SEVERITY_HIGH,
                       "GPU stall due to ReadPixels");

    // Triggers a full finish.
    // TODO(jmadill): Don't block on asynchronous readback.
    ANGLE_TRY(contextVk->finishImpl());

    // The buffer we copied to needs to be invalidated before we read from it because its not been
    // created with the host coherent bit.
    ANGLE_TRY(stagingBuffer->invalidate(contextVk));

    if (packPixelsParams.packBuffer)
    {
        // Must map the PBO in order to read its contents (and then unmap it later)
        BufferVk *packBufferVk = GetImpl(packPixelsParams.packBuffer);
        void *mapPtr           = nullptr;
        ANGLE_TRY(packBufferVk->mapImpl(contextVk, &mapPtr));
        uint8_t *dest = static_cast<uint8_t *>(mapPtr) + reinterpret_cast<ptrdiff_t>(pixels);
        PackPixels(packPixelsParams, *readFormat, area.width * readFormat->pixelBytes,
                   readPixelBuffer, static_cast<uint8_t *>(dest));
        ANGLE_TRY(packBufferVk->unmapImpl(contextVk));
    }
    else
    {
        PackPixels(packPixelsParams, *readFormat, area.width * readFormat->pixelBytes,
                   readPixelBuffer, static_cast<uint8_t *>(pixels));
    }

    return angle::Result::Continue;
}

// ImageHelper::SubresourceUpdate implementation
ImageHelper::SubresourceUpdate::SubresourceUpdate()
    : updateSource(UpdateSource::Buffer), image(nullptr)
{
    data.buffer.bufferHelper = nullptr;
}

ImageHelper::SubresourceUpdate::~SubresourceUpdate() {}

ImageHelper::SubresourceUpdate::SubresourceUpdate(BufferHelper *bufferHelperIn,
                                                  const VkBufferImageCopy &copyRegionIn)
    : updateSource(UpdateSource::Buffer), image(nullptr)
{
    data.buffer.bufferHelper = bufferHelperIn;
    data.buffer.copyRegion   = copyRegionIn;
}

ImageHelper::SubresourceUpdate::SubresourceUpdate(RefCounted<ImageHelper> *imageIn,
                                                  const VkImageCopy &copyRegionIn)
    : updateSource(UpdateSource::Image), image(imageIn)
{
    image->addRef();
    data.image.copyRegion = copyRegionIn;
}

ImageHelper::SubresourceUpdate::SubresourceUpdate(VkImageAspectFlags aspectFlags,
                                                  const VkClearValue &clearValue,
                                                  const gl::ImageIndex &imageIndex)
    : updateSource(UpdateSource::Clear), image(nullptr)
{
    data.clear.aspectFlags = aspectFlags;
    data.clear.value       = clearValue;
    data.clear.levelIndex  = imageIndex.getLevelIndex();
    data.clear.layerIndex  = imageIndex.hasLayer() ? imageIndex.getLayerIndex() : 0;
    data.clear.layerCount =
        imageIndex.hasLayer() ? imageIndex.getLayerCount() : VK_REMAINING_ARRAY_LAYERS;
}

ImageHelper::SubresourceUpdate::SubresourceUpdate(SubresourceUpdate &&other)
    : updateSource(other.updateSource), image(nullptr)
{
    switch (updateSource)
    {
        case UpdateSource::Clear:
            data.clear = other.data.clear;
            break;
        case UpdateSource::Buffer:
            data.buffer = other.data.buffer;
            break;
        case UpdateSource::Image:
            data.image  = other.data.image;
            image       = other.image;
            other.image = nullptr;
            break;
        default:
            UNREACHABLE();
    }
}

ImageHelper::SubresourceUpdate &ImageHelper::SubresourceUpdate::operator=(SubresourceUpdate &&other)
{
    // Given that the update is a union of three structs, we can't use std::swap on the fields.  For
    // example, |this| may be an Image update and |other| may be a Buffer update.
    // The following could work:
    //
    // SubresourceUpdate oldThis;
    // Set oldThis to this->field based on updateSource
    // Set this->otherField to other.otherField based on other.updateSource
    // Set other.field to oldThis->field based on updateSource
    // std::Swap(updateSource, other.updateSource);
    //
    // It's much simpler to just swap the memory instead.

    SubresourceUpdate oldThis;
    memcpy(&oldThis, this, sizeof(*this));
    memcpy(this, &other, sizeof(*this));
    memcpy(&other, &oldThis, sizeof(*this));

    return *this;
}

void ImageHelper::SubresourceUpdate::release(RendererVk *renderer)
{
    if (updateSource == UpdateSource::Image)
    {
        image->releaseRef();

        if (!image->isReferenced())
        {
            // Staging images won't be used in render pass attachments.
            image->get().releaseImage(renderer);
            image->get().releaseStagingBuffer(renderer);
            SafeDelete(image);
        }

        image = nullptr;
    }
}

bool ImageHelper::SubresourceUpdate::isUpdateToLayers(uint32_t layerIndex,
                                                      uint32_t layerCount) const
{
    uint32_t updateBaseLayer, updateLayerCount;
    getDestSubresource(gl::ImageIndex::kEntireLevel, &updateBaseLayer, &updateLayerCount);

    return updateBaseLayer == layerIndex &&
           (updateLayerCount == layerCount || updateLayerCount == VK_REMAINING_ARRAY_LAYERS);
}

void ImageHelper::SubresourceUpdate::getDestSubresource(uint32_t imageLayerCount,
                                                        uint32_t *baseLayerOut,
                                                        uint32_t *layerCountOut) const
{
    if (updateSource == UpdateSource::Clear)
    {
        *baseLayerOut  = data.clear.layerIndex;
        *layerCountOut = data.clear.layerCount;

        if (*layerCountOut == static_cast<uint32_t>(gl::ImageIndex::kEntireLevel))
        {
            *layerCountOut = imageLayerCount;
        }
    }
    else
    {
        const VkImageSubresourceLayers &dstSubresource =
            updateSource == UpdateSource::Buffer ? data.buffer.copyRegion.imageSubresource
                                                 : data.image.copyRegion.dstSubresource;
        *baseLayerOut  = dstSubresource.baseArrayLayer;
        *layerCountOut = dstSubresource.layerCount;

        ASSERT(*layerCountOut != static_cast<uint32_t>(gl::ImageIndex::kEntireLevel));
    }
}

VkImageAspectFlags ImageHelper::SubresourceUpdate::getDestAspectFlags() const
{
    if (updateSource == UpdateSource::Clear)
    {
        return data.clear.aspectFlags;
    }
    else if (updateSource == UpdateSource::Buffer)
    {
        return data.buffer.copyRegion.imageSubresource.aspectMask;
    }
    else
    {
        ASSERT(updateSource == UpdateSource::Image);
        return data.image.copyRegion.dstSubresource.aspectMask;
    }
}

std::vector<ImageHelper::SubresourceUpdate> *ImageHelper::getLevelUpdates(gl::LevelIndex level)
{
    return static_cast<size_t>(level.get()) < mSubresourceUpdates.size()
               ? &mSubresourceUpdates[level.get()]
               : nullptr;
}

const std::vector<ImageHelper::SubresourceUpdate> *ImageHelper::getLevelUpdates(
    gl::LevelIndex level) const
{
    return static_cast<size_t>(level.get()) < mSubresourceUpdates.size()
               ? &mSubresourceUpdates[level.get()]
               : nullptr;
}

void ImageHelper::appendSubresourceUpdate(gl::LevelIndex level, SubresourceUpdate &&update)
{
    if (mSubresourceUpdates.size() <= static_cast<size_t>(level.get()))
    {
        mSubresourceUpdates.resize(level.get() + 1);
    }

    mSubresourceUpdates[level.get()].emplace_back(std::move(update));
    onStateChange(angle::SubjectMessage::SubjectChanged);
}

void ImageHelper::prependSubresourceUpdate(gl::LevelIndex level, SubresourceUpdate &&update)
{
    if (mSubresourceUpdates.size() <= static_cast<size_t>(level.get()))
    {
        mSubresourceUpdates.resize(level.get() + 1);
    }

    mSubresourceUpdates[level.get()].insert(mSubresourceUpdates[level.get()].begin(),
                                            std::move(update));
    onStateChange(angle::SubjectMessage::SubjectChanged);
}

// FramebufferHelper implementation.
FramebufferHelper::FramebufferHelper() = default;

FramebufferHelper::~FramebufferHelper() = default;

FramebufferHelper::FramebufferHelper(FramebufferHelper &&other) : Resource(std::move(other))
{
    mFramebuffer = std::move(other.mFramebuffer);
}

FramebufferHelper &FramebufferHelper::operator=(FramebufferHelper &&other)
{
    std::swap(mUse, other.mUse);
    std::swap(mFramebuffer, other.mFramebuffer);
    return *this;
}

angle::Result FramebufferHelper::init(ContextVk *contextVk,
                                      const VkFramebufferCreateInfo &createInfo)
{
    ANGLE_VK_TRY(contextVk, mFramebuffer.init(contextVk->getDevice(), createInfo));
    return angle::Result::Continue;
}

void FramebufferHelper::release(ContextVk *contextVk)
{
    contextVk->addGarbage(&mFramebuffer);
}

LayerMode GetLayerMode(const vk::ImageHelper &image, uint32_t layerCount)
{
    const uint32_t imageLayerCount = GetImageLayerCountForView(image);
    const bool allLayers           = layerCount == imageLayerCount;

    ASSERT(allLayers || layerCount > 0 && layerCount <= gl::IMPLEMENTATION_MAX_TEXTURE_LEVELS);
    return allLayers ? LayerMode::All : static_cast<LayerMode>(layerCount);
}

// ImageViewHelper implementation.
ImageViewHelper::ImageViewHelper() : mCurrentMaxLevel(0), mLinearColorspace(true) {}

ImageViewHelper::ImageViewHelper(ImageViewHelper &&other) : Resource(std::move(other))
{
    std::swap(mUse, other.mUse);

    std::swap(mCurrentMaxLevel, other.mCurrentMaxLevel);
    std::swap(mPerLevelLinearReadImageViews, other.mPerLevelLinearReadImageViews);
    std::swap(mPerLevelSRGBReadImageViews, other.mPerLevelSRGBReadImageViews);
    std::swap(mPerLevelLinearFetchImageViews, other.mPerLevelLinearFetchImageViews);
    std::swap(mPerLevelSRGBFetchImageViews, other.mPerLevelSRGBFetchImageViews);
    std::swap(mPerLevelLinearCopyImageViews, other.mPerLevelLinearCopyImageViews);
    std::swap(mPerLevelSRGBCopyImageViews, other.mPerLevelSRGBCopyImageViews);
    std::swap(mLinearColorspace, other.mLinearColorspace);

    std::swap(mPerLevelStencilReadImageViews, other.mPerLevelStencilReadImageViews);
    std::swap(mLayerLevelDrawImageViews, other.mLayerLevelDrawImageViews);
    std::swap(mLayerLevelDrawImageViewsLinear, other.mLayerLevelDrawImageViewsLinear);
    std::swap(mSubresourceDrawImageViews, other.mSubresourceDrawImageViews);
    std::swap(mLevelStorageImageViews, other.mLevelStorageImageViews);
    std::swap(mLayerLevelStorageImageViews, other.mLayerLevelStorageImageViews);
    std::swap(mImageViewSerial, other.mImageViewSerial);
}

ImageViewHelper::~ImageViewHelper() {}

void ImageViewHelper::init(RendererVk *renderer)
{
    if (!mImageViewSerial.valid())
    {
        mImageViewSerial = renderer->getResourceSerialFactory().generateImageOrBufferViewSerial();
    }
}

void ImageViewHelper::release(RendererVk *renderer)
{
    std::vector<GarbageObject> garbage;

    mCurrentMaxLevel = LevelIndex(0);

    // Release the read views
    ReleaseImageViews(&mPerLevelLinearReadImageViews, &garbage);
    ReleaseImageViews(&mPerLevelSRGBReadImageViews, &garbage);
    ReleaseImageViews(&mPerLevelLinearFetchImageViews, &garbage);
    ReleaseImageViews(&mPerLevelSRGBFetchImageViews, &garbage);
    ReleaseImageViews(&mPerLevelLinearCopyImageViews, &garbage);
    ReleaseImageViews(&mPerLevelSRGBCopyImageViews, &garbage);
    ReleaseImageViews(&mPerLevelStencilReadImageViews, &garbage);

    // Release the draw views
    for (ImageViewVector &layerViews : mLayerLevelDrawImageViews)
    {
        for (ImageView &imageView : layerViews)
        {
            if (imageView.valid())
            {
                garbage.emplace_back(GetGarbage(&imageView));
            }
        }
    }
    mLayerLevelDrawImageViews.clear();
    for (ImageViewVector &layerViews : mLayerLevelDrawImageViewsLinear)
    {
        for (ImageView &imageView : layerViews)
        {
            if (imageView.valid())
            {
                garbage.emplace_back(GetGarbage(&imageView));
            }
        }
    }
    mLayerLevelDrawImageViewsLinear.clear();
    for (auto &iter : mSubresourceDrawImageViews)
    {
        std::unique_ptr<ImageView> &imageView = iter.second;
        if (imageView->valid())
        {
            garbage.emplace_back(GetGarbage(imageView.get()));
        }
    }
    mSubresourceDrawImageViews.clear();

    // Release the storage views
    ReleaseImageViews(&mLevelStorageImageViews, &garbage);
    for (ImageViewVector &layerViews : mLayerLevelStorageImageViews)
    {
        for (ImageView &imageView : layerViews)
        {
            if (imageView.valid())
            {
                garbage.emplace_back(GetGarbage(&imageView));
            }
        }
    }
    mLayerLevelStorageImageViews.clear();

    if (!garbage.empty())
    {
        renderer->collectGarbage(std::move(mUse), std::move(garbage));

        // Ensure the resource use is always valid.
        mUse.init();
    }

    // Update image view serial.
    mImageViewSerial = renderer->getResourceSerialFactory().generateImageOrBufferViewSerial();
}

void ImageViewHelper::destroy(VkDevice device)
{
    mCurrentMaxLevel = LevelIndex(0);

    // Release the read views
    DestroyImageViews(&mPerLevelLinearReadImageViews, device);
    DestroyImageViews(&mPerLevelSRGBReadImageViews, device);
    DestroyImageViews(&mPerLevelLinearFetchImageViews, device);
    DestroyImageViews(&mPerLevelSRGBFetchImageViews, device);
    DestroyImageViews(&mPerLevelLinearCopyImageViews, device);
    DestroyImageViews(&mPerLevelSRGBCopyImageViews, device);
    DestroyImageViews(&mPerLevelStencilReadImageViews, device);

    // Release the draw views
    for (ImageViewVector &layerViews : mLayerLevelDrawImageViews)
    {
        for (ImageView &imageView : layerViews)
        {
            imageView.destroy(device);
        }
    }
    mLayerLevelDrawImageViews.clear();
    for (ImageViewVector &layerViews : mLayerLevelDrawImageViewsLinear)
    {
        for (ImageView &imageView : layerViews)
        {
            imageView.destroy(device);
        }
    }
    mLayerLevelDrawImageViewsLinear.clear();
    for (auto &iter : mSubresourceDrawImageViews)
    {
        std::unique_ptr<ImageView> &imageView = iter.second;
        imageView->destroy(device);
    }
    mSubresourceDrawImageViews.clear();

    // Release the storage views
    DestroyImageViews(&mLevelStorageImageViews, device);
    for (ImageViewVector &layerViews : mLayerLevelStorageImageViews)
    {
        for (ImageView &imageView : layerViews)
        {
            imageView.destroy(device);
        }
    }
    mLayerLevelStorageImageViews.clear();

    mImageViewSerial = kInvalidImageOrBufferViewSerial;
}

angle::Result ImageViewHelper::initReadViews(ContextVk *contextVk,
                                             gl::TextureType viewType,
                                             const ImageHelper &image,
                                             const Format &format,
                                             const gl::SwizzleState &formatSwizzle,
                                             const gl::SwizzleState &readSwizzle,
                                             LevelIndex baseLevel,
                                             uint32_t levelCount,
                                             uint32_t baseLayer,
                                             uint32_t layerCount,
                                             bool requiresSRGBViews,
                                             VkImageUsageFlags imageUsageFlags)
{
    ASSERT(levelCount > 0);
    if (levelCount > mPerLevelLinearReadImageViews.size())
    {
        mPerLevelLinearReadImageViews.resize(levelCount);
        mPerLevelSRGBReadImageViews.resize(levelCount);
        mPerLevelLinearFetchImageViews.resize(levelCount);
        mPerLevelSRGBFetchImageViews.resize(levelCount);
        mPerLevelLinearCopyImageViews.resize(levelCount);
        mPerLevelSRGBCopyImageViews.resize(levelCount);
        mPerLevelStencilReadImageViews.resize(levelCount);
    }
    mCurrentMaxLevel = LevelIndex(levelCount - 1);

    // Determine if we already have ImageViews for the new max level
    if (getReadImageView().valid())
    {
        return angle::Result::Continue;
    }

    // Since we don't have a readImageView, we must create ImageViews for the new max level
    ANGLE_TRY(initReadViewsImpl(contextVk, viewType, image, format, formatSwizzle, readSwizzle,
                                baseLevel, levelCount, baseLayer, layerCount));

    if (requiresSRGBViews)
    {
        ANGLE_TRY(initSRGBReadViewsImpl(contextVk, viewType, image, format, formatSwizzle,
                                        readSwizzle, baseLevel, levelCount, baseLayer, layerCount,
                                        imageUsageFlags));
    }

    return angle::Result::Continue;
}

angle::Result ImageViewHelper::initReadViewsImpl(ContextVk *contextVk,
                                                 gl::TextureType viewType,
                                                 const ImageHelper &image,
                                                 const Format &format,
                                                 const gl::SwizzleState &formatSwizzle,
                                                 const gl::SwizzleState &readSwizzle,
                                                 LevelIndex baseLevel,
                                                 uint32_t levelCount,
                                                 uint32_t baseLayer,
                                                 uint32_t layerCount)
{
    ASSERT(mImageViewSerial.valid());

    const VkImageAspectFlags aspectFlags = GetFormatAspectFlags(format.intendedFormat());
    mLinearColorspace                    = !format.actualImageFormat().isSRGB;

    if (HasBothDepthAndStencilAspects(aspectFlags))
    {
        ANGLE_TRY(image.initLayerImageViewWithFormat(
            contextVk, viewType, format, VK_IMAGE_ASPECT_DEPTH_BIT, readSwizzle,
            &getReadImageView(), baseLevel, levelCount, baseLayer, layerCount));
        ANGLE_TRY(image.initLayerImageViewWithFormat(
            contextVk, viewType, format, VK_IMAGE_ASPECT_STENCIL_BIT, readSwizzle,
            &mPerLevelStencilReadImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount,
            baseLayer, layerCount));
    }
    else
    {
        ANGLE_TRY(image.initLayerImageViewWithFormat(contextVk, viewType, format, aspectFlags,
                                                     readSwizzle, &getReadImageView(), baseLevel,
                                                     levelCount, baseLayer, layerCount));
    }

    gl::TextureType fetchType = viewType;

    if (viewType == gl::TextureType::CubeMap || viewType == gl::TextureType::_2DArray ||
        viewType == gl::TextureType::_2DMultisampleArray)
    {
        fetchType = Get2DTextureType(layerCount, image.getSamples());

        ANGLE_TRY(image.initLayerImageViewWithFormat(contextVk, fetchType, format, aspectFlags,
                                                     readSwizzle, &getFetchImageView(), baseLevel,
                                                     levelCount, baseLayer, layerCount));
    }

    ANGLE_TRY(image.initLayerImageViewWithFormat(contextVk, fetchType, format, aspectFlags,
                                                 formatSwizzle, &getCopyImageView(), baseLevel,
                                                 levelCount, baseLayer, layerCount));

    return angle::Result::Continue;
}

angle::Result ImageViewHelper::initSRGBReadViewsImpl(ContextVk *contextVk,
                                                     gl::TextureType viewType,
                                                     const ImageHelper &image,
                                                     const Format &format,
                                                     const gl::SwizzleState &formatSwizzle,
                                                     const gl::SwizzleState &readSwizzle,
                                                     LevelIndex baseLevel,
                                                     uint32_t levelCount,
                                                     uint32_t baseLayer,
                                                     uint32_t layerCount,
                                                     VkImageUsageFlags imageUsageFlags)
{
    // When we select the linear/srgb counterpart formats, we must first make sure they're
    // actually supported by the ICD. If they are not supported by the ICD, then we treat that as if
    // there is no counterpart format. (In this case, the relevant extension should not be exposed)
    angle::FormatID srgbOverrideFormat = ConvertToSRGB(image.getFormat().actualImageFormatID);
    ASSERT((srgbOverrideFormat == angle::FormatID::NONE) ||
           (HasNonRenderableTextureFormatSupport(contextVk->getRenderer(), srgbOverrideFormat)));

    angle::FormatID linearOverrideFormat = ConvertToLinear(image.getFormat().actualImageFormatID);
    ASSERT((linearOverrideFormat == angle::FormatID::NONE) ||
           (HasNonRenderableTextureFormatSupport(contextVk->getRenderer(), linearOverrideFormat)));

    angle::FormatID linearFormat = (linearOverrideFormat != angle::FormatID::NONE)
                                       ? linearOverrideFormat
                                       : format.actualImageFormatID;
    ASSERT(linearFormat != angle::FormatID::NONE);

    const VkImageAspectFlags aspectFlags = GetFormatAspectFlags(format.intendedFormat());

    if (!mPerLevelLinearReadImageViews[mCurrentMaxLevel.get()].valid())
    {
        ANGLE_TRY(image.initReinterpretedLayerImageView(
            contextVk, viewType, aspectFlags, readSwizzle,
            &mPerLevelLinearReadImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount,
            baseLayer, layerCount, imageUsageFlags, linearFormat));
    }
    if (srgbOverrideFormat != angle::FormatID::NONE &&
        !mPerLevelSRGBReadImageViews[mCurrentMaxLevel.get()].valid())
    {
        ANGLE_TRY(image.initReinterpretedLayerImageView(
            contextVk, viewType, aspectFlags, readSwizzle,
            &mPerLevelSRGBReadImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount, baseLayer,
            layerCount, imageUsageFlags, srgbOverrideFormat));
    }

    gl::TextureType fetchType = viewType;

    if (viewType == gl::TextureType::CubeMap || viewType == gl::TextureType::_2DArray ||
        viewType == gl::TextureType::_2DMultisampleArray)
    {
        fetchType = Get2DTextureType(layerCount, image.getSamples());

        if (!mPerLevelLinearFetchImageViews[mCurrentMaxLevel.get()].valid())
        {

            ANGLE_TRY(image.initReinterpretedLayerImageView(
                contextVk, fetchType, aspectFlags, readSwizzle,
                &mPerLevelLinearFetchImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount,
                baseLayer, layerCount, imageUsageFlags, linearFormat));
        }
        if (srgbOverrideFormat != angle::FormatID::NONE &&
            !mPerLevelSRGBFetchImageViews[mCurrentMaxLevel.get()].valid())
        {
            ANGLE_TRY(image.initReinterpretedLayerImageView(
                contextVk, fetchType, aspectFlags, readSwizzle,
                &mPerLevelSRGBFetchImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount,
                baseLayer, layerCount, imageUsageFlags, srgbOverrideFormat));
        }
    }

    if (!mPerLevelLinearCopyImageViews[mCurrentMaxLevel.get()].valid())
    {
        ANGLE_TRY(image.initReinterpretedLayerImageView(
            contextVk, fetchType, aspectFlags, formatSwizzle,
            &mPerLevelLinearCopyImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount,
            baseLayer, layerCount, imageUsageFlags, linearFormat));
    }
    if (srgbOverrideFormat != angle::FormatID::NONE &&
        !mPerLevelSRGBCopyImageViews[mCurrentMaxLevel.get()].valid())
    {
        ANGLE_TRY(image.initReinterpretedLayerImageView(
            contextVk, fetchType, aspectFlags, formatSwizzle,
            &mPerLevelSRGBCopyImageViews[mCurrentMaxLevel.get()], baseLevel, levelCount, baseLayer,
            layerCount, imageUsageFlags, srgbOverrideFormat));
    }

    return angle::Result::Continue;
}

angle::Result ImageViewHelper::getLevelStorageImageView(ContextVk *contextVk,
                                                        gl::TextureType viewType,
                                                        const ImageHelper &image,
                                                        LevelIndex levelVk,
                                                        uint32_t layer,
                                                        VkImageUsageFlags imageUsageFlags,
                                                        angle::FormatID formatID,
                                                        const ImageView **imageViewOut)
{
    ASSERT(mImageViewSerial.valid());

    retain(&contextVk->getResourceUseList());

    ImageView *imageView =
        GetLevelImageView(&mLevelStorageImageViews, levelVk, image.getLevelCount());

    *imageViewOut = imageView;
    if (imageView->valid())
    {
        return angle::Result::Continue;
    }

    // Create the view.  Note that storage images are not affected by swizzle parameters.
    return image.initReinterpretedLayerImageView(contextVk, viewType, image.getAspectFlags(),
                                                 gl::SwizzleState(), imageView, levelVk, 1, layer,
                                                 image.getLayerCount(), imageUsageFlags, formatID);
}

angle::Result ImageViewHelper::getLevelLayerStorageImageView(ContextVk *contextVk,
                                                             const ImageHelper &image,
                                                             LevelIndex levelVk,
                                                             uint32_t layer,
                                                             VkImageUsageFlags imageUsageFlags,
                                                             angle::FormatID formatID,
                                                             const ImageView **imageViewOut)
{
    ASSERT(image.valid());
    ASSERT(mImageViewSerial.valid());
    ASSERT(!image.getFormat().actualImageFormat().isBlock);

    retain(&contextVk->getResourceUseList());

    ImageView *imageView =
        GetLevelLayerImageView(&mLayerLevelStorageImageViews, levelVk, layer, image.getLevelCount(),
                               GetImageLayerCountForView(image));
    *imageViewOut = imageView;

    if (imageView->valid())
    {
        return angle::Result::Continue;
    }

    // Create the view.  Note that storage images are not affected by swizzle parameters.
    gl::TextureType viewType = Get2DTextureType(1, image.getSamples());
    return image.initReinterpretedLayerImageView(contextVk, viewType, image.getAspectFlags(),
                                                 gl::SwizzleState(), imageView, levelVk, 1, layer,
                                                 1, imageUsageFlags, formatID);
}

angle::Result ImageViewHelper::getLevelDrawImageView(ContextVk *contextVk,
                                                     const ImageHelper &image,
                                                     LevelIndex levelVk,
                                                     uint32_t layer,
                                                     uint32_t layerCount,
                                                     gl::SrgbWriteControlMode mode,
                                                     const ImageView **imageViewOut)
{
    ASSERT(image.valid());
    ASSERT(mImageViewSerial.valid());
    ASSERT(!image.getFormat().actualImageFormat().isBlock);

    retain(&contextVk->getResourceUseList());

    ImageSubresourceRange range = MakeImageSubresourceDrawRange(
        image.toGLLevel(levelVk), layer, GetLayerMode(image, layerCount), mode);

    std::unique_ptr<ImageView> &view = mSubresourceDrawImageViews[range];
    if (view)
    {
        *imageViewOut = view.get();
        return angle::Result::Continue;
    }

    view          = std::make_unique<ImageView>();
    *imageViewOut = view.get();

    // Lazily allocate the image view.
    // Note that these views are specifically made to be used as framebuffer attachments, and
    // therefore don't have swizzle.
    gl::TextureType viewType = Get2DTextureType(layerCount, image.getSamples());
    return image.initLayerImageView(contextVk, viewType, image.getAspectFlags(), gl::SwizzleState(),
                                    view.get(), levelVk, 1, layer, layerCount, mode);
}

angle::Result ImageViewHelper::getLevelLayerDrawImageView(ContextVk *contextVk,
                                                          const ImageHelper &image,
                                                          LevelIndex levelVk,
                                                          uint32_t layer,
                                                          gl::SrgbWriteControlMode mode,
                                                          const ImageView **imageViewOut)
{
    ASSERT(image.valid());
    ASSERT(mImageViewSerial.valid());
    ASSERT(!image.getFormat().actualImageFormat().isBlock);

    retain(&contextVk->getResourceUseList());

    LayerLevelImageViewVector &imageViews = (mode == gl::SrgbWriteControlMode::Linear)
                                                ? mLayerLevelDrawImageViewsLinear
                                                : mLayerLevelDrawImageViews;

    // Lazily allocate the storage for image views
    ImageView *imageView = GetLevelLayerImageView(
        &imageViews, levelVk, layer, image.getLevelCount(), GetImageLayerCountForView(image));
    *imageViewOut = imageView;

    if (imageView->valid())
    {
        return angle::Result::Continue;
    }

    // Lazily allocate the image view itself.
    // Note that these views are specifically made to be used as framebuffer attachments, and
    // therefore don't have swizzle.
    gl::TextureType viewType = Get2DTextureType(1, image.getSamples());
    return image.initLayerImageView(contextVk, viewType, image.getAspectFlags(), gl::SwizzleState(),
                                    imageView, levelVk, 1, layer, 1, mode);
}

ImageOrBufferViewSubresourceSerial ImageViewHelper::getSubresourceSerial(
    gl::LevelIndex levelGL,
    uint32_t levelCount,
    uint32_t layer,
    LayerMode layerMode,
    SrgbDecodeMode srgbDecodeMode,
    gl::SrgbOverride srgbOverrideMode) const
{
    ASSERT(mImageViewSerial.valid());

    ImageOrBufferViewSubresourceSerial serial;
    serial.viewSerial  = mImageViewSerial;
    serial.subresource = MakeImageSubresourceReadRange(levelGL, levelCount, layer, layerMode,
                                                       srgbDecodeMode, srgbOverrideMode);
    return serial;
}

ImageSubresourceRange MakeImageSubresourceReadRange(gl::LevelIndex level,
                                                    uint32_t levelCount,
                                                    uint32_t layer,
                                                    LayerMode layerMode,
                                                    SrgbDecodeMode srgbDecodeMode,
                                                    gl::SrgbOverride srgbOverrideMode)
{
    ImageSubresourceRange range;

    SetBitField(range.level, level.get());
    SetBitField(range.levelCount, levelCount);
    SetBitField(range.layer, layer);
    SetBitField(range.layerMode, layerMode);
    SetBitField(range.srgbDecodeMode, srgbDecodeMode);
    SetBitField(range.srgbMode, srgbOverrideMode);

    return range;
}

ImageSubresourceRange MakeImageSubresourceDrawRange(gl::LevelIndex level,
                                                    uint32_t layer,
                                                    LayerMode layerMode,
                                                    gl::SrgbWriteControlMode srgbWriteControlMode)
{
    ImageSubresourceRange range;

    SetBitField(range.level, level.get());
    SetBitField(range.levelCount, 1);
    SetBitField(range.layer, layer);
    SetBitField(range.layerMode, layerMode);
    SetBitField(range.srgbDecodeMode, 0);
    SetBitField(range.srgbMode, srgbWriteControlMode);

    return range;
}

// BufferViewHelper implementation.
BufferViewHelper::BufferViewHelper() : mOffset(0), mSize(0) {}

BufferViewHelper::BufferViewHelper(BufferViewHelper &&other) : Resource(std::move(other))
{
    std::swap(mOffset, other.mOffset);
    std::swap(mSize, other.mSize);
    std::swap(mViews, other.mViews);
    std::swap(mViewSerial, other.mViewSerial);
}

BufferViewHelper::~BufferViewHelper() {}

void BufferViewHelper::init(RendererVk *renderer, VkDeviceSize offset, VkDeviceSize size)
{
    ASSERT(mViews.empty());

    mOffset = offset;
    mSize   = size;

    if (!mViewSerial.valid())
    {
        mViewSerial = renderer->getResourceSerialFactory().generateImageOrBufferViewSerial();
    }
}

void BufferViewHelper::release(RendererVk *renderer)
{
    std::vector<GarbageObject> garbage;

    for (auto &formatAndView : mViews)
    {
        BufferView &view = formatAndView.second;
        ASSERT(view.valid());

        garbage.emplace_back(GetGarbage(&view));
    }

    if (!garbage.empty())
    {
        renderer->collectGarbage(std::move(mUse), std::move(garbage));

        // Ensure the resource use is always valid.
        mUse.init();

        // Update image view serial.
        mViewSerial = renderer->getResourceSerialFactory().generateImageOrBufferViewSerial();
    }

    mViews.clear();

    mOffset = 0;
    mSize   = 0;
}

void BufferViewHelper::destroy(VkDevice device)
{
    for (auto &formatAndView : mViews)
    {
        BufferView &view = formatAndView.second;
        view.destroy(device);
    }

    mViews.clear();

    mOffset = 0;
    mSize   = 0;

    mViewSerial = kInvalidImageOrBufferViewSerial;
}

angle::Result BufferViewHelper::getView(ContextVk *contextVk,
                                        const BufferHelper &buffer,
                                        VkDeviceSize bufferOffset,
                                        const Format &format,
                                        const BufferView **viewOut)
{
    ASSERT(format.valid());

    VkFormat viewVkFormat = format.actualBufferVkFormat(false);

    auto iter = mViews.find(viewVkFormat);
    if (iter != mViews.end())
    {
        *viewOut = &iter->second;
        return angle::Result::Continue;
    }

    // If the size is not a multiple of pixelBytes, remove the extra bytes.  The last element cannot
    // be read anyway, and this is a requirement of Vulkan (for size to be a multiple of format
    // texel block size).
    const angle::Format &bufferFormat = format.actualBufferFormat(false);
    const GLuint pixelBytes           = bufferFormat.pixelBytes;
    VkDeviceSize size                 = mSize - mSize % pixelBytes;

    VkBufferViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType                  = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    viewCreateInfo.buffer                 = buffer.getBuffer().getHandle();
    viewCreateInfo.format                 = viewVkFormat;
    viewCreateInfo.offset                 = mOffset + bufferOffset;
    viewCreateInfo.range                  = size;

    BufferView view;
    ANGLE_VK_TRY(contextVk, view.init(contextVk->getDevice(), viewCreateInfo));

    // Cache the view
    auto insertIter = mViews.insert({viewVkFormat, std::move(view)});
    *viewOut        = &insertIter.first->second;
    ASSERT(insertIter.second);

    return angle::Result::Continue;
}

ImageOrBufferViewSubresourceSerial BufferViewHelper::getSerial() const
{
    ASSERT(mViewSerial.valid());

    ImageOrBufferViewSubresourceSerial serial = {};
    serial.viewSerial                         = mViewSerial;
    return serial;
}

// ShaderProgramHelper implementation.
ShaderProgramHelper::ShaderProgramHelper() : mSpecializationConstants{} {}

ShaderProgramHelper::~ShaderProgramHelper() = default;

bool ShaderProgramHelper::valid(const gl::ShaderType shaderType) const
{
    return mShaders[shaderType].valid();
}

void ShaderProgramHelper::destroy(RendererVk *rendererVk)
{
    mGraphicsPipelines.destroy(rendererVk);
    mComputePipeline.destroy(rendererVk->getDevice());
    for (BindingPointer<ShaderAndSerial> &shader : mShaders)
    {
        shader.reset();
    }
}

void ShaderProgramHelper::release(ContextVk *contextVk)
{
    mGraphicsPipelines.release(contextVk);
    contextVk->addGarbage(&mComputePipeline.get());
    for (BindingPointer<ShaderAndSerial> &shader : mShaders)
    {
        shader.reset();
    }
}

void ShaderProgramHelper::setShader(gl::ShaderType shaderType, RefCounted<ShaderAndSerial> *shader)
{
    mShaders[shaderType].set(shader);
}

void ShaderProgramHelper::setSpecializationConstant(sh::vk::SpecializationConstantId id,
                                                    uint32_t value)
{
    ASSERT(id < sh::vk::SpecializationConstantId::EnumCount);
    switch (id)
    {
        case sh::vk::SpecializationConstantId::LineRasterEmulation:
            mSpecializationConstants.lineRasterEmulation = value;
            break;
        case sh::vk::SpecializationConstantId::SurfaceRotation:
            mSpecializationConstants.surfaceRotation = value;
            break;
        case sh::vk::SpecializationConstantId::DrawableWidth:
            mSpecializationConstants.drawableWidth = static_cast<float>(value);
            break;
        case sh::vk::SpecializationConstantId::DrawableHeight:
            mSpecializationConstants.drawableHeight = static_cast<float>(value);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

angle::Result ShaderProgramHelper::getComputePipeline(Context *context,
                                                      const PipelineLayout &pipelineLayout,
                                                      PipelineAndSerial **pipelineOut)
{
    if (mComputePipeline.valid())
    {
        *pipelineOut = &mComputePipeline;
        return angle::Result::Continue;
    }

    RendererVk *renderer = context->getRenderer();

    VkPipelineShaderStageCreateInfo shaderStage = {};
    VkComputePipelineCreateInfo createInfo      = {};

    shaderStage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStage.flags               = 0;
    shaderStage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStage.module              = mShaders[gl::ShaderType::Compute].get().get().getHandle();
    shaderStage.pName               = "main";
    shaderStage.pSpecializationInfo = nullptr;

    createInfo.sType              = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.flags              = 0;
    createInfo.stage              = shaderStage;
    createInfo.layout             = pipelineLayout.getHandle();
    createInfo.basePipelineHandle = VK_NULL_HANDLE;
    createInfo.basePipelineIndex  = 0;

    PipelineCache *pipelineCache = nullptr;
    ANGLE_TRY(renderer->getPipelineCache(&pipelineCache));
    ANGLE_VK_TRY(context, mComputePipeline.get().initCompute(context->getDevice(), createInfo,
                                                             *pipelineCache));

    *pipelineOut = &mComputePipeline;
    return angle::Result::Continue;
}

// ActiveHandleCounter implementation.
ActiveHandleCounter::ActiveHandleCounter() : mActiveCounts{}, mAllocatedCounts{} {}

ActiveHandleCounter::~ActiveHandleCounter() = default;

// CommandBufferAccess implementation.
CommandBufferAccess::CommandBufferAccess()  = default;
CommandBufferAccess::~CommandBufferAccess() = default;

void CommandBufferAccess::onBufferRead(VkAccessFlags readAccessType,
                                       PipelineStage readStage,
                                       BufferHelper *buffer)
{
    ASSERT(!buffer->isReleasedToExternal());
    mReadBuffers.emplace_back(buffer, readAccessType, readStage);
}

void CommandBufferAccess::onBufferWrite(VkAccessFlags writeAccessType,
                                        PipelineStage writeStage,
                                        BufferHelper *buffer)
{
    ASSERT(!buffer->isReleasedToExternal());
    mWriteBuffers.emplace_back(buffer, writeAccessType, writeStage);
}

void CommandBufferAccess::onImageRead(VkImageAspectFlags aspectFlags,
                                      ImageLayout imageLayout,
                                      ImageHelper *image)
{
    ASSERT(!image->isReleasedToExternal());
    ASSERT(image->getImageSerial().valid());
    mReadImages.emplace_back(image, aspectFlags, imageLayout);
}

void CommandBufferAccess::onImageWrite(gl::LevelIndex levelStart,
                                       uint32_t levelCount,
                                       uint32_t layerStart,
                                       uint32_t layerCount,
                                       VkImageAspectFlags aspectFlags,
                                       ImageLayout imageLayout,
                                       ImageHelper *image)
{
    ASSERT(!image->isReleasedToExternal());
    ASSERT(image->getImageSerial().valid());
    mWriteImages.emplace_back(CommandBufferImageAccess{image, aspectFlags, imageLayout}, levelStart,
                              levelCount, layerStart, layerCount);
}

}  // namespace vk
}  // namespace rx

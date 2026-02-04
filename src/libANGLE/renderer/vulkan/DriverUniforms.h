//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DriverUniforms.h:
//    Defines the interface for DriverUniforms
//

#ifndef LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_
#define LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_

#include "GLSLANG/ShaderLang.h"
#include "common/PackedEnums.h"
#include "common/angleutils.h"
#include "libANGLE/RefCountObject.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"

namespace
{
uint32_t MakeFlipUniform(bool flipX, bool flipY, bool invertViewport)
{
    // Create snorm values of either -1 or 1, based on whether flipping is enabled or not
    // respectively.
    constexpr uint8_t kSnormOne      = 0x7F;
    constexpr uint8_t kSnormMinusOne = 0x81;

    // .xy are flips for the fragment stage.
    uint32_t x = flipX ? kSnormMinusOne : kSnormOne;
    uint32_t y = flipY ? kSnormMinusOne : kSnormOne;

    // .zw are flips for the vertex stage.
    uint32_t z = x;
    uint32_t w = flipY != invertViewport ? kSnormMinusOne : kSnormOne;

    return x | y << 8 | z << 16 | w << 24;
}
}  // namespace

namespace rx
{
template <typename OffsetsArrayT>
void UpdateAtomicCounterBufferOffset(vk::Renderer *renderer,
                                     size_t atomicCounterBufferCount,
                                     const gl::BufferVector &atomicCounterBuffers,
                                     OffsetsArrayT &offsetsOut)
{
    const VkDeviceSize offsetAlignment =
        renderer->getPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;

    ASSERT(atomicCounterBufferCount <= offsetsOut.size() * 4);

    for (uint32_t bufferIndex = 0; bufferIndex < atomicCounterBufferCount; ++bufferIndex)
    {
        uint32_t offsetDiff = 0;
        const gl::OffsetBindingPointer<gl::Buffer> *atomicCounterBuffer =
            &atomicCounterBuffers[bufferIndex];

        if (atomicCounterBuffer->get())
        {
            VkDeviceSize offset        = atomicCounterBuffer->getOffset();
            VkDeviceSize alignedOffset = (offset / offsetAlignment) * offsetAlignment;

            // GL requires the atomic counter buffer offset to be aligned with uint.
            ASSERT((offset - alignedOffset) % sizeof(uint32_t) == 0);
            offsetDiff = static_cast<uint32_t>((offset - alignedOffset) / sizeof(uint32_t));

            // We expect offsetDiff to fit in an 8-bit value.  The maximum difference is
            // minStorageBufferOffsetAlignment / 4, where minStorageBufferOffsetAlignment
            // currently has a maximum value of 256 on any device.
            ASSERT(offsetDiff < (1 << 8));
        }

        // The output array is already cleared prior to this call.
        ASSERT(bufferIndex % 4 != 0 || offsetsOut[bufferIndex / 4] == 0);

        offsetsOut[bufferIndex / 4] |= static_cast<uint8_t>(offsetDiff) << ((bufferIndex % 4) * 8);
    }
}

class GraphicsDriverUniforms
{
  public:
    GraphicsDriverUniforms()
    {
        std::ranges::fill(mUniformData.acbBufferOffsets, 0);
        std::ranges::fill(mUniformData.depthRange, 0.0f);
        mUniformData.renderArea = 0;
        mUniformData.flipXY     = 0;
        mUniformData.dither     = 0;
        mUniformData.uint32Misc = 0;
    }

    void updateDepthRange(float nearPlane, float farPlane)
    {
        mUniformData.depthRange = {nearPlane, farPlane};
    }

    void updateRenderArea(int width, int height)
    {
        static_assert(gl::IMPLEMENTATION_MAX_FRAMEBUFFER_SIZE <= 0xFFFF,
                      "Not enough bits for render area");
        static_assert(gl::IMPLEMENTATION_MAX_RENDERBUFFER_SIZE <= 0xFFFF,
                      "Not enough bits for render area");

        uint16_t renderAreaWidth, renderAreaHeight;
        SetBitField(renderAreaWidth, width);
        SetBitField(renderAreaHeight, height);
        mUniformData.renderArea = renderAreaHeight << 16 | renderAreaWidth;
    }

    void updateflipXY(SurfaceRotation rotation,
                      bool viewportFlipped,
                      uint32_t numSamples,
                      uint32_t layeredFramebuffer)
    {
        bool flipX = false;
        bool flipY = false;
        // Y-axis flipping only comes into play with the default framebuffer (i.e. a swapchain
        // image). For 0-degree rotation, an FBO or pbuffer could be the draw framebuffer, and so we
        // must check whether flipY should be positive or negative.  All other rotations, will be to
        // the default framebuffer, and so the value of isViewportFlipEnabledForDrawFBO() is assumed
        // true; the appropriate flipY value is chosen such that gl_FragCoord is positioned at the
        // lower-left corner of the window.
        switch (rotation)
        {
            case SurfaceRotation::Identity:
                flipY = viewportFlipped;
                break;
            case SurfaceRotation::Rotated90Degrees:
                ASSERT(viewportFlipped);
                break;
            case SurfaceRotation::Rotated180Degrees:
                ASSERT(viewportFlipped);
                flipX = true;
                break;
            case SurfaceRotation::Rotated270Degrees:
                ASSERT(viewportFlipped);
                flipX = true;
                flipY = true;
                break;
            default:
                UNREACHABLE();
                break;
        }

        mUniformData.flipXY = MakeFlipUniform(flipX, flipY, viewportFlipped);

        const uint32_t swapXY = IsRotatedAspectRatio(rotation);
        SetBitField(mUniformData.misc.swapXY, swapXY);
        SetBitField(mUniformData.misc.numSamples, numSamples);
        SetBitField(mUniformData.misc.layeredFramebuffer, layeredFramebuffer);
    }

    void updateAtomicCounterBufferOffset(vk::Renderer *renderer,
                                         size_t atomicCounterBufferCount,
                                         const gl::BufferVector &atomicCounterBuffers)
    {
        UpdateAtomicCounterBufferOffset(renderer, atomicCounterBufferCount, atomicCounterBuffers,
                                        mUniformData.acbBufferOffsets);
    }

    void updateEmulatedDitherControl(uint32_t emulatedDitherControl)
    {
        mUniformData.dither = emulatedDitherControl;
    }

    void updateAdvancedBlendEquation(uint32_t advancedBlendEquation)
    {
        SetBitField(mUniformData.misc.advancedBlendEquation, advancedBlendEquation);
    }

    void updateEnabledClipDistances(uint32_t enabledClipDistances)
    {
        SetBitField(mUniformData.misc.clipDistancesEnabledMask, enabledClipDistances);
    }

    void updateTransformDepth(uint32_t transformDepth)
    {
        SetBitField(mUniformData.misc.transformDepth, transformDepth);
    }

    // Update push constant driver uniforms.
    void pushConstants(vk::Renderer *renderer,
                       const vk::PipelineLayout &pipelineLayout,
                       vk::RenderPassCommandBuffer *commandBuffer)
    {
        commandBuffer->pushConstants(pipelineLayout, renderer->getSupportedVulkanShaderStageMask(),
                                     0, sizeof(mUniformData), &mUniformData);
    }

    uint32_t getUniformDataSize() const { return sizeof(mUniformData); }
    uint32_t getRenderArea() const { return mUniformData.renderArea; }

  private:
    ANGLE_ENABLE_STRUCT_PADDING_WARNINGS
    struct UniformData
    {
        // Contain packed 8-bit values for atomic counter buffer offsets.  These offsets are within
        // Vulkan's minStorageBufferOffsetAlignment limit and are used to support unaligned offsets
        // allowed in GL.
        std::array<uint32_t, 2> acbBufferOffsets;

        // .x is near, .y is far
        std::array<float, 2> depthRange;

        // Used to flip gl_FragCoord.  Packed uvec2
        uint32_t renderArea;

        // Packed vec4 of snorm8
        uint32_t flipXY;

        // Only the lower 16 bits used
        uint32_t dither;

        // Packing information for driver uniform's misc field:
        union
        {
            struct
            {
                // - 1 bit for whether surface rotation results in swapped axes
                uint32_t swapXY : 1;
                static_assert(0x00000001 == sh::vk::kDriverUniformsMiscSwapXYMask);

                // - 5 bits for advanced blend equation
                uint32_t advancedBlendEquation : 5;
                static_assert((0x0000003E >>
                               sh::vk::kDriverUniformsMiscAdvancedBlendEquationOffset) ==
                              sh::vk::kDriverUniformsMiscAdvancedBlendEquationMask);

                // - 6 bits for sample count
                uint32_t numSamples : 6;
                static_assert((0x00000FC0 >> sh::vk::kDriverUniformsMiscSampleCountOffset) ==
                              sh::vk::kDriverUniformsMiscSampleCountMask);

                // - 8 bits for enabled clip planes
                uint32_t clipDistancesEnabledMask : 8;
                static_assert((0x000FF000 >> sh::vk::kDriverUniformsMiscEnabledClipPlanesOffset) ==
                              sh::vk::kDriverUniformsMiscEnabledClipPlanesMask);

                // - 1 bit for whether depth should be transformed to Vulkan clip space
                uint32_t transformDepth : 1;
                static_assert((0x00100000 >> sh::vk::kDriverUniformsMiscTransformDepthOffset) ==
                              sh::vk::kDriverUniformsMiscTransformDepthMask);

                // - 1 bit for whether alpha to coverage is enabled
                uint32_t alphaToCoverage : 1;
                static_assert((0x00200000 >> sh::vk::kDriverUniformsMiscAlphaToCoverageOffset) ==
                              sh::vk::kDriverUniformsMiscAlphaToCoverageMask);

                // - 1 bit for whether the framebuffer is layered
                uint32_t layeredFramebuffer : 1;
                static_assert((0x00400000 >> sh::vk::kDriverUniformsMiscLayeredFramebufferOffset) ==
                              sh::vk::kDriverUniformsMiscLayeredFramebufferMask);

                // - 9 bits unused
                uint32_t unused : 9;
            } misc;
            uint32_t uint32Misc;
        };
    } UniformData;
    ANGLE_DISABLE_STRUCT_PADDING_WARNINGS

    static_assert(sizeof(UniformData) % (sizeof(uint32_t) * 4) == 0,
                  "GraphicsDriverUniforms should be 16bytes aligned");

    static_assert(angle::BitMask<uint32_t>(gl::IMPLEMENTATION_MAX_CLIP_DISTANCES) <=
                      sh::vk::kDriverUniformsMiscEnabledClipPlanesMask,
                  "Not enough bits for enabled clip planes");

    struct UniformData mUniformData;
};

// Only used when transform feedback is emulated.
struct XFBEmulationGraphicsDriverUniforms
{
    std::array<int32_t, 4> xfbBufferOffsets;
    int32_t xfbVerticesPerInstance;
    int32_t padding[3];
};
static_assert(sizeof(XFBEmulationGraphicsDriverUniforms) % (sizeof(uint32_t) * 4) == 0,
              "GraphicsDriverUniformsExtended should be 16bytes aligned");
// Driver uniforms are updated using push constants and Vulkan spec guarantees universal support for
// 128 bytes worth of push constants. For maximum compatibility ensure
// GraphicsDriverUniforms plus extended size are within that limit.
static_assert(sizeof(GraphicsDriverUniforms) + sizeof(XFBEmulationGraphicsDriverUniforms) <= 128,
              "Only 128 bytes are guaranteed for push constants");

struct ComputeDriverUniforms
{
    // Atomic counter buffer offsets with the same layout as in GraphicsDriverUniforms.
    std::array<uint32_t, 4> acbBufferOffsets;
};
}  // namespace rx
#endif  // LIBANGLE_RENDERER_VULKAN_DRIVER_UNIFORMS_H_

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_utils:
//    Helper functions for the Vulkan Caps.
//

#include "libANGLE/renderer/vulkan/vk_caps_utils.h"

#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"

#include "vk_format_utils.h"

namespace
{
constexpr unsigned int kComponentsPerVector = 4;
}  // anonymous namespace

namespace rx
{
namespace vk
{

void GenerateCaps(const VkPhysicalDeviceProperties &physicalDeviceProperties,
                  const gl::TextureCapsMap &textureCaps,
                  gl::Caps *outCaps,
                  gl::Extensions *outExtensions,
                  gl::Limitations * /* outLimitations */)
{
    outExtensions->setTextureExtensionSupport(textureCaps);

    // Enable this for simple buffer readback testing, but some functionality is missing.
    // TODO(jmadill): Support full mapBufferRange extension.
    outExtensions->mapBuffer      = true;
    outExtensions->mapBufferRange = true;
    outExtensions->textureStorage = true;
    outExtensions->framebufferBlit = true;

    // TODO(lucferron): Eventually remove everything above this line in this function as the caps
    // get implemented.
    // https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s02.html
    outCaps->maxElementIndex       = std::numeric_limits<GLuint>::max() - 1;
    outCaps->max3DTextureSize      = physicalDeviceProperties.limits.maxImageDimension3D;
    outCaps->max2DTextureSize      = physicalDeviceProperties.limits.maxImageDimension2D;
    outCaps->maxArrayTextureLayers = physicalDeviceProperties.limits.maxImageArrayLayers;
    outCaps->maxLODBias            = physicalDeviceProperties.limits.maxSamplerLodBias;
    outCaps->maxCubeMapTextureSize = physicalDeviceProperties.limits.maxImageDimensionCube;
    outCaps->maxRenderbufferSize   = outCaps->max2DTextureSize;
    outCaps->minAliasedPointSize =
        std::max(1.0f, physicalDeviceProperties.limits.pointSizeRange[0]);
    outCaps->maxAliasedPointSize   = physicalDeviceProperties.limits.pointSizeRange[1];
    outCaps->minAliasedLineWidth   = physicalDeviceProperties.limits.lineWidthRange[0];
    outCaps->maxAliasedLineWidth   = physicalDeviceProperties.limits.lineWidthRange[1];
    outCaps->maxDrawBuffers =
        std::min<uint32_t>(physicalDeviceProperties.limits.maxColorAttachments,
                           physicalDeviceProperties.limits.maxFragmentOutputAttachments);
    outCaps->maxFramebufferWidth    = physicalDeviceProperties.limits.maxFramebufferWidth;
    outCaps->maxFramebufferHeight   = physicalDeviceProperties.limits.maxFramebufferHeight;
    outCaps->maxColorAttachments    = physicalDeviceProperties.limits.maxColorAttachments;
    outCaps->maxViewportWidth       = physicalDeviceProperties.limits.maxViewportDimensions[0];
    outCaps->maxViewportHeight      = physicalDeviceProperties.limits.maxViewportDimensions[1];
    outCaps->maxSampleMaskWords     = physicalDeviceProperties.limits.maxSampleMaskWords;
    outCaps->maxColorTextureSamples = physicalDeviceProperties.limits.sampledImageColorSampleCounts;
    outCaps->maxDepthTextureSamples = physicalDeviceProperties.limits.sampledImageDepthSampleCounts;
    outCaps->maxIntegerSamples = physicalDeviceProperties.limits.sampledImageIntegerSampleCounts;

    outCaps->maxVertexAttributes     = physicalDeviceProperties.limits.maxVertexInputAttributes;
    outCaps->maxVertexAttribBindings = physicalDeviceProperties.limits.maxVertexInputBindings;
    outCaps->maxVertexAttribRelativeOffset =
        physicalDeviceProperties.limits.maxVertexInputAttributeOffset;
    outCaps->maxVertexAttribStride = physicalDeviceProperties.limits.maxVertexInputBindingStride;

    outCaps->maxElementsIndices  = std::numeric_limits<GLuint>::max();
    outCaps->maxElementsVertices = std::numeric_limits<GLuint>::max();

    // Looks like all floats are IEEE according to the docs here:
    // https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#spirvenv-precision-operation
    outCaps->vertexHighpFloat.setIEEEFloat();
    outCaps->vertexMediumpFloat.setIEEEFloat();
    outCaps->vertexLowpFloat.setIEEEFloat();
    outCaps->fragmentHighpFloat.setIEEEFloat();
    outCaps->fragmentMediumpFloat.setIEEEFloat();
    outCaps->fragmentLowpFloat.setIEEEFloat();

    // Can't find documentation on the int precision in Vulkan.
    outCaps->vertexHighpInt.setTwosComplementInt(32);
    outCaps->vertexMediumpInt.setTwosComplementInt(32);
    outCaps->vertexLowpInt.setTwosComplementInt(32);
    outCaps->fragmentHighpInt.setTwosComplementInt(32);
    outCaps->fragmentMediumpInt.setTwosComplementInt(32);
    outCaps->fragmentLowpInt.setTwosComplementInt(32);

    // TODO(lucferron): This is something we'll need to implement custom in the back-end.
    // Vulkan doesn't do any waiting for you, our back-end code is going to manage sync objects,
    // and we'll have to check that we've exceeded the max wait timeout. Also, this is ES 3.0 so
    // we'll defer the implementation until we tackle the next version.
    // outCaps->maxServerWaitTimeout

    GLuint maxUniformVectors = physicalDeviceProperties.limits.maxUniformBufferRange /
                               (sizeof(GLfloat) * kComponentsPerVector);

    // Clamp the maxUniformVectors to 1024u, on AMD the maxUniformBufferRange is way too high.
    maxUniformVectors = std::min(1024u, maxUniformVectors);

    const GLuint maxUniformComponents = maxUniformVectors * kComponentsPerVector;

    // Uniforms are implemented using a uniform buffer, so the max number of uniforms we can
    // support is the max buffer range divided by the size of a single uniform (4X float).
    outCaps->maxVertexUniformVectors      = maxUniformVectors;
    outCaps->maxShaderUniformComponents[gl::ShaderType::Vertex]   = maxUniformComponents;
    outCaps->maxFragmentUniformVectors    = maxUniformVectors;
    outCaps->maxShaderUniformComponents[gl::ShaderType::Fragment] = maxUniformComponents;

    // TODO(jmadill): this is an ES 3.0 property and we can skip implementing it for now.
    // This is maxDescriptorSetUniformBuffers minus the number of uniform buffers we
    // reserve for internal variables. We reserve one per shader stage for default uniforms
    // and likely one per shader stage for ANGLE internal variables.
    // outCaps->maxShaderUniformBlocks[gl::ShaderType::Vertex] = ...

    // we use the same bindings on each stage, so the limitation is the same combined or not.
    outCaps->maxCombinedTextureImageUnits =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;
    outCaps->maxShaderTextureImageUnits[gl::ShaderType::Fragment] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;
    outCaps->maxShaderTextureImageUnits[gl::ShaderType::Vertex] =
        physicalDeviceProperties.limits.maxPerStageDescriptorSamplers;

    // The max vertex output components should not include gl_Position.
    // The gles2.0 section 2.10 states that "gl_Position is not a varying variable and does
    // not count against this limit.", but the Vulkan spec has no such mention in its Built-in
    // vars section. It is implicit that we need to actually reserve it for Vulkan in that case.
    // TODO(lucferron): AMD has a weird behavior when we edge toward the maximum number of varyings
    // and can often crash. Reserving an additional varying just for them bringing the total to 2.
    // http://anglebug.com/2483
    constexpr GLint kReservedVaryingCount = 2;
    outCaps->maxVaryingVectors =
        (physicalDeviceProperties.limits.maxVertexOutputComponents / 4) - kReservedVaryingCount;
    outCaps->maxVertexOutputComponents = outCaps->maxVaryingVectors * 4;
}
}  // namespace vk

namespace egl_vk
{
egl::Config GenerateDefaultConfig(const gl::InternalFormat &colorFormat,
                                  const gl::InternalFormat &depthStencilFormat,
                                  EGLint sampleCount)
{
    egl::Config config;

    config.renderTargetFormat    = colorFormat.internalFormat;
    config.depthStencilFormat    = depthStencilFormat.internalFormat;
    config.bufferSize            = colorFormat.pixelBytes * 8;
    config.redSize               = colorFormat.redBits;
    config.greenSize             = colorFormat.greenBits;
    config.blueSize              = colorFormat.blueBits;
    config.alphaSize             = colorFormat.alphaBits;
    config.alphaMaskSize         = 0;
    config.bindToTextureRGB      = EGL_FALSE;
    config.bindToTextureRGBA     = EGL_FALSE;
    config.colorBufferType       = EGL_RGB_BUFFER;
    config.configCaveat          = EGL_NONE;
    config.conformant            = 0;
    config.depthSize             = depthStencilFormat.depthBits;
    config.stencilSize           = depthStencilFormat.stencilBits;
    config.level                 = 0;
    config.matchNativePixmap     = EGL_NONE;
    config.maxPBufferWidth       = 0;
    config.maxPBufferHeight      = 0;
    config.maxPBufferPixels      = 0;
    config.maxSwapInterval       = 1;
    config.minSwapInterval       = 1;
    config.nativeRenderable      = EGL_TRUE;
    config.nativeVisualID        = 0;
    config.nativeVisualType      = EGL_NONE;
    config.renderableType        = EGL_OPENGL_ES2_BIT;
    config.sampleBuffers         = (sampleCount > 0) ? 1 : 0;
    config.samples               = sampleCount;
    config.surfaceType           = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
    config.optimalOrientation    = 0;
    config.transparentType       = EGL_NONE;
    config.transparentRedValue   = 0;
    config.transparentGreenValue = 0;
    config.transparentBlueValue  = 0;
    config.colorComponentType =
        gl_egl::GLComponentTypeToEGLColorComponentType(colorFormat.componentType);

    return config;
}

egl::ConfigSet GenerateConfigs(const GLenum *colorFormats,
                               size_t colorFormatsCount,
                               const GLenum *depthStencilFormats,
                               size_t depthStencilFormatCount,
                               const EGLint *sampleCounts,
                               size_t sampleCountsCount,
                               DisplayVk *display)
{
    ASSERT(colorFormatsCount > 0);
    ASSERT(display != nullptr);

    egl::ConfigSet configSet;

    for (size_t colorFormatIdx = 0; colorFormatIdx < colorFormatsCount; colorFormatIdx++)
    {
        const gl::InternalFormat &colorFormatInfo =
            gl::GetSizedInternalFormatInfo(colorFormats[colorFormatIdx]);
        ASSERT(colorFormatInfo.sized);

        for (size_t depthStencilFormatIdx = 0; depthStencilFormatIdx < depthStencilFormatCount;
             depthStencilFormatIdx++)
        {
            const gl::InternalFormat &depthStencilFormatInfo =
                gl::GetSizedInternalFormatInfo(depthStencilFormats[depthStencilFormatIdx]);
            ASSERT(depthStencilFormats[depthStencilFormatIdx] == GL_NONE ||
                   depthStencilFormatInfo.sized);

            for (size_t sampleCountIndex = 0; sampleCountIndex < sampleCountsCount;
                 sampleCountIndex++)
            {
                egl::Config config = GenerateDefaultConfig(colorFormatInfo, depthStencilFormatInfo,
                                                           sampleCounts[sampleCountIndex]);
                if (display->checkConfigSupport(&config))
                {
                    configSet.add(config);
                }
            }
        }
    }

    return configSet;
}

}  // namespace egl_vk

}  // namespace rx

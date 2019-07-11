//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_utils:
//    Helper functions for the Vulkan Caps.
//

#include "libANGLE/renderer/vulkan/vk_caps_utils.h"

#include <type_traits>

#include "common/utilities.h"
#include "libANGLE/Caps.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "vk_format_utils.h"

namespace
{
constexpr unsigned int kComponentsPerVector = 4;
}  // anonymous namespace

namespace rx
{

void RendererVk::ensureCapsInitialized() const
{
    if (mCapsInitialized)
        return;
    mCapsInitialized = true;

    ASSERT(mCurrentQueueFamilyIndex < mQueueFamilyProperties.size());
    const VkQueueFamilyProperties &queueFamilyProperties =
        mQueueFamilyProperties[mCurrentQueueFamilyIndex];

    mNativeExtensions.setTextureExtensionSupport(mNativeTextureCaps);

    // Enable this for simple buffer readback testing, but some functionality is missing.
    // TODO(jmadill): Support full mapBufferRange extension.
    mNativeExtensions.mapBuffer              = true;
    mNativeExtensions.mapBufferRange         = true;
    mNativeExtensions.textureStorage         = true;
    mNativeExtensions.drawBuffers            = true;
    mNativeExtensions.fragDepth              = true;
    mNativeExtensions.framebufferBlit        = true;
    mNativeExtensions.framebufferMultisample = true;
    mNativeExtensions.copyTexture            = true;
    mNativeExtensions.copyCompressedTexture  = true;
    mNativeExtensions.debugMarker            = true;
    mNativeExtensions.robustness             = true;
    mNativeExtensions.textureBorderClamp     = false;  // not implemented yet
    mNativeExtensions.translatedShaderSource = true;
    mNativeExtensions.discardFramebuffer     = true;

    // Enable EXT_blend_minmax
    mNativeExtensions.blendMinMax = true;

    mNativeExtensions.eglImage         = true;
    mNativeExtensions.eglImageExternal = true;
    // TODO(geofflang): Support GL_OES_EGL_image_external_essl3. http://anglebug.com/2668
    mNativeExtensions.eglImageExternalEssl3 = false;

    mNativeExtensions.memoryObject   = true;
    mNativeExtensions.memoryObjectFd = getFeatures().supportsExternalMemoryFd.enabled;

    mNativeExtensions.semaphore   = true;
    mNativeExtensions.semaphoreFd = getFeatures().supportsExternalSemaphoreFd.enabled;

    // TODO: Enable this always and emulate instanced draws if any divisor exceeds the maximum
    // supported.  http://anglebug.com/2672
    mNativeExtensions.instancedArraysANGLE = mMaxVertexAttribDivisor > 1;

    // Only expose robust buffer access if the physical device supports it.
    mNativeExtensions.robustBufferAccessBehavior = mPhysicalDeviceFeatures.robustBufferAccess;

    mNativeExtensions.eglSync = true;

    // We use secondary command buffers almost everywhere and they require a feature to be
    // able to execute in the presence of queries.  As a result, we won't support queries
    // unless that feature is available.
    mNativeExtensions.occlusionQueryBoolean =
        vk::CommandBuffer::SupportsQueries(mPhysicalDeviceFeatures);

    // From the Vulkan specs:
    // > The number of valid bits in a timestamp value is determined by the
    // > VkQueueFamilyProperties::timestampValidBits property of the queue on which the timestamp is
    // > written. Timestamps are supported on any queue which reports a non-zero value for
    // > timestampValidBits via vkGetPhysicalDeviceQueueFamilyProperties.
    mNativeExtensions.disjointTimerQuery          = queueFamilyProperties.timestampValidBits > 0;
    mNativeExtensions.queryCounterBitsTimeElapsed = queueFamilyProperties.timestampValidBits;
    mNativeExtensions.queryCounterBitsTimestamp   = queueFamilyProperties.timestampValidBits;

    mNativeExtensions.textureFilterAnisotropic =
        mPhysicalDeviceFeatures.samplerAnisotropy &&
        mPhysicalDeviceProperties.limits.maxSamplerAnisotropy > 1.0f;
    mNativeExtensions.maxTextureAnisotropy =
        mNativeExtensions.textureFilterAnisotropic
            ? mPhysicalDeviceProperties.limits.maxSamplerAnisotropy
            : 0.0f;

    // Vulkan natively supports non power-of-two textures
    mNativeExtensions.textureNPOT = true;

    mNativeExtensions.texture3DOES = true;

    // Vulkan natively supports standard derivatives
    mNativeExtensions.standardDerivatives = true;

    // Vulkan natively supports 32-bit indices, entry in kIndexTypeMap
    mNativeExtensions.elementIndexUint = true;

    // https://vulkan.lunarg.com/doc/view/1.0.30.0/linux/vkspec.chunked/ch31s02.html
    mNativeCaps.maxElementIndex       = std::numeric_limits<GLuint>::max() - 1;
    mNativeCaps.max3DTextureSize      = mPhysicalDeviceProperties.limits.maxImageDimension3D;
    mNativeCaps.max2DTextureSize      = mPhysicalDeviceProperties.limits.maxImageDimension2D;
    mNativeCaps.maxArrayTextureLayers = mPhysicalDeviceProperties.limits.maxImageArrayLayers;
    mNativeCaps.maxLODBias            = mPhysicalDeviceProperties.limits.maxSamplerLodBias;
    mNativeCaps.maxCubeMapTextureSize = mPhysicalDeviceProperties.limits.maxImageDimensionCube;
    mNativeCaps.maxRenderbufferSize   = mNativeCaps.max2DTextureSize;
    mNativeCaps.minAliasedPointSize =
        std::max(1.0f, mPhysicalDeviceProperties.limits.pointSizeRange[0]);
    mNativeCaps.maxAliasedPointSize = mPhysicalDeviceProperties.limits.pointSizeRange[1];

    mNativeCaps.minAliasedLineWidth = 1.0f;
    mNativeCaps.maxAliasedLineWidth = 1.0f;

    mNativeCaps.maxDrawBuffers =
        std::min<uint32_t>(mPhysicalDeviceProperties.limits.maxColorAttachments,
                           mPhysicalDeviceProperties.limits.maxFragmentOutputAttachments);
    mNativeCaps.maxFramebufferWidth  = mPhysicalDeviceProperties.limits.maxFramebufferWidth;
    mNativeCaps.maxFramebufferHeight = mPhysicalDeviceProperties.limits.maxFramebufferHeight;
    mNativeCaps.maxColorAttachments  = mPhysicalDeviceProperties.limits.maxColorAttachments;
    mNativeCaps.maxViewportWidth     = mPhysicalDeviceProperties.limits.maxViewportDimensions[0];
    mNativeCaps.maxViewportHeight    = mPhysicalDeviceProperties.limits.maxViewportDimensions[1];
    mNativeCaps.maxSampleMaskWords   = mPhysicalDeviceProperties.limits.maxSampleMaskWords;
    mNativeCaps.maxColorTextureSamples =
        mPhysicalDeviceProperties.limits.sampledImageColorSampleCounts;
    mNativeCaps.maxDepthTextureSamples =
        mPhysicalDeviceProperties.limits.sampledImageDepthSampleCounts;
    mNativeCaps.maxIntegerSamples =
        mPhysicalDeviceProperties.limits.sampledImageIntegerSampleCounts;

    mNativeCaps.maxVertexAttributes     = mPhysicalDeviceProperties.limits.maxVertexInputAttributes;
    mNativeCaps.maxVertexAttribBindings = mPhysicalDeviceProperties.limits.maxVertexInputBindings;
    mNativeCaps.maxVertexAttribRelativeOffset =
        mPhysicalDeviceProperties.limits.maxVertexInputAttributeOffset;
    mNativeCaps.maxVertexAttribStride =
        mPhysicalDeviceProperties.limits.maxVertexInputBindingStride;

    mNativeCaps.maxElementsIndices  = std::numeric_limits<GLuint>::max();
    mNativeCaps.maxElementsVertices = std::numeric_limits<GLuint>::max();

    // Looks like all floats are IEEE according to the docs here:
    // https://www.khronos.org/registry/vulkan/specs/1.0-wsi_extensions/html/vkspec.html#spirvenv-precision-operation
    mNativeCaps.vertexHighpFloat.setIEEEFloat();
    mNativeCaps.vertexMediumpFloat.setIEEEFloat();
    mNativeCaps.vertexLowpFloat.setIEEEFloat();
    mNativeCaps.fragmentHighpFloat.setIEEEFloat();
    mNativeCaps.fragmentMediumpFloat.setIEEEFloat();
    mNativeCaps.fragmentLowpFloat.setIEEEFloat();

    // Can't find documentation on the int precision in Vulkan.
    mNativeCaps.vertexHighpInt.setTwosComplementInt(32);
    mNativeCaps.vertexMediumpInt.setTwosComplementInt(32);
    mNativeCaps.vertexLowpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentHighpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentMediumpInt.setTwosComplementInt(32);
    mNativeCaps.fragmentLowpInt.setTwosComplementInt(32);

    // TODO(lucferron): This is something we'll need to implement custom in the back-end.
    // Vulkan doesn't do any waiting for you, our back-end code is going to manage sync objects,
    // and we'll have to check that we've exceeded the max wait timeout. Also, this is ES 3.0 so
    // we'll defer the implementation until we tackle the next version.
    // mNativeCaps.maxServerWaitTimeout

    GLuint maxUniformBlockSize = mPhysicalDeviceProperties.limits.maxUniformBufferRange;

    // Clamp the maxUniformBlockSize to 64KB (majority of devices support up to this size
    // currently), on AMD the maxUniformBufferRange is near uint32_t max.
    maxUniformBlockSize = std::min(0x10000u, maxUniformBlockSize);

    const GLuint maxUniformVectors = maxUniformBlockSize / (sizeof(GLfloat) * kComponentsPerVector);
    const GLuint maxUniformComponents = maxUniformVectors * kComponentsPerVector;

    // Uniforms are implemented using a uniform buffer, so the max number of uniforms we can
    // support is the max buffer range divided by the size of a single uniform (4X float).
    mNativeCaps.maxVertexUniformVectors                              = maxUniformVectors;
    mNativeCaps.maxShaderUniformComponents[gl::ShaderType::Vertex]   = maxUniformComponents;
    mNativeCaps.maxFragmentUniformVectors                            = maxUniformVectors;
    mNativeCaps.maxShaderUniformComponents[gl::ShaderType::Fragment] = maxUniformComponents;

    // Every stage has 1 reserved uniform buffer for the default uniforms, and 1 for the driver
    // uniforms.
    constexpr uint32_t kTotalReservedPerStageUniformBuffers =
        kReservedDriverUniformBindingCount + kReservedPerStageDefaultUniformBindingCount;
    constexpr uint32_t kTotalReservedUniformBuffers =
        kReservedDriverUniformBindingCount + kReservedDefaultUniformBindingCount;

    const uint32_t maxPerStageUniformBuffers =
        mPhysicalDeviceProperties.limits.maxPerStageDescriptorUniformBuffers -
        kTotalReservedPerStageUniformBuffers;
    const uint32_t maxCombinedUniformBuffers =
        mPhysicalDeviceProperties.limits.maxDescriptorSetUniformBuffers -
        kTotalReservedUniformBuffers;
    mNativeCaps.maxShaderUniformBlocks[gl::ShaderType::Vertex]   = maxPerStageUniformBuffers;
    mNativeCaps.maxShaderUniformBlocks[gl::ShaderType::Fragment] = maxPerStageUniformBuffers;
    mNativeCaps.maxCombinedUniformBlocks                         = maxCombinedUniformBuffers;

    mNativeCaps.maxUniformBufferBindings = maxCombinedUniformBuffers;
    mNativeCaps.maxUniformBlockSize      = maxUniformBlockSize;
    mNativeCaps.uniformBufferOffsetAlignment =
        static_cast<GLuint>(mPhysicalDeviceProperties.limits.minUniformBufferOffsetAlignment);

    // Note that Vulkan currently implements textures as combined image+samplers, so the limit is
    // the minimum of supported samplers and sampled images.
    const uint32_t maxPerStageTextures =
        std::min(mPhysicalDeviceProperties.limits.maxPerStageDescriptorSamplers,
                 mPhysicalDeviceProperties.limits.maxPerStageDescriptorSampledImages);
    const uint32_t maxCombinedTextures =
        std::min(mPhysicalDeviceProperties.limits.maxDescriptorSetSamplers,
                 mPhysicalDeviceProperties.limits.maxDescriptorSetSampledImages);
    mNativeCaps.maxShaderTextureImageUnits[gl::ShaderType::Vertex]   = maxPerStageTextures;
    mNativeCaps.maxShaderTextureImageUnits[gl::ShaderType::Fragment] = maxPerStageTextures;
    mNativeCaps.maxCombinedTextureImageUnits                         = maxCombinedTextures;

    const uint32_t maxPerStageStorageBuffers =
        mPhysicalDeviceProperties.limits.maxPerStageDescriptorStorageBuffers;
    const uint32_t maxCombinedStorageBuffers =
        mPhysicalDeviceProperties.limits.maxDescriptorSetStorageBuffers;
    mNativeCaps.maxShaderStorageBlocks[gl::ShaderType::Vertex] =
        mPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics ? maxPerStageStorageBuffers : 0;
    mNativeCaps.maxShaderStorageBlocks[gl::ShaderType::Fragment] =
        mPhysicalDeviceFeatures.fragmentStoresAndAtomics ? maxPerStageStorageBuffers : 0;
    mNativeCaps.maxCombinedShaderStorageBlocks = maxCombinedStorageBuffers;

    // A number of storage buffer slots are used in the vertex shader to emulate transform feedback.
    // Note that Vulkan requires maxPerStageDescriptorStorageBuffers to be at least 4 (i.e. the same
    // as gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS).
    // TODO(syoussefi): This should be conditioned to transform feedback extension not being
    // present.  http://anglebug.com/3206.
    // TODO(syoussefi): If geometry shader is supported, emulation will be done at that stage, and
    // so the reserved storage buffers should be accounted in that stage.  http://anglebug.com/3606
    static_assert(
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS == 4,
        "Limit to ES2.0 if supported SSBO count < supporting transform feedback buffer count");
    if (mPhysicalDeviceFeatures.vertexPipelineStoresAndAtomics)
    {
        ASSERT(maxPerStageStorageBuffers >= gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS);
        mNativeCaps.maxShaderStorageBlocks[gl::ShaderType::Vertex] -=
            gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS;
        mNativeCaps.maxCombinedShaderStorageBlocks -=
            gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS;
    }

    mNativeCaps.maxShaderStorageBufferBindings = maxCombinedStorageBuffers;
    mNativeCaps.maxShaderStorageBlockSize = mPhysicalDeviceProperties.limits.maxStorageBufferRange;
    mNativeCaps.shaderStorageBufferOffsetAlignment =
        static_cast<GLuint>(mPhysicalDeviceProperties.limits.minStorageBufferOffsetAlignment);

    mNativeCaps.minProgramTexelOffset = mPhysicalDeviceProperties.limits.minTexelOffset;
    mNativeCaps.maxProgramTexelOffset = mPhysicalDeviceProperties.limits.maxTexelOffset;

    // There is no additional limit to the combined number of components.  We can have up to a
    // maximum number of uniform buffers, each having the maximum number of components.  Note that
    // this limit includes both components in and out of uniform buffers.
    const uint32_t maxCombinedUniformComponents =
        (maxPerStageUniformBuffers + kReservedPerStageDefaultUniformBindingCount) *
        maxUniformComponents;
    for (gl::ShaderType shaderType : gl::kAllGraphicsShaderTypes)
    {
        mNativeCaps.maxCombinedShaderUniformComponents[shaderType] = maxCombinedUniformComponents;
    }

    // Total number of resources available to the user are as many as Vulkan allows minus everything
    // that ANGLE uses internally.  That is, one dynamic uniform buffer used per stage for default
    // uniforms and a single dynamic uniform buffer for driver uniforms.  Additionally, Vulkan uses
    // up to IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS + 1 buffers for transform feedback (Note:
    // +1 is for the "counter" buffer of transform feedback, which will be necessary for transform
    // feedback extension and ES3.2 transform feedback emulation, but is not yet present).
    constexpr uint32_t kReservedPerStageUniformBufferCount = 1;
    constexpr uint32_t kReservedPerStageBindingCount =
        kReservedDriverUniformBindingCount + kReservedPerStageUniformBufferCount +
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_BUFFERS + 1;

    // Note: maxPerStageResources is required to be at least the sum of per stage UBOs, SSBOs etc
    // which total a minimum of 44 resources, so no underflow is possible here.  Limit the total
    // number of resources reported by Vulkan to 2 billion though to avoid seeing negative numbers
    // in applications that take the value as signed int (including dEQP).
    const uint32_t maxPerStageResources = std::min<uint32_t>(
        std::numeric_limits<int32_t>::max(), mPhysicalDeviceProperties.limits.maxPerStageResources);
    mNativeCaps.maxCombinedShaderOutputResources =
        maxPerStageResources - kReservedPerStageBindingCount;

    // The max vertex output components should not include gl_Position.
    // The gles2.0 section 2.10 states that "gl_Position is not a varying variable and does
    // not count against this limit.", but the Vulkan spec has no such mention in its Built-in
    // vars section. It is implicit that we need to actually reserve it for Vulkan in that case.
    //
    // Note: AMD has a weird behavior when we edge toward the maximum number of varyings and can
    // often crash. Reserving an additional varying just for them bringing the total to 2.
    constexpr GLint kReservedVaryingCount = 2;
    mNativeCaps.maxVaryingVectors =
        (mPhysicalDeviceProperties.limits.maxVertexOutputComponents / 4) - kReservedVaryingCount;
    mNativeCaps.maxVertexOutputComponents = mNativeCaps.maxVaryingVectors * 4;

    mNativeCaps.maxTransformFeedbackInterleavedComponents =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_INTERLEAVED_COMPONENTS;
    mNativeCaps.maxTransformFeedbackSeparateAttributes =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_SEPARATE_ATTRIBS;
    mNativeCaps.maxTransformFeedbackSeparateComponents =
        gl::IMPLEMENTATION_MAX_TRANSFORM_FEEDBACK_SEPARATE_COMPONENTS;

    const VkPhysicalDeviceLimits &limits = mPhysicalDeviceProperties.limits;
    const uint32_t sampleCounts          = limits.framebufferColorSampleCounts &
                                  limits.framebufferDepthSampleCounts &
                                  limits.framebufferStencilSampleCounts;

    mNativeCaps.maxSamples = vk_gl::GetMaxSampleCount(sampleCounts);

    mNativeCaps.subPixelBits = mPhysicalDeviceProperties.limits.subPixelPrecisionBits;
}

namespace egl_vk
{

namespace
{

EGLint ComputeMaximumPBufferPixels(const VkPhysicalDeviceProperties &physicalDeviceProperties)
{
    // EGLints are signed 32-bit integers, it's fairly easy to overflow them, especially since
    // Vulkan's minimum guaranteed VkImageFormatProperties::maxResourceSize is 2^31 bytes.
    constexpr uint64_t kMaxValueForEGLint =
        static_cast<uint64_t>(std::numeric_limits<EGLint>::max());

    // TODO(geofflang): Compute the maximum size of a pbuffer by using the maxResourceSize result
    // from vkGetPhysicalDeviceImageFormatProperties for both the color and depth stencil format and
    // the exact image creation parameters that would be used to create the pbuffer. Because it is
    // always safe to return out-of-memory errors on pbuffer allocation, it's fine to simply return
    // the number of pixels in a max width by max height pbuffer for now. http://anglebug.com/2622

    // Storing the result of squaring a 32-bit unsigned int in a 64-bit unsigned int is safe.
    static_assert(std::is_same<decltype(physicalDeviceProperties.limits.maxImageDimension2D),
                               uint32_t>::value,
                  "physicalDeviceProperties.limits.maxImageDimension2D expected to be a uint32_t.");
    const uint64_t maxDimensionsSquared =
        static_cast<uint64_t>(physicalDeviceProperties.limits.maxImageDimension2D) *
        static_cast<uint64_t>(physicalDeviceProperties.limits.maxImageDimension2D);

    return static_cast<EGLint>(std::min(maxDimensionsSquared, kMaxValueForEGLint));
}

// Generates a basic config for a combination of color format, depth stencil format and sample
// count.
egl::Config GenerateDefaultConfig(const RendererVk *renderer,
                                  const gl::InternalFormat &colorFormat,
                                  const gl::InternalFormat &depthStencilFormat,
                                  EGLint sampleCount)
{
    const VkPhysicalDeviceProperties &physicalDeviceProperties =
        renderer->getPhysicalDeviceProperties();
    gl::Version maxSupportedESVersion = renderer->getMaxSupportedESVersion();

    EGLint es2Support = (maxSupportedESVersion.major >= 2 ? EGL_OPENGL_ES2_BIT : 0);
    EGLint es3Support = (maxSupportedESVersion.major >= 3 ? EGL_OPENGL_ES3_BIT : 0);

    egl::Config config;

    config.renderTargetFormat = colorFormat.internalFormat;
    config.depthStencilFormat = depthStencilFormat.internalFormat;
    config.bufferSize         = colorFormat.pixelBytes * 8;
    config.redSize            = colorFormat.redBits;
    config.greenSize          = colorFormat.greenBits;
    config.blueSize           = colorFormat.blueBits;
    config.alphaSize          = colorFormat.alphaBits;
    config.alphaMaskSize      = 0;
    config.bindToTextureRGB   = colorFormat.format == GL_RGB;
    config.bindToTextureRGBA  = colorFormat.format == GL_RGBA || colorFormat.format == GL_BGRA_EXT;
    config.colorBufferType    = EGL_RGB_BUFFER;
    config.configCaveat       = EGL_NONE;
    config.conformant         = es2Support | es3Support;
    config.depthSize          = depthStencilFormat.depthBits;
    config.stencilSize        = depthStencilFormat.stencilBits;
    config.level              = 0;
    config.matchNativePixmap  = EGL_NONE;
    config.maxPBufferWidth    = physicalDeviceProperties.limits.maxImageDimension2D;
    config.maxPBufferHeight   = physicalDeviceProperties.limits.maxImageDimension2D;
    config.maxPBufferPixels   = ComputeMaximumPBufferPixels(physicalDeviceProperties);
    config.maxSwapInterval    = 1;
    config.minSwapInterval    = 0;
    config.nativeRenderable   = EGL_TRUE;
    config.nativeVisualID     = 0;
    config.nativeVisualType   = EGL_NONE;
    config.renderableType     = es2Support | es3Support;
    config.sampleBuffers      = (sampleCount > 0) ? 1 : 0;
    config.samples            = sampleCount;
    config.surfaceType        = EGL_WINDOW_BIT | EGL_PBUFFER_BIT;
    // Vulkan surfaces use a different origin than OpenGL, always prefer to be flipped vertically if
    // possible.
    config.optimalOrientation    = EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE;
    config.transparentType       = EGL_NONE;
    config.transparentRedValue   = 0;
    config.transparentGreenValue = 0;
    config.transparentBlueValue  = 0;
    config.colorComponentType =
        gl_egl::GLComponentTypeToEGLColorComponentType(colorFormat.componentType);

    return config;
}

}  // anonymous namespace

egl::ConfigSet GenerateConfigs(const GLenum *colorFormats,
                               size_t colorFormatsCount,
                               const GLenum *depthStencilFormats,
                               size_t depthStencilFormatCount,
                               DisplayVk *display)
{
    ASSERT(colorFormatsCount > 0);
    ASSERT(display != nullptr);

    gl::SupportedSampleSet colorSampleCounts;
    gl::SupportedSampleSet depthStencilSampleCounts;
    gl::SupportedSampleSet sampleCounts;

    const VkPhysicalDeviceLimits &limits =
        display->getRenderer()->getPhysicalDeviceProperties().limits;
    const uint32_t depthStencilSampleCountsLimit =
        limits.framebufferDepthSampleCounts & limits.framebufferStencilSampleCounts;

    vk_gl::AddSampleCounts(limits.framebufferColorSampleCounts, &colorSampleCounts);
    vk_gl::AddSampleCounts(depthStencilSampleCountsLimit, &depthStencilSampleCounts);

    // Always support 0 samples
    colorSampleCounts.insert(0);
    depthStencilSampleCounts.insert(0);

    std::set_intersection(colorSampleCounts.begin(), colorSampleCounts.end(),
                          depthStencilSampleCounts.begin(), depthStencilSampleCounts.end(),
                          std::inserter(sampleCounts, sampleCounts.begin()));

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

            const gl::SupportedSampleSet *configSampleCounts = &sampleCounts;
            // If there is no depth/stencil buffer, use the color samples set.
            if (depthStencilFormats[depthStencilFormatIdx] == GL_NONE)
            {
                configSampleCounts = &colorSampleCounts;
            }
            // If there is no color buffer, use the depth/stencil samples set.
            else if (colorFormats[colorFormatIdx] == GL_NONE)
            {
                configSampleCounts = &depthStencilSampleCounts;
            }

            for (EGLint sampleCount : *configSampleCounts)
            {
                egl::Config config = GenerateDefaultConfig(display->getRenderer(), colorFormatInfo,
                                                           depthStencilFormatInfo, sampleCount);
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

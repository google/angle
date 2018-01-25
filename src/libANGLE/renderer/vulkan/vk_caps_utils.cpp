//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_utils:
//    Helper functions for the Vulkan Caps.
//

#include "libANGLE/renderer/vulkan/vk_caps_utils.h"
#include "libANGLE/Caps.h"

namespace rx
{

namespace vk
{

void GenerateCaps(const VkPhysicalDeviceProperties &physicalDeviceProperties,
                  gl::Caps *outCaps,
                  gl::TextureCapsMap * /*outTextureCaps*/,
                  gl::Extensions *outExtensions,
                  gl::Limitations * /* outLimitations */)
{
    // TODO(jmadill): Caps.
    outCaps->maxVertexAttributes          = gl::MAX_VERTEX_ATTRIBS;
    outCaps->maxVertexAttribBindings      = gl::MAX_VERTEX_ATTRIB_BINDINGS;
    outCaps->maxVaryingVectors            = 16;
    outCaps->maxTextureImageUnits         = 1;
    outCaps->maxCombinedTextureImageUnits = 1;
    outCaps->maxFragmentUniformVectors    = 8;
    outCaps->maxVertexUniformVectors      = 8;

    // Enable this for simple buffer readback testing, but some functionality is missing.
    // TODO(jmadill): Support full mapBufferRange extension.
    outExtensions->mapBuffer      = true;
    outExtensions->mapBufferRange = true;

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
    outCaps->minAliasedPointSize   = physicalDeviceProperties.limits.pointSizeRange[0];
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

    // TODO(lucferron): This is something we'll need to implement custom in the back-end.
    // Vulkan doesn't do any waiting for you, our back-end code is going to manage sync objects,
    // and we'll have to check that we've exceeded the max wait timeout. Alsom this is ES 3.0 so
    // we'll defer the implementation until we tackle the next version.
    // outCaps->maxServerWaitTimeout
}
}  // namespace vk
}  // namespace rx
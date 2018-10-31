//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FeaturesVk.h: Optional features for the Vulkan renderer.
//

#ifndef ANGLE_PLATFORM_FEATURESVK_H_
#define ANGLE_PLATFORM_FEATURESVK_H_

namespace angle
{

struct FeaturesVk
{
    // Line segment rasterization must follow OpenGL rules. This means using an algorithm similar
    // to Bresenham's. Vulkan uses a different algorithm. This feature enables the use of pixel
    // shader patching to implement OpenGL basic line rasterization rules. This feature will
    // normally always be enabled. Exposing it as an option enables performance testing.
    bool basicGLLineRasterization = false;

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
    bool flipViewportY = false;

    // Add an extra copy region when using vkCmdCopyBuffer as the Windows Intel driver seems
    // to have a bug where the last region is ignored.
    bool extraCopyBufferRegion = false;

    // This flag is added for the sole purpose of end2end tests, to test the correctness
    // of various algorithms when a fallback format is used, such as using a packed format to
    // emulate a depth- or stencil-only format.
    bool forceFallbackFormat = false;
};

}  // namespace angle

#endif  // ANGLE_PLATFORM_FEATURESVK_H_

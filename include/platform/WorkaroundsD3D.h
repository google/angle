//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WorkaroundsD3D.h: Workarounds for D3D driver bugs and other issues.

#ifndef ANGLE_PLATFORM_WORKAROUNDSD3D_H_
#define ANGLE_PLATFORM_WORKAROUNDSD3D_H_

#include "platform/Feature.h"

namespace angle
{

// Workarounds attached to each shader. Do not need to expose information about these workarounds so
// a simple bool struct suffices.
struct CompilerWorkaroundsD3D
{
    bool skipOptimization = false;

    bool useMaxOptimization = false;

    // IEEE strictness needs to be enabled for NANs to work.
    bool enableIEEEStrictness = false;
};

struct WorkaroundsD3D : angle::FeatureSetBase
{
    WorkaroundsD3D();
    ~WorkaroundsD3D();

    // On some systems, having extra rendertargets than necessary slows down the shader.
    // We can fix this by optimizing those out of the shader. At the same time, we can
    // work around a bug on some nVidia drivers that they ignore "null" render targets
    // in D3D11, by compacting the active color attachments list to omit null entries.
    angle::Feature mrtPerfWorkaround = {
        "mrt_perf_workaround", angle::FeatureCategory::D3DWorkarounds,
        "Some NVIDIA D3D11 drivers have a bug where they ignore null render targets", &members};

    angle::Feature setDataFasterThanImageUpload = {"set_data_faster_than_image_upload",
                                                   angle::FeatureCategory::D3DWorkarounds,
                                                   "Set data faster than image upload", &members};

    // Some renderers can't disable mipmaps on a mipmapped texture (i.e. solely sample from level
    // zero, and ignore the other levels). D3D11 Feature Level 10+ does this by setting MaxLOD to
    // 0.0f in the Sampler state. D3D9 sets D3DSAMP_MIPFILTER to D3DTEXF_NONE. There is no
    // equivalent to this in D3D11 Feature Level 9_3. This causes problems when (for example) an
    // application creates a mipmapped texture2D, but sets GL_TEXTURE_MIN_FILTER to GL_NEAREST
    // (i.e disables mipmaps). To work around this, D3D11 FL9_3 has to create two copies of the
    // texture. The textures' level zeros are identical, but only one texture has mips.
    angle::Feature zeroMaxLodWorkaround = {
        "zero_max_lod", angle::FeatureCategory::D3DWorkarounds,
        "D3D11 is missing an option to disable mipmaps on a mipmapped texture", &members};

    // Some renderers do not support Geometry Shaders so the Geometry Shader-based PointSprite
    // emulation will not work. To work around this, D3D11 FL9_3 has to use a different pointsprite
    // emulation that is implemented using instanced quads.
    angle::Feature useInstancedPointSpriteEmulation = {
        "use_instanced_point_sprite_emulation", angle::FeatureCategory::D3DWorkarounds,
        "Some D3D11 renderers do not support geometry shaders for pointsprite emulation", &members};

    // A bug fixed in NVIDIA driver version 347.88 < x <= 368.81 triggers a TDR when using
    // CopySubresourceRegion from a staging texture to a depth/stencil in D3D11. The workaround
    // is to use UpdateSubresource to trigger an extra copy. We disable this workaround on newer
    // NVIDIA driver versions because of a second driver bug present with the workaround enabled.
    // (See: http://anglebug.com/1452)
    angle::Feature depthStencilBlitExtraCopy = {
        "depth_stencil_blit_extra_copy", angle::FeatureCategory::D3DWorkarounds,
        "Bug in NVIDIA D3D11 Driver version <=347.88 and >368.81 triggers a TDR when using "
        "CopySubresourceRegion from a staging texture to a depth/stencil",
        &members, "http://anglebug.com/1452"};

    // The HLSL optimizer has a bug with optimizing "pow" in certain integer-valued expressions.
    // We can work around this by expanding the pow into a series of multiplies if we're running
    // under the affected compiler.
    angle::Feature expandIntegerPowExpressions = {
        "expand_integer_pow_expressions", angle::FeatureCategory::D3DWorkarounds,
        "The HLSL optimizer has a bug with optimizing 'pow' in certain integer-valued expressions",
        &members};

    // NVIDIA drivers sometimes write out-of-order results to StreamOut buffers when transform
    // feedback is used to repeatedly write to the same buffer positions.
    angle::Feature flushAfterEndingTransformFeedback = {
        "flush_after_ending_transform_feedback", angle::FeatureCategory::D3DWorkarounds,
        "NVIDIA drivers sometimes write out-of-order results to StreamOut buffers when transform "
        "feedback is used to repeatedly write to the same buffer positions",
        &members};

    // Some drivers (NVIDIA) do not take into account the base level of the texture in the results
    // of the HLSL GetDimensions builtin.
    angle::Feature getDimensionsIgnoresBaseLevel = {
        "get_dimensions_ignores_base_level", angle::FeatureCategory::D3DWorkarounds,
        "Some NVIDIA drivers o not take into account the base level of the "
        "texture in the results of the HLSL GetDimensions builtin",
        &members};

    // On some Intel drivers, HLSL's function texture.Load returns 0 when the parameter Location
    // is negative, even if the sum of Offset and Location is in range. This may cause errors when
    // translating GLSL's function texelFetchOffset into texture.Load, as it is valid for
    // texelFetchOffset to use negative texture coordinates as its parameter P when the sum of P
    // and Offset is in range. To work around this, we translate texelFetchOffset into texelFetch
    // by adding Offset directly to Location before reading the texture.
    angle::Feature preAddTexelFetchOffsets = {
        "pre_add_texel_fetch_offsets", angle::FeatureCategory::D3DWorkarounds,
        "On some Intel drivers, HLSL's function texture.Load returns 0 when the parameter Location "
        "is negative, even if the sum of Offset and Location is in range",
        &members};

    // On some AMD drivers, 1x1 and 2x2 mips of depth/stencil textures aren't sampled correctly.
    // We can work around this bug by doing an internal blit to a temporary single-channel texture
    // before we sample.
    angle::Feature emulateTinyStencilTextures = {
        "emulate_tiny_stencil_textures", angle::FeatureCategory::D3DWorkarounds,
        "On some AMD drivers, 1x1 and 2x2 mips of depth/stencil textures aren't sampled correctly",
        &members};

    // In Intel driver, the data with format DXGI_FORMAT_B5G6R5_UNORM will be parsed incorrectly.
    // This workaroud will disable B5G6R5 support when it's Intel driver. By default, it will use
    // R8G8B8A8 format. This bug is fixed in version 4539 on Intel drivers.
    angle::Feature disableB5G6R5Support = {
        "disable_b5g6r5_support", angle::FeatureCategory::D3DWorkarounds,
        "In Intel driver, the data with format DXGI_FORMAT_B5G6R5_UNORM will be parsed incorrectly",
        &members};

    // On some Intel drivers, evaluating unary minus operator on integer may get wrong answer in
    // vertex shaders. To work around this bug, we translate -(int) into ~(int)+1.
    // This driver bug is fixed in 20.19.15.4624.
    angle::Feature rewriteUnaryMinusOperator = {
        "rewrite_unary_minus_operator", angle::FeatureCategory::D3DWorkarounds,
        "On some Intel drivers, evaluating unary minus operator on integer may "
        "get wrong answer in vertex shaders",
        &members};

    // On some Intel drivers, using isnan() on highp float will get wrong answer. To work around
    // this bug, we use an expression to emulate function isnan().
    // Tracking bug: https://crbug.com/650547
    // This driver bug is fixed in 21.20.16.4542.
    angle::Feature emulateIsnanFloat = {
        "emulate_isnan_float", angle::FeatureCategory::D3DWorkarounds,
        "On some Intel drivers, using isnan() on highp float will get wrong answer", &members,
        "https://crbug.com/650547"};

    // On some Intel drivers, using clear() may not take effect. To work around this bug, we call
    // clear() twice on these platforms.
    // Tracking bug: https://crbug.com/655534
    angle::Feature callClearTwice = {"call_clear_twice", angle::FeatureCategory::D3DWorkarounds,
                                     "On some Intel drivers, using clear() may not take effect",
                                     &members, "https://crbug.com/655534"};

    // On some Intel drivers, copying from staging storage to constant buffer storage does not
    // seem to work. Work around this by keeping system memory storage as a canonical reference
    // for buffer data.
    // D3D11-only workaround. See http://crbug.com/593024.
    angle::Feature useSystemMemoryForConstantBuffers = {
        "use_system_memory_for_constant_buffers", angle::FeatureCategory::D3DWorkarounds,
        "On some Intel drivers, copying from staging storage to constant buffer "
        "storage does not work",
        &members, "https://crbug.com/593024"};

    // This workaround is for the ANGLE_multiview extension. If enabled the viewport or render
    // target slice will be selected in the geometry shader stage. The workaround flag is added to
    // make it possible to select the code path in end2end and performance tests.
    angle::Feature selectViewInGeometryShader = {
        "select_view_in_geometry_shader", angle::FeatureCategory::D3DWorkarounds,
        "The viewport or render target slice will be selected in the geometry shader stage",
        &members};

    // When rendering with no render target on D3D, two bugs lead to incorrect behavior on Intel
    // drivers < 4815. The rendering samples always pass neglecting discard statements in pixel
    // shader.
    // 1. If rendertarget is not set, the pixel shader will be recompiled to drop 'SV_TARGET'.
    // When using a pixel shader with no 'SV_TARGET' in a draw, the pixels are always generated even
    // if they should be discard by 'discard' statements.
    // 2. If ID3D11BlendState.RenderTarget[].RenderTargetWriteMask is 0 and rendertarget is not set,
    // then rendering samples also pass neglecting discard statements in pixel shader.
    // So we add a dummy texture as render target in such case. See http://anglebug.com/2152
    angle::Feature addDummyTextureNoRenderTarget = {
        "add_dummy_texture_no_render_target", angle::FeatureCategory::D3DWorkarounds,
        "On D3D ntel drivers <4815 when rendering with no render target, two "
        "bugs lead to incorrect behavior",
        &members, "http://anglebug.com/2152"};

    // Don't use D3D constant register zero when allocating space for uniforms in the vertex shader.
    // This is targeted to work around a bug in NVIDIA D3D driver version 388.59 where in very
    // specific cases the driver would not handle constant register zero correctly.
    angle::Feature skipVSConstantRegisterZero = {
        "skip_vs_constant_register_zero", angle::FeatureCategory::D3DWorkarounds,
        "On NVIDIA D3D driver v388.59 in specific cases the driver doesn't "
        "handle constant register zero correctly",
        &members};

    // Forces the value returned from an atomic operations to be always be resolved. This is
    // targeted to workaround a bug in NVIDIA D3D driver where the return value from
    // RWByteAddressBuffer.InterlockedAdd does not get resolved when used in the .yzw components of
    // a RWByteAddressBuffer.Store operation. Only has an effect on HLSL translation.
    // http://anglebug.com/3246
    angle::Feature forceAtomicValueResolution = {
        "force_atomic_value_resolution", angle::FeatureCategory::D3DWorkarounds,
        "On an NVIDIA D3D driver, the return value from RWByteAddressBuffer.InterlockedAdd does "
        "not resolve when used in the .yzw components of a RWByteAddressBuffer.Store operation",
        &members, "http://anglebug.com/3246"};
};

inline WorkaroundsD3D::WorkaroundsD3D()                            = default;
inline WorkaroundsD3D::~WorkaroundsD3D()                           = default;

}  // namespace angle

#endif  // ANGLE_PLATFORM_WORKAROUNDSD3D_H_

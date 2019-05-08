//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// WorkaroundsGL.h: Workarounds for GL driver bugs and other issues.

#ifndef LIBANGLE_RENDERER_GL_WORKAROUNDSGL_H_
#define LIBANGLE_RENDERER_GL_WORKAROUNDSGL_H_

#include "platform/Feature.h"

namespace rx
{

struct WorkaroundsGL : angle::FeatureSetBase
{
    WorkaroundsGL();
    ~WorkaroundsGL();

    // When writing a float to a normalized integer framebuffer, desktop OpenGL is allowed to write
    // one of the two closest normalized integer representations (although round to nearest is
    // preferred) (see section 2.3.5.2 of the GL 4.5 core specification). OpenGL ES requires that
    // round-to-nearest is used (see "Conversion from Floating-Point to Framebuffer Fixed-Point" in
    // section 2.1.2 of the OpenGL ES 2.0.25 spec).  This issue only shows up on Intel and AMD
    // drivers on framebuffer formats that have 1-bit alpha, work around this by using higher
    // precision formats instead.
    angle::Feature avoid1BitAlphaTextureFormats = {
        "avoid_1_bit_alpha_texture_formats", angle::FeatureCategory::OpenGLWorkarounds,
        "Issue on Intel and AMD drivers with 1-bit alpha framebuffer formats", &members};

    // On some older Intel drivers, GL_RGBA4 is not color renderable, glCheckFramebufferStatus
    // returns GL_FRAMEBUFFER_UNSUPPORTED. Work around this by using a known color-renderable
    // format.
    angle::Feature rgba4IsNotSupportedForColorRendering = {
        "rgba4_is_not_supported_for_color_rendering", angle::FeatureCategory::OpenGLWorkarounds,
        "Issue on older Intel drivers, GL_RGBA4 is not color renderable", &members};

    // When clearing a framebuffer on Intel or AMD drivers, when GL_FRAMEBUFFER_SRGB is enabled, the
    // driver clears to the linearized clear color despite the framebuffer not supporting SRGB
    // blending.  It only seems to do this when the framebuffer has only linear attachments, mixed
    // attachments appear to get the correct clear color.
    angle::Feature doesSRGBClearsOnLinearFramebufferAttachments = {
        "does_srgb_clears_on_linear_framebuffer_attachments",
        angle::FeatureCategory::OpenGLWorkarounds,
        "Issue clearing framebuffers with linear attachments on Indel or AMD "
        "drivers when GL_FRAMEBUFFER_SRGB is enabled",
        &members};

    // On Mac some GLSL constructs involving do-while loops cause GPU hangs, such as the following:
    //  int i = 1;
    //  do {
    //      i --;
    //      continue;
    //  } while (i > 0)
    // Work around this by rewriting the do-while to use another GLSL construct (block + while)
    angle::Feature doWhileGLSLCausesGPUHang = {
        "do_while_glsl_causes_gpu_hang", angle::FeatureCategory::OpenGLWorkarounds,
        "On Mac some GLSL constructs involving do-while loops cause GPU hangs", &members};

    // Calling glFinish doesn't cause all queries to report that the result is available on some
    // (NVIDIA) drivers.  It was found that enabling GL_DEBUG_OUTPUT_SYNCHRONOUS before the finish
    // causes it to fully finish.
    angle::Feature finishDoesNotCauseQueriesToBeAvailable = {
        "finish_does_not_cause_queries_to_be_available", angle::FeatureCategory::OpenGLWorkarounds,
        "On some NVIDIA drivers, glFinish doesn't cause all queries to report available result",
        &members};

    // Always call useProgram after a successful link to avoid a driver bug.
    // This workaround is meant to reproduce the use_current_program_after_successful_link
    // workaround in Chromium (http://crbug.com/110263). It has been shown that this workaround is
    // not necessary for MacOSX 10.9 and higher (http://crrev.com/39eb535b).
    angle::Feature alwaysCallUseProgramAfterLink = {
        "always_call_use_program_after_link", angle::FeatureCategory::OpenGLWorkarounds,
        "Always call useProgram after a successful link to avoid a driver bug", &members};

    // In the case of unpacking from a pixel unpack buffer, unpack overlapping rows row by row.
    angle::Feature unpackOverlappingRowsSeparatelyUnpackBuffer = {
        "unpack_overlapping_rows_separately_unpack_buffer",
        angle::FeatureCategory::OpenGLWorkarounds,
        "In the case of unpacking from a pixel unpack buffer, unpack overlapping rows row by row",
        &members};

    // In the case of packing to a pixel pack buffer, pack overlapping rows row by row.
    angle::Feature packOverlappingRowsSeparatelyPackBuffer = {
        "pack_overlapping_rows_separately_pack_buffer", angle::FeatureCategory::OpenGLWorkarounds,
        "In the case of packing to a pixel pack buffer, pack overlapping rows row by row",
        &members};

    // During initialization, assign the current vertex attributes to the spec-mandated defaults.
    angle::Feature initializeCurrentVertexAttributes = {
        "initialize_current_vertex_attributes", angle::FeatureCategory::OpenGLWorkarounds,
        "During initialization, assign the current vertex attributes to the spec-mandated defaults",
        &members};

    // abs(i) where i is an integer returns unexpected result on Intel Mac.
    // Emulate abs(i) with i * sign(i).
    angle::Feature emulateAbsIntFunction = {
        "emulate_abs_int_function", angle::FeatureCategory::OpenGLWorkarounds,
        "On Intel mac, abs(i) where i is an integer returns unexpected result", &members};

    // On Intel Mac, calculation of loop conditions in for and while loop has bug.
    // Add "&& true" to the end of the condition expression to work around the bug.
    angle::Feature addAndTrueToLoopCondition = {
        "add_and_true_to_loop_condition", angle::FeatureCategory::OpenGLWorkarounds,
        "On Intel Mac, calculation of loop conditions in for and while loop has bug", &members};

    // When uploading textures from an unpack buffer, some drivers count an extra row padding when
    // checking if the pixel unpack buffer is big enough. Tracking bug: http://anglebug.com/1512
    // For example considering the pixel buffer below where in memory, each row data (D) of the
    // texture is followed by some unused data (the dots):
    //     +-------+--+
    //     |DDDDDDD|..|
    //     |DDDDDDD|..|
    //     |DDDDDDD|..|
    //     |DDDDDDD|..|
    //     +-------A--B
    // The last pixel read will be A, but the driver will think it is B, causing it to generate an
    // error when the pixel buffer is just big enough.
    angle::Feature unpackLastRowSeparatelyForPaddingInclusion = {
        "unpack_last_row_separately_for_padding_inclusion",
        angle::FeatureCategory::OpenGLWorkarounds,
        "When uploading textures from an unpack buffer, some drivers count an extra row padding",
        &members};

    // Equivalent workaround when uploading data from a pixel pack buffer.
    angle::Feature packLastRowSeparatelyForPaddingInclusion = {
        "pack_last_row_separately_for_padding_inclusion", angle::FeatureCategory::OpenGLWorkarounds,
        "When uploading textures from an pack buffer, some drivers count an extra row padding",
        &members};

    // On some Intel drivers, using isnan() on highp float will get wrong answer. To work around
    // this bug, we use an expression to emulate function isnan().
    // Tracking bug: http://crbug.com/650547
    angle::Feature emulateIsnanFloat = {
        "emulate_isnan_float", angle::FeatureCategory::OpenGLWorkarounds,
        "On some Intel drivers, using isnan() on highp float will get wrong answer", &members};

    // On Mac with OpenGL version 4.1, unused std140 or shared uniform blocks will be
    // treated as inactive which is not consistent with WebGL2.0 spec. Reference all members in a
    // unused std140 or shared uniform block at the beginning of main to work around it.
    // Also used on Linux AMD.
    angle::Feature useUnusedBlocksWithStandardOrSharedLayout = {
        "use_unused_blocks_with_standard_or_shared_layout",
        angle::FeatureCategory::OpenGLWorkarounds,
        "On Mac with OpenGL version 4.1, unused std140 or shared uniform blocks "
        "will be treated as inactive",
        &members};

    // This flag is used to fix spec difference between GLSL 4.1 or lower and ESSL3.
    angle::Feature removeInvariantAndCentroidForESSL3 = {
        "remove_invarient_and_centroid_for_essl3", angle::FeatureCategory::OpenGLWorkarounds,
        "Fix spec difference between GLSL 4.1 or lower and ESSL3", &members};

    // On Intel Mac OSX 10.11 driver, using "-float" will get wrong answer. Use "0.0 - float" to
    // replace "-float".
    // Tracking bug: http://crbug.com/308366
    angle::Feature rewriteFloatUnaryMinusOperator = {
        "rewrite_float_unary_minus_operator", angle::FeatureCategory::OpenGLWorkarounds,
        "On Intel Mac OSX 10.11 driver, using '-<float>' will get wrong answer", &members,
        "http://crbug.com/308366"};

    // On NVIDIA drivers, atan(y, x) may return a wrong answer.
    // Tracking bug: http://crbug.com/672380
    angle::Feature emulateAtan2Float = {"emulate_atan_2_float",
                                        angle::FeatureCategory::OpenGLWorkarounds,
                                        "On NVIDIA drivers, atan(y, x) may return a wrong answer",
                                        &members, "http://crbug.com/672380"};

    // Some drivers seem to forget about UBO bindings when using program binaries. Work around
    // this by re-applying the bindings after the program binary is loaded or saved.
    // This only seems to affect AMD OpenGL drivers, and some Android devices.
    // http://anglebug.com/1637
    angle::Feature reapplyUBOBindingsAfterUsingBinaryProgram = {
        "reapply_ubo_bindings_after_using_binary_program",
        angle::FeatureCategory::OpenGLWorkarounds,
        "Some AMD OpenGL drivers and Android devices forget about UBO bindings "
        "when using program binaries",
        &members, "http://anglebug.com/1637"};

    // Some OpenGL drivers return 0 when we query MAX_VERTEX_ATTRIB_STRIDE in an OpenGL 4.4 or
    // higher context.
    // This only seems to affect AMD OpenGL drivers.
    // Tracking bug: http://anglebug.com/1936
    angle::Feature emulateMaxVertexAttribStride = {
        "emulate_max_vertex_attrib_stride", angle::FeatureCategory::OpenGLWorkarounds,
        "Some AMD OpenGL >= 4.4 drivers return 0 when MAX_VERTEX_ATTRIB_STRIED queried", &members,
        "http://anglebug.com/1936"};

    // Initializing uninitialized locals caused odd behavior on Mac in a few WebGL 2 tests.
    // Tracking bug: http://anglebug/2041
    angle::Feature dontInitializeUninitializedLocals = {
        "dont_initialize_uninitialized_locals", angle::FeatureCategory::OpenGLWorkarounds,
        "On Mac initializing uninitialized locals caused odd behavior in a few WebGL 2 tests",
        &members, "http://anglebug.com/2041"};

    // On some NVIDIA drivers the point size range reported from the API is inconsistent with the
    // actual behavior. Clamp the point size to the value from the API to fix this.
    angle::Feature clampPointSize = {
        "clamp_point_size", angle::FeatureCategory::OpenGLWorkarounds,
        "On some NVIDIA drivers the point size range reported from the API is "
        "inconsistent with the actual behavior",
        &members};

    // On some NVIDIA drivers certain types of GLSL arithmetic ops mixing vectors and scalars may be
    // executed incorrectly. Change them in the shader translator. Tracking bug:
    // http://crbug.com/772651
    angle::Feature rewriteVectorScalarArithmetic = {
        "rewrite_vector_scalar_arithmetic", angle::FeatureCategory::OpenGLWorkarounds,
        "On some NVIDIA drivers certain types of GLSL arithmetic ops mixing "
        "vectors and scalars may be executed incorrectly",
        &members, "http://crbug.com/772651"};

    // On some Android devices for loops used to initialize variables hit native GLSL compiler bugs.
    angle::Feature dontUseLoopsToInitializeVariables = {
        "dont_use_loops_to_initialize_variables", angle::FeatureCategory::OpenGLWorkarounds,
        "On some Android devices for loops used to initialize variables hit "
        "native GLSL compiler bugs",
        &members};

    // On some NVIDIA drivers gl_FragDepth is not clamped correctly when rendering to a floating
    // point depth buffer. Clamp it in the translated shader to fix this.
    angle::Feature clampFragDepth = {
        "clamp_frag_depth", angle::FeatureCategory::OpenGLWorkarounds,
        "On some NVIDIA drivers gl_FragDepth is not clamped correctly when "
        "rendering to a floating point depth buffer",
        &members};

    // On some NVIDIA drivers before version 397.31 repeated assignment to swizzled values inside a
    // GLSL user-defined function have incorrect results. Rewrite this type of statements to fix
    // this.
    angle::Feature rewriteRepeatedAssignToSwizzled = {
        "rewrite_repeated_assign_to_swizzled", angle::FeatureCategory::OpenGLWorkarounds,
        "On some NVIDIA drivers < v397.31, repeated assignment to swizzled "
        "values inside a GLSL user-defined function have incorrect results",
        &members};
    // On some AMD and Intel GL drivers ARB_blend_func_extended does not pass the tests.
    // It might be possible to work around the Intel bug by rewriting *FragData to *FragColor
    // instead of disabling the functionality entirely. The AMD bug looked like incorrect blending,
    // not sure if a workaround is feasible. http://anglebug.com/1085
    angle::Feature disableBlendFuncExtended = {
        "disable_blend_func_extended", angle::FeatureCategory::OpenGLWorkarounds,
        "On some AMD and Intel GL drivers ARB_blend_func_extended does not pass the tests",
        &members, "http://anglebug.com/1085"};

    // Qualcomm drivers returns raw sRGB values instead of linearized values when calling
    // glReadPixels on unsized sRGB texture formats. http://crbug.com/550292 and
    // http://crbug.com/565179
    angle::Feature unsizedsRGBReadPixelsDoesntTransform = {
        "unsized_srgb_read_pixels_doesnt_transform", angle::FeatureCategory::OpenGLWorkarounds,
        "Qualcomm drivers returns raw sRGB values instead of linearized values "
        "when calling glReadPixels on unsized sRGB texture formats",
        &members, "http://crbug.com/565179"};

    // Older Qualcomm drivers generate errors when querying the number of bits in timer queries, ex:
    // GetQueryivEXT(GL_TIME_ELAPSED, GL_QUERY_COUNTER_BITS).  http://anglebug.com/3027
    angle::Feature queryCounterBitsGeneratesErrors = {
        "query_counter_bits_generates_errors", angle::FeatureCategory::OpenGLWorkarounds,
        "Older Qualcomm drivers generate errors when querying the number of bits in timer queries",
        &members, "http://anglebug.com/3027"};

    // Re-linking a program in parallel is buggy on some Intel Windows OpenGL drivers and Android
    // platforms.
    // http://anglebug.com/3045
    angle::Feature dontRelinkProgramsInParallel = {
        "query_counter_bits_generates_errors", angle::FeatureCategory::OpenGLWorkarounds,
        "On some Intel Windows OpenGL drivers and Android, relinking a program "
        "in parallel is buggy",
        &members, "http://anglebug.com/3045"};

    // Some tests have been seen to fail using worker contexts, this switch allows worker contexts
    // to be disabled for some platforms. http://crbug.com/849576
    angle::Feature disableWorkerContexts = {
        "disable_worker_contexts", angle::FeatureCategory::OpenGLWorkarounds,
        "Some tests have been seen to fail using worker contexts", &members,
        "http://crbug.com/849576"};
};

inline WorkaroundsGL::WorkaroundsGL()  = default;
inline WorkaroundsGL::~WorkaroundsGL() = default;

}  // namespace rx

#endif  // LIBANGLE_RENDERER_GL_WORKAROUNDSGL_H_

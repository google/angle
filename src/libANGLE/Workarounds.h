//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Workarounds.h: Workarounds for driver bugs and other behaviors seen
// on all platforms.

#ifndef LIBANGLE_WORKAROUNDS_H_
#define LIBANGLE_WORKAROUNDS_H_

#include "platform/Feature.h"

namespace gl
{

struct Workarounds : angle::FeatureSetBase
{
    Workarounds();
    ~Workarounds();

    // Force the context to be lost (via KHR_robustness) if a GL_OUT_OF_MEMORY error occurs. The
    // driver may be in an inconsistent state if this happens, and some users of ANGLE rely on this
    // notification to prevent further execution.
    angle::Feature loseContextOnOutOfMemory = {
        "lose_context_on_out_of_memory", angle::FeatureCategory::FrontendWorkarounds,
        "Some users rely on a lost context notification if a GL_OUT_OF_MEMORY "
        "error occurs",
        &members};

    // Program binaries don't contain transform feedback varyings on Qualcomm GPUs.
    // Work around this by disabling the program cache for programs with transform feedback.
    angle::Feature disableProgramCachingForTransformFeedback = {
        "disable_program_caching_for_transform_feedback",
        angle::FeatureCategory::FrontendWorkarounds,
        "On Qualcomm GPUs, program binaries don't contain transform feedback varyings", &members};

    // On Windows Intel OpenGL drivers TexImage sometimes seems to interact with the Framebuffer.
    // Flaky crashes can occur unless we sync the Framebuffer bindings. The workaround is to add
    // Framebuffer binding dirty bits to TexImage updates. See http://anglebug.com/2906
    angle::Feature syncFramebufferBindingsOnTexImage = {
        "sync_framebuffer_bindings_on_tex_image", angle::FeatureCategory::FrontendWorkarounds,
        "On Windows Intel OpenGL drivers TexImage sometimes seems to interact "
        "with the Framebuffer",
        &members};
};

inline Workarounds::Workarounds()  = default;
inline Workarounds::~Workarounds() = default;

}  // namespace gl

#endif  // LIBANGLE_WORKAROUNDS_H_

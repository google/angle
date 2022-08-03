//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CompilerGL:
//   Implementation of the GL compiler methods.
//

#include "libANGLE/renderer/gl/CompilerGL.h"

#include "libANGLE/gles_extensions_autogen.h"
#include "libANGLE/renderer/gl/ContextGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "platform/FeaturesGL_autogen.h"

namespace rx
{

namespace
{

ShShaderOutput GetShaderOutputType(const FunctionsGL *functions)
{
    ASSERT(functions);

    if (functions->standard == STANDARD_GL_DESKTOP)
    {
        // GLSL outputs
        if (functions->isAtLeastGL(gl::Version(4, 5)))
        {
            return SH_GLSL_450_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(4, 4)))
        {
            return SH_GLSL_440_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(4, 3)))
        {
            return SH_GLSL_430_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(4, 2)))
        {
            return SH_GLSL_420_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(4, 1)))
        {
            return SH_GLSL_410_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(4, 0)))
        {
            return SH_GLSL_400_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(3, 3)))
        {
            return SH_GLSL_330_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(3, 2)))
        {
            return SH_GLSL_150_CORE_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(3, 1)))
        {
            return SH_GLSL_140_OUTPUT;
        }
        else if (functions->isAtLeastGL(gl::Version(3, 0)))
        {
            return SH_GLSL_130_OUTPUT;
        }
        else
        {
            return SH_GLSL_COMPATIBILITY_OUTPUT;
        }
    }
    else if (functions->standard == STANDARD_GL_ES)
    {
        // ESSL outputs
        return SH_ESSL_OUTPUT;
    }
    else
    {
        UNREACHABLE();
        return ShShaderOutput(0);
    }
}

}  // anonymous namespace

CompilerGL::CompilerGL(const ContextGL *context)
    : mTranslatorOutputType(GetShaderOutputType(context->getFunctions()))
{
    if (context->getExtensions().shaderPixelLocalStorageCoherentANGLE)
    {
        const angle::FeaturesGL &features = context->getFeaturesGL();
        // Prefer vendor-specific extensions first. The PixelLocalStorageTest.Coherency test doesn't
        // always pass on Intel when we use the ARB extension.
        if (features.supportsFragmentShaderInterlockNV.enabled)
        {
            // This extension requires 430+. GetShaderOutputType() should always select 430+ on a GL
            // 4.3 context, where this extension is defined.
            ASSERT(context->getFunctions()->isAtLeastGL(gl::Version(4, 3)));
            ASSERT(mTranslatorOutputType >= SH_GLSL_430_CORE_OUTPUT);
            mBackendFeatures.fragmentSynchronizationType =
                ShFragmentSynchronizationType::FragmentShaderInterlock_NV_GL;
        }
        else if (features.supportsFragmentShaderOrderingINTEL.enabled)
        {
            // This extension requires 440+. GetShaderOutputType() should always select 440+ on a GL
            // 4.4 context, where this extension is defined.
            ASSERT(context->getFunctions()->isAtLeastGL(gl::Version(4, 4)));
            ASSERT(mTranslatorOutputType >= SH_GLSL_440_CORE_OUTPUT);
            mBackendFeatures.fragmentSynchronizationType =
                ShFragmentSynchronizationType::FragmentShaderOrdering_INTEL_GL;
        }
        else
        {
            ASSERT(features.supportsFragmentShaderInterlockARB.enabled);
            // This extension requires 450+. GetShaderOutputType() should always select 450+ on a GL
            // 4.5 context, where this extension is defined.
            ASSERT(context->getFunctions()->isAtLeastGL(gl::Version(4, 5)));
            ASSERT(mTranslatorOutputType >= SH_GLSL_450_CORE_OUTPUT);
            mBackendFeatures.fragmentSynchronizationType =
                ShFragmentSynchronizationType::FragmentShaderInterlock_ARB_GL;
        }
    }
}

ShShaderOutput CompilerGL::getTranslatorOutputType() const
{
    return mTranslatorOutputType;
}

}  // namespace rx

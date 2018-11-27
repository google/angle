//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angle_test_instantiate.cpp: Adds support for filtering parameterized
// tests by platform, so we skip unsupported configs.

#include "test_utils/angle_test_instantiate.h"

#include <iostream>
#include <map>

#include "EGLWindow.h"
#include "OSWindow.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/InitializeGlobals.h"
#include "test_utils/angle_test_configs.h"

namespace angle
{

bool IsPlatformAvailable(const CompilerParameters &param)
{
    switch (param.output)
    {
        case SH_HLSL_4_1_OUTPUT:
        case SH_HLSL_4_0_FL9_3_OUTPUT:
        case SH_HLSL_3_0_OUTPUT:
        {
            TPoolAllocator allocator;
            InitializePoolIndex();
            allocator.push();
            SetGlobalPoolAllocator(&allocator);
            ShHandle translator =
                sh::ConstructCompiler(GL_FRAGMENT_SHADER, SH_WEBGL2_SPEC, param.output);
            bool success = translator != nullptr;
            SetGlobalPoolAllocator(nullptr);
            allocator.pop();
            FreePoolIndex();
            if (!success)
            {
                return false;
            }
            break;
        }
        default:
            break;
    }
    return true;
}

bool IsPlatformAvailable(const PlatformParameters &param)
{
    switch (param.getRenderer())
    {
        case EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE:
            break;

        case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE:
#if !defined(ANGLE_ENABLE_D3D9)
            return false;
#else
            break;
#endif

        case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE:
#if !defined(ANGLE_ENABLE_D3D11)
            return false;
#else
            break;
#endif

        case EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE:
        case EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE:
#if !defined(ANGLE_ENABLE_OPENGL)
            return false;
#else
            break;
#endif

        case EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE:
#if !defined(ANGLE_ENABLE_VULKAN)
            return false;
#else
            break;
#endif

        case EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE:
#ifndef ANGLE_ENABLE_NULL
            return false;
#endif
            break;

        default:
            std::cout << "Unknown test platform: " << param << std::endl;
            return false;
    }

    static std::map<PlatformParameters, bool> paramAvailabilityCache;
    auto iter = paramAvailabilityCache.find(param);
    if (iter != paramAvailabilityCache.end())
    {
        return iter->second;
    }
    else
    {
        OSWindow *osWindow = CreateOSWindow();
        bool result        = osWindow->initialize("CONFIG_TESTER", 1, 1);

        if (result)
        {
            EGLWindow *eglWindow =
                new EGLWindow(param.majorVersion, param.minorVersion, param.eglParameters);
            result = eglWindow->initializeGL(osWindow);

            eglWindow->destroyGL();
            SafeDelete(eglWindow);
        }

        osWindow->destroy();
        SafeDelete(osWindow);

        paramAvailabilityCache[param] = result;

        if (!result)
        {
            std::cout << "Skipping tests using configuration " << param
                      << " because it is not available." << std::endl;
        }

        return result;
    }
}

}  // namespace angle

//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UTIL_TEST_FIXTURE_TYPES_H
#define UTIL_TEST_FIXTURE_TYPES_H

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace Gles
{

struct Two
{
    static EGLint GetGlesMajorVersion()
    {
        return 2;
    };
};

struct Three
{
    static EGLint GetGlesMajorVersion()
    {
        return 3;
    };
};

}

namespace Rend
{

struct D3D11
{
    static EGLint GetRequestedRenderer()
    {
        return EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE;
    };
};

struct D3D9
{
    static EGLint GetRequestedRenderer()
    {
        return EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE;
    };
};

struct WARP
{
    static EGLint GetRequestedRenderer()
    {
        return EGL_PLATFORM_ANGLE_TYPE_D3D11_WARP_ANGLE;
    };
};

}

// Test Fixture Type
template<typename GlesVersionT, typename RendererT>
struct TFT
{
    static EGLint GetGlesMajorVersion()
    {
        return GlesVersionT::GetGlesMajorVersion();
    }

    static EGLint GetRequestedRenderer()
    {
        return RendererT::GetRequestedRenderer();
    }
};

#endif // UTIL_TEST_FIXTURE_TYPES_H
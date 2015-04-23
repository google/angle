/*-------------------------------------------------------------------------
 * drawElements Quality Program Tester Core
 * ----------------------------------------
 *
 * Copyright 2014 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "tcuANGLEWin32Platform.h"
#include "tcuANGLEWin32NativeDisplayFactory.h"
#include "egluGLContextFactory.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

namespace tcu
{

ANGLEWin32Platform::ANGLEWin32Platform()
    : mInstance(GetModuleHandle(nullptr))
{
    // Set process priority to lower.
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);

#if defined(ANGLE_DEQP_GLES2_TESTS) || defined(ANGLE_DEQP_GLES3_TESTS)
    std::vector<eglw::EGLAttrib> d3d11Attribs;
    d3d11Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    d3d11Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);
    d3d11Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
    d3d11Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    d3d11Attribs.push_back(EGL_NONE);
    d3d11Attribs.push_back(EGL_NONE);

    auto *d3d11Factory = new ANGLEWin32NativeDisplayFactory(
        "angle-d3d11", "ANGLE D3D11 Display", d3d11Attribs, mInstance);
    m_nativeDisplayFactoryRegistry.registerFactory(d3d11Factory);
#endif

#if defined(ANGLE_DEQP_GLES2_TESTS)
    std::vector<eglw::EGLAttrib> d3d9Attribs;
    d3d9Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    d3d9Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE);
    d3d9Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
    d3d9Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    d3d9Attribs.push_back(EGL_NONE);
    d3d9Attribs.push_back(EGL_NONE);

    auto *d3d9Factory = new ANGLEWin32NativeDisplayFactory(
        "angle-d3d9", "ANGLE D3D9 Display", d3d9Attribs, mInstance);
    m_nativeDisplayFactoryRegistry.registerFactory(d3d9Factory);

    std::vector<eglw::EGLAttrib> d3d1193Attribs;
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
    d3d1193Attribs.push_back(9);
    d3d1193Attribs.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
    d3d1193Attribs.push_back(3);
    d3d1193Attribs.push_back(EGL_NONE);
    d3d1193Attribs.push_back(EGL_NONE);

    auto *d3d1193Factory = new ANGLEWin32NativeDisplayFactory(
        "angle-d3d11-fl93", "ANGLE D3D11 FL9_3 Display", d3d1193Attribs, mInstance);
    m_nativeDisplayFactoryRegistry.registerFactory(d3d1193Factory);

    std::vector<eglw::EGLAttrib> glAttribs;
    glAttribs.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    glAttribs.push_back(EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE);
    glAttribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
    glAttribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    glAttribs.push_back(EGL_NONE);
    glAttribs.push_back(EGL_NONE);

    auto *glFactory = new ANGLEWin32NativeDisplayFactory(
        "angle-gl", "ANGLE OpenGL Display", glAttribs, mInstance);
    m_nativeDisplayFactoryRegistry.registerFactory(glFactory);
#endif

    m_contextFactoryRegistry.registerFactory(new eglu::GLContextFactory(m_nativeDisplayFactoryRegistry));
}

ANGLEWin32Platform::~ANGLEWin32Platform()
{
}

bool ANGLEWin32Platform::processEvents()
{
    MSG msg;
    while (PeekMessage(&msg, reinterpret_cast<HWND>(-1), 0, 0, PM_REMOVE))
    {
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            return false;
    }
    return true;
}

} // tcu

// Create platform
tcu::Platform *createPlatform()
{
    return new tcu::ANGLEWin32Platform();
}

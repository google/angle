//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef ANGLE_TEST_CONFIGS_H_
#define ANGLE_TEST_CONFIGS_H_

#include <EGL/egl.h>
#include <EGL/eglext.h>

#include "common/debug.h"
#include "EGLWindow.h"

namespace angle
{

struct PlatformParameters
{
    PlatformParameters(EGLint clientVersion,
                       const EGLPlatformParameters &eglPlatformParameters)
        : mClientVersion(clientVersion),
          mEGLPlatformParameters(eglPlatformParameters)
    {
    }

    EGLint mClientVersion;
    EGLPlatformParameters mEGLPlatformParameters;

};

inline std::ostream &operator<<(std::ostream& stream,
                                const PlatformParameters &pp)
{
    stream << "ES" << pp.mClientVersion << "_";

    switch (pp.mEGLPlatformParameters.renderer)
    {
      case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE:
        stream << "D3D9";
        break;
      case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE:
        stream << "D3D11";
        break;
      case EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE:
        stream << "OPENGL";
        break;
      case EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE:
        stream << "GLES";
        break;
      default:
        UNREACHABLE();
        break;
    }

    if (pp.mEGLPlatformParameters.majorVersion != EGL_DONT_CARE)
    {
        stream << "_" << pp.mEGLPlatformParameters.majorVersion;
    }

    if (pp.mEGLPlatformParameters.minorVersion != EGL_DONT_CARE)
    {
        stream << "_" << pp.mEGLPlatformParameters.minorVersion;
    }

    switch (pp.mEGLPlatformParameters.deviceType)
    {
      case EGL_DONT_CARE:
      case EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE:
        // default
        break;

      case EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE:
        stream << "_REFERENCE";
        break;

      case EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE:
        stream << "_WARP";
        break;

      default:
        UNREACHABLE();
        break;
    }

    return stream;
}

inline PlatformParameters ES2_D3D9()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D9_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL11_0()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_1()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_0()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL9_3()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        9, 3,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL11_0_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_1_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_0_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL9_3_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        9, 3,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL11_0_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_1_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL10_0_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_D3D11_FL9_3_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        9, 3,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES3_D3D11()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_1()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_0()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_1()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_0()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_1_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_0_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_1_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_0_WARP()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_WARP_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        EGL_DONT_CARE, EGL_DONT_CARE,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_1_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL11_0_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        11, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_1_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 1,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_D3D11_FL10_0_REFERENCE()
{
    EGLPlatformParameters eglParams(
        EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE,
        10, 0,
        EGL_PLATFORM_ANGLE_DEVICE_TYPE_REFERENCE_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES2_OPENGL()
{
    EGLPlatformParameters eglParams(EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES2_OPENGLES()
{
    EGLPlatformParameters eglParams(EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE);
    return PlatformParameters(2, eglParams);
}

inline PlatformParameters ES3_OPENGL()
{
    EGLPlatformParameters eglParams(EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE);
    return PlatformParameters(3, eglParams);
}

inline PlatformParameters ES3_OPENGLES()
{
    EGLPlatformParameters eglParams(EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE);
    return PlatformParameters(3, eglParams);
}

#define ANGLE_INSTANTIATE_TEST(testName, ...) \
    INSTANTIATE_TEST_CASE_P(, testName, testing::Values(__VA_ARGS__));

} // namespace angle

#endif // ANGLE_TEST_CONFIGS_H_

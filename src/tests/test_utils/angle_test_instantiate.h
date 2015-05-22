//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angle_test_instantiate.h: Adds support for filtering parameterized
// tests by platform, so we skip unsupported configs.

#ifndef ANGLE_TEST_INSTANTIATE_H_
#define ANGLE_TEST_INSTANTIATE_H_

#include "common/debug.h"

namespace angle
{

// This functions is used to filter which tests should be registered,
// internally it T::getRenderer() const that should be implemented for test
// parameter types.
template <typename T>
std::vector<T> FilterTestParams(const T *params, size_t numParams)
{
    std::vector<T> filtered;

    for (size_t i = 0; i < numParams; i++)
    {
        switch (params[i].getRenderer())
        {
          case EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE:
            filtered.push_back(params[i]);
            break;

          case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE:
#if defined(ANGLE_ENABLE_D3D9)
            filtered.push_back(params[i]);
#endif
            break;

          case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE:
#if defined(ANGLE_ENABLE_D3D11)
            filtered.push_back(params[i]);
#endif
            break;

          case EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE:
          case EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE:
#if defined(ANGLE_ENABLE_OPENGL)
            filtered.push_back(params[i]);
#endif
            break;

          default:
            UNREACHABLE();
            break;
        }
    }

    return filtered;
}

// Instantiate the test once for each extra argument. The types of all the
// arguments must match, and getRenderer must be implemented for that type.
#define ANGLE_INSTANTIATE_TEST(testName, firstParam, ...) \
    const decltype(firstParam) testName##params[] = { firstParam, ##__VA_ARGS__ }; \
    INSTANTIATE_TEST_CASE_P(, testName, testing::ValuesIn(::angle::FilterTestParams(testName##params, ArraySize(testName##params))));

} // namespace angle

#endif // ANGLE_TEST_INSTANTIATE_H_

//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests the eglQueryStringiANGLE function exposed by the extension
// EGL_ANGLE_workaround_control.

#include <gtest/gtest.h>

#include "libANGLE/Display.h"
#include "test_utils/ANGLETest.h"

using namespace angle;

class EGLQueryStringIndexedTest : public ANGLETest
{
  public:
    void testSetUp() override
    {
        ASSERT_TRUE(IsEGLDisplayExtensionEnabled(getEGLWindow()->getDisplay(),
                                                 "EGL_ANGLE_workaround_control"));
    }

    void testTearDown() override {}
};

// Ensure eglQueryStringiANGLE generates EGL_BAD_DISPLAY if the display passed in is invalid.
TEST_P(EGLQueryStringIndexedTest, InvalidDisplay)
{
    EXPECT_EQ(nullptr, eglQueryStringiANGLE(EGL_NO_DISPLAY, EGL_WORKAROUND_NAME_ANGLE, 0));
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the index is negative.
TEST_P(EGLQueryStringIndexedTest, NegativeIndex)
{
    EXPECT_EQ(nullptr,
              eglQueryStringiANGLE(getEGLWindow()->getDisplay(), EGL_WORKAROUND_NAME_ANGLE, -1));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the index is out of bounds.
TEST_P(EGLQueryStringIndexedTest, IndexOutOfBounds)
{
    EGLDisplay dpy        = getEGLWindow()->getDisplay();
    egl::Display *display = static_cast<egl::Display *>(dpy);
    EXPECT_EQ(nullptr,
              eglQueryStringiANGLE(dpy, EGL_WORKAROUND_NAME_ANGLE, display->getFeatures().size()));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the name is not one of the valid
// options specified in the workaround.
TEST_P(EGLQueryStringIndexedTest, InvalidName)
{
    EXPECT_EQ(nullptr, eglQueryStringiANGLE(getEGLWindow()->getDisplay(), 100, 0));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// For each valid name and index in the workaround description arrays, query the values and ensure
// that no error is generated, and that the values match the correct values frim ANGLE's display's
// FeatureList.
TEST_P(EGLQueryStringIndexedTest, QueryAll)
{
    EGLDisplay dpy              = getEGLWindow()->getDisplay();
    egl::Display *display       = static_cast<egl::Display *>(dpy);
    angle::FeatureList features = display->getFeatures();
    for (size_t i = 0; i < features.size(); i++)
    {
        EXPECT_STREQ(features[i]->name, eglQueryStringiANGLE(dpy, EGL_WORKAROUND_NAME_ANGLE, i));
        EXPECT_STREQ(FeatureCategoryToString(features[i]->category),
                     eglQueryStringiANGLE(dpy, EGL_WORKAROUND_CATEGORY_ANGLE, i));
        EXPECT_STREQ(features[i]->description,
                     eglQueryStringiANGLE(dpy, EGL_WORKAROUND_DESCRIPTION_ANGLE, i));
        EXPECT_STREQ(features[i]->bug, eglQueryStringiANGLE(dpy, EGL_WORKAROUND_BUG_ANGLE, i));
        if (features[i]->enabled)
        {
            EXPECT_STREQ("true", eglQueryStringiANGLE(dpy, EGL_WORKAROUND_ENABLED_ANGLE, i));
        }
        else
        {
            EXPECT_STREQ("false", eglQueryStringiANGLE(dpy, EGL_WORKAROUND_ENABLED_ANGLE, i));
        }
        ASSERT_EGL_SUCCESS();
    }
}

ANGLE_INSTANTIATE_TEST(EGLQueryStringIndexedTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_OPENGL(),
                       ES2_VULKAN(),
                       ES3_D3D11(),
                       ES3_OPENGL());

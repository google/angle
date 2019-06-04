//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Tests the eglQueryStringiANGLE and eglQueryDisplayAttribANGLE functions exposed by the
// extension EGL_ANGLE_feature_control.

#include <gtest/gtest.h>

#include "libANGLE/Display.h"
#include "test_utils/ANGLETest.h"

using namespace angle;

class EGLFeatureControlTest : public ANGLETest
{
  public:
    void testSetUp() override
    {
        ASSERT_TRUE(IsEGLDisplayExtensionEnabled(getEGLWindow()->getDisplay(),
                                                 "EGL_ANGLE_feature_control"));
    }

    void testTearDown() override {}
};

// Ensure eglQueryStringiANGLE generates EGL_BAD_DISPLAY if the display passed in is invalid.
TEST_P(EGLFeatureControlTest, InvalidDisplay)
{
    EXPECT_EQ(nullptr, eglQueryStringiANGLE(EGL_NO_DISPLAY, EGL_FEATURE_NAME_ANGLE, 0));
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the index is negative.
TEST_P(EGLFeatureControlTest, NegativeIndex)
{
    EXPECT_EQ(nullptr,
              eglQueryStringiANGLE(getEGLWindow()->getDisplay(), EGL_FEATURE_NAME_ANGLE, -1));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the index is out of bounds.
TEST_P(EGLFeatureControlTest, IndexOutOfBounds)
{
    EGLDisplay dpy        = getEGLWindow()->getDisplay();
    egl::Display *display = static_cast<egl::Display *>(dpy);
    EXPECT_EQ(nullptr,
              eglQueryStringiANGLE(dpy, EGL_FEATURE_NAME_ANGLE, display->getFeatures().size()));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Ensure eglQueryStringiANGLE generates EGL_BAD_PARAMETER if the name is not one of the valid
// options specified in EGL_ANGLE_feature_control.
TEST_P(EGLFeatureControlTest, InvalidName)
{
    EXPECT_EQ(nullptr, eglQueryStringiANGLE(getEGLWindow()->getDisplay(), 100, 0));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// For each valid name and index in the feature description arrays, query the values and ensure
// that no error is generated, and that the values match the correct values frim ANGLE's display's
// FeatureList.
TEST_P(EGLFeatureControlTest, QueryAll)
{
    EGLDisplay dpy              = getEGLWindow()->getDisplay();
    egl::Display *display       = static_cast<egl::Display *>(dpy);
    angle::FeatureList features = display->getFeatures();
    for (size_t i = 0; i < features.size(); i++)
    {
        EXPECT_STREQ(features[i]->name, eglQueryStringiANGLE(dpy, EGL_FEATURE_NAME_ANGLE, i));
        EXPECT_STREQ(FeatureCategoryToString(features[i]->category),
                     eglQueryStringiANGLE(dpy, EGL_FEATURE_CATEGORY_ANGLE, i));
        EXPECT_STREQ(features[i]->description,
                     eglQueryStringiANGLE(dpy, EGL_FEATURE_DESCRIPTION_ANGLE, i));
        EXPECT_STREQ(features[i]->bug, eglQueryStringiANGLE(dpy, EGL_FEATURE_BUG_ANGLE, i));
        EXPECT_STREQ(FeatureStatusToString(features[i]->enabled),
                     eglQueryStringiANGLE(dpy, EGL_FEATURE_STATUS_ANGLE, i));
        ASSERT_EGL_SUCCESS();
    }
}

// Ensure eglQueryDisplayAttribANGLE returns the correct number of features when queried with
// attribute EGL_FEATURE_COUNT_ANGLE
TEST_P(EGLFeatureControlTest, FeatureCount)
{
    EGLDisplay dpy        = getEGLWindow()->getDisplay();
    egl::Display *display = static_cast<egl::Display *>(dpy);
    EGLAttrib value       = -1;
    EXPECT_EQ(static_cast<EGLBoolean>(EGL_TRUE),
              eglQueryDisplayAttribANGLE(dpy, EGL_FEATURE_COUNT_ANGLE, &value));
    EXPECT_EQ(display->getFeatures().size(), static_cast<size_t>(value));
    ASSERT_EGL_SUCCESS();
}

ANGLE_INSTANTIATE_TEST(EGLFeatureControlTest,
                       ES2_D3D9(),
                       ES2_D3D11(),
                       ES2_OPENGL(),
                       ES2_VULKAN(),
                       ES3_D3D11(),
                       ES3_OPENGL());

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLDebugTest.cpp:
//   Tests of EGL_KHR_debug extension

#include <gtest/gtest.h>

#include "test_utils/ANGLETest.h"
#include "test_utils/angle_test_configs.h"
#include "test_utils/gl_raii.h"
#include "util/EGLWindow.h"

namespace angle
{
class EGLDebugTest : public ANGLETest<>
{
  protected:
    void testTearDown() override { eglDebugMessageControlKHR(nullptr, nullptr); }

    bool hasExtension() const { return IsEGLClientExtensionEnabled("EGL_KHR_debug"); }

    static void EGLAPIENTRY StubCallback(EGLenum error,
                                         const char *command,
                                         EGLint messageType,
                                         EGLLabelKHR threadLabel,
                                         EGLLabelKHR objectLabel,
                                         const char *message)
    {}

    static void EGLAPIENTRY CheckBadBindAPIError(EGLenum error,
                                                 const char *command,
                                                 EGLint messageType,
                                                 EGLLabelKHR threadLabel,
                                                 EGLLabelKHR objectLabel,
                                                 const char *message)
    {
        EXPECT_STREQ("eglBindAPI", command);
        ASSERT_EGLENUM_EQ(EGL_BAD_PARAMETER, error);
        EXPECT_STREQ("Thread", static_cast<const char *>(threadLabel));
    }

    static EGLDEBUGPROCKHR EGLAttribToDebugCallback(EGLAttrib attrib)
    {
        return reinterpret_cast<EGLDEBUGPROCKHR>(static_cast<uintptr_t>(attrib));
    }

    static EGLAttrib DebugCallbackToEGLAttrib(EGLDEBUGPROCKHR callback)
    {
        return static_cast<EGLAttrib>(reinterpret_cast<intptr_t>(callback));
    }
};

// Test that the extension is always available (it is implemented in ANGLE's frontend).
TEST_P(EGLDebugTest, ExtensionAlwaysAvailable)
{
    ASSERT_TRUE(hasExtension());
}

// Check that the default message filters and callbacks are correct
TEST_P(EGLDebugTest, DefaultParameters)
{
    ANGLE_SKIP_TEST_IF(!hasExtension());

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS), eglDebugMessageControlKHR(nullptr, nullptr));

    EGLAttrib result = 0;

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_ERROR_KHR, &result));
    EXPECT_EGL_TRUE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_WARN_KHR, &result));
    EXPECT_EGL_FALSE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_INFO_KHR, &result));
    EXPECT_EGL_FALSE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_CALLBACK_KHR, &result));
    EXPECT_EQ(nullptr, EGLAttribToDebugCallback(result));
}

// Check that the message control and callback parameters can be set and then queried back
TEST_P(EGLDebugTest, SetMessageControl)
{
    ANGLE_SKIP_TEST_IF(!hasExtension());

    EGLAttrib controls[] = {
        EGL_DEBUG_MSG_CRITICAL_KHR,
        EGL_FALSE,
        // EGL_DEBUG_MSG_ERROR_KHR left unset
        EGL_DEBUG_MSG_WARN_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_INFO_KHR,
        EGL_FALSE,
        EGL_NONE,
        EGL_NONE,
    };

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS), eglDebugMessageControlKHR(&StubCallback, controls));

    EGLAttrib result = 0;

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_CRITICAL_KHR, &result));
    EXPECT_EGL_FALSE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_ERROR_KHR, &result));
    EXPECT_EGL_TRUE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_WARN_KHR, &result));
    EXPECT_EGL_TRUE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_MSG_INFO_KHR, &result));
    EXPECT_EGL_FALSE(result);

    EXPECT_EGL_TRUE(eglQueryDebugKHR(EGL_DEBUG_CALLBACK_KHR, &result));
    EXPECT_EQ(DebugCallbackToEGLAttrib(&StubCallback), result);
}

// Set a thread label and then trigger a callback to verify the callback parameters are correct
TEST_P(EGLDebugTest, CorrectCallbackParameters)
{
    ANGLE_SKIP_TEST_IF(!hasExtension());

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS), eglDebugMessageControlKHR(nullptr, nullptr));

    EXPECT_EQ(EGL_SUCCESS, eglLabelObjectKHR(EGL_NO_DISPLAY, EGL_OBJECT_THREAD_KHR, nullptr,
                                             const_cast<char *>("Thread")));

    // Enable all messages
    EGLAttrib controls[] = {
        EGL_DEBUG_MSG_CRITICAL_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_ERROR_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_WARN_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_INFO_KHR,
        EGL_TRUE,
        EGL_NONE,
        EGL_NONE,
    };

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS),
              eglDebugMessageControlKHR(&CheckBadBindAPIError, controls));

    // Generate an error and trigger the callback
    EXPECT_EGL_FALSE(eglBindAPI(0xBADDBADD));
}

// Test that labels can be set and that errors are generated if the wrong object type is used
TEST_P(EGLDebugTest, SetLabel)
{
    ANGLE_SKIP_TEST_IF(!hasExtension());

    EGLDisplay display = getEGLWindow()->getDisplay();
    EGLSurface surface = getEGLWindow()->getSurface();

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS), eglDebugMessageControlKHR(nullptr, nullptr));

    // Display display and object must be equal when setting a display label
    EXPECT_EQ(
        static_cast<EGLint>(EGL_SUCCESS),
        eglLabelObjectKHR(display, EGL_OBJECT_DISPLAY_KHR, display, const_cast<char *>("Display")));
    EXPECT_NE(static_cast<EGLint>(EGL_SUCCESS),
              eglLabelObjectKHR(nullptr, EGL_OBJECT_DISPLAY_KHR, getEGLWindow()->getDisplay(),
                                const_cast<char *>("Display")));

    //  Set a surface label
    EXPECT_EQ(
        static_cast<EGLint>(EGL_SUCCESS),
        eglLabelObjectKHR(display, EGL_OBJECT_SURFACE_KHR, surface, const_cast<char *>("Surface")));
    EXPECT_EGL_ERROR(EGL_SUCCESS);

    // Provide a surface but use an image label type
    EXPECT_EQ(
        static_cast<EGLint>(EGL_BAD_PARAMETER),
        eglLabelObjectKHR(display, EGL_OBJECT_IMAGE_KHR, surface, const_cast<char *>("Image")));
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Test that labels can be set on contexts, images and syncs, and that errors are generated for
// invalid objects of these types and for streams.
TEST_P(EGLDebugTest, SetLabelOnContextImageSyncAndStream)
{
    ANGLE_SKIP_TEST_IF(!hasExtension());

    EGLDisplay display         = getEGLWindow()->getDisplay();
    EGLContext context         = getEGLWindow()->getContext();
    EGLLabelKHR label          = const_cast<char *>("Label");
    EGLObjectKHR invalidObject = reinterpret_cast<EGLObjectKHR>(static_cast<uintptr_t>(0xBAD));

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS), eglDebugMessageControlKHR(nullptr, nullptr));

    EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS),
              eglLabelObjectKHR(display, EGL_OBJECT_CONTEXT_KHR, context, label));
    EXPECT_EQ(static_cast<EGLint>(EGL_BAD_CONTEXT),
              eglLabelObjectKHR(display, EGL_OBJECT_CONTEXT_KHR, invalidObject, label));

    if (IsEGLDisplayExtensionEnabled(display, "EGL_KHR_gl_texture_2D_image"))
    {
        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        EGLImageKHR image = eglCreateImageKHR(
            display, context, EGL_GL_TEXTURE_2D_KHR,
            reinterpret_cast<EGLClientBuffer>(static_cast<uintptr_t>(texture.get())), nullptr);
        ASSERT_NE(image, EGL_NO_IMAGE_KHR);
        EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS),
                  eglLabelObjectKHR(display, EGL_OBJECT_IMAGE_KHR, image, label));
        EXPECT_EGL_TRUE(eglDestroyImageKHR(display, image));
        EXPECT_EQ(static_cast<EGLint>(EGL_BAD_PARAMETER),
                  eglLabelObjectKHR(display, EGL_OBJECT_IMAGE_KHR, image, label));
    }

    if (IsEGLDisplayExtensionEnabled(display, "EGL_KHR_fence_sync"))
    {
        EGLSyncKHR sync = eglCreateSyncKHR(display, EGL_SYNC_FENCE_KHR, nullptr);
        ASSERT_NE(sync, EGL_NO_SYNC_KHR);
        EXPECT_EQ(static_cast<EGLint>(EGL_SUCCESS),
                  eglLabelObjectKHR(display, EGL_OBJECT_SYNC_KHR, sync, label));
        EXPECT_EGL_TRUE(eglDestroySyncKHR(display, sync));
        EXPECT_EQ(static_cast<EGLint>(EGL_BAD_PARAMETER),
                  eglLabelObjectKHR(display, EGL_OBJECT_SYNC_KHR, sync, label));
    }

    // |invalidObject| is not a stream, so this fails even if EGL_KHR_stream is supported.
    const EGLint expectedStreamError = IsEGLDisplayExtensionEnabled(display, "EGL_KHR_stream")
                                           ? EGL_BAD_STREAM_KHR
                                           : EGL_BAD_ACCESS;
    EXPECT_EQ(expectedStreamError,
              eglLabelObjectKHR(display, EGL_OBJECT_STREAM_KHR, invalidObject, label));
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLDebugTest);
ANGLE_INSTANTIATE_TEST(EGLDebugTest,
                       ES2_D3D11(),
                       ES3_D3D11(),
                       ES2_OPENGL(),
                       ES3_OPENGL(),
                       ES2_VULKAN());

}  // namespace angle

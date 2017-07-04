//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
//    EGLIOSurfaceClientBufferTest.cpp: tests for the EGL_ANGLE_iosurface_client_buffer extension.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

#include <CoreFoundation/CoreFoundation.h>
#include <IOSurface/IOSurface.h>

#define GL_UNSIGNED_INT_8_8_8_8_REV 0x8367

using namespace angle;

namespace
{

void AddIntegerValue(CFMutableDictionaryRef dictionary, const CFStringRef key, int32_t value)
{
    CFNumberRef number = CFNumberCreate(nullptr, kCFNumberSInt32Type, &value);
    CFDictionaryAddValue(dictionary, key, number);
    CFRelease(number);
}

}  // anonymous namespace

class IOSurfaceClientBufferTest : public ANGLETest
{
  protected:
    IOSurfaceClientBufferTest() : mConfig(0), mDisplay(nullptr) {}

    void SetUp() override
    {
        ANGLETest::SetUp();

        mConfig  = getEGLWindow()->getConfig();
        mDisplay = getEGLWindow()->getDisplay();
    }

    void TearDown() override { ANGLETest::TearDown(); }

    EGLConfig mConfig;
    EGLDisplay mDisplay;
};

TEST_P(IOSurfaceClientBufferTest, RenderToBGRA8888IOSurface)
{
    // Create a 1 by 1 BGRA8888 IOSurface
    IOSurfaceRef ioSurface = nullptr;

    CFMutableDictionaryRef dict = CFDictionaryCreateMutable(
        kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    AddIntegerValue(dict, kIOSurfaceWidth, 1);
    AddIntegerValue(dict, kIOSurfaceHeight, 1);
    AddIntegerValue(dict, kIOSurfacePixelFormat, 'BGRA');
    AddIntegerValue(dict, kIOSurfaceBytesPerElement, 4);

    ioSurface = IOSurfaceCreate(dict);
    CFRelease(dict);

    EXPECT_NE(nullptr, ioSurface);

    // Make a PBuffer from it using the EGL_ANGLE_iosurface_client_buffer extension
    // clang-format off
    const EGLint attribs[] = {
        EGL_WIDTH,                         1,
        EGL_HEIGHT,                        1,
        EGL_IOSURFACE_PLANE_ANGLE,         0,
        EGL_TEXTURE_TARGET,                EGL_TEXTURE_RECTANGLE_ANGLE,
        EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, GL_BGRA_EXT,
        EGL_TEXTURE_FORMAT,                EGL_TEXTURE_RGBA,
        EGL_TEXTURE_TYPE_ANGLE,            GL_UNSIGNED_BYTE,
        EGL_NONE,                          EGL_NONE,
    };
    // clang-format off

    EGLSurface pbuffer = eglCreatePbufferFromClientBuffer(mDisplay, EGL_IOSURFACE_ANGLE, ioSurface, mConfig, attribs);
    EXPECT_NE(EGL_NO_SURFACE, pbuffer);

    // Bind and glClear the pbuffer
    GLTexture tex;
    glBindTexture(GL_TEXTURE_RECTANGLE_ANGLE, tex);
    EGLBoolean result = eglBindTexImage(mDisplay, pbuffer, EGL_BACK_BUFFER);
    EXPECT_EGL_TRUE(result);
    EXPECT_EGL_SUCCESS();

    GLFramebuffer fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    EXPECT_GL_NO_ERROR();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ANGLE, tex, 0);
    EXPECT_GL_NO_ERROR();
    EXPECT_GLENUM_EQ(glCheckFramebufferStatus(GL_FRAMEBUFFER), GL_FRAMEBUFFER_COMPLETE);
    EXPECT_GL_NO_ERROR();

    glClearColor(0, 1, 0, 1);
    EXPECT_GL_NO_ERROR();
    glClear(GL_COLOR_BUFFER_BIT);
    EXPECT_GL_NO_ERROR();

    // Unbind pbuffer and check content.
    result = eglReleaseTexImage(mDisplay, pbuffer, EGL_BACK_BUFFER);
    EXPECT_EGL_TRUE(result);
    EXPECT_EGL_SUCCESS();

    IOSurfaceLock(ioSurface, kIOSurfaceLockReadOnly, nullptr);
    GLColor color = *reinterpret_cast<const GLColor*>(IOSurfaceGetBaseAddress(ioSurface));
    IOSurfaceUnlock(ioSurface, kIOSurfaceLockReadOnly, nullptr);

    CFRelease(ioSurface);

    EXPECT_EQ(color, GLColor::green);
}

ANGLE_INSTANTIATE_TEST(IOSurfaceClientBufferTest, ES3_OPENGL());

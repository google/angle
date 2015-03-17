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

#include "tcuANGLEWin32NativeDisplayFactory.h"

#include "egluDefs.hpp"
#include "tcuWin32Window.hpp"
#include "tcuWin32API.h"
#include "tcuTexture.hpp"
#include "deMemory.h"
#include "deThread.h"
#include "deClock.h"
#include "eglwLibrary.hpp"

#include <EGL/egl.h>
#include <EGL/eglext.h>

// Assume no call translation is needed
DE_STATIC_ASSERT(sizeof(eglw::EGLNativeDisplayType) == sizeof(HDC));
DE_STATIC_ASSERT(sizeof(eglw::EGLNativePixmapType) == sizeof(HBITMAP));
DE_STATIC_ASSERT(sizeof(eglw::EGLNativeWindowType) == sizeof(HWND));

namespace tcu
{
namespace
{

enum
{
    DEFAULT_SURFACE_WIDTH       = 400,
    DEFAULT_SURFACE_HEIGHT      = 300,
    WAIT_WINDOW_VISIBLE_MS      = 500   //!< Time to wait before issuing screenshot after changing window visibility (hack for DWM)
};

static const eglu::NativeDisplay::Capability DISPLAY_CAPABILITIES = eglu::NativeDisplay::CAPABILITY_GET_DISPLAY_PLATFORM;
static const eglu::NativePixmap::Capability  BITMAP_CAPABILITIES  = eglu::NativePixmap::CAPABILITY_CREATE_SURFACE_LEGACY;
static const eglu::NativeWindow::Capability  WINDOW_CAPABILITIES  = (eglu::NativeWindow::Capability)
                                                                     (eglu::NativeWindow::CAPABILITY_CREATE_SURFACE_LEGACY    |
                                                                      eglu::NativeWindow::CAPABILITY_GET_SURFACE_SIZE         |
                                                                      eglu::NativeWindow::CAPABILITY_GET_SCREEN_SIZE          |
                                                                      eglu::NativeWindow::CAPABILITY_READ_SCREEN_PIXELS       |
                                                                      eglu::NativeWindow::CAPABILITY_SET_SURFACE_SIZE         |
                                                                      eglu::NativeWindow::CAPABILITY_CHANGE_VISIBILITY);

class NativeDisplay : public eglu::NativeDisplay
{
  public:
    NativeDisplay();
    virtual ~NativeDisplay() {}

    void *getPlatformNative() override { return mDeviceContext; }
    const eglw::EGLAttrib *getPlatformAttributes() const override { return &mPlatformAttributes[0]; }
    const eglw::Library &getLibrary() const override { return mLibrary; }

    HDC getDeviceContext() { return mDeviceContext; }

  private:
    HDC mDeviceContext;
    eglw::DefaultLibrary mLibrary;
    std::vector<eglw::EGLAttrib> mPlatformAttributes;
};

class NativePixmapFactory : public eglu::NativePixmapFactory
{
  public:
    NativePixmapFactory();
    ~NativePixmapFactory() {}

    eglu::NativePixmap *createPixmap(eglu::NativeDisplay *nativeDisplay, int width, int height) const override;
    eglu::NativePixmap *createPixmap(eglu::NativeDisplay *nativeDisplay, eglw::EGLDisplay display, eglw::EGLConfig config, const eglw::EGLAttrib* attribList, int width, int height) const override;
};

class NativePixmap : public eglu::NativePixmap
{
  public:
    NativePixmap(NativeDisplay* nativeDisplay, int width, int height, int bitDepth);
    virtual ~NativePixmap();

    eglw::EGLNativePixmapType getLegacyNative() override { return mBitmap; }

  private:
    HBITMAP mBitmap;
};

class NativeWindowFactory : public eglu::NativeWindowFactory
{
  public:
    NativeWindowFactory(HINSTANCE instance);
    ~NativeWindowFactory() override {}

    eglu::NativeWindow *createWindow(eglu::NativeDisplay *nativeDisplay, const eglu::WindowParams &params) const override;

  private:
    const HINSTANCE mInstance;
};

class NativeWindow : public eglu::NativeWindow
{
  public:
    NativeWindow(NativeDisplay *nativeDisplay, HINSTANCE instance, const eglu::WindowParams &params);
    ~NativeWindow() override;

    eglw::EGLNativeWindowType getLegacyNative() override { return mWindow.getHandle(); }
    IVec2 getSurfaceSize() const override;
    IVec2 getScreenSize() const override { return getSurfaceSize(); }
    void processEvents() override;
    void setSurfaceSize(IVec2 size) override;
    void setVisibility(eglu::WindowParams::Visibility visibility) override;
    void readScreenPixels(tcu::TextureLevel* dst) const override;

  private:
    Win32Window mWindow;
    eglu::WindowParams::Visibility mCurVisibility;
    deUint64 mSetVisibleTime;       //!< Time window was set visible.
};

// NativeDisplay

NativeDisplay::NativeDisplay()
    : eglu::NativeDisplay(DISPLAY_CAPABILITIES, EGL_PLATFORM_ANGLE_ANGLE, "EGL_EXT_platform_base"),
      mDeviceContext((HDC)EGL_DEFAULT_DISPLAY),
      mLibrary("libEGL.dll")
{
    mPlatformAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    mPlatformAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);
    mPlatformAttributes.push_back(EGL_NONE);
    mPlatformAttributes.push_back(EGL_NONE);
}

// NativePixmap

NativePixmap::NativePixmap(NativeDisplay* nativeDisplay, int width, int height, int bitDepth)
    : eglu::NativePixmap(BITMAP_CAPABILITIES),
      mBitmap(DE_NULL)
{
    const HDC       deviceCtx   = nativeDisplay->getDeviceContext();
    BITMAPINFO      bitmapInfo;

    memset(&bitmapInfo, 0, sizeof(bitmapInfo));

    if (bitDepth != 24 && bitDepth != 32)
        throw NotSupportedError("Unsupported pixmap bit depth", DE_NULL, __FILE__, __LINE__);

    bitmapInfo.bmiHeader.biSize             = sizeof(bitmapInfo);
    bitmapInfo.bmiHeader.biWidth            = width;
    bitmapInfo.bmiHeader.biHeight           = height;
    bitmapInfo.bmiHeader.biPlanes           = 1;
    bitmapInfo.bmiHeader.biBitCount         = bitDepth;
    bitmapInfo.bmiHeader.biCompression      = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage        = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter    = 1;
    bitmapInfo.bmiHeader.biYPelsPerMeter    = 1;
    bitmapInfo.bmiHeader.biClrUsed          = 0;
    bitmapInfo.bmiHeader.biClrImportant     = 0;

    void* bitmapPtr = DE_NULL;
    mBitmap = CreateDIBSection(deviceCtx, &bitmapInfo, DIB_RGB_COLORS, &bitmapPtr, NULL, 0);

    if (!mBitmap)
        throw ResourceError("Failed to create bitmap", DE_NULL, __FILE__, __LINE__);
}

NativePixmap::~NativePixmap()
{
    DeleteObject(mBitmap);
}

// NativePixmapFactory

NativePixmapFactory::NativePixmapFactory()
    : eglu::NativePixmapFactory("bitmap", "ANGLE Bitmap", BITMAP_CAPABILITIES)
{
}

eglu::NativePixmap *NativePixmapFactory::createPixmap (eglu::NativeDisplay* nativeDisplay, eglw::EGLDisplay display, eglw::EGLConfig config, const eglw::EGLAttrib* attribList, int width, int height) const
{
    const eglw::Library&    egl         = nativeDisplay->getLibrary();
    int             redBits     = 0;
    int             greenBits   = 0;
    int             blueBits    = 0;
    int             alphaBits   = 0;
    int             bitSum      = 0;

    DE_ASSERT(display != EGL_NO_DISPLAY);

    egl.getConfigAttrib(display, config, EGL_RED_SIZE,      &redBits);
    egl.getConfigAttrib(display, config, EGL_GREEN_SIZE,    &greenBits);
    egl.getConfigAttrib(display, config, EGL_BLUE_SIZE,     &blueBits);
    egl.getConfigAttrib(display, config, EGL_ALPHA_SIZE,    &alphaBits);
    EGLU_CHECK_MSG(egl, "eglGetConfigAttrib()");

    bitSum = redBits+greenBits+blueBits+alphaBits;

    return new NativePixmap(dynamic_cast<NativeDisplay*>(nativeDisplay), width, height, bitSum);
}

eglu::NativePixmap *NativePixmapFactory::createPixmap(eglu::NativeDisplay* nativeDisplay, int width, int height) const
{
    const int defaultDepth = 32;
    return new NativePixmap(dynamic_cast<NativeDisplay*>(nativeDisplay), width, height, defaultDepth);
}

// NativeWindowFactory

NativeWindowFactory::NativeWindowFactory(HINSTANCE instance)
    : eglu::NativeWindowFactory("window", "ANGLE Window", WINDOW_CAPABILITIES),
      mInstance(instance)
{
}

eglu::NativeWindow *NativeWindowFactory::createWindow (eglu::NativeDisplay* nativeDisplay, const eglu::WindowParams& params) const
{
    return new NativeWindow(dynamic_cast<NativeDisplay*>(nativeDisplay), mInstance, params);
}

// NativeWindow

NativeWindow::NativeWindow(NativeDisplay *nativeDisplay, HINSTANCE instance, const eglu::WindowParams& params)
    : eglu::NativeWindow(WINDOW_CAPABILITIES),
      mWindow(instance,
              params.width   == eglu::WindowParams::SIZE_DONT_CARE ? DEFAULT_SURFACE_WIDTH   : params.width,
              params.height  == eglu::WindowParams::SIZE_DONT_CARE ? DEFAULT_SURFACE_HEIGHT  : params.height),
      mCurVisibility(eglu::WindowParams::VISIBILITY_HIDDEN),
      mSetVisibleTime(0)
{
    if (params.visibility != eglu::WindowParams::VISIBILITY_DONT_CARE)
        setVisibility(params.visibility);
}

void NativeWindow::setVisibility(eglu::WindowParams::Visibility visibility)
{
    switch (visibility)
    {
      case eglu::WindowParams::VISIBILITY_HIDDEN:
        mWindow.setVisible(false);
        mCurVisibility     = visibility;
        break;

      case eglu::WindowParams::VISIBILITY_VISIBLE:
      case eglu::WindowParams::VISIBILITY_FULLSCREEN:
        // \todo [2014-03-12 pyry] Implement FULLSCREEN, or at least SW_MAXIMIZE.
        mWindow.setVisible(true);
        mCurVisibility     = eglu::WindowParams::VISIBILITY_VISIBLE;
        mSetVisibleTime    = deGetMicroseconds();
        break;

      default:
        DE_ASSERT(DE_FALSE);
    }
}

NativeWindow::~NativeWindow()
{
}

IVec2 NativeWindow::getSurfaceSize() const
{
    return mWindow.getSize();
}

void NativeWindow::processEvents()
{
    mWindow.processEvents();
}

void NativeWindow::setSurfaceSize(IVec2 size)
{
    mWindow.setSize(size.x(), size.y());
}

void NativeWindow::readScreenPixels(tcu::TextureLevel *dst) const
{
    HDC         windowDC    = DE_NULL;
    HDC         screenDC    = DE_NULL;
    HDC         tmpDC       = DE_NULL;
    HBITMAP     tmpBitmap   = DE_NULL;
    RECT        rect;

    TCU_CHECK_INTERNAL(mCurVisibility != eglu::WindowParams::VISIBILITY_HIDDEN);

    // Hack for DWM: There is no way to wait for DWM animations to finish, so we just have to wait
    // for a while before issuing screenshot if window was just made visible.
    {
        const deInt64 timeSinceVisibleUs = (deInt64)(deGetMicroseconds()-mSetVisibleTime);

        if (timeSinceVisibleUs < (deInt64)WAIT_WINDOW_VISIBLE_MS*1000)
            deSleep(WAIT_WINDOW_VISIBLE_MS - (deUint32)(timeSinceVisibleUs/1000));
    }

    TCU_CHECK(GetClientRect(mWindow.getHandle(), &rect));

    try
    {
        const int           width       = rect.right - rect.left;
        const int           height      = rect.bottom - rect.top;
        BITMAPINFOHEADER    bitmapInfo;

        deMemset(&bitmapInfo, 0, sizeof(bitmapInfo));

        screenDC = GetDC(DE_NULL);
        TCU_CHECK(screenDC);

        windowDC = GetDC(mWindow.getHandle());
        TCU_CHECK(windowDC);

        tmpDC = CreateCompatibleDC(screenDC);
        TCU_CHECK(tmpDC != DE_NULL);

        MapWindowPoints(mWindow.getHandle(), DE_NULL, (LPPOINT)&rect, 2);

        tmpBitmap = CreateCompatibleBitmap(screenDC, width, height);
        TCU_CHECK(tmpBitmap != DE_NULL);

        TCU_CHECK(SelectObject(tmpDC, tmpBitmap) != DE_NULL);

        TCU_CHECK(BitBlt(tmpDC, 0, 0, width, height, screenDC, rect.left, rect.top, SRCCOPY));


        bitmapInfo.biSize           = sizeof(BITMAPINFOHEADER);
        bitmapInfo.biWidth          = width;
        bitmapInfo.biHeight         = -height;
        bitmapInfo.biPlanes         = 1;
        bitmapInfo.biBitCount       = 32;
        bitmapInfo.biCompression    = BI_RGB;
        bitmapInfo.biSizeImage      = 0;
        bitmapInfo.biXPelsPerMeter  = 0;
        bitmapInfo.biYPelsPerMeter  = 0;
        bitmapInfo.biClrUsed        = 0;
        bitmapInfo.biClrImportant   = 0;

        dst->setStorage(TextureFormat(TextureFormat::BGRA, TextureFormat::UNORM_INT8), width, height);

        TCU_CHECK(GetDIBits(screenDC, tmpBitmap, 0, height, dst->getAccess().getDataPtr(), (BITMAPINFO*)&bitmapInfo, DIB_RGB_COLORS));

        DeleteObject(tmpBitmap);
        tmpBitmap = DE_NULL;

        ReleaseDC(DE_NULL, screenDC);
        screenDC = DE_NULL;

        ReleaseDC(mWindow.getHandle(), windowDC);
        windowDC = DE_NULL;

        DeleteDC(tmpDC);
        tmpDC = DE_NULL;
    }
    catch (...)
    {
        if (screenDC)
            ReleaseDC(DE_NULL, screenDC);

        if (windowDC)
            ReleaseDC(mWindow.getHandle(), windowDC);

        if (tmpBitmap)
            DeleteObject(tmpBitmap);

        if (tmpDC)
            DeleteDC(tmpDC);

        throw;
    }
}

} // anonymous

ANGLEWin32NativeDisplayFactory::ANGLEWin32NativeDisplayFactory(HINSTANCE instance)
    : eglu::NativeDisplayFactory("angle", "Native ANGLE Display", DISPLAY_CAPABILITIES, EGL_PLATFORM_ANGLE_ANGLE, "EGL_EXT_platform_base"),
      mInstance(instance)
{
    m_nativeWindowRegistry.registerFactory(new NativeWindowFactory(mInstance));
    m_nativePixmapRegistry.registerFactory(new NativePixmapFactory());
}

ANGLEWin32NativeDisplayFactory::~ANGLEWin32NativeDisplayFactory()
{
}

eglu::NativeDisplay *ANGLEWin32NativeDisplayFactory::createDisplay(const eglw::EGLAttrib *attribList) const
{
    DE_UNREF(attribList);
    return new NativeDisplay();
}

} // tcu

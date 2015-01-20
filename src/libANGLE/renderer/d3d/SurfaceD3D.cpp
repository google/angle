//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SurfaceD3D.cpp: D3D implementation of an EGL surface

#include "libANGLE/renderer/d3d/SurfaceD3D.h"

#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/SwapChainD3D.h"

#include <tchar.h>
#include <EGL/eglext.h>
#include <algorithm>

namespace rx
{

SurfaceD3D *SurfaceD3D::createOffscreen(RendererD3D *renderer, egl::Display *display, const egl::Config *config, EGLClientBuffer shareHandle,
                                        EGLint width, EGLint height, EGLenum textureFormat, EGLenum textureType)
{
    return new SurfaceD3D(renderer, display, config, width, height, EGL_TRUE, EGL_FALSE,
                          textureFormat, textureType, shareHandle, NULL);
}

SurfaceD3D *SurfaceD3D::createFromWindow(RendererD3D *renderer, egl::Display *display, const egl::Config *config, EGLNativeWindowType window,
                                         EGLint fixedSize, EGLint width, EGLint height, EGLint postSubBufferSupported)
{
    return new SurfaceD3D(renderer, display, config, width, height, fixedSize, postSubBufferSupported,
                          EGL_NO_TEXTURE, EGL_NO_TEXTURE, static_cast<EGLClientBuffer>(0), window);
}

SurfaceD3D::SurfaceD3D(RendererD3D *renderer, egl::Display *display, const egl::Config *config, EGLint width, EGLint height,
                       EGLint fixedSize, EGLint postSubBufferSupported, EGLenum textureFormat,
                       EGLenum textureType, EGLClientBuffer shareHandle, EGLNativeWindowType window)
    : SurfaceImpl(display, config, fixedSize, postSubBufferSupported, textureFormat, textureType),
      mRenderer(renderer),
      mSwapChain(NULL),
      mSwapIntervalDirty(true),
      mWindowSubclassed(false),
      mNativeWindow(window),
      mWidth(width),
      mHeight(height),
      mSwapInterval(1),
      mShareHandle(reinterpret_cast<HANDLE*>(shareHandle))
{
    subclassWindow();
}

SurfaceD3D::~SurfaceD3D()
{
    unsubclassWindow();
    releaseSwapChain();
}

void SurfaceD3D::releaseSwapChain()
{
    SafeDelete(mSwapChain);
}

egl::Error SurfaceD3D::initialize()
{
    if (mNativeWindow.getNativeWindow())
    {
        if (!mNativeWindow.initialize())
        {
            return egl::Error(EGL_BAD_SURFACE);
        }
    }

    egl::Error error = resetSwapChain();
    if (error.isError())
    {
        return error;
    }

    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::bindTexImage(EGLint)
{
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::releaseTexImage(EGLint)
{
    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::resetSwapChain()
{
    ASSERT(!mSwapChain);

    int width;
    int height;

    if (!mFixedSize)
    {
        RECT windowRect;
        if (!mNativeWindow.getClientRect(&windowRect))
        {
            ASSERT(false);

            return egl::Error(EGL_BAD_SURFACE, "Could not retrieve the window dimensions");
        }

        width = windowRect.right - windowRect.left;
        height = windowRect.bottom - windowRect.top;
    }
    else
    {
        // non-window surface - size is determined at creation
        width = mWidth;
        height = mHeight;
    }

    mSwapChain = mRenderer->createSwapChain(mNativeWindow, mShareHandle,
                                            mConfig->renderTargetFormat,
                                            mConfig->depthStencilFormat);
    if (!mSwapChain)
    {
        return egl::Error(EGL_BAD_ALLOC);
    }

    egl::Error error = resetSwapChain(width, height);
    if (error.isError())
    {
        SafeDelete(mSwapChain);
        return error;
    }

    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::resizeSwapChain(int backbufferWidth, int backbufferHeight)
{
    ASSERT(backbufferWidth >= 0 && backbufferHeight >= 0);
    ASSERT(mSwapChain);

    EGLint status = mSwapChain->resize(std::max(1, backbufferWidth), std::max(1, backbufferHeight));

    if (status == EGL_CONTEXT_LOST)
    {
        mDisplay->notifyDeviceLost();
        return egl::Error(status);
    }
    else if (status != EGL_SUCCESS)
    {
        return egl::Error(status);
    }

    mWidth = backbufferWidth;
    mHeight = backbufferHeight;

    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::resetSwapChain(int backbufferWidth, int backbufferHeight)
{
    ASSERT(backbufferWidth >= 0 && backbufferHeight >= 0);
    ASSERT(mSwapChain);

    EGLint status = mSwapChain->reset(std::max(1, backbufferWidth), std::max(1, backbufferHeight), mSwapInterval);

    if (status == EGL_CONTEXT_LOST)
    {
        mRenderer->notifyDeviceLost();
        return egl::Error(status);
    }
    else if (status != EGL_SUCCESS)
    {
        return egl::Error(status);
    }

    mWidth = backbufferWidth;
    mHeight = backbufferHeight;
    mSwapIntervalDirty = false;

    return egl::Error(EGL_SUCCESS);
}

egl::Error SurfaceD3D::swapRect(EGLint x, EGLint y, EGLint width, EGLint height)
{
    if (!mSwapChain)
    {
        return egl::Error(EGL_SUCCESS);
    }

    if (x + width > mWidth)
    {
        width = mWidth - x;
    }

    if (y + height > mHeight)
    {
        height = mHeight - y;
    }

    if (width == 0 || height == 0)
    {
        return egl::Error(EGL_SUCCESS);
    }

    EGLint status = mSwapChain->swapRect(x, y, width, height);

    if (status == EGL_CONTEXT_LOST)
    {
        mRenderer->notifyDeviceLost();
        return egl::Error(status);
    }
    else if (status != EGL_SUCCESS)
    {
        return egl::Error(status);
    }

    checkForOutOfDateSwapChain();

    return egl::Error(EGL_SUCCESS);
}

EGLNativeWindowType SurfaceD3D::getWindowHandle() const
{
    return mNativeWindow.getNativeWindow();
}

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
#define kSurfaceProperty _TEXT("Egl::SurfaceOwner")
#define kParentWndProc _TEXT("Egl::SurfaceParentWndProc")

static LRESULT CALLBACK SurfaceWindowProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  if (message == WM_SIZE)
  {
      SurfaceD3D* surf = reinterpret_cast<SurfaceD3D*>(GetProp(hwnd, kSurfaceProperty));
      if(surf)
      {
          surf->checkForOutOfDateSwapChain();
      }
  }
  WNDPROC prevWndFunc = reinterpret_cast<WNDPROC >(GetProp(hwnd, kParentWndProc));
  return CallWindowProc(prevWndFunc, hwnd, message, wparam, lparam);
}
#endif

void SurfaceD3D::subclassWindow()
{
#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
    HWND window = mNativeWindow.getNativeWindow();
    if (!window)
    {
        return;
    }

    DWORD processId;
    DWORD threadId = GetWindowThreadProcessId(window, &processId);
    if (processId != GetCurrentProcessId() || threadId != GetCurrentThreadId())
    {
        return;
    }

    SetLastError(0);
    LONG_PTR oldWndProc = SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(SurfaceWindowProc));
    if(oldWndProc == 0 && GetLastError() != ERROR_SUCCESS)
    {
        mWindowSubclassed = false;
        return;
    }

    SetProp(window, kSurfaceProperty, reinterpret_cast<HANDLE>(this));
    SetProp(window, kParentWndProc, reinterpret_cast<HANDLE>(oldWndProc));
    mWindowSubclassed = true;
#endif
}

void SurfaceD3D::unsubclassWindow()
{
    if (!mWindowSubclassed)
    {
        return;
    }

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
    HWND window = mNativeWindow.getNativeWindow();
    if (!window)
    {
        return;
    }

    // un-subclass
    LONG_PTR parentWndFunc = reinterpret_cast<LONG_PTR>(GetProp(window, kParentWndProc));

    // Check the windowproc is still SurfaceWindowProc.
    // If this assert fails, then it is likely the application has subclassed the
    // hwnd as well and did not unsubclass before destroying its EGL context. The
    // application should be modified to either subclass before initializing the
    // EGL context, or to unsubclass before destroying the EGL context.
    if(parentWndFunc)
    {
        LONG_PTR prevWndFunc = SetWindowLongPtr(window, GWLP_WNDPROC, parentWndFunc);
        UNUSED_ASSERTION_VARIABLE(prevWndFunc);
        ASSERT(prevWndFunc == reinterpret_cast<LONG_PTR>(SurfaceWindowProc));
    }

    RemoveProp(window, kSurfaceProperty);
    RemoveProp(window, kParentWndProc);
#endif
    mWindowSubclassed = false;
}

bool SurfaceD3D::checkForOutOfDateSwapChain()
{
    RECT client;
    int clientWidth = getWidth();
    int clientHeight = getHeight();
    bool sizeDirty = false;
    if (!mFixedSize && !mNativeWindow.isIconic())
    {
        // The window is automatically resized to 150x22 when it's minimized, but the swapchain shouldn't be resized
        // because that's not a useful size to render to.
        if (!mNativeWindow.getClientRect(&client))
        {
            ASSERT(false);
            return false;
        }

        // Grow the buffer now, if the window has grown. We need to grow now to avoid losing information.
        clientWidth = client.right - client.left;
        clientHeight = client.bottom - client.top;
        sizeDirty = clientWidth != getWidth() || clientHeight != getHeight();
    }

    bool wasDirty = (mSwapIntervalDirty || sizeDirty);

    if (mSwapIntervalDirty)
    {
        resetSwapChain(clientWidth, clientHeight);
    }
    else if (sizeDirty)
    {
        resizeSwapChain(clientWidth, clientHeight);
    }

    return wasDirty;
}

egl::Error SurfaceD3D::swap()
{
    return swapRect(0, 0, mWidth, mHeight);
}

egl::Error SurfaceD3D::postSubBuffer(EGLint x, EGLint y, EGLint width, EGLint height)
{
    return swapRect(x, y, width, height);
}

rx::SwapChainD3D *SurfaceD3D::getSwapChain() const
{
    return mSwapChain;
}

void SurfaceD3D::setSwapInterval(EGLint interval)
{
    if (mSwapInterval == interval)
    {
        return;
    }

    mSwapInterval = interval;
    mSwapInterval = std::max(mSwapInterval, mConfig->minSwapInterval);
    mSwapInterval = std::min(mSwapInterval, mConfig->maxSwapInterval);

    mSwapIntervalDirty = true;
}

EGLint SurfaceD3D::getWidth() const
{
    return mWidth;
}

EGLint SurfaceD3D::getHeight() const
{
    return mHeight;
}

egl::Error SurfaceD3D::querySurfacePointerANGLE(EGLint attribute, void **value)
{
    ASSERT(attribute == EGL_D3D_TEXTURE_2D_SHARE_HANDLE_ANGLE);
    *value = mSwapChain->getShareHandle();
    return egl::Error(EGL_SUCCESS);
}

}

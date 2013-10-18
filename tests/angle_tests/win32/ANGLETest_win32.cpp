#include "ANGLETest.h"

#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
      case WM_CLOSE:
        PostQuitMessage(0);
        return 1;

      default:
        break;
    }

    return DefWindowProc(hWnd, message, wParam, lParam);
}

static const PTCHAR GetTestWindowName()
{
    return TEXT("ANGLE_TEST");
}

bool ANGLETest::InitTestWindow()
{
    WNDCLASS sWC;
    sWC.style = CS_OWNDC;
    sWC.lpfnWndProc = WndProc;
    sWC.cbClsExtra = 0;
    sWC.cbWndExtra = 0;
    sWC.hInstance = NULL;
    sWC.hIcon = NULL;
    sWC.hCursor = LoadCursor(NULL, IDC_ARROW);
    sWC.lpszMenuName = NULL;
    sWC.hbrBackground = NULL;
    sWC.lpszClassName = GetTestWindowName();

    if (!RegisterClass(&sWC))
    {
        return false;
    }

    mNativeWindow = CreateWindow(GetTestWindowName(), NULL, WS_BORDER, 128, 128, 128, 128, NULL, NULL, NULL, NULL);

    SetWindowLong(mNativeWindow, GWL_STYLE, 0);
    ShowWindow(mNativeWindow, SW_SHOW);

    mNativeDisplay = GetDC(mNativeWindow);
    if (!mNativeDisplay)
    {
        DestroyTestWindow();
        return false;
    }

    mDisplay = eglGetDisplay(mNativeDisplay);
    if(mDisplay == EGL_NO_DISPLAY)
    {
         mDisplay = eglGetDisplay((EGLNativeDisplayType)EGL_DEFAULT_DISPLAY);
    }

    EGLint majorVersion, minorVersion;
    if (!eglInitialize(mDisplay, &majorVersion, &minorVersion))
    {
        DestroyTestWindow();
        return false;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    if (eglGetError() != EGL_SUCCESS)
    {
        DestroyTestWindow();
        return false;
    }

    return true;
}

bool ANGLETest::DestroyTestWindow()
{
    eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    eglTerminate(mDisplay);

    if (mNativeDisplay)
    {
        ReleaseDC(mNativeWindow, mNativeDisplay);
        mNativeDisplay = 0;
    }

    if (mNativeWindow)
    {
        DestroyWindow(mNativeWindow);
        mNativeWindow = 0;
    }

    UnregisterClass(GetTestWindowName(), NULL);

    return true;
}

bool ANGLETest::ReizeWindow(int width, int height)
{
    RECT windowRect;
    if (!GetWindowRect(mNativeWindow, &windowRect))
    {
        return false;
    }

    if (!MoveWindow(mNativeWindow, windowRect.left, windowRect.top, width, height, FALSE))
    {
        return false;
    }

    return true;
}

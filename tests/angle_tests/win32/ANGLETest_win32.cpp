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

    return true;
}

bool ANGLETest::DestroyTestWindow()
{
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

bool ANGLETest::ResizeWindow(int width, int height)
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

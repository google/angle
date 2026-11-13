#include "CapturedTest_MECActiveSideContext_ES3_Vulkan.h"
#include "angle_trace_gl.h"

// Private Functions

void ReplayFrame1(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[19]);
    UpdateCurrentContext(19);
    eglGetCurrentContext();
    eglGetCurrentSurface(12377);
    eglGetCurrentSurface(12378);
    glClearColor(1, 0.000000000000000, 0.000000000000000, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, (void *)gReadBuffer);
    glGetError();
    glGetError();
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    eglGetError();
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[0]);
}

void ReplayFrame2(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    eglGetError();
}

void ReplayFrame3(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    eglGetError();
}

void ReplayFrame4(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    eglGetError();
}

void ResetReplayContextShared(void)
{
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
}

void ResetReplayContext18(void)
{
}

void ResetReplayContext19(void)
{
}

void ReplayFrame5(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    eglGetError();
}

// Public Functions

void SetupReplay(void)
{
    InitReplay();
    SetupReplayContextShared();
    if (gReplayResourceMode == angle::ReplayResourceMode::All)
    {
        SetupReplayContextSharedInactive();
    }
    SetCurrentContextID(18);
    CreateContext(19);
    SetupReplayContext19();
    SetupReplayContext18();


    eglMakeCurrent(NULL, NULL, NULL, gContextMap2[18]);
    UpdateCurrentContext(18);
}

void ResetReplay(void)
{
    ResetReplayContextShared();
    ResetReplayContext18();
    ResetReplayContext19();

    // Reset main context state
}


#include "CapturedTest_MECActiveSideContext_ES3_Vulkan.h"
#include "angle_trace_gl.h"

// Private Functions

void SetupReplayContext18(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[18]);
    UpdateCurrentContext(18);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformFeedbackMap[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glClearColor(0.000000000000000, 0.000000000000000, 1, 1);
    glViewport(0, 0, 128, 128);
    glScissor(0, 0, 128, 128);
}

void SetupReplayContext19(void)
{
    eglMakeCurrent(gEGLDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, gContextMap2[19]);
    UpdateCurrentContext(19);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, gTransformFeedbackMap[0]);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glClearColor(0.000000000000000, 1, 0.000000000000000, 1);
    glViewport(0, 0, 1, 1);
    glScissor(0, 0, 1, 1);
}

// Public Functions


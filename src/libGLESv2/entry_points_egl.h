//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// entry_points_egl.h : Defines the EGL entry points.

#ifndef LIBGLESV2_ENTRYPOINTSEGL_H_
#define LIBGLESV2_ENTRYPOINTSEGL_H_

#include "libGLESv2/export.h"

#include <EGL/egl.h>

namespace egl
{

// EGL 1.0
ANGLE_EXPORT EGLBoolean ChooseConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *configs, EGLint config_size, EGLint *num_config);
ANGLE_EXPORT EGLBoolean CopyBuffers(EGLDisplay dpy, EGLSurface surface, EGLNativePixmapType target);
ANGLE_EXPORT EGLContext CreateContext(EGLDisplay dpy, EGLConfig config, EGLContext share_context, const EGLint *attrib_list);
ANGLE_EXPORT EGLSurface CreatePbufferSurface(EGLDisplay dpy, EGLConfig config, const EGLint *attrib_list);
ANGLE_EXPORT EGLSurface CreatePixmapSurface(EGLDisplay dpy, EGLConfig config, EGLNativePixmapType pixmap, const EGLint *attrib_list);
ANGLE_EXPORT EGLSurface CreateWindowSurface(EGLDisplay dpy, EGLConfig config, EGLNativeWindowType win, const EGLint *attrib_list);
ANGLE_EXPORT EGLBoolean DestroyContext(EGLDisplay dpy, EGLContext ctx);
ANGLE_EXPORT EGLBoolean DestroySurface(EGLDisplay dpy, EGLSurface surface);
ANGLE_EXPORT EGLBoolean GetConfigAttrib(EGLDisplay dpy, EGLConfig config, EGLint attribute, EGLint *value);
ANGLE_EXPORT EGLBoolean GetConfigs(EGLDisplay dpy, EGLConfig *configs, EGLint config_size, EGLint *num_config);
ANGLE_EXPORT EGLDisplay GetCurrentDisplay(void);
ANGLE_EXPORT EGLSurface GetCurrentSurface(EGLint readdraw);
ANGLE_EXPORT EGLDisplay GetDisplay(EGLNativeDisplayType display_id);
ANGLE_EXPORT EGLint GetError(void);
ANGLE_EXPORT __eglMustCastToProperFunctionPointerType GetProcAddress(const char *procname);
ANGLE_EXPORT EGLBoolean Initialize(EGLDisplay dpy, EGLint *major, EGLint *minor);
ANGLE_EXPORT EGLBoolean MakeCurrent(EGLDisplay dpy, EGLSurface draw, EGLSurface read, EGLContext ctx);
ANGLE_EXPORT EGLBoolean QueryContext(EGLDisplay dpy, EGLContext ctx, EGLint attribute, EGLint *value);
ANGLE_EXPORT const char *QueryString(EGLDisplay dpy, EGLint name);
ANGLE_EXPORT EGLBoolean QuerySurface(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint *value);
ANGLE_EXPORT EGLBoolean SwapBuffers(EGLDisplay dpy, EGLSurface surface);
ANGLE_EXPORT EGLBoolean Terminate(EGLDisplay dpy);
ANGLE_EXPORT EGLBoolean WaitGL(void);
ANGLE_EXPORT EGLBoolean WaitNative(EGLint engine);

// EGL 1.1
ANGLE_EXPORT EGLBoolean BindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
ANGLE_EXPORT EGLBoolean ReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer);
ANGLE_EXPORT EGLBoolean SurfaceAttrib(EGLDisplay dpy, EGLSurface surface, EGLint attribute, EGLint value);
ANGLE_EXPORT EGLBoolean SwapInterval(EGLDisplay dpy, EGLint interval);

// EGL 1.2
ANGLE_EXPORT EGLBoolean BindAPI(EGLenum api);
ANGLE_EXPORT EGLenum QueryAPI(void);
ANGLE_EXPORT EGLSurface CreatePbufferFromClientBuffer(EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer, EGLConfig config, const EGLint *attrib_list);
ANGLE_EXPORT EGLBoolean ReleaseThread(void);
ANGLE_EXPORT EGLBoolean WaitClient(void);

// EGL 1.4
ANGLE_EXPORT EGLContext GetCurrentContext(void);

// EGL 1.5
ANGLE_EXPORT EGLSync CreateSync(EGLDisplay dpy, EGLenum type, const EGLAttrib *attrib_list);
ANGLE_EXPORT EGLBoolean DestroySync(EGLDisplay dpy, EGLSync sync);
ANGLE_EXPORT EGLint ClientWaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags, EGLTime timeout);
ANGLE_EXPORT EGLBoolean GetSyncAttrib(EGLDisplay dpy, EGLSync sync, EGLint attribute, EGLAttrib *value);
ANGLE_EXPORT EGLDisplay GetPlatformDisplay(EGLenum platform, void *native_display, const EGLAttrib *attrib_list);
ANGLE_EXPORT EGLSurface CreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config, void *native_window, const EGLAttrib *attrib_list);
ANGLE_EXPORT EGLSurface CreatePlatformPixmapSurface(EGLDisplay dpy, EGLConfig config, void *native_pixmap, const EGLAttrib *attrib_list);
ANGLE_EXPORT EGLBoolean WaitSync(EGLDisplay dpy, EGLSync sync, EGLint flags);

}

#endif // LIBGLESV2_ENTRYPOINTSEGL_H_

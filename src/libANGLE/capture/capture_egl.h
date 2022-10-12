//
// Copyright 2022 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// capture_egl.h:
//   EGL capture functions
//

#ifndef LIBANGLE_CAPTURE_EGL_H_
#define LIBANGLE_CAPTURE_EGL_H_

#include "libANGLE/Context.h"
#include "libANGLE/Thread.h"
#include "libANGLE/capture/FrameCapture.h"

namespace gl
{
enum class GLenumGroup;
}

namespace egl
{

template <typename CaptureFuncT, typename... ArgsT>
void CaptureCallToCaptureEGL(CaptureFuncT captureFunc, egl::Thread *thread, ArgsT... captureParams)
{
    gl::Context *context = thread->getContext();
    if (!context)
    {
        return;
    }

    angle::FrameCaptureShared *frameCaptureShared =
        context->getShareGroup()->getFrameCaptureShared();
    if (!frameCaptureShared->isCapturing())
    {
        return;
    }

    angle::CallCapture call = captureFunc(captureParams...);

    frameCaptureShared->captureCall(context, std::move(call), true);
}

angle::CallCapture CaptureCreateNativeClientBufferANDROID(const egl::AttributeMap &attribMap,
                                                          EGLClientBuffer eglClientBuffer);
angle::CallCapture CaptureEGLCreateImage(egl::Display *display,
                                         gl::Context *context,
                                         EGLenum target,
                                         EGLClientBuffer buffer,
                                         const egl::AttributeMap &attributes,
                                         egl::Image *image);
angle::CallCapture CaptureEGLDestroyImage(egl::Display *display, egl::Image *image);

angle::CallCapture CaptureEGLCreatePbufferSurface(egl::Display *display,
                                                  egl::Config *config,
                                                  const AttributeMap &attrib_list,
                                                  egl::Surface *surface);

angle::CallCapture CaptureEGLDestroySurface(egl::Display *display, Surface *surface);

angle::CallCapture CaptureEGLBindTexImage(egl::Display *display,
                                          egl::Surface *surface,
                                          EGLint buffer);

angle::CallCapture CaptureEGLReleaseTexImage(egl::Display *display,
                                             egl::Surface *surface,
                                             EGLint buffer);

angle::CallCapture CaptureEGLMakeCurrent(egl::Display *display,
                                         Surface *drawSurface,
                                         Surface *readSurface,
                                         gl::Context *context);
}  // namespace egl

#endif  // LIBANGLE_FRAME_CAPTURE_H_

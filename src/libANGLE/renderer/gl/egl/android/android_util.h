//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// android_util.h: Utilities for the using the Android platform

#ifndef LIBANGLE_RENDERER_GL_EGL_ANDROID_ANDROID_UTIL_H_
#define LIBANGLE_RENDERER_GL_EGL_ANDROID_ANDROID_UTIL_H_

#include "angle_gl.h"

namespace rx
{

namespace android
{
GLenum NativePixelFormatToGLInternalFormat(int pixelFormat);
}
}

#endif  // LIBANGLE_RENDERER_GL_EGL_ANDROID_ANDROID_UTIL_H_
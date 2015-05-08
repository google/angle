//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// platform_glx.h: Includes specific to GLX.

#ifndef LIBANGLE_RENDERER_GL_GLX_PLATFORMGLX_H_
#define LIBANGLE_RENDERER_GL_GLX_PLATFORMGLX_H_

// HACK(cwallez) this is a horrible hack to prevent glx from including GL/glext.h
// as it causes a bunch of conflicts (macro redefinition, etc) with GLES2/gl2ext.h
#define __glext_h_ 1
#include <GL/glx.h>
#undef __glext_h_

#endif // LIBANGLE_RENDERER_GL_GLX_PLATFORMGLX_H_

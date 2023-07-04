//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// context_local_call.h:
//   Helpers that set/get state that is entirely locally accessed by the context.

#include "libANGLE/context_local_call_autogen.h"

namespace gl
{
void ContextLocalClearColor(Context *context,
                            GLfloat red,
                            GLfloat green,
                            GLfloat blue,
                            GLfloat alpha)
{
    context->getMutableLocalState()->setColorClearValue(red, green, blue, alpha);
}

void ContextLocalClearDepthf(Context *context, GLfloat depth)
{
    context->getMutableLocalState()->setDepthClearValue(clamp01(depth));
}

void ContextLocalClearStencil(Context *context, GLint stencil)
{
    context->getMutableLocalState()->setStencilClearValue(stencil);
}

void ContextLocalClearColorx(Context *context,
                             GLfixed red,
                             GLfixed green,
                             GLfixed blue,
                             GLfixed alpha)
{
    context->getMutableLocalState()->setColorClearValue(
        ConvertFixedToFloat(red), ConvertFixedToFloat(green), ConvertFixedToFloat(blue),
        ConvertFixedToFloat(alpha));
}

void ContextLocalClearDepthx(Context *context, GLfixed depth)
{
    context->getMutableLocalState()->setDepthClearValue(clamp01(ConvertFixedToFloat(depth)));
}

}  // namespace gl

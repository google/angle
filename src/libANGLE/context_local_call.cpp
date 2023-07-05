//
// Copyright 2023 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// context_local_call.h:
//   Helpers that set/get state that is entirely locally accessed by the context.

#include "libANGLE/context_local_call_autogen.h"

#include "libANGLE/queryconversions.h"

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

void ContextLocalColorMask(Context *context,
                           GLboolean red,
                           GLboolean green,
                           GLboolean blue,
                           GLboolean alpha)
{
    context->getMutableLocalState()->setColorMask(ConvertToBool(red), ConvertToBool(green),
                                                  ConvertToBool(blue), ConvertToBool(alpha));
    context->onContextLocalColorMaskChange();
}

void ContextLocalColorMaski(Context *context,
                            GLuint index,
                            GLboolean r,
                            GLboolean g,
                            GLboolean b,
                            GLboolean a)
{
    context->getMutableLocalState()->setColorMaskIndexed(ConvertToBool(r), ConvertToBool(g),
                                                         ConvertToBool(b), ConvertToBool(a), index);
    context->onContextLocalColorMaskChange();
}

void ContextLocalDepthMask(Context *context, GLboolean flag)
{
    context->getMutableLocalState()->setDepthMask(ConvertToBool(flag));
}

void ContextLocalDisable(Context *context, GLenum cap)
{
    context->getMutableLocalState()->setEnableFeature(cap, false);
    context->onContextLocalCapChange();
}

void ContextLocalDisablei(Context *context, GLenum target, GLuint index)
{
    context->getMutableLocalState()->setEnableFeatureIndexed(target, false, index);
    context->onContextLocalCapChange();
}

void ContextLocalEnable(Context *context, GLenum cap)
{
    context->getMutableLocalState()->setEnableFeature(cap, true);
    context->onContextLocalCapChange();
}

void ContextLocalEnablei(Context *context, GLenum target, GLuint index)
{
    context->getMutableLocalState()->setEnableFeatureIndexed(target, true, index);
    context->onContextLocalCapChange();
}
}  // namespace gl

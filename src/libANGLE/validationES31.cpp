//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES31.cpp: Validation functions for OpenGL ES 3.1 entry point parameters

#include "libANGLE/validationES31.h"

#include "libANGLE/Context.h"
#include "libANGLE/validationES.h"
#include "libANGLE/validationES3.h"
#include "libANGLE/VertexArray.h"

#include "common/utilities.h"

using namespace angle;

namespace gl
{

bool ValidateGetBooleani_v(Context *context, GLenum target, GLuint index, GLboolean *data)
{
    if (context->getClientVersion() < ES_3_1)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "Context does not support GLES3.1"));
        return false;
    }

    if (!ValidateIndexedStateQuery(context, target, index, nullptr))
    {
        return false;
    }

    return true;
}

bool ValidateGetBooleani_vRobustANGLE(Context *context,
                                      GLenum target,
                                      GLuint index,
                                      GLsizei bufSize,
                                      GLsizei *length,
                                      GLboolean *data)
{
    if (context->getClientVersion() < ES_3_1)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "Context does not support GLES3.1"));
        return false;
    }

    if (!ValidateRobustEntryPoint(context, bufSize))
    {
        return false;
    }

    if (!ValidateIndexedStateQuery(context, target, index, length))
    {
        return false;
    }

    if (!ValidateRobustBufferSize(context, bufSize, *length))
    {
        return false;
    }

    return true;
}

bool ValidateDrawIndirectBase(Context *context, GLenum mode, const GLvoid *indirect)
{
    if (context->getClientVersion() < ES_3_1)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "Context does not support GLES3.1"));
        return false;
    }

    // Here the third parameter 1 is only to pass the count validation.
    if (!ValidateDrawBase(context, mode, 1))
    {
        return false;
    }

    const State &state = context->getGLState();

    // An INVALID_OPERATION error is generated if zero is bound to VERTEX_ARRAY_BINDING,
    // DRAW_INDIRECT_BUFFER or to any enabled vertex array.
    if (!state.getVertexArrayId())
    {
        context->handleError(Error(GL_INVALID_OPERATION, "zero is bound to VERTEX_ARRAY_BINDING"));
        return false;
    }

    gl::Buffer *drawIndirectBuffer = state.getDrawIndirectBuffer();
    if (!drawIndirectBuffer)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "zero is bound to DRAW_INDIRECT_BUFFER"));
        return false;
    }

    // An INVALID_VALUE error is generated if indirect is not a multiple of the size, in basic
    // machine units, of uint.
    GLint64 offset = reinterpret_cast<GLint64>(indirect);
    if ((static_cast<GLuint>(offset) % sizeof(GLuint)) != 0)
    {
        context->handleError(
            Error(GL_INVALID_VALUE,
                  "indirect is not a multiple of the size, in basic machine units, of uint"));
        return false;
    }

    return true;
}

bool ValidateDrawArraysIndirect(Context *context, GLenum mode, const GLvoid *indirect)
{
    const State &state                          = context->getGLState();
    gl::TransformFeedback *curTransformFeedback = state.getCurrentTransformFeedback();
    if (curTransformFeedback && curTransformFeedback->isActive() &&
        !curTransformFeedback->isPaused())
    {
        // An INVALID_OPERATION error is generated if transform feedback is active and not paused.
        context->handleError(
            Error(GL_INVALID_OPERATION, "transform feedback is active and not paused."));
        return false;
    }

    if (!ValidateDrawIndirectBase(context, mode, indirect))
        return false;

    gl::Buffer *drawIndirectBuffer = state.getDrawIndirectBuffer();
    CheckedNumeric<size_t> checkedOffset(reinterpret_cast<size_t>(indirect));
    // In OpenGL ES3.1 spec, session 10.5, it defines the struct of DrawArraysIndirectCommand
    // which's size is 4 * sizeof(uint).
    auto checkedSum = checkedOffset + 4 * sizeof(GLuint);
    if (!checkedSum.IsValid() ||
        checkedSum.ValueOrDie() > static_cast<size_t>(drawIndirectBuffer->getSize()))
    {
        context->handleError(
            Error(GL_INVALID_OPERATION,
                  "the  command  would source data beyond the end of the buffer object."));
        return false;
    }

    return true;
}

bool ValidateDrawElementsIndirect(Context *context,
                                  GLenum mode,
                                  GLenum type,
                                  const GLvoid *indirect)
{
    if (!ValidateDrawElementsBase(context, type))
        return false;

    const State &state             = context->getGLState();
    const VertexArray *vao         = state.getVertexArray();
    gl::Buffer *elementArrayBuffer = vao->getElementArrayBuffer().get();
    if (!elementArrayBuffer)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "zero is bound to ELEMENT_ARRAY_BUFFER"));
        return false;
    }

    if (!ValidateDrawIndirectBase(context, mode, indirect))
        return false;

    gl::Buffer *drawIndirectBuffer = state.getDrawIndirectBuffer();
    CheckedNumeric<size_t> checkedOffset(reinterpret_cast<size_t>(indirect));
    // In OpenGL ES3.1 spec, session 10.5, it defines the struct of DrawElementsIndirectCommand
    // which's size is 5 * sizeof(uint).
    auto checkedSum = checkedOffset + 5 * sizeof(GLuint);
    if (!checkedSum.IsValid() ||
        checkedSum.ValueOrDie() > static_cast<size_t>(drawIndirectBuffer->getSize()))
    {
        context->handleError(
            Error(GL_INVALID_OPERATION,
                  "the  command  would source data beyond the end of the buffer object."));
        return false;
    }

    return true;
}

bool ValidateGetTexLevelParameterBase(Context *context,
                                      GLenum target,
                                      GLint level,
                                      GLenum pname,
                                      GLsizei *length)
{
    if (context->getClientVersion() < ES_3_1)
    {
        context->handleError(Error(GL_INVALID_OPERATION, "Context does not support GLES3.1"));
        return false;
    }

    if (length)
    {
        *length = 0;
    }

    if (!ValidTexLevelDestinationTarget(context, target))
    {
        context->handleError(Error(GL_INVALID_ENUM, "Invalid texture target"));
        return false;
    }

    if (context->getTargetTexture(IsCubeMapTextureTarget(target) ? GL_TEXTURE_CUBE_MAP : target) ==
        nullptr)
    {
        context->handleError(Error(GL_INVALID_ENUM, "No texture bound."));
        return false;
    }

    if (!ValidMipLevel(context, target, level))
    {
        context->handleError(Error(GL_INVALID_VALUE));
        return false;
    }

    switch (pname)
    {
        case GL_TEXTURE_RED_TYPE:
        case GL_TEXTURE_GREEN_TYPE:
        case GL_TEXTURE_BLUE_TYPE:
        case GL_TEXTURE_ALPHA_TYPE:
        case GL_TEXTURE_DEPTH_TYPE:
            break;
        case GL_TEXTURE_RED_SIZE:
        case GL_TEXTURE_GREEN_SIZE:
        case GL_TEXTURE_BLUE_SIZE:
        case GL_TEXTURE_ALPHA_SIZE:
        case GL_TEXTURE_DEPTH_SIZE:
        case GL_TEXTURE_STENCIL_SIZE:
        case GL_TEXTURE_SHARED_SIZE:
            break;
        case GL_TEXTURE_INTERNAL_FORMAT:
        case GL_TEXTURE_WIDTH:
        case GL_TEXTURE_HEIGHT:
        case GL_TEXTURE_DEPTH:
            break;
        case GL_TEXTURE_SAMPLES:
        case GL_TEXTURE_FIXED_SAMPLE_LOCATIONS:
            break;
        case GL_TEXTURE_COMPRESSED:
            break;
        default:
            context->handleError(Error(GL_INVALID_ENUM, "Unknown pname."));
            return false;
    }

    if (length)
    {
        *length = 1;
    }
    return true;
}

bool ValidateGetTexLevelParameterfv(Context *context,
                                    GLenum target,
                                    GLint level,
                                    GLenum pname,
                                    GLfloat *params)
{
    return ValidateGetTexLevelParameterBase(context, target, level, pname, nullptr);
}

bool ValidateGetTexLevelParameteriv(Context *context,
                                    GLenum target,
                                    GLint level,
                                    GLenum pname,
                                    GLint *params)
{
    return ValidateGetTexLevelParameterBase(context, target, level, pname, nullptr);
}

}  // namespace gl

//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// queryutils.cpp: Utilities for querying values from GL objects

#include "libANGLE/queryutils.h"

#include "libANGLE/Buffer.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/Program.h"

namespace gl
{
void QueryFramebufferAttachmentParameteriv(const Framebuffer *framebuffer,
                                           GLenum attachment,
                                           GLenum pname,
                                           GLint *params)
{
    ASSERT(framebuffer);

    const FramebufferAttachment *attachmentObject = framebuffer->getAttachment(attachment);
    if (attachmentObject == nullptr)
    {
        // ES 2.0.25 spec pg 127 states that if the value of FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE
        // is NONE, then querying any other pname will generate INVALID_ENUM.

        // ES 3.0.2 spec pg 235 states that if the attachment type is none,
        // GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME will return zero and be an
        // INVALID_OPERATION for all other pnames

        switch (pname)
        {
            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
                *params = GL_NONE;
                break;

            case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
                *params = 0;
                break;

            default:
                UNREACHABLE();
                break;
        }

        return;
    }

    switch (pname)
    {
        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE:
            *params = attachmentObject->type();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME:
            *params = attachmentObject->id();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL:
            *params = attachmentObject->mipLevel();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE:
            *params = attachmentObject->cubeMapFace();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_RED_SIZE:
            *params = attachmentObject->getRedSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_GREEN_SIZE:
            *params = attachmentObject->getGreenSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_BLUE_SIZE:
            *params = attachmentObject->getBlueSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_ALPHA_SIZE:
            *params = attachmentObject->getAlphaSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_DEPTH_SIZE:
            *params = attachmentObject->getDepthSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_STENCIL_SIZE:
            *params = attachmentObject->getStencilSize();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_COMPONENT_TYPE:
            *params = attachmentObject->getComponentType();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING:
            *params = attachmentObject->getColorEncoding();
            break;

        case GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LAYER:
            *params = attachmentObject->layer();
            break;

        default:
            UNREACHABLE();
            break;
    }
}

void QueryBufferParameteriv(const Buffer *buffer, GLenum pname, GLint *params)
{
    ASSERT(buffer != nullptr);

    switch (pname)
    {
        case GL_BUFFER_USAGE:
            *params = static_cast<GLint>(buffer->getUsage());
            break;
        case GL_BUFFER_SIZE:
            *params = clampCast<GLint>(buffer->getSize());
            break;
        case GL_BUFFER_ACCESS_FLAGS:
            *params = buffer->getAccessFlags();
            break;
        case GL_BUFFER_ACCESS_OES:
            *params = buffer->getAccess();
            break;
        case GL_BUFFER_MAPPED:
            *params = static_cast<GLint>(buffer->isMapped());
            break;
        case GL_BUFFER_MAP_OFFSET:
            *params = clampCast<GLint>(buffer->getMapOffset());
            break;
        case GL_BUFFER_MAP_LENGTH:
            *params = clampCast<GLint>(buffer->getMapLength());
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void QueryProgramiv(const Program *program, GLenum pname, GLint *params)
{
    ASSERT(program != nullptr);

    switch (pname)
    {
        case GL_DELETE_STATUS:
            *params = program->isFlaggedForDeletion();
            return;
        case GL_LINK_STATUS:
            *params = program->isLinked();
            return;
        case GL_VALIDATE_STATUS:
            *params = program->isValidated();
            return;
        case GL_INFO_LOG_LENGTH:
            *params = program->getInfoLogLength();
            return;
        case GL_ATTACHED_SHADERS:
            *params = program->getAttachedShadersCount();
            return;
        case GL_ACTIVE_ATTRIBUTES:
            *params = program->getActiveAttributeCount();
            return;
        case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
            *params = program->getActiveAttributeMaxLength();
            return;
        case GL_ACTIVE_UNIFORMS:
            *params = program->getActiveUniformCount();
            return;
        case GL_ACTIVE_UNIFORM_MAX_LENGTH:
            *params = program->getActiveUniformMaxLength();
            return;
        case GL_PROGRAM_BINARY_LENGTH_OES:
            *params = program->getBinaryLength();
            return;
        case GL_ACTIVE_UNIFORM_BLOCKS:
            *params = program->getActiveUniformBlockCount();
            return;
        case GL_ACTIVE_UNIFORM_BLOCK_MAX_NAME_LENGTH:
            *params = program->getActiveUniformBlockMaxLength();
            break;
        case GL_TRANSFORM_FEEDBACK_BUFFER_MODE:
            *params = program->getTransformFeedbackBufferMode();
            break;
        case GL_TRANSFORM_FEEDBACK_VARYINGS:
            *params = program->getTransformFeedbackVaryingCount();
            break;
        case GL_TRANSFORM_FEEDBACK_VARYING_MAX_LENGTH:
            *params = program->getTransformFeedbackVaryingMaxLength();
            break;
        case GL_PROGRAM_BINARY_RETRIEVABLE_HINT:
            *params = program->getBinaryRetrievableHint();
            break;
        default:
            UNREACHABLE();
            break;
    }
}
}

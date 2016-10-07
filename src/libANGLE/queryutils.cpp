//
// Copyright (c) 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// queryutils.cpp: Utilities for querying values from GL objects

#include "libANGLE/queryutils.h"

#include "common/utilities.h"

#include "libANGLE/Buffer.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/Program.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/Sampler.h"
#include "libANGLE/Shader.h"
#include "libANGLE/Texture.h"

namespace gl
{

namespace
{
template <typename ParamType>
void QueryTexParameterBase(const Texture *texture, GLenum pname, ParamType *params)
{
    ASSERT(texture != nullptr);

    switch (pname)
    {
        case GL_TEXTURE_MAG_FILTER:
            *params = ConvertFromGLenum<ParamType>(texture->getMagFilter());
            break;
        case GL_TEXTURE_MIN_FILTER:
            *params = ConvertFromGLenum<ParamType>(texture->getMinFilter());
            break;
        case GL_TEXTURE_WRAP_S:
            *params = ConvertFromGLenum<ParamType>(texture->getWrapS());
            break;
        case GL_TEXTURE_WRAP_T:
            *params = ConvertFromGLenum<ParamType>(texture->getWrapT());
            break;
        case GL_TEXTURE_WRAP_R:
            *params = ConvertFromGLenum<ParamType>(texture->getWrapR());
            break;
        case GL_TEXTURE_IMMUTABLE_FORMAT:
            *params = ConvertFromGLboolean<ParamType>(texture->getImmutableFormat());
            break;
        case GL_TEXTURE_IMMUTABLE_LEVELS:
            *params = ConvertFromGLuint<ParamType>(texture->getImmutableLevels());
            break;
        case GL_TEXTURE_USAGE_ANGLE:
            *params = ConvertFromGLenum<ParamType>(texture->getUsage());
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            *params = ConvertFromGLfloat<ParamType>(texture->getMaxAnisotropy());
            break;
        case GL_TEXTURE_SWIZZLE_R:
            *params = ConvertFromGLenum<ParamType>(texture->getSwizzleRed());
            break;
        case GL_TEXTURE_SWIZZLE_G:
            *params = ConvertFromGLenum<ParamType>(texture->getSwizzleGreen());
            break;
        case GL_TEXTURE_SWIZZLE_B:
            *params = ConvertFromGLenum<ParamType>(texture->getSwizzleBlue());
            break;
        case GL_TEXTURE_SWIZZLE_A:
            *params = ConvertFromGLenum<ParamType>(texture->getSwizzleAlpha());
            break;
        case GL_TEXTURE_BASE_LEVEL:
            *params = ConvertFromGLuint<ParamType>(texture->getBaseLevel());
            break;
        case GL_TEXTURE_MAX_LEVEL:
            *params = ConvertFromGLuint<ParamType>(texture->getMaxLevel());
            break;
        case GL_TEXTURE_MIN_LOD:
            *params = ConvertFromGLfloat<ParamType>(texture->getSamplerState().minLod);
            break;
        case GL_TEXTURE_MAX_LOD:
            *params = ConvertFromGLfloat<ParamType>(texture->getSamplerState().maxLod);
            break;
        case GL_TEXTURE_COMPARE_MODE:
            *params = ConvertFromGLenum<ParamType>(texture->getCompareMode());
            break;
        case GL_TEXTURE_COMPARE_FUNC:
            *params = ConvertFromGLenum<ParamType>(texture->getCompareFunc());
            break;
        default:
            UNREACHABLE();
            break;
    }
}

template <typename ParamType>
void SetTexParameterBase(Texture *texture, GLenum pname, const ParamType *params)
{
    ASSERT(texture != nullptr);

    switch (pname)
    {
        case GL_TEXTURE_WRAP_S:
            texture->setWrapS(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_WRAP_T:
            texture->setWrapT(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_WRAP_R:
            texture->setWrapR(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MIN_FILTER:
            texture->setMinFilter(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MAG_FILTER:
            texture->setMagFilter(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_USAGE_ANGLE:
            texture->setUsage(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            texture->setMaxAnisotropy(ConvertToGLfloat(params[0]));
            break;
        case GL_TEXTURE_COMPARE_MODE:
            texture->setCompareMode(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_COMPARE_FUNC:
            texture->setCompareFunc(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_SWIZZLE_R:
            texture->setSwizzleRed(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_SWIZZLE_G:
            texture->setSwizzleGreen(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_SWIZZLE_B:
            texture->setSwizzleBlue(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_SWIZZLE_A:
            texture->setSwizzleAlpha(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_BASE_LEVEL:
            texture->setBaseLevel(ConvertToGLuint(params[0]));
            break;
        case GL_TEXTURE_MAX_LEVEL:
            texture->setMaxLevel(ConvertToGLuint(params[0]));
            break;
        case GL_TEXTURE_MIN_LOD:
            texture->setMinLod(ConvertToGLfloat(params[0]));
            break;
        case GL_TEXTURE_MAX_LOD:
            texture->setMaxLod(ConvertToGLfloat(params[0]));
            break;
        default:
            UNREACHABLE();
            break;
    }
}

template <typename ParamType>
void QuerySamplerParameterBase(const Sampler *sampler, GLenum pname, ParamType *params)
{
    switch (pname)
    {
        case GL_TEXTURE_MIN_FILTER:
            *params = ConvertFromGLenum<ParamType>(sampler->getMinFilter());
            break;
        case GL_TEXTURE_MAG_FILTER:
            *params = ConvertFromGLenum<ParamType>(sampler->getMagFilter());
            break;
        case GL_TEXTURE_WRAP_S:
            *params = ConvertFromGLenum<ParamType>(sampler->getWrapS());
            break;
        case GL_TEXTURE_WRAP_T:
            *params = ConvertFromGLenum<ParamType>(sampler->getWrapT());
            break;
        case GL_TEXTURE_WRAP_R:
            *params = ConvertFromGLenum<ParamType>(sampler->getWrapR());
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            *params = ConvertFromGLfloat<ParamType>(sampler->getMaxAnisotropy());
            break;
        case GL_TEXTURE_MIN_LOD:
            *params = ConvertFromGLfloat<ParamType>(sampler->getMinLod());
            break;
        case GL_TEXTURE_MAX_LOD:
            *params = ConvertFromGLfloat<ParamType>(sampler->getMaxLod());
            break;
        case GL_TEXTURE_COMPARE_MODE:
            *params = ConvertFromGLenum<ParamType>(sampler->getCompareMode());
            break;
        case GL_TEXTURE_COMPARE_FUNC:
            *params = ConvertFromGLenum<ParamType>(sampler->getCompareFunc());
            break;
        default:
            UNREACHABLE();
            break;
    }
}

template <typename ParamType>
void SetSamplerParameterBase(Sampler *sampler, GLenum pname, const ParamType *params)
{
    switch (pname)
    {
        case GL_TEXTURE_WRAP_S:
            sampler->setWrapS(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_WRAP_T:
            sampler->setWrapT(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_WRAP_R:
            sampler->setWrapR(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MIN_FILTER:
            sampler->setMinFilter(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MAG_FILTER:
            sampler->setMagFilter(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MAX_ANISOTROPY_EXT:
            sampler->setMaxAnisotropy(ConvertToGLfloat(params[0]));
            break;
        case GL_TEXTURE_COMPARE_MODE:
            sampler->setCompareMode(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_COMPARE_FUNC:
            sampler->setCompareFunc(ConvertToGLenum(params[0]));
            break;
        case GL_TEXTURE_MIN_LOD:
            sampler->setMinLod(ConvertToGLfloat(params[0]));
            break;
        case GL_TEXTURE_MAX_LOD:
            sampler->setMaxLod(ConvertToGLfloat(params[0]));
            break;
        default:
            UNREACHABLE();
            break;
    }
}

}  // anonymous namespace

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

void QueryRenderbufferiv(const Renderbuffer *renderbuffer, GLenum pname, GLint *params)
{
    ASSERT(renderbuffer != nullptr);

    switch (pname)
    {
        case GL_RENDERBUFFER_WIDTH:
            *params = renderbuffer->getWidth();
            break;
        case GL_RENDERBUFFER_HEIGHT:
            *params = renderbuffer->getHeight();
            break;
        case GL_RENDERBUFFER_INTERNAL_FORMAT:
            *params = renderbuffer->getFormat().info->internalFormat;
            break;
        case GL_RENDERBUFFER_RED_SIZE:
            *params = renderbuffer->getRedSize();
            break;
        case GL_RENDERBUFFER_GREEN_SIZE:
            *params = renderbuffer->getGreenSize();
            break;
        case GL_RENDERBUFFER_BLUE_SIZE:
            *params = renderbuffer->getBlueSize();
            break;
        case GL_RENDERBUFFER_ALPHA_SIZE:
            *params = renderbuffer->getAlphaSize();
            break;
        case GL_RENDERBUFFER_DEPTH_SIZE:
            *params = renderbuffer->getDepthSize();
            break;
        case GL_RENDERBUFFER_STENCIL_SIZE:
            *params = renderbuffer->getStencilSize();
            break;
        case GL_RENDERBUFFER_SAMPLES_ANGLE:
            *params = renderbuffer->getSamples();
            break;
        default:
            UNREACHABLE();
            break;
    }
}

void QueryShaderiv(const Shader *shader, GLenum pname, GLint *params)
{
    ASSERT(shader != nullptr);

    switch (pname)
    {
        case GL_SHADER_TYPE:
            *params = shader->getType();
            return;
        case GL_DELETE_STATUS:
            *params = shader->isFlaggedForDeletion();
            return;
        case GL_COMPILE_STATUS:
            *params = shader->isCompiled() ? GL_TRUE : GL_FALSE;
            return;
        case GL_INFO_LOG_LENGTH:
            *params = shader->getInfoLogLength();
            return;
        case GL_SHADER_SOURCE_LENGTH:
            *params = shader->getSourceLength();
            return;
        case GL_TRANSLATED_SHADER_SOURCE_LENGTH_ANGLE:
            *params = shader->getTranslatedSourceWithDebugInfoLength();
            return;
        default:
            UNREACHABLE();
            break;
    }
}

void QueryTexParameterfv(const Texture *texture, GLenum pname, GLfloat *params)
{
    QueryTexParameterBase(texture, pname, params);
}

void QueryTexParameteriv(const Texture *texture, GLenum pname, GLint *params)
{
    QueryTexParameterBase(texture, pname, params);
}

void QuerySamplerParameterfv(const Sampler *sampler, GLenum pname, GLfloat *params)
{
    QuerySamplerParameterBase(sampler, pname, params);
}

void QuerySamplerParameteriv(const Sampler *sampler, GLenum pname, GLint *params)
{
    QuerySamplerParameterBase(sampler, pname, params);
}

void SetTexParameterf(Texture *texture, GLenum pname, GLfloat param)
{
    SetTexParameterBase(texture, pname, &param);
}

void SetTexParameterfv(Texture *texture, GLenum pname, const GLfloat *params)
{
    SetTexParameterBase(texture, pname, params);
}

void SetTexParameteri(Texture *texture, GLenum pname, GLint param)
{
    SetTexParameterBase(texture, pname, &param);
}

void SetTexParameteriv(Texture *texture, GLenum pname, const GLint *params)
{
    SetTexParameterBase(texture, pname, params);
}

void SetSamplerParameterf(Sampler *sampler, GLenum pname, GLfloat param)
{
    SetSamplerParameterBase(sampler, pname, &param);
}

void SetSamplerParameterfv(Sampler *sampler, GLenum pname, const GLfloat *params)
{
    SetSamplerParameterBase(sampler, pname, params);
}

void SetSamplerParameteri(Sampler *sampler, GLenum pname, GLint param)
{
    SetSamplerParameterBase(sampler, pname, &param);
}

void SetSamplerParameteriv(Sampler *sampler, GLenum pname, const GLint *params)
{
    SetSamplerParameterBase(sampler, pname, params);
}
}

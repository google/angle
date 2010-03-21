//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// libGLESv2.cpp: Implements the exported OpenGL ES 2.0 functions.

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <exception>
#include <limits>

#include "Context.h"
#include "main.h"
#include "Program.h"
#include "Shader.h"
#include "Buffer.h"
#include "Texture.h"
#include "Renderbuffer.h"
#include "Framebuffer.h"
#include "mathutil.h"
#include "debug.h"
#include "utilities.h"

extern "C"
{

void __stdcall glActiveTexture(GLenum texture)
{
    TRACE("GLenum texture = 0x%X", texture);

    try
    {
        if (texture < GL_TEXTURE0 || texture > GL_TEXTURE0 + gl::MAX_TEXTURE_IMAGE_UNITS - 1)
        {
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->activeSampler = texture - GL_TEXTURE0;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glAttachShader(GLuint program, GLuint shader)
{
    TRACE("GLuint program = %d, GLuint shader = %d", program, shader);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);
            gl::Shader *shaderObject = context->getShader(shader);

            if (!programObject || !shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            if (!programObject->attachShader(shaderObject))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBindAttribLocation(GLuint program, GLuint index, const char* name)
{
    TRACE("GLuint program = %d, GLuint index = %d, const char* name = 0x%0.8p", program, index, name);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (!programObject)
            {
                return error(GL_INVALID_VALUE);
            }

            programObject->bindAttributeLocation(index, name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBindBuffer(GLenum target, GLuint buffer)
{
    TRACE("GLenum target = 0x%X, GLuint buffer = %d", target, buffer);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (target)
            {
              case GL_ARRAY_BUFFER:
                context->bindArrayBuffer(buffer);
                return;
              case GL_ELEMENT_ARRAY_BUFFER:
                context->bindElementArrayBuffer(buffer);
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBindFramebuffer(GLenum target, GLuint framebuffer)
{
    TRACE("GLenum target = 0x%X, GLuint framebuffer = %d", target, framebuffer);

    try
    {
        if (target != GL_FRAMEBUFFER)
        {
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->bindFramebuffer(framebuffer);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBindRenderbuffer(GLenum target, GLuint renderbuffer)
{
    TRACE("GLenum target = 0x%X, GLuint renderbuffer = %d", target, renderbuffer);

    try
    {
        if (target != GL_RENDERBUFFER)
        {
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->bindRenderbuffer(renderbuffer);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBindTexture(GLenum target, GLuint texture)
{
    TRACE("GLenum target = 0x%X, GLuint texture = %d", target, texture);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Texture *textureObject = context->getTexture(texture);

            if (textureObject && textureObject->getTarget() != target && texture != 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch (target)
            {
              case GL_TEXTURE_2D:
                context->bindTexture2D(texture);
                return;
              case GL_TEXTURE_CUBE_MAP:
                context->bindTextureCubeMap(texture);
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    TRACE("GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f", red, green, blue, alpha);

    try
    {
        gl::Context* context = gl::getContext();

        if (context)
        {
            context->blendColor.red = gl::clamp01(red);
            context->blendColor.blue = gl::clamp01(blue);
            context->blendColor.green = gl::clamp01(green);
            context->blendColor.alpha = gl::clamp01(alpha);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBlendEquation(GLenum mode)
{
    glBlendEquationSeparate(mode, mode);
}

void __stdcall glBlendEquationSeparate(GLenum modeRGB, GLenum modeAlpha)
{
    TRACE("GLenum modeRGB = 0x%X, GLenum modeAlpha = 0x%X", modeRGB, modeAlpha);

    try
    {
        switch (modeRGB)
        {
          case GL_FUNC_ADD:
          case GL_FUNC_SUBTRACT:
          case GL_FUNC_REVERSE_SUBTRACT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (modeAlpha)
        {
          case GL_FUNC_ADD:
          case GL_FUNC_SUBTRACT:
          case GL_FUNC_REVERSE_SUBTRACT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->blendEquationRGB = modeRGB;
            context->blendEquationAlpha = modeAlpha;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    glBlendFuncSeparate(sfactor, dfactor, sfactor, dfactor);
}

void __stdcall glBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
{
    TRACE("GLenum srcRGB = 0x%X, GLenum dstRGB = 0x%X, GLenum srcAlpha = 0x%X, GLenum dstAlpha = 0x%X", srcRGB, dstRGB, srcAlpha, dstAlpha);

    try
    {
        switch (srcRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (dstRGB)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (srcAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
          case GL_SRC_ALPHA_SATURATE:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (dstAlpha)
        {
          case GL_ZERO:
          case GL_ONE:
          case GL_SRC_COLOR:
          case GL_ONE_MINUS_SRC_COLOR:
          case GL_DST_COLOR:
          case GL_ONE_MINUS_DST_COLOR:
          case GL_SRC_ALPHA:
          case GL_ONE_MINUS_SRC_ALPHA:
          case GL_DST_ALPHA:
          case GL_ONE_MINUS_DST_ALPHA:
          case GL_CONSTANT_COLOR:
          case GL_ONE_MINUS_CONSTANT_COLOR:
          case GL_CONSTANT_ALPHA:
          case GL_ONE_MINUS_CONSTANT_ALPHA:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        bool constantColorUsed = (srcRGB == GL_CONSTANT_COLOR || srcRGB == GL_ONE_MINUS_CONSTANT_COLOR ||
                                  dstRGB == GL_CONSTANT_COLOR || dstRGB == GL_ONE_MINUS_CONSTANT_COLOR);

        bool constantAlphaUsed = (srcRGB == GL_CONSTANT_ALPHA || srcRGB == GL_ONE_MINUS_CONSTANT_ALPHA ||
                                  dstRGB == GL_CONSTANT_ALPHA || dstRGB == GL_ONE_MINUS_CONSTANT_ALPHA);

        if (constantColorUsed && constantAlphaUsed)
        {
            ERR("Simultaneous use of GL_CONSTANT_ALPHA/GL_ONE_MINUS_CONSTANT_ALPHA and GL_CONSTANT_COLOR/GL_ONE_MINUS_CONSTANT_COLOR invalid under WebGL");
            return error(GL_INVALID_OPERATION);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->sourceBlendRGB = srcRGB;
            context->sourceBlendAlpha = srcAlpha;
            context->destBlendRGB = dstRGB;
            context->destBlendAlpha = dstAlpha;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBufferData(GLenum target, GLsizeiptr size, const void* data, GLenum usage)
{
    TRACE("GLenum target = 0x%X, GLsizeiptr size = %d, const void* data = 0x%0.8p, GLenum usage = %d", target, size, data, usage);

    try
    {
        if (size < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch (usage)
        {
          case GL_STREAM_DRAW:
          case GL_STATIC_DRAW:
          case GL_DYNAMIC_DRAW:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Buffer *buffer;

            switch (target)
            {
              case GL_ARRAY_BUFFER:
                buffer = context->getArrayBuffer();
                break;
              case GL_ELEMENT_ARRAY_BUFFER:
                buffer = context->getElementArrayBuffer();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if (!buffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            buffer->bufferData(data, size, usage);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glBufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void* data)
{
    TRACE("GLenum target = 0x%X, GLintptr offset = %d, GLsizeiptr size = %d, const void* data = 0x%0.8p", target, offset, size, data);

    try
    {
        if (size < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Buffer *buffer;

            switch (target)
            {
              case GL_ARRAY_BUFFER:
                buffer = context->getArrayBuffer();
                break;
              case GL_ELEMENT_ARRAY_BUFFER:
                buffer = context->getElementArrayBuffer();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            if (!buffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            GLenum err = buffer->bufferSubData(data, size, offset);

            if (err != GL_NO_ERROR)
            {
                return error(err);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLenum __stdcall glCheckFramebufferStatus(GLenum target)
{
    TRACE("GLenum target = 0x%X", target);

    try
    {
        if (target != GL_FRAMEBUFFER)
        {
            return error(GL_INVALID_ENUM, 0);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Framebuffer *framebuffer = context->getFramebuffer();

            return framebuffer->completeness();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

void __stdcall glClear(GLbitfield mask)
{
    TRACE("GLbitfield mask = %X", mask);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->clear(mask);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    TRACE("GLclampf red = %f, GLclampf green = %f, GLclampf blue = %f, GLclampf alpha = %f", red, green, blue, alpha);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->setClearColor(red, green, blue, alpha);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glClearDepthf(GLclampf depth)
{
    TRACE("GLclampf depth = %f", depth);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->setClearDepth(depth);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glClearStencil(GLint s)
{
    TRACE("GLint s = %d", s);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->setClearStencil(s);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    TRACE("GLboolean red = %d, GLboolean green = %d, GLboolean blue = %d, GLboolean alpha = %d", red, green, blue, alpha);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->colorMaskRed = red != GL_FALSE;
            context->colorMaskGreen = green != GL_FALSE;
            context->colorMaskBlue = blue != GL_FALSE;
            context->colorMaskAlpha = alpha != GL_FALSE;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glCompileShader(GLuint shader)
{
    TRACE("GLuint shader = %d", shader);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Shader *shaderObject = context->getShader(shader);

            if (!shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            shaderObject->compile();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void* data)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d, GLint border = %d, GLsizei imageSize = %d, const void* data = 0x%0.8p",
          target, level, internalformat, width, height, border, imageSize, data);

    try
    {
        if (target != GL_TEXTURE_2D && !es2dx::IsCubemapTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if (level < 0 || level > gl::MAX_TEXTURE_LEVELS)
        {
            return error(GL_INVALID_VALUE);
        }

        if (width < 0 || height < 0 || (level > 0 && !gl::isPow2(width)) || (level > 0 && !gl::isPow2(height)) || border != 0 || imageSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        return error(GL_INVALID_ENUM); // ultimately we don't support compressed textures
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void* data)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLsizei imageSize = %d, const void* data = 0x%0.8p",
          target, level, xoffset, yoffset, width, height, format, imageSize, data);

    try
    {
        if (target != GL_TEXTURE_2D && !es2dx::IsCubemapTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if (level < 0 || level > gl::MAX_TEXTURE_LEVELS)
        {
            return error(GL_INVALID_VALUE);
        }

        if (xoffset < 0 || yoffset < 0 || width < 0 || height < 0 || (level > 0 && !gl::isPow2(width)) || (level > 0 && !gl::isPow2(height)) || imageSize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if (xoffset != 0 || yoffset != 0)
        {
            return error(GL_INVALID_OPERATION);
        }

        return error(GL_INVALID_OPERATION); // The texture being operated on is not a compressed texture.
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, GLint border = %d",
          target, level, internalformat, x, y, width, height, border);

    try
    {
        if (width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d",
          target, level, xoffset, yoffset, x, y, width, height);

    try
    {
        if (width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLuint __stdcall glCreateProgram(void)
{
    TRACE("");

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            return context->createProgram();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

GLuint __stdcall glCreateShader(GLenum type)
{
    TRACE("GLenum type = 0x%X", type);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (type)
            {
              case GL_FRAGMENT_SHADER:
              case GL_VERTEX_SHADER:
                return context->createShader(type);
              default:
                return error(GL_INVALID_ENUM, 0);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, 0);
    }

    return 0;
}

void __stdcall glCullFace(GLenum mode)
{
    TRACE("GLenum mode = 0x%X", mode);

    try
    {
        switch (mode)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            {
                gl::Context *context = gl::getContext();

                if (context)
                {
                    context->cullMode = mode;
                }
            }
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteBuffers(GLsizei n, const GLuint* buffers)
{
    TRACE("GLsizei n = %d, const GLuint* buffers = 0x%0.8p", n, buffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                context->deleteBuffer(buffers[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteFramebuffers(GLsizei n, const GLuint* framebuffers)
{
    TRACE("GLsizei n = %d, const GLuint* framebuffers = 0x%0.8p", n, framebuffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                if (framebuffers[i] != 0)
                {
                    context->deleteFramebuffer(framebuffers[i]);
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteProgram(GLuint program)
{
    TRACE("GLuint program = %d", program);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->deleteProgram(program);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteRenderbuffers(GLsizei n, const GLuint* renderbuffers)
{
    TRACE("GLsizei n = %d, const GLuint* renderbuffers = 0x%0.8p", n, renderbuffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                context->deleteRenderbuffer(renderbuffers[i]);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteShader(GLuint shader)
{
    TRACE("GLuint shader = %d", shader);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->deleteShader(shader);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDeleteTextures(GLsizei n, const GLuint* textures)
{
    TRACE("GLsizei n = %d, const GLuint* textures = 0x%0.8p", n, textures);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                if (textures[i] != 0)
                {
                    context->deleteTexture(textures[i]);
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDepthFunc(GLenum func)
{
    TRACE("GLenum func = 0x%X", func);

    try
    {
        switch (func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GREATER:
          case GL_GEQUAL:
          case GL_NOTEQUAL:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->depthFunc = func;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDepthMask(GLboolean flag)
{
    TRACE("GLboolean flag = %d", flag);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->depthMask = flag != GL_FALSE;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDepthRangef(GLclampf zNear, GLclampf zFar)
{
    TRACE("GLclampf zNear = %f, GLclampf zFar = %f", zNear, zFar);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->zNear = zNear;
            context->zFar = zFar;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDetachShader(GLuint program, GLuint shader)
{
    TRACE("GLuint program = %d, GLuint shader = %d", program, shader);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);
            gl::Shader *shaderObject = context->getShader(shader);

            if (!programObject || !shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            if (!programObject->detachShader(shaderObject))
            {
                return error(GL_INVALID_OPERATION);
            }

            if (shaderObject->isDeletable())
            {
                context->deleteShader(shader);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDisable(GLenum cap)
{
    TRACE("GLenum cap = 0x%X", cap);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (cap)
            {
              case GL_CULL_FACE:                context->cullFace = false;              break;
              case GL_POLYGON_OFFSET_FILL:      context->polygonOffsetFill = false;     break;
              case GL_SAMPLE_ALPHA_TO_COVERAGE: context->sampleAlphaToCoverage = false; break;
              case GL_SAMPLE_COVERAGE:          context->sampleCoverage = false;        break;
              case GL_SCISSOR_TEST:             context->scissorTest = false;           break;
              case GL_STENCIL_TEST:             context->stencilTest = false;           break;
              case GL_DEPTH_TEST:               context->depthTest = false;             break;
              case GL_BLEND:                    context->blend = false;                 break;
              case GL_DITHER:                   context->dither = false;                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDisableVertexAttribArray(GLuint index)
{
    TRACE("GLuint index = %d", index);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->vertexAttribute[index].mEnabled = false;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    TRACE("GLenum mode = 0x%X, GLint first = %d, GLsizei count = %d", mode, first, count);

    try
    {
        if (count < 0 || first < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->drawArrays(mode, first, count);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glDrawElements(GLenum mode, GLsizei count, GLenum type, const void* indices)
{
    TRACE("GLenum mode = 0x%X, GLsizei count = %d, GLenum type = 0x%X, const void* indices = 0x%0.8p", mode, count, type, indices);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            UNIMPLEMENTED();   // FIXME
          case GL_UNSIGNED_SHORT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->drawElements(mode, count, type, indices);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glEnable(GLenum cap)
{
    TRACE("GLenum cap = 0x%X", cap);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (cap)
            {
              case GL_CULL_FACE:                context->cullFace = true;              break;
              case GL_POLYGON_OFFSET_FILL:      context->polygonOffsetFill = true;     break;
              case GL_SAMPLE_ALPHA_TO_COVERAGE: context->sampleAlphaToCoverage = true; break;
              case GL_SAMPLE_COVERAGE:          context->sampleCoverage = true;        break;
              case GL_SCISSOR_TEST:             context->scissorTest = true;           break;
              case GL_STENCIL_TEST:             context->stencilTest = true;           break;
              case GL_DEPTH_TEST:               context->depthTest = true;             break;
              case GL_BLEND:                    context->blend = true;                 break;
              case GL_DITHER:                   context->dither = true;                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glEnableVertexAttribArray(GLuint index)
{
    TRACE("GLuint index = %d", index);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->vertexAttribute[index].mEnabled = true;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glFinish(void)
{
    TRACE("");

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->finish();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glFlush(void)
{
    TRACE("");

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            context->flush();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glFramebufferRenderbuffer(GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer)
{
    TRACE("GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum renderbuffertarget = 0x%X, GLuint renderbuffer = %d", target, attachment, renderbuffertarget, renderbuffer);

    try
    {
        if (target != GL_FRAMEBUFFER || renderbuffertarget != GL_RENDERBUFFER)
        {
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Framebuffer *framebuffer = context->getFramebuffer();

            if (context->framebuffer == 0 || !framebuffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch (attachment)
            {
              case GL_COLOR_ATTACHMENT0:
                framebuffer->setColorbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              case GL_DEPTH_ATTACHMENT:
                framebuffer->setDepthbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              case GL_STENCIL_ATTACHMENT:
                framebuffer->setStencilbuffer(GL_RENDERBUFFER, renderbuffer);
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
{
    TRACE("GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum textarget = 0x%X, GLuint texture = %d, GLint level = %d", target, attachment, textarget, texture, level);

    try
    {
        if (target != GL_FRAMEBUFFER)
        {
            return error(GL_INVALID_ENUM);
        }

        switch (attachment)
        {
          case GL_COLOR_ATTACHMENT0:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (texture)
            {
                switch (textarget)
                {
                  case GL_TEXTURE_2D:
                    if (!context->getTexture2D())
                    {
                        return error(GL_INVALID_OPERATION);
                    }
                    break;
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    UNIMPLEMENTED();   // FIXME
                    break;
                  default:
                    return error(GL_INVALID_ENUM);
                }

                if (level != 0)
                {
                    return error(GL_INVALID_VALUE);
                }
            }

            gl::Framebuffer *framebuffer = context->getFramebuffer();

            if (context->framebuffer == 0 || !framebuffer)
            {
                return error(GL_INVALID_OPERATION);
            }

            framebuffer->setColorbuffer(GL_TEXTURE, texture);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glFrontFace(GLenum mode)
{
    TRACE("GLenum mode = 0x%X", mode);

    try
    {
        switch (mode)
        {
          case GL_CW:
          case GL_CCW:
            {
                gl::Context *context = gl::getContext();

                if (context)
                {
                    context->frontFace = mode;
                }
            }
            break;
          default:
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGenBuffers(GLsizei n, GLuint* buffers)
{
    TRACE("GLsizei n = %d, GLuint* buffers = 0x%0.8p", n, buffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                buffers[i] = context->createBuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGenerateMipmap(GLenum target)
{
    TRACE("GLenum target = 0x%X", target);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGenFramebuffers(GLsizei n, GLuint* framebuffers)
{
    TRACE("GLsizei n = %d, GLuint* framebuffers = 0x%0.8p", n, framebuffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                framebuffers[i] = context->createFramebuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGenRenderbuffers(GLsizei n, GLuint* renderbuffers)
{
    TRACE("GLsizei n = %d, GLuint* renderbuffers = 0x%0.8p", n, renderbuffers);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                renderbuffers[i] = context->createRenderbuffer();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGenTextures(GLsizei n, GLuint* textures)
{
    TRACE("GLsizei n = %d, GLuint* textures =  0x%0.8p", n, textures);

    try
    {
        if (n < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            for (int i = 0; i < n; i++)
            {
                textures[i] = context->createTexture();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetActiveAttrib(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    TRACE("GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLint* size = 0x%0.8p, GLenum* type = %0.8p, char* name = %0.8p",
          program, index, bufsize, length, size, type, name);

    try
    {
        if (bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetActiveUniform(GLuint program, GLuint index, GLsizei bufsize, GLsizei* length, GLint* size, GLenum* type, char* name)
{
    TRACE("GLuint program = %d, GLuint index = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, GLint* size = 0x%0.8p, GLenum* type = 0x%0.8p, char* name = 0x%0.8p",
          program, index, bufsize, length, size, type, name);

    try
    {
        if (bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetAttachedShaders(GLuint program, GLsizei maxcount, GLsizei* count, GLuint* shaders)
{
    TRACE("GLuint program = %d, GLsizei maxcount = %d, GLsizei* count = 0x%0.8p, GLuint* shaders = 0x%0.8p", program, maxcount, count, shaders);

    try
    {
        if (maxcount < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

int __stdcall glGetAttribLocation(GLuint program, const char* name)
{
    TRACE("GLuint program = %d, const char* name = %s", program, name);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (!programObject)
            {
                return error(GL_INVALID_VALUE, -1);
            }

            return programObject->getAttributeLocation(name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, -1);
    }

    return -1;
}

void __stdcall glGetBooleanv(GLenum pname, GLboolean* params)
{
    TRACE("GLenum pname = 0x%X, GLboolean* params = 0x%0.8p",  pname, params);

    try
    {
        switch (pname)
        {
          case GL_SHADER_COMPILER: *params = GL_TRUE; break;
          default:
            UNIMPLEMENTED();   // FIXME
            return error(GL_INVALID_ENUM);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetBufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p", target, pname, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLenum __stdcall glGetError(void)
{
    TRACE("");

    gl::Context *context = gl::getContext();

    if (context)
    {
        return context->getError();
    }

    return GL_NO_ERROR;
}

void __stdcall glGetFloatv(GLenum pname, GLfloat* params)
{
    TRACE("GLenum pname = 0x%X, GLfloat* params = 0x%0.8p", pname, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetFramebufferAttachmentParameteriv(GLenum target, GLenum attachment, GLenum pname, GLint* params)
{
    TRACE("GLenum target = 0x%X, GLenum attachment = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p", target, attachment, pname, params);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            if (context->framebuffer == 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            UNIMPLEMENTED();   // FIXME
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetIntegerv(GLenum pname, GLint* params)
{
    TRACE("GLenum pname = 0x%X, GLint* params = 0x%0.8p", pname, params);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (pname)
            {
              case GL_MAX_VERTEX_ATTRIBS:               *params = gl::MAX_VERTEX_ATTRIBS;               break;
              case GL_MAX_VERTEX_UNIFORM_VECTORS:       *params = gl::MAX_VERTEX_UNIFORM_VECTORS;       break;
              case GL_MAX_VARYING_VECTORS:              *params = gl::MAX_VARYING_VECTORS;              break;
              case GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS: *params = gl::MAX_COMBINED_TEXTURE_IMAGE_UNITS; break;
              case GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS:   *params = gl::MAX_VERTEX_TEXTURE_IMAGE_UNITS;   break;
              case GL_MAX_TEXTURE_IMAGE_UNITS:          *params = gl::MAX_TEXTURE_IMAGE_UNITS;          break;
              case GL_MAX_FRAGMENT_UNIFORM_VECTORS:     *params = gl::MAX_FRAGMENT_UNIFORM_VECTORS;     break;
              case GL_MAX_RENDERBUFFER_SIZE:            *params = gl::MAX_RENDERBUFFER_SIZE;            break;
              case GL_NUM_SHADER_BINARY_FORMATS:        *params = 0;                                break;
              case GL_NUM_COMPRESSED_TEXTURE_FORMATS:   *params = 0;                                break;
              case GL_COMPRESSED_TEXTURE_FORMATS: /* no compressed texture formats are supported */ break;
              case GL_ARRAY_BUFFER_BINDING:             *params = context->arrayBuffer;             break;
              case GL_FRAMEBUFFER_BINDING:              *params = context->framebuffer;             break;
              case GL_RENDERBUFFER_BINDING:             *params = context->renderbuffer;            break;
              case GL_CURRENT_PROGRAM:                  *params = context->currentProgram;          break;
              case GL_PACK_ALIGNMENT:                   *params = context->packAlignment;           break;
              case GL_UNPACK_ALIGNMENT:                 *params = context->unpackAlignment;         break;
              case GL_GENERATE_MIPMAP_HINT:             *params = context->generateMipmapHint;      break;
              case GL_RED_BITS:
              case GL_GREEN_BITS:
              case GL_BLUE_BITS:
              case GL_ALPHA_BITS:
                {
                    gl::Framebuffer *framebuffer = context->getFramebuffer();
                    gl::Colorbuffer *colorbuffer = framebuffer->getColorbuffer();

                    if (colorbuffer)
                    {
                        switch (pname)
                        {
                          case GL_RED_BITS:   *params = colorbuffer->getRedSize();   break;
                          case GL_GREEN_BITS: *params = colorbuffer->getGreenSize(); break;
                          case GL_BLUE_BITS:  *params = colorbuffer->getBlueSize();  break;
                          case GL_ALPHA_BITS: *params = colorbuffer->getAlphaSize(); break;
                        }
                    }
                    else
                    {
                        *params = 0;
                    }
                }
                break;
              case GL_DEPTH_BITS:
                {
                    gl::Framebuffer *framebuffer = context->getFramebuffer();
                    gl::Depthbuffer *depthbuffer = framebuffer->getDepthbuffer();

                    if (depthbuffer)
                    {
                        *params = depthbuffer->getDepthSize();
                    }
                    else
                    {
                        *params = 0;
                    }
                }
                break;
              case GL_STENCIL_BITS:
                {
                    gl::Framebuffer *framebuffer = context->getFramebuffer();
                    gl::Stencilbuffer *stencilbuffer = framebuffer->getStencilbuffer();

                    if (stencilbuffer)
                    {
                        *params = stencilbuffer->getStencilSize();
                    }
                    else
                    {
                        *params = 0;
                    }
                }
                break;
              default:
                UNIMPLEMENTED();   // FIXME
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetProgramiv(GLuint program, GLenum pname, GLint* params)
{
    TRACE("GLuint program = %d, GLenum pname = %d, GLint* params = 0x%0.8p", program, pname, params);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (!programObject)
            {
                return error(GL_INVALID_VALUE);
            }

            switch (pname)
            {
              case GL_DELETE_STATUS:
                UNIMPLEMENTED();   // FIXME
                *params = GL_FALSE;
                return;
              case GL_LINK_STATUS:
                *params = programObject->isLinked();
                return;
              case GL_VALIDATE_STATUS:
                UNIMPLEMENTED();   // FIXME
                *params = GL_TRUE;
                return;
              case GL_INFO_LOG_LENGTH:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              case GL_ATTACHED_SHADERS:
                UNIMPLEMENTED();   // FIXME
                *params = 2;
                return;
              case GL_ACTIVE_ATTRIBUTES:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              case GL_ACTIVE_ATTRIBUTE_MAX_LENGTH:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              case GL_ACTIVE_UNIFORMS:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              case GL_ACTIVE_UNIFORM_MAX_LENGTH:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetProgramInfoLog(GLuint program, GLsizei bufsize, GLsizei* length, char* infolog)
{
    TRACE("GLuint program = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, char* infolog = 0x%0.8p", program, bufsize, length, infolog);

    try
    {
        if (bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetRenderbufferParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p", target, pname, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetShaderiv(GLuint shader, GLenum pname, GLint* params)
{
    TRACE("GLuint shader = %d, GLenum pname = %d, GLint* params = 0x%0.8p", shader, pname, params);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Shader *shaderObject = context->getShader(shader);

            if (!shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            switch (pname)
            {
              case GL_SHADER_TYPE:
                *params = shaderObject->getType();
                return;
              case GL_DELETE_STATUS:
                UNIMPLEMENTED();   // FIXME
                *params = GL_FALSE;
                return;
              case GL_COMPILE_STATUS:
                *params = shaderObject->isCompiled() ? GL_TRUE : GL_FALSE;
                return;
              case GL_INFO_LOG_LENGTH:
                UNIMPLEMENTED();   // FIXME
                *params = 0;
                return;
              case GL_SHADER_SOURCE_LENGTH:
                UNIMPLEMENTED();   // FIXME
                *params = 1;
                return;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetShaderInfoLog(GLuint shader, GLsizei bufsize, GLsizei* length, char* infolog)
{
    TRACE("GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, char* infolog = 0x%0.8p", shader, bufsize, length, infolog);

    try
    {
        if (bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetShaderPrecisionFormat(GLenum shadertype, GLenum precisiontype, GLint* range, GLint* precision)
{
    TRACE("GLenum shadertype = 0x%X, GLenum precisiontype = 0x%X, GLint* range = 0x%0.8p, GLint* precision = 0x%0.8p", shadertype, precisiontype, range, precision);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetShaderSource(GLuint shader, GLsizei bufsize, GLsizei* length, char* source)
{
    TRACE("GLuint shader = %d, GLsizei bufsize = %d, GLsizei* length = 0x%0.8p, char* source = 0x%0.8p", shader, bufsize, length, source);

    try
    {
        if (bufsize < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

const GLubyte* __stdcall glGetString(GLenum name)
{
    TRACE("GLenum name = 0x%X", name);

    try
    {
        switch (name)
        {
          case GL_VENDOR:
            return (GLubyte*)"TransGaming Inc.";
          case GL_RENDERER:
            return (GLubyte*)"ANGLE";
          case GL_VERSION:
            return (GLubyte*)"OpenGL ES 2.0 (git-devel "__DATE__ " " __TIME__")";
          case GL_SHADING_LANGUAGE_VERSION:
            return (GLubyte*)"OpenGL ES GLSL ES 1.00 (git-devel "__DATE__ " " __TIME__")";
          case GL_EXTENSIONS:
            return (GLubyte*)"";
          default:
            return error(GL_INVALID_ENUM, (GLubyte*)NULL);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, (GLubyte*)NULL);
    }

    return NULL;
}

void __stdcall glGetTexParameterfv(GLenum target, GLenum pname, GLfloat* params)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p", target, pname, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetTexParameteriv(GLenum target, GLenum pname, GLint* params)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLint* params = 0x%0.8p", target, pname, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetUniformfv(GLuint program, GLint location, GLfloat* params)
{
    TRACE("GLuint program = %d, GLint location = %d, GLfloat* params = 0x%0.8p", program, location, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetUniformiv(GLuint program, GLint location, GLint* params)
{
    TRACE("GLuint program = %d, GLint location = %d, GLint* params = 0x%0.8p", program, location, params);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

int __stdcall glGetUniformLocation(GLuint program, const char* name)
{
    TRACE("GLuint program = %d, const char* name = 0x%0.8p", program, name);

    try
    {
        gl::Context *context = gl::getContext();

        if (strstr(name, "gl_") == name)
        {
            return -1;
        }

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (!programObject)
            {
                return error(GL_INVALID_VALUE, -1);
            }

            if (!programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION, -1);
            }

            return programObject->getUniformLocation(name);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, -1);
    }

    return -1;
}

void __stdcall glGetVertexAttribfv(GLuint index, GLenum pname, GLfloat* params)
{
    TRACE("GLuint index = %d, GLenum pname = 0x%X, GLfloat* params = 0x%0.8p", index, pname, params);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetVertexAttribiv(GLuint index, GLenum pname, GLint* params)
{
    TRACE("GLuint index = %d, GLenum pname = 0x%X, GLint* params = 0x%0.8p", index, pname, params);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glGetVertexAttribPointerv(GLuint index, GLenum pname, void** pointer)
{
    TRACE("GLuint index = %d, GLenum pname = 0x%X, void** pointer = 0x%0.8p", index, pname, pointer);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glHint(GLenum target, GLenum mode)
{
    TRACE("GLenum target = 0x%X, GLenum mode = 0x%X", target, mode);

    try
    {
        switch (target)
        {
          case GL_GENERATE_MIPMAP_HINT:
            switch (mode)
            {
              case GL_FASTEST:
              case GL_NICEST:
              case GL_DONT_CARE:
                break;
              default:
                return error(GL_INVALID_ENUM); 
            }
            break;
          default:
              return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();
        if (context)
        {
            if (target == GL_GENERATE_MIPMAP_HINT)
                context->generateMipmapHint = mode;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

GLboolean __stdcall glIsBuffer(GLuint buffer)
{
    TRACE("GLuint buffer = %d", buffer);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && buffer)
        {
            gl::Buffer *bufferObject = context->getBuffer(buffer);

            if (bufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean __stdcall glIsEnabled(GLenum cap)
{
    TRACE("GLenum cap = 0x%X", cap);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (cap)
            {
              case GL_CULL_FACE:                return context->cullFace;
              case GL_POLYGON_OFFSET_FILL:      return context->polygonOffsetFill;
              case GL_SAMPLE_ALPHA_TO_COVERAGE: return context->sampleAlphaToCoverage;
              case GL_SAMPLE_COVERAGE:          return context->sampleCoverage;
              case GL_SCISSOR_TEST:             return context->scissorTest;
              case GL_STENCIL_TEST:             return context->stencilTest;
              case GL_DEPTH_TEST:               return context->depthTest;
              case GL_BLEND:                    return context->blend;
              case GL_DITHER:                   return context->dither;
              default:
                return error(GL_INVALID_ENUM, false);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, false);
    }

    return false;
}

GLboolean __stdcall glIsFramebuffer(GLuint framebuffer)
{
    TRACE("GLuint framebuffer = %d", framebuffer);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && framebuffer)
        {
            gl::Framebuffer *framebufferObject = context->getFramebuffer(framebuffer);

            if (framebufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean __stdcall glIsProgram(GLuint program)
{
    TRACE("GLuint program = %d", program);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && program)
        {
            gl::Program *programObject = context->getProgram(program);

            if (programObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean __stdcall glIsRenderbuffer(GLuint renderbuffer)
{
    TRACE("GLuint renderbuffer = %d", renderbuffer);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && renderbuffer)
        {
            gl::Renderbuffer *renderbufferObject = context->getRenderbuffer(renderbuffer);

            if (renderbufferObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean __stdcall glIsShader(GLuint shader)
{
    TRACE("GLuint shader = %d", shader);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && shader)
        {
            gl::Shader *shaderObject = context->getShader(shader);

            if (shaderObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

GLboolean __stdcall glIsTexture(GLuint texture)
{
    TRACE("GLuint texture = %d", texture);

    try
    {
        gl::Context *context = gl::getContext();

        if (context && texture)
        {
            gl::Texture *textureObject = context->getTexture(texture);

            if (textureObject)
            {
                return GL_TRUE;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY, GL_FALSE);
    }

    return GL_FALSE;
}

void __stdcall glLineWidth(GLfloat width)
{
    TRACE("GLfloat width = %f", width);

    try
    {
        if (width <= 0.0f)
        {
            return error(GL_INVALID_VALUE);
        }

        if (width != 1.0f)
        {
            UNIMPLEMENTED();   // FIXME
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glLinkProgram(GLuint program)
{
    TRACE("GLuint program = %d", program);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (!programObject)
            {
                return error(GL_INVALID_VALUE);
            }

            programObject->link();
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glPixelStorei(GLenum pname, GLint param)
{
    TRACE("GLenum pname = 0x%X, GLint param = %d", pname, param);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            switch (pname)
            {
              case GL_UNPACK_ALIGNMENT:
                if (param != 1 && param != 2 && param != 4 && param != 8)
                {
                    return error(GL_INVALID_VALUE);
                }

                context->unpackAlignment = param;
                break;

              case GL_PACK_ALIGNMENT:
                if (param != 1 && param != 2 && param != 4 && param != 8)
                {
                    return error(GL_INVALID_VALUE);
                }

                context->packAlignment = param;
                break;

              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glPolygonOffset(GLfloat factor, GLfloat units)
{
    TRACE("GLfloat factor = %f, GLfloat units = %f", factor, units);

    try
    {
        if (factor != 0.0f || units != 0.0f)
        {
            UNIMPLEMENTED();   // FIXME
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, void* pixels)
{
    TRACE("GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, void* pixels = 0x%0.8p", x, y, width, height, format, type,  pixels);

    try
    {
        if (width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        switch (format)
        {
          case GL_RGBA:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
                break;
              default:
                return error(GL_INVALID_OPERATION);
            }
            break;
          case gl::IMPLEMENTATION_COLOR_READ_FORMAT:
            switch (type)
            {
              case gl::IMPLEMENTATION_COLOR_READ_TYPE:
                break;
              default:
                return error(GL_INVALID_OPERATION);
            }
            break;
          default:
            return error(GL_INVALID_OPERATION);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->readPixels(x, y, width, height, format, type, pixels);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glReleaseShaderCompiler(void)
{
    TRACE("");

    try
    {
        gl::Shader::releaseCompiler();
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glRenderbufferStorage(GLenum target, GLenum internalformat, GLsizei width, GLsizei height)
{
    TRACE("GLenum target = 0x%X, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d", target, internalformat, width, height);

    try
    {
        switch (target)
        {
          case GL_RENDERBUFFER:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (internalformat)
        {
          case GL_DEPTH_COMPONENT16:
          case GL_RGBA4:
          case GL_RGB5_A1:
          case GL_RGB565:
          case GL_STENCIL_INDEX8:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if (width < 0 || height < 0 || width > gl::MAX_RENDERBUFFER_SIZE || height > gl::MAX_RENDERBUFFER_SIZE)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (context->framebuffer == 0 || context->renderbuffer == 0)
            {
                return error(GL_INVALID_OPERATION);
            }

            switch (internalformat)
            {
              case GL_DEPTH_COMPONENT16:
                context->setRenderbuffer(new gl::Depthbuffer(width, height));
                break;
              case GL_RGBA4:
              case GL_RGB5_A1:
              case GL_RGB565:
                UNIMPLEMENTED();   // FIXME
            //  context->setRenderbuffer(new Colorbuffer(renderTarget));
                break;
              case GL_STENCIL_INDEX8:
                context->setRenderbuffer(new gl::Stencilbuffer(width, height));
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glSampleCoverage(GLclampf value, GLboolean invert)
{
    TRACE("GLclampf value = %f, GLboolean invert = %d", value, invert);

    try
    {
        gl::Context* context = gl::getContext();

        if (context)
        {
            context->sampleCoverageValue = gl::clamp01(value);
            context->sampleCoverageInvert = invert;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d", x, y, width, height);

    try
    {
        if (width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context* context = gl::getContext();

        if (context)
        {
            context->scissorX = x;
            context->scissorY = y;
            context->scissorWidth = width;
            context->scissorHeight = height;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glShaderBinary(GLsizei n, const GLuint* shaders, GLenum binaryformat, const void* binary, GLsizei length)
{
    TRACE("GLsizei n = %d, const GLuint* shaders = 0x%0.8p, GLenum binaryformat = 0x%X, const void* binary = 0x%0.8p, GLsizei length = %d",
           n, shaders, binaryformat, binary, length);

    try
    {
        if (n < 0 || length < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glShaderSource(GLuint shader, GLsizei count, const char** string, const GLint* length)
{
    TRACE("GLuint shader = %d, GLsizei count = %d, const char** string = 0x%0.8p, const GLint* length = 0x%0.8p", shader, count, string, length);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Shader *shaderObject = context->getShader(shader);

            if (!shaderObject)
            {
                return error(GL_INVALID_VALUE);
            }

            shaderObject->setSource(count, string, length);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    glStencilFuncSeparate(GL_FRONT_AND_BACK, func, ref, mask);
}

void __stdcall glStencilFuncSeparate(GLenum face, GLenum func, GLint ref, GLuint mask)
{
    TRACE("GLenum face = 0x%X, GLenum func = 0x%X, GLint ref = %d, GLuint mask = %d", face, func, ref, mask);

    try
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (func)
        {
          case GL_NEVER:
          case GL_ALWAYS:
          case GL_LESS:
          case GL_LEQUAL:
          case GL_EQUAL:
          case GL_GEQUAL:
          case GL_GREATER:
          case GL_NOTEQUAL:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->stencilFunc = func;
                context->stencilRef = ref;
                context->stencilMask = mask;
            }

            if (face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->stencilBackFunc = func;
                context->stencilBackRef = ref;
                context->stencilBackMask = mask;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glStencilMask(GLuint mask)
{
    glStencilMaskSeparate(GL_FRONT_AND_BACK, mask);
}

void __stdcall glStencilMaskSeparate(GLenum face, GLuint mask)
{
    TRACE("GLenum face = 0x%X, GLuint mask = %d", face, mask);

    try
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->stencilWritemask = mask;
            }

            if (face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->stencilBackWritemask = mask;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    glStencilOpSeparate(GL_FRONT_AND_BACK, fail, zfail, zpass);
}

void __stdcall glStencilOpSeparate(GLenum face, GLenum fail, GLenum zfail, GLenum zpass)
{
    TRACE("GLenum face = 0x%X, GLenum fail = 0x%X, GLenum zfail = 0x%X, GLenum zpas = 0x%Xs", face, fail, zfail, zpass);

    try
    {
        switch (face)
        {
          case GL_FRONT:
          case GL_BACK:
          case GL_FRONT_AND_BACK:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (fail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (zfail)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        switch (zpass)
        {
          case GL_ZERO:
          case GL_KEEP:
          case GL_REPLACE:
          case GL_INCR:
          case GL_DECR:
          case GL_INVERT:
          case GL_INCR_WRAP:
          case GL_DECR_WRAP:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (face == GL_FRONT || face == GL_FRONT_AND_BACK)
            {
                context->stencilFail = fail;
                context->stencilPassDepthFail = zfail;
                context->stencilPassDepthPass = zpass;
            }

            if (face == GL_BACK || face == GL_FRONT_AND_BACK)
            {
                context->stencilBackFail = fail;
                context->stencilBackPassDepthFail = zfail;
                context->stencilBackPassDepthPass = zpass;
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLint internalformat = %d, GLsizei width = %d, GLsizei height = %d, GLint border = %d, GLenum format = 0x%X, GLenum type = 0x%X, const void* pixels =  0x%0.8p", target, level, internalformat, width, height, border, format, type, pixels);

    try
    {
        if (level < 0 || width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if (level > 0 && (!gl::isPow2(width) || !gl::isPow2(height)))
        {
            return error(GL_INVALID_VALUE);
        }

        switch (target)
        {
          case GL_TEXTURE_2D:
            if (width > (gl::MAX_TEXTURE_SIZE >> level) || height > (gl::MAX_TEXTURE_SIZE >> level))
            {
                return error(GL_INVALID_VALUE);
            }
            break;
          case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
          case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
          case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
          case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            if (!gl::isPow2(width) || !gl::isPow2(height))
            {
                return error(GL_INVALID_VALUE);
            }

            if (width > (gl::MAX_CUBE_MAP_TEXTURE_SIZE >> level) || height > (gl::MAX_CUBE_MAP_TEXTURE_SIZE >> level))
            {
                return error(GL_INVALID_VALUE);
            }
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if (internalformat != format)
        {
            return error(GL_INVALID_OPERATION);
        }

        switch (internalformat)
        {
          case GL_ALPHA:
          case GL_LUMINANCE:
          case GL_LUMINANCE_ALPHA:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
            break;
          case GL_RGB:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_UNSIGNED_SHORT_5_6_5:
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
            break;
          case GL_RGBA:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_UNSIGNED_SHORT_4_4_4_4:
              case GL_UNSIGNED_SHORT_5_5_5_1:
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
            break;
          default:
            return error(GL_INVALID_VALUE);
        }

        if (border != 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (target == GL_TEXTURE_2D)
            {
                gl::Texture2D *texture = context->getTexture2D();

                if (!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->setImage(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
            }
            else
            {
                gl::TextureCubeMap *texture = context->getTextureCubeMap();

                if (!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                switch (target)
                {
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
                    texture->setImagePosX(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
                    texture->setImageNegX(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
                    texture->setImagePosY(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
                    texture->setImageNegY(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
                    texture->setImagePosZ(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
                    texture->setImageNegZ(level, internalformat, width, height, format, type, context->unpackAlignment, pixels);
                    break;
                  default: UNREACHABLE();
                }
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    glTexParameteri(target, pname, (GLint)param);
}

void __stdcall glTexParameterfv(GLenum target, GLenum pname, const GLfloat* params)
{
    glTexParameteri(target, pname, (GLint)*params);
}

void __stdcall glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    TRACE("GLenum target = 0x%X, GLenum pname = 0x%X, GLfloat param = %f", target, pname, param);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Texture *texture;

            switch (target)
            {
              case GL_TEXTURE_2D:
                texture = context->getTexture2D();
                break;
              case GL_TEXTURE_CUBE_MAP:
                texture = context->getTextureCubeMap();
                break;
              default:
                return error(GL_INVALID_ENUM);
            }

            switch (pname)
            {
              case GL_TEXTURE_WRAP_S:
                if (!texture->setWrapS((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              case GL_TEXTURE_WRAP_T:
                if (!texture->setWrapT((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              case GL_TEXTURE_MIN_FILTER:
                if (!texture->setMinFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              case GL_TEXTURE_MAG_FILTER:
                if (!texture->setMagFilter((GLenum)param))
                {
                    return error(GL_INVALID_ENUM);
                }
                break;
              default:
                return error(GL_INVALID_ENUM);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glTexParameteriv(GLenum target, GLenum pname, const GLint* params)
{
    glTexParameteri(target, pname, *params);
}

void __stdcall glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void* pixels)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLint xoffset = %d, GLint yoffset = %d, GLsizei width = %d, GLsizei height = %d, GLenum format = 0x%X, GLenum type = 0x%X, const void* pixels = 0x%0.8p",
           target, level, xoffset, yoffset, width, height, format, type, pixels);

    try
    {
        if (target != GL_TEXTURE_2D && !es2dx::IsCubemapTextureTarget(target))
        {
            return error(GL_INVALID_ENUM);
        }

        if (level < 0 || level > gl::MAX_TEXTURE_LEVELS || xoffset < 0 || yoffset < 0 || width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if (std::numeric_limits<GLsizei>::max() - xoffset < width || std::numeric_limits<GLsizei>::max() - yoffset < height)
        {
            return error(GL_INVALID_VALUE);
        }

        if (!es2dx::CheckTextureFormatType(format, type))
        {
            return error(GL_INVALID_ENUM);
        }

        if (width == 0 || height == 0 || pixels == NULL)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            if (target == GL_TEXTURE_2D)
            {
                gl::Texture2D *texture = context->getTexture2D();

                if (!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->subImage(level, xoffset, yoffset, width, height, format, type, context->unpackAlignment, pixels);
            }
            else if (es2dx::IsCubemapTextureTarget(target))
            {
                gl::TextureCubeMap *texture = context->getTextureCubeMap();

                if (!texture)
                {
                    return error(GL_INVALID_OPERATION);
                }

                texture->subImage(target, level, xoffset, yoffset, width, height, format, type, context->unpackAlignment, pixels);
            }
            else
            {
                UNREACHABLE();
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform1f(GLint location, GLfloat x)
{
    glUniform1fv(location, 1, &x);
}

void __stdcall glUniform1fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p", location, count, v);

    try
    {
        if (location == -1)
        {
            return;
        }

        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniform1fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform1i(GLint location, GLint x)
{
    glUniform1iv(location, 1, &x);
}

void __stdcall glUniform1iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniform1iv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform2f(GLint location, GLfloat x, GLfloat y)
{
    GLfloat xy[2] = {x, y};

    glUniform2fv(location, 1, (GLfloat*)&xy);
}

void __stdcall glUniform2fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p", location, count, v);

    try
    {
        if (location == -1)
        {
            return;
        }

        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniform2fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform2i(GLint location, GLint x, GLint y)
{
    GLint xy[4] = {x, y};

    glUniform2iv(location, 1, (GLint*)&xy);
}

void __stdcall glUniform2iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform3f(GLint location, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat xyz[3] = {x, y, z};

    glUniform3fv(location, 1, (GLfloat*)&xyz);
}

void __stdcall glUniform3fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if (location == -1)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniform3fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform3i(GLint location, GLint x, GLint y, GLint z)
{
    GLint xyz[3] = {x, y, z};

    glUniform3iv(location, 1, (GLint*)&xyz);
}

void __stdcall glUniform3iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform4f(GLint location, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    GLfloat xyzw[4] = {x, y, z, w};

    glUniform4fv(location, 1, (GLfloat*)&xyzw);
}

void __stdcall glUniform4fv(GLint location, GLsizei count, const GLfloat* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLfloat* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        if (location == -1)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniform4fv(location, count, v))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniform4i(GLint location, GLint x, GLint y, GLint z, GLint w)
{
    GLint xyzw[4] = {x, y, z, w};

    glUniform4iv(location, 1, (GLint*)&xyzw);
}

void __stdcall glUniform4iv(GLint location, GLsizei count, const GLint* v)
{
    TRACE("GLint location = %d, GLsizei count = %d, const GLint* v = 0x%0.8p", location, count, v);

    try
    {
        if (count < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniformMatrix2fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p", location, count, transpose, value);

    try
    {
        if (count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if (location == -1)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniformMatrix2fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniformMatrix3fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p", location, count, transpose, value);

    try
    {
        if (count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if (location == -1)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniformMatrix3fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUniformMatrix4fv(GLint location, GLsizei count, GLboolean transpose, const GLfloat* value)
{
    TRACE("GLint location = %d, GLsizei count = %d, GLboolean transpose = %d, const GLfloat* value = 0x%0.8p", location, count, transpose, value);

    try
    {
        if (count < 0 || transpose != GL_FALSE)
        {
            return error(GL_INVALID_VALUE);
        }

        if (location == -1)
        {
            return;
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *program = context->getCurrentProgram();

            if (!program)
            {
                return error(GL_INVALID_OPERATION);
            }

            if (!program->setUniformMatrix4fv(location, count, value))
            {
                return error(GL_INVALID_OPERATION);
            }
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glUseProgram(GLuint program)
{
    TRACE("GLuint program = %d", program);

    try
    {
        gl::Context *context = gl::getContext();

        if (context)
        {
            gl::Program *programObject = context->getProgram(program);

            if (programObject && !programObject->isLinked())
            {
                return error(GL_INVALID_OPERATION);
            }

            context->useProgram(program);
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glValidateProgram(GLuint program)
{
    TRACE("GLuint program = %d", program);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib1f(GLuint index, GLfloat x)
{
    TRACE("GLuint index = %d, GLfloat x = %f", index, x);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib1fv(GLuint index, const GLfloat* values)
{
    TRACE("GLuint index = %d, const GLfloat* values = 0x%0.8p", index, values);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib2f(GLuint index, GLfloat x, GLfloat y)
{
    TRACE("GLuint index = %d, GLfloat x = %f, GLfloat y = %f", index, x, y);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib2fv(GLuint index, const GLfloat* values)
{
    TRACE("GLuint index = %d, const GLfloat* values = 0x%0.8p", index, values);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib3f(GLuint index, GLfloat x, GLfloat y, GLfloat z)
{
    TRACE("GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f", index, x, y, z);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib3fv(GLuint index, const GLfloat* values)
{
    TRACE("GLuint index = %d, const GLfloat* values = 0x%0.8p", index, values);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    TRACE("GLuint index = %d, GLfloat x = %f, GLfloat y = %f, GLfloat z = %f, GLfloat w = %f", index, x, y, z, w);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttrib4fv(GLuint index, const GLfloat* values)
{
    TRACE("GLuint index = %d, const GLfloat* values = 0x%0.8p", index, values);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glVertexAttribPointer(GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void* ptr)
{
    TRACE("GLuint index = %d, GLint size = %d, GLenum type = 0x%X, GLboolean normalized = %d, GLsizei stride = %d, const void* ptr = 0x%0.8p", index, size, type, normalized, stride, ptr);

    try
    {
        if (index >= gl::MAX_VERTEX_ATTRIBS)
        {
            return error(GL_INVALID_VALUE);
        }

        if (size < 1 || size > 4)
        {
            return error(GL_INVALID_VALUE);
        }

        switch (type)
        {
          case GL_BYTE:
          case GL_UNSIGNED_BYTE:
          case GL_SHORT:
          case GL_UNSIGNED_SHORT:
          case GL_FIXED:
          case GL_FLOAT:
            break;
          default:
            return error(GL_INVALID_ENUM);
        }

        if (stride < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->vertexAttribute[index].mBoundBuffer = context->arrayBuffer;
            context->vertexAttribute[index].mSize = size;
            context->vertexAttribute[index].mType = type;
            context->vertexAttribute[index].mNormalized = normalized;
            context->vertexAttribute[index].mStride = stride;
            context->vertexAttribute[index].mPointer = ptr;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACE("GLint x = %d, GLint y = %d, GLsizei width = %d, GLsizei height = %d", x, y, width, height);

    try
    {
        if (width < 0 || height < 0)
        {
            return error(GL_INVALID_VALUE);
        }

        gl::Context *context = gl::getContext();

        if (context)
        {
            context->viewportX = x;
            context->viewportY = y;
            context->viewportWidth = width;
            context->viewportHeight = height;
        }
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}

void __stdcall glTexImage3DOES(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void* pixels)
{
    TRACE("GLenum target = 0x%X, GLint level = %d, GLenum internalformat = 0x%X, GLsizei width = %d, GLsizei height = %d, GLsizei depth = %d, GLint border = %d, GLenum format = 0x%X, GLenum type = 0x%x, const void* pixels = 0x%0.8p",
          target, level, internalformat, width, height, depth, border, format, type, pixels);

    try
    {
        UNIMPLEMENTED();   // FIXME
    }
    catch(std::bad_alloc&)
    {
        return error(GL_OUT_OF_MEMORY);
    }
}
}

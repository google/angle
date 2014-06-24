#include "precompiled.h"
//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// validationES2.cpp: Validation functions for OpenGL ES 2.0 entry point parameters

#include "libGLESv2/validationES2.h"
#include "libGLESv2/validationES.h"
#include "libGLESv2/Context.h"
#include "libGLESv2/Texture.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/Renderbuffer.h"
#include "libGLESv2/formatutils.h"
#include "libGLESv2/main.h"
#include "libGLESv2/FramebufferAttachment.h"

#include "common/mathutil.h"
#include "common/utilities.h"

namespace gl
{

static bool validateSubImageParams2D(bool compressed, GLsizei width, GLsizei height,
                                     GLint xoffset, GLint yoffset, GLint level, GLenum format, GLenum type,
                                     gl::Texture2D *texture)
{
    if (!texture)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (compressed != texture->isCompressed(level))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (format != GL_NONE)
    {
        GLenum internalformat = gl::GetSizedInternalFormat(format, type);
        if (internalformat != texture->getInternalFormat(level))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    if (compressed)
    {
        if ((width % 4 != 0 && width != texture->getWidth(level)) ||
            (height % 4 != 0 && height != texture->getHeight(level)))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    if (xoffset + width > texture->getWidth(level) ||
        yoffset + height > texture->getHeight(level))
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    return true;
}

static bool validateSubImageParamsCube(bool compressed, GLsizei width, GLsizei height,
                                       GLint xoffset, GLint yoffset, GLenum target, GLint level, GLenum format, GLenum type,
                                       gl::TextureCubeMap *texture)
{
    if (!texture)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (compressed != texture->isCompressed(target, level))
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (format != GL_NONE)
    {
        GLenum internalformat = gl::GetSizedInternalFormat(format, type);
        if (internalformat != texture->getInternalFormat(target, level))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    if (compressed)
    {
        if ((width % 4 != 0 && width != texture->getWidth(target, 0)) ||
            (height % 4 != 0 && height != texture->getHeight(target, 0)))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    if (xoffset + width > texture->getWidth(target, level) ||
        yoffset + height > texture->getHeight(target, level))
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    return true;
}

bool ValidateES2TexImageParameters(gl::Context *context, GLenum target, GLint level, GLenum internalformat, bool isCompressed, bool isSubImage,
                                   GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
                                   GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
    if (!ValidTexture2DDestinationTarget(context, target))
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (!ValidImageSize(context, target, level, width, height, 1))
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (level < 0 || xoffset < 0 ||
        std::numeric_limits<GLsizei>::max() - xoffset < width ||
        std::numeric_limits<GLsizei>::max() - yoffset < height)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (!isSubImage && !isCompressed && internalformat != format)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    const gl::Caps &caps = context->getCaps();

    gl::Texture *texture = NULL;
    bool textureCompressed = false;
    GLenum textureInternalFormat = GL_NONE;
    GLint textureLevelWidth = 0;
    GLint textureLevelHeight = 0;
    switch (target)
    {
      case GL_TEXTURE_2D:
        {
            if (static_cast<GLuint>(width) > (caps.max2DTextureSize >> level) ||
                static_cast<GLuint>(height) > (caps.max2DTextureSize >> level))
            {
                return gl::error(GL_INVALID_VALUE, false);
            }

            gl::Texture2D *tex2d = context->getTexture2D();
            if (tex2d)
            {
                textureCompressed = tex2d->isCompressed(level);
                textureInternalFormat = tex2d->getInternalFormat(level);
                textureLevelWidth = tex2d->getWidth(level);
                textureLevelHeight = tex2d->getHeight(level);
                texture = tex2d;
            }

            if (isSubImage && !validateSubImageParams2D(isCompressed, width, height, xoffset, yoffset,
                                                        level, format, type, tex2d))
            {
                return false;
            }

            texture = tex2d;
        }
        break;

      case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
      case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
      case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
        {
            if (!isSubImage && width != height)
            {
                return gl::error(GL_INVALID_VALUE, false);
            }

            if (static_cast<GLuint>(width) > (caps.maxCubeMapTextureSize >> level) ||
                static_cast<GLuint>(height) > (caps.maxCubeMapTextureSize >> level))
            {
                return gl::error(GL_INVALID_VALUE, false);
            }

            gl::TextureCubeMap *texCube = context->getTextureCubeMap();
            if (texCube)
            {
                textureCompressed = texCube->isCompressed(target, level);
                textureInternalFormat = texCube->getInternalFormat(target, level);
                textureLevelWidth = texCube->getWidth(target, level);
                textureLevelHeight = texCube->getHeight(target, level);
                texture = texCube;
            }

            if (isSubImage && !validateSubImageParamsCube(isCompressed, width, height, xoffset, yoffset,
                                                          target, level, format, type, texCube))
            {
                return false;
            }
        }
        break;

      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (!texture)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (!isSubImage && texture->isImmutable())
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    // Verify zero border
    if (border != 0)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    GLenum actualInternalFormat = isSubImage ? textureInternalFormat : internalformat;
    if (isCompressed)
    {
        if (!ValidCompressedImageSize(context, actualInternalFormat, width, height))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }

        switch (actualInternalFormat)
        {
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            if (!context->getExtensions().textureCompressionDXT1)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
            if (!context->getExtensions().textureCompressionDXT1)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
              if (!context->getExtensions().textureCompressionDXT5)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
    }
    else
    {
        // validate <type> by itself (used as secondary key below)
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
          case GL_UNSIGNED_SHORT_5_6_5:
          case GL_UNSIGNED_SHORT_4_4_4_4:
          case GL_UNSIGNED_SHORT_5_5_5_1:
          case GL_UNSIGNED_SHORT:
          case GL_UNSIGNED_INT:
          case GL_UNSIGNED_INT_24_8_OES:
          case GL_HALF_FLOAT_OES:
          case GL_FLOAT:
            break;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }

        // validate <format> + <type> combinations
        // - invalid <format> -> sets INVALID_ENUM
        // - invalid <format>+<type> combination -> sets INVALID_OPERATION
        switch (format)
        {
          case GL_ALPHA:
          case GL_LUMINANCE:
          case GL_LUMINANCE_ALPHA:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_FLOAT:
              case GL_HALF_FLOAT_OES:
                break;
            default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_RED:
          case GL_RG:
              if (!context->getExtensions().textureRG)
              {
                  return gl::error(GL_INVALID_ENUM, false);
              }
              switch (type)
              {
                case GL_UNSIGNED_BYTE:
                case GL_FLOAT:
                case GL_HALF_FLOAT_OES:
                  break;
                default:
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RGB:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_UNSIGNED_SHORT_5_6_5:
              case GL_FLOAT:
              case GL_HALF_FLOAT_OES:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_RGBA:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
              case GL_UNSIGNED_SHORT_4_4_4_4:
              case GL_UNSIGNED_SHORT_5_5_5_1:
              case GL_FLOAT:
              case GL_HALF_FLOAT_OES:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_BGRA_EXT:
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_SRGB_EXT:
          case GL_SRGB_ALPHA_EXT:
            if (!context->getExtensions().sRGB)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            switch (type)
            {
              case GL_UNSIGNED_BYTE:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:  // error cases for compressed textures are handled below
          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
          case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
            break;
          case GL_DEPTH_COMPONENT:
            switch (type)
            {
              case GL_UNSIGNED_SHORT:
              case GL_UNSIGNED_INT:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_DEPTH_STENCIL_OES:
            switch (type)
            {
              case GL_UNSIGNED_INT_24_8_OES:
                break;
              default:
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }

        switch (format)
        {
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            if (context->getExtensions().textureCompressionDXT1)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
            if (context->getExtensions().textureCompressionDXT3)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
            if (context->getExtensions().textureCompressionDXT5)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_DEPTH_COMPONENT:
          case GL_DEPTH_STENCIL_OES:
            if (!context->getExtensions().depthTextures)
            {
                return gl::error(GL_INVALID_VALUE, false);
            }
            if (target != GL_TEXTURE_2D)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            // OES_depth_texture supports loading depth data and multiple levels,
            // but ANGLE_depth_texture does not
            if (pixels != NULL || level != 0)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          default:
            break;
        }

        if (type == GL_FLOAT)
        {
            if (!context->getExtensions().textureFloat)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
        }
        else if (type == GL_HALF_FLOAT_OES)
        {
            if (!context->getExtensions().textureHalfFloat)
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
        }
    }

    return true;
}



bool ValidateES2CopyTexImageParameters(gl::Context* context, GLenum target, GLint level, GLenum internalformat, bool isSubImage,
                                       GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height,
                                       GLint border)
{
    GLenum textureInternalFormat = GL_NONE;

    if (!ValidateCopyTexImageParametersBase(context, target, level, internalformat, isSubImage,
                                            xoffset, yoffset, 0, x, y, width, height, border, &textureInternalFormat))
    {
        return false;
    }

    gl::Framebuffer *framebuffer = context->getState().getReadFramebuffer();
    GLenum colorbufferFormat = framebuffer->getReadColorbuffer()->getInternalFormat();
    GLenum textureFormat = gl::GetFormat(textureInternalFormat);

    // [OpenGL ES 2.0.24] table 3.9
    if (isSubImage)
    {
        switch (textureFormat)
        {
          case GL_ALPHA:
            if (colorbufferFormat != GL_ALPHA8_EXT &&
                colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_LUMINANCE:
              if (colorbufferFormat != GL_R8_EXT &&
                  colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RED_EXT:
              if (colorbufferFormat != GL_R8_EXT &&
                  colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RG_EXT:
              if (colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RGB:
            if (colorbufferFormat != GL_RGB565 &&
                colorbufferFormat != GL_RGB8_OES &&
                colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_LUMINANCE_ALPHA:
          case GL_RGBA:
            if (colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
          case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
            return gl::error(GL_INVALID_OPERATION, false);
          case GL_DEPTH_COMPONENT:
          case GL_DEPTH_STENCIL_OES:
            return gl::error(GL_INVALID_OPERATION, false);
          default:
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }
    else
    {
        switch (internalformat)
        {
          case GL_ALPHA:
            if (colorbufferFormat != GL_ALPHA8_EXT &&
                colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_BGRA8_EXT &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_LUMINANCE:
              if (colorbufferFormat != GL_R8_EXT &&
                  colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_BGRA8_EXT &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RED_EXT:
              if (colorbufferFormat != GL_R8_EXT &&
                  colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_BGRA8_EXT &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RG_EXT:
              if (colorbufferFormat != GL_RG8_EXT &&
                  colorbufferFormat != GL_RGB565 &&
                  colorbufferFormat != GL_RGB8_OES &&
                  colorbufferFormat != GL_RGBA4 &&
                  colorbufferFormat != GL_RGB5_A1 &&
                  colorbufferFormat != GL_BGRA8_EXT &&
                  colorbufferFormat != GL_RGBA8_OES)
              {
                  return gl::error(GL_INVALID_OPERATION, false);
              }
              break;
          case GL_RGB:
            if (colorbufferFormat != GL_RGB565 &&
                colorbufferFormat != GL_RGB8_OES &&
                colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_BGRA8_EXT &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_LUMINANCE_ALPHA:
          case GL_RGBA:
            if (colorbufferFormat != GL_RGBA4 &&
                colorbufferFormat != GL_RGB5_A1 &&
                colorbufferFormat != GL_BGRA8_EXT &&
                colorbufferFormat != GL_RGBA8_OES)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            break;
          case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
          case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            if (context->getExtensions().textureCompressionDXT1)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
            if (context->getExtensions().textureCompressionDXT3)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
            if (context->getExtensions().textureCompressionDXT5)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
            break;
          case GL_DEPTH_COMPONENT:
          case GL_DEPTH_COMPONENT16:
          case GL_DEPTH_COMPONENT32_OES:
          case GL_DEPTH_STENCIL_OES:
          case GL_DEPTH24_STENCIL8_OES:
            if (context->getExtensions().depthTextures)
            {
                return gl::error(GL_INVALID_OPERATION, false);
            }
            else
            {
                return gl::error(GL_INVALID_ENUM, false);
            }
          default:
            return gl::error(GL_INVALID_ENUM, false);
        }
    }

    // If width or height is zero, it is a no-op.  Return false without setting an error.
    return (width > 0 && height > 0);
}

bool ValidateES2TexStorageParameters(gl::Context *context, GLenum target, GLsizei levels, GLenum internalformat,
                                     GLsizei width, GLsizei height)
{
    if (target != GL_TEXTURE_2D && target != GL_TEXTURE_CUBE_MAP)
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (width < 1 || height < 1 || levels < 1)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (target == GL_TEXTURE_CUBE_MAP && width != height)
    {
        return gl::error(GL_INVALID_VALUE, false);
    }

    if (levels != 1 && levels != gl::log2(std::max(width, height)) + 1)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    GLenum format = gl::GetFormat(internalformat);
    GLenum type = gl::GetType(internalformat);

    if (format == GL_NONE || type == GL_NONE)
    {
        return gl::error(GL_INVALID_ENUM, false);
    }

    const gl::Caps &caps = context->getCaps();

    switch (target)
    {
      case GL_TEXTURE_2D:
        if (static_cast<GLuint>(width) > caps.max2DTextureSize ||
            static_cast<GLuint>(height) > caps.max2DTextureSize)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
        break;
      case GL_TEXTURE_CUBE_MAP:
        if (static_cast<GLuint>(width) > caps.maxCubeMapTextureSize ||
            static_cast<GLuint>(height) > caps.maxCubeMapTextureSize)
        {
            return gl::error(GL_INVALID_VALUE, false);
        }
        break;
      default:
        return gl::error(GL_INVALID_ENUM, false);
    }

    if (levels != 1 && !context->getExtensions().textureNPOT)
    {
        if (!gl::isPow2(width) || !gl::isPow2(height))
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
    }

    switch (internalformat)
    {
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        if (!context->getExtensions().textureCompressionDXT1)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
        if (!context->getExtensions().textureCompressionDXT3)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        if (!context->getExtensions().textureCompressionDXT5)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        if (!context->getExtensions().textureFloat)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
        if (!context->getExtensions().textureHalfFloat)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_R8_EXT:
      case GL_RG8_EXT:
      case GL_R16F_EXT:
      case GL_RG16F_EXT:
      case GL_R32F_EXT:
      case GL_RG32F_EXT:
        if (!context->getExtensions().textureRG)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        break;
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT32_OES:
      case GL_DEPTH24_STENCIL8_OES:
        if (!context->getExtensions().depthTextures)
        {
            return gl::error(GL_INVALID_ENUM, false);
        }
        if (target != GL_TEXTURE_2D)
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
        // ANGLE_depth_texture only supports 1-level textures
        if (levels != 1)
        {
            return gl::error(GL_INVALID_OPERATION, false);
        }
        break;
      default:
        break;
    }

    gl::Texture *texture = NULL;
    switch(target)
    {
      case GL_TEXTURE_2D:
        texture = context->getTexture2D();
        break;
      case GL_TEXTURE_CUBE_MAP:
        texture = context->getTextureCubeMap();
        break;
      default:
        UNREACHABLE();
    }

    if (!texture || texture->id() == 0)
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    if (texture->isImmutable())
    {
        return gl::error(GL_INVALID_OPERATION, false);
    }

    return true;
}

// check for combinations of format and type that are valid for ReadPixels
bool ValidES2ReadFormatType(gl::Context *context, GLenum format, GLenum type)
{
    switch (format)
    {
      case GL_RGBA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            break;
          default:
            return false;
        }
        break;
      case GL_BGRA_EXT:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
          case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
          case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
            break;
          default:
            return false;
        }
        break;
      case GL_RG_EXT:
      case GL_RED_EXT:
        if (!context->getExtensions().textureRG)
        {
            return false;
        }
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            break;
          default:
            return false;
        }
        break;

      default:
        return false;
    }
    return true;
}

}

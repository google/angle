//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// formatutils.h: Queries for GL image formats.

#ifndef LIBGLESV2_FORMATUTILS_H_
#define LIBGLESV2_FORMATUTILS_H_

#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

typedef void (*MipGenerationFunction)(unsigned int sourceWidth, unsigned int sourceHeight, unsigned int sourceDepth,
                                      const unsigned char *sourceData, int sourceRowPitch, int sourceDepthPitch,
                                      unsigned char *destData, int destRowPitch, int destDepthPitch);

typedef void (*LoadImageFunction)(int width, int height, int depth,
                                  const void *input, unsigned int inputRowPitch, unsigned int inputDepthPitch,
                                  void *output, unsigned int outputRowPitch, unsigned int outputDepthPitch);

namespace rx
{

class Renderer;

}

namespace gl
{

class Context;

bool IsValidInternalFormat(GLint internalFormat, const Context *context);
bool IsValidFormat(GLenum format, GLuint clientVersion);
bool IsValidType(GLenum type, GLuint clientVersion);

bool IsValidFormatCombination(GLint internalFormat, GLenum format, GLenum type, GLuint clientVersion);
bool IsValidCopyTexImageCombination(GLenum textureInternalFormat, GLenum frameBufferInternalFormat, GLuint clientVersion);

bool IsSizedInternalFormat(GLint internalFormat, GLuint clientVersion);
GLint GetSizedInternalFormat(GLenum format, GLenum type, GLuint clientVersion);

GLuint GetPixelBytes(GLint internalFormat, GLuint clientVersion);
GLuint GetAlphaBits(GLint internalFormat, GLuint clientVersion);
GLuint GetRedBits(GLint internalFormat, GLuint clientVersion);
GLuint GetGreenBits(GLint internalFormat, GLuint clientVersion);
GLuint GetBlueBits(GLint internalFormat, GLuint clientVersion);
GLuint GetLuminanceBits(GLint internalFormat, GLuint clientVersion);
GLuint GetDepthBits(GLint internalFormat, GLuint clientVersion);
GLuint GetStencilBits(GLint internalFormat, GLuint clientVersion);

GLenum GetFormat(GLint internalFormat, GLuint clientVersion);
GLenum GetType(GLint internalFormat, GLuint clientVersion);

bool IsNormalizedFixedPointFormat(GLint internalFormat, GLuint clientVersion);
bool IsIntegerFormat(GLint internalFormat, GLuint clientVersion);
bool IsSignedIntegerFormat(GLint internalFormat, GLuint clientVersion);
bool IsUnsignedIntegerFormat(GLint internalFormat, GLuint clientVersion);
bool IsFloatingPointFormat(GLint internalFormat, GLuint clientVersion);

bool IsColorRenderingSupported(GLint internalFormat, const rx::Renderer *renderer);
bool IsColorRenderingSupported(GLint internalFormat, const Context *context);
bool IsTextureFilteringSupported(GLint internalFormat, const rx::Renderer *renderer);
bool IsTextureFilteringSupported(GLint internalFormat, const Context *context);
bool IsDepthRenderingSupported(GLint internalFormat, const rx::Renderer *renderer);
bool IsDepthRenderingSupported(GLint internalFormat, const Context *context);
bool IsStencilRenderingSupported(GLint internalFormat, const rx::Renderer *renderer);
bool IsStencilRenderingSupported(GLint internalFormat, const Context *context);

GLuint GetRowPitch(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLint alignment);
GLuint GetDepthPitch(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLsizei height, GLint alignment);
GLuint GetBlockSize(GLint internalFormat, GLenum type, GLuint clientVersion, GLsizei width, GLsizei height);

bool IsFormatCompressed(GLint internalFormat, GLuint clientVersion);
GLuint GetCompressedBlockWidth(GLint internalFormat, GLuint clientVersion);
GLuint GetCompressedBlockHeight(GLint internalFormat, GLuint clientVersion);

}

#endif LIBGLESV2_FORMATUTILS_H_

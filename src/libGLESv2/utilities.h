//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// utilities.h: Conversion functions and other utility routines.

#ifndef LIBGLESV2_UTILITIES_H
#define LIBGLESV2_UTILITIES_H

#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace gl
{

struct Color;

int UniformComponentCount(GLenum type);
GLenum UniformComponentType(GLenum type);
size_t UniformInternalSize(GLenum type);
size_t UniformExternalSize(GLenum type);
GLenum UniformBoolVectorType(GLenum type);
int VariableRowCount(GLenum type);
int VariableColumnCount(GLenum type);
bool IsMatrixType(GLenum type);

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

void MakeValidSize(bool isImage, bool isCompressed, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset);
int ComputePixelSize(GLint internalformat);
GLsizei ComputeRowPitch(GLsizei width, GLint internalformat, GLint alignment);
GLsizei ComputeDepthPitch(GLsizei width, GLsizei height, GLint internalformat, GLint alignment);
GLsizei ComputeCompressedRowPitch(GLsizei width, GLenum format);
GLsizei ComputeCompressedDepthPitch(GLsizei width, GLsizei height, GLenum format);
GLsizei ComputeCompressedSize(GLsizei width, GLsizei height, GLenum format);
bool IsCompressed(GLenum format);
bool IsDepthTexture(GLenum format);
bool IsStencilTexture(GLenum format);
bool IsCubemapTextureTarget(GLenum target);
bool IsInternalTextureTarget(GLenum target);
GLint ConvertSizedInternalFormat(GLenum format, GLenum type);
GLenum ExtractFormat(GLenum internalformat);
GLenum ExtractType(GLenum internalformat);

bool IsColorRenderable(GLenum internalformat);
bool IsDepthRenderable(GLenum internalformat);
bool IsStencilRenderable(GLenum internalformat);

bool IsFloat32Format(GLint internalformat);
bool IsFloat16Format(GLint internalformat);

GLuint GetAlphaSize(GLenum colorFormat);
GLuint GetRedSize(GLenum colorFormat);
GLuint GetGreenSize(GLenum colorFormat);
GLuint GetBlueSize(GLenum colorFormat);
GLuint GetDepthSize(GLenum depthFormat);
GLuint GetStencilSize(GLenum stencilFormat);
bool IsTriangleMode(GLenum drawMode);

bool IsValidES3FormatCombination(GLint internalformat, GLenum format, GLenum type, GLenum* err);
bool IsValidES3CompressedFormat(GLenum format);
bool IsValidES3CopyTexImageCombination(GLenum textureFormat, GLenum frameBufferFormat);

}

std::string getTempPath();
void writeFile(const char* path, const void* data, size_t size);

#endif  // LIBGLESV2_UTILITIES_H

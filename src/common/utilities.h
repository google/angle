//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// utilities.h: Conversion functions and other utility routines.

#ifndef LIBGLESV2_UTILITIES_H
#define LIBGLESV2_UTILITIES_H

#ifndef GL_APICALL
#define GL_APICALL
#endif
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <string>

namespace gl
{

int UniformComponentCount(GLenum type);
GLenum UniformComponentType(GLenum type);
size_t UniformComponentSize(GLenum type);
size_t UniformInternalSize(GLenum type);
size_t UniformExternalSize(GLenum type);
GLenum UniformBoolVectorType(GLenum type);
int VariableRowCount(GLenum type);
int VariableColumnCount(GLenum type);
bool IsMatrixType(GLenum type);
GLenum TransposeMatrixType(GLenum type);
int AttributeRegisterCount(GLenum type);
int MatrixRegisterCount(GLenum type, bool isRowMajorMatrix);
int MatrixComponentCount(GLenum type, bool isRowMajorMatrix);

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize);

bool IsCubemapTextureTarget(GLenum target);
bool IsInternalTextureTarget(GLenum target);

bool IsTriangleMode(GLenum drawMode);

}

std::string getTempPath();
void writeFile(const char* path, const void* data, size_t size);

#endif  // LIBGLESV2_UTILITIES_H

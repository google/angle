#include "precompiled.h"
//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// utilities.cpp: Conversion functions and other utility routines.

#include "libGLESv2/utilities.h"
#include "libGLESv2/mathutil.h"

namespace gl
{

int UniformComponentCount(GLenum type)
{
    switch (type)
    {
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
      case GL_UNSIGNED_INT:
          return 1;
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_UNSIGNED_INT_VEC2:
          return 2;
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
      case GL_UNSIGNED_INT_VEC3:
          return 3;
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_UNSIGNED_INT_VEC4:
      case GL_FLOAT_MAT2:
          return 4;
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
          return 6;
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT4x2:
          return 8;
      case GL_FLOAT_MAT3:
          return 9;
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x3:
          return 12;
      case GL_FLOAT_MAT4:
          return 16;
      default:
          UNREACHABLE();
    }

    return 0;
}

GLenum UniformComponentType(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:
      case GL_BOOL_VEC2:
      case GL_BOOL_VEC3:
      case GL_BOOL_VEC4:
          return GL_BOOL;
      case GL_FLOAT:
      case GL_FLOAT_VEC2:
      case GL_FLOAT_VEC3:
      case GL_FLOAT_VEC4:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT3x4:
      case GL_FLOAT_MAT4x3:
          return GL_FLOAT;
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
      case GL_INT_VEC2:
      case GL_INT_VEC3:
      case GL_INT_VEC4:
          return GL_INT;
      case GL_UNSIGNED_INT:
      case GL_UNSIGNED_INT_VEC2:
      case GL_UNSIGNED_INT_VEC3:
      case GL_UNSIGNED_INT_VEC4:
          return GL_UNSIGNED_INT;
      default:
          UNREACHABLE();
    }

    return GL_NONE;
}

size_t UniformComponentSize(GLenum type)
{
    switch(type)
    {
      case GL_BOOL:         return sizeof(GLint);
      case GL_FLOAT:        return sizeof(GLfloat);
      case GL_INT:          return sizeof(GLint);
      case GL_UNSIGNED_INT: return sizeof(GLuint);
      default:       UNREACHABLE();
    }

    return 0;
}

size_t UniformInternalSize(GLenum type)
{
    // Expanded to 4-element vectors
    return UniformComponentSize(UniformComponentType(type)) * VariableRowCount(type) * 4;
}

size_t UniformExternalSize(GLenum type)
{
    return UniformComponentSize(UniformComponentType(type)) * UniformComponentCount(type);
}

GLenum UniformBoolVectorType(GLenum type)
{
    switch (type)
    {
      case GL_FLOAT:
      case GL_INT:
      case GL_UNSIGNED_INT:
        return GL_BOOL;
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_UNSIGNED_INT_VEC2:
        return GL_BOOL_VEC2;
      case GL_FLOAT_VEC3:
      case GL_INT_VEC3:
      case GL_UNSIGNED_INT_VEC3:
        return GL_BOOL_VEC3;
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_UNSIGNED_INT_VEC4:
        return GL_BOOL_VEC4;

      default:
        UNREACHABLE();
        return GL_NONE;
    }
}

int VariableRowCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
        return 1;
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT4x2:
        return 2;
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT4x3:
        return 3;
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT2x4:
      case GL_FLOAT_MAT3x4:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

int VariableColumnCount(GLenum type)
{
    switch (type)
    {
      case GL_NONE:
        return 0;
      case GL_BOOL:
      case GL_FLOAT:
      case GL_INT:
      case GL_SAMPLER_2D:
      case GL_SAMPLER_CUBE:
        return 1;
      case GL_BOOL_VEC2:
      case GL_FLOAT_VEC2:
      case GL_INT_VEC2:
      case GL_FLOAT_MAT2:
      case GL_FLOAT_MAT2x3:
      case GL_FLOAT_MAT2x4:
        return 2;
      case GL_INT_VEC3:
      case GL_FLOAT_VEC3:
      case GL_BOOL_VEC3:
      case GL_FLOAT_MAT3:
      case GL_FLOAT_MAT3x2:
      case GL_FLOAT_MAT3x4:
        return 3;
      case GL_BOOL_VEC4:
      case GL_FLOAT_VEC4:
      case GL_INT_VEC4:
      case GL_FLOAT_MAT4:
      case GL_FLOAT_MAT4x2:
      case GL_FLOAT_MAT4x3:
        return 4;
      default:
        UNREACHABLE();
    }

    return 0;
}

bool IsMatrixType(GLenum type)
{
    return VariableRowCount(type) > 1;
}

int AllocateFirstFreeBits(unsigned int *bits, unsigned int allocationSize, unsigned int bitsSize)
{
    ASSERT(allocationSize <= bitsSize);

    unsigned int mask = std::numeric_limits<unsigned int>::max() >> (std::numeric_limits<unsigned int>::digits - allocationSize);

    for (unsigned int i = 0; i < bitsSize - allocationSize + 1; i++)
    {
        if ((*bits & mask) == 0)
        {
            *bits |= mask;
            return i;
        }

        mask <<= 1;
    }

    return -1;
}

bool IsStencilTexture(GLenum format)
{
    if (format == GL_DEPTH_STENCIL_OES ||
        format == GL_DEPTH24_STENCIL8_OES)
    {
        return true;
    }

    return false;
}

void MakeValidSize(bool isImage, bool isCompressed, GLsizei *requestWidth, GLsizei *requestHeight, int *levelOffset)
{
    int upsampleCount = 0;

    if (isCompressed)
    {
        // Don't expand the size of full textures that are at least 4x4
        // already.
        if (isImage || *requestWidth < 4 || *requestHeight < 4)
        {
            while (*requestWidth % 4 != 0 || *requestHeight % 4 != 0)
            {
                *requestWidth <<= 1;
                *requestHeight <<= 1;
                upsampleCount++;
            }
        }
    }
    *levelOffset = upsampleCount;
}

// Returns the size, in bytes, of a single texel in an Image
int ComputePixelSize(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_ALPHA8_EXT:                       return sizeof(unsigned char);
      case GL_LUMINANCE8_EXT:                   return sizeof(unsigned char);
      case GL_ALPHA32F_EXT:                     return sizeof(float);
      case GL_LUMINANCE32F_EXT:                 return sizeof(float);
      case GL_ALPHA16F_EXT:                     return sizeof(unsigned short);
      case GL_LUMINANCE16F_EXT:                 return sizeof(unsigned short);
      case GL_LUMINANCE8_ALPHA8_EXT:            return sizeof(unsigned char) * 2;
      case GL_LUMINANCE_ALPHA32F_EXT:           return sizeof(float) * 2;
      case GL_LUMINANCE_ALPHA16F_EXT:           return sizeof(unsigned short) * 2;
      case GL_RGB8_OES:                         return sizeof(unsigned char) * 3;
      case GL_RGB565:                           return sizeof(unsigned short);
      case GL_RGB32F_EXT:                       return sizeof(float) * 3;
      case GL_RGB16F_EXT:                       return sizeof(unsigned short) * 3;
      case GL_RGBA8_OES:                        return sizeof(unsigned char) * 4;
      case GL_RGBA4:                            return sizeof(unsigned short);
      case GL_RGB5_A1:                          return sizeof(unsigned short);
      case GL_RGBA32F_EXT:                      return sizeof(float) * 4;
      case GL_RGBA16F_EXT:                      return sizeof(unsigned short) * 4;
      case GL_BGRA8_EXT:                        return sizeof(unsigned char) * 4;
      case GL_SRGB8_ALPHA8:                     return sizeof(unsigned char) * 4;
      case GL_RGB10_A2:                         return sizeof(unsigned char) * 4;
      case GL_RG8:                              return sizeof(unsigned char) * 2;
      case GL_R8:                               return sizeof(unsigned char);
      case GL_BGRA4_ANGLEX:                     return sizeof(unsigned short);
      case GL_BGR5_A1_ANGLEX:                   return sizeof(unsigned short);
      default:
        UNIMPLEMENTED();   // TODO: Remaining ES3 formats
        UNREACHABLE();
    }

    return 0;
}

bool IsCubemapTextureTarget(GLenum target)
{
    return (target >= GL_TEXTURE_CUBE_MAP_POSITIVE_X && target <= GL_TEXTURE_CUBE_MAP_NEGATIVE_Z);
}

bool IsInternalTextureTarget(GLenum target)
{
    return target == GL_TEXTURE_2D || IsCubemapTextureTarget(target);
}

bool IsTriangleMode(GLenum drawMode)
{
    switch (drawMode)
    {
      case GL_TRIANGLES:
      case GL_TRIANGLE_FAN:
      case GL_TRIANGLE_STRIP:
        return true;
      case GL_POINTS:
      case GL_LINES:
      case GL_LINE_LOOP:
      case GL_LINE_STRIP:
        return false;
      default: UNREACHABLE();
    }

    return false;
}

}

std::string getTempPath()
{
    char path[MAX_PATH];
    DWORD pathLen = GetTempPathA(sizeof(path) / sizeof(path[0]), path);
    if (pathLen == 0)
    {
        UNREACHABLE();
        return std::string();
    }

    UINT unique = GetTempFileNameA(path, "sh", 0, path);
    if (unique == 0)
    {
        UNREACHABLE();
        return std::string();
    }
    
    return path;
}

void writeFile(const char* path, const void* content, size_t size)
{
    FILE* file = fopen(path, "w");
    if (!file)
    {
        UNREACHABLE();
        return;
    }

    fwrite(content, sizeof(char), size, file);
    fclose(file);
}

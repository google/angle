//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Helper structure describing a single vertex attribute
//

#ifndef LIBGLESV2_VERTEXATTRIBUTE_H_
#define LIBGLESV2_VERTEXATTRIBUTE_H_

#include "common/RefCountObject.h"

namespace gl
{
class Buffer;

class VertexAttribute
{
  public:
    VertexAttribute() : mType(GL_FLOAT), mSize(0), mNormalized(false), mPureInteger(false),
                        mStride(0), mPointer(NULL), mArrayEnabled(false), mDivisor(0)
    {
        mCurrentValue.FloatValues[0] = 0.0f;
        mCurrentValue.FloatValues[1] = 0.0f;
        mCurrentValue.FloatValues[2] = 0.0f;
        mCurrentValue.FloatValues[3] = 1.0f;
        mCurrentValue.Type = GL_FLOAT;
    }

    int typeSize() const
    {
        switch (mType)
        {
          case GL_BYTE:                        return mSize * sizeof(GLbyte);
          case GL_UNSIGNED_BYTE:               return mSize * sizeof(GLubyte);
          case GL_SHORT:                       return mSize * sizeof(GLshort);
          case GL_UNSIGNED_SHORT:              return mSize * sizeof(GLushort);
          case GL_INT:                         return mSize * sizeof(GLint);
          case GL_UNSIGNED_INT:                return mSize * sizeof(GLuint);
          case GL_INT_2_10_10_10_REV:          return 4;
          case GL_UNSIGNED_INT_2_10_10_10_REV: return 4;
          case GL_FIXED:                       return mSize * sizeof(GLfixed);
          case GL_HALF_FLOAT:                  return mSize * sizeof(GLhalf);
          case GL_FLOAT:                       return mSize * sizeof(GLfloat);
          default: UNREACHABLE();              return mSize * sizeof(GLfloat);
        }
    }

    GLsizei stride() const
    {
        return mStride ? mStride : typeSize();
    }

    // From glVertexAttribPointer
    GLenum mType;
    GLint mSize;
    bool mNormalized;
    bool mPureInteger;
    GLsizei mStride;   // 0 means natural stride

    union
    {
        const void *mPointer;
        intptr_t mOffset;
    };

    BindingPointer<Buffer> mBoundBuffer;   // Captured when glVertexAttribPointer is called.
    bool mArrayEnabled;   // From glEnable/DisableVertexAttribArray

    struct CurrentValueData
    {
        union
        {
            GLfloat FloatValues[4];
            GLint IntValues[4];
            GLuint UnsignedIntValues[4];
        };
        GLenum Type;
    };
    CurrentValueData mCurrentValue; // From glVertexAttrib

    unsigned int mDivisor;
};

}

#endif // LIBGLESV2_VERTEXATTRIBUTE_H_

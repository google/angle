//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Error.h: Defines the egl::Error and gl::Error classes which encapsulate API errors
// and optional error messages.

#ifndef LIBANGLE_ERROR_H_
#define LIBANGLE_ERROR_H_

#include "angle_gl.h"
#include <EGL/egl.h>

#include <string>

namespace gl
{

class Error
{
  public:
    explicit Error(GLenum errorCode);
    Error(GLenum errorCode, const char *msg, ...);
    Error(const Error &other);
    ~Error();
    Error &operator=(const Error &other);

    GLenum getCode() const { return mCode; }
    bool isError() const { return (mCode != GL_NO_ERROR); }

    const std::string &getMessage() const;

  private:
    void createMessageString() const;

    GLenum mCode;
    mutable std::string *mMessage;
};

}

namespace egl
{

class Error
{
  public:
    explicit Error(EGLint errorCode);
    Error(EGLint errorCode, const char *msg, ...);
    Error(EGLint errorCode, EGLint id, const char *msg, ...);
    Error(const Error &other);
    ~Error();
    Error &operator=(const Error &other);

    EGLint getCode() const { return mCode; }
    EGLint getID() const { return mID; }
    bool isError() const { return (mCode != EGL_SUCCESS); }

    const std::string &getMessage() const;

  private:
    void createMessageString() const;

    EGLint mCode;
    EGLint mID;
    mutable std::string *mMessage;
};

}

#endif // LIBANGLE_ERROR_H_

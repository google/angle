//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Error.cpp: Implements the egl::Error and gl::Error classes which encapsulate API errors
// and optional error messages.

#include "libANGLE/Error.h"

#include "common/angleutils.h"

#include <cstdarg>

namespace gl
{

Error::Error(GLenum errorCode)
    : mCode(errorCode),
      mMessage()
{
}

Error::Error(GLenum errorCode, const char *msg, ...)
    : mCode(errorCode),
      mMessage()
{
    va_list vararg;
    va_start(vararg, msg);
    mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(const Error &other)
    : mCode(other.mCode),
      mMessage(other.mMessage)
{
}

Error &Error::operator=(const Error &other)
{
    mCode = other.mCode;
    mMessage = other.mMessage;
    return *this;
}

}

namespace egl
{

Error::Error(EGLint errorCode)
    : mCode(errorCode),
      mMessage()
{
}

Error::Error(EGLint errorCode, const char *msg, ...)
    : mCode(errorCode),
    mMessage()
{
    va_list vararg;
    va_start(vararg, msg);
    mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(const Error &other)
    : mCode(other.mCode),
    mMessage(other.mMessage)
{
}

Error &Error::operator=(const Error &other)
{
    mCode = other.mCode;
    mMessage = other.mMessage;
    return *this;
}

}

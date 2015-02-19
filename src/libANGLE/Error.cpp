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
      mMessage(NULL)
{
}

Error::Error(GLenum errorCode, const char *msg, ...)
    : mCode(errorCode),
      mMessage(NULL)
{
    va_list vararg;
    va_start(vararg, msg);
    createMessageString();
    *mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(const Error &other)
    : mCode(other.mCode),
      mMessage(NULL)
{
    if (other.mMessage != NULL)
    {
        createMessageString();
        *mMessage = *(other.mMessage);
    }
}

Error::~Error()
{
    SafeDelete(mMessage);
}

Error &Error::operator=(const Error &other)
{
    mCode = other.mCode;

    if (other.mMessage != NULL)
    {
        createMessageString();
        *mMessage = *(other.mMessage);
    }
    else
    {
        SafeDelete(mMessage);
    }

    return *this;
}

void Error::createMessageString() const
{
    if (mMessage == nullptr)
    {
        mMessage = new std::string();
    }
}

const std::string &Error::getMessage() const
{
    createMessageString();
    return *mMessage;
}

}

namespace egl
{

Error::Error(EGLint errorCode)
    : mCode(errorCode),
      mID(0),
      mMessage(nullptr)
{
}

Error::Error(EGLint errorCode, const char *msg, ...)
    : mCode(errorCode),
      mID(0),
      mMessage(nullptr)
{
    va_list vararg;
    va_start(vararg, msg);
    createMessageString();
    *mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(EGLint errorCode, EGLint id, const char *msg, ...)
    : mCode(errorCode),
      mID(id),
      mMessage(nullptr)
{
    va_list vararg;
    va_start(vararg, msg);
    createMessageString();
    *mMessage = FormatString(msg, vararg);
    va_end(vararg);
}

Error::Error(const Error &other)
    : mCode(other.mCode),
      mID(other.mID),
      mMessage(nullptr)
{
    if (other.mMessage != nullptr)
    {
        createMessageString();
        *mMessage = *(other.mMessage);
    }
}

Error::~Error()
{
    SafeDelete(mMessage);
}

Error &Error::operator=(const Error &other)
{
    mCode = other.mCode;
    mID = other.mID;

    if (other.mMessage != nullptr)
    {
        createMessageString();
        *mMessage = *(other.mMessage);
    }
    else
    {
        SafeDelete(mMessage);
    }

    return *this;
}

void Error::createMessageString() const
{
    if (mMessage == nullptr)
    {
        mMessage = new std::string();
    }
}

const std::string &Error::getMessage() const
{
    createMessageString();
    return *mMessage;
}

}

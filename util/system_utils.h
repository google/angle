//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// system_utils.h: declaration of OS-specific utility functions

#ifndef UTIL_SYSTEM_UTILS_H_
#define UTIL_SYSTEM_UTILS_H_

#include <string>

#include "common/angleutils.h"

namespace angle
{

std::string GetExecutablePath();
std::string GetExecutableDirectory();
std::string GetSharedLibraryExtension();

// Cross platform equivalent of the Windows Sleep function
void Sleep(unsigned int milliseconds);

void SetLowPriorityProcess();

// Write a debug message, either to a standard output or Debug window.
void WriteDebugMessage(const char *format, ...);

class Library : angle::NonCopyable
{
  public:
    virtual ~Library() {}
    virtual void *getSymbol(const std::string &symbolName) = 0;
};

Library *loadLibrary(const std::string &libraryName);

} // namespace angle

#endif  // UTIL_SYSTEM_UTILS_H_

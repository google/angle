//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Win32_system_utils.cpp: Implementation of OS-specific functions for Windows

#include "system_utils.h"

#include <windows.h>
#include <array>

namespace angle
{

std::string GetExecutablePath()
{
    std::array<char, MAX_PATH> executableFileBuf;
    DWORD executablePathLen = GetModuleFileNameA(NULL, executableFileBuf.data(), executableFileBuf.size());
    return (executablePathLen > 0 ? std::string(executableFileBuf.data()) : "");
}

std::string GetExecutableDirectory()
{
    std::string executablePath = GetExecutablePath();
    size_t lastPathSepLoc = executablePath.find_last_of("\\/");
    return (lastPathSepLoc != std::string::npos) ? executablePath.substr(0, lastPathSepLoc) : "";
}

void Sleep(unsigned int milliseconds)
{
    ::Sleep(static_cast<DWORD>(milliseconds));
}

void SetLowPriorityProcess()
{
    SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
}

} // namespace angle

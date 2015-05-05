//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Linux_path_utils.cpp: Implementation of OS-specific path functions for Linux

#include "path_utils.h"
#include <array>
#include <sys/stat.h>
#include <unistd.h>

std::string GetExecutablePath()
{
    struct stat sb;
    if (lstat("/proc/self/exe", &sb) == -1)
    {
        return "";
    }

    char *path = static_cast<char*>(malloc(sb.st_size + 1));
    if (!path)
    {
        return "";
    }

    ssize_t result = readlink("/proc/self/exe", path, sb.st_size);
    if (result != sb.st_size)
    {
        free(path);
        return "";
    }

    path[sb.st_size] = '\0';

    std::string strPath(path);
    free(path);
    return strPath;
}

std::string GetExecutableDirectory()
{
    std::string executablePath = GetExecutablePath();
    size_t lastPathSepLoc = executablePath.find_last_of("/");
    return (lastPathSepLoc != std::string::npos) ? executablePath.substr(0, lastPathSepLoc) : "";
}

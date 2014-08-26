//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UTIL_SHARED_UTILS_H
#define UTIL_SHARED_UTILS_H

#define SHADER_SOURCE(...) #__VA_ARGS__

// A macro to disallow the copy constructor and operator= functions
// This must be used in the private: declarations for a class
#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
    TypeName(const TypeName&);             \
    void operator=(const TypeName&)

#endif // UTIL_SHARED_UTILS_H

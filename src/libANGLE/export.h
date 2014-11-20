//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBANGLE_EXPORT_H_
#define LIBANGLE_EXPORT_H_

#include "common/platform.h"

#if defined(LIBANGLE_STATIC)
#   define ANGLE_EXPORT
#else
#   if defined(_WIN32)
#       if defined(LIBANGLE_IMPLEMENTATION)
#           define ANGLE_EXPORT __declspec(dllexport)
#      else
#         define ANGLE_EXPORT __declspec(dllimport)
#      endif
#   elif defined(__GNUC__)
#       if defined(LIBANGLE_IMPLEMENTATION)
#           define ANGLE_EXPORT __attribute__((visibility ("default")))
#       else
#           define ANGLE_EXPORT
#       endif
#   else
#       define ANGLE_EXPORT
#   endif
#endif

#endif // LIBANGLE_EXPORT_H_

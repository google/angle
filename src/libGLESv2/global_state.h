//
// Copyright(c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// global_state.h : Defines functions for querying the thread-local GL and EGL state.

#ifndef LIBGLESV2_GLOBALSTATE_H_
#define LIBGLESV2_GLOBALSTATE_H_

#include "libANGLE/features.h"

#include <mutex>

namespace gl
{
class Context;

Context *GetGlobalContext();
Context *GetValidGlobalContext();

}  // namespace gl

namespace egl
{
class Debug;
class Thread;

Thread *GetCurrentThread();
Debug *GetDebug();

}  // namespace egl

#if ANGLE_FORCE_THREAD_SAFETY == ANGLE_ENABLED
namespace angle
{
std::mutex &GetGlobalMutex();
}  // namespace angle

#define ANGLE_SCOPED_GLOBAL_LOCK() \
    std::lock_guard<std::mutex> globalMutexLock(angle::GetGlobalMutex())
#else
#define ANGLE_SCOPED_GLOBAL_LOCK()
#endif

#endif  // LIBGLESV2_GLOBALSTATE_H_

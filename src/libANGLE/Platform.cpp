//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Platform.cpp: Implementation methods for angle::Platform.

#include <platform/Platform.h>

#include "common/debug.h"
#include "platform/Platform.h"

namespace angle
{

namespace
{
Platform *currentPlatform = nullptr;
}

// static
ANGLE_EXPORT Platform *Platform::current()
{
    return currentPlatform;
}

// static
ANGLE_EXPORT void Platform::initialize(Platform *platformImpl)
{
    ASSERT(platformImpl != nullptr);
    currentPlatform = platformImpl;
}

// static
ANGLE_EXPORT void Platform::shutdown()
{
    currentPlatform = nullptr;
}

}

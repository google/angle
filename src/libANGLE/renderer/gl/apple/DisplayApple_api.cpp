//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DisplayApple_api.cpp:
//    Chooses CGL or EAGL either at compile time or runtime based on the platform.
//

#include "libANGLE/renderer/gl/apple/DisplayApple_api.h"

#include "gpu_info_util/SystemInfo.h"
#include "libANGLE/renderer/DisplayImpl.h"

#if defined(ANGLE_ENABLE_CGL)
#    include "libANGLE/renderer/gl/cgl/DisplayCGL.h"
#endif
#if defined(ANGLE_ENABLE_EAGL)
#    include "libANGLE/renderer/gl/eagl/DisplayEAGL.h"
#endif

namespace rx
{

DisplayImpl *CreateDisplayCGLOrEAGL(const egl::DisplayState &state)
{
#if defined(ANGLE_ENABLE_EAGL) && defined(ANGLE_ENABLE_CGL)
    angle::SystemInfo info;
    if (!angle::GetSystemInfo(&info))
    {
        return nullptr;
    }

    if (info.needsEAGLOnMac)
    {
        return new rx::DisplayEAGL(state);
    }
    else
    {
        return new rx::DisplayCGL(state);
    }
#elif defined(ANGLE_ENABLE_CGL)
    return new rx::DisplayCGL(state);
#elif defined(ANGLE_ENABLE_EAGL)
    return new rx::DisplayEAGL(state);
#endif
}

}  // namespace rx

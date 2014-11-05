//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Trim11.h: Trim support utility class.

#ifndef LIBGLESV2_TRIM11_H_
#define LIBGLESV2_TRIM11_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"
#include "libGLESv2/Error.h"

#if !defined(ANGLE_ENABLE_WINDOWS_STORE)
typedef void* EventRegistrationToken;
#else
#include <EventToken.h>
#endif

namespace rx
{
class Renderer11;

class Trim11
{
  public:
    explicit Trim11(Renderer11 *renderer);
    ~Trim11();

  private:
    Renderer11 *mRenderer;
    EventRegistrationToken mApplicationSuspendedEventToken;

    void trim();
    bool registerForRendererTrimRequest();
    void unregisterForRendererTrimRequest();

    DISALLOW_COPY_AND_ASSIGN(Trim11);
};

}

#endif   // LIBGLESV2_TRIM11_H_

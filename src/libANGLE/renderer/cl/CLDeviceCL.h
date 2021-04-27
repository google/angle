//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CLDeviceCL.h:
//    Defines the class interface for CLDeviceCL, implementing CLDeviceImpl.
//

#ifndef LIBANGLE_RENDERER_CL_CLDEVICECL_H_
#define LIBANGLE_RENDERER_CL_CLDEVICECL_H_

#include "libANGLE/renderer/CLDeviceImpl.h"

namespace rx
{

class CLDeviceCL : public CLDeviceImpl
{
  public:
    CLDeviceCL();
    ~CLDeviceCL() override;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_CL_CLDEVICECL_H_

//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererMtl.mm:
//    Implements the class methods for RendererMtl.
//

#include "libANGLE/renderer/metal/RendererMtl.h"

#include "libANGLE/renderer/metal/mtl_common.h"

namespace rx
{
RendererMtl::RendererMtl() {}

RendererMtl::~RendererMtl() {}

angle::Result RendererMtl::initialize(egl::Display *display)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
void RendererMtl::onDestroy()
{
    UNIMPLEMENTED();
}

std::string RendererMtl::getVendorString() const
{
    std::string vendorString = "Google Inc.";
    UNIMPLEMENTED();

    return vendorString;
}

std::string RendererMtl::getRendererDescription() const
{
    std::string desc = "Metal Renderer";
    UNIMPLEMENTED();

    return desc;
}

const gl::Limitations &RendererMtl::getNativeLimitations() const
{
    UNIMPLEMENTED();
    return mNativeLimitations;
}

}

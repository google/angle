//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "libANGLE/renderer/wgpu/wgpu_format_utils.h"

namespace rx
{
namespace webgpu
{
Format::Format()
    : mIntendedFormatID(angle::FormatID::NONE),
      mIntendedGLFormat(GL_NONE),
      mActualImageFormatID(angle::FormatID::NONE),
      mActualBufferFormatID(angle::FormatID::NONE),
      mImageInitializerFunction(nullptr),
      mIsRenderable(false)
{}
void Format::initImageFallback(const ImageFormatInitInfo *info, int numInfo)
{
    UNIMPLEMENTED();
}

void Format::initBufferFallback(const BufferFormatInitInfo *fallbackInfo, int numInfo)
{
    UNIMPLEMENTED();
}
}  // namespace webgpu
}  // namespace rx

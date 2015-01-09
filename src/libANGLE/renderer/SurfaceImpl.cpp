//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SurfaceImpl.cpp: Implementation of Surface stub method class

#include "libANGLE/renderer/SurfaceImpl.h"

#include "libANGLE/Config.h"

namespace rx
{

SurfaceImpl::SurfaceImpl(egl::Display *display, const egl::Config *config, EGLint width, EGLint height,
                         EGLint fixedSize, EGLint postSubBufferSupported, EGLenum textureFormat,
                         EGLenum textureType, EGLClientBuffer shareHandle)
    : mDisplay(display),
      mConfig(config),
      mWidth(width),
      mHeight(height),
      mFixedSize(fixedSize),
      mSwapInterval(-1),
      mPostSubBufferSupported(postSubBufferSupported),
      mTextureFormat(textureFormat),
      mTextureTarget(textureType),
      mShareHandle(shareHandle)
{
}

SurfaceImpl::~SurfaceImpl()
{
}

EGLenum SurfaceImpl::getFormat() const
{
    return mConfig->renderTargetFormat;
}

}

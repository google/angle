//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RenderBufferMtl.mm:
//    Implements the class methods for RenderBufferMtl.
//

#include "libANGLE/renderer/metal/RenderBufferMtl.h"

#include "libANGLE/renderer/metal/ContextMtl.h"

namespace rx
{

RenderbufferMtl::RenderbufferMtl(const gl::RenderbufferState &state) : RenderbufferImpl(state) {}

RenderbufferMtl::~RenderbufferMtl() {}

void RenderbufferMtl::onDestroy(const gl::Context *context)
{
    UNIMPLEMENTED();
}

angle::Result RenderbufferMtl::setStorage(const gl::Context *context,
                                          GLenum internalformat,
                                          size_t width,
                                          size_t height)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result RenderbufferMtl::setStorageMultisample(const gl::Context *context,
                                                     size_t samples,
                                                     GLenum internalformat,
                                                     size_t width,
                                                     size_t height)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result RenderbufferMtl::setStorageEGLImageTarget(const gl::Context *context,
                                                        egl::Image *image)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result RenderbufferMtl::getAttachmentRenderTarget(const gl::Context *context,
                                                         GLenum binding,
                                                         const gl::ImageIndex &imageIndex,
                                                         GLsizei samples,
                                                         FramebufferAttachmentRenderTarget **rtOut)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result RenderbufferMtl::initializeContents(const gl::Context *context,
                                                  const gl::ImageIndex &imageIndex)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
}
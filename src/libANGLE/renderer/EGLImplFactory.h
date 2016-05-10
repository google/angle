//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLImplFactory.h:
//   Factory interface for EGL Impl objects.
//

#ifndef LIBANGLE_RENDERER_EGLIMPLFACTORY_H_
#define LIBANGLE_RENDERER_EGLIMPLFACTORY_H_

#include "libANGLE/Stream.h"

namespace egl
{
class AttributeMap;
struct Config;
class ImageSibling;
}

namespace gl
{
class Context;
}

namespace rx
{
class ImageImpl;
class SurfaceImpl;

class EGLImplFactory : angle::NonCopyable
{
  public:
    EGLImplFactory() {}
    virtual ~EGLImplFactory() {}

    virtual SurfaceImpl *createWindowSurface(const egl::Config *configuration,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs) = 0;
    virtual SurfaceImpl *createPbufferSurface(const egl::Config *configuration,
                                              const egl::AttributeMap &attribs) = 0;
    virtual SurfaceImpl *createPbufferFromClientBuffer(const egl::Config *configuration,
                                                       EGLClientBuffer shareHandle,
                                                       const egl::AttributeMap &attribs) = 0;
    virtual SurfaceImpl *createPixmapSurface(const egl::Config *configuration,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs) = 0;

    virtual ImageImpl *createImage(EGLenum target,
                                   egl::ImageSibling *buffer,
                                   const egl::AttributeMap &attribs) = 0;

    virtual gl::Context *createContext(const egl::Config *config,
                                       const gl::Context *shareContext,
                                       const egl::AttributeMap &attribs) = 0;

    virtual StreamProducerImpl *createStreamProducerD3DTextureNV12(
        egl::Stream::ConsumerType consumerType,
        const egl::AttributeMap &attribs) = 0;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_EGLIMPLFACTORY_H_

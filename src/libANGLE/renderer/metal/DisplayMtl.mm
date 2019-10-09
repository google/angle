//
// Copyright (c) 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayMtl.mm: Metal implementation of DisplayImpl

#include "libANGLE/renderer/metal/DisplayMtl.h"

#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/metal/ContextMtl.h"
#include "libANGLE/renderer/metal/RendererMtl.h"
#include "libANGLE/renderer/metal/SurfaceMtl.h"

#include "EGL/eglext.h"

namespace rx
{

bool DisplayMtl::IsMetalAvailable()
{
    // We only support macos 10.13+ and iOS 11 for now. Since they are requirements for Metal 2.0.
    if (@available(macOS 10.13, iOS 11, *))
    {
        return true;
    }
    return false;
}

DisplayMtl::DisplayMtl(const egl::DisplayState &state)
    : DisplayImpl(state), mRenderer(new RendererMtl())
{}

DisplayMtl::~DisplayMtl() {}

egl::Error DisplayMtl::initialize(egl::Display *display)
{
    ASSERT(IsMetalAvailable());

    angle::Result result = mRenderer->initialize(display);
    if (result != angle::Result::Continue)
    {
        return egl::EglNotInitialized();
    }
    return egl::NoError();
}

void DisplayMtl::terminate()
{
    mRenderer->onDestroy();
}

bool DisplayMtl::testDeviceLost()
{
    return false;
}

egl::Error DisplayMtl::restoreLostDevice(const egl::Display *display)
{
    return egl::NoError();
}

std::string DisplayMtl::getVendorString() const
{
    return mRenderer->getVendorString();
}

DeviceImpl *DisplayMtl::createDevice()
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayMtl::waitClient(const gl::Context *context)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error DisplayMtl::waitNative(const gl::Context *context, EGLint engine)
{
    UNIMPLEMENTED();
    return egl::EglBadAccess();
}

SurfaceImpl *DisplayMtl::createWindowSurface(const egl::SurfaceState &state,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    EGLint width  = attribs.getAsInt(EGL_WIDTH, 0);
    EGLint height = attribs.getAsInt(EGL_HEIGHT, 0);

    return new SurfaceMtl(state, window, width, height);
}

SurfaceImpl *DisplayMtl::createPbufferSurface(const egl::SurfaceState &state,
                                              const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return static_cast<SurfaceImpl *>(0);
}

SurfaceImpl *DisplayMtl::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                       EGLenum buftype,
                                                       EGLClientBuffer clientBuffer,
                                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return static_cast<SurfaceImpl *>(0);
}

SurfaceImpl *DisplayMtl::createPixmapSurface(const egl::SurfaceState &state,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return static_cast<SurfaceImpl *>(0);
}

ImageImpl *DisplayMtl::createImage(const egl::ImageState &state,
                                   const gl::Context *context,
                                   EGLenum target,
                                   const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

rx::ContextImpl *DisplayMtl::createContext(const gl::State &state,
                                           gl::ErrorSet *errorSet,
                                           const egl::Config *configuration,
                                           const gl::Context *shareContext,
                                           const egl::AttributeMap &attribs)
{
    return new ContextMtl(state, errorSet, mRenderer.get());
}

StreamProducerImpl *DisplayMtl::createStreamProducerD3DTexture(
    egl::Stream::ConsumerType consumerType,
    const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

gl::Version DisplayMtl::getMaxSupportedESVersion() const
{
    UNIMPLEMENTED();
    return gl::Version(1, 0);
}

gl::Version DisplayMtl::getMaxConformantESVersion() const
{
    return std::min(getMaxSupportedESVersion(), gl::Version(2, 0));
}

EGLSyncImpl *DisplayMtl::createSync(const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayMtl::makeCurrent(egl::Surface *drawSurface,
                                   egl::Surface *readSurface,
                                   gl::Context *context)
{
    if (!context)
    {
        return egl::NoError();
    }

    return egl::NoError();
}

void DisplayMtl::generateExtensions(egl::DisplayExtensions *outExtensions) const {}

void DisplayMtl::generateCaps(egl::Caps *outCaps) const {}

void DisplayMtl::populateFeatureList(angle::FeatureList *features) {}

egl::ConfigSet DisplayMtl::generateConfigs()
{
    UNIMPLEMENTED();
    egl::ConfigSet configs;

    return configs;
}

bool DisplayMtl::isValidNativeWindow(EGLNativeWindowType window) const
{
    UNIMPLEMENTED();
    return false;
}

}  // namespace rx
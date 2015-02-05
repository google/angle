//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayD3D.cpp: D3D implementation of egl::Display

#include "libANGLE/renderer/d3d/DisplayD3D.h"

#include "libANGLE/Context.h"
#include "libANGLE/Config.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/SurfaceD3D.h"
#include "libANGLE/renderer/d3d/SwapChainD3D.h"

#include <EGL/eglext.h>

#if defined (ANGLE_ENABLE_D3D9)
#   include "libANGLE/renderer/d3d/d3d9/Renderer9.h"
#endif // ANGLE_ENABLE_D3D9

#if defined (ANGLE_ENABLE_D3D11)
#   include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#endif // ANGLE_ENABLE_D3D11

#if defined (ANGLE_TEST_CONFIG)
#   define ANGLE_DEFAULT_D3D11 1
#endif

#if !defined(ANGLE_DEFAULT_D3D11)
// Enables use of the Direct3D 11 API for a default display, when available
#   define ANGLE_DEFAULT_D3D11 0
#endif

namespace rx
{

typedef RendererD3D *(*CreateRendererD3DFunction)(egl::Display*, EGLNativeDisplayType, const egl::AttributeMap &);

template <typename RendererType>
static RendererD3D *CreateTypedRendererD3D(egl::Display *display, EGLNativeDisplayType nativeDisplay, const egl::AttributeMap &attributes)
{
    return new RendererType(display, nativeDisplay, attributes);
}

egl::Error CreateRendererD3D(egl::Display *display, EGLNativeDisplayType nativeDisplay, const egl::AttributeMap &attribMap, RendererD3D **outRenderer)
{
    ASSERT(outRenderer != nullptr);

    std::vector<CreateRendererD3DFunction> rendererCreationFunctions;

    EGLint requestedDisplayType = attribMap.get(EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE);

#   if defined(ANGLE_ENABLE_D3D11)
        if (nativeDisplay == EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE ||
            nativeDisplay == EGL_D3D11_ONLY_DISPLAY_ANGLE ||
            requestedDisplayType == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
        {
            rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer11>);
        }
#   endif

#   if defined(ANGLE_ENABLE_D3D9)
        if (nativeDisplay == EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE ||
            requestedDisplayType == EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE)
        {
            rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer9>);
        }
#   endif

    if (nativeDisplay != EGL_D3D11_ELSE_D3D9_DISPLAY_ANGLE &&
        nativeDisplay != EGL_D3D11_ONLY_DISPLAY_ANGLE &&
        requestedDisplayType == EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE)
    {
        // The default display is requested, try the D3D9 and D3D11 renderers, order them using
        // the definition of ANGLE_DEFAULT_D3D11
#       if ANGLE_DEFAULT_D3D11
#           if defined(ANGLE_ENABLE_D3D11)
                rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer11>);
#           endif
#           if defined(ANGLE_ENABLE_D3D9)
                rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer9>);
#           endif
#       else
#           if defined(ANGLE_ENABLE_D3D9)
                rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer9>);
#           endif
#           if defined(ANGLE_ENABLE_D3D11)
                rendererCreationFunctions.push_back(CreateTypedRendererD3D<Renderer11>);
#           endif
#       endif
    }

    EGLint result = EGL_NOT_INITIALIZED;
    for (size_t i = 0; i < rendererCreationFunctions.size(); i++)
    {
        RendererD3D *renderer = rendererCreationFunctions[i](display, nativeDisplay, attribMap);
        result = renderer->initialize();
        if (result == EGL_SUCCESS)
        {
            *outRenderer = renderer;
            break;
        }
        else
        {
            // Failed to create the renderer, try the next
            SafeDelete(renderer);
        }
    }

    return egl::Error(result);
}

DisplayD3D::DisplayD3D()
    : mRenderer(nullptr)
{
}

SurfaceImpl *DisplayD3D::createWindowSurface(egl::Display *display, const egl::Config *config,
                                             EGLNativeWindowType window, EGLint fixedSize,
                                             EGLint width, EGLint height, EGLint postSubBufferSupported)
{
    ASSERT(mRenderer != nullptr);

    return SurfaceD3D::createFromWindow(mRenderer, display, config, window, fixedSize,
                                        width, height, postSubBufferSupported);
}

SurfaceImpl *DisplayD3D::createOffscreenSurface(egl::Display *display, const egl::Config *config,
                                                EGLClientBuffer shareHandle, EGLint width, EGLint height,
                                                EGLenum textureFormat, EGLenum textureTarget)
{
    ASSERT(mRenderer != nullptr);

    return SurfaceD3D::createOffscreen(mRenderer, display, config, shareHandle,
                                       width, height, textureFormat, textureTarget);
}

egl::Error DisplayD3D::createContext(const egl::Config *config, const gl::Context *shareContext, const egl::AttributeMap &attribs,
                                     gl::Context **outContext)
{
    ASSERT(mRenderer != nullptr);

    EGLint clientVersion = attribs.get(EGL_CONTEXT_CLIENT_VERSION, 1);
    bool notifyResets = (attribs.get(EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT, EGL_NO_RESET_NOTIFICATION_EXT) == EGL_LOSE_CONTEXT_ON_RESET_EXT);
    bool robustAccess = (attribs.get(EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT, EGL_FALSE) == EGL_TRUE);

    *outContext = new gl::Context(config, clientVersion, shareContext, mRenderer, notifyResets, robustAccess);
    return egl::Error(EGL_SUCCESS);
}

egl::Error DisplayD3D::makeCurrent(egl::Surface *drawSurface, egl::Surface *readSurface, gl::Context *context)
{
    return egl::Error(EGL_SUCCESS);
}

egl::Error DisplayD3D::initialize(egl::Display *display, EGLNativeDisplayType nativeDisplay, const egl::AttributeMap &attribMap)
{
    ASSERT(mRenderer == nullptr);
    return CreateRendererD3D(display, nativeDisplay, attribMap, &mRenderer);
}

void DisplayD3D::terminate()
{
    SafeDelete(mRenderer);
}

egl::ConfigSet DisplayD3D::generateConfigs() const
{
    ASSERT(mRenderer != nullptr);
    return mRenderer->generateConfigs();
}

bool DisplayD3D::isDeviceLost() const
{
    ASSERT(mRenderer != nullptr);
    return mRenderer->isDeviceLost();
}

bool DisplayD3D::testDeviceLost()
{
    ASSERT(mRenderer != nullptr);
    return mRenderer->testDeviceLost();
}

egl::Error DisplayD3D::restoreLostDevice()
{
    // Release surface resources to make the Reset() succeed
    for (auto &surface : mSurfaceSet)
    {
        if (surface->getBoundTexture())
        {
            surface->releaseTexImage(EGL_BACK_BUFFER);
        }
        SurfaceD3D *surfaceD3D = GetImplAs<SurfaceD3D>(surface);
        surfaceD3D->releaseSwapChain();
    }

    if (!mRenderer->resetDevice())
    {
        return egl::Error(EGL_BAD_ALLOC);
    }

    // Restore any surfaces that may have been lost
    for (const auto &surface : mSurfaceSet)
    {
        SurfaceD3D *surfaceD3D = GetImplAs<SurfaceD3D>(surface);

        egl::Error error = surfaceD3D->resetSwapChain();
        if (error.isError())
        {
            return error;
        }
    }

    return egl::Error(EGL_SUCCESS);
}

bool DisplayD3D::isValidNativeWindow(EGLNativeWindowType window) const
{
    return NativeWindow::isValidNativeWindow(window);
}

void DisplayD3D::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    outExtensions->createContextRobustness = true;

    // ANGLE-specific extensions
    if (mRenderer->getShareHandleSupport())
    {
        outExtensions->d3dShareHandleClientBuffer = true;
        outExtensions->surfaceD3DTexture2DShareHandle = true;
    }

    outExtensions->querySurfacePointer = true;
    outExtensions->windowFixedSize = true;

    if (mRenderer->getPostSubBufferSupport())
    {
        outExtensions->postSubBuffer = true;
    }

    outExtensions->createContext = true;
}

std::string DisplayD3D::getVendorString() const
{
    std::string vendorString = "Google Inc.";
    if (mRenderer)
    {
        vendorString += " " + mRenderer->getVendorString();
    }

    return vendorString;
}

void DisplayD3D::generateCaps(egl::Caps *outCaps) const
{
    // Display must be initialized to generate caps
    ASSERT(mRenderer != nullptr);

    outCaps->textureNPOT = mRenderer->getRendererExtensions().textureNPOT;
}

}

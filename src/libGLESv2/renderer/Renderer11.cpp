//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.cpp: Implements a back-end specific class for the D3D11 renderer.

#include "common/debug.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/Renderer11.h"

#include "libEGL/Config.h"
#include "libEGL/Display.h"

namespace rx
{

Renderer11::Renderer11(egl::Display *display, HDC hDc) : Renderer(display), mDc(hDc)
{
    mD3d11Module = NULL;
    mDxgiModule = NULL;

    mDevice = NULL;
    mDeviceContext = NULL;
}

Renderer11::~Renderer11()
{
    releaseDeviceResources();

    if (mDeviceContext)
    {
        mDeviceContext->Release();
        mDeviceContext = NULL;
    }

    if (mDevice)
    {
        mDevice->Release();
        mDevice = NULL;
    }

    if (mD3d11Module)
    {
        FreeLibrary(mD3d11Module);
        mD3d11Module = NULL;
    }

    if (mDxgiModule)
    {
        FreeLibrary(mDxgiModule);
        mDxgiModule = NULL;
    }
}

EGLint Renderer11::initialize()
{
    mDxgiModule = LoadLibrary(TEXT("dxgi.dll"));
    mD3d11Module = LoadLibrary(TEXT("d3d11.dll"));

    if (mD3d11Module == NULL || mDxgiModule == NULL)
    {
        ERR("Could not load D3D11 or DXGI library - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    PFN_D3D11_CREATE_DEVICE D3D11CreateDevice = (PFN_D3D11_CREATE_DEVICE)GetProcAddress(mD3d11Module, "D3D11CreateDevice");

    if (D3D11CreateDevice == NULL)
    {
        ERR("Could not retrieve D3D11CreateDevice address - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }
    
    D3D_FEATURE_LEVEL featureLevel[] = 
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
        
    HRESULT result = D3D11CreateDevice(NULL,
                                       D3D_DRIVER_TYPE_HARDWARE,
                                       NULL,
                                       0,   // D3D11_CREATE_DEVICE_DEBUG
                                       featureLevel,
                                       sizeof(featureLevel)/sizeof(featureLevel[0]),
                                       D3D11_SDK_VERSION,
                                       &mDevice,
                                       &mFeatureLevel,
                                       &mDeviceContext);
    
    if (!mDevice || FAILED(result))
    {
        ERR("Could not create D3D11 device - aborting!\n");
        return EGL_NOT_INITIALIZED;   // Cleanup done by destructor through glDestroyRenderer
    }
    
    initializeDevice();

    return EGL_SUCCESS;
}

// do any one-time device initialization
// NOTE: this is also needed after a device lost/reset
// to reset the scene status and ensure the default states are reset.
void Renderer11::initializeDevice()
{
    // Permanent non-default states
    // TODO
    // UNIMPLEMENTED();
}


int Renderer11::generateConfigs(ConfigDesc **configDescList)
{
    // TODO
    UNIMPLEMENTED();
    return 0;
}

void Renderer11::deleteConfigs(ConfigDesc *configDescList)
{
    delete [] (configDescList);
}

void Renderer11::startScene()
{
    // TODO: nop in d3d11?
}

void Renderer11::endScene()
{
    // TODO: nop in d3d11?
}

void Renderer11::sync(bool block)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &samplerState)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::setTexture(gl::SamplerType type, int index, gl::Texture *texture)
{
    // TODO
    UNIMPLEMENTED();
}


void Renderer11::releaseDeviceResources()
{
    // TODO
    // UNIMPLEMENTED();
}

void Renderer11::markDeviceLost()
{
    mDeviceLost = true;
}

bool Renderer11::isDeviceLost()
{
    return mDeviceLost;
}

// set notify to true to broadcast a message to all contexts of the device loss
bool Renderer11::testDeviceLost(bool notify)
{
    bool isLost = false;

    // TODO
    UNIMPLEMENTED();

    if (isLost)
    {
        // ensure we note the device loss --
        // we'll probably get this done again by markDeviceLost
        // but best to remember it!
        // Note that we don't want to clear the device loss status here
        // -- this needs to be done by resetDevice
        mDeviceLost = true;
        if (notify)
        {
            mDisplay->notifyDeviceLost();
        }
    }

    return isLost;
}

bool Renderer11::testDeviceResettable()
{
    HRESULT status = D3D_OK;

    // TODO
    UNIMPLEMENTED();

    switch (status)
    {
      case D3DERR_DEVICENOTRESET:
      case D3DERR_DEVICEHUNG:
        return true;
      default:
        return false;
    }
}

bool Renderer11::resetDevice()
{
    releaseDeviceResources();

    // TODO
    UNIMPLEMENTED();

    // reset device defaults
    initializeDevice();
    mDeviceLost = false;

    return true;
}

DWORD Renderer11::getAdapterVendor() const
{
    // TODO
    UNIMPLEMENTED();
    return 0;
}

const char *Renderer11::getAdapterDescription() const
{
    // TODO
    UNIMPLEMENTED();
    return "UNIMPLEMENTED";
}

GUID Renderer11::getAdapterIdentifier() const
{
    // TODO
    UNIMPLEMENTED();
    GUID foo = {};
    return foo;
}

bool Renderer11::getDXT1TextureSupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDXT3TextureSupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDXT5TextureSupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDepthTextureSupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getFloat32TextureSupport(bool *filtering, bool *renderable)
{
    // TODO
    UNIMPLEMENTED();

    *filtering = false;
    *renderable = false;
    return false;
}

bool Renderer11::getFloat16TextureSupport(bool *filtering, bool *renderable)
{
    // TODO
    UNIMPLEMENTED();

    *filtering = false;
    *renderable = false;
    return false;
}

bool Renderer11::getLuminanceTextureSupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getLuminanceAlphaTextureSupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getTextureFilterAnisotropySupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

float Renderer11::getTextureMaxAnisotropy() const
{
    // TODO
    UNIMPLEMENTED();
    return 1.0f;
}

bool Renderer11::getEventQuerySupport()
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getVertexTextureSupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getNonPower2TextureSupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getOcclusionQuerySupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getInstancingSupport() const
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::getShareHandleSupport() const
{
    // TODO
    UNIMPLEMENTED();

    // PIX doesn't seem to support using share handles, so disable them.
    return false && !gl::perfActive();
}

bool Renderer11::getShaderModel3Support() const
{
    // TODO
    UNIMPLEMENTED();
    return true;
}

float Renderer11::getMaxPointSize() const
{
    // TODO
    UNIMPLEMENTED();
    return 1.0f;
}

int Renderer11::getMaxTextureWidth() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;   // 16384
      case D3D_FEATURE_LEVEL_10_1:
      case D3D_FEATURE_LEVEL_10_0: return D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;   // 8192
      default: UNREACHABLE();      return 0;
    }
}

int Renderer11::getMaxTextureHeight() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: return D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;   // 16384
      case D3D_FEATURE_LEVEL_10_1:
      case D3D_FEATURE_LEVEL_10_0: return D3D10_REQ_TEXTURE2D_U_OR_V_DIMENSION;   // 8192
      default: UNREACHABLE();      return 0;
    }
}

bool Renderer11::get32BitIndexSupport() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: 
      case D3D_FEATURE_LEVEL_10_1:
      case D3D_FEATURE_LEVEL_10_0: return D3D10_REQ_DRAWINDEXED_INDEX_COUNT_2_TO_EXP >= 32;   // true
      default: UNREACHABLE();      return false;
    }
}

int Renderer11::getMinSwapInterval() const
{
    // TODO
    UNIMPLEMENTED();
    return 1;
}

int Renderer11::getMaxSwapInterval() const
{
    // TODO
    UNIMPLEMENTED();
    return 1;
}

int Renderer11::getMaxSupportedSamples() const
{
    // TODO
    UNIMPLEMENTED();
    return 1;
}

bool Renderer11::copyToRenderTarget(gl::TextureStorage2D *dest, gl::TextureStorage2D *source)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::copyToRenderTarget(gl::TextureStorageCubeMap *dest, gl::TextureStorageCubeMap *source)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

}
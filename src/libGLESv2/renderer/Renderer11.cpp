//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.cpp: Implements a back-end specific class for the D3D11 renderer.

#include "common/debug.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/renderer/renderer11_utils.h"

#include "libEGL/Config.h"
#include "libEGL/Display.h"

namespace rx
{
static const DXGI_FORMAT RenderTargetFormats[] =
    {
        DXGI_FORMAT_R8G8B8A8_UNORM
    };

static const DXGI_FORMAT DepthStencilFormats[] =
    {
        DXGI_FORMAT_D24_UNORM_S8_UINT
    };

Renderer11::Renderer11(egl::Display *display, HDC hDc) : Renderer(display), mDc(hDc)
{
    mD3d11Module = NULL;
    mDxgiModule = NULL;

    mDeviceLost = false;

    mDevice = NULL;
    mDeviceContext = NULL;
    mDxgiAdapter = NULL;
    mDxgiFactory = NULL;

    mForceSetBlendState = true;
}

Renderer11::~Renderer11()
{
    releaseDeviceResources();

    if (mDxgiFactory)
    {
        mDxgiFactory->Release();
        mDxgiFactory = NULL;
    }

    if (mDxgiAdapter)
    {
        mDxgiAdapter->Release();
        mDxgiAdapter = NULL;
    }

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

    IDXGIDevice *dxgiDevice = NULL;
    result = mDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);

    if (FAILED(result))
    {
        ERR("Could not query DXGI device - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    result = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&mDxgiAdapter);

    if (FAILED(result))
    {
        ERR("Could not retrieve DXGI adapter - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    dxgiDevice->Release();

    result = mDxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&mDxgiFactory);

    if (!mDxgiFactory || FAILED(result))
    {
        ERR("Could not create DXGI factory - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    initializeDevice();

    return EGL_SUCCESS;
}

// do any one-time device initialization
// NOTE: this is also needed after a device lost/reset
// to reset the scene status and ensure the default states are reset.
void Renderer11::initializeDevice()
{
    mStateCache.initialize(mDevice);

    // Permanent non-default states
    // TODO
    // UNIMPLEMENTED();
}

int Renderer11::generateConfigs(ConfigDesc **configDescList)
{
    int numRenderFormats = sizeof(RenderTargetFormats) / sizeof(RenderTargetFormats[0]);
    int numDepthFormats = sizeof(DepthStencilFormats) / sizeof(DepthStencilFormats[0]);
    (*configDescList) = new ConfigDesc[numRenderFormats * numDepthFormats];
    int numConfigs = 0;
    
    for (int formatIndex = 0; formatIndex < numRenderFormats; formatIndex++)
    {
        for (int depthStencilIndex = 0; depthStencilIndex < numDepthFormats; depthStencilIndex++)
        {
            DXGI_FORMAT renderTargetFormat = RenderTargetFormats[formatIndex];

            UINT formatSupport = 0;
            HRESULT result = mDevice->CheckFormatSupport(renderTargetFormat, &formatSupport);
            
            if (SUCCEEDED(result) && (formatSupport & D3D11_FORMAT_SUPPORT_RENDER_TARGET))
            {
                DXGI_FORMAT depthStencilFormat = DepthStencilFormats[depthStencilIndex];

                UINT formatSupport = 0;
                HRESULT result = mDevice->CheckFormatSupport(depthStencilFormat, &formatSupport);

                if (SUCCEEDED(result) && (formatSupport & D3D11_FORMAT_SUPPORT_DEPTH_STENCIL))
                {
                    ConfigDesc newConfig;
                    newConfig.renderTargetFormat = d3d11_gl::ConvertBackBufferFormat(renderTargetFormat);
                    newConfig.depthStencilFormat = d3d11_gl::ConvertDepthStencilFormat(depthStencilFormat);
                    newConfig.multiSample = 0;     // FIXME: enumerate multi-sampling
                    newConfig.fastConfig = true;   // Assume all DX11 format conversions to be fast

                    (*configDescList)[numConfigs++] = newConfig;
                }
            }
        }
    }

    return numConfigs;
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

SwapChain *Renderer11::createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
{
    // TODO
    UNIMPLEMENTED();

    //return new rx::SwapChain(this, window, shareHandle, backBufferFormat, depthBufferFormat);

    return NULL;
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

void Renderer11::setRasterizerState(const gl::RasterizerState &rasterState, unsigned int depthSize)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::setBlendState(const gl::BlendState &blendState, const gl::Color &blendColor,
                               unsigned int sampleMask)
{
    if (mForceSetBlendState ||
        memcmp(&blendState, &mCurBlendState, sizeof(gl::BlendState)) != 0 ||
        memcmp(&blendColor, &mCurBlendColor, sizeof(gl::Color)) != 0 ||
        sampleMask != mCurSampleMask)
    {
        ID3D11BlendState *dxBlendState = mStateCache.getBlendState(blendState);
        if (!dxBlendState)
        {
            ERR("NULL blend state returned by RenderStateCache::getBlendState, setting the default "
                "blend state.");
        }

        const float blendColors[] = { blendColor.red, blendColor.green, blendColor.blue, blendColor.alpha };
        mDeviceContext->OMSetBlendState(dxBlendState, blendColors, sampleMask);

        if (dxBlendState)
        {
            dxBlendState->Release();
        }
        mCurBlendState = blendState;
        mCurBlendColor = blendColor;
        mCurSampleMask = sampleMask;
    }

    mForceSetBlendState = false;
}

void Renderer11::setDepthStencilState(const gl::DepthStencilState &depthStencilState, bool frontFaceCCW,
                                      unsigned int stencilSize)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::setScissorRectangle(const gl::Rectangle& scissor, unsigned int renderTargetWidth,
                                     unsigned int renderTargetHeight)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::applyRenderTarget(gl::Framebuffer *frameBuffer)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::clear(GLbitfield mask, const gl::Color &colorClear, float depthClear, int stencilClear,
                       gl::Framebuffer *frameBuffer)
{
    // TODO
    UNIMPLEMENTED();
}

void Renderer11::releaseDeviceResources()
{
    // TODO
    // UNIMPLEMENTED();
    mStateCache.clear();
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
    //UNIMPLEMENTED();

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

    mForceSetBlendState = true;

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
    //UNIMPLEMENTED();

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
    return 0;
}

int Renderer11::getMaxSwapInterval() const
{
    return 4;
}

int Renderer11::getMaxSupportedSamples() const
{
    // TODO
    UNIMPLEMENTED();
    return 1;
}

bool Renderer11::copyToRenderTarget(TextureStorage2D *dest, TextureStorage2D *source)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::copyToRenderTarget(TextureStorageCubeMap *dest, TextureStorageCubeMap *source)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorage2D *storage, GLint level)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageCubeMap *storage, GLenum target, GLint level)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

}
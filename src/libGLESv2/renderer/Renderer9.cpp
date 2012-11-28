//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer9.cpp: Implements a back-end specific class for the D3D9 renderer.

#include "common/debug.h"
#include "libGLESv2/main.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/renderer/Renderer9.h"
#include "libGLESv2/renderer/renderer9_utils.h"
#include "libGLESv2/renderer/TextureStorage.h"
#include "libGLESv2/renderer/Image.h"
#include "libGLESv2/renderer/Blit.h"

#include "libEGL/Config.h"
#include "libEGL/Display.h"

// Can also be enabled by defining FORCE_REF_RAST in the project's predefined macros
#define REF_RAST 0

// The "Debug This Pixel..." feature in PIX often fails when using the
// D3D9Ex interfaces.  In order to get debug pixel to work on a Vista/Win 7
// machine, define "ANGLE_ENABLE_D3D9EX=0" in your project file.
#if !defined(ANGLE_ENABLE_D3D9EX)
// Enables use of the IDirect3D9Ex interface, when available
#define ANGLE_ENABLE_D3D9EX 1
#endif // !defined(ANGLE_ENABLE_D3D9EX)

namespace rx
{
static const D3DFORMAT RenderTargetFormats[] =
    {
        D3DFMT_A1R5G5B5,
    //  D3DFMT_A2R10G10B10,   // The color_ramp conformance test uses ReadPixels with UNSIGNED_BYTE causing it to think that rendering skipped a colour value.
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
    //  D3DFMT_X1R5G5B5,      // Has no compatible OpenGL ES renderbuffer format
        D3DFMT_X8R8G8B8
    };

static const D3DFORMAT DepthStencilFormats[] =
    {
        D3DFMT_UNKNOWN,
    //  D3DFMT_D16_LOCKABLE,
        D3DFMT_D32,
    //  D3DFMT_D15S1,
        D3DFMT_D24S8,
        D3DFMT_D24X8,
    //  D3DFMT_D24X4S4,
        D3DFMT_D16,
    //  D3DFMT_D32F_LOCKABLE,
    //  D3DFMT_D24FS8
    };

Renderer9::Renderer9(egl::Display *display, HDC hDc, bool softwareDevice) : Renderer(display), mDc(hDc), mSoftwareDevice(softwareDevice)
{
    mD3d9Module = NULL;

    mD3d9 = NULL;
    mD3d9Ex = NULL;
    mDevice = NULL;
    mDeviceEx = NULL;
    mDeviceWindow = NULL;
    mBlit = NULL;

    mAdapter = D3DADAPTER_DEFAULT;

    #if REF_RAST == 1 || defined(FORCE_REF_RAST)
        mDeviceType = D3DDEVTYPE_REF;
    #else
        mDeviceType = D3DDEVTYPE_HAL;
    #endif

    mDeviceLost = false;

    mMaxSupportedSamples = 0;
}

Renderer9::~Renderer9()
{
    releaseDeviceResources();
    
    delete mBlit;

    if (mDevice)
    {
        // If the device is lost, reset it first to prevent leaving the driver in an unstable state
        if (testDeviceLost(false))
        {
            resetDevice();
        }

        mDevice->Release();
        mDevice = NULL;
    }

    if (mDeviceEx)
    {
        mDeviceEx->Release();
        mDeviceEx = NULL;
    }

    if (mD3d9)
    {
        mD3d9->Release();
        mD3d9 = NULL;
    }

    if (mDeviceWindow)
    {
        DestroyWindow(mDeviceWindow);
        mDeviceWindow = NULL;
    }

    if (mD3d9Ex)
    {
        mD3d9Ex->Release();
        mD3d9Ex = NULL;
    }

    if (mD3d9Module)
    {
        mD3d9Module = NULL;
    }

    while (!mMultiSampleSupport.empty())
    {
        delete [] mMultiSampleSupport.begin()->second;
        mMultiSampleSupport.erase(mMultiSampleSupport.begin());
    }

    mForceSetDepthStencilState = true;
    mForceSetRasterState = true;
    mForceSetBlendState = true;
    mForceSetScissor = true;
}

EGLint Renderer9::initialize()
{
    if (mSoftwareDevice)
    {
        mD3d9Module = GetModuleHandle(TEXT("swiftshader_d3d9.dll"));
    }
    else
    {
        mD3d9Module = GetModuleHandle(TEXT("d3d9.dll"));
    }

    if (mD3d9Module == NULL)
    {
        ERR("No D3D9 module found - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    typedef HRESULT (WINAPI *Direct3DCreate9ExFunc)(UINT, IDirect3D9Ex**);
    Direct3DCreate9ExFunc Direct3DCreate9ExPtr = reinterpret_cast<Direct3DCreate9ExFunc>(GetProcAddress(mD3d9Module, "Direct3DCreate9Ex"));

    // Use Direct3D9Ex if available. Among other things, this version is less
    // inclined to report a lost context, for example when the user switches
    // desktop. Direct3D9Ex is available in Windows Vista and later if suitable drivers are available.
    if (ANGLE_ENABLE_D3D9EX && Direct3DCreate9ExPtr && SUCCEEDED(Direct3DCreate9ExPtr(D3D_SDK_VERSION, &mD3d9Ex)))
    {
        ASSERT(mD3d9Ex);
        mD3d9Ex->QueryInterface(IID_IDirect3D9, reinterpret_cast<void**>(&mD3d9));
        ASSERT(mD3d9);
    }
    else
    {
        mD3d9 = Direct3DCreate9(D3D_SDK_VERSION);
    }

    if (!mD3d9)
    {
        ERR("Could not create D3D9 device - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }
    if (mDc != NULL)
    {
    //  UNIMPLEMENTED();   // FIXME: Determine which adapter index the device context corresponds to
    }

    HRESULT result;

    // Give up on getting device caps after about one second.
    for (int i = 0; i < 10; ++i)
    {
        result = mD3d9->GetDeviceCaps(mAdapter, mDeviceType, &mDeviceCaps);
        if (SUCCEEDED(result))
        {
            break;
        }
        else if (result == D3DERR_NOTAVAILABLE)
        {
            Sleep(100);   // Give the driver some time to initialize/recover
        }
        else if (FAILED(result))   // D3DERR_OUTOFVIDEOMEMORY, E_OUTOFMEMORY, D3DERR_INVALIDDEVICE, or another error we can't recover from
        {
            ERR("failed to get device caps (0x%x)\n", result);
            return EGL_NOT_INITIALIZED;
        }
    }

    if (mDeviceCaps.PixelShaderVersion < D3DPS_VERSION(2, 0))
    {
        ERR("Renderer does not support PS 2.0. aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    // When DirectX9 is running with an older DirectX8 driver, a StretchRect from a regular texture to a render target texture is not supported.
    // This is required by Texture2D::convertToRenderTarget.
    if ((mDeviceCaps.DevCaps2 & D3DDEVCAPS2_CAN_STRETCHRECT_FROM_TEXTURES) == 0)
    {
        ERR("Renderer does not support stretctrect from textures!\n");
        return EGL_NOT_INITIALIZED;
    }

    mD3d9->GetAdapterIdentifier(mAdapter, 0, &mAdapterIdentifier);

    // ATI cards on XP have problems with non-power-of-two textures.
    mSupportsNonPower2Textures = !(mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_POW2) &&
        !(mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2) &&
        !(mDeviceCaps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) &&
        !(getComparableOSVersion() < versionWindowsVista && mAdapterIdentifier.VendorId == VENDOR_ID_AMD);

    // Must support a minimum of 2:1 anisotropy for max anisotropy to be considered supported, per the spec
    mSupportsTextureFilterAnisotropy = ((mDeviceCaps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY) && (mDeviceCaps.MaxAnisotropy >= 2));

    mMinSwapInterval = 4;
    mMaxSwapInterval = 0;

    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 0);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 0);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 1);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 1);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_TWO)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 2);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 2);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_THREE)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 3);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 3);
    }
    if (mDeviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_FOUR)
    {
        mMinSwapInterval = std::min(mMinSwapInterval, 4);
        mMaxSwapInterval = std::max(mMaxSwapInterval, 4);
    }

    int max = 0;
    for (int i = 0; i < sizeof(RenderTargetFormats) / sizeof(D3DFORMAT); ++i)
    {
        bool *multisampleArray = new bool[D3DMULTISAMPLE_16_SAMPLES + 1];
        getMultiSampleSupport(RenderTargetFormats[i], multisampleArray);
        mMultiSampleSupport[RenderTargetFormats[i]] = multisampleArray;

        for (int j = D3DMULTISAMPLE_16_SAMPLES; j >= 0; --j)
        {
            if (multisampleArray[j] && j != D3DMULTISAMPLE_NONMASKABLE && j > max)
            {
                max = j;
            }
        }
    }

    for (int i = 0; i < sizeof(DepthStencilFormats) / sizeof(D3DFORMAT); ++i)
    {
        if (DepthStencilFormats[i] == D3DFMT_UNKNOWN)
            continue;

        bool *multisampleArray = new bool[D3DMULTISAMPLE_16_SAMPLES + 1];
        getMultiSampleSupport(DepthStencilFormats[i], multisampleArray);
        mMultiSampleSupport[DepthStencilFormats[i]] = multisampleArray;

        for (int j = D3DMULTISAMPLE_16_SAMPLES; j >= 0; --j)
        {
            if (multisampleArray[j] && j != D3DMULTISAMPLE_NONMASKABLE && j > max)
            {
                max = j;
            }
        }
    }

    mMaxSupportedSamples = max;

    static const TCHAR windowName[] = TEXT("AngleHiddenWindow");
    static const TCHAR className[] = TEXT("STATIC");

    mDeviceWindow = CreateWindowEx(WS_EX_NOACTIVATE, className, windowName, WS_DISABLED | WS_POPUP, 0, 0, 1, 1, HWND_MESSAGE, NULL, GetModuleHandle(NULL), NULL);

    D3DPRESENT_PARAMETERS presentParameters = getDefaultPresentParameters();
    DWORD behaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_NOWINDOWCHANGES;

    result = mD3d9->CreateDevice(mAdapter, mDeviceType, mDeviceWindow, behaviorFlags | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &presentParameters, &mDevice);
    if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_DEVICELOST)
    {
        return EGL_BAD_ALLOC;
    }

    if (FAILED(result))
    {
        result = mD3d9->CreateDevice(mAdapter, mDeviceType, mDeviceWindow, behaviorFlags | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &mDevice);

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY || result == D3DERR_NOTAVAILABLE || result == D3DERR_DEVICELOST);
            return EGL_BAD_ALLOC;
        }
    }

    if (mD3d9Ex)
    {
        result = mDevice->QueryInterface(IID_IDirect3DDevice9Ex, (void**) &mDeviceEx);
        ASSERT(SUCCEEDED(result));
    }

    mVertexShaderCache.initialize(mDevice);
    mPixelShaderCache.initialize(mDevice);

    initializeDevice();

    mBlit = new Blit(this);

    return EGL_SUCCESS;
}

// do any one-time device initialization
// NOTE: this is also needed after a device lost/reset
// to reset the scene status and ensure the default states are reset.
void Renderer9::initializeDevice()
{
    // Permanent non-default states
    mDevice->SetRenderState(D3DRS_POINTSPRITEENABLE, TRUE);
    mDevice->SetRenderState(D3DRS_LASTPIXEL, FALSE);

    if (mDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0))
    {
        mDevice->SetRenderState(D3DRS_POINTSIZE_MAX, (DWORD&)mDeviceCaps.MaxPointSize);
    }
    else
    {
        mDevice->SetRenderState(D3DRS_POINTSIZE_MAX, 0x3F800000);   // 1.0f
    }

    mSceneStarted = false;
}

D3DPRESENT_PARAMETERS Renderer9::getDefaultPresentParameters()
{
    D3DPRESENT_PARAMETERS presentParameters = {0};

    // The default swap chain is never actually used. Surface will create a new swap chain with the proper parameters.
    presentParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
    presentParameters.BackBufferCount = 1;
    presentParameters.BackBufferFormat = D3DFMT_UNKNOWN;
    presentParameters.BackBufferWidth = 1;
    presentParameters.BackBufferHeight = 1;
    presentParameters.EnableAutoDepthStencil = FALSE;
    presentParameters.Flags = 0;
    presentParameters.hDeviceWindow = mDeviceWindow;
    presentParameters.MultiSampleQuality = 0;
    presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
    presentParameters.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    presentParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
    presentParameters.Windowed = TRUE;

    return presentParameters;
}

int Renderer9::generateConfigs(ConfigDesc **configDescList)
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    int numRenderFormats = sizeof(RenderTargetFormats) / sizeof(RenderTargetFormats[0]);
    int numDepthFormats = sizeof(DepthStencilFormats) / sizeof(DepthStencilFormats[0]);
    (*configDescList) = new ConfigDesc[numRenderFormats * numDepthFormats];
    int numConfigs = 0;

    for (int formatIndex = 0; formatIndex < numRenderFormats; formatIndex++)
    {
        D3DFORMAT renderTargetFormat = RenderTargetFormats[formatIndex];

        HRESULT result = mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, renderTargetFormat);

        if (SUCCEEDED(result))
        {
            for (int depthStencilIndex = 0; depthStencilIndex < numDepthFormats; depthStencilIndex++)
            {
                D3DFORMAT depthStencilFormat = DepthStencilFormats[depthStencilIndex];
                HRESULT result = D3D_OK;

                if(depthStencilFormat != D3DFMT_UNKNOWN)
                {
                    result = mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthStencilFormat);
                }

                if (SUCCEEDED(result))
                {
                    if(depthStencilFormat != D3DFMT_UNKNOWN)
                    {
                        result = mD3d9->CheckDepthStencilMatch(mAdapter, mDeviceType, currentDisplayMode.Format, renderTargetFormat, depthStencilFormat);
                    }

                    if (SUCCEEDED(result))
                    {
                        ConfigDesc newConfig;
                        newConfig.renderTargetFormat = d3d9_gl::ConvertBackBufferFormat(renderTargetFormat);
                        newConfig.depthStencilFormat = d3d9_gl::ConvertDepthStencilFormat(depthStencilFormat);
                        newConfig.multiSample = 0; // FIXME: enumerate multi-sampling
                        newConfig.fastConfig = (currentDisplayMode.Format == renderTargetFormat);

                        (*configDescList)[numConfigs++] = newConfig;
                    }
                }
            }
        }
    }

    return numConfigs;
}

void Renderer9::deleteConfigs(ConfigDesc *configDescList)
{
    delete [] (configDescList);
}

void Renderer9::startScene()
{
    if (!mSceneStarted)
    {
        long result = mDevice->BeginScene();
        if (SUCCEEDED(result)) {
            // This is defensive checking against the device being
            // lost at unexpected times.
            mSceneStarted = true;
        }
    }
}

void Renderer9::endScene()
{
    if (mSceneStarted)
    {
        // EndScene can fail if the device was lost, for example due
        // to a TDR during a draw call.
        mDevice->EndScene();
        mSceneStarted = false;
    }
}

// D3D9_REPLACE
void Renderer9::sync(bool block)
{
    HRESULT result;

    IDirect3DQuery9* query = allocateEventQuery();
    if (!query)
    {
        return;
    }

    result = query->Issue(D3DISSUE_END);
    ASSERT(SUCCEEDED(result));

    do
    {
        result = query->GetData(NULL, 0, D3DGETDATA_FLUSH);

        if(block && result == S_FALSE)
        {
            // Keep polling, but allow other threads to do something useful first
            Sleep(0);
            // explicitly check for device loss
            // some drivers seem to return S_FALSE even if the device is lost
            // instead of D3DERR_DEVICELOST like they should
            if (testDeviceLost(false))
            {
                result = D3DERR_DEVICELOST;
            }
        }
    }
    while(block && result == S_FALSE);

    freeEventQuery(query);

    if (isDeviceLostError(result))
    {
        mDisplay->notifyDeviceLost();
    }
}

// D3D9_REPLACE
IDirect3DQuery9* Renderer9::allocateEventQuery()
{
    IDirect3DQuery9 *query = NULL;

    if (mEventQueryPool.empty())
    {
        HRESULT result = mDevice->CreateQuery(D3DQUERYTYPE_EVENT, &query);
        ASSERT(SUCCEEDED(result));
    }
    else
    {
        query = mEventQueryPool.back();
        mEventQueryPool.pop_back();
    }

    return query;
}

// D3D9_REPLACE
void Renderer9::freeEventQuery(IDirect3DQuery9* query)
{
    if (mEventQueryPool.size() > 1000)
    {
        query->Release();
    }
    else
    {
        mEventQueryPool.push_back(query);
    }
}

IDirect3DVertexShader9 *Renderer9::createVertexShader(const DWORD *function, size_t length)
{
    return mVertexShaderCache.create(function, length);
}

IDirect3DPixelShader9 *Renderer9::createPixelShader(const DWORD *function, size_t length)
{
    return mPixelShaderCache.create(function, length);
}

HRESULT Renderer9::createVertexBuffer(UINT Length, DWORD Usage, IDirect3DVertexBuffer9 **ppVertexBuffer)
{
    D3DPOOL Pool = getBufferPool(Usage);
    return mDevice->CreateVertexBuffer(Length, Usage, 0, Pool, ppVertexBuffer, NULL);
}

HRESULT Renderer9::createIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, IDirect3DIndexBuffer9 **ppIndexBuffer)
{
    D3DPOOL Pool = getBufferPool(Usage);
    return mDevice->CreateIndexBuffer(Length, Usage, Format, Pool, ppIndexBuffer, NULL);
}

void Renderer9::setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &samplerState)
{
    int d3dSamplerOffset = (type == gl::SAMPLER_PIXEL) ? 0 : D3DVERTEXTEXTURESAMPLER0;
    int d3dSampler = index + d3dSamplerOffset;

    mDevice->SetSamplerState(d3dSampler, D3DSAMP_ADDRESSU, gl_d3d9::ConvertTextureWrap(samplerState.wrapS));
    mDevice->SetSamplerState(d3dSampler, D3DSAMP_ADDRESSV, gl_d3d9::ConvertTextureWrap(samplerState.wrapT));

    mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAGFILTER, gl_d3d9::ConvertMagFilter(samplerState.magFilter, samplerState.maxAnisotropy));
    D3DTEXTUREFILTERTYPE d3dMinFilter, d3dMipFilter;
    gl_d3d9::ConvertMinFilter(samplerState.minFilter, &d3dMinFilter, &d3dMipFilter, samplerState.maxAnisotropy);
    mDevice->SetSamplerState(d3dSampler, D3DSAMP_MINFILTER, d3dMinFilter);
    mDevice->SetSamplerState(d3dSampler, D3DSAMP_MIPFILTER, d3dMipFilter);
    mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAXMIPLEVEL, samplerState.lodOffset);
    if (mSupportsTextureFilterAnisotropy)
    {
        mDevice->SetSamplerState(d3dSampler, D3DSAMP_MAXANISOTROPY, (DWORD)samplerState.maxAnisotropy);
    }
}

void Renderer9::setTexture(gl::SamplerType type, int index, gl::Texture *texture)
{
    int d3dSamplerOffset = (type == gl::SAMPLER_PIXEL) ? 0 : D3DVERTEXTEXTURESAMPLER0;
    int d3dSampler = index + d3dSamplerOffset;
    IDirect3DBaseTexture9 *d3dTexture = NULL;

    if (texture)
    {
        TextureStorage *texStorage = texture->getNativeTexture();
        if (texStorage)
        {
            d3dTexture = texStorage->getBaseTexture();
        }
        // If we get NULL back from getBaseTexture here, something went wrong
        // in the texture class and we're unexpectedly missing the d3d texture
        ASSERT(d3dTexture != NULL);
    }

    mDevice->SetTexture(d3dSampler, d3dTexture);
}

void Renderer9::setRasterizerState(const gl::RasterizerState &rasterState, unsigned int depthSize)
{
    bool rasterStateChanged = mForceSetRasterState || memcmp(&rasterState, &mCurRasterState, sizeof(gl::RasterizerState)) != 0;
    bool depthSizeChanged = mForceSetRasterState || depthSize != mCurDepthSize;

    if (rasterStateChanged)
    {
        // Set the cull mode
        if (rasterState.cullFace)
        {
            mDevice->SetRenderState(D3DRS_CULLMODE, gl_d3d9::ConvertCullMode(rasterState.cullMode, rasterState.frontFace));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
        }

        mDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, rasterState.scissorTest ? TRUE : FALSE);

        mCurRasterState = rasterState;
    }

    if (rasterStateChanged || depthSizeChanged)
    {
        if (rasterState.polygonOffsetFill)
        {
            if (depthSize > 0)
            {
                mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&rasterState.polygonOffsetFactor);

                float depthBias = ldexp(rasterState.polygonOffsetUnits, -static_cast<int>(depthSize));
                mDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&depthBias);
            }
        }
        else
        {
            mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
            mDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
        }

        mCurDepthSize = depthSize;
    }

    mForceSetRasterState = false;
}

void Renderer9::setBlendState(const gl::BlendState &blendState, const gl::Color &blendColor, unsigned int sampleMask)
{
    bool blendStateChanged = mForceSetBlendState || memcmp(&blendState, &mCurBlendState, sizeof(gl::BlendState)) != 0;
    bool blendColorChanged = mForceSetBlendState || memcmp(&blendColor, &mCurBlendColor, sizeof(gl::Color)) != 0;
    bool sampleMaskChanged = mForceSetBlendState || sampleMask != mCurSampleMask;

    if (blendStateChanged || blendColorChanged)
    {
        if (blendState.blend)
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            if (blendState.sourceBlendRGB != GL_CONSTANT_ALPHA && blendState.sourceBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA &&
                blendState.destBlendRGB != GL_CONSTANT_ALPHA && blendState.destBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA)
            {
                mDevice->SetRenderState(D3DRS_BLENDFACTOR, gl_d3d9::ConvertColor(blendColor));
            }
            else
            {
                mDevice->SetRenderState(D3DRS_BLENDFACTOR, D3DCOLOR_RGBA(gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha),
                                                                         gl::unorm<8>(blendColor.alpha)));
            }

            mDevice->SetRenderState(D3DRS_SRCBLEND, gl_d3d9::ConvertBlendFunc(blendState.sourceBlendRGB));
            mDevice->SetRenderState(D3DRS_DESTBLEND, gl_d3d9::ConvertBlendFunc(blendState.destBlendRGB));
            mDevice->SetRenderState(D3DRS_BLENDOP, gl_d3d9::ConvertBlendOp(blendState.blendEquationRGB));

            if (blendState.sourceBlendRGB != blendState.sourceBlendAlpha ||
                blendState.destBlendRGB != blendState.destBlendAlpha ||
                blendState.blendEquationRGB != blendState.blendEquationAlpha)
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

                mDevice->SetRenderState(D3DRS_SRCBLENDALPHA, gl_d3d9::ConvertBlendFunc(blendState.sourceBlendAlpha));
                mDevice->SetRenderState(D3DRS_DESTBLENDALPHA, gl_d3d9::ConvertBlendFunc(blendState.destBlendAlpha));
                mDevice->SetRenderState(D3DRS_BLENDOPALPHA, gl_d3d9::ConvertBlendOp(blendState.blendEquationAlpha));
            }
            else
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            }
        }
        else
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        }

        if (blendState.sampleAlphaToCoverage)
        {
            FIXME("Sample alpha to coverage is unimplemented.");
        }

        // Set the color mask
        bool zeroColorMaskAllowed = getAdapterVendor() != VENDOR_ID_AMD;
        // Apparently some ATI cards have a bug where a draw with a zero color
        // write mask can cause later draws to have incorrect results. Instead,
        // set a nonzero color write mask but modify the blend state so that no
        // drawing is done.
        // http://code.google.com/p/angleproject/issues/detail?id=169

        DWORD colorMask = gl_d3d9::ConvertColorMask(blendState.colorMaskRed, blendState.colorMaskGreen,
                                                    blendState.colorMaskBlue, blendState.colorMaskAlpha);
        if (colorMask == 0 && !zeroColorMaskAllowed)
        {
            // Enable green channel, but set blending so nothing will be drawn.
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN);
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            mDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            mDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            mDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        }
        else
        {
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
        }

        mDevice->SetRenderState(D3DRS_DITHERENABLE, blendState.dither ? TRUE : FALSE);

        mCurBlendState = blendState;
        mCurBlendColor = blendColor;
    }

    if (sampleMaskChanged)
    {
        // Set the multisample mask
        mDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
        mDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, static_cast<DWORD>(sampleMask));

        mCurSampleMask = sampleMask;
    }

    mForceSetBlendState = false;
}

void Renderer9::setDepthStencilState(const gl::DepthStencilState &depthStencilState, bool frontFaceCCW,
                                     unsigned int stencilSize)
{
    bool depthStencilStateChanged = mForceSetDepthStencilState ||
                                    memcmp(&depthStencilState, &mCurDepthStencilState, sizeof(gl::DepthStencilState)) != 0;
    bool frontFaceCCWChanged = mForceSetDepthStencilState || frontFaceCCW != mCurFrontFaceCCW;
    bool stencilSizeChanged = mForceSetDepthStencilState || stencilSize != mCurStencilSize;

    if (depthStencilStateChanged)
    {
        if (depthStencilState.depthTest)
        {
            mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
            mDevice->SetRenderState(D3DRS_ZFUNC, gl_d3d9::ConvertComparison(depthStencilState.depthFunc));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
        }

        mCurDepthStencilState = depthStencilState;
    }

    if (depthStencilStateChanged || frontFaceCCWChanged || stencilSizeChanged)
    {
        if (depthStencilState.stencilTest && stencilSize > 0)
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
            mDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);

            // FIXME: Unsupported by D3D9
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILREF = D3DRS_STENCILREF;
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILMASK = D3DRS_STENCILMASK;
            const D3DRENDERSTATETYPE D3DRS_CCW_STENCILWRITEMASK = D3DRS_STENCILWRITEMASK;
            if (depthStencilState.stencilWritemask != depthStencilState.stencilBackWritemask ||
                depthStencilState.stencilRef != depthStencilState.stencilBackRef ||
                depthStencilState.stencilMask != depthStencilState.stencilBackMask)
            {
                ERR("Separate front/back stencil writemasks, reference values, or stencil mask values are invalid under WebGL.");
                return error(GL_INVALID_OPERATION);
            }

            // get the maximum size of the stencil ref
            GLuint maxStencil = (1 << stencilSize) - 1;

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                                    depthStencilState.stencilWritemask);
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                                    gl_d3d9::ConvertComparison(depthStencilState.stencilFunc));

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                                    (depthStencilState.stencilRef < (GLint)maxStencil) ? depthStencilState.stencilRef : maxStencil);
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK,
                                    depthStencilState.stencilMask);

            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilFail));
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilPassDepthFail));
            mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilPassDepthPass));

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                                    depthStencilState.stencilBackWritemask);
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                                    gl_d3d9::ConvertComparison(depthStencilState.stencilBackFunc));

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                                    (depthStencilState.stencilBackRef < (GLint)maxStencil) ? depthStencilState.stencilBackRef : maxStencil);
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK,
                                    depthStencilState.stencilBackMask);

            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackFail));
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackPassDepthFail));
            mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                                    gl_d3d9::ConvertStencilOp(depthStencilState.stencilBackPassDepthPass));
        }
        else
        {
            mDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
        }

        mDevice->SetRenderState(D3DRS_ZWRITEENABLE, depthStencilState.depthMask ? TRUE : FALSE);

        mCurFrontFaceCCW = frontFaceCCW;
        mCurStencilSize = stencilSize;
    }

    mForceSetDepthStencilState = false;
}

void Renderer9::setScissorRectangle(const gl::Rectangle& scissor, unsigned int renderTargetWidth,
                                    unsigned int renderTargetHeight)
{
    bool renderTargetSizedChanged = mForceSetScissor ||
                                    renderTargetWidth != mCurRenderTargetWidth ||
                                    renderTargetHeight != mCurRenderTargetHeight;
    bool scissorChanged = mForceSetScissor || memcmp(&scissor, &mCurScissor, sizeof(gl::Rectangle)) != 0;

    if (renderTargetSizedChanged || scissorChanged)
    {
        RECT rect;
        rect.left = gl::clamp(scissor.x, 0, static_cast<int>(renderTargetWidth));
        rect.top = gl::clamp(scissor.y, 0, static_cast<int>(renderTargetWidth));
        rect.right = gl::clamp(scissor.x + scissor.width, 0, static_cast<int>(renderTargetWidth));
        rect.bottom = gl::clamp(scissor.y + scissor.height, 0, static_cast<int>(renderTargetWidth));
        mDevice->SetScissorRect(&rect);

        mCurScissor = scissor;
        mCurRenderTargetWidth = renderTargetWidth;
        mCurRenderTargetHeight = renderTargetHeight;
    }

    mForceSetScissor = false;
}

void Renderer9::applyRenderTarget(gl::Framebuffer *frameBuffer)
{
    mForceSetScissor = true;

    // TODO
}

void Renderer9::releaseDeviceResources()
{
    while (!mEventQueryPool.empty())
    {
        mEventQueryPool.back()->Release();
        mEventQueryPool.pop_back();
    }

    mVertexShaderCache.clear();
    mPixelShaderCache.clear();
}


void Renderer9::markDeviceLost()
{
    mDeviceLost = true;
}

bool Renderer9::isDeviceLost()
{
    return mDeviceLost;
}

// set notify to true to broadcast a message to all contexts of the device loss
bool Renderer9::testDeviceLost(bool notify)
{
    bool isLost = false;

    if (mDeviceEx)
    {
        isLost = FAILED(mDeviceEx->CheckDeviceState(NULL));
    }
    else if (mDevice)
    {
        isLost = FAILED(mDevice->TestCooperativeLevel());
    }
    else
    {
        // No device yet, so no reset required
    }

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

bool Renderer9::testDeviceResettable()
{
    HRESULT status = D3D_OK;

    if (mDeviceEx)
    {
        status = mDeviceEx->CheckDeviceState(NULL);
    }
    else if (mDevice)
    {
        status = mDevice->TestCooperativeLevel();
    }

    switch (status)
    {
      case D3DERR_DEVICENOTRESET:
      case D3DERR_DEVICEHUNG:
        return true;
      default:
        return false;
    }
}

bool Renderer9::resetDevice()
{
    releaseDeviceResources();

    D3DPRESENT_PARAMETERS presentParameters = getDefaultPresentParameters();

    HRESULT result = D3D_OK;
    bool lost = testDeviceLost(false);
    int attempts = 3;

    while (lost && attempts > 0)
    {
        if (mDeviceEx)
        {
            Sleep(500);   // Give the graphics driver some CPU time
            result = mDeviceEx->ResetEx(&presentParameters, NULL);
        }
        else
        {
            result = mDevice->TestCooperativeLevel();
            while (result == D3DERR_DEVICELOST)
            {
                Sleep(100);   // Give the graphics driver some CPU time
                result = mDevice->TestCooperativeLevel();
            }

            if (result == D3DERR_DEVICENOTRESET)
            {
                result = mDevice->Reset(&presentParameters);
            }
        }

        lost = testDeviceLost(false);
        attempts --;
    }

    if (FAILED(result))
    {
        ERR("Reset/ResetEx failed multiple times: 0x%08X", result);
        return false;
    }

    // reset device defaults
    initializeDevice();
    mDeviceLost = false;

    mForceSetDepthStencilState = true;
    mForceSetRasterState = true;
    mForceSetBlendState = true;
    mForceSetScissor = true;

    return true;
}

DWORD Renderer9::getAdapterVendor() const
{
    return mAdapterIdentifier.VendorId;
}

const char *Renderer9::getAdapterDescription() const
{
    return mAdapterIdentifier.Description;
}

GUID Renderer9::getAdapterIdentifier() const
{
    return mAdapterIdentifier.DeviceIdentifier;
}

void Renderer9::getMultiSampleSupport(D3DFORMAT format, bool *multiSampleArray)
{
    for (int multiSampleIndex = 0; multiSampleIndex <= D3DMULTISAMPLE_16_SAMPLES; multiSampleIndex++)
    {
        HRESULT result = mD3d9->CheckDeviceMultiSampleType(mAdapter, mDeviceType, format,
                                                           TRUE, (D3DMULTISAMPLE_TYPE)multiSampleIndex, NULL);

        multiSampleArray[multiSampleIndex] = SUCCEEDED(result);
    }
}

bool Renderer9::getDXT1TextureSupport()
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1));
}

bool Renderer9::getDXT3TextureSupport()
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT3));
}

bool Renderer9::getDXT5TextureSupport()
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT5));
}

// we use INTZ for depth textures in Direct3D9
// we also want NULL texture support to ensure the we can make depth-only FBOs
// see http://aras-p.info/texts/D3D9GPUHacks.html
bool Renderer9::getDepthTextureSupport() const
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    bool intz = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format,
                                                   D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_INTZ));
    bool null = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format,
                                                   D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, D3DFMT_NULL));

    return intz && null;
}

bool Renderer9::getFloat32TextureSupport(bool *filtering, bool *renderable)
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    *filtering = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_QUERY_FILTER, 
                                                    D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)) &&
                 SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_QUERY_FILTER,
                                                    D3DRTYPE_CUBETEXTURE, D3DFMT_A32B32G32R32F));

    *renderable = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET,
                                                     D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F))&&
                  SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET,
                                                     D3DRTYPE_CUBETEXTURE, D3DFMT_A32B32G32R32F));

    if (!*filtering && !*renderable)
    {
        return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0,
                                                  D3DRTYPE_TEXTURE, D3DFMT_A32B32G32R32F)) &&
               SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0,
                                                  D3DRTYPE_CUBETEXTURE, D3DFMT_A32B32G32R32F));
    }
    else
    {
        return true;
    }
}

bool Renderer9::getFloat16TextureSupport(bool *filtering, bool *renderable)
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    *filtering = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_QUERY_FILTER, 
                                                    D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) &&
                 SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_QUERY_FILTER,
                                                    D3DRTYPE_CUBETEXTURE, D3DFMT_A16B16G16R16F));

    *renderable = SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET, 
                                                    D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) &&
                 SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET,
                                                    D3DRTYPE_CUBETEXTURE, D3DFMT_A16B16G16R16F));

    if (!*filtering && !*renderable)
    {
        return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0,
                                                  D3DRTYPE_TEXTURE, D3DFMT_A16B16G16R16F)) &&
               SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0,
                                                  D3DRTYPE_CUBETEXTURE, D3DFMT_A16B16G16R16F));
    }
    else
    {
        return true;
    }
}

bool Renderer9::getLuminanceTextureSupport()
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_L8));
}

bool Renderer9::getLuminanceAlphaTextureSupport()
{
    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    return SUCCEEDED(mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, 0, D3DRTYPE_TEXTURE, D3DFMT_A8L8));
}

bool Renderer9::getTextureFilterAnisotropySupport() const
{
    return mSupportsTextureFilterAnisotropy;
}

float Renderer9::getTextureMaxAnisotropy() const
{
    if (mSupportsTextureFilterAnisotropy)
    {
        return mDeviceCaps.MaxAnisotropy;
    }
    return 1.0f;
}

bool Renderer9::getEventQuerySupport()
{
    IDirect3DQuery9 *query = allocateEventQuery();
    if (query)
    {
        freeEventQuery(query);
        return true;
    }
    else
    {
        return false;
    }
    return true;
}

// Only Direct3D 10 ready devices support all the necessary vertex texture formats.
// We test this using D3D9 by checking support for the R16F format.
bool Renderer9::getVertexTextureSupport() const
{
    if (!mDevice || mDeviceCaps.PixelShaderVersion < D3DPS_VERSION(3, 0))
    {
        return false;
    }

    D3DDISPLAYMODE currentDisplayMode;
    mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

    HRESULT result = mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_QUERY_VERTEXTEXTURE, D3DRTYPE_TEXTURE, D3DFMT_R16F);

    return SUCCEEDED(result);
}

bool Renderer9::getNonPower2TextureSupport() const
{
    return mSupportsNonPower2Textures;
}

bool Renderer9::getOcclusionQuerySupport() const
{
    if (!mDevice)
    {
        return false;
    }

    IDirect3DQuery9 *query = NULL;
    HRESULT result = mDevice->CreateQuery(D3DQUERYTYPE_OCCLUSION, &query);
    if (SUCCEEDED(result) && query)
    {
        query->Release();
        return true;
    }
    else
    {
        return false;
    }
}

bool Renderer9::getInstancingSupport() const
{
    return mDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0);
}

bool Renderer9::getShareHandleSupport() const
{
    // PIX doesn't seem to support using share handles, so disable them.
    // D3D9_REPLACE
    return (mD3d9Ex != NULL) && !gl::perfActive();
}

bool Renderer9::getShaderModel3Support() const
{
    return mDeviceCaps.PixelShaderVersion >= D3DPS_VERSION(3, 0);
}

float Renderer9::getMaxPointSize() const
{
    return mDeviceCaps.MaxPointSize;
}

int Renderer9::getMaxTextureWidth() const
{
    return (int)mDeviceCaps.MaxTextureWidth;
}

int Renderer9::getMaxTextureHeight() const
{
    return (int)mDeviceCaps.MaxTextureHeight;
}

bool Renderer9::get32BitIndexSupport() const
{
    return mDeviceCaps.MaxVertexIndex >= (1 << 16);
}

DWORD Renderer9::getCapsDeclTypes() const
{
    return mDeviceCaps.DeclTypes;
}

int Renderer9::getMinSwapInterval() const
{
    return mMinSwapInterval;
}

int Renderer9::getMaxSwapInterval() const
{
    return mMaxSwapInterval;
}

int Renderer9::getMaxSupportedSamples() const
{
    return mMaxSupportedSamples;
}

int Renderer9::getNearestSupportedSamples(D3DFORMAT format, int requested) const
{
    if (requested == 0)
    {
        return requested;
    }

    std::map<D3DFORMAT, bool *>::const_iterator itr = mMultiSampleSupport.find(format);
    if (itr == mMultiSampleSupport.end())
    {
        if (format == D3DFMT_UNKNOWN)
            return 0;
        return -1;
    }

    for (int i = requested; i <= D3DMULTISAMPLE_16_SAMPLES; ++i)
    {
        if (itr->second[i] && i != D3DMULTISAMPLE_NONMASKABLE)
        {
            return i;
        }
    }

    return -1;
}

D3DFORMAT Renderer9::ConvertTextureInternalFormat(GLint internalformat)
{
    switch (internalformat)
    {
      case GL_DEPTH_COMPONENT16:
      case GL_DEPTH_COMPONENT32_OES:
      case GL_DEPTH24_STENCIL8_OES:
        return D3DFMT_INTZ;
      case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
      case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        return D3DFMT_DXT1;
      case GL_COMPRESSED_RGBA_S3TC_DXT3_ANGLE:
        return D3DFMT_DXT3;
      case GL_COMPRESSED_RGBA_S3TC_DXT5_ANGLE:
        return D3DFMT_DXT5;
      case GL_RGBA32F_EXT:
      case GL_RGB32F_EXT:
      case GL_ALPHA32F_EXT:
      case GL_LUMINANCE32F_EXT:
      case GL_LUMINANCE_ALPHA32F_EXT:
        return D3DFMT_A32B32G32R32F;
      case GL_RGBA16F_EXT:
      case GL_RGB16F_EXT:
      case GL_ALPHA16F_EXT:
      case GL_LUMINANCE16F_EXT:
      case GL_LUMINANCE_ALPHA16F_EXT:
        return D3DFMT_A16B16G16R16F;
      case GL_LUMINANCE8_EXT:
        if (getLuminanceTextureSupport())
        {
            return D3DFMT_L8;
        }
        break;
      case GL_LUMINANCE8_ALPHA8_EXT:
        if (getLuminanceAlphaTextureSupport())
        {
            return D3DFMT_A8L8;
        }
        break;
      case GL_RGB8_OES:
      case GL_RGB565:
        return D3DFMT_X8R8G8B8;
    }

    return D3DFMT_A8R8G8B8;
}

bool Renderer9::copyToRenderTarget(TextureStorage2D *dest, TextureStorage2D *source)
{
    bool result = false;

    if (source && dest)
    {
        int levels = source->levelCount();
        for (int i = 0; i < levels; ++i)
        {
            IDirect3DSurface9 *srcSurf = source->getSurfaceLevel(i, false);
            IDirect3DSurface9 *dstSurf = dest->getSurfaceLevel(i, false);
            
            result = copyToRenderTarget(dstSurf, srcSurf, source->isManaged());

            if (srcSurf) srcSurf->Release();
            if (dstSurf) dstSurf->Release();

            if (!result)
                return false;
        }
    }

    return result;
}

bool Renderer9::copyToRenderTarget(TextureStorageCubeMap *dest, TextureStorageCubeMap *source)
{
    bool result = false;

    if (source && dest)
    {
        int levels = source->levelCount();
        for (int f = 0; f < 6; f++)
        {
            for (int i = 0; i < levels; i++)
            {
                IDirect3DSurface9 *srcSurf = source->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, false);
                IDirect3DSurface9 *dstSurf = dest->getCubeMapSurface(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, i, true);

                result = copyToRenderTarget(dstSurf, srcSurf, source->isManaged());

                if (srcSurf) srcSurf->Release();
                if (dstSurf) dstSurf->Release();

                if (!result)
                    return false;
            }
        }
    }

    return result;
}

D3DPOOL Renderer9::getBufferPool(DWORD usage) const
{
    if (mD3d9Ex != NULL)
    {
        return D3DPOOL_DEFAULT;
    }
    else
    {
        if (!(usage & D3DUSAGE_DYNAMIC))
        {
            return D3DPOOL_MANAGED;
        }
    }

    return D3DPOOL_DEFAULT;
}

bool Renderer9::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset,
                          TextureStorage2D *storage, GLint level)
{
    return mBlit->copy(framebuffer, sourceRect, destFormat, xoffset, yoffset, storage, level);
}

bool Renderer9::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat, GLint xoffset, GLint yoffset,
                          TextureStorageCubeMap *storage, GLenum target, GLint level)
{
    return mBlit->copy(framebuffer, sourceRect, destFormat, xoffset, yoffset, storage, target, level);
}

bool Renderer9::boxFilter(IDirect3DSurface9 *source, IDirect3DSurface9 *dest)
{
    return mBlit->boxFilter(source, dest);
}

D3DPOOL Renderer9::getTexturePool(DWORD usage) const
{
    if (mD3d9Ex != NULL)
    {
        return D3DPOOL_DEFAULT;
    }
    else
    {
        if (!(usage & (D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_RENDERTARGET)))
        {
            return D3DPOOL_MANAGED;
        }
    }

    return D3DPOOL_DEFAULT;
}

bool Renderer9::copyToRenderTarget(IDirect3DSurface9 *dest, IDirect3DSurface9 *source, bool fromManaged)
{
    if (source && dest)
    {
        HRESULT result = D3DERR_OUTOFVIDEOMEMORY;
        IDirect3DDevice9 *device = getDevice(); // D3D9_REPLACE

        if (fromManaged)
        {
            D3DSURFACE_DESC desc;
            source->GetDesc(&desc);

            IDirect3DSurface9 *surf = 0;
            result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format, D3DPOOL_SYSTEMMEM, &surf, NULL);

            if (SUCCEEDED(result))
            {
                Image::CopyLockableSurfaces(surf, source);
                result = device->UpdateSurface(surf, NULL, dest, NULL);
                surf->Release();
            }
        }
        else
        {
            endScene();
            result = device->StretchRect(source, NULL, dest, NULL, D3DTEXF_NONE);
        }

        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            return false;
        }
    }

    return true;
} 

}
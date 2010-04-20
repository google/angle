//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.cpp: Implements the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#include "libEGL/Display.h"

#include <algorithm>
#include <vector>

#include "common/debug.h"

#include "libEGL/main.h"

namespace egl
{
Display::Display(HDC deviceContext) : mDc(deviceContext)
{
    mD3d9 = NULL;
    mDevice = NULL;

    mAdapter = D3DADAPTER_DEFAULT;
    mDeviceType = D3DDEVTYPE_HAL;
}

Display::~Display()
{
    terminate();
}

bool Display::initialize()
{
    if (isInitialized())
    {
        return true;
    }

    mD3d9 = Direct3DCreate9(D3D_SDK_VERSION);

    if (mD3d9)
    {
        if (mDc != NULL)
        {
        //  UNIMPLEMENTED();   // FIXME: Determine which adapter index the device context corresponds to
        }

        D3DCAPS9 caps;
        HRESULT result = mD3d9->GetDeviceCaps(mAdapter, mDeviceType, &caps);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            return error(EGL_BAD_ALLOC, false);
        }

        if (caps.PixelShaderVersion < D3DPS_VERSION(2, 0))
        {
            mD3d9->Release();
            mD3d9 = NULL;
        }
        else
        {
            EGLint minSwapInterval = 4;
            EGLint maxSwapInterval = 0;

            if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) {minSwapInterval = std::min(minSwapInterval, 0); maxSwapInterval = std::max(maxSwapInterval, 0);}
            if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)       {minSwapInterval = std::min(minSwapInterval, 1); maxSwapInterval = std::max(maxSwapInterval, 1);}
            if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_TWO)       {minSwapInterval = std::min(minSwapInterval, 2); maxSwapInterval = std::max(maxSwapInterval, 2);}
            if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_THREE)     {minSwapInterval = std::min(minSwapInterval, 3); maxSwapInterval = std::max(maxSwapInterval, 3);}
            if (caps.PresentationIntervals & D3DPRESENT_INTERVAL_FOUR)      {minSwapInterval = std::min(minSwapInterval, 4); maxSwapInterval = std::max(maxSwapInterval, 4);}

            const D3DFORMAT adapterFormats[] =
            {
                D3DFMT_A1R5G5B5,
                D3DFMT_A2R10G10B10,
                D3DFMT_A8R8G8B8,
                D3DFMT_R5G6B5,
                D3DFMT_X1R5G5B5,
                D3DFMT_X8R8G8B8
            };

            const D3DFORMAT depthStencilFormats[] =
            {
            //  D3DFMT_D16_LOCKABLE,
                D3DFMT_D32,
                D3DFMT_D15S1,
                D3DFMT_D24S8,
                D3DFMT_D24X8,
                D3DFMT_D24X4S4,
                D3DFMT_D16,
            //  D3DFMT_D32F_LOCKABLE,
            //  D3DFMT_D24FS8
            };

            D3DDISPLAYMODE currentDisplayMode;
            mD3d9->GetAdapterDisplayMode(mAdapter, &currentDisplayMode);

            for (int formatIndex = 0; formatIndex < sizeof(adapterFormats) / sizeof(D3DFORMAT); formatIndex++)
            {
                D3DFORMAT renderTargetFormat = adapterFormats[formatIndex];

                HRESULT result = mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, renderTargetFormat);

                if (SUCCEEDED(result))
                {
                    for (int depthStencilIndex = 0; depthStencilIndex < sizeof(depthStencilFormats) / sizeof(D3DFORMAT); depthStencilIndex++)
                    {
                        D3DFORMAT depthStencilFormat = depthStencilFormats[depthStencilIndex];
                        HRESULT result = mD3d9->CheckDeviceFormat(mAdapter, mDeviceType, currentDisplayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthStencilFormat);

                        if (SUCCEEDED(result))
                        {
                            HRESULT result = mD3d9->CheckDepthStencilMatch(mAdapter, mDeviceType, currentDisplayMode.Format, renderTargetFormat, depthStencilFormat);   // FIXME: Only accept color formats available both in fullscreen and windowed?

                            if (SUCCEEDED(result))
                            {
                                // FIXME: Enumerate multi-sampling

                                mConfigSet.add(currentDisplayMode, minSwapInterval, maxSwapInterval, renderTargetFormat, depthStencilFormat, 0);
                            }
                        }
                    }
                }
            }
        }

        mConfigSet.enumerate();
    }

    if (!isInitialized())
    {
        terminate();

        return false;
    }

    return true;
}

void Display::terminate()
{
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        delete *surface;
    }

    for (ContextSet::iterator context = mContextSet.begin(); context != mContextSet.end(); context++)
    {
        glDestroyContext(*context);
    }

    if (mDevice)
    {
        mDevice->Release();
        mDevice = NULL;
    }

    if (mD3d9)
    {
        mD3d9->Release();
        mD3d9 = NULL;
    }
}

bool Display::getConfigs(EGLConfig *configs, const EGLint *attribList, EGLint configSize, EGLint *numConfig)
{
    return mConfigSet.getConfigs(configs, attribList, configSize, numConfig);
}

bool Display::getConfigAttrib(EGLConfig config, EGLint attribute, EGLint *value)
{
    const egl::Config *configuration = mConfigSet.get(config);

    switch (attribute)
    {
      case EGL_BUFFER_SIZE:               *value = configuration->mBufferSize;             break;
      case EGL_ALPHA_SIZE:                *value = configuration->mAlphaSize;              break;
      case EGL_BLUE_SIZE:                 *value = configuration->mBlueSize;               break;
      case EGL_GREEN_SIZE:                *value = configuration->mGreenSize;              break;
      case EGL_RED_SIZE:                  *value = configuration->mRedSize;                break;
      case EGL_DEPTH_SIZE:                *value = configuration->mDepthSize;              break;
      case EGL_STENCIL_SIZE:              *value = configuration->mStencilSize;            break;
      case EGL_CONFIG_CAVEAT:             *value = configuration->mConfigCaveat;           break;
      case EGL_CONFIG_ID:                 *value = configuration->mConfigID;               break;
      case EGL_LEVEL:                     *value = configuration->mLevel;                  break;
      case EGL_NATIVE_RENDERABLE:         *value = configuration->mNativeRenderable;       break;
      case EGL_NATIVE_VISUAL_TYPE:        *value = configuration->mNativeVisualType;       break;
      case EGL_SAMPLES:                   *value = configuration->mSamples;                break;
      case EGL_SAMPLE_BUFFERS:            *value = configuration->mSampleBuffers;          break;
      case EGL_SURFACE_TYPE:              *value = configuration->mSurfaceType;            break;
      case EGL_TRANSPARENT_TYPE:          *value = configuration->mTransparentType;        break;
      case EGL_TRANSPARENT_BLUE_VALUE:    *value = configuration->mTransparentBlueValue;   break;
      case EGL_TRANSPARENT_GREEN_VALUE:   *value = configuration->mTransparentGreenValue;  break;
      case EGL_TRANSPARENT_RED_VALUE:     *value = configuration->mTransparentRedValue;    break;
      case EGL_BIND_TO_TEXTURE_RGB:       *value = configuration->mBindToTextureRGB;       break;
      case EGL_BIND_TO_TEXTURE_RGBA:      *value = configuration->mBindToTextureRGBA;      break;
      case EGL_MIN_SWAP_INTERVAL:         *value = configuration->mMinSwapInterval;        break;
      case EGL_MAX_SWAP_INTERVAL:         *value = configuration->mMaxSwapInterval;        break;
      case EGL_LUMINANCE_SIZE:            *value = configuration->mLuminanceSize;          break;
      case EGL_ALPHA_MASK_SIZE:           *value = configuration->mAlphaMaskSize;          break;
      case EGL_COLOR_BUFFER_TYPE:         *value = configuration->mColorBufferType;        break;
      case EGL_RENDERABLE_TYPE:           *value = configuration->mRenderableType;         break;
      case EGL_MATCH_NATIVE_PIXMAP:       *value = false; UNIMPLEMENTED();                break;
      case EGL_CONFORMANT:                *value = configuration->mConformant;             break;
      default:
        return false;
    }

    return true;
}

egl::Surface *Display::createWindowSurface(HWND window, EGLConfig config)
{
    const egl::Config *configuration = mConfigSet.get(config);

    D3DPRESENT_PARAMETERS presentParameters = {0};

    presentParameters.AutoDepthStencilFormat = configuration->mDepthStencilFormat;
    presentParameters.BackBufferCount = 1;
    presentParameters.BackBufferFormat = configuration->mRenderTargetFormat;
    presentParameters.BackBufferWidth = 0;
    presentParameters.BackBufferHeight = 0;
    presentParameters.EnableAutoDepthStencil = configuration->mDepthSize ? TRUE : FALSE;
    presentParameters.Flags = 0;
    presentParameters.hDeviceWindow = window;
    presentParameters.MultiSampleQuality = 0;                  // FIXME: Unimplemented
    presentParameters.MultiSampleType = D3DMULTISAMPLE_NONE;   // FIXME: Unimplemented
    presentParameters.PresentationInterval = configuration->mMinSwapInterval;
    presentParameters.SwapEffect = D3DSWAPEFFECT_COPY;
    presentParameters.Windowed = TRUE;   // FIXME

    IDirect3DSwapChain9 *swapChain = NULL;
    IDirect3DSurface9 *depthStencilSurface = NULL;

    if (!mDevice)
    {
        UINT adapter = D3DADAPTER_DEFAULT;
        D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;
        DWORD behaviorFlags = D3DCREATE_FPU_PRESERVE | D3DCREATE_NOWINDOWCHANGES;

        HRESULT result = mD3d9->CreateDevice(adapter, deviceType, window, behaviorFlags | D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE, &presentParameters, &mDevice);

        if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
        {
            return error(EGL_BAD_ALLOC, (egl::Surface*)NULL);
        }

        if (FAILED(result))
        {
            result = mD3d9->CreateDevice(adapter, deviceType, window, behaviorFlags | D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParameters, &mDevice);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                return error(EGL_BAD_ALLOC, (egl::Surface*)NULL);
            }
        }

        ASSERT(SUCCEEDED(result));

        if (mDevice)
        {
            mDevice->GetSwapChain(0, &swapChain);
            mDevice->GetDepthStencilSurface(&depthStencilSurface);
        }
    }
    else
    {
        if (!mSurfaceSet.empty())
        {
            // if the device already exists, and there are other surfaces/windows currently in use, we need to create
            // a separate swap chain for the new draw surface.
            HRESULT result = mDevice->CreateAdditionalSwapChain(&presentParameters, &swapChain);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                ERR("Could not create additional swap chains. Out of memory.");
                return error(EGL_BAD_ALLOC, (egl::Surface*)NULL);
            }

            ASSERT(SUCCEEDED(result));

            // CreateAdditionalSwapChain does not automatically generate a depthstencil surface, unlike 
            // CreateDevice, so we must do so explicitly.
            result = mDevice->CreateDepthStencilSurface(presentParameters.BackBufferWidth, presentParameters.BackBufferHeight,
                                                        presentParameters.AutoDepthStencilFormat, presentParameters.MultiSampleType,
                                                        presentParameters.MultiSampleQuality, FALSE, &depthStencilSurface, NULL);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                swapChain->Release();
                ERR("Could not create depthstencil surface for new swap chain. Out of memory.");
                return error(EGL_BAD_ALLOC, (egl::Surface*)NULL);
            }

            ASSERT(SUCCEEDED(result));
        }
        else
        {
            // if the device already exists, but there are no surfaces in use, then all the surfaces/windows
            // have been destroyed, and we should repurpose the originally created depthstencil surface for
            // use with the new surface we are creating.
            HRESULT result = mDevice->Reset(&presentParameters);

            if (result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY)
            {
                ERR("Could not resent presentation parameters for device. Out of memory.");
                return error(EGL_BAD_ALLOC, (egl::Surface*)NULL);
            }

            ASSERT(SUCCEEDED(result));

            if (mDevice)
            {
                mDevice->GetSwapChain(0, &swapChain);
                mDevice->GetDepthStencilSurface(&depthStencilSurface);
            }
        }
    }

    Surface *surface = NULL;

    if (swapChain)
    {
        surface = new Surface(mDevice, swapChain, depthStencilSurface, configuration->mConfigID);
        mSurfaceSet.insert(surface);

        swapChain->Release();
    }

    return surface;
}

EGLContext Display::createContext(EGLConfig configHandle)
{
    const egl::Config *config = mConfigSet.get(configHandle);

    gl::Context *context = glCreateContext(config);
    mContextSet.insert(context);

    return context;
}

void Display::destroySurface(egl::Surface *surface)
{
    delete surface;
    mSurfaceSet.erase(surface);
}

void Display::destroyContext(gl::Context *context)
{
    glDestroyContext(context);
    mContextSet.erase(context);
}

bool Display::isInitialized()
{
    return mD3d9 != NULL && mConfigSet.size() > 0;
}

bool Display::isValidConfig(EGLConfig config)
{
    return mConfigSet.get(config) != NULL;
}

bool Display::isValidContext(gl::Context *context)
{
    return mContextSet.find(context) != mContextSet.end();
}

bool Display::isValidSurface(egl::Surface *surface)
{
    return mSurfaceSet.find(surface) != mSurfaceSet.end();
}

bool Display::hasExistingWindowSurface(HWND window)
{
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        if ((*surface)->getWindowHandle() == window)
        {
            return true;
        }
    }

    return false;
}

IDirect3DDevice9 *Display::getDevice()
{
    return mDevice;
}
}

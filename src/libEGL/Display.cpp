//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.cpp: Implements the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#include "libEGL/Display.h"

#include <algorithm>
#include <map>
#include <vector>

#include "common/debug.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/utilities.h"

#include "libEGL/main.h"


namespace egl
{
namespace
{
    typedef std::map<EGLNativeDisplayType, Display*> DisplayMap; 
    DisplayMap displays;
}

egl::Display *Display::getDisplay(EGLNativeDisplayType displayId)
{
    if (displays.find(displayId) != displays.end())
    {
        return displays[displayId];
    }

    egl::Display *display = NULL;

    if (displayId == EGL_DEFAULT_DISPLAY)
    {
        display = new egl::Display(displayId, (HDC)NULL, false);
    }
    else if (displayId == EGL_SOFTWARE_DISPLAY_ANGLE)
    {
        display = new egl::Display(displayId, (HDC)NULL, true);
    }
    else
    {
        // FIXME: Check if displayId is a valid display device context

        display = new egl::Display(displayId, (HDC)displayId, false);
    }

    displays[displayId] = display;
    return display;
}

Display::Display(EGLNativeDisplayType displayId, HDC deviceContext, bool software) : mDc(deviceContext)
{

    mMinSwapInterval = 1;
    mMaxSwapInterval = 1;
    mSoftwareDevice = software;
    mDisplayId = displayId;
    mRenderer = NULL;
}

Display::~Display()
{
    terminate();

    DisplayMap::iterator thisDisplay = displays.find(mDisplayId);

    if (thisDisplay != displays.end())
    {
      displays.erase(thisDisplay);
    }
}

bool Display::initialize()
{
    if (isInitialized())
    {
        return true;
    }

    HMODULE hModule = NULL;
    if (mSoftwareDevice)
    {
      hModule = GetModuleHandle(TEXT("swiftshader_d3d9.dll"));
    } 
    else
    {
      hModule = GetModuleHandle(TEXT("d3d9.dll"));
    }
    if (hModule == NULL)
    {
        terminate();
        return false;
    }

    mRenderer = glCreateRenderer(hModule, mDc);
    EGLint status = EGL_BAD_ALLOC;
    if (mRenderer)
        status = mRenderer->initialize();
    if (status != EGL_SUCCESS)
    {
        terminate();
        return error(status, false);
    }
    D3DCAPS9 deviceCaps = mRenderer->getDeviceCaps();  // D3D9_REPLACE
    IDirect3D9 *d3d9 = mRenderer->getD3D(); // D3D9_REPLACE
    UINT adapter = mRenderer->getAdapter(); // D3D9_REPLACE
    D3DDEVTYPE deviceType = mRenderer->getDeviceType(); // D3D9_REPLACE
    IDirect3DDevice9 *device = mRenderer->getDevice(); // D3D9_REPLACE

    mMinSwapInterval = 4;
    mMaxSwapInterval = 0;

    // START D3D9_REPLACE
    if (deviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_IMMEDIATE) {mMinSwapInterval = std::min(mMinSwapInterval, 0); mMaxSwapInterval = std::max(mMaxSwapInterval, 0);}
    if (deviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_ONE)       {mMinSwapInterval = std::min(mMinSwapInterval, 1); mMaxSwapInterval = std::max(mMaxSwapInterval, 1);}
    if (deviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_TWO)       {mMinSwapInterval = std::min(mMinSwapInterval, 2); mMaxSwapInterval = std::max(mMaxSwapInterval, 2);}
    if (deviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_THREE)     {mMinSwapInterval = std::min(mMinSwapInterval, 3); mMaxSwapInterval = std::max(mMaxSwapInterval, 3);}
    if (deviceCaps.PresentationIntervals & D3DPRESENT_INTERVAL_FOUR)      {mMinSwapInterval = std::min(mMinSwapInterval, 4); mMaxSwapInterval = std::max(mMaxSwapInterval, 4);}

    const D3DFORMAT renderTargetFormats[] =
    {
        D3DFMT_A1R5G5B5,
    //  D3DFMT_A2R10G10B10,   // The color_ramp conformance test uses ReadPixels with UNSIGNED_BYTE causing it to think that rendering skipped a colour value.
        D3DFMT_A8R8G8B8,
        D3DFMT_R5G6B5,
    //  D3DFMT_X1R5G5B5,      // Has no compatible OpenGL ES renderbuffer format
        D3DFMT_X8R8G8B8
    };

    const D3DFORMAT depthStencilFormats[] =
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

    D3DDISPLAYMODE currentDisplayMode;
    d3d9->GetAdapterDisplayMode(adapter, &currentDisplayMode);

    ConfigSet configSet;

    for (int formatIndex = 0; formatIndex < sizeof(renderTargetFormats) / sizeof(D3DFORMAT); formatIndex++)
    {
        D3DFORMAT renderTargetFormat = renderTargetFormats[formatIndex];

        HRESULT result = d3d9->CheckDeviceFormat(adapter, deviceType, currentDisplayMode.Format, D3DUSAGE_RENDERTARGET, D3DRTYPE_SURFACE, renderTargetFormat);

        if (SUCCEEDED(result))
        {
            for (int depthStencilIndex = 0; depthStencilIndex < sizeof(depthStencilFormats) / sizeof(D3DFORMAT); depthStencilIndex++)
            {
                D3DFORMAT depthStencilFormat = depthStencilFormats[depthStencilIndex];
                HRESULT result = D3D_OK;
                    
                if(depthStencilFormat != D3DFMT_UNKNOWN)
                {
                    result = d3d9->CheckDeviceFormat(adapter, deviceType, currentDisplayMode.Format, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, depthStencilFormat);
                }

                if (SUCCEEDED(result))
                {
                    if(depthStencilFormat != D3DFMT_UNKNOWN)
                    {
                        result = d3d9->CheckDepthStencilMatch(adapter, deviceType, currentDisplayMode.Format, renderTargetFormat, depthStencilFormat);
                    }

                    if (SUCCEEDED(result))
                    {
                        // FIXME: enumerate multi-sampling

                        configSet.add(currentDisplayMode, mMinSwapInterval, mMaxSwapInterval, renderTargetFormat, depthStencilFormat, 0,
                                        deviceCaps.MaxTextureWidth, deviceCaps.MaxTextureHeight);
                    }
                }
            }
        }
    }
    // END D3D9_REPLACE

    // Give the sorted configs a unique ID and store them internally
    EGLint index = 1;
    for (ConfigSet::Iterator config = configSet.mSet.begin(); config != configSet.mSet.end(); config++)
    {
        Config configuration = *config;
        configuration.mConfigID = index;
        index++;

        mConfigSet.mSet.insert(configuration);
    }

    if (!isInitialized())
    {
        terminate();
        return false;
    }

    initExtensionString();

    mVertexShaderCache.initialize(device);
    mPixelShaderCache.initialize(device);

    return true;
}

void Display::terminate()
{
    while (!mSurfaceSet.empty())
    {
        destroySurface(*mSurfaceSet.begin());
    }

    while (!mContextSet.empty())
    {
        destroyContext(*mContextSet.begin());
    }

    while (!mEventQueryPool.empty())
    {
        mEventQueryPool.back()->Release();
        mEventQueryPool.pop_back();
    }

    mVertexShaderCache.clear();
    mPixelShaderCache.clear();

    glDestroyRenderer(mRenderer);
    mRenderer = NULL;
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
      case EGL_MATCH_NATIVE_PIXMAP:       *value = false; UNIMPLEMENTED();                 break;
      case EGL_CONFORMANT:                *value = configuration->mConformant;             break;
      case EGL_MAX_PBUFFER_WIDTH:         *value = configuration->mMaxPBufferWidth;        break;
      case EGL_MAX_PBUFFER_HEIGHT:        *value = configuration->mMaxPBufferHeight;       break;
      case EGL_MAX_PBUFFER_PIXELS:        *value = configuration->mMaxPBufferPixels;       break;
      default:
        return false;
    }

    return true;
}



EGLSurface Display::createWindowSurface(HWND window, EGLConfig config, const EGLint *attribList)
{
    const Config *configuration = mConfigSet.get(config);
    EGLint postSubBufferSupported = EGL_FALSE;

    if (attribList)
    {
        while (*attribList != EGL_NONE)
        {
            switch (attribList[0])
            {
              case EGL_RENDER_BUFFER:
                switch (attribList[1])
                {
                  case EGL_BACK_BUFFER:
                    break;
                  case EGL_SINGLE_BUFFER:
                    return error(EGL_BAD_MATCH, EGL_NO_SURFACE);   // Rendering directly to front buffer not supported
                  default:
                    return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
                }
                break;
              case EGL_POST_SUB_BUFFER_SUPPORTED_NV:
                postSubBufferSupported = attribList[1];
                break;
              case EGL_VG_COLORSPACE:
                return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
              case EGL_VG_ALPHA_FORMAT:
                return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
              default:
                return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
            }

            attribList += 2;
        }
    }

    if (hasExistingWindowSurface(window))
    {
        return error(EGL_BAD_ALLOC, EGL_NO_SURFACE);
    }

    if (mRenderer->testDeviceLost())
    {
        if (!restoreLostDevice())
            return EGL_NO_SURFACE;
    }

    Surface *surface = new Surface(this, configuration, window, postSubBufferSupported);

    if (!surface->initialize())
    {
        delete surface;
        return EGL_NO_SURFACE;
    }

    mSurfaceSet.insert(surface);

    return success(surface);
}

EGLSurface Display::createOffscreenSurface(EGLConfig config, HANDLE shareHandle, const EGLint *attribList)
{
    EGLint width = 0, height = 0;
    EGLenum textureFormat = EGL_NO_TEXTURE;
    EGLenum textureTarget = EGL_NO_TEXTURE;
    const Config *configuration = mConfigSet.get(config);

    if (attribList)
    {
        while (*attribList != EGL_NONE)
        {
            switch (attribList[0])
            {
              case EGL_WIDTH:
                width = attribList[1];
                break;
              case EGL_HEIGHT:
                height = attribList[1];
                break;
              case EGL_LARGEST_PBUFFER:
                if (attribList[1] != EGL_FALSE)
                  UNIMPLEMENTED(); // FIXME
                break;
              case EGL_TEXTURE_FORMAT:
                switch (attribList[1])
                {
                  case EGL_NO_TEXTURE:
                  case EGL_TEXTURE_RGB:
                  case EGL_TEXTURE_RGBA:
                    textureFormat = attribList[1];
                    break;
                  default:
                    return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
                }
                break;
              case EGL_TEXTURE_TARGET:
                switch (attribList[1])
                {
                  case EGL_NO_TEXTURE:
                  case EGL_TEXTURE_2D:
                    textureTarget = attribList[1];
                    break;
                  default:
                    return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
                }
                break;
              case EGL_MIPMAP_TEXTURE:
                if (attribList[1] != EGL_FALSE)
                  return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
                break;
              case EGL_VG_COLORSPACE:
                return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
              case EGL_VG_ALPHA_FORMAT:
                return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
              default:
                return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
            }

            attribList += 2;
        }
    }

    if (width < 0 || height < 0)
    {
        return error(EGL_BAD_PARAMETER, EGL_NO_SURFACE);
    }

    if (width == 0 || height == 0)
    {
        return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
    }

    if (textureFormat != EGL_NO_TEXTURE && !mRenderer->getNonPower2TextureSupport() && (!gl::isPow2(width) || !gl::isPow2(height)))
    {
        return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
    }

    if ((textureFormat != EGL_NO_TEXTURE && textureTarget == EGL_NO_TEXTURE) ||
        (textureFormat == EGL_NO_TEXTURE && textureTarget != EGL_NO_TEXTURE))
    {
        return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
    }

    if (!(configuration->mSurfaceType & EGL_PBUFFER_BIT))
    {
        return error(EGL_BAD_MATCH, EGL_NO_SURFACE);
    }

    if ((textureFormat == EGL_TEXTURE_RGB && configuration->mBindToTextureRGB != EGL_TRUE) ||
        (textureFormat == EGL_TEXTURE_RGBA && configuration->mBindToTextureRGBA != EGL_TRUE))
    {
        return error(EGL_BAD_ATTRIBUTE, EGL_NO_SURFACE);
    }

    if (mRenderer->testDeviceLost())
    {
        if (!restoreLostDevice())
            return EGL_NO_SURFACE;
    }

    Surface *surface = new Surface(this, configuration, shareHandle, width, height, textureFormat, textureTarget);

    if (!surface->initialize())
    {
        delete surface;
        return EGL_NO_SURFACE;
    }

    mSurfaceSet.insert(surface);

    return success(surface);
}

EGLContext Display::createContext(EGLConfig configHandle, const gl::Context *shareContext, bool notifyResets, bool robustAccess)
{
    if (!mRenderer)
    {
        return NULL;
    }
    else if (mRenderer->testDeviceLost())   // Lost device
    {
        if (!restoreLostDevice())
            return NULL;
    }

    const egl::Config *config = mConfigSet.get(configHandle);

    gl::Context *context = glCreateContext(config, shareContext, notifyResets, robustAccess);
    mContextSet.insert(context);

    return context;
}

bool Display::restoreLostDevice()
{
    for (ContextSet::iterator ctx = mContextSet.begin(); ctx != mContextSet.end(); ctx++)
    {
        if ((*ctx)->isResetNotificationEnabled())
            return false;   // If reset notifications have been requested, application must delete all contexts first
    }
 
    // Release surface resources to make the Reset() succeed
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        (*surface)->release();
    }

    while (!mEventQueryPool.empty())
    {
        mEventQueryPool.back()->Release();
        mEventQueryPool.pop_back();
    }

    mVertexShaderCache.clear();
    mPixelShaderCache.clear();

    if (!mRenderer->resetDevice())
    {
        return error(EGL_BAD_ALLOC, false);
    }

    // Restore any surfaces that may have been lost
    for (SurfaceSet::iterator surface = mSurfaceSet.begin(); surface != mSurfaceSet.end(); surface++)
    {
        (*surface)->resetSwapChain();
    }

    return true;
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

void Display::notifyDeviceLost()
{
    for (ContextSet::iterator context = mContextSet.begin(); context != mContextSet.end(); context++)
    {
        (*context)->markContextLost();
    }
    mRenderer->markDeviceLost();
    error(EGL_CONTEXT_LOST);
}

bool Display::isInitialized() const
{
    return mRenderer != NULL && mConfigSet.size() > 0;
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

EGLint Display::getMinSwapInterval()
{
    return mMinSwapInterval;
}

EGLint Display::getMaxSwapInterval()
{
    return mMaxSwapInterval;
}

// D3D9_REPLACE
void Display::sync(bool block)
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
            if (mRenderer->testDeviceLost())
            {
                result = D3DERR_DEVICELOST;
            }
        }
    }
    while(block && result == S_FALSE);

    freeEventQuery(query);

    if (isDeviceLostError(result))
    {
        notifyDeviceLost();
    }
}

// D3D9_REPLACE
IDirect3DQuery9* Display::allocateEventQuery()
{
    IDirect3DQuery9 *query = NULL;

    if (mEventQueryPool.empty())
    {
        HRESULT result = mRenderer->getDevice()->CreateQuery(D3DQUERYTYPE_EVENT, &query);
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
void Display::freeEventQuery(IDirect3DQuery9* query)
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


void Display::initExtensionString()
{
    HMODULE swiftShader = GetModuleHandle(TEXT("swiftshader_d3d9.dll"));
    bool shareHandleSupported = mRenderer->getShareHandleSupport();

    mExtensionString = "";

    // Multi-vendor (EXT) extensions
    mExtensionString += "EGL_EXT_create_context_robustness ";

    // ANGLE-specific extensions
    if (shareHandleSupported)
    {
        mExtensionString += "EGL_ANGLE_d3d_share_handle_client_buffer ";
    }

    mExtensionString += "EGL_ANGLE_query_surface_pointer ";

    if (swiftShader)
    {
        mExtensionString += "EGL_ANGLE_software_display ";
    }

    if (shareHandleSupported)
    {
        mExtensionString += "EGL_ANGLE_surface_d3d_texture_2d_share_handle ";
    }

    mExtensionString += "EGL_NV_post_sub_buffer";

    std::string::size_type end = mExtensionString.find_last_not_of(' ');
    if (end != std::string::npos)
    {
        mExtensionString.resize(end+1);
    }
}

const char *Display::getExtensionString() const
{
    return mExtensionString.c_str();
}

IDirect3DVertexShader9 *Display::createVertexShader(const DWORD *function, size_t length)
{
    return mVertexShaderCache.create(function, length);
}

IDirect3DPixelShader9 *Display::createPixelShader(const DWORD *function, size_t length)
{
    return mPixelShaderCache.create(function, length);
}


}

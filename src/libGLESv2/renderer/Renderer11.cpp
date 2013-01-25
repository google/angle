//
// Copyright (c) 2012-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.cpp: Implements a back-end specific class for the D3D11 renderer.

#include "common/debug.h"
#include "libGLESv2/main.h"
#include "libGLESv2/utilities.h"
#include "libGLESv2/mathutil.h"
#include "libGLESv2/Buffer.h"
#include "libGLESv2/Program.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/Framebuffer.h"
#include "libGLESv2/renderer/Renderer11.h"
#include "libGLESv2/renderer/RenderTarget11.h"
#include "libGLESv2/renderer/renderer11_utils.h"
#include "libGLESv2/renderer/ShaderExecutable11.h"
#include "libGLESv2/renderer/SwapChain11.h"
#include "libGLESv2/renderer/Image11.h"
#include "libGLESv2/renderer/VertexBuffer11.h"
#include "libGLESv2/renderer/IndexBuffer11.h"
#include "libGLESv2/renderer/VertexDataManager.h"
#include "libGLESv2/renderer/IndexDataManager.h"
#include "libGLESv2/renderer/TextureStorage11.h"

#include "libGLESv2/renderer/shaders/compiled/passthrough11vs.h"
#include "libGLESv2/renderer/shaders/compiled/passthroughrgba11ps.h"
#include "libGLESv2/renderer/shaders/compiled/passthroughrgb11ps.h"
#include "libGLESv2/renderer/shaders/compiled/passthroughlum11ps.h"
#include "libGLESv2/renderer/shaders/compiled/passthroughlumalpha11ps.h"

#include "libGLESv2/renderer/shaders/compiled/clear11vs.h"
#include "libGLESv2/renderer/shaders/compiled/clear11ps.h"

#include <sstream>

namespace rx
{
static const DXGI_FORMAT RenderTargetFormats[] =
    {
        DXGI_FORMAT_B8G8R8A8_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM
    };

static const DXGI_FORMAT DepthStencilFormats[] =
    {
        DXGI_FORMAT_D24_UNORM_S8_UINT
    };

enum
{
    MAX_TEXTURE_IMAGE_UNITS_VTF_SM4 = 16
};

Renderer11::Renderer11(egl::Display *display, HDC hDc) : Renderer(display), mDc(hDc)
{
    mVertexDataManager = NULL;
    mIndexDataManager = NULL;

    mLineLoopIB = NULL;
    mTriangleFanIB = NULL;

    mCopyResourcesInitialized = false;
    mCopyVB = NULL;
    mCopySampler = NULL;
    mCopyIL = NULL;
    mCopyVS = NULL;
    mCopyRGBAPS = NULL;
    mCopyRGBPS = NULL;
    mCopyLumPS = NULL;
    mCopyLumAlphaPS = NULL;

    mClearResourcesInitialized = false;
    mClearVB = NULL;
    mClearIL = NULL;
    mClearVS = NULL;
    mClearPS = NULL;
    mClearScissorRS = NULL;
    mClearNoScissorRS = NULL;

    mD3d11Module = NULL;
    mDxgiModule = NULL;

    mDeviceLost = false;

    mDevice = NULL;
    mDeviceContext = NULL;
    mDxgiAdapter = NULL;
    mDxgiFactory = NULL;

    mDriverConstantBufferVS = NULL;
    mDriverConstantBufferPS = NULL;

    mBGRATextureSupport = false;
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
        mDeviceContext->ClearState();
        mDeviceContext->Flush();
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

Renderer11 *Renderer11::makeRenderer11(Renderer *renderer)
{
    ASSERT(dynamic_cast<rx::Renderer11*>(renderer) != NULL);
    return static_cast<rx::Renderer11*>(renderer);
}

EGLint Renderer11::initialize()
{
    if (!initializeCompiler())
    {
        return EGL_NOT_INITIALIZED;
    }

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
                                       #if defined(_DEBUG)
                                       D3D11_CREATE_DEVICE_DEBUG,
                                       #else
                                       0,
                                       #endif
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

    mDxgiAdapter->GetDesc(&mAdapterDescription);
    memset(mDescription, 0, sizeof(mDescription));
    wcstombs(mDescription, mAdapterDescription.Description, sizeof(mDescription) - 1);

    result = mDxgiAdapter->GetParent(__uuidof(IDXGIFactory), (void**)&mDxgiFactory);

    if (!mDxgiFactory || FAILED(result))
    {
        ERR("Could not create DXGI factory - aborting!\n");
        return EGL_NOT_INITIALIZED;
    }

    initializeDevice();

    // BGRA texture support is optional in feature levels 10 and 10_1
    UINT formatSupport;
    result = mDevice->CheckFormatSupport(DXGI_FORMAT_B8G8R8A8_UNORM, &formatSupport);
    if (FAILED(result))
    {
        ERR("Error checking BGRA format support: 0x%08X", result);
    }
    else
    {
        const int flags = (D3D11_FORMAT_SUPPORT_TEXTURE2D | D3D11_FORMAT_SUPPORT_RENDER_TARGET);
        mBGRATextureSupport = (formatSupport & flags) == flags;
    }

    return EGL_SUCCESS;
}

// do any one-time device initialization
// NOTE: this is also needed after a device lost/reset
// to reset the scene status and ensure the default states are reset.
void Renderer11::initializeDevice()
{
    mStateCache.initialize(mDevice);
    mInputLayoutCache.initialize(mDevice, mDeviceContext);

    ASSERT(!mVertexDataManager && !mIndexDataManager);
    mVertexDataManager = new VertexDataManager(this);
    mIndexDataManager = new IndexDataManager(this);

    markAllStateDirty();
}

int Renderer11::generateConfigs(ConfigDesc **configDescList)
{
    unsigned int numRenderFormats = sizeof(RenderTargetFormats) / sizeof(RenderTargetFormats[0]);
    unsigned int numDepthFormats = sizeof(DepthStencilFormats) / sizeof(DepthStencilFormats[0]);
    (*configDescList) = new ConfigDesc[numRenderFormats * numDepthFormats];
    int numConfigs = 0;
    
    for (unsigned int formatIndex = 0; formatIndex < numRenderFormats; formatIndex++)
    {
        for (unsigned int depthStencilIndex = 0; depthStencilIndex < numDepthFormats; depthStencilIndex++)
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

void Renderer11::sync(bool block)
{
    // TODO
    // UNIMPLEMENTED();
}

SwapChain *Renderer11::createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat)
{
    return new rx::SwapChain11(this, window, shareHandle, backBufferFormat, depthBufferFormat);
}

void Renderer11::setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &samplerState)
{
    if (type == gl::SAMPLER_PIXEL)
    {
        if (index < 0 || index >= gl::MAX_TEXTURE_IMAGE_UNITS)
        {
            ERR("Pixel shader sampler index %i is not valid.", index);
            return;
        }

        if (mForceSetPixelSamplerStates[index] || memcmp(&samplerState, &mCurPixelSamplerStates[index], sizeof(gl::SamplerState)) != 0)
        {
            ID3D11SamplerState *dxSamplerState = mStateCache.getSamplerState(samplerState);

            if (!dxSamplerState)
            {
                ERR("NULL sampler state returned by RenderStateCache::getSamplerState, setting the default"
                    "sampler state for pixel shaders at slot %i.", index);
            }

            mDeviceContext->PSSetSamplers(index, 1, &dxSamplerState);

            mCurPixelSamplerStates[index] = samplerState;
        }

        mForceSetPixelSamplerStates[index] = false;
    }
    else if (type == gl::SAMPLER_VERTEX)
    {
        if (index < 0 || index >= (int)getMaxVertexTextureImageUnits())
        {
            ERR("Vertex shader sampler index %i is not valid.", index);
            return;
        }

        if (mForceSetVertexSamplerStates[index] || memcmp(&samplerState, &mCurVertexSamplerStates[index], sizeof(gl::SamplerState)) != 0)
        {
            ID3D11SamplerState *dxSamplerState = mStateCache.getSamplerState(samplerState);

            if (!dxSamplerState)
            {
                ERR("NULL sampler state returned by RenderStateCache::getSamplerState, setting the default"
                    "sampler state for vertex shaders at slot %i.", index);
            }

            mDeviceContext->VSSetSamplers(index, 1, &dxSamplerState);

            mCurVertexSamplerStates[index] = samplerState;
        }

        mForceSetVertexSamplerStates[index] = false;
    }
    else UNREACHABLE();
}

void Renderer11::setTexture(gl::SamplerType type, int index, gl::Texture *texture)
{
    ID3D11ShaderResourceView *textureSRV = NULL;
    unsigned int serial = 0;
    bool forceSetTexture = false;

    if (texture)
    {
        TextureStorageInterface *texStorage = texture->getNativeTexture();
        if (texStorage)
        {
            TextureStorage11 *storage11 = TextureStorage11::makeTextureStorage11(texStorage->getStorageInstance());
            textureSRV = storage11->getSRV();
        }

        // If we get NULL back from getSRV here, something went wrong in the texture class and we're unexpectedly
        // missing the shader resource view
        ASSERT(textureSRV != NULL);

        serial = texture->getTextureSerial();
        forceSetTexture = texture->hasDirtyImages();
    }

    if (type == gl::SAMPLER_PIXEL)
    {
        if (index < 0 || index >= gl::MAX_TEXTURE_IMAGE_UNITS)
        {
            ERR("Pixel shader sampler index %i is not valid.", index);
            return;
        }

        if (forceSetTexture || mCurPixelTextureSerials[index] != serial)
        {
            mDeviceContext->PSSetShaderResources(index, 1, &textureSRV);
        }

        mCurPixelTextureSerials[index] = serial;
    }
    else if (type == gl::SAMPLER_VERTEX)
    {
        if (index < 0 || index >= (int)getMaxVertexTextureImageUnits())
        {
            ERR("Vertex shader sampler index %i is not valid.", index);
            return;
        }

        if (forceSetTexture || mCurVertexTextureSerials[index] != serial)
        {
            mDeviceContext->VSSetShaderResources(index, 1, &textureSRV);
        }

        mCurVertexTextureSerials[index] = serial;
    }
    else UNREACHABLE();
}

void Renderer11::setRasterizerState(const gl::RasterizerState &rasterState)
{
    if (mForceSetRasterState || memcmp(&rasterState, &mCurRasterState, sizeof(gl::RasterizerState)) != 0)
    {
        ID3D11RasterizerState *dxRasterState = mStateCache.getRasterizerState(rasterState, mScissorEnabled,
                                                                              mCurDepthSize);
        if (!dxRasterState)
        {
            ERR("NULL rasterizer state returned by RenderStateCache::getRasterizerState, setting the default"
                "rasterizer state.");
        }

        mDeviceContext->RSSetState(dxRasterState);

        mCurRasterState = rasterState;
    }

    mForceSetRasterState = false;
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

        mCurBlendState = blendState;
        mCurBlendColor = blendColor;
        mCurSampleMask = sampleMask;
    }

    mForceSetBlendState = false;
}

void Renderer11::setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW)
{
    if (mForceSetDepthStencilState ||
        memcmp(&depthStencilState, &mCurDepthStencilState, sizeof(gl::DepthStencilState)) != 0 ||
        stencilRef != mCurStencilRef || stencilBackRef != mCurStencilBackRef)
    {
        if (depthStencilState.stencilWritemask != depthStencilState.stencilBackWritemask ||
            stencilRef != stencilBackRef ||
            depthStencilState.stencilMask != depthStencilState.stencilBackMask)
        {
            ERR("Separate front/back stencil writemasks, reference values, or stencil mask values are "
                "invalid under WebGL.");
            return error(GL_INVALID_OPERATION);
        }

        ID3D11DepthStencilState *dxDepthStencilState = mStateCache.getDepthStencilState(depthStencilState);
        if (!dxDepthStencilState)
        {
            ERR("NULL depth stencil state returned by RenderStateCache::getDepthStencilState, "
                "setting the default depth stencil state.");
        }

        mDeviceContext->OMSetDepthStencilState(dxDepthStencilState, static_cast<UINT>(stencilRef));

        mCurDepthStencilState = depthStencilState;
        mCurStencilRef = stencilRef;
        mCurStencilBackRef = stencilBackRef;
    }

    mForceSetDepthStencilState = false;
}

void Renderer11::setScissorRectangle(const gl::Rectangle &scissor, bool enabled)
{
    if (mForceSetScissor || memcmp(&scissor, &mCurScissor, sizeof(gl::Rectangle)) != 0 ||
        enabled != mScissorEnabled)
    {
        if (enabled)
        {
            D3D11_RECT rect;
            rect.left = gl::clamp(scissor.x, 0, static_cast<int>(mRenderTargetDesc.width));
            rect.top = gl::clamp(scissor.y, 0, static_cast<int>(mRenderTargetDesc.height));
            rect.right = gl::clamp(scissor.x + scissor.width, 0, static_cast<int>(mRenderTargetDesc.width));
            rect.bottom = gl::clamp(scissor.y + scissor.height, 0, static_cast<int>(mRenderTargetDesc.height));

            mDeviceContext->RSSetScissorRects(1, &rect);
        }

        if (enabled != mScissorEnabled)
        {
            mForceSetRasterState = true;
        }

        mCurScissor = scissor;
        mScissorEnabled = enabled;
    }

    mForceSetScissor = false;
}

bool Renderer11::setViewport(const gl::Rectangle &viewport, float zNear, float zFar, GLenum drawMode, GLenum frontFace, 
                             bool ignoreViewport, gl::ProgramBinary *currentProgram)
{
    gl::Rectangle actualViewport = viewport;
    float actualZNear = gl::clamp01(zNear);
    float actualZFar = gl::clamp01(zFar);
    if (ignoreViewport)
    {
        actualViewport.x = 0;
        actualViewport.y = 0;
        actualViewport.width = mRenderTargetDesc.width;
        actualViewport.height = mRenderTargetDesc.height;
        actualZNear = 0.0f;
        actualZFar = 1.0f;
    }

    D3D11_VIEWPORT dxViewport;
    dxViewport.TopLeftX = gl::clamp(actualViewport.x, 0, static_cast<int>(mRenderTargetDesc.width));
    dxViewport.TopLeftY = gl::clamp(actualViewport.y, 0, static_cast<int>(mRenderTargetDesc.height));
    dxViewport.Width = gl::clamp(actualViewport.width, 0, static_cast<int>(mRenderTargetDesc.width) - static_cast<int>(dxViewport.TopLeftX));
    dxViewport.Height = gl::clamp(actualViewport.height, 0, static_cast<int>(mRenderTargetDesc.height) - static_cast<int>(dxViewport.TopLeftY));
    dxViewport.MinDepth = actualZNear;
    dxViewport.MaxDepth = actualZFar;

    if (dxViewport.Width <= 0 || dxViewport.Height <= 0)
    {
        return false;   // Nothing to render
    }

    bool viewportChanged = mForceSetViewport || memcmp(&actualViewport, &mCurViewport, sizeof(gl::Rectangle)) != 0 ||
                           actualZNear != mCurNear || actualZFar != mCurFar;

    if (viewportChanged)
    {
        mDeviceContext->RSSetViewports(1, &dxViewport);

        mCurViewport = actualViewport;
        mCurNear = actualZNear;
        mCurFar = actualZFar;
    }

    if (currentProgram && viewportChanged)
    {
        mVertexConstants.halfPixelSize[0] = 0.0f;
        mVertexConstants.halfPixelSize[1] = 0.0f;

        mPixelConstants.coord[0] = actualViewport.width  * 0.5f;
        mPixelConstants.coord[1] = actualViewport.height * 0.5f;
        mPixelConstants.coord[2] = actualViewport.x + (actualViewport.width  * 0.5f);
        mPixelConstants.coord[3] = actualViewport.y + (actualViewport.height * 0.5f);

        mPixelConstants.depthFront[0] = (actualZFar - actualZNear) * 0.5f;
        mPixelConstants.depthFront[1] = (actualZNear + actualZFar) * 0.5f;
        mPixelConstants.depthFront[2] = !gl::IsTriangleMode(drawMode) ? 0.0f : (frontFace == GL_CCW ? 1.0f : -1.0f);;

        mVertexConstants.depthRange[0] = actualZNear;
        mVertexConstants.depthRange[1] = actualZFar;
        mVertexConstants.depthRange[2] = actualZFar - actualZNear;

        mPixelConstants.depthRange[0] = actualZNear;
        mPixelConstants.depthRange[1] = actualZFar;
        mPixelConstants.depthRange[2] = actualZFar - actualZNear;
    }

    mForceSetViewport = false;
    return true;
}

bool Renderer11::applyPrimitiveType(GLenum mode, GLsizei count)
{
    D3D11_PRIMITIVE_TOPOLOGY primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;

    switch (mode)
    {
      case GL_POINTS:         primitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;   break;
      case GL_LINES:          primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;      break;
      case GL_LINE_LOOP:      primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;     break;
      case GL_LINE_STRIP:     primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;     break;
      case GL_TRIANGLES:      primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;  break;
      case GL_TRIANGLE_STRIP: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
          // emulate fans via rewriting index buffer
      case GL_TRIANGLE_FAN:   primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;	break;
      default:
        return error(GL_INVALID_ENUM, false);
    }

    mDeviceContext->IASetPrimitiveTopology(primitiveTopology);

    return count > 0;
}

bool Renderer11::applyRenderTarget(gl::Framebuffer *framebuffer)
{
    // Get the color render buffer and serial
    gl::Renderbuffer *renderbufferObject = NULL;
    unsigned int renderTargetSerial = 0;
    if (framebuffer->getColorbufferType() != GL_NONE)
    {
        renderbufferObject = framebuffer->getColorbuffer();

        if (!renderbufferObject)
        {
            ERR("render target pointer unexpectedly null.");
            return false;
        }

        renderTargetSerial = renderbufferObject->getSerial();
    }

    // Get the depth stencil render buffer and serials
    gl::Renderbuffer *depthStencil = NULL;
    unsigned int depthbufferSerial = 0;
    unsigned int stencilbufferSerial = 0;
    if (framebuffer->getDepthbufferType() != GL_NONE)
    {
        depthStencil = framebuffer->getDepthbuffer();
        if (!depthStencil)
        {
            ERR("Depth stencil pointer unexpectedly null.");
            return false;
        }

        depthbufferSerial = depthStencil->getSerial();
    }
    else if (framebuffer->getStencilbufferType() != GL_NONE)
    {
        depthStencil = framebuffer->getStencilbuffer();
        if (!depthStencil)
        {
            ERR("Depth stencil pointer unexpectedly null.");
            return false;
        }

        stencilbufferSerial = depthStencil->getSerial();
    }

    // Extract the render target dimensions and view
    unsigned int renderTargetWidth = 0;
    unsigned int renderTargetHeight = 0;
    GLenum renderTargetFormat = 0;
    ID3D11RenderTargetView* framebufferRTV = NULL;
    if (renderbufferObject)
    {
        RenderTarget11 *renderTarget = RenderTarget11::makeRenderTarget11(renderbufferObject->getRenderTarget());
        if (!renderTarget)
        {
            ERR("render target pointer unexpectedly null.");
            return false;
        }

        framebufferRTV = renderTarget->getRenderTargetView();
        if (!framebufferRTV)
        {
            ERR("render target view pointer unexpectedly null.");
            return false;
        }

        renderTargetWidth = renderbufferObject->getWidth();
        renderTargetHeight = renderbufferObject->getHeight();
        renderTargetFormat = renderbufferObject->getActualFormat();
    }

    // Extract the depth stencil sizes and view
    unsigned int depthSize = 0;
    unsigned int stencilSize = 0;
    ID3D11DepthStencilView* framebufferDSV = NULL;
    if (depthStencil)
    {
        RenderTarget11 *depthStencilRenderTarget = RenderTarget11::makeRenderTarget11(depthStencil->getDepthStencil());
        if (!depthStencilRenderTarget)
        {
            ERR("render target pointer unexpectedly null.");
            if (framebufferRTV)
            {
                framebufferRTV->Release();
            }
            return false;
        }

        framebufferDSV = depthStencilRenderTarget->getDepthStencilView();
        if (!framebufferDSV)
        {
            ERR("depth stencil view pointer unexpectedly null.");
            if (framebufferRTV)
            {
                framebufferRTV->Release();
            }
            return false;
        }

        // If there is no render buffer, the width, height and format values come from
        // the depth stencil
        if (!renderbufferObject)
        {
            renderTargetWidth = depthStencil->getWidth();
            renderTargetHeight = depthStencil->getHeight();
            renderTargetFormat = depthStencil->getActualFormat();
        }

        depthSize = depthStencil->getDepthSize();
        stencilSize = depthStencil->getStencilSize();
    }

    // Apply the render target and depth stencil
    if (!mRenderTargetDescInitialized || !mDepthStencilInitialized ||
        renderTargetSerial != mAppliedRenderTargetSerial ||
        depthbufferSerial != mAppliedDepthbufferSerial ||
        stencilbufferSerial != mAppliedStencilbufferSerial)
    {
        mDeviceContext->OMSetRenderTargets(1, &framebufferRTV, framebufferDSV);

        mRenderTargetDesc.width = renderTargetWidth;
        mRenderTargetDesc.height = renderTargetHeight;
        mRenderTargetDesc.format = renderTargetFormat;
        mForceSetViewport = true; // TODO: It may not be required to clamp the viewport in D3D11
        mForceSetScissor = true; // TODO: It may not be required to clamp the scissor in D3D11

        if (!mDepthStencilInitialized || depthSize != mCurDepthSize)
        {
            mCurDepthSize = depthSize;
            mForceSetRasterState = true;
        }

        mCurStencilSize = stencilSize;

        mAppliedRenderTargetSerial = renderTargetSerial;
        mAppliedDepthbufferSerial = depthbufferSerial;
        mAppliedStencilbufferSerial = stencilbufferSerial;
        mRenderTargetDescInitialized = true;
        mDepthStencilInitialized = true;
    }

    if (framebufferRTV)
    {
        framebufferRTV->Release();
    }
    if (framebufferDSV)
    {
        framebufferDSV->Release();
    }

    return true;
}

GLenum Renderer11::applyVertexBuffer(gl::ProgramBinary *programBinary, gl::VertexAttribute vertexAttributes[], GLint first, GLsizei count, GLsizei instances)
{
    TranslatedAttribute attributes[gl::MAX_VERTEX_ATTRIBS];
    GLenum err = mVertexDataManager->prepareVertexData(vertexAttributes, programBinary, first, count, attributes, instances);
    if (err != GL_NO_ERROR)
    {
        return err;
    }

    return mInputLayoutCache.applyVertexBuffers(attributes, programBinary);
}

GLenum Renderer11::applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, TranslatedIndexData *indexInfo)
{
    GLenum err = mIndexDataManager->prepareIndexData(type, count, elementArrayBuffer, indices, indexInfo);

    if (err == GL_NO_ERROR)
    {
        if (indexInfo->serial != mAppliedIBSerial || indexInfo->startOffset != mAppliedIBOffset)
        {
            IndexBuffer11* indexBuffer = IndexBuffer11::makeIndexBuffer11(indexInfo->indexBuffer);

            mDeviceContext->IASetIndexBuffer(indexBuffer->getBuffer(), indexBuffer->getIndexFormat(), indexInfo->startOffset);
            mAppliedIBSerial = indexInfo->serial;
            mAppliedIBOffset = indexInfo->startOffset;
        }
    }

    return err;
}

void Renderer11::drawArrays(GLenum mode, GLsizei count, GLsizei instances)
{
    if (mode == GL_LINE_LOOP)
    {
        drawLineLoop(count, GL_NONE, NULL, 0, NULL);
    }
    else if (mode == GL_TRIANGLE_FAN)
    {
        drawTriangleFan(count, GL_NONE, NULL, 0, NULL);
    }
    else if (instances > 0)
    {
        // TODO
        UNIMPLEMENTED();
    }
    else
    {
        mDeviceContext->Draw(count, 0);
    }
}

void Renderer11::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo)
{
    if (mode == GL_LINE_LOOP)
    {
        drawLineLoop(count, type, indices, indexInfo.minIndex, elementArrayBuffer);
    }
    else if (mode == GL_TRIANGLE_FAN)
    {
        drawTriangleFan(count, type, indices, indexInfo.minIndex, elementArrayBuffer);
    }
    else
    {
        mDeviceContext->DrawIndexed(count, 0, -static_cast<int>(indexInfo.minIndex));
    }
}

void Renderer11::drawLineLoop(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer)
{
    // Get the raw indices for an indexed draw
    if (type != GL_NONE && elementArrayBuffer)
    {
        gl::Buffer *indexBuffer = elementArrayBuffer;
        intptr_t offset = reinterpret_cast<intptr_t>(indices);
        indices = static_cast<const GLubyte*>(indexBuffer->data()) + offset;
    }

    if (!mLineLoopIB)
    {
        mLineLoopIB = new StreamingIndexBufferInterface(this);
        if (!mLineLoopIB->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT))
        {
            delete mLineLoopIB;
            mLineLoopIB = NULL;

            ERR("Could not create a 32-bit looping index buffer for GL_LINE_LOOP.");
            return error(GL_OUT_OF_MEMORY);
        }
    }

    const int spaceNeeded = (count + 1) * sizeof(unsigned int);
    if (!mLineLoopIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT))
    {
        ERR("Could not reserve enough space in looping index buffer for GL_LINE_LOOP.");
        return error(GL_OUT_OF_MEMORY);
    }

    void* mappedMemory = NULL;
    int offset = mLineLoopIB->mapBuffer(spaceNeeded, &mappedMemory);
    if (offset == -1 || mappedMemory == NULL)
    {
        ERR("Could not map index buffer for GL_LINE_LOOP.");
        return error(GL_OUT_OF_MEMORY);
    }

    unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);
    unsigned int indexBufferOffset = static_cast<unsigned int>(offset);

    switch (type)
    {
      case GL_NONE:   // Non-indexed draw
        for (int i = 0; i < count; i++)
        {
            data[i] = i;
        }
        data[count] = 0;
        break;
      case GL_UNSIGNED_BYTE:
        for (int i = 0; i < count; i++)
        {
            data[i] = static_cast<const GLubyte*>(indices)[i];
        }
        data[count] = static_cast<const GLubyte*>(indices)[0];
        break;
      case GL_UNSIGNED_SHORT:
        for (int i = 0; i < count; i++)
        {
            data[i] = static_cast<const GLushort*>(indices)[i];
        }
        data[count] = static_cast<const GLushort*>(indices)[0];
        break;
      case GL_UNSIGNED_INT:
        for (int i = 0; i < count; i++)
        {
            data[i] = static_cast<const GLuint*>(indices)[i];
        }
        data[count] = static_cast<const GLuint*>(indices)[0];
        break;
      default: UNREACHABLE();
    }

    if (!mLineLoopIB->unmapBuffer())
    {
        ERR("Could not unmap index buffer for GL_LINE_LOOP.");
        return error(GL_OUT_OF_MEMORY);
    }

    if (mAppliedIBSerial != mLineLoopIB->getSerial() || mAppliedIBOffset != indexBufferOffset)
    {
        IndexBuffer11 *indexBuffer = IndexBuffer11::makeIndexBuffer11(mLineLoopIB->getIndexBuffer());

        mDeviceContext->IASetIndexBuffer(indexBuffer->getBuffer(), indexBuffer->getIndexFormat(), indexBufferOffset);
        mAppliedIBSerial = mLineLoopIB->getSerial();
        mAppliedIBOffset = indexBufferOffset;
    }

    mDeviceContext->DrawIndexed(count + 1, 0, -minIndex);
}

void Renderer11::drawTriangleFan(GLsizei count, GLenum type, const GLvoid *indices, int minIndex, gl::Buffer *elementArrayBuffer)
{
    // Get the raw indices for an indexed draw
    if (type != GL_NONE && elementArrayBuffer)
    {
        gl::Buffer *indexBuffer = elementArrayBuffer;
        intptr_t offset = reinterpret_cast<intptr_t>(indices);
        indices = static_cast<const GLubyte*>(indexBuffer->data()) + offset;
    }

    if (!mTriangleFanIB)
    {
        mTriangleFanIB = new StreamingIndexBufferInterface(this);
        if (!mTriangleFanIB->reserveBufferSpace(INITIAL_INDEX_BUFFER_SIZE, GL_UNSIGNED_INT))
        {
            delete mTriangleFanIB;
            mTriangleFanIB = NULL;

            ERR("Could not create a scratch index buffer for GL_TRIANGLE_FAN.");
            return error(GL_OUT_OF_MEMORY);
        }
    }

    const int numTris = count - 2;
    const int spaceNeeded = (numTris * 3) * sizeof(unsigned int);
    if (!mTriangleFanIB->reserveBufferSpace(spaceNeeded, GL_UNSIGNED_INT))
    {
        ERR("Could not reserve enough space in scratch index buffer for GL_TRIANGLE_FAN.");
        return error(GL_OUT_OF_MEMORY);
    }

    void* mappedMemory = NULL;
    int offset = mTriangleFanIB->mapBuffer(spaceNeeded, &mappedMemory);
    if (offset == -1 || mappedMemory == NULL)
    {
        ERR("Could not map scratch index buffer for GL_TRIANGLE_FAN.");
        return error(GL_OUT_OF_MEMORY);
    }

    unsigned int *data = reinterpret_cast<unsigned int*>(mappedMemory);
    unsigned int indexBufferOffset = static_cast<unsigned int>(offset);

    switch (type)
    {
      case GL_NONE:   // Non-indexed draw
        for (int i = 0; i < numTris; i++)
        {
            data[i*3 + 0] = 0;
            data[i*3 + 1] = i + 1;
            data[i*3 + 2] = i + 2;
        }
        break;
      case GL_UNSIGNED_BYTE:
        for (int i = 0; i < numTris; i++)
        {
            data[i*3 + 0] = static_cast<const GLubyte*>(indices)[0];
            data[i*3 + 1] = static_cast<const GLubyte*>(indices)[i + 1];
            data[i*3 + 2] = static_cast<const GLubyte*>(indices)[i + 2];
        }
        break;
      case GL_UNSIGNED_SHORT:
        for (int i = 0; i < numTris; i++)
        {
            data[i*3 + 0] = static_cast<const GLushort*>(indices)[0];
            data[i*3 + 1] = static_cast<const GLushort*>(indices)[i + 1];
            data[i*3 + 2] = static_cast<const GLushort*>(indices)[i + 2];
        }
        break;
      case GL_UNSIGNED_INT:
        for (int i = 0; i < numTris; i++)
        {
            data[i*3 + 0] = static_cast<const GLuint*>(indices)[0];
            data[i*3 + 1] = static_cast<const GLuint*>(indices)[i + 1];
            data[i*3 + 2] = static_cast<const GLuint*>(indices)[i + 2];
        }
        break;
      default: UNREACHABLE();
    }

    if (!mTriangleFanIB->unmapBuffer())
    {
        ERR("Could not unmap scratch index buffer for GL_TRIANGLE_FAN.");
        return error(GL_OUT_OF_MEMORY);
    }

    if (mAppliedIBSerial != mTriangleFanIB->getSerial() || mAppliedIBOffset != indexBufferOffset)
    {
        IndexBuffer11 *indexBuffer = IndexBuffer11::makeIndexBuffer11(mTriangleFanIB->getIndexBuffer());

        mDeviceContext->IASetIndexBuffer(indexBuffer->getBuffer(), indexBuffer->getIndexFormat(), indexBufferOffset);
        mAppliedIBSerial = mTriangleFanIB->getSerial();
        mAppliedIBOffset = indexBufferOffset;
    }

    mDeviceContext->DrawIndexed(numTris * 3, 0, -minIndex);
}

void Renderer11::applyShaders(gl::ProgramBinary *programBinary)
{
    unsigned int programBinarySerial = programBinary->getSerial();
    if (programBinarySerial != mAppliedProgramBinarySerial)
    {
        ShaderExecutable *vertexExe = programBinary->getVertexExecutable();
        ShaderExecutable *pixelExe = programBinary->getPixelExecutable();

        ID3D11VertexShader *vertexShader = NULL;
        if (vertexExe) vertexShader = ShaderExecutable11::makeShaderExecutable11(vertexExe)->getVertexShader();

        ID3D11PixelShader *pixelShader = NULL;
        if (pixelExe) pixelShader = ShaderExecutable11::makeShaderExecutable11(pixelExe)->getPixelShader();

        mDeviceContext->PSSetShader(pixelShader, NULL, 0);
        mDeviceContext->VSSetShader(vertexShader, NULL, 0);
        programBinary->dirtyAllUniforms();

        mAppliedProgramBinarySerial = programBinarySerial;
    }
}

void Renderer11::applyUniforms(gl::ProgramBinary *programBinary, gl::UniformArray *uniformArray)
{
    ShaderExecutable11 *vertexExecutable = ShaderExecutable11::makeShaderExecutable11(programBinary->getVertexExecutable());
    ShaderExecutable11 *pixelExecutable = ShaderExecutable11::makeShaderExecutable11(programBinary->getPixelExecutable());

    unsigned int totalRegisterCountVS = 0;
    unsigned int totalRegisterCountPS = 0;

    bool vertexUniformsDirty = false;
    bool pixelUniformsDirty = false;

    for (gl::UniformArray::const_iterator uniform_iterator = uniformArray->begin(); uniform_iterator != uniformArray->end(); uniform_iterator++)
    {
        const gl::Uniform *uniform = *uniform_iterator;

        if (uniform->vsRegisterIndex >= 0)
        {
            totalRegisterCountVS += uniform->registerCount;
            vertexUniformsDirty = vertexUniformsDirty || uniform->dirty;
        }

        if (uniform->psRegisterIndex >= 0)
        {
            totalRegisterCountPS += uniform->registerCount;
            pixelUniformsDirty = pixelUniformsDirty || uniform->dirty;
        }
    }

    ID3D11Buffer *vertexConstantBuffer = vertexExecutable->getConstantBuffer(mDevice, totalRegisterCountVS);
    ID3D11Buffer *pixelConstantBuffer = pixelExecutable->getConstantBuffer(mDevice, totalRegisterCountPS);

    void *mapVS = (totalRegisterCountVS > 0 && vertexUniformsDirty) ? new float[4 * totalRegisterCountVS] : NULL;
    void *mapPS = (totalRegisterCountPS > 0 && pixelUniformsDirty) ? new float[4 * totalRegisterCountPS] : NULL;

    for (gl::UniformArray::iterator uniform_iterator = uniformArray->begin(); uniform_iterator != uniformArray->end(); uniform_iterator++)
    {
        gl::Uniform *uniform = *uniform_iterator;

        switch (uniform->type)
        {
          case GL_SAMPLER_2D:
          case GL_SAMPLER_CUBE:
            break;
          case GL_FLOAT:
          case GL_FLOAT_VEC2:
          case GL_FLOAT_VEC3:
          case GL_FLOAT_VEC4:
          case GL_FLOAT_MAT2:
          case GL_FLOAT_MAT3:
          case GL_FLOAT_MAT4:
            if (uniform->vsRegisterIndex >= 0 && mapVS)
            {
                GLfloat (*c)[4] = (GLfloat(*)[4])mapVS;
                float (*f)[4] = (float(*)[4])uniform->data;

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    c[uniform->vsRegisterIndex + i][0] = f[i][0];
                    c[uniform->vsRegisterIndex + i][1] = f[i][1];
                    c[uniform->vsRegisterIndex + i][2] = f[i][2];
                    c[uniform->vsRegisterIndex + i][3] = f[i][3];
                }
            }
            if (uniform->psRegisterIndex >= 0 && mapPS)
            {
                GLfloat (*c)[4] = (GLfloat(*)[4])mapPS;
                float (*f)[4] = (float(*)[4])uniform->data;

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    c[uniform->psRegisterIndex + i][0] = f[i][0];
                    c[uniform->psRegisterIndex + i][1] = f[i][1];
                    c[uniform->psRegisterIndex + i][2] = f[i][2];
                    c[uniform->psRegisterIndex + i][3] = f[i][3];
                }
            }
            break;
          case GL_INT:
          case GL_INT_VEC2:
          case GL_INT_VEC3:
          case GL_INT_VEC4:
            if (uniform->vsRegisterIndex >= 0 && mapVS)
            {
                int (*c)[4] = (int(*)[4])mapVS;
                GLint *x = (GLint*)uniform->data;
                int count = gl::VariableColumnCount(uniform->type);

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    if (count >= 1) c[uniform->vsRegisterIndex + i][0] = x[i * count + 0];
                    if (count >= 2) c[uniform->vsRegisterIndex + i][1] = x[i * count + 1];
                    if (count >= 3) c[uniform->vsRegisterIndex + i][2] = x[i * count + 2];
                    if (count >= 4) c[uniform->vsRegisterIndex + i][3] = x[i * count + 3];
                }
            }
            if (uniform->psRegisterIndex >= 0 && mapPS)
            {
                int (*c)[4] = (int(*)[4])mapPS;
                GLint *x = (GLint*)uniform->data;
                int count = gl::VariableColumnCount(uniform->type);

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    if (count >= 1) c[uniform->psRegisterIndex + i][0] = x[i * count + 0];
                    if (count >= 2) c[uniform->psRegisterIndex + i][1] = x[i * count + 1];
                    if (count >= 3) c[uniform->psRegisterIndex + i][2] = x[i * count + 2];
                    if (count >= 4) c[uniform->psRegisterIndex + i][3] = x[i * count + 3];
                }
            }
            break;
          case GL_BOOL:
          case GL_BOOL_VEC2:
          case GL_BOOL_VEC3:
          case GL_BOOL_VEC4:
            if (uniform->vsRegisterIndex >= 0 && mapVS)
            {
                int (*c)[4] = (int(*)[4])mapVS;
                GLboolean *b = (GLboolean*)uniform->data;
                int count = gl::VariableColumnCount(uniform->type);

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    if (count >= 1) c[uniform->vsRegisterIndex + i][0] = b[i * count + 0];
                    if (count >= 2) c[uniform->vsRegisterIndex + i][1] = b[i * count + 1];
                    if (count >= 3) c[uniform->vsRegisterIndex + i][2] = b[i * count + 2];
                    if (count >= 4) c[uniform->vsRegisterIndex + i][3] = b[i * count + 3];
                }
            }
            if (uniform->psRegisterIndex >= 0 && mapPS)
            {
                int (*c)[4] = (int(*)[4])mapPS;
                GLboolean *b = (GLboolean*)uniform->data;
                int count = gl::VariableColumnCount(uniform->type);

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    if (count >= 1) c[uniform->psRegisterIndex + i][0] = b[i * count + 0];
                    if (count >= 2) c[uniform->psRegisterIndex + i][1] = b[i * count + 1];
                    if (count >= 3) c[uniform->psRegisterIndex + i][2] = b[i * count + 2];
                    if (count >= 4) c[uniform->psRegisterIndex + i][3] = b[i * count + 3];
                }
            }
            break;
          default:
            UNREACHABLE();
        }

        uniform->dirty = false;
    }

    if (vertexConstantBuffer && mapVS)
    {
        mDeviceContext->UpdateSubresource(vertexConstantBuffer, 0, NULL, mapVS, 0, 0);
    }

    if (pixelConstantBuffer && mapPS)
    {
        mDeviceContext->UpdateSubresource(pixelConstantBuffer, 0, NULL, mapPS, 0, 0);
    }
    
    mDeviceContext->VSSetConstantBuffers(0, 1, &vertexConstantBuffer);
    mDeviceContext->PSSetConstantBuffers(0, 1, &pixelConstantBuffer);
    
    delete[] mapVS;
    delete[] mapPS;

    // Driver uniforms
    if (!mDriverConstantBufferVS)
    {
        D3D11_BUFFER_DESC constantBufferDescription = {0};
        constantBufferDescription.ByteWidth = sizeof(dx_VertexConstants);
        constantBufferDescription.Usage = D3D11_USAGE_DEFAULT;
        constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDescription.CPUAccessFlags = 0;
        constantBufferDescription.MiscFlags = 0;
        constantBufferDescription.StructureByteStride = 0;

        HRESULT result = mDevice->CreateBuffer(&constantBufferDescription, NULL, &mDriverConstantBufferVS);
        ASSERT(SUCCEEDED(result));

        mDeviceContext->VSSetConstantBuffers(1, 1, &mDriverConstantBufferVS);
    }

    if (!mDriverConstantBufferPS)
    {
        D3D11_BUFFER_DESC constantBufferDescription = {0};
        constantBufferDescription.ByteWidth = sizeof(dx_PixelConstants);
        constantBufferDescription.Usage = D3D11_USAGE_DEFAULT;
        constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        constantBufferDescription.CPUAccessFlags = 0;
        constantBufferDescription.MiscFlags = 0;
        constantBufferDescription.StructureByteStride = 0;

        HRESULT result = mDevice->CreateBuffer(&constantBufferDescription, NULL, &mDriverConstantBufferPS);
        ASSERT(SUCCEEDED(result));

        mDeviceContext->PSSetConstantBuffers(1, 1, &mDriverConstantBufferPS);
    }

    if (memcmp(&mVertexConstants, &mAppliedVertexConstants, sizeof(dx_VertexConstants)) != 0)
    {
        mDeviceContext->UpdateSubresource(mDriverConstantBufferVS, 0, NULL, &mVertexConstants, 16, 0);
        memcpy(&mAppliedVertexConstants, &mVertexConstants, sizeof(dx_VertexConstants));
    }

    if (memcmp(&mPixelConstants, &mAppliedPixelConstants, sizeof(dx_PixelConstants)) != 0)
    {
        mDeviceContext->UpdateSubresource(mDriverConstantBufferPS, 0, NULL, &mPixelConstants, 16, 0);
        memcpy(&mAppliedPixelConstants, &mPixelConstants, sizeof(dx_PixelConstants));
    }
}

void Renderer11::clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer)
{
     bool alphaUnmasked = (gl::GetAlphaSize(mRenderTargetDesc.format) == 0) || clearParams.colorMaskAlpha;
     bool needMaskedColorClear = (clearParams.mask & GL_COLOR_BUFFER_BIT) &&
                                 !(clearParams.colorMaskRed && clearParams.colorMaskGreen &&
                                   clearParams.colorMaskBlue && alphaUnmasked);

     unsigned int stencilUnmasked = 0x0;
     if (frameBuffer->hasStencil())
     {
         unsigned int stencilSize = gl::GetStencilSize(frameBuffer->getStencilbuffer()->getActualFormat());
         stencilUnmasked = (0x1 << stencilSize) - 1;
     }
     bool needMaskedStencilClear = (clearParams.mask & GL_STENCIL_BUFFER_BIT) &&
                                   (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

     bool needScissoredClear = mScissorEnabled && (mCurScissor.x > 0 || mCurScissor.y > 0 ||
                                                   mCurScissor.x + mCurScissor.width < mRenderTargetDesc.width ||
                                                   mCurScissor.y + mCurScissor.height < mRenderTargetDesc.height);

     if (needMaskedColorClear || needMaskedStencilClear || needScissoredClear)
     {
         maskedClear(clearParams);
     }
     else
     {
         if (clearParams.mask & GL_COLOR_BUFFER_BIT)
         {
             gl::Renderbuffer *renderbufferObject = frameBuffer->getColorbuffer();
             if (renderbufferObject)
             {
                RenderTarget11 *renderTarget = RenderTarget11::makeRenderTarget11(renderbufferObject->getRenderTarget());
                if (!renderTarget)
                {
                    ERR("render target pointer unexpectedly null.");
                    return;
                }

                ID3D11RenderTargetView *framebufferRTV = renderTarget->getRenderTargetView();
                if (!framebufferRTV)
                {
                    ERR("render target view pointer unexpectedly null.");
                    return;
                }

                const float clearValues[4] = { clearParams.colorClearValue.red,
                                               clearParams.colorClearValue.green,
                                               clearParams.colorClearValue.blue,
                                               clearParams.colorClearValue.alpha };
                mDeviceContext->ClearRenderTargetView(framebufferRTV, clearValues);

                framebufferRTV->Release();
            }
        }
        if (clearParams.mask & GL_DEPTH_BUFFER_BIT || clearParams.mask & GL_STENCIL_BUFFER_BIT)
        {
            gl::Renderbuffer *renderbufferObject = frameBuffer->getDepthOrStencilbuffer();
            if (renderbufferObject)
            {
                RenderTarget11 *renderTarget = RenderTarget11::makeRenderTarget11(renderbufferObject->getDepthStencil());
                if (!renderTarget)
                {
                    ERR("render target pointer unexpectedly null.");
                    return;
                }

                ID3D11DepthStencilView *framebufferDSV = renderTarget->getDepthStencilView();
                if (!framebufferDSV)
                {
                    ERR("depth stencil view pointer unexpectedly null.");
                    return;
                }

                UINT clearFlags = 0;
                if (clearParams.mask & GL_DEPTH_BUFFER_BIT)
                {
                    clearFlags |= D3D11_CLEAR_DEPTH;
                }
                if (clearParams.mask & GL_STENCIL_BUFFER_BIT)
                {
                    clearFlags |= D3D11_CLEAR_STENCIL;
                }

                float depthClear = clearParams.depthClearValue;
                UINT8 stencilClear = clearParams.stencilClearValue & 0x000000FF;

                mDeviceContext->ClearDepthStencilView(framebufferDSV, clearFlags, depthClear, stencilClear);

                framebufferDSV->Release();
            }
        }
    }
}

void Renderer11::maskedClear(const gl::ClearParameters &clearParams)
{
    HRESULT result;

    if (!mClearResourcesInitialized)
    {
        ASSERT(!mClearVB && !mClearVS && !mClearPS && !mClearScissorRS && !mClearNoScissorRS);

        D3D11_BUFFER_DESC vbDesc;
        vbDesc.ByteWidth = sizeof(d3d11::PositionDepthColorVertex) * 4;
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.MiscFlags = 0;
        vbDesc.StructureByteStride = 0;

        result = mDevice->CreateBuffer(&vbDesc, NULL, &mClearVB);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearVB, "Renderer11 masked clear vertex buffer");

        D3D11_INPUT_ELEMENT_DESC quadLayout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        result = mDevice->CreateInputLayout(quadLayout, 2, g_VS_Clear, sizeof(g_VS_Clear), &mClearIL);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearIL, "Renderer11 masked clear input layout");

        result = mDevice->CreateVertexShader(g_VS_Clear, sizeof(g_VS_Clear), NULL, &mClearVS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearVS, "Renderer11 masked clear vertex shader");

        result = mDevice->CreatePixelShader(g_PS_Clear, sizeof(g_PS_Clear), NULL, &mClearPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearPS, "Renderer11 masked clear pixel shader");

        D3D11_RASTERIZER_DESC rsScissorDesc;
        rsScissorDesc.FillMode = D3D11_FILL_SOLID;
        rsScissorDesc.CullMode = D3D11_CULL_NONE;
        rsScissorDesc.FrontCounterClockwise = FALSE;
        rsScissorDesc.DepthBias = 0;
        rsScissorDesc.DepthBiasClamp = 0.0f;
        rsScissorDesc.SlopeScaledDepthBias = 0.0f;
        rsScissorDesc.DepthClipEnable = FALSE;
        rsScissorDesc.ScissorEnable = TRUE;
        rsScissorDesc.MultisampleEnable = FALSE;
        rsScissorDesc.AntialiasedLineEnable = FALSE;

        result = mDevice->CreateRasterizerState(&rsScissorDesc, &mClearScissorRS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearScissorRS, "Renderer11 masked clear scissor rasterizer state");

        D3D11_RASTERIZER_DESC rsNoScissorDesc;
        rsNoScissorDesc.FillMode = D3D11_FILL_SOLID;
        rsNoScissorDesc.CullMode = D3D11_CULL_NONE;
        rsNoScissorDesc.FrontCounterClockwise = FALSE;
        rsNoScissorDesc.DepthBias = 0;
        rsNoScissorDesc.DepthBiasClamp = 0.0f;
        rsNoScissorDesc.SlopeScaledDepthBias = 0.0f;
        rsNoScissorDesc.DepthClipEnable = FALSE;
        rsNoScissorDesc.ScissorEnable = FALSE;
        rsNoScissorDesc.MultisampleEnable = FALSE;
        rsNoScissorDesc.AntialiasedLineEnable = FALSE;

        result = mDevice->CreateRasterizerState(&rsNoScissorDesc, &mClearNoScissorRS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mClearNoScissorRS, "Renderer11 masked clear no scissor rasterizer state");

        mClearResourcesInitialized = true;
    }

    // Prepare the depth stencil state to write depth values if the depth should be cleared
    // and stencil values if the stencil should be cleared
    gl::DepthStencilState glDSState;
    glDSState.depthTest = (clearParams.mask & GL_DEPTH_BUFFER_BIT) != 0;
    glDSState.depthFunc = GL_ALWAYS;
    glDSState.depthMask = (clearParams.mask & GL_DEPTH_BUFFER_BIT) != 0;
    glDSState.stencilTest = (clearParams.mask & GL_STENCIL_BUFFER_BIT) != 0;
    glDSState.stencilFunc = GL_ALWAYS;
    glDSState.stencilMask = 0;
    glDSState.stencilFail = GL_REPLACE;
    glDSState.stencilPassDepthFail = GL_REPLACE;
    glDSState.stencilPassDepthPass = GL_REPLACE;
    glDSState.stencilWritemask = clearParams.stencilWriteMask;
    glDSState.stencilBackFunc = GL_ALWAYS;
    glDSState.stencilBackMask = 0;
    glDSState.stencilBackFail = GL_REPLACE;
    glDSState.stencilBackPassDepthFail = GL_REPLACE;
    glDSState.stencilBackPassDepthPass = GL_REPLACE;
    glDSState.stencilBackWritemask = clearParams.stencilWriteMask;

    int stencilClear = clearParams.stencilClearValue & 0x000000FF;

    ID3D11DepthStencilState *dsState = mStateCache.getDepthStencilState(glDSState);

    // Prepare the blend state to use a write mask if the color buffer should be cleared
    gl::BlendState glBlendState;
    glBlendState.blend = false;
    glBlendState.sourceBlendRGB = GL_ONE;
    glBlendState.destBlendRGB = GL_ZERO;
    glBlendState.sourceBlendAlpha = GL_ONE;
    glBlendState.destBlendAlpha = GL_ZERO;
    glBlendState.blendEquationRGB = GL_FUNC_ADD;
    glBlendState.blendEquationAlpha = GL_FUNC_ADD;
    glBlendState.colorMaskRed = (clearParams.mask & GL_COLOR_BUFFER_BIT) ? clearParams.colorMaskRed : false;
    glBlendState.colorMaskGreen = (clearParams.mask & GL_COLOR_BUFFER_BIT) ? clearParams.colorMaskGreen : false;
    glBlendState.colorMaskBlue = (clearParams.mask & GL_COLOR_BUFFER_BIT) ? clearParams.colorMaskBlue : false;
    glBlendState.colorMaskBlue = (clearParams.mask & GL_COLOR_BUFFER_BIT) ? clearParams.colorMaskBlue : false;
    glBlendState.sampleAlphaToCoverage = false;
    glBlendState.dither = false;

    static const float blendFactors[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    static const UINT sampleMask = 0xFFFFFFFF;

    ID3D11BlendState *blendState = mStateCache.getBlendState(glBlendState);

    // Set the vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = mDeviceContext->Map(mClearVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        ERR("Failed to map masked clear vertex buffer, HRESULT: 0x%X.", result);
        return;
    }

    d3d11::PositionDepthColorVertex *vertices = reinterpret_cast<d3d11::PositionDepthColorVertex*>(mappedResource.pData);

    float depthClear = gl::clamp01(clearParams.depthClearValue);
    d3d11::SetPositionDepthColorVertex(&vertices[0], -1.0f,  1.0f, depthClear, clearParams.colorClearValue);
    d3d11::SetPositionDepthColorVertex(&vertices[1], -1.0f, -1.0f, depthClear, clearParams.colorClearValue);
    d3d11::SetPositionDepthColorVertex(&vertices[2],  1.0f,  1.0f, depthClear, clearParams.colorClearValue);
    d3d11::SetPositionDepthColorVertex(&vertices[3],  1.0f, -1.0f, depthClear, clearParams.colorClearValue);

    mDeviceContext->Unmap(mClearVB, 0);

    // Apply state
    mDeviceContext->OMSetBlendState(blendState, blendFactors, sampleMask);
    mDeviceContext->OMSetDepthStencilState(dsState, stencilClear);
    mDeviceContext->RSSetState(mScissorEnabled ? mClearScissorRS : mClearNoScissorRS);

    // Apply shaders
    mDeviceContext->IASetInputLayout(mClearIL);
    mDeviceContext->VSSetShader(mClearVS, NULL, 0);
    mDeviceContext->PSSetShader(mClearPS, NULL, 0);

    // Apply vertex buffer
    static UINT stride = sizeof(d3d11::PositionDepthColorVertex);
    static UINT startIdx = 0;
    mDeviceContext->IASetVertexBuffers(0, 1, &mClearVB, &stride, &startIdx);
    mDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Draw the clear quad
    mDeviceContext->Draw(4, 0);

    // Clean up
    markAllStateDirty();
}

void Renderer11::markAllStateDirty()
{
    mAppliedRenderTargetSerial = 0;
    mAppliedDepthbufferSerial = 0;
    mAppliedStencilbufferSerial = 0;
    mDepthStencilInitialized = false;
    mRenderTargetDescInitialized = false;

    for (int i = 0; i < gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS; i++)
    {
        mForceSetVertexSamplerStates[i] = true;
        mCurVertexTextureSerials[i] = 0;
    }
    for (int i = 0; i < gl::MAX_TEXTURE_IMAGE_UNITS; i++)
    {
        mForceSetPixelSamplerStates[i] = true;
        mCurPixelTextureSerials[i] = 0;
    }

    mForceSetBlendState = true;
    mForceSetRasterState = true;
    mForceSetDepthStencilState = true;
    mForceSetScissor = true;
    mForceSetViewport = true;

    mAppliedIBSerial = 0;
    mAppliedIBOffset = 0;

    mAppliedProgramBinarySerial = 0;
    memset(&mAppliedVertexConstants, 0, sizeof(dx_VertexConstants));
    memset(&mAppliedPixelConstants, 0, sizeof(dx_PixelConstants));
}

void Renderer11::releaseDeviceResources()
{
    mStateCache.clear();
    mInputLayoutCache.clear();

    delete mVertexDataManager;
    mVertexDataManager = NULL;

    delete mIndexDataManager;
    mIndexDataManager = NULL;

    delete mLineLoopIB;
    mLineLoopIB = NULL;

    delete mTriangleFanIB;
    mTriangleFanIB = NULL;

    if (mCopyVB)
    {
        mCopyVB->Release();
        mCopyVB = NULL;
    }

    if (mCopySampler)
    {
        mCopySampler->Release();
        mCopySampler = NULL;
    }

    if (mCopyIL)
    {
        mCopyIL->Release();
        mCopyIL = NULL;
    }

    if (mCopyVS)
    {
        mCopyVS->Release();
        mCopyVS = NULL;
    }

    if (mCopyRGBAPS)
    {
        mCopyRGBAPS->Release();
        mCopyRGBAPS = NULL;
    }

    if (mCopyRGBPS)
    {
        mCopyRGBPS->Release();
        mCopyRGBPS = NULL;
    }

    if (mCopyLumPS)
    {
        mCopyLumPS->Release();
        mCopyLumPS = NULL;
    }

    if (mCopyLumAlphaPS)
    {
        mCopyLumAlphaPS->Release();
        mCopyLumAlphaPS = NULL;
    }

    mCopyResourcesInitialized = false;

    if (mClearVB)
    {
        mClearVB->Release();
        mClearVB = NULL;
    }

    if (mClearIL)
    {
        mClearIL->Release();
        mClearIL = NULL;
    }

    if (mClearVS)
    {
        mClearVS->Release();
        mClearVS = NULL;
    }

    if (mClearPS)
    {
        mClearPS->Release();
        mClearPS = NULL;
    }

    if (mClearScissorRS)
    {
        mClearScissorRS->Release();
        mClearScissorRS = NULL;
    }

    if (mClearNoScissorRS)
    {
        mClearNoScissorRS->Release();
        mClearNoScissorRS = NULL;
    }

    mClearResourcesInitialized = false;

    if (mDriverConstantBufferVS)
    {
        mDriverConstantBufferVS->Release();
        mDriverConstantBufferVS = NULL;
    }

    if (mDriverConstantBufferPS)
    {
        mDriverConstantBufferPS->Release();
        mDriverConstantBufferPS = NULL;
    }
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

    return true;
}

DWORD Renderer11::getAdapterVendor() const
{
    return mAdapterDescription.VendorId;
}

std::string Renderer11::getRendererDescription() const
{
    std::ostringstream rendererString;

    rendererString << mDescription;
    rendererString << " Direct3D11";

    rendererString << " vs_" << getMajorShaderModel() << "_" << getMinorShaderModel();
    rendererString << " ps_" << getMajorShaderModel() << "_" << getMinorShaderModel();

    return rendererString.str();
}

GUID Renderer11::getAdapterIdentifier() const
{
    // TODO
    // UNIMPLEMENTED();
    GUID foo = {0};
    return foo;
}

bool Renderer11::getBGRATextureSupport() const
{
    return mBGRATextureSupport;
}

bool Renderer11::getDXT1TextureSupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDXT3TextureSupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDXT5TextureSupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getDepthTextureSupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getFloat32TextureSupport(bool *filtering, bool *renderable)
{
    // TODO
    // UNIMPLEMENTED();

    *filtering = false;
    *renderable = false;
    return false;
}

bool Renderer11::getFloat16TextureSupport(bool *filtering, bool *renderable)
{
    // TODO
    // UNIMPLEMENTED();

    *filtering = false;
    *renderable = false;
    return false;
}

bool Renderer11::getLuminanceTextureSupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getLuminanceAlphaTextureSupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getTextureFilterAnisotropySupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

float Renderer11::getTextureMaxAnisotropy() const
{
    // TODO
    // UNIMPLEMENTED();
    return 1.0f;
}

bool Renderer11::getEventQuerySupport()
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

unsigned int Renderer11::getMaxVertexTextureImageUnits() const
{
    META_ASSERT(MAX_TEXTURE_IMAGE_UNITS_VTF_SM4 <= gl::IMPLEMENTATION_MAX_VERTEX_TEXTURE_IMAGE_UNITS);
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0:
      case D3D_FEATURE_LEVEL_10_1:
      case D3D_FEATURE_LEVEL_10_0:
        return MAX_TEXTURE_IMAGE_UNITS_VTF_SM4;
      default: UNREACHABLE();
        return 0;
    }
}

int Renderer11::getMaxVertexUniformVectors() const
{
    return gl::MAX_VERTEX_UNIFORM_VECTORS;
}

int Renderer11::getMaxFragmentUniformVectors() const
{
    return getMajorShaderModel() >= 3 ? gl::MAX_FRAGMENT_UNIFORM_VECTORS_SM3 : gl::MAX_FRAGMENT_UNIFORM_VECTORS_SM2;
}

bool Renderer11::getNonPower2TextureSupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getOcclusionQuerySupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getInstancingSupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

bool Renderer11::getShareHandleSupport() const
{
    // We only currently support share handles with BGRA surfaces, because
    // chrome needs BGRA. Once chrome fixes this, we should always support them.
    // PIX doesn't seem to support using share handles, so disable them.
    return getBGRATextureSupport() && !gl::perfActive();
}

bool Renderer11::getDerivativeInstructionSupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
}

int Renderer11::getMajorShaderModel() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: return D3D11_SHADER_MAJOR_VERSION;   // 5
      case D3D_FEATURE_LEVEL_10_1: return D3D10_1_SHADER_MAJOR_VERSION; // 4
      case D3D_FEATURE_LEVEL_10_0: return D3D10_SHADER_MAJOR_VERSION;   // 4
      default: UNREACHABLE();      return 0;
    }
}

int Renderer11::getMinorShaderModel() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: return D3D11_SHADER_MINOR_VERSION;   // 0
      case D3D_FEATURE_LEVEL_10_1: return D3D10_1_SHADER_MINOR_VERSION; // 1
      case D3D_FEATURE_LEVEL_10_0: return D3D10_SHADER_MINOR_VERSION;   // 0
      default: UNREACHABLE();      return 0;
    }
}

float Renderer11::getMaxPointSize() const
{
    // TODO
    // UNIMPLEMENTED();
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
    // UNIMPLEMENTED();
    return 1;
}

bool Renderer11::copyToRenderTarget(TextureStorageInterface2D *dest, TextureStorageInterface2D *source)
{
    if (source && dest)
    {
        TextureStorage11_2D *source11 = TextureStorage11_2D::makeTextureStorage11_2D(source->getStorageInstance());
        TextureStorage11_2D *dest11 = TextureStorage11_2D::makeTextureStorage11_2D(dest->getStorageInstance());

        mDeviceContext->CopyResource(dest11->getBaseTexture(), source11->getBaseTexture());
        return true;
    }

    return false;
}

bool Renderer11::copyToRenderTarget(TextureStorageInterfaceCube *dest, TextureStorageInterfaceCube *source)
{
    if (source && dest)
    {
        TextureStorage11_Cube *source11 = TextureStorage11_Cube::makeTextureStorage11_Cube(source->getStorageInstance());
        TextureStorage11_Cube *dest11 = TextureStorage11_Cube::makeTextureStorage11_Cube(dest->getStorageInstance());

        mDeviceContext->CopyResource(dest11->getBaseTexture(), source11->getBaseTexture());
        return true;
    }

    return false;
}

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level)
{
    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer();
    if (!colorbuffer)
    {
        ERR("Failed to retrieve the color buffer from the frame buffer.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    RenderTarget11 *sourceRenderTarget = RenderTarget11::makeRenderTarget11(colorbuffer->getRenderTarget());
    if (!sourceRenderTarget)
    {
        ERR("Failed to retrieve the render target from the frame buffer.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    ID3D11ShaderResourceView *source = sourceRenderTarget->getShaderResourceView();
    if (!source)
    {
        ERR("Failed to retrieve the render target view from the render target.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    TextureStorage11_2D *storage11 = TextureStorage11_2D::makeTextureStorage11_2D(storage->getStorageInstance());
    if (!storage11)
    {
        source->Release();
        ERR("Failed to retrieve the texture storage from the destination.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    RenderTarget11 *destRenderTarget = RenderTarget11::makeRenderTarget11(storage11->getRenderTarget(level));
    if (!destRenderTarget)
    {
        source->Release();
        ERR("Failed to retrieve the render target from the destination storage.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    ID3D11RenderTargetView *dest = destRenderTarget->getRenderTargetView();
    if (!dest)
    {
        source->Release();
        ERR("Failed to retrieve the render target view from the destination render target.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    gl::Rectangle destRect;
    destRect.x = xoffset;
    destRect.y = yoffset;
    destRect.width = sourceRect.width;
    destRect.height = sourceRect.height;

    bool ret = copyTexture(source, sourceRect, sourceRenderTarget->getWidth(), sourceRenderTarget->getHeight(),
                           dest, destRect, destRenderTarget->getWidth(), destRenderTarget->getHeight(), destFormat);

    source->Release();
    dest->Release();

    return ret;
}

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const gl::Rectangle &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level)
{
    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer();
    if (!colorbuffer)
    {
        ERR("Failed to retrieve the color buffer from the frame buffer.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    RenderTarget11 *sourceRenderTarget = RenderTarget11::makeRenderTarget11(colorbuffer->getRenderTarget());
    if (!sourceRenderTarget)
    {
        ERR("Failed to retrieve the render target from the frame buffer.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    ID3D11ShaderResourceView *source = sourceRenderTarget->getShaderResourceView();
    if (!source)
    {
        ERR("Failed to retrieve the render target view from the render target.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    TextureStorage11_Cube *storage11 = TextureStorage11_Cube::makeTextureStorage11_Cube(storage->getStorageInstance());
    if (!storage11)
    {
        source->Release();
        ERR("Failed to retrieve the texture storage from the destination.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    RenderTarget11 *destRenderTarget = RenderTarget11::makeRenderTarget11(storage11->getRenderTarget(target, level));
    if (!destRenderTarget)
    {
        source->Release();
        ERR("Failed to retrieve the render target from the destination storage.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    ID3D11RenderTargetView *dest = destRenderTarget->getRenderTargetView();
    if (!dest)
    {
        source->Release();
        ERR("Failed to retrieve the render target view from the destination render target.");
        return error(GL_OUT_OF_MEMORY, false);
    }

    gl::Rectangle destRect;
    destRect.x = xoffset;
    destRect.y = yoffset;
    destRect.width = sourceRect.width;
    destRect.height = sourceRect.height;

    bool ret = copyTexture(source, sourceRect, sourceRenderTarget->getWidth(), sourceRenderTarget->getHeight(),
                           dest, destRect, destRenderTarget->getWidth(), destRenderTarget->getHeight(), destFormat);

    source->Release();
    dest->Release();

    return ret;
}

bool Renderer11::copyTexture(ID3D11ShaderResourceView *source, const gl::Rectangle &sourceArea, unsigned int sourceWidth, unsigned int sourceHeight,
                             ID3D11RenderTargetView *dest, const gl::Rectangle &destArea, unsigned int destWidth, unsigned int destHeight, GLenum destFormat)
{
    HRESULT result;

    if (!mCopyResourcesInitialized)
    {
        ASSERT(!mCopyVB && !mCopySampler && !mCopyIL && !mCopyVS && !mCopyRGBAPS && !mCopyRGBPS && !mCopyLumPS && !mCopyLumAlphaPS);

        D3D11_BUFFER_DESC vbDesc;
        vbDesc.ByteWidth = sizeof(d3d11::PositionTexCoordVertex) * 4;
        vbDesc.Usage = D3D11_USAGE_DYNAMIC;
        vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        vbDesc.MiscFlags = 0;
        vbDesc.StructureByteStride = 0;

        result = mDevice->CreateBuffer(&vbDesc, NULL, &mCopyVB);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyVB, "Renderer11 copy texture vertex buffer");

        D3D11_SAMPLER_DESC samplerDesc;
        samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        samplerDesc.MipLODBias = 0.0f;
        samplerDesc.MaxAnisotropy = 0;
        samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
        samplerDesc.BorderColor[0] = 0.0f;
        samplerDesc.BorderColor[1] = 0.0f;
        samplerDesc.BorderColor[2] = 0.0f;
        samplerDesc.BorderColor[3] = 0.0f;
        samplerDesc.MinLOD = 0.0f;
        samplerDesc.MaxLOD = 0.0f;

        result = mDevice->CreateSamplerState(&samplerDesc, &mCopySampler);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopySampler, "Renderer11 copy sampler");

        D3D11_INPUT_ELEMENT_DESC quadLayout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        result = mDevice->CreateInputLayout(quadLayout, 2, g_VS_Passthrough, sizeof(g_VS_Passthrough), &mCopyIL);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyIL, "Renderer11 copy texture input layout");

        result = mDevice->CreateVertexShader(g_VS_Passthrough, sizeof(g_VS_Passthrough), NULL, &mCopyVS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyVS, "Renderer11 copy texture vertex shader");

        result = mDevice->CreatePixelShader(g_PS_PassthroughRGBA, sizeof(g_PS_PassthroughRGBA), NULL, &mCopyRGBAPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyRGBAPS, "Renderer11 copy texture RGBA pixel shader");

        result = mDevice->CreatePixelShader(g_PS_PassthroughRGB, sizeof(g_PS_PassthroughRGB), NULL, &mCopyRGBPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyRGBPS, "Renderer11 copy texture RGB pixel shader");

        result = mDevice->CreatePixelShader(g_PS_PassthroughLum, sizeof(g_PS_PassthroughLum), NULL, &mCopyLumPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyLumPS, "Renderer11 copy texture luminance pixel shader");

        result = mDevice->CreatePixelShader(g_PS_PassthroughLumAlpha, sizeof(g_PS_PassthroughLumAlpha), NULL, &mCopyLumAlphaPS);
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mCopyLumAlphaPS, "Renderer11 copy texture luminance alpha pixel shader");

        mCopyResourcesInitialized = true;
    }

    // Verify the source and destination area sizes
    if (sourceArea.x < 0 || sourceArea.x + sourceArea.width > static_cast<int>(sourceWidth) ||
        sourceArea.y < 0 || sourceArea.y + sourceArea.height > static_cast<int>(sourceHeight) ||
        destArea.x < 0 || destArea.x + destArea.width > static_cast<int>(destWidth) ||
        destArea.y < 0 || destArea.y + destArea.height > static_cast<int>(destHeight))
    {
        return error(GL_INVALID_VALUE, false);
    }

    // Set vertices
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    result = mDeviceContext->Map(mCopyVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (FAILED(result))
    {
        ERR("Failed to map vertex buffer for texture copy, HRESULT: 0x%X.", result);
        return error(GL_OUT_OF_MEMORY, false);
    }

    d3d11::PositionTexCoordVertex *vertices = static_cast<d3d11::PositionTexCoordVertex*>(mappedResource.pData);

    // Create a quad in homogeneous coordinates
    float x1 = (destArea.x / destWidth) * 2.0f - 1.0f;
    float y1 = (destArea.y / destHeight) * 2.0f - 1.0f;
    float x2 = ((destArea.x + destArea.width) / destWidth) * 2.0f - 1.0f;
    float y2 = ((destArea.y + destArea.height) / destHeight) * 2.0f - 1.0f;

    float u1 = sourceArea.x / float(sourceWidth);
    float v1 = sourceArea.y / float(sourceHeight);
    float u2 = (sourceArea.x + sourceArea.width) / float(sourceWidth);
    float v2 = (sourceArea.y + sourceArea.height) / float(sourceHeight);

    d3d11::SetPositionTexCoordVertex(&vertices[0], x1, y1, u1, v2);
    d3d11::SetPositionTexCoordVertex(&vertices[1], x1, y2, u1, v1);
    d3d11::SetPositionTexCoordVertex(&vertices[2], x2, y1, u2, v2);
    d3d11::SetPositionTexCoordVertex(&vertices[3], x2, y2, u2, v1);

    mDeviceContext->Unmap(mCopyVB, 0);

    static UINT stride = sizeof(d3d11::PositionTexCoordVertex);
    static UINT startIdx = 0;
    mDeviceContext->IASetVertexBuffers(0, 1, &mCopyVB, &stride, &startIdx);

    // Apply state
    static const float blendFactor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    mDeviceContext->OMSetBlendState(NULL, blendFactor, 0xFFFFFFF);
    mDeviceContext->OMSetDepthStencilState(NULL, 0xFFFFFFFF);
    mDeviceContext->RSSetState(NULL);

    // Apply shaders
    mDeviceContext->IASetInputLayout(mCopyIL);
    mDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    mDeviceContext->VSSetShader(mCopyVS, NULL, 0);

    ID3D11PixelShader *ps = NULL;
    switch(destFormat)
    {
      case GL_RGBA:            ps = mCopyRGBAPS;     break;
      case GL_RGB:             ps = mCopyRGBPS;      break;
      case GL_ALPHA:           ps = mCopyRGBAPS;     break;
      case GL_BGRA_EXT:        ps = mCopyRGBAPS;     break;
      case GL_LUMINANCE:       ps = mCopyLumPS;      break;
      case GL_LUMINANCE_ALPHA: ps = mCopyLumAlphaPS; break;
      default: UNREACHABLE();  ps = NULL;            break;
    }

    mDeviceContext->PSSetShader(ps, NULL, 0);

    // Unset the currently bound shader resource to avoid conflicts
    static ID3D11ShaderResourceView *const nullSRV = NULL;
    mDeviceContext->PSSetShaderResources(0, 1, &nullSRV);

    // Apply render targets
    mDeviceContext->OMSetRenderTargets(1, &dest, NULL);

    // Set the viewport
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = destWidth;
    viewport.Height = destHeight;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    mDeviceContext->RSSetViewports(1, &viewport);

    // Apply textures
    mDeviceContext->PSSetShaderResources(0, 1, &source);
    mDeviceContext->PSSetSamplers(0, 1, &mCopySampler);

    // Draw the quad
    mDeviceContext->Draw(4, 0);

    // Unbind textures and render targets and vertex buffer
    mDeviceContext->PSSetShaderResources(0, 1, &nullSRV);

    static ID3D11RenderTargetView *const nullRTV = NULL;
    mDeviceContext->OMSetRenderTargets(1, &nullRTV, NULL);

    static UINT zero = 0;
    static ID3D11Buffer *const nullBuffer = NULL;
    mDeviceContext->IASetVertexBuffers(0, 1, &nullBuffer, &zero, &zero);

    markAllStateDirty();

    return true;
}

RenderTarget *Renderer11::createRenderTarget(SwapChain *swapChain, bool depth)
{
    SwapChain11 *swapChain11 = SwapChain11::makeSwapChain11(swapChain);
    RenderTarget11 *renderTarget = NULL;
    if (depth)
    {
        renderTarget = new RenderTarget11(this, swapChain11->getDepthStencil(), NULL,
                                          swapChain11->getWidth(), swapChain11->getHeight());
    }
    else
    {
        renderTarget = new RenderTarget11(this, swapChain11->getRenderTarget(),
                                          swapChain11->getRenderTargetShaderResource(),
                                          swapChain11->getWidth(), swapChain11->getHeight());
    }
    return renderTarget;
}

RenderTarget *Renderer11::createRenderTarget(int width, int height, GLenum format, GLsizei samples, bool depth)
{
    RenderTarget11 *renderTarget = new RenderTarget11(this, width, height, format, samples, depth);
    return renderTarget;
}

ShaderExecutable *Renderer11::loadExecutable(const void *function, size_t length, GLenum type)
{
    ShaderExecutable11 *executable = NULL;

    switch (type)
    {
      case GL_VERTEX_SHADER:
        {
            ID3D11VertexShader *vshader = NULL;
            HRESULT result = mDevice->CreateVertexShader(function, length, NULL, &vshader);
            ASSERT(SUCCEEDED(result));

            if (vshader)
            {
                executable = new ShaderExecutable11(function, length, vshader);
            }
        }
        break;
      case GL_FRAGMENT_SHADER:
        {
            ID3D11PixelShader *pshader = NULL;
            HRESULT result = mDevice->CreatePixelShader(function, length, NULL, &pshader);
            ASSERT(SUCCEEDED(result));

            if (pshader)
            {
                executable = new ShaderExecutable11(function, length, pshader);
            }
        }
        break;
      default:
        UNREACHABLE();
        break;
    }

    return executable;
}

ShaderExecutable *Renderer11::compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, GLenum type)
{
    const char *profile = NULL;

    switch (type)
    {
      case GL_VERTEX_SHADER:
        profile = "vs_4_0";
        break;
      case GL_FRAGMENT_SHADER:
        profile = "ps_4_0";
        break;
      default:
        UNREACHABLE();
        return NULL;
    }

    ID3DBlob *binary = compileToBinary(infoLog, shaderHLSL, profile, false);
    if (!binary)
        return NULL;

    ShaderExecutable *executable = loadExecutable((DWORD *)binary->GetBufferPointer(), binary->GetBufferSize(), type);
    binary->Release();

    return executable;
}

VertexBuffer *Renderer11::createVertexBuffer()
{
    return new VertexBuffer11(this);
}

IndexBuffer *Renderer11::createIndexBuffer()
{
    return new IndexBuffer11(this);
}

bool Renderer11::getRenderTargetResource(gl::Framebuffer *framebuffer, unsigned int *subresourceIndex, ID3D11Texture2D **resource)
{
    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer();
    if (colorbuffer)
    {
        RenderTarget11 *renderTarget = RenderTarget11::makeRenderTarget11(colorbuffer->getRenderTarget());
        if (renderTarget)
        {
            *subresourceIndex = renderTarget->getSubresourceIndex();

            ID3D11RenderTargetView *colorBufferRTV = renderTarget->getRenderTargetView();
            if (colorBufferRTV)
            {
                ID3D11Resource *textureResource = NULL;
                colorBufferRTV->GetResource(&textureResource);
                colorBufferRTV->Release();

                if (textureResource)
                {
                    HRESULT result = textureResource->QueryInterface(IID_ID3D11Texture2D, (void**)resource);
                    textureResource->Release();

                    if (SUCCEEDED(result))
                    {
                        return true;
                    }
                    else
                    {
                        ERR("Failed to extract the ID3D11Texture2D from the render target resource, "
                            "HRESULT: 0x%X.", result);
                    }
                }
            }
        }
    }

    return false;
}

bool Renderer11::blitRect(gl::Framebuffer *readTarget, gl::Rectangle *readRect, gl::Framebuffer *drawTarget, gl::Rectangle *drawRect,
                          bool blitRenderTarget, bool blitDepthStencil)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

void Renderer11::readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            GLsizei outputPitch, bool packReverseRowOrder, GLint packAlignment, void* pixels)
{
    ID3D11Texture2D *colorBufferTexture = NULL;
    unsigned int subresourceIndex = 0;

    if (getRenderTargetResource(framebuffer, &subresourceIndex, &colorBufferTexture))
    {
        gl::Rectangle area;
        area.x = x;
        area.y = y;
        area.width = width;
        area.height = height;

        readTextureData(colorBufferTexture, subresourceIndex, area, format, type, outputPitch,
                        packReverseRowOrder, packAlignment, pixels);

        colorBufferTexture->Release();
        colorBufferTexture = NULL;
    }
}

Image *Renderer11::createImage()
{
    return new Image11();
}

void Renderer11::generateMipmap(Image *dest, Image *src)
{
    Image11 *dest11 = Image11::makeImage11(dest);
    Image11 *src11 = Image11::makeImage11(src);
    Image11::generateMipmap(dest11, src11);
}

TextureStorage *Renderer11::createTextureStorage2D(SwapChain *swapChain)
{
    SwapChain11 *swapChain11 = SwapChain11::makeSwapChain11(swapChain);
    return new TextureStorage11_2D(this, swapChain11);
}

TextureStorage *Renderer11::createTextureStorage2D(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, GLsizei width, GLsizei height)
{
    return new TextureStorage11_2D(this, levels, internalformat, usage, forceRenderable, width, height);
}

TextureStorage *Renderer11::createTextureStorageCube(int levels, GLenum internalformat, GLenum usage, bool forceRenderable, int size)
{
    return new TextureStorage11_Cube(this, levels, internalformat, usage, forceRenderable, size);
}

static inline unsigned int getFastPixelCopySize(DXGI_FORMAT sourceFormat, GLenum destFormat, GLenum destType)
{
    if (sourceFormat == DXGI_FORMAT_A8_UNORM &&
        destFormat   == GL_ALPHA &&
        destType     == GL_UNSIGNED_BYTE)
    {
        return 1;
    }
    else if (sourceFormat == DXGI_FORMAT_R8G8B8A8_UNORM &&
             destFormat   == GL_RGBA &&
             destType     == GL_UNSIGNED_BYTE)
    {
        return 4;
    }
    else if (sourceFormat == DXGI_FORMAT_B8G8R8A8_UNORM &&
             destFormat   == GL_BGRA_EXT &&
             destType     == GL_UNSIGNED_BYTE)
    {
        return 4;
    }
    else if (sourceFormat == DXGI_FORMAT_R16G16B16A16_FLOAT &&
             destFormat   == GL_RGBA &&
             destType     == GL_HALF_FLOAT_OES)
    {
        return 8;
    }
    else if (sourceFormat == DXGI_FORMAT_R32G32B32_FLOAT &&
             destFormat   == GL_RGB &&
             destType     == GL_FLOAT)
    {
        return 12;
    }
    else if (sourceFormat == DXGI_FORMAT_R32G32B32A32_FLOAT &&
             destFormat   == GL_RGBA &&
             destType     == GL_FLOAT)
    {
        return 16;
    }
    else
    {
        return 0;
    }
}

static inline void readPixelColor(const unsigned char *data, DXGI_FORMAT format, unsigned int x,
                                  unsigned int y, int inputPitch, gl::Color *outColor)
{
    switch (format)
    {
      case DXGI_FORMAT_R8G8B8A8_UNORM:
        {
            unsigned int rgba = *reinterpret_cast<const unsigned int*>(data + 4 * x + y * inputPitch);
            outColor->red =   (rgba & 0xFF000000) * (1.0f / 0xFF000000);
            outColor->green = (rgba & 0x00FF0000) * (1.0f / 0x00FF0000);
            outColor->blue =  (rgba & 0x0000FF00) * (1.0f / 0x0000FF00);
            outColor->alpha = (rgba & 0x000000FF) * (1.0f / 0x000000FF);
        }
        break;

      case DXGI_FORMAT_A8_UNORM:
        {
            outColor->red =   0.0f;
            outColor->green = 0.0f;
            outColor->blue =  0.0f;
            outColor->alpha = *(data + x + y * inputPitch) / 255.0f;
        }
        break;

      case DXGI_FORMAT_R32G32B32A32_FLOAT:
        {
            outColor->red =   *(reinterpret_cast<const float*>(data + 16 * x + y * inputPitch) + 0);
            outColor->green = *(reinterpret_cast<const float*>(data + 16 * x + y * inputPitch) + 1);
            outColor->blue =  *(reinterpret_cast<const float*>(data + 16 * x + y * inputPitch) + 2);
            outColor->alpha = *(reinterpret_cast<const float*>(data + 16 * x + y * inputPitch) + 3);
        }
        break;

      case DXGI_FORMAT_R32G32B32_FLOAT:
        {
            outColor->red =   *(reinterpret_cast<const float*>(data + 12 * x + y * inputPitch) + 0);
            outColor->green = *(reinterpret_cast<const float*>(data + 12 * x + y * inputPitch) + 1);
            outColor->blue =  *(reinterpret_cast<const float*>(data + 12 * x + y * inputPitch) + 2);
            outColor->alpha = 1.0f;
        }
        break;

      case DXGI_FORMAT_R16G16B16A16_FLOAT:
        {
            outColor->red =   gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 8 * x + y * inputPitch) + 0));
            outColor->green = gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 8 * x + y * inputPitch) + 1));
            outColor->blue =  gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 8 * x + y * inputPitch) + 2));
            outColor->alpha = gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 8 * x + y * inputPitch) + 3));
        }
        break;

      case DXGI_FORMAT_B8G8R8A8_UNORM:
        {
            unsigned int bgra = *reinterpret_cast<const unsigned int*>(data + 4 * x + y * inputPitch);
            outColor->red =   (bgra & 0x0000FF00) * (1.0f / 0x0000FF00);
            outColor->blue =  (bgra & 0xFF000000) * (1.0f / 0xFF000000);
            outColor->green = (bgra & 0x00FF0000) * (1.0f / 0x00FF0000);
            outColor->alpha = (bgra & 0x000000FF) * (1.0f / 0x000000FF);
        }
        break;

      case DXGI_FORMAT_R8_UNORM:
        {
            outColor->red =   *(data + x + y * inputPitch) / 255.0f;
            outColor->green = 0.0f;
            outColor->blue =  0.0f;
            outColor->alpha = 1.0f;
        }
        break;

      case DXGI_FORMAT_R8G8_UNORM:
        {
            unsigned short rg = *reinterpret_cast<const unsigned short*>(data + 2 * x + y * inputPitch);

            outColor->red =   (rg & 0xFF00) * (1.0f / 0xFF00);
            outColor->green = (rg & 0x00FF) * (1.0f / 0x00FF);
            outColor->blue =  0.0f;
            outColor->alpha = 1.0f;
        }
        break;

      case DXGI_FORMAT_R16_FLOAT:
        {
            outColor->red =   gl::float16ToFloat32(*reinterpret_cast<const unsigned short*>(data + 2 * x + y * inputPitch));
            outColor->green = 0.0f;
            outColor->blue =  0.0f;
            outColor->alpha = 1.0f;
        }
        break;

      case DXGI_FORMAT_R16G16_FLOAT:
        {
            outColor->red =   gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 4 * x + y * inputPitch) + 0));
            outColor->green = gl::float16ToFloat32(*(reinterpret_cast<const unsigned short*>(data + 4 * x + y * inputPitch) + 1));
            outColor->blue =  0.0f;
            outColor->alpha = 1.0f;
        }
        break;

      default:
        ERR("ReadPixelColor not implemented for DXGI format %u.", format);
        UNIMPLEMENTED();
        break;
    }
}

static inline void writePixelColor(const gl::Color &color, GLenum format, GLenum type, unsigned int x,
                                   unsigned int y, int outputPitch, void *outData)
{
    unsigned char* byteData = reinterpret_cast<unsigned char*>(outData);
    unsigned short* shortData = reinterpret_cast<unsigned short*>(outData);

    switch (format)
    {
      case GL_RGBA:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            byteData[4 * x + y * outputPitch + 0] = static_cast<unsigned char>(255 * color.red   + 0.5f);
            byteData[4 * x + y * outputPitch + 1] = static_cast<unsigned char>(255 * color.green + 0.5f);
            byteData[4 * x + y * outputPitch + 2] = static_cast<unsigned char>(255 * color.blue  + 0.5f);
            byteData[4 * x + y * outputPitch + 3] = static_cast<unsigned char>(255 * color.alpha + 0.5f);
            break;

          default:
            ERR("WritePixelColor not implemented for format GL_RGBA and type 0x%X.", type);
            UNIMPLEMENTED();
            break;
        }
        break;

      case GL_BGRA_EXT:
        switch (type)
        {
          case GL_UNSIGNED_BYTE:
            byteData[4 * x + y * outputPitch + 0] = static_cast<unsigned char>(255 * color.blue  + 0.5f);
            byteData[4 * x + y * outputPitch + 1] = static_cast<unsigned char>(255 * color.green + 0.5f);
            byteData[4 * x + y * outputPitch + 2] = static_cast<unsigned char>(255 * color.red   + 0.5f);
            byteData[4 * x + y * outputPitch + 3] = static_cast<unsigned char>(255 * color.alpha + 0.5f);
            break;

          case GL_UNSIGNED_SHORT_4_4_4_4_REV_EXT:
            // According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
            // this type is packed as follows:
            //   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
            //  --------------------------------------------------------------------------------
            // |       4th         |        3rd         |        2nd        |   1st component   |
            //  --------------------------------------------------------------------------------
            // in the case of BGRA_EXT, B is the first component, G the second, and so forth.
            shortData[x + y * outputPitch / sizeof(unsigned short)] =
                (static_cast<unsigned short>(15 * color.alpha + 0.5f) << 12) |
                (static_cast<unsigned short>(15 * color.red   + 0.5f) <<  8) |
                (static_cast<unsigned short>(15 * color.green + 0.5f) <<  4) |
                (static_cast<unsigned short>(15 * color.blue  + 0.5f) <<  0);
            break;

          case GL_UNSIGNED_SHORT_1_5_5_5_REV_EXT:
            // According to the desktop GL spec in the "Transfer of Pixel Rectangles" section
            // this type is packed as follows:
            //   15   14   13   12   11   10    9    8    7    6    5    4    3    2    1    0
            //  --------------------------------------------------------------------------------
            // | 4th |          3rd           |           2nd          |      1st component     |
            //  --------------------------------------------------------------------------------
            // in the case of BGRA_EXT, B is the first component, G the second, and so forth.
            shortData[x + y * outputPitch / sizeof(unsigned short)] =
                (static_cast<unsigned short>(     color.alpha + 0.5f) << 15) |
                (static_cast<unsigned short>(31 * color.red   + 0.5f) << 10) |
                (static_cast<unsigned short>(31 * color.green + 0.5f) <<  5) |
                (static_cast<unsigned short>(31 * color.blue  + 0.5f) <<  0);
            break;

          default:
            ERR("WritePixelColor not implemented for format GL_BGRA_EXT and type 0x%X.", type);
            UNIMPLEMENTED();
            break;
        }
        break;

      case GL_RGB:
        switch (type)
        {
          case GL_UNSIGNED_SHORT_5_6_5:
            shortData[x + y * outputPitch / sizeof(unsigned short)] =
                (static_cast<unsigned short>(31 * color.blue  + 0.5f) <<  0) |
                (static_cast<unsigned short>(63 * color.green + 0.5f) <<  5) |
                (static_cast<unsigned short>(31 * color.red   + 0.5f) << 11);
            break;

          case GL_UNSIGNED_BYTE:
            byteData[3 * x + y * outputPitch + 0] = static_cast<unsigned char>(255 * color.red +   0.5f);
            byteData[3 * x + y * outputPitch + 1] = static_cast<unsigned char>(255 * color.green + 0.5f);
            byteData[3 * x + y * outputPitch + 2] = static_cast<unsigned char>(255 * color.blue +  0.5f);
            break;

          default:
            ERR("WritePixelColor not implemented for format GL_RGB and type 0x%X.", type);
            UNIMPLEMENTED();
            break;
        }
        break;

      default:
        ERR("WritePixelColor not implemented for format 0x%X.", format);
        UNIMPLEMENTED();
        break;
    }
}

void Renderer11::readTextureData(ID3D11Texture2D *texture, unsigned int subResource, const gl::Rectangle &area,
                                 GLenum format, GLenum type, GLsizei outputPitch, bool packReverseRowOrder,
                                 GLint packAlignment, void *pixels)
{
    D3D11_TEXTURE2D_DESC textureDesc;
    texture->GetDesc(&textureDesc);

    D3D11_TEXTURE2D_DESC stagingDesc;
    stagingDesc.Width = area.width;
    stagingDesc.Height = area.height;
    stagingDesc.MipLevels = 1;
    stagingDesc.ArraySize = 1;
    stagingDesc.Format = textureDesc.Format;
    stagingDesc.SampleDesc.Count = 1;
    stagingDesc.SampleDesc.Quality = 0;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    stagingDesc.MiscFlags = 0;

    ID3D11Texture2D* stagingTex = NULL;
    HRESULT result = mDevice->CreateTexture2D(&stagingDesc, NULL, &stagingTex);
    if (FAILED(result))
    {
        ERR("Failed to create staging texture for readPixels, HRESULT: 0x%X.", result);
        return;
    }

    ID3D11Texture2D* srcTex = NULL;
    if (textureDesc.SampleDesc.Count > 1)
    {
        D3D11_TEXTURE2D_DESC resolveDesc;
        resolveDesc.Width = textureDesc.Width;
        resolveDesc.Height = textureDesc.Height;
        resolveDesc.MipLevels = 1;
        resolveDesc.ArraySize = 1;
        resolveDesc.Format = textureDesc.Format;
        resolveDesc.SampleDesc.Count = 1;
        resolveDesc.SampleDesc.Quality = 0;
        resolveDesc.Usage = D3D11_USAGE_DEFAULT;
        resolveDesc.BindFlags = 0;
        resolveDesc.CPUAccessFlags = 0;
        resolveDesc.MiscFlags = 0;

        result = mDevice->CreateTexture2D(&resolveDesc, NULL, &srcTex);
        if (FAILED(result))
        {
            ERR("Failed to create resolve texture for readPixels, HRESULT: 0x%X.", result);
            stagingTex->Release();
            return;
        }

        mDeviceContext->ResolveSubresource(srcTex, 0, texture, subResource, textureDesc.Format);
        subResource = 0;
    }
    else
    {
        srcTex = texture;
        srcTex->AddRef();
    }

    D3D11_BOX srcBox;
    srcBox.left = area.x;
    srcBox.right = area.x + area.width;
    srcBox.top = area.y;
    srcBox.bottom = area.y + area.height;
    srcBox.front = 0;
    srcBox.back = 1;

    mDeviceContext->CopySubresourceRegion(stagingTex, 0, 0, 0, 0, srcTex, subResource, &srcBox);

    srcTex->Release();
    srcTex = NULL;

    D3D11_MAPPED_SUBRESOURCE mapping;
    mDeviceContext->Map(stagingTex, 0, D3D11_MAP_READ, 0, &mapping);

    unsigned char *source;
    int inputPitch;
    if (packReverseRowOrder)
    {
        source = static_cast<unsigned char*>(mapping.pData) + mapping.RowPitch * (area.height - 1);
        inputPitch = -static_cast<int>(mapping.RowPitch);
    }
    else
    {
        source = static_cast<unsigned char*>(mapping.pData);
        inputPitch = static_cast<int>(mapping.RowPitch);
    }

    unsigned int fastPixelSize = getFastPixelCopySize(textureDesc.Format, format, type);
    if (fastPixelSize != 0)
    {
        unsigned char *dest = static_cast<unsigned char*>(pixels);
        for (int j = 0; j < area.height; j++)
        {
            memcpy(dest + j * outputPitch, source + j * inputPitch, area.width * fastPixelSize);
        }
    }
    else
    {
        gl::Color pixelColor;
        for (int j = 0; j < area.height; j++)
        {
            for (int i = 0; i < area.width; i++)
            {
                readPixelColor(source, textureDesc.Format, i, j, inputPitch, &pixelColor);
                writePixelColor(pixelColor, format, type, i, j, outputPitch, pixels);
            }
        }
    }

    mDeviceContext->Unmap(stagingTex, 0);

    stagingTex->Release();
    stagingTex = NULL;
}

}

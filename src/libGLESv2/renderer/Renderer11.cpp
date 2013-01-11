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
    mVertexDataManager = NULL;
    mIndexDataManager = NULL;

    mLineLoopIB = NULL;

    mD3d11Module = NULL;
    mDxgiModule = NULL;

    mDeviceLost = false;

    mDevice = NULL;
    mDeviceContext = NULL;
    mDxgiAdapter = NULL;
    mDxgiFactory = NULL;
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
        if (index < 0 || index >= gl::MAX_VERTEX_TEXTURE_IMAGE_UNITS_VTF)
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
    }

    if (type == gl::SAMPLER_PIXEL)
    {
        if (index < 0 || index >= gl::MAX_TEXTURE_IMAGE_UNITS)
        {
            ERR("Pixel shader sampler index %i is not valid.", index);
            return;
        }

        mDeviceContext->PSSetShaderResources(index, 1, &textureSRV);
    }
    else if (type == gl::SAMPLER_VERTEX)
    {
        if (index < 0 || index >= gl::MAX_VERTEX_TEXTURE_IMAGE_UNITS_VTF)
        {
            ERR("Vertex shader sampler index %i is not valid.", index);
            return;
        }

        mDeviceContext->VSSetShaderResources(index, 1, &textureSRV);
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
                             bool ignoreViewport, gl::ProgramBinary *currentProgram, bool forceSetUniforms)
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

    bool viewportChanged =  mForceSetViewport || memcmp(&actualViewport, &mCurViewport, sizeof(gl::Rectangle)) != 0 ||
                            actualZNear != mCurNear || actualZFar != mCurFar;

    if (viewportChanged)
    {
        mDeviceContext->RSSetViewports(1, &dxViewport);

        mCurViewport = actualViewport;
        mCurNear = actualZNear;
        mCurFar = actualZFar;
    }

    if (currentProgram && (viewportChanged || forceSetUniforms))
    {
        currentProgram->applyDxHalfPixelSize(0.0f, 0.0f);

        // These values are used for computing gl_FragCoord in Program::linkVaryings().
        currentProgram->applyDxCoord(actualViewport.width  * 0.5f,
                                     actualViewport.height * 0.5f,
                                     actualViewport.x + (actualViewport.width  * 0.5f),
                                     actualViewport.y + (actualViewport.height * 0.5f));

        GLfloat ccw = !gl::IsTriangleMode(drawMode) ? 0.0f : (frontFace == GL_CCW ? 1.0f : -1.0f);
        currentProgram->applyDxDepthFront((actualZFar - actualZNear) * 0.5f, (actualZNear + actualZFar) * 0.5f, ccw);

        currentProgram->applyDxDepthRange(actualZNear, actualZFar, actualZFar - actualZNear);
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
      case GL_LINE_LOOP:      UNIMPLEMENTED();   /* TODO */                              break;
      case GL_LINE_STRIP:     primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;     break;
      case GL_TRIANGLES:      primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;  break;
      case GL_TRIANGLE_STRIP: primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
      case GL_TRIANGLE_FAN:   UNIMPLEMENTED();   /* TODO */                              break;
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
    mDeviceContext->Draw(count, 0);
}

void Renderer11::drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, gl::Buffer *elementArrayBuffer, const TranslatedIndexData &indexInfo)
{
    if (mode == GL_LINE_LOOP)
    {
        drawLineLoop(count, type, indices, indexInfo.minIndex, elementArrayBuffer);
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

    mDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
    mDeviceContext->DrawIndexed(count, 0, -minIndex);
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

void Renderer11::applyUniforms(const gl::UniformArray *uniformArray, const dx_VertexConstants &vertexConstants, const dx_PixelConstants &pixelConstants)
{
    D3D11_BUFFER_DESC constantBufferDescription = {0};
    constantBufferDescription.ByteWidth = D3D10_REQ_CONSTANT_BUFFER_ELEMENT_COUNT * sizeof(float[4]);
    constantBufferDescription.Usage = D3D11_USAGE_DYNAMIC;
    constantBufferDescription.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    constantBufferDescription.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    constantBufferDescription.MiscFlags = 0;
    constantBufferDescription.StructureByteStride = 0;

    ID3D11Buffer *constantBufferVS = NULL;
    HRESULT result = mDevice->CreateBuffer(&constantBufferDescription, NULL, &constantBufferVS);
    ASSERT(SUCCEEDED(result));

    ID3D11Buffer *constantBufferPS = NULL;
    result = mDevice->CreateBuffer(&constantBufferDescription, NULL, &constantBufferPS);
    ASSERT(SUCCEEDED(result));

    D3D11_MAPPED_SUBRESOURCE mapVS = {0};
    result = mDeviceContext->Map(constantBufferVS, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapVS);
    ASSERT(SUCCEEDED(result));

    D3D11_MAPPED_SUBRESOURCE mapPS = {0};
    result = mDeviceContext->Map(constantBufferPS, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapPS);
    ASSERT(SUCCEEDED(result));

    for (gl::UniformArray::const_iterator uniform_iterator = uniformArray->begin(); uniform_iterator != uniformArray->end(); uniform_iterator++)
    {
        const gl::Uniform *uniform = *uniform_iterator;

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
            if (uniform->vsRegisterIndex >= 0)
            {
                GLfloat (*c)[4] = (GLfloat(*)[4])mapVS.pData;
                float (*f)[4] = (float(*)[4])uniform->data;

                for (unsigned int i = 0; i < uniform->registerCount; i++)
                {
                    c[uniform->vsRegisterIndex + i][0] = f[i][0];
                    c[uniform->vsRegisterIndex + i][1] = f[i][1];
                    c[uniform->vsRegisterIndex + i][2] = f[i][2];
                    c[uniform->vsRegisterIndex + i][3] = f[i][3];
                }
            }
            if (uniform->psRegisterIndex >= 0)
            {
                GLfloat (*c)[4] = (GLfloat(*)[4])mapPS.pData;
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
            if (uniform->vsRegisterIndex >= 0)
            {
                int (*c)[4] = (int(*)[4])mapVS.pData;
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
            if (uniform->psRegisterIndex >= 0)
            {
                int (*c)[4] = (int(*)[4])mapPS.pData;
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
            if (uniform->vsRegisterIndex >= 0)
            {
                int (*c)[4] = (int(*)[4])mapVS.pData;
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
            if (uniform->psRegisterIndex >= 0)
            {
                int (*c)[4] = (int(*)[4])mapPS.pData;
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
    }

    // Driver uniforms
    memcpy(mapVS.pData, &vertexConstants, sizeof(dx_VertexConstants));
    memcpy(mapPS.pData, &pixelConstants, sizeof(dx_PixelConstants));

    mDeviceContext->Unmap(constantBufferVS, 0);
    mDeviceContext->VSSetConstantBuffers(0, 1, &constantBufferVS);
    constantBufferVS->Release();

    mDeviceContext->Unmap(constantBufferPS, 0);
    mDeviceContext->PSSetConstantBuffers(0, 1, &constantBufferPS);
    constantBufferPS->Release();
}

void Renderer11::clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer)
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

            if (mScissorEnabled && (mCurScissor.x > 0 || mCurScissor.y > 0 ||
                                    mCurScissor.x + mCurScissor.width < renderTarget->getWidth() ||
                                    mCurScissor.y + mCurScissor.height < renderTarget->getHeight()))
            {
                // TODO: clearing of subregion of render target
                UNIMPLEMENTED();
            }

            bool alphaUnmasked = (gl::GetAlphaSize(mRenderTargetDesc.format) == 0) || clearParams.colorMaskAlpha;
            const bool needMaskedColorClear = (clearParams.mask & GL_COLOR_BUFFER_BIT) &&
                                              !(clearParams.colorMaskRed && clearParams.colorMaskGreen &&
                                               clearParams.colorMaskBlue && alphaUnmasked);

            if (needMaskedColorClear)
            {
                // TODO: masked color clearing
                UNIMPLEMENTED();
            }
            else
            {
                const float clearValues[4] = { clearParams.colorClearValue.red,
                                               clearParams.colorClearValue.green,
                                               clearParams.colorClearValue.blue,
                                               clearParams.colorClearValue.alpha };
                mDeviceContext->ClearRenderTargetView(framebufferRTV, clearValues);
            }

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

            if (mScissorEnabled && (mCurScissor.x > 0 || mCurScissor.y > 0 ||
                                    mCurScissor.x + mCurScissor.width < renderTarget->getWidth() ||
                                    mCurScissor.y + mCurScissor.height < renderTarget->getHeight()))
            {
                // TODO: clearing of subregion of depth stencil view
                UNIMPLEMENTED();
            }

            unsigned int stencilUnmasked = 0x0;
            if ((clearParams.mask & GL_STENCIL_BUFFER_BIT) && frameBuffer->hasStencil())
            {
                unsigned int stencilSize = gl::GetStencilSize(frameBuffer->getStencilbuffer()->getActualFormat());
                stencilUnmasked = (0x1 << stencilSize) - 1;
            }

            const bool needMaskedStencilClear = (clearParams.mask & GL_STENCIL_BUFFER_BIT) &&
                                                (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

            if (needMaskedStencilClear)
            {
                // TODO: masked clearing of depth stencil
                UNIMPLEMENTED();
            }
            else
            {
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
            }

            framebufferDSV->Release();
        }
    }
}

void Renderer11::markAllStateDirty()
{
    mAppliedRenderTargetSerial = 0;
    mAppliedDepthbufferSerial = 0;
    mAppliedStencilbufferSerial = 0;
    mDepthStencilInitialized = false;
    mRenderTargetDescInitialized = false;

    for (int i = 0; i < gl::MAX_VERTEX_TEXTURE_IMAGE_UNITS_VTF; i++)
    {
        mForceSetVertexSamplerStates[i] = true;
    }
    for (int i = 0; i < gl::MAX_TEXTURE_IMAGE_UNITS; i++)
    {
        mForceSetPixelSamplerStates[i] = true;
    }

    mForceSetBlendState = true;
    mForceSetRasterState = true;
    mForceSetDepthStencilState = true;
    mForceSetScissor = true;
    mForceSetViewport = true;

    mAppliedIBSerial = 0;
    mAppliedIBOffset = 0;

    mAppliedProgramBinarySerial = 0;
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

const char *Renderer11::getAdapterDescription() const
{
    return mDescription;
}

GUID Renderer11::getAdapterIdentifier() const
{
    // TODO
    // UNIMPLEMENTED();
    GUID foo = {0};
    return foo;
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

bool Renderer11::getVertexTextureSupport() const
{
    // TODO
    // UNIMPLEMENTED();
    return false;
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
    // TODO
    // UNIMPLEMENTED();

    // PIX doesn't seem to support using share handles, so disable them.
    return false && !gl::perfActive();
}

int Renderer11::getMajorShaderModel() const
{
    switch (mFeatureLevel)
    {
      case D3D_FEATURE_LEVEL_11_0: return D3D11_SHADER_MAJOR_VERSION;   // 5
      case D3D_FEATURE_LEVEL_10_1:
      case D3D_FEATURE_LEVEL_10_0: return D3D10_SHADER_MAJOR_VERSION;   // 4
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

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterface2D *storage, GLint level)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

bool Renderer11::copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageInterfaceCube *storage, GLenum target, GLint level)
{
    // TODO
    UNIMPLEMENTED();
    return false;
}

RenderTarget *Renderer11::createRenderTarget(SwapChain *swapChain, bool depth)
{
    SwapChain11 *swapChain11 = SwapChain11::makeSwapChain11(swapChain); 
    RenderTarget11 *renderTarget = NULL;
    if (depth)
    {
        renderTarget = new RenderTarget11(this, swapChain11->getDepthStencil(), swapChain11->getWidth(), swapChain11->getHeight());
    }
    else
    {
        renderTarget = new RenderTarget11(this, swapChain11->getRenderTarget(), swapChain11->getWidth(), swapChain11->getHeight());
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

    gl::Renderbuffer *colorbuffer = framebuffer->getColorbuffer();
    if (colorbuffer)
    {
        RenderTarget11 *renderTarget = RenderTarget11::makeRenderTarget11(colorbuffer->getRenderTarget());
        if (renderTarget)
        {
            ID3D11RenderTargetView *colorBufferRTV = renderTarget->getRenderTargetView();
            if (colorBufferRTV)
            {
                ID3D11Resource *textureResource = NULL;
                colorBufferRTV->GetResource(&textureResource);

                if (textureResource)
                {
                    HRESULT result = textureResource->QueryInterface(IID_ID3D11Texture2D, (void**)&colorBufferTexture);
                    textureResource->Release();

                    if (FAILED(result))
                    {
                        ERR("Failed to extract the ID3D11Texture2D from the render target resource, "
                            "HRESULT: 0x%X.", result);
                        return;
                    }
                }
            }
        }
    }

    if (colorBufferTexture)
    {
        gl::Rectangle area;
        area.x = x;
        area.y = y;
        area.width = width;
        area.height = height;

        readTextureData(colorBufferTexture, 0, area, format, type, outputPitch, packReverseRowOrder,
                        packAlignment, pixels);

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
    // TODO
    UNIMPLEMENTED();
    return;
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
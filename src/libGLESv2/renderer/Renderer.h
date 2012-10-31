//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer.h: Defines a back-end specific class that hides the details of the
// implementation-specific renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER_H_
#define LIBGLESV2_RENDERER_RENDERER_H_

#include <set>
#include <map>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <d3d9.h>  // D3D9_REPLACE

#include "common/angleutils.h"
#include "libGLESv2/renderer/ShaderCache.h"
#include "libGLESv2/EnumTypes.h"
#include "libGLESv2/Texture.h"

const int versionWindowsVista = MAKEWORD(0x00, 0x06);
const int versionWindows7 = MAKEWORD(0x01, 0x06);

// Return the version of the operating system in a format suitable for ordering
// comparison.
inline int getComparableOSVersion()
{
    DWORD version = GetVersion();
    int majorVersion = LOBYTE(LOWORD(version));
    int minorVersion = HIBYTE(LOWORD(version));
    return MAKEWORD(minorVersion, majorVersion);
}

namespace egl
{
class Display;
}

namespace renderer
{

class Renderer
{
  public:
    Renderer(egl::Display *display, HMODULE hModule, HDC hDc);
    virtual ~Renderer();

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual void startScene();
    virtual void endScene();

    virtual void sync(bool block);
    virtual IDirect3DQuery9* allocateEventQuery();
    virtual void freeEventQuery(IDirect3DQuery9* query);

    // resource creation
    virtual IDirect3DVertexShader9 *createVertexShader(const DWORD *function, size_t length); // D3D9_REPLACE
    virtual IDirect3DPixelShader9 *createPixelShader(const DWORD *function, size_t length); // D3D9_REPLACE
#if 0
    virtual void *createTexture2D();
    virtual void *createTextureCube();
    virtual void *createQuery();;
    virtual void *createIndexBuffer();
    virtual void *createVertexbuffer();

    // state setup
    virtual void applyShaders();
    virtual void applyConstants();
    virtual void applyRenderTargets();
    virtual void applyState();
#endif
    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler);
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    // lost device
    virtual void markDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost(bool notify);
    virtual bool testDeviceResettable();

    // Renderer capabilities
    virtual IDirect3DDevice9 *getDevice() {return mDevice;};  // D3D9_REPLACE
    virtual D3DADAPTER_IDENTIFIER9 *getAdapterIdentifier() {return &mAdapterIdentifier;}; // D3D9_REPLACE
    virtual IDirect3D9 *getD3D() {return mD3d9;}; // D3D9_REMOVE
    virtual UINT getAdapter() {return mAdapter;}; // D3D9_REMOVE
    virtual D3DDEVTYPE getDeviceType() {return mDeviceType;}; // D3D9_REMOVE
    virtual bool isD3d9ExDevice() const { return mD3d9Ex != NULL; } // D3D9_REMOVE

    virtual bool getDXT1TextureSupport();
    virtual bool getDXT3TextureSupport();
    virtual bool getDXT5TextureSupport();
    virtual bool getEventQuerySupport();
    virtual bool getFloat32TextureSupport(bool *filtering, bool *renderable);
    virtual bool getFloat16TextureSupport(bool *filtering, bool *renderable);
    virtual bool getLuminanceTextureSupport();
    virtual bool getLuminanceAlphaTextureSupport();
    virtual bool getVertexTextureSupport() const;
    virtual bool getNonPower2TextureSupport() const;
    virtual bool getDepthTextureSupport() const;
    virtual bool getOcclusionQuerySupport() const;
    virtual bool getInstancingSupport() const;
    virtual bool getTextureFilterAnisotropySupport() const;
    virtual float getTextureMaxAnisotropy() const;
    virtual bool getShareHandleSupport() const;

    virtual bool getShaderModel3Support() const;
    virtual float getMaxPointSize() const;
    virtual int getMaxTextureWidth() const;
    virtual int getMaxTextureHeight() const;
    virtual bool get32BitIndexSupport() const;
    virtual DWORD getCapsDeclTypes() const; // D3D9_REPLACE
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    virtual GLsizei getMaxSupportedSamples() const;
    virtual int getNearestSupportedSamples(D3DFORMAT format, int requested) const;

    virtual D3DPOOL getBufferPool(DWORD usage) const;
    virtual D3DPOOL getTexturePool(DWORD usage) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer);

    void getMultiSampleSupport(D3DFORMAT format, bool *multiSampleArray); // D3D9_REPLACE

    static const D3DFORMAT mRenderTargetFormats[];
    static const D3DFORMAT mDepthStencilFormats[];

    egl::Display *mDisplay;
    const HDC mDc;
    HMODULE mD3d9Module;

    void initializeDevice();
    D3DPRESENT_PARAMETERS getDefaultPresentParameters();
    void releaseDeviceResources();

    UINT mAdapter;
    D3DDEVTYPE mDeviceType;
    IDirect3D9 *mD3d9;  // Always valid after successful initialization.
    IDirect3D9Ex *mD3d9Ex;  // Might be null if D3D9Ex is not supported.
    IDirect3DDevice9 *mDevice;
    IDirect3DDevice9Ex *mDeviceEx;  // Might be null if D3D9Ex is not supported.

    HWND mDeviceWindow;

    bool mDeviceLost;
    D3DCAPS9 mDeviceCaps;
    D3DADAPTER_IDENTIFIER9 mAdapterIdentifier;

    bool mSceneStarted;
    bool mSupportsNonPower2Textures;
    bool mSupportsTextureFilterAnisotropy;
    int mMinSwapInterval;
    int mMaxSwapInterval;

    std::map<D3DFORMAT, bool *> mMultiSampleSupport;
    GLsizei mMaxSupportedSamples;

    // A pool of event queries that are currently unused.
    std::vector<IDirect3DQuery9*> mEventQueryPool;
    VertexShaderCache mVertexShaderCache;
    PixelShaderCache mPixelShaderCache;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER_H_

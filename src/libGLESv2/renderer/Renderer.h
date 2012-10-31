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
#include <vector>

#include "common/angleutils.h"
#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <d3d9.h>  // D3D9_REPLACE

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

#if 0
    // resource creation
    virtual void *createVertexShader(const DWORD *function, size_t length);
    virtual void *createPixelShader(const DWORD *function, size_t length);
    virtual void *createTexture2D();
    virtual void *createTextureCube();
    virtual void *createQuery();;
    virtual void *createIndexBuffer();
    virtual void *createVertexbuffer();

    // state setup
    virtual void applyTexture();
    virtual void applyShaders();
    virtual void applyConstants();
    virtual void applyRenderTargets();
    virtual void applyState();
#endif

    // lost device
    virtual void markDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost();
    virtual bool testDeviceResettable();

    // Renderer capabilities
    virtual IDirect3DDevice9 *getDevice() {return mDevice;};  // D3D9_REPLACE
    virtual D3DADAPTER_IDENTIFIER9 *getAdapterIdentifier() {return &mAdapterIdentifier;}; // D3D9_REPLACE
    virtual D3DCAPS9 getDeviceCaps() {return mDeviceCaps;};       // D3D9_REMOVE
    virtual IDirect3D9 *getD3D() {return mD3d9;}; // D3D9_REMOVE
    virtual UINT getAdapter() {return mAdapter;}; // D3D9_REMOVE
    virtual D3DDEVTYPE getDeviceType() {return mDeviceType;}; // D3D9_REMOVE
    virtual bool isD3d9ExDevice() const { return mD3d9Ex != NULL; } // D3D9_REMOVE

    virtual void getMultiSampleSupport(D3DFORMAT format, bool *multiSampleArray); // D3D9_REPLACE
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
    virtual float getTextureFilterAnisotropySupport() const;
    virtual bool getShareHandleSupport() const;

    virtual D3DPOOL getBufferPool(DWORD usage) const;
    virtual D3DPOOL getTexturePool(DWORD usage) const;

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer);

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

    // A pool of event queries that are currently unused.
    std::vector<IDirect3DQuery9*> mEventQueryPool;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER_H_

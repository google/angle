//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Renderer11.h: Defines a back-end specific class for the D3D11 renderer.

#ifndef LIBGLESV2_RENDERER_RENDERER11_H_
#define LIBGLESV2_RENDERER_RENDERER11_H_

#define GL_APICALL
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#define EGLAPI
#include <EGL/egl.h>

#include <dxgi.h>
#include <d3d11.h>

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"

#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/RenderStateCache.h"

namespace rx
{

class Renderer11 : public Renderer
{
  public:
    Renderer11(egl::Display *display, HDC hDc);
    virtual ~Renderer11();

    static Renderer11 *makeRenderer11(Renderer *renderer);

    virtual EGLint initialize();
    virtual bool resetDevice();

    virtual int generateConfigs(ConfigDesc **configDescList);
    virtual void deleteConfigs(ConfigDesc *configDescList);

    virtual void sync(bool block);

    virtual SwapChain *createSwapChain(HWND window, HANDLE shareHandle, GLenum backBufferFormat, GLenum depthBufferFormat);

    virtual void setSamplerState(gl::SamplerType type, int index, const gl::SamplerState &sampler);
    virtual void setTexture(gl::SamplerType type, int index, gl::Texture *texture);

    virtual void setRasterizerState(const gl::RasterizerState &rasterState, unsigned int depthSize);
    virtual void setBlendState(const gl::BlendState &blendState, const gl::Color &blendColor,
                               unsigned int sampleMask);
    virtual void setDepthStencilState(const gl::DepthStencilState &depthStencilState, int stencilRef,
                                      int stencilBackRef, bool frontFaceCCW, unsigned int stencilSize);

    virtual void setScissorRectangle(const gl::Rectangle &scissor);
    virtual bool setViewport(const gl::Rectangle& viewport, float zNear, float zFar,
                             unsigned int renderTargetWidth, unsigned int renderTargetHeight,
                             gl::ProgramBinary *currentProgram, bool forceSetUniforms);

    virtual bool applyPrimitiveType(GLenum mode, GLsizei count);
    virtual bool applyRenderTarget(gl::Framebuffer *frameBuffer);
    virtual void applyShaders(gl::ProgramBinary *programBinary);
    virtual GLenum applyVertexBuffer(gl::ProgramBinary *programBinary, gl::VertexAttribute vertexAttributes[], GLint first, GLsizei count, GLsizei instances);
    virtual GLenum applyIndexBuffer(const GLvoid *indices, gl::Buffer *elementArrayBuffer, GLsizei count, GLenum mode, GLenum type, gl::TranslatedIndexData *indexInfo);

    virtual void drawArrays(GLenum mode, GLsizei count, GLsizei instances);
    virtual void drawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices, gl::Buffer *elementArrayBuffer, const gl::TranslatedIndexData &indexInfo);

    virtual void clear(const gl::ClearParameters &clearParams, gl::Framebuffer *frameBuffer);

    virtual void markAllStateDirty();

    // lost device
    virtual void markDeviceLost();
    virtual bool isDeviceLost();
    virtual bool testDeviceLost(bool notify);
    virtual bool testDeviceResettable();

    // Renderer capabilities
    virtual DWORD getAdapterVendor() const;
    virtual const char *getAdapterDescription() const;
    virtual GUID getAdapterIdentifier() const;

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

    virtual int getMajorShaderModel() const;
    virtual float getMaxPointSize() const;
    virtual int getMaxTextureWidth() const;
    virtual int getMaxTextureHeight() const;
    virtual bool get32BitIndexSupport() const;
    virtual int getMinSwapInterval() const;
    virtual int getMaxSwapInterval() const;

    virtual GLsizei getMaxSupportedSamples() const;

    // Pixel operations
    virtual bool copyToRenderTarget(TextureStorage2D *dest, TextureStorage2D *source);
    virtual bool copyToRenderTarget(TextureStorageCubeMap *dest, TextureStorageCubeMap *source);

    virtual bool copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorage2D *storage, GLint level);
    virtual bool copyImage(gl::Framebuffer *framebuffer, const RECT &sourceRect, GLenum destFormat,
                           GLint xoffset, GLint yoffset, TextureStorageCubeMap *storage, GLenum target, GLint level);

    virtual bool blitRect(gl::Framebuffer *readTarget, gl::Rectangle *readRect, gl::Framebuffer *drawTarget, gl::Rectangle *drawRect,
                          bool blitRenderTarget, bool blitDepthStencil);
    virtual void readPixels(gl::Framebuffer *framebuffer, GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type,
                            GLsizei outputPitch, bool packReverseRowOrder, GLint packAlignment, void* pixels);

    // RenderTarget creation
    virtual RenderTarget *createRenderTarget(SwapChain *swapChain, bool depth);
    virtual RenderTarget *createRenderTarget(int width, int height, GLenum format, GLsizei samples, bool depth);

    // Shader operations
    virtual ShaderExecutable *loadExecutable(const DWORD *function, size_t length, GLenum type, void *data);
    virtual ShaderExecutable *compileToExecutable(gl::InfoLog &infoLog, const char *shaderHLSL, GLenum type);

    // D3D11-renderer specific methods
    ID3D11Device *getDevice() { return mDevice; }
    ID3D11DeviceContext *getDeviceContext() { return mDeviceContext; };
    IDXGIFactory *getDxgiFactory() { return mDxgiFactory; };

  private:
    DISALLOW_COPY_AND_ASSIGN(Renderer11);

    HMODULE mD3d11Module;
    HMODULE mDxgiModule;
    HDC mDc;

    bool mDeviceLost;

    void initializeDevice();
    void releaseDeviceResources();

    RenderStateCache mStateCache;

    // current render target states
    bool mDepthStencilInitialized;
    bool mRenderTargetDescInitialized;
    rx::RenderTarget::Desc mRenderTargetDesc;
    unsigned int mCurDepthSize;

    // Currently applied blend state
    bool mForceSetBlendState;
    gl::BlendState mCurBlendState;
    gl::Color mCurBlendColor;
    unsigned int mCurSampleMask;

    // Currently applied rasterizer state
    bool mForceSetRasterState;
    gl::RasterizerState mCurRasterState;

    // Currently applied depth stencil state
    bool mForceSetDepthStencilState;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;

    // Currently applied scissor rectangle
    bool mForceSetScissor;
    gl::Rectangle mCurScissor;

    // Currently applied viewport
    bool mForceSetViewport;
    gl::Rectangle mCurViewport;
    float mCurNear;
    float mCurFar;

    ID3D11Device *mDevice;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11DeviceContext *mDeviceContext;
    IDXGIAdapter *mDxgiAdapter;
    DXGI_ADAPTER_DESC mAdapterDescription;
    char mDescription[128];
    IDXGIFactory *mDxgiFactory;
};

}
#endif // LIBGLESV2_RENDERER_RENDERER11_H_

//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager11.h: Defines a class for caching D3D11 state

#ifndef LIBANGLE_RENDERER_D3D11_STATEMANAGER11_H_
#define LIBANGLE_RENDERER_D3D11_STATEMANAGER11_H_

#include "libANGLE/angletypes.h"
#include "libANGLE/Data.h"
#include "libANGLE/State.h"
#include "libANGLE/renderer/d3d/d3d11/RenderStateCache.h"

namespace rx
{

class Renderer11;

class StateManager11 final : angle::NonCopyable
{
  public:
    StateManager11();
    ~StateManager11();

    void initialize(ID3D11DeviceContext *deviceContext, RenderStateCache *stateCache);

    void syncState(const gl::State &state, const gl::State::DirtyBits &dirtyBits);

    gl::Error setBlendState(const gl::Framebuffer *framebuffer,
                            const gl::BlendState &blendState,
                            const gl::ColorF &blendColor,
                            unsigned int sampleMask);

    gl::Error setDepthStencilState(const gl::State &glState);

    gl::Error setRasterizerState(const gl::RasterizerState &rasterState);

    void forceSetBlendState() { mBlendStateIsDirty = true; }
    void forceSetDepthStencilState() { mDepthStencilStateIsDirty = true; }
    void forceSetRasterState() { mRasterizerStateIsDirty = true; }

    void setCurScissorEnabled(bool enabled) { mCurScissorEnabled = enabled; }

    void updateStencilSizeIfChanged(bool depthStencilInitialized, unsigned int stencilSize);

  private:
    // Blend State
    bool mBlendStateIsDirty;
    // TODO(dianx) temporary representation of a dirty bit. once we move enough states in,
    // try experimenting with dirty bit instead of a bool
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    unsigned int mCurSampleMask;

    // Currently applied depth stencil state
    bool mDepthStencilStateIsDirty;
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;
    unsigned int mCurStencilSize;
    Optional<bool> mCurDisableDepth;
    Optional<bool> mCurDisableStencil;

    // Currenly applied rasterizer state
    bool mRasterizerStateIsDirty;
    gl::RasterizerState mCurRasterState;

    bool mCurScissorEnabled;

    ID3D11DeviceContext *mDeviceContext;
    RenderStateCache *mStateCache;
};

}  // namespace rx
#endif  // LIBANGLE_RENDERER_D3D11_STATEMANAGER11_H_
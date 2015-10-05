//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager11.h: Defines a class for caching D3D11 state

#ifndef LIBANGLE_RENDERER_D3D11_STATEMANAGER11_H_
#define LIBANGLE_RENDERER_D3D11_STATEMANAGER11_H_

#include "libANGLE/renderer/d3d/StateManagerD3D.h"

#include "libANGLE/renderer/d3d/d3d11/RenderStateCache.h"

namespace rx
{

class StateManager11 final : public StateManagerD3D
{
  public:
    StateManager11(ID3D11DeviceContext *deviceContext, RenderStateCache *stateCache);

    ~StateManager11() override;

    gl::Error setBlendState(const gl::Framebuffer *framebuffer,
                            const gl::BlendState &blendState,
                            const gl::ColorF &blendColor,
                            unsigned int sampleMask,
                            const gl::State::DirtyBits &dirtyBits) override;

    gl::Error setDepthStencilState(const gl::DepthStencilState &depthStencilState,
                                   int stencilRef,
                                   int stencilBackRef,
                                   bool frontFaceCCW,
                                   const gl::State::DirtyBits &dirtyBits) override;

    gl::Error setRasterizerState(const gl::RasterizerState &rasterizerState,
                                 const gl::State::DirtyBits &dirtyBits) override;

    const gl::RasterizerState &getCurRasterizerState();

  private:
    // TODO(dianx) d3d11 and cached states are not in sync, hence we need to force set things
    // but this should not be necessary.

    ID3D11DeviceContext *mDeviceContext;
    RenderStateCache *mStateCache;
};
}  // namespace rx
#endif
//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerD3D.h: Defines a class for caching D3D state

#ifndef LIBANGLE_RENDERER_D3D_STATEMANAGERD3D_H_
#define LIBANGLE_RENDERER_D3D_STATEMANAGERD3D_H_

#include "libANGLE/angletypes.h"
#include "libANGLE/Data.h"
#include "libANGLE/State.h"

namespace rx
{

class StateManagerD3D : angle::NonCopyable
{
  public:
    explicit StateManagerD3D();

    virtual ~StateManagerD3D();

    void syncExternalDirtyBits(const gl::State::DirtyBits &dirtyBits);

    gl::Error syncState(const gl::Data &data, const gl::State::DirtyBits &dirtyBits);

    virtual gl::Error setBlendState(const gl::Framebuffer *framebuffer,
                                    const gl::BlendState &blendState,
                                    const gl::ColorF &blendColor,
                                    unsigned int sampleMask,
                                    const gl::State::DirtyBits &dirtyBits) = 0;

    virtual gl::Error setDepthStencilState(const gl::DepthStencilState &depthStencilState,
                                           int stencilRef,
                                           int stencilBackRef,
                                           bool frontFaceCCW,
                                           const gl::State::DirtyBits &dirtyBits) = 0;

    virtual gl::Error setRasterizerState(const gl::RasterizerState &rasterizerState,
                                         const gl::State::DirtyBits &dirtyBits) = 0;

    void setRasterizerScissorEnabled(bool enabled);
    void setCurStencilSize(unsigned int size);
    unsigned int getCurStencilSize() const;

    void forceSetBlendState();
    void forceSetDepthStencilState();
    void forceSetRasterizerState();

    bool isForceSetRasterizerState() const;
    bool isForceSetDepthStencilState() const;
    bool isForceSetBlendState() const;

  protected:
    static bool IsBlendStateDirty(const gl::State::DirtyBits &dirtyBits);
    static bool IsDepthStencilStateDirty(const gl::State::DirtyBits &dirtyBits);
    static bool IsRasterizerStateDirty(const gl::State::DirtyBits &dirtyBits);

    void resetRasterizerForceBits();
    void resetBlendForceBits();
    void resetDepthStencilForceBits();

    bool mForceSetDepthStencilState;
    bool mForceSetBlendState;

    // Blend State
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    unsigned int mCurSampleMask;

    // Depth Stencil State
    gl::DepthStencilState mCurDepthStencilState;
    int mCurStencilRef;
    int mCurStencilBackRef;
    unsigned int mCurStencilSize;

    // Rasterizer State
    gl::RasterizerState mCurRasterizerState;

    // Scissor State
    bool mCurScissorTestEnabled;

    // Local force dirty bits
    gl::State::DirtyBits mLocalDirtyBits;
    // Copy of dirty bits in state. Synced on syncState. Should be removed after all states are
    // moved in
    gl::State::DirtyBits mExternalDirtyBits;
    static const gl::State::DirtyBits mBlendDirtyBits;
    static const gl::State::DirtyBits mDepthStencilDirtyBits;
    static const gl::State::DirtyBits mRasterizerDirtyBits;

  private:
    unsigned int getBlendStateMask(const gl::Framebuffer *framebufferObject,
                                   int samples,
                                   const gl::State &state) const;
};
}  // namespace rx
#endif
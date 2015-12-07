//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager9.h: Defines a class for caching D3D9 state

#ifndef LIBANGLE_RENDERER_D3D9_STATEMANAGER9_H_
#define LIBANGLE_RENDERER_D3D9_STATEMANAGER9_H_

#include "libANGLE/angletypes.h"
#include "libANGLE/Data.h"
#include "libANGLE/State.h"

namespace rx
{

class Renderer9;

class StateManager9 final : angle::NonCopyable
{
  public:
    StateManager9(Renderer9 *renderer9);
    ~StateManager9();

    void syncState(const gl::State &state, const gl::State::DirtyBits &dirtyBits);

    gl::Error setBlendState(const gl::Framebuffer *framebuffer,
                            const gl::BlendState &blendState,
                            const gl::ColorF &blendColor,
                            unsigned int sampleMask);

    void forceSetBlendState();
    void resetDirtyBits() { mDirtyBits.reset(); }
    VendorID getVendorId() const;

  private:
    void setBlendEnabled(bool enabled);
    void setBlendColor(const gl::BlendState &blendState, const gl::ColorF &blendColor);

    void setBlendFuncsEquations(const gl::BlendState &blendState);

    void setColorMask(const gl::Framebuffer *framebuffer,
                      bool red,
                      bool blue,
                      bool green,
                      bool alpha);

    void setSampleAlphaToCoverage(bool enabled);

    void setDither(bool dither);

    void setSampleMask(unsigned int sampleMask);

    enum DirtyBitType
    {
        DIRTY_BIT_BLEND_ENABLED,
        DIRTY_BIT_BLEND_COLOR,
        DIRTY_BIT_BLEND_FUNCS_EQUATIONS,
        DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE,
        DIRTY_BIT_COLOR_MASK,
        DIRTY_BIT_DITHER,
        DIRTY_BIT_SAMPLE_MASK,

        DIRTY_BIT_MAX
    };

    typedef std::bitset<DIRTY_BIT_MAX> DirtyBits;

    // Currently applied blend state
    gl::BlendState mCurBlendState;
    gl::ColorF mCurBlendColor;
    unsigned int mCurSampleMask;
    DirtyBits mBlendStateDirtyBits;

    Renderer9 *mRenderer9;
    DirtyBits mDirtyBits;
};

}  // namespace rx
#endif  // LIBANGLE_RENDERER_D3D9_STATEMANAGER9_H_

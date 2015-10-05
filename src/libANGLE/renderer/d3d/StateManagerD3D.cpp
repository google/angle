//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerD3D.cpp: Defines a class for caching D3D state

#include "libANGLE/renderer/d3d/StateManagerD3D.h"

#include "common/BitSetIterator.h"
#include "libANGLE/Framebuffer.h"

namespace rx
{

namespace
{

void SetGivenBitsDirty(gl::State::DirtyBits &dirtyBits,
                       const gl::State::DirtyBitType *givenBits,
                       size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        dirtyBits.set(givenBits[i]);
    }
}

const gl::State::DirtyBitType kRasterizerDirtyBits[] = {
    gl::State::DIRTY_BIT_CULL_FACE_ENABLED, gl::State::DIRTY_BIT_CULL_FACE,
    gl::State::DIRTY_BIT_FRONT_FACE,        gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED,
    gl::State::DIRTY_BIT_POLYGON_OFFSET,    gl::State::DIRTY_BIT_RASTERIZER_DISCARD_ENABLED};

const gl::State::DirtyBitType kDepthStencilDirtyBits[] = {
    gl::State::DIRTY_BIT_DEPTH_MASK,
    gl::State::DIRTY_BIT_DEPTH_TEST_ENABLED,
    gl::State::DIRTY_BIT_DEPTH_FUNC,
    gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED,
    gl::State::DIRTY_BIT_STENCIL_FUNCS_FRONT,
    gl::State::DIRTY_BIT_STENCIL_FUNCS_BACK,
    gl::State::DIRTY_BIT_STENCIL_WRITEMASK_FRONT,
    gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK,
    gl::State::DIRTY_BIT_STENCIL_OPS_FRONT,
    gl::State::DIRTY_BIT_STENCIL_OPS_BACK,
};

const gl::State::DirtyBitType kBlendStateDirtyBits[] = {
    gl::State::DIRTY_BIT_BLEND_EQUATIONS, gl::State::DIRTY_BIT_BLEND_FUNCS,
    gl::State::DIRTY_BIT_BLEND_ENABLED,   gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED,
    gl::State::DIRTY_BIT_DITHER_ENABLED,  gl::State::DIRTY_BIT_COLOR_MASK,
    gl::State::DIRTY_BIT_BLEND_COLOR};

}  // anonymous namespace

StateManagerD3D::StateManagerD3D()
    : mCurBlendColor(0, 0, 0, 0),
      mCurSampleMask(0),
      mCurStencilRef(0),
      mCurStencilBackRef(0),
      mCurStencilSize(0),
      mCurScissorTestEnabled(false),
      mLocalDirtyBits(),
      mExternalDirtyBits()
{
    mCurBlendState.blend                 = false;
    mCurBlendState.sourceBlendRGB        = GL_ONE;
    mCurBlendState.destBlendRGB          = GL_ZERO;
    mCurBlendState.sourceBlendAlpha      = GL_ONE;
    mCurBlendState.destBlendAlpha        = GL_ZERO;
    mCurBlendState.blendEquationRGB      = GL_FUNC_ADD;
    mCurBlendState.blendEquationAlpha    = GL_FUNC_ADD;
    mCurBlendState.colorMaskRed          = true;
    mCurBlendState.colorMaskBlue         = true;
    mCurBlendState.colorMaskGreen        = true;
    mCurBlendState.colorMaskAlpha        = true;
    mCurBlendState.sampleAlphaToCoverage = false;
    mCurBlendState.dither                = false;

    mCurDepthStencilState.depthTest                = false;
    mCurDepthStencilState.depthFunc                = GL_LESS;
    mCurDepthStencilState.depthMask                = true;
    mCurDepthStencilState.stencilTest              = false;
    mCurDepthStencilState.stencilMask              = true;
    mCurDepthStencilState.stencilFail              = GL_KEEP;
    mCurDepthStencilState.stencilPassDepthFail     = GL_KEEP;
    mCurDepthStencilState.stencilPassDepthPass     = GL_KEEP;
    mCurDepthStencilState.stencilWritemask         = static_cast<GLuint>(-1);
    mCurDepthStencilState.stencilBackFunc          = GL_ALWAYS;
    mCurDepthStencilState.stencilBackMask          = static_cast<GLuint>(-1);
    mCurDepthStencilState.stencilBackFail          = GL_KEEP;
    mCurDepthStencilState.stencilBackPassDepthFail = GL_KEEP;
    mCurDepthStencilState.stencilBackPassDepthPass = GL_KEEP;
    mCurDepthStencilState.stencilBackWritemask     = static_cast<GLuint>(-1);

    mCurRasterizerState.rasterizerDiscard   = false;
    mCurRasterizerState.cullFace            = false;
    mCurRasterizerState.cullMode            = GL_BACK;
    mCurRasterizerState.frontFace           = GL_CCW;
    mCurRasterizerState.polygonOffsetFill   = false;
    mCurRasterizerState.polygonOffsetFactor = 0.0f;
    mCurRasterizerState.polygonOffsetUnits  = 0.0f;
    mCurRasterizerState.pointDrawMode       = false;
    mCurRasterizerState.multiSample         = false;
}

StateManagerD3D::~StateManagerD3D()
{
}

const gl::State::DirtyBits StateManagerD3D::mRasterizerDirtyBits = []()
{
    gl::State::DirtyBits rasterizerDirtyBits;
    SetGivenBitsDirty(rasterizerDirtyBits, kRasterizerDirtyBits, ArraySize(kRasterizerDirtyBits));
    return rasterizerDirtyBits;
}();

const gl::State::DirtyBits StateManagerD3D::mDepthStencilDirtyBits = []()
{
    gl::State::DirtyBits depthStencilDirtyBits;
    SetGivenBitsDirty(depthStencilDirtyBits, kDepthStencilDirtyBits,
                      ArraySize(kDepthStencilDirtyBits));
    return depthStencilDirtyBits;
}();

const gl::State::DirtyBits StateManagerD3D::mBlendDirtyBits = []()
{
    gl::State::DirtyBits blendDirtyBits;
    SetGivenBitsDirty(blendDirtyBits, kBlendStateDirtyBits, ArraySize(kBlendStateDirtyBits));
    return blendDirtyBits;
}();

bool StateManagerD3D::IsBlendStateDirty(const gl::State::DirtyBits &dirtyBits)
{
    return (dirtyBits & mBlendDirtyBits).any();
}

bool StateManagerD3D::IsDepthStencilStateDirty(const gl::State::DirtyBits &dirtyBits)
{
    return (dirtyBits & mDepthStencilDirtyBits).any();
}

bool StateManagerD3D::IsRasterizerStateDirty(const gl::State::DirtyBits &dirtyBits)
{
    return (dirtyBits & mRasterizerDirtyBits).any();
}

void StateManagerD3D::resetRasterizerForceBits()
{
    // If a bit is inside of mRasterizerDirtyBits, we want to reset it, otherwise, leave it alone
    mLocalDirtyBits &= (~mRasterizerDirtyBits);
}

void StateManagerD3D::resetBlendForceBits()
{
    mLocalDirtyBits &= (~mBlendDirtyBits);
}

void StateManagerD3D::resetDepthStencilForceBits()
{
    mLocalDirtyBits &= (~mDepthStencilDirtyBits);
}

bool StateManagerD3D::isForceSetRasterizerState() const
{
    return (mLocalDirtyBits & mRasterizerDirtyBits).any();
}

bool StateManagerD3D::isForceSetDepthStencilState() const
{
    return (mLocalDirtyBits & mDepthStencilDirtyBits).any();
}

bool StateManagerD3D::isForceSetBlendState() const
{
    return (mLocalDirtyBits & mBlendDirtyBits).any();
}

void StateManagerD3D::setCurStencilSize(unsigned int size)
{
    mCurStencilSize = size;
}

unsigned int StateManagerD3D::getCurStencilSize() const
{
    return mCurStencilSize;
}

void StateManagerD3D::setRasterizerScissorEnabled(bool enabled)
{
    mCurScissorTestEnabled = enabled;
}

unsigned int StateManagerD3D::getBlendStateMask(const gl::Framebuffer *framebufferObject,
                                                int samples,
                                                const gl::State &state) const
{
    unsigned int mask = 0;
    if (state.isSampleCoverageEnabled())
    {
        GLclampf coverageValue = state.getSampleCoverageValue();
        if (coverageValue != 0)
        {
            float threshold = 0.5f;

            for (int i = 0; i < samples; ++i)
            {
                mask <<= 1;

                if ((i + 1) * coverageValue >= threshold)
                {
                    threshold += 1.0f;
                    mask |= 1;
                }
            }
        }

        bool coverageInvert = state.getSampleCoverageInvert();
        if (coverageInvert)
        {
            mask = ~mask;
        }
    }
    else
    {
        mask = 0xFFFFFFFF;
    }

    return mask;
}

void StateManagerD3D::forceSetBlendState()
{
    mLocalDirtyBits |= mBlendDirtyBits;
}

void StateManagerD3D::forceSetDepthStencilState()
{
    mLocalDirtyBits |= mDepthStencilDirtyBits;
}

void StateManagerD3D::forceSetRasterizerState()
{
    mLocalDirtyBits |= mRasterizerDirtyBits;
}

void StateManagerD3D::syncExternalDirtyBits(const gl::State::DirtyBits &dirtyBits)
{
    mExternalDirtyBits = dirtyBits;
}

gl::Error StateManagerD3D::syncState(const gl::Data &data, const gl::State::DirtyBits &dirtyBits)
{
    const gl::Framebuffer *framebufferObject = data.state->getDrawFramebuffer();
    int samples                              = framebufferObject->getSamples(data);

    gl::State::DirtyBits allDirtyBits = mExternalDirtyBits | mLocalDirtyBits;

    unsigned int sampleMask = getBlendStateMask(framebufferObject, samples, *(data.state));
    gl::Error error = setBlendState(framebufferObject, data.state->getBlendState(),
                                    data.state->getBlendColor(), sampleMask, allDirtyBits);

    if (error.isError())
    {
        return error;
    }

    resetBlendForceBits();

    error = setDepthStencilState(data.state->getDepthStencilState(), data.state->getStencilRef(),
                                 data.state->getStencilBackRef(),
                                 data.state->getRasterizerState().frontFace == GL_CCW,
                                 data.state->getDirtyBits());

    if (error.isError())
    {
        return error;
    }

    resetDepthStencilForceBits();

    mExternalDirtyBits.reset();

    return gl::Error(GL_NO_ERROR);
}
}  // namespace rx
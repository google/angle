//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager9.cpp: Defines a class for caching D3D9 state
#include "libANGLE/renderer/d3d/d3d9/StateManager9.h"

#include "common/BitSetIterator.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/d3d/d3d9/renderer9_utils.h"
#include "libANGLE/renderer/d3d/d3d9/Framebuffer9.h"
#include "libANGLE/renderer/d3d/d3d9/Renderer9.h"

namespace rx
{

StateManager9::StateManager9(Renderer9 *renderer9)
    : mCurBlendState(),
      mCurBlendColor(0, 0, 0, 0),
      mCurSampleMask(0),
      mCurRasterState(),
      mCurDepthSize(0),
      mRenderer9(renderer9),
      mDirtyBits()
{
    mBlendStateDirtyBits.set(DIRTY_BIT_BLEND_ENABLED);
    mBlendStateDirtyBits.set(DIRTY_BIT_BLEND_COLOR);
    mBlendStateDirtyBits.set(DIRTY_BIT_BLEND_FUNCS_EQUATIONS);
    mBlendStateDirtyBits.set(DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE);
    mBlendStateDirtyBits.set(DIRTY_BIT_COLOR_MASK);
    mBlendStateDirtyBits.set(DIRTY_BIT_DITHER);
    mBlendStateDirtyBits.set(DIRTY_BIT_SAMPLE_MASK);

    mRasterizerStateDirtyBits.set(DIRTY_BIT_CULL_MODE);
    mRasterizerStateDirtyBits.set(DIRTY_BIT_DEPTH_BIAS);
}

StateManager9::~StateManager9()
{
}

void StateManager9::forceSetBlendState()
{
    mDirtyBits |= mBlendStateDirtyBits;
}

void StateManager9::forceSetRasterState()
{
    mDirtyBits |= mRasterizerStateDirtyBits;
}

void StateManager9::syncState(const gl::State &state, const gl::State::DirtyBits &dirtyBits)
{
    for (unsigned int dirtyBit : angle::IterateBitSet(dirtyBits))
    {
        switch (dirtyBit)
        {
            case gl::State::DIRTY_BIT_BLEND_ENABLED:
                if (state.getBlendState().blend != mCurBlendState.blend)
                {
                    mDirtyBits.set(DIRTY_BIT_BLEND_ENABLED);
                    // BlendColor and funcs and equations has to be set if blend is enabled
                    mDirtyBits.set(DIRTY_BIT_BLEND_COLOR);
                    mDirtyBits.set(DIRTY_BIT_BLEND_FUNCS_EQUATIONS);
                }
                break;
            case gl::State::DIRTY_BIT_BLEND_FUNCS:
            {
                const gl::BlendState &blendState = state.getBlendState();
                if (blendState.sourceBlendRGB != mCurBlendState.sourceBlendRGB ||
                    blendState.destBlendRGB != mCurBlendState.destBlendRGB ||
                    blendState.sourceBlendAlpha != mCurBlendState.sourceBlendAlpha ||
                    blendState.destBlendAlpha != mCurBlendState.destBlendAlpha)
                {
                    mDirtyBits.set(DIRTY_BIT_BLEND_FUNCS_EQUATIONS);
                    // BlendColor depends on the values of blend funcs
                    mDirtyBits.set(DIRTY_BIT_BLEND_COLOR);
                }
                break;
            }
            case gl::State::DIRTY_BIT_BLEND_EQUATIONS:
            {
                const gl::BlendState &blendState = state.getBlendState();
                if (blendState.blendEquationRGB != mCurBlendState.blendEquationRGB ||
                    blendState.blendEquationAlpha != mCurBlendState.blendEquationAlpha)
                {
                    mDirtyBits.set(DIRTY_BIT_BLEND_FUNCS_EQUATIONS);
                }
                break;
            }
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED:
                if (state.getBlendState().sampleAlphaToCoverage !=
                    mCurBlendState.sampleAlphaToCoverage)
                {
                    mDirtyBits.set(DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE);
                }
                break;
            case gl::State::DIRTY_BIT_COLOR_MASK:
            {
                const gl::BlendState &blendState = state.getBlendState();
                if (blendState.colorMaskRed != mCurBlendState.colorMaskRed ||
                    blendState.colorMaskGreen != mCurBlendState.colorMaskGreen ||
                    blendState.colorMaskBlue != mCurBlendState.colorMaskBlue ||
                    blendState.colorMaskAlpha != mCurBlendState.colorMaskAlpha)
                {
                    mDirtyBits.set(DIRTY_BIT_COLOR_MASK);
                }
                break;
            }
            case gl::State::DIRTY_BIT_DITHER_ENABLED:
                if (state.getBlendState().dither != mCurBlendState.dither)
                {
                    mDirtyBits.set(DIRTY_BIT_DITHER);
                }
                break;
            case gl::State::DIRTY_BIT_BLEND_COLOR:
                if (state.getBlendColor() != mCurBlendColor)
                {
                    mDirtyBits.set(DIRTY_BIT_BLEND_COLOR);
                }
                break;
            case gl::State::DIRTY_BIT_CULL_FACE_ENABLED:
                if (state.getRasterizerState().cullFace != mCurRasterState.cullFace)
                {
                    mDirtyBits.set(DIRTY_BIT_CULL_MODE);
                }
                break;
            case gl::State::DIRTY_BIT_CULL_FACE:
                if (state.getRasterizerState().cullMode != mCurRasterState.cullMode)
                {
                    mDirtyBits.set(DIRTY_BIT_CULL_MODE);
                }
                break;
            case gl::State::DIRTY_BIT_FRONT_FACE:
                if (state.getRasterizerState().frontFace != mCurRasterState.frontFace)
                {
                    mDirtyBits.set(DIRTY_BIT_CULL_MODE);
                }
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED:
                if (state.getRasterizerState().polygonOffsetFill !=
                    mCurRasterState.polygonOffsetFill)
                {
                    mDirtyBits.set(DIRTY_BIT_DEPTH_BIAS);
                }
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET:
            {
                const gl::RasterizerState &rasterizerState = state.getRasterizerState();
                if (rasterizerState.polygonOffsetFactor != mCurRasterState.polygonOffsetFactor ||
                    rasterizerState.polygonOffsetUnits != mCurRasterState.polygonOffsetUnits)
                {
                    mDirtyBits.set(DIRTY_BIT_DEPTH_BIAS);
                }
                break;
            }
            default:
                break;
        }
    }
}

gl::Error StateManager9::setBlendAndRasterizerState(const gl::State &glState,
                                                    unsigned int sampleMask)
{
    const gl::Framebuffer *framebuffer     = glState.getDrawFramebuffer();
    const gl::BlendState &blendState       = glState.getBlendState();
    const gl::ColorF &blendColor           = glState.getBlendColor();
    const gl::RasterizerState &rasterState = glState.getRasterizerState();

    for (auto dirtyBit : angle::IterateBitSet(mDirtyBits))
    {
        switch (dirtyBit)
        {
            case DIRTY_BIT_BLEND_ENABLED:
                setBlendEnabled(blendState.blend);
                break;
            case DIRTY_BIT_BLEND_COLOR:
                setBlendColor(blendState, blendColor);
                break;
            case DIRTY_BIT_BLEND_FUNCS_EQUATIONS:
                setBlendFuncsEquations(blendState);
                break;
            case DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE:
                setSampleAlphaToCoverage(blendState.sampleAlphaToCoverage);
                break;
            case DIRTY_BIT_COLOR_MASK:
                setColorMask(framebuffer, blendState.colorMaskRed, blendState.colorMaskBlue,
                             blendState.colorMaskGreen, blendState.colorMaskAlpha);
                break;
            case DIRTY_BIT_DITHER:
                setDither(blendState.dither);
                break;
            case DIRTY_BIT_CULL_MODE:
                setCullMode(rasterState.cullFace, rasterState.cullMode, rasterState.frontFace);
                break;
            case DIRTY_BIT_DEPTH_BIAS:
                setDepthBias(rasterState.polygonOffsetFill, rasterState.polygonOffsetFactor,
                             rasterState.polygonOffsetUnits);
            default:
                break;
        }
    }

    if (sampleMask != mCurSampleMask)
    {
        setSampleMask(sampleMask);
    }

    return gl::Error(GL_NO_ERROR);
}

// TODO(dianx) one bit for sampleAlphaToCoverage
void StateManager9::setSampleAlphaToCoverage(bool enabled)
{
    if (enabled)
    {
        FIXME("Sample alpha to coverage is unimplemented.");
    }
}

void StateManager9::setBlendColor(const gl::BlendState &blendState, const gl::ColorF &blendColor)
{
    if (!blendState.blend)
        return;

    if (blendState.sourceBlendRGB != GL_CONSTANT_ALPHA &&
        blendState.sourceBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA &&
        blendState.destBlendRGB != GL_CONSTANT_ALPHA &&
        blendState.destBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA)
    {
        mRenderer9->getDevice()->SetRenderState(D3DRS_BLENDFACTOR,
                                                gl_d3d9::ConvertColor(blendColor));
    }
    else
    {
        mRenderer9->getDevice()->SetRenderState(
            D3DRS_BLENDFACTOR,
            D3DCOLOR_RGBA(gl::unorm<8>(blendColor.alpha), gl::unorm<8>(blendColor.alpha),
                          gl::unorm<8>(blendColor.alpha), gl::unorm<8>(blendColor.alpha)));
    }
    mCurBlendColor = blendColor;
}

void StateManager9::setBlendFuncsEquations(const gl::BlendState &blendState)
{
    if (!blendState.blend)
        return;

    IDirect3DDevice9 *device = mRenderer9->getDevice();

    device->SetRenderState(D3DRS_SRCBLEND, gl_d3d9::ConvertBlendFunc(blendState.sourceBlendRGB));
    device->SetRenderState(D3DRS_DESTBLEND, gl_d3d9::ConvertBlendFunc(blendState.destBlendRGB));
    device->SetRenderState(D3DRS_BLENDOP, gl_d3d9::ConvertBlendOp(blendState.blendEquationRGB));

    if (blendState.sourceBlendRGB != blendState.sourceBlendAlpha ||
        blendState.destBlendRGB != blendState.destBlendAlpha ||
        blendState.blendEquationRGB != blendState.blendEquationAlpha)
    {
        device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

        device->SetRenderState(D3DRS_SRCBLENDALPHA,
                               gl_d3d9::ConvertBlendFunc(blendState.sourceBlendAlpha));
        device->SetRenderState(D3DRS_DESTBLENDALPHA,
                               gl_d3d9::ConvertBlendFunc(blendState.destBlendAlpha));
        device->SetRenderState(D3DRS_BLENDOPALPHA,
                               gl_d3d9::ConvertBlendOp(blendState.blendEquationAlpha));
    }
    else
    {
        device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
    }

    mCurBlendState.sourceBlendRGB     = blendState.sourceBlendRGB;
    mCurBlendState.destBlendRGB       = blendState.destBlendRGB;
    mCurBlendState.blendEquationRGB   = blendState.blendEquationRGB;
    mCurBlendState.blendEquationAlpha = blendState.blendEquationAlpha;
}

void StateManager9::setBlendEnabled(bool enabled)
{
    mRenderer9->getDevice()->SetRenderState(D3DRS_ALPHABLENDENABLE, enabled ? TRUE : FALSE);
    mCurBlendState.blend = enabled;
}

void StateManager9::setDither(bool dither)
{
    mRenderer9->getDevice()->SetRenderState(D3DRS_DITHERENABLE, dither ? TRUE : FALSE);
    mCurBlendState.dither = dither;
}

// TODO(dianx) one bit for color mask
void StateManager9::setColorMask(const gl::Framebuffer *framebuffer,
                                 bool red,
                                 bool blue,
                                 bool green,
                                 bool alpha)
{
    // Set the color mask
    bool zeroColorMaskAllowed = mRenderer9->getVendorId() != VENDOR_ID_AMD;
    // Apparently some ATI cards have a bug where a draw with a zero color
    // write mask can cause later draws to have incorrect results. Instead,
    // set a nonzero color write mask but modify the blend state so that no
    // drawing is done.
    // http://code.google.com/p/angleproject/issues/detail?id=169

    const gl::FramebufferAttachment *attachment = framebuffer->getFirstColorbuffer();
    GLenum internalFormat                       = attachment ? attachment->getInternalFormat() : GL_NONE;

    const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);

    DWORD colorMask = gl_d3d9::ConvertColorMask(
        formatInfo.redBits > 0 && red, formatInfo.greenBits > 0 && green,
        formatInfo.blueBits > 0 && blue, formatInfo.alphaBits > 0 && alpha);

    if (colorMask == 0 && !zeroColorMaskAllowed)
    {
        IDirect3DDevice9 *device = mRenderer9->getDevice();
        // Enable green channel, but set blending so nothing will be drawn.
        device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

        device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
        device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
        device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
    }
    else
    {
        mRenderer9->getDevice()->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
    }

    mCurBlendState.colorMaskRed   = red;
    mCurBlendState.colorMaskGreen = green;
    mCurBlendState.colorMaskBlue  = blue;
    mCurBlendState.colorMaskAlpha = alpha;
}

void StateManager9::setSampleMask(unsigned int sampleMask)
{
    IDirect3DDevice9 *device = mRenderer9->getDevice();
    // Set the multisample mask
    device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
    device->SetRenderState(D3DRS_MULTISAMPLEMASK, static_cast<DWORD>(sampleMask));

    mCurSampleMask = sampleMask;
}

void StateManager9::setCullMode(bool cullFace, GLenum cullMode, GLenum frontFace)
{
    if (cullFace)
    {
        mRenderer9->getDevice()->SetRenderState(D3DRS_CULLMODE,
                                                gl_d3d9::ConvertCullMode(cullMode, frontFace));
    }
    else
    {
        mRenderer9->getDevice()->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    }

    mCurRasterState.cullFace  = cullFace;
    mCurRasterState.cullMode  = cullMode;
    mCurRasterState.frontFace = frontFace;
}

void StateManager9::setDepthBias(bool polygonOffsetFill,
                                 GLfloat polygonOffsetFactor,
                                 GLfloat polygonOffsetUnits)
{
    if (polygonOffsetFill)
    {
        if (mCurDepthSize > 0)
        {
            IDirect3DDevice9 *device = mRenderer9->getDevice();
            device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&polygonOffsetFactor);

            float depthBias = ldexp(polygonOffsetUnits, -static_cast<int>(mCurDepthSize));
            device->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&depthBias);
        }
    }
    else
    {
        IDirect3DDevice9 *device = mRenderer9->getDevice();
        device->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
        device->SetRenderState(D3DRS_DEPTHBIAS, 0);
    }

    mCurRasterState.polygonOffsetFill   = polygonOffsetFill;
    mCurRasterState.polygonOffsetFactor = polygonOffsetFactor;
    mCurRasterState.polygonOffsetUnits  = polygonOffsetUnits;
}

void StateManager9::updateDepthSizeIfChanged(bool depthStencilInitialized, unsigned int depthSize)
{
    if (!depthStencilInitialized || depthSize != mCurDepthSize)
    {
        mCurDepthSize = depthSize;
        forceSetRasterState();
    }
}
}  // namespace rx

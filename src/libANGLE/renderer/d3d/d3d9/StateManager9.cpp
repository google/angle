//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager9.cpp: Defines a class for caching D3D9 state

#include "libANGLE/renderer/d3d/d3d9/StateManager9.h"

#include "common/BitSetIterator.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/renderer/d3d/d3d9/renderer9_utils.h"

namespace rx
{

StateManager9::StateManager9(IDirect3DDevice9 *device, D3DADAPTER_IDENTIFIER9 &adapterIdentifier)
    : mDevice(device),
      mAdapterIdentifier(adapterIdentifier),
      mCurDepthSize(0),
      mCurFrontFaceCCW(false)
{
}

StateManager9::~StateManager9()
{
}

VendorID StateManager9::getVendorId() const
{
    return static_cast<VendorID>(mAdapterIdentifier.VendorId);
}

void StateManager9::setCurDepthSize(unsigned int size)
{
    mCurDepthSize = size;
}

unsigned int StateManager9::getCurDepthSize() const
{
    return mCurDepthSize;
}

gl::Error StateManager9::setBlendState(const gl::Framebuffer *framebuffer,
                                       const gl::BlendState &blendState,
                                       const gl::ColorF &blendColor,
                                       unsigned int sampleMask,
                                       const gl::State::DirtyBits &dirtyBits)
{
    if (dirtyBits.test(gl::State::DIRTY_BIT_BLEND_ENABLED) ||
        dirtyBits.test(gl::State::DIRTY_BIT_BLEND_FUNCS) ||
        dirtyBits.test(gl::State::DIRTY_BIT_BLEND_EQUATIONS))
    {
        setBlendEnableFuncsEquations(blendState, blendColor);
    }

    if (dirtyBits.test(gl::State::DIRTY_BIT_BLEND_COLOR))
    {
        setBlendColor(blendColor, blendState);
    }

    if (dirtyBits.test(gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED))
    {
        setSampleAlphaToCoverageEnabled(blendState.sampleAlphaToCoverage);
    }

    if (dirtyBits.test(gl::State::DIRTY_BIT_DITHER_ENABLED))
    {
        setDitherEnabled(blendState.dither);
    }

    if (dirtyBits.test(gl::State::DIRTY_BIT_COLOR_MASK))
    {
        setBlendColorMask(blendState, framebuffer);
    }

    if (mCurSampleMask != sampleMask)
    {
        setSampleMask(sampleMask);
    }

    return gl::Error(GL_NO_ERROR);
}

void StateManager9::setBlendEnableFuncsEquations(const gl::BlendState &blendState,
                                                 const gl::ColorF &blendColor)
{
    if (blendState.blend != mCurBlendState.blend ||
        blendState.sourceBlendRGB != mCurBlendState.sourceBlendRGB ||
        blendState.destBlendRGB != mCurBlendState.destBlendRGB ||
        blendState.sourceBlendAlpha != mCurBlendState.sourceBlendAlpha ||
        blendState.destBlendAlpha != mCurBlendState.destBlendAlpha ||
        blendState.blendEquationRGB != mCurBlendState.blendEquationRGB ||
        blendState.blendEquationAlpha != mCurBlendState.blendEquationAlpha)
    {
        if (blendState.blend)
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            mDevice->SetRenderState(D3DRS_SRCBLEND,
                                    gl_d3d9::ConvertBlendFunc(blendState.sourceBlendRGB));
            mDevice->SetRenderState(D3DRS_DESTBLEND,
                                    gl_d3d9::ConvertBlendFunc(blendState.destBlendRGB));
            mDevice->SetRenderState(D3DRS_BLENDOP,
                                    gl_d3d9::ConvertBlendOp(blendState.blendEquationRGB));

            if (blendState.sourceBlendRGB != blendState.sourceBlendAlpha ||
                blendState.destBlendRGB != blendState.destBlendAlpha ||
                blendState.blendEquationRGB != blendState.blendEquationAlpha)
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);

                mDevice->SetRenderState(D3DRS_SRCBLENDALPHA,
                                        gl_d3d9::ConvertBlendFunc(blendState.sourceBlendAlpha));
                mDevice->SetRenderState(D3DRS_DESTBLENDALPHA,
                                        gl_d3d9::ConvertBlendFunc(blendState.destBlendAlpha));
                mDevice->SetRenderState(D3DRS_BLENDOPALPHA,
                                        gl_d3d9::ConvertBlendOp(blendState.blendEquationAlpha));
            }
            else
            {
                mDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, FALSE);
            }

            mCurBlendState.sourceBlendRGB   = blendState.sourceBlendRGB;
            mCurBlendState.destBlendRGB     = blendState.destBlendRGB;
            mCurBlendState.sourceBlendAlpha = blendState.sourceBlendAlpha;
            mCurBlendState.destBlendAlpha   = blendState.destBlendAlpha;
        }
        else
        {
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        }

        mCurBlendState.blend = blendState.blend;
    }
}

void StateManager9::setBlendColor(const gl::ColorF &blendColor, const gl::BlendState &blendState)
{
    if (blendState.blend &&
        (blendColor.red != mCurBlendColor.red || blendColor.green != mCurBlendColor.green ||
         blendColor.blue != mCurBlendColor.blue || blendColor.alpha != mCurBlendColor.alpha))
    {
        if (blendState.sourceBlendRGB != GL_CONSTANT_ALPHA &&
            blendState.sourceBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA &&
            blendState.destBlendRGB != GL_CONSTANT_ALPHA &&
            blendState.destBlendRGB != GL_ONE_MINUS_CONSTANT_ALPHA)
        {
            mDevice->SetRenderState(D3DRS_BLENDFACTOR, gl_d3d9::ConvertColor(blendColor));
        }
        else
        {
            mDevice->SetRenderState(
                D3DRS_BLENDFACTOR,
                D3DCOLOR_RGBA(gl::unorm<8>(blendColor.alpha), gl::unorm<8>(blendColor.alpha),
                              gl::unorm<8>(blendColor.alpha), gl::unorm<8>(blendColor.alpha)));
        }

        mCurBlendColor.alpha = blendColor.alpha;
        mCurBlendColor.red   = blendColor.red;
        mCurBlendColor.green = blendColor.green;
        mCurBlendColor.blue  = blendColor.blue;
    }
}

void StateManager9::setSampleAlphaToCoverageEnabled(bool sampleAlphaToCoverage)
{
    if (sampleAlphaToCoverage)
    {
        FIXME("Sample alpha to coverage is unimplemented.");
    }
}

void StateManager9::setDitherEnabled(bool ditherEnabled)
{
    if (mCurBlendState.dither != ditherEnabled)
    {
        mDevice->SetRenderState(D3DRS_DITHERENABLE, ditherEnabled ? TRUE : FALSE);
        mCurBlendState.dither = ditherEnabled;
    }
}

void StateManager9::setBlendColorMask(const gl::BlendState &blendState,
                                      const gl::Framebuffer *framebuffer)
{
    if (mCurBlendState.colorMaskRed != blendState.colorMaskRed ||
        mCurBlendState.colorMaskGreen != blendState.colorMaskGreen ||
        mCurBlendState.colorMaskBlue != blendState.colorMaskBlue ||
        mCurBlendState.colorMaskAlpha != blendState.colorMaskAlpha)
    {
        const gl::FramebufferAttachment *attachment = framebuffer->getFirstColorbuffer();
        GLenum internalFormat                       = attachment ? attachment->getInternalFormat() : GL_NONE;

        // Set the color mask
        bool zeroColorMaskAllowed = getVendorId() != VENDOR_ID_AMD;
        // Apparently some ATI cards have a bug where a draw with a zero color
        // write mask can cause later draws to have incorrect results. Instead,
        // set a nonzero color write mask but modify the blend state so that no
        // drawing is done.
        // http://code.google.com/p/angleproject/issues/detail?id=169

        const gl::InternalFormat &formatInfo = gl::GetInternalFormatInfo(internalFormat);
        DWORD colorMask =
            gl_d3d9::ConvertColorMask(formatInfo.redBits > 0 && blendState.colorMaskRed,
                                      formatInfo.greenBits > 0 && blendState.colorMaskGreen,
                                      formatInfo.blueBits > 0 && blendState.colorMaskBlue,
                                      formatInfo.alphaBits > 0 && blendState.colorMaskAlpha);
        if (colorMask == 0 && !zeroColorMaskAllowed)
        {
            // Enable green channel, but set blending so nothing will be drawn.
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_GREEN);
            mDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

            mDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
            mDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
            mDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
        }
        else
        {
            mDevice->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
        }

        mCurBlendState.colorMaskRed   = blendState.colorMaskRed;
        mCurBlendState.colorMaskBlue  = blendState.colorMaskBlue;
        mCurBlendState.colorMaskGreen = blendState.colorMaskGreen;
        mCurBlendState.colorMaskAlpha = blendState.colorMaskAlpha;
    }
}

void StateManager9::setSampleMask(unsigned int sampleMask)
{
    // Set the multisample mask
    if (mCurSampleMask != sampleMask)
    {
        mDevice->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE);
        mDevice->SetRenderState(D3DRS_MULTISAMPLEMASK, static_cast<DWORD>(sampleMask));
        mCurSampleMask = sampleMask;
    }
}

gl::Error StateManager9::setDepthStencilState(const gl::DepthStencilState &depthStencilState,
                                              int stencilRef,
                                              int stencilBackRef,
                                              bool frontFaceCCW,
                                              const gl::State::DirtyBits &dirtyBits)
{
    // TODO(dianx) we should use dirty bits here
    unsigned int maxStencil = (1 << mCurStencilSize) - 1;

    ASSERT((depthStencilState.stencilWritemask & maxStencil) ==
           (depthStencilState.stencilBackWritemask & maxStencil));
    ASSERT(stencilRef == stencilBackRef);
    ASSERT((depthStencilState.stencilMask & maxStencil) ==
           (depthStencilState.stencilBackMask & maxStencil));

    bool forceSetDepthStencilState = isForceSetDepthStencilState();

    // DIRTY_BIT_DEPTH_MASK
    if (forceSetDepthStencilState || depthStencilState.depthMask != mCurDepthStencilState.depthMask)
    {
        setDepthMask(depthStencilState.depthMask);
    }

    // DIRTY_BIT_DEPTH_TEST_ENABLED
    // DIRTY_BIT_DEPTH_FUNC
    if (forceSetDepthStencilState ||
        depthStencilState.depthTest != mCurDepthStencilState.depthTest ||
        depthStencilState.depthFunc != mCurDepthStencilState.depthFunc)
    {
        setDepthTestAndFunc(depthStencilState.depthTest, depthStencilState.depthFunc);
    }

    // DIRTY_BIT_STENCIL_TEST_ENABLED
    if (forceSetDepthStencilState ||
        depthStencilState.stencilTest != mCurDepthStencilState.stencilTest)
    {
        setStencilTestEnabled(depthStencilState.stencilTest);
    }

    // DIRTY_BIT_STENCIL_FUNCS_FRONT
    if (forceSetDepthStencilState ||
        depthStencilState.stencilFunc != mCurDepthStencilState.stencilFunc ||
        depthStencilState.stencilMask != mCurDepthStencilState.stencilMask ||
        stencilRef != mCurStencilRef || frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilFuncsFront(depthStencilState.stencilFunc, depthStencilState.stencilMask,
                             stencilRef, maxStencil, frontFaceCCW);
    }

    // DIRTY_BIT_STENCIL_FUNCS_BACK
    if (forceSetDepthStencilState ||
        depthStencilState.stencilBackFunc != mCurDepthStencilState.stencilBackFunc ||
        depthStencilState.stencilBackMask != mCurDepthStencilState.stencilMask ||
        stencilBackRef != mCurStencilBackRef || frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilFuncsBack(depthStencilState.stencilBackFunc, depthStencilState.stencilBackMask,
                            stencilBackRef, maxStencil, frontFaceCCW);
    }

    // DIRTY_BIT_STENCIL_WRITEMASK_FRONT
    if (forceSetDepthStencilState ||
        depthStencilState.stencilWritemask != mCurDepthStencilState.stencilWritemask ||
        frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilWriteMaskFront(depthStencilState.stencilWritemask, frontFaceCCW);
    }

    // DIRTY_BIT_STENCIL_WRITEMASK_BACK
    if (forceSetDepthStencilState ||
        depthStencilState.stencilBackWritemask != mCurDepthStencilState.stencilBackWritemask ||
        frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilWriteMaskBack(depthStencilState.stencilBackWritemask, frontFaceCCW);
    }

    // DIRTY_BIT_STENCIL_OPS_FRONT
    if (forceSetDepthStencilState ||
        depthStencilState.stencilFail != mCurDepthStencilState.stencilFail ||
        depthStencilState.stencilPassDepthFail != mCurDepthStencilState.stencilPassDepthFail ||
        depthStencilState.stencilPassDepthPass != mCurDepthStencilState.stencilPassDepthPass ||
        frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilOpsFront(depthStencilState.stencilFail, depthStencilState.stencilPassDepthFail,
                           depthStencilState.stencilPassDepthPass, frontFaceCCW);
    }

    // DIRTY_BIT_STENCIL_OPS_BACK
    if (forceSetDepthStencilState ||
        depthStencilState.stencilBackFail != mCurDepthStencilState.stencilBackFail ||
        depthStencilState.stencilBackPassDepthFail !=
            mCurDepthStencilState.stencilBackPassDepthFail ||
        depthStencilState.stencilBackPassDepthPass !=
            mCurDepthStencilState.stencilBackPassDepthPass ||
        frontFaceCCW != mCurFrontFaceCCW)
    {
        setStencilOpsBack(depthStencilState.stencilBackFail,
                          depthStencilState.stencilBackPassDepthFail,
                          depthStencilState.stencilBackPassDepthPass, frontFaceCCW);
    }

    mCurFrontFaceCCW = frontFaceCCW;

    return gl::Error(GL_NO_ERROR);
}

gl::Error StateManager9::setRasterizerState(const gl::RasterizerState &rasterizerState,
                                            const gl::State::DirtyBits &dirtyBits)
{
    // TODO(dianx) setRasterizerState is being called after syncRendererState in Context.cpp
    // which means the force bits are being cleared before this call is reached. The current
    // fix only resets the necessary dirty force bits. But we really want to be using dirty bits
    // instead of memcmping to check for changes

    bool forceSetRasterizerState = isForceSetRasterizerState();

    if (forceSetRasterizerState || mCurRasterizerState.cullFace != rasterizerState.cullFace ||
        mCurRasterizerState.cullMode != rasterizerState.cullMode ||
        mCurRasterizerState.frontFace != rasterizerState.frontFace)
    {
        setRasterizerMode(rasterizerState.cullFace, rasterizerState.cullMode,
                          rasterizerState.frontFace);
    }

    if (forceSetRasterizerState ||
        mCurRasterizerState.polygonOffsetFill != rasterizerState.polygonOffsetFill ||
        mCurRasterizerState.polygonOffsetFactor != rasterizerState.polygonOffsetFactor ||
        mCurRasterizerState.polygonOffsetUnits != rasterizerState.polygonOffsetUnits)
    {
        setRasterizerPolygonOffset(rasterizerState.polygonOffsetFill,
                                   rasterizerState.polygonOffsetFactor,
                                   rasterizerState.polygonOffsetUnits);
    }

    return gl::Error(GL_NO_ERROR);
}

void StateManager9::setRasterizerMode(bool cullFace, GLenum cullMode, GLenum frontFace)
{
    mDevice->SetRenderState(
        D3DRS_CULLMODE, cullFace ? gl_d3d9::ConvertCullMode(cullMode, frontFace) : D3DCULL_NONE);

    mCurRasterizerState.cullFace  = cullFace;
    mCurRasterizerState.cullMode  = cullMode;
    mCurRasterizerState.frontFace = frontFace;
}

void StateManager9::setRasterizerPolygonOffset(bool polygonOffsetFill,
                                               GLfloat polygonOffsetFactor,
                                               GLfloat polygonOffsetUnits)
{
    if (polygonOffsetFill)
    {
        if (mCurDepthSize > 0)
        {
            mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD *)&polygonOffsetFactor);

            float depthBias = ldexp(polygonOffsetUnits, -static_cast<int>(mCurDepthSize));
            mDevice->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD *)&depthBias);
        }
    }
    else
    {
        mDevice->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
        mDevice->SetRenderState(D3DRS_DEPTHBIAS, 0);
    }

    mCurRasterizerState.polygonOffsetFill   = polygonOffsetFill;
    mCurRasterizerState.polygonOffsetFactor = polygonOffsetFactor;
    mCurRasterizerState.polygonOffsetUnits  = polygonOffsetUnits;
}

void StateManager9::setDepthMask(bool depthMask)
{
    mDevice->SetRenderState(D3DRS_ZWRITEENABLE, depthMask ? TRUE : FALSE);
    mCurDepthStencilState.depthMask = depthMask;
}

void StateManager9::setDepthTestAndFunc(bool depthTest, GLenum depthFunc)
{
    if (depthTest)
    {
        mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_TRUE);
        mDevice->SetRenderState(D3DRS_ZFUNC, gl_d3d9::ConvertComparison(depthFunc));
    }
    else
    {
        mDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
    }

    mCurDepthStencilState.depthTest = depthTest;
    mCurDepthStencilState.depthFunc = depthFunc;
}

void StateManager9::setStencilTestEnabled(bool stencilTest)
{

    if (stencilTest && mCurStencilSize > 0)
    {
        mDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
        mDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, TRUE);
    }
    else
    {
        mDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    }

    mCurDepthStencilState.stencilTest = stencilTest;
}

void StateManager9::setStencilFuncsFront(GLenum stencilFunc,
                                         GLuint stencilMask,
                                         int stencilRef,
                                         unsigned int maxStencil,
                                         bool frontFaceCCW)
{
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                            gl_d3d9::ConvertComparison(stencilFunc));
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                            (stencilRef < (int)maxStencil) ? stencilRef : maxStencil);
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK, stencilMask);

    mCurDepthStencilState.stencilFunc = stencilFunc;
    mCurDepthStencilState.stencilMask = stencilMask;
    mCurStencilRef                    = stencilRef;
}

void StateManager9::setStencilFuncsBack(GLenum stencilBackFunc,
                                        GLuint stencilBackMask,
                                        int stencilBackRef,
                                        unsigned int maxStencil,
                                        bool frontFaceCCW)
{
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFUNC : D3DRS_CCW_STENCILFUNC,
                            gl_d3d9::ConvertComparison(stencilBackFunc));
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILREF : D3DRS_CCW_STENCILREF,
                            (stencilBackRef < (int)maxStencil) ? stencilBackRef : maxStencil);
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILMASK : D3DRS_CCW_STENCILMASK,
                            stencilBackMask);

    mCurDepthStencilState.stencilBackFunc = stencilBackFunc;
    mCurDepthStencilState.stencilBackMask = stencilBackMask;
    mCurStencilBackRef                    = stencilBackRef;
}

void StateManager9::setStencilWriteMaskFront(GLuint stencilWritemask, bool frontFaceCCW)
{

    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                            stencilWritemask);

    mCurDepthStencilState.stencilWritemask = stencilWritemask;
}

void StateManager9::setStencilWriteMaskBack(GLuint stencilBackWritemask, bool frontFaceCCW)
{

    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILWRITEMASK : D3DRS_CCW_STENCILWRITEMASK,
                            stencilBackWritemask);

    mCurDepthStencilState.stencilBackWritemask = stencilBackWritemask;
}

void StateManager9::setStencilOpsFront(GLenum stencilFail,
                                       GLenum stencilPassDepthFail,
                                       GLenum stencilPassDepthPass,
                                       bool frontFaceCCW)
{
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                            gl_d3d9::ConvertStencilOp(stencilFail));
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                            gl_d3d9::ConvertStencilOp(stencilPassDepthFail));
    mDevice->SetRenderState(frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                            gl_d3d9::ConvertStencilOp(stencilPassDepthPass));

    mCurDepthStencilState.stencilFail          = stencilFail;
    mCurDepthStencilState.stencilPassDepthFail = stencilPassDepthFail;
    mCurDepthStencilState.stencilPassDepthPass = stencilPassDepthPass;
}

void StateManager9::setStencilOpsBack(GLenum stencilBackFail,
                                      GLenum stencilBackPassDepthFail,
                                      GLenum stencilBackPassDepthPass,
                                      bool frontFaceCCW)
{
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILFAIL : D3DRS_CCW_STENCILFAIL,
                            gl_d3d9::ConvertStencilOp(stencilBackFail));
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILZFAIL : D3DRS_CCW_STENCILZFAIL,
                            gl_d3d9::ConvertStencilOp(stencilBackPassDepthFail));
    mDevice->SetRenderState(!frontFaceCCW ? D3DRS_STENCILPASS : D3DRS_CCW_STENCILPASS,
                            gl_d3d9::ConvertStencilOp(stencilBackPassDepthPass));

    mCurDepthStencilState.stencilBackFail          = stencilBackFail;
    mCurDepthStencilState.stencilBackPassDepthFail = stencilBackPassDepthFail;
    mCurDepthStencilState.stencilBackPassDepthPass = stencilBackPassDepthPass;
}

}  // namespace rx
//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.cpp: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#include "libANGLE/renderer/d3d/d3d11/RenderStateCache.h"

#include <float.h>

#include "common/debug.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "third_party/murmurhash/MurmurHash3.h"

namespace rx
{
using namespace gl_d3d11;

namespace
{
template <typename T>
void TrimCache(unsigned int maxStates, unsigned int gcLimit, const char *name, T *cache)
{
    unsigned int kGarbageCollectionLimit = maxStates / 2 + gcLimit;

    if (cache->size() >= kGarbageCollectionLimit)
    {
        WARN() << "Overflowed the limit of " << (maxStates / 2) << " " << name
               << " states, removing the least recently used to make room.";
        cache->ShrinkToSize(maxStates / 2);
    }
}
}  // anonymous namespace

template <typename T>
std::size_t ComputeGenericHash(const T &key)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&key, sizeof(key), seed, &hash);
    return hash;
}

template std::size_t ComputeGenericHash(const rx::d3d11::BlendStateKey &);
template std::size_t ComputeGenericHash(const rx::d3d11::RasterizerStateKey &);
template std::size_t ComputeGenericHash(const gl::DepthStencilState &);
template std::size_t ComputeGenericHash(const gl::SamplerState &);

RenderStateCache::RenderStateCache()
    : mBlendStateCache(kMaxStates),
      mRasterizerStateCache(kMaxStates),
      mDepthStencilStateCache(kMaxStates),
      mSamplerStateCache(kMaxStates)
{
}

RenderStateCache::~RenderStateCache()
{
}

void RenderStateCache::clear()
{
    mBlendStateCache.Clear();
    mRasterizerStateCache.Clear();
    mDepthStencilStateCache.Clear();
    mSamplerStateCache.Clear();
}

// static
d3d11::BlendStateKey RenderStateCache::GetBlendStateKey(const gl::Framebuffer *framebuffer,
                                                        const gl::BlendState &blendState)
{
    d3d11::BlendStateKey key;
    const FramebufferD3D *framebufferD3D   = GetImplAs<FramebufferD3D>(framebuffer);
    const gl::AttachmentList &colorbuffers = framebufferD3D->getColorAttachmentsForRender();
    const UINT8 blendStateMask =
        gl_d3d11::ConvertColorMask(blendState.colorMaskRed, blendState.colorMaskGreen,
                                   blendState.colorMaskBlue, blendState.colorMaskAlpha);

    key.blendState = blendState;
    key.mrt        = false;

    for (size_t i = 0; i < colorbuffers.size(); i++)
    {
        const gl::FramebufferAttachment *attachment = colorbuffers[i];

        if (attachment)
        {
            if (i > 0)
            {
                key.mrt = true;
            }

            key.rtvMasks[i] =
                (gl_d3d11::GetColorMask(attachment->getFormat().info)) & blendStateMask;
        }
        else
        {
            key.rtvMasks[i] = 0;
        }
    }

    for (size_t i = colorbuffers.size(); i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        key.rtvMasks[i] = 0;
    }

    return key;
}

gl::Error RenderStateCache::getBlendState(Renderer11 *renderer,
                                          const d3d11::BlendStateKey &key,
                                          ID3D11BlendState **outBlendState)
{
    auto keyIter = mBlendStateCache.Get(key);
    if (keyIter != mBlendStateCache.end())
    {
        *outBlendState = keyIter->second.get();
        return gl::NoError();
    }

    TrimCache(kMaxStates, kGCLimit, "blend", &mBlendStateCache);

    // Create a new blend state and insert it into the cache
    D3D11_BLEND_DESC blendDesc;
    D3D11_RENDER_TARGET_BLEND_DESC &rtDesc0 = blendDesc.RenderTarget[0];
    const gl::BlendState &blendState        = key.blendState;

    blendDesc.AlphaToCoverageEnable  = blendState.sampleAlphaToCoverage;
    blendDesc.IndependentBlendEnable = key.mrt ? TRUE : FALSE;

    rtDesc0 = {};

    if (blendState.blend)
    {
        rtDesc0.BlendEnable    = true;
        rtDesc0.SrcBlend       = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendRGB, false);
        rtDesc0.DestBlend      = gl_d3d11::ConvertBlendFunc(blendState.destBlendRGB, false);
        rtDesc0.BlendOp        = gl_d3d11::ConvertBlendOp(blendState.blendEquationRGB);
        rtDesc0.SrcBlendAlpha  = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendAlpha, true);
        rtDesc0.DestBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.destBlendAlpha, true);
        rtDesc0.BlendOpAlpha   = gl_d3d11::ConvertBlendOp(blendState.blendEquationAlpha);
    }

    rtDesc0.RenderTargetWriteMask = key.rtvMasks[0];

    for (unsigned int i = 1; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
    {
        blendDesc.RenderTarget[i]                       = rtDesc0;
        blendDesc.RenderTarget[i].RenderTargetWriteMask = key.rtvMasks[i];
    }

    d3d11::BlendState d3dBlendState;
    ANGLE_TRY(renderer->allocateResource(blendDesc, &d3dBlendState));
    *outBlendState = d3dBlendState.get();
    mBlendStateCache.Put(key, std::move(d3dBlendState));

    return gl::NoError();
}

gl::Error RenderStateCache::getRasterizerState(Renderer11 *renderer,
                                               const gl::RasterizerState &rasterState,
                                               bool scissorEnabled,
                                               ID3D11RasterizerState **outRasterizerState)
{
    d3d11::RasterizerStateKey key;
    key.rasterizerState = rasterState;
    key.scissorEnabled = scissorEnabled;

    auto keyIter = mRasterizerStateCache.Get(key);
    if (keyIter != mRasterizerStateCache.end())
    {
        *outRasterizerState = keyIter->second.get();
        return gl::NoError();
    }

    TrimCache(kMaxStates, kGCLimit, "rasterizer", &mRasterizerStateCache);

    D3D11_CULL_MODE cullMode =
        gl_d3d11::ConvertCullMode(rasterState.cullFace, rasterState.cullMode);

    // Disable culling if drawing points
    if (rasterState.pointDrawMode)
    {
        cullMode = D3D11_CULL_NONE;
    }

    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode              = D3D11_FILL_SOLID;
    rasterDesc.CullMode              = cullMode;
    rasterDesc.FrontCounterClockwise = (rasterState.frontFace == GL_CCW) ? FALSE : TRUE;
    rasterDesc.DepthBiasClamp = 0.0f;  // MSDN documentation of DepthBiasClamp implies a value of
                                       // zero will preform no clamping, must be tested though.
    rasterDesc.DepthClipEnable       = TRUE;
    rasterDesc.ScissorEnable         = scissorEnabled ? TRUE : FALSE;
    rasterDesc.MultisampleEnable     = rasterState.multiSample;
    rasterDesc.AntialiasedLineEnable = FALSE;

    if (rasterState.polygonOffsetFill)
    {
        rasterDesc.SlopeScaledDepthBias = rasterState.polygonOffsetFactor;
        rasterDesc.DepthBias            = (INT)rasterState.polygonOffsetUnits;
    }
    else
    {
        rasterDesc.SlopeScaledDepthBias = 0.0f;
        rasterDesc.DepthBias            = 0;
    }

    d3d11::RasterizerState dx11RasterizerState;
    ANGLE_TRY(renderer->allocateResource(rasterDesc, &dx11RasterizerState));
    *outRasterizerState = dx11RasterizerState.get();
    mRasterizerStateCache.Put(key, std::move(dx11RasterizerState));

    return gl::NoError();
}

gl::Error RenderStateCache::getDepthStencilState(Renderer11 *renderer,
                                                 const gl::DepthStencilState &glState,
                                                 ID3D11DepthStencilState **outDSState)
{
    auto keyIter = mDepthStencilStateCache.Get(glState);
    if (keyIter != mDepthStencilStateCache.end())
    {
        *outDSState = keyIter->second.get();
        return gl::NoError();
    }

    TrimCache(kMaxStates, kGCLimit, "depth stencil", &mDepthStencilStateCache);

    D3D11_DEPTH_STENCIL_DESC dsDesc     = {0};
    dsDesc.DepthEnable                  = glState.depthTest ? TRUE : FALSE;
    dsDesc.DepthWriteMask               = ConvertDepthMask(glState.depthMask);
    dsDesc.DepthFunc                    = ConvertComparison(glState.depthFunc);
    dsDesc.StencilEnable                = glState.stencilTest ? TRUE : FALSE;
    dsDesc.StencilReadMask              = ConvertStencilMask(glState.stencilMask);
    dsDesc.StencilWriteMask             = ConvertStencilMask(glState.stencilWritemask);
    dsDesc.FrontFace.StencilFailOp      = ConvertStencilOp(glState.stencilFail);
    dsDesc.FrontFace.StencilDepthFailOp = ConvertStencilOp(glState.stencilPassDepthFail);
    dsDesc.FrontFace.StencilPassOp      = ConvertStencilOp(glState.stencilPassDepthPass);
    dsDesc.FrontFace.StencilFunc        = ConvertComparison(glState.stencilFunc);
    dsDesc.BackFace.StencilFailOp       = ConvertStencilOp(glState.stencilBackFail);
    dsDesc.BackFace.StencilDepthFailOp  = ConvertStencilOp(glState.stencilBackPassDepthFail);
    dsDesc.BackFace.StencilPassOp       = ConvertStencilOp(glState.stencilBackPassDepthPass);
    dsDesc.BackFace.StencilFunc         = ConvertComparison(glState.stencilBackFunc);

    d3d11::DepthStencilState dx11DepthStencilState;
    ANGLE_TRY(renderer->allocateResource(dsDesc, &dx11DepthStencilState));
    *outDSState = dx11DepthStencilState.get();
    mDepthStencilStateCache.Put(glState, std::move(dx11DepthStencilState));

    return gl::NoError();
}

gl::Error RenderStateCache::getSamplerState(Renderer11 *renderer,
                                            const gl::SamplerState &samplerState,
                                            ID3D11SamplerState **outSamplerState)
{
    auto keyIter = mSamplerStateCache.Get(samplerState);
    if (keyIter != mSamplerStateCache.end())
    {
        *outSamplerState = keyIter->second.get();
        return gl::NoError();
    }

    TrimCache(kMaxStates, kGCLimit, "sampler stencil", &mSamplerStateCache);

    const auto &featureLevel = renderer->getRenderer11DeviceCaps().featureLevel;

    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter =
        gl_d3d11::ConvertFilter(samplerState.minFilter, samplerState.magFilter,
                                samplerState.maxAnisotropy, samplerState.compareMode);
    samplerDesc.AddressU   = gl_d3d11::ConvertTextureWrap(samplerState.wrapS);
    samplerDesc.AddressV   = gl_d3d11::ConvertTextureWrap(samplerState.wrapT);
    samplerDesc.AddressW   = gl_d3d11::ConvertTextureWrap(samplerState.wrapR);
    samplerDesc.MipLODBias = 0;
    samplerDesc.MaxAnisotropy =
        gl_d3d11::ConvertMaxAnisotropy(samplerState.maxAnisotropy, featureLevel);
    samplerDesc.ComparisonFunc = gl_d3d11::ConvertComparison(samplerState.compareFunc);
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD         = samplerState.minLod;
    samplerDesc.MaxLOD         = samplerState.maxLod;

    if (featureLevel <= D3D_FEATURE_LEVEL_9_3)
    {
        // Check that maxLOD is nearly FLT_MAX (1000.0f is the default), since 9_3 doesn't support
        // anything other than FLT_MAX. Note that Feature Level 9_* only supports GL ES 2.0, so the
        // consumer of ANGLE can't modify the Max LOD themselves.
        ASSERT(samplerState.maxLod >= 999.9f);

        // Now just set MaxLOD to FLT_MAX. Other parts of the renderer (e.g. the non-zero max LOD
        // workaround) should take account of this.
        samplerDesc.MaxLOD = FLT_MAX;
    }

    d3d11::SamplerState dx11SamplerState;
    ANGLE_TRY(renderer->allocateResource(samplerDesc, &dx11SamplerState));
    *outSamplerState = dx11SamplerState.get();
    mSamplerStateCache.Put(samplerState, std::move(dx11SamplerState));

    return gl::NoError();
}

}  // namespace rx

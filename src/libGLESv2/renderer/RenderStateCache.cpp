//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.cpp: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#include "libGLESv2/renderer/RenderStateCache.h"
#include "libGLESv2/renderer/renderer11_utils.h"

#include "common/debug.h"
#include "third_party/murmurhash/MurmurHash3.h"

namespace rx
{

// MSDN's documentation of ID3D11Device::CreateBlendState, ID3D11Device::CreateRasterizerState
// and ID3D11Device::CreateDepthStencilState claims the maximum number of unique states of each
// type an application can create is 4096
const unsigned int RenderStateCache::kMaxBlendStates = 4096;
const unsigned int RenderStateCache::kMaxRasterizerStates = 4096;
const unsigned int RenderStateCache::kMaxDepthStencilStates = 4096;

RenderStateCache::RenderStateCache() : mDevice(NULL), mCounter(0),
                                       mBlendStateCache(kMaxBlendStates, hashBlendState, compareBlendStates),
                                       mRasterizerStateCache(kMaxRasterizerStates, hashRasterizerState, compareRasterizerStates),
                                       mDepthStencilStateCache(kMaxDepthStencilStates, hashDepthStencilState, compareDepthStencilStates)
{
}

RenderStateCache::~RenderStateCache()
{
    clear();
}

void RenderStateCache::initialize(ID3D11Device* device)
{
    clear();
    mDevice = device;
}

void RenderStateCache::clear()
{
    for (BlendStateMap::iterator i = mBlendStateCache.begin(); i != mBlendStateCache.end(); i++)
    {
        i->second.first->Release();
    }
    mBlendStateCache.clear();

    for (RasterizerStateMap::iterator i = mRasterizerStateCache.begin(); i != mRasterizerStateCache.end(); i++)
    {
        i->second.first->Release();
    }
    mRasterizerStateCache.clear();

    for (DepthStencilStateMap::iterator i = mDepthStencilStateCache.begin(); i != mDepthStencilStateCache.end(); i++)
    {
        i->second.first->Release();
    }
    mDepthStencilStateCache.clear();
}

std::size_t RenderStateCache::hashBlendState(const gl::BlendState &blendState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&blendState, sizeof(gl::BlendState), seed, &hash);
    return hash;
}

bool RenderStateCache::compareBlendStates(const gl::BlendState &a, const gl::BlendState &b)
{
    return memcmp(&a, &b, sizeof(gl::BlendState)) == 0;
}

ID3D11BlendState *RenderStateCache::getBlendState(const gl::BlendState &blendState)
{
    if (!mDevice)
    {
        ERR("RenderStateCache is not initialized.");
        return NULL;
    }

    BlendStateMap::iterator i = mBlendStateCache.find(blendState);
    if (i != mBlendStateCache.end())
    {
        BlendStateCounterPair &state = i->second;
        state.first->AddRef();
        state.second = mCounter++;
        return state.first;
    }
    else
    {
        if (mBlendStateCache.size() >= kMaxBlendStates)
        {
            TRACE("Overflowed the limit of %u blend states, removing the least recently used "
                  "to make room.", kMaxBlendStates);

            BlendStateMap::iterator leastRecentlyUsed = mBlendStateCache.begin();
            for (BlendStateMap::iterator i = mBlendStateCache.begin(); i != mBlendStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            leastRecentlyUsed->second.first->Release();
            mBlendStateCache.erase(leastRecentlyUsed);
        }

        // Create a new blend state and insert it into the cache
        D3D11_BLEND_DESC blendDesc = { 0 };
        blendDesc.AlphaToCoverageEnable = blendState.sampleAlphaToCoverage;
        blendDesc.IndependentBlendEnable = FALSE;

        for (unsigned int i = 0; i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
        {
            D3D11_RENDER_TARGET_BLEND_DESC &rtBlend = blendDesc.RenderTarget[i];

            rtBlend.BlendEnable = blendState.blend;
            if (blendState.blend)
            {
                rtBlend.SrcBlend = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendRGB);
                rtBlend.DestBlend = gl_d3d11::ConvertBlendFunc(blendState.destBlendRGB);
                rtBlend.BlendOp = gl_d3d11::ConvertBlendOp(blendState.blendEquationRGB);

                rtBlend.SrcBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.sourceBlendAlpha);
                rtBlend.DestBlendAlpha = gl_d3d11::ConvertBlendFunc(blendState.destBlendAlpha);
                rtBlend.BlendOpAlpha = gl_d3d11::ConvertBlendOp(blendState.blendEquationAlpha);
            }

            rtBlend.RenderTargetWriteMask = gl_d3d11::ConvertColorMask(blendState.colorMaskRed,
                                                                       blendState.colorMaskGreen,
                                                                       blendState.colorMaskBlue,
                                                                       blendState.colorMaskAlpha);
        }

        ID3D11BlendState* dx11BlendState = NULL;
        HRESULT result = mDevice->CreateBlendState(&blendDesc, &dx11BlendState);
        if (FAILED(result) || !dx11BlendState)
        {
            ERR("Unable to create a ID3D11BlendState, HRESULT: 0x%X.", result);
            return NULL;
        }

        mBlendStateCache.insert(std::make_pair(blendState, std::make_pair(dx11BlendState, mCounter++)));

        dx11BlendState->AddRef();
        return dx11BlendState;
    }
}

std::size_t RenderStateCache::hashRasterizerState(const RasterizerStateKey &rasterState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&rasterState, sizeof(RasterizerStateKey), seed, &hash);
    return hash;
}

bool RenderStateCache::compareRasterizerStates(const RasterizerStateKey &a, const RasterizerStateKey &b)
{
    return memcmp(&a, &b, sizeof(RasterizerStateKey)) == 0;
}

ID3D11RasterizerState *RenderStateCache::getRasterizerState(const gl::RasterizerState &rasterState,
                                                            unsigned int depthSize)
{
    if (!mDevice)
    {
        ERR("RenderStateCache is not initialized.");
        return NULL;
    }

    RasterizerStateKey key;
    key.rasterizerState = rasterState;
    key.depthSize = depthSize;

    RasterizerStateMap::iterator i = mRasterizerStateCache.find(key);
    if (i != mRasterizerStateCache.end())
    {
        RasterizerStateCounterPair &state = i->second;
        state.first->AddRef();
        state.second = mCounter++;
        return state.first;
    }
    else
    {
        if (mRasterizerStateCache.size() >= kMaxRasterizerStates)
        {
            TRACE("Overflowed the limit of %u rasterizer states, removing the least recently used "
                  "to make room.", kMaxRasterizerStates);

            RasterizerStateMap::iterator leastRecentlyUsed = mRasterizerStateCache.begin();
            for (RasterizerStateMap::iterator i = mRasterizerStateCache.begin(); i != mRasterizerStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            leastRecentlyUsed->second.first->Release();
            mRasterizerStateCache.erase(leastRecentlyUsed);
        }

        D3D11_RASTERIZER_DESC rasterDesc;
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = gl_d3d11::ConvertCullMode(rasterState.cullFace, rasterState.cullMode);
        rasterDesc.FrontCounterClockwise = (rasterState.frontFace == GL_CCW) ? TRUE : FALSE;
        rasterDesc.DepthBias = ldexp(rasterState.polygonOffsetUnits, -static_cast<int>(depthSize));
        rasterDesc.DepthBiasClamp = 0.0f; // MSDN documentation of DepthBiasClamp implies a value of zero will preform no clamping, must be tested though.
        rasterDesc.SlopeScaledDepthBias = rasterState.polygonOffsetUnits;
        rasterDesc.DepthClipEnable = TRUE;
        rasterDesc.ScissorEnable = rasterState.scissorTest ? TRUE : FALSE;
        rasterDesc.MultisampleEnable = TRUE;
        rasterDesc.AntialiasedLineEnable = FALSE;

        ID3D11RasterizerState* dx11RasterizerState = NULL;
        HRESULT result = mDevice->CreateRasterizerState(&rasterDesc, &dx11RasterizerState);
        if (FAILED(result) || !dx11RasterizerState)
        {
            ERR("Unable to create a ID3D11RasterizerState, HRESULT: 0x%X.", result);
            return NULL;
        }

        mRasterizerStateCache.insert(std::make_pair(key, std::make_pair(dx11RasterizerState, mCounter++)));

        dx11RasterizerState->AddRef();
        return dx11RasterizerState;
    }
}

std::size_t RenderStateCache::hashDepthStencilState(const gl::DepthStencilState &dsState)
{
    static const unsigned int seed = 0xABCDEF98;

    std::size_t hash = 0;
    MurmurHash3_x86_32(&dsState, sizeof(gl::DepthStencilState), seed, &hash);
    return hash;
}

bool RenderStateCache::compareDepthStencilStates(const gl::DepthStencilState &a, const gl::DepthStencilState &b)
{
    return memcmp(&a, &b, sizeof(gl::DepthStencilState)) == 0;
}

ID3D11DepthStencilState* RenderStateCache::getDepthStencilState(const gl::DepthStencilState &dsState)
{
    if (!mDevice)
    {
        ERR("RenderStateCache is not initialized.");
        return NULL;
    }

    DepthStencilStateMap::iterator i = mDepthStencilStateCache.find(dsState);
    if (i != mDepthStencilStateCache.end())
    {
        DepthStencilStateCounterPair &state = i->second;
        state.first->AddRef();
        state.second = mCounter++;
        return state.first;
    }
    else
    {
        if (mDepthStencilStateCache.size() >= kMaxDepthStencilStates)
        {
            TRACE("Overflowed the limit of %u depth stencil states, removing the least recently used "
                  "to make room.", kMaxDepthStencilStates);

            DepthStencilStateMap::iterator leastRecentlyUsed = mDepthStencilStateCache.begin();
            for (DepthStencilStateMap::iterator i = mDepthStencilStateCache.begin(); i != mDepthStencilStateCache.end(); i++)
            {
                if (i->second.second < leastRecentlyUsed->second.second)
                {
                    leastRecentlyUsed = i;
                }
            }
            leastRecentlyUsed->second.first->Release();
            mDepthStencilStateCache.erase(leastRecentlyUsed);
        }

        D3D11_DEPTH_STENCIL_DESC dsDesc = { 0 };
        dsDesc.DepthEnable = dsState.depthTest ? TRUE : FALSE;
        dsDesc.DepthWriteMask = gl_d3d11::ConvertDepthMask(dsState.depthMask);
        dsDesc.DepthFunc = gl_d3d11::ConvertComparison(dsState.depthFunc);
        dsDesc.StencilEnable = dsState.stencilTest ? TRUE : FALSE;
        dsDesc.StencilReadMask = gl_d3d11::ConvertStencilMask(dsState.stencilMask);
        dsDesc.StencilWriteMask = gl_d3d11::ConvertStencilMask(dsState.stencilWritemask);
        dsDesc.FrontFace.StencilFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilFail);
        dsDesc.FrontFace.StencilDepthFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilPassDepthFail);
        dsDesc.FrontFace.StencilPassOp = gl_d3d11::ConvertStencilOp(dsState.stencilPassDepthPass);
        dsDesc.FrontFace.StencilFunc = gl_d3d11::ConvertComparison(dsState.stencilFunc);
        dsDesc.BackFace.StencilFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackFail);
        dsDesc.BackFace.StencilDepthFailOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackPassDepthFail);
        dsDesc.BackFace.StencilPassOp = gl_d3d11::ConvertStencilOp(dsState.stencilBackPassDepthPass);
        dsDesc.BackFace.StencilFunc = gl_d3d11::ConvertComparison(dsState.stencilBackFunc);

        ID3D11DepthStencilState* dx11DepthStencilState = NULL;
        HRESULT result = mDevice->CreateDepthStencilState(&dsDesc, &dx11DepthStencilState);
        if (FAILED(result) || !dx11DepthStencilState)
        {
            ERR("Unable to create a ID3D11DepthStencilState, HRESULT: 0x%X.", result);
            return NULL;
        }

        mDepthStencilStateCache.insert(std::make_pair(dsState, std::make_pair(dx11DepthStencilState, mCounter++)));

        dx11DepthStencilState->AddRef();
        return dx11DepthStencilState;
    }
}

}
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

RenderStateCache::RenderStateCache() : mDevice(NULL), mCounter(0),
                                       mBlendStateCache(kMaxBlendStates, hashBlendState, compareBlendStates)
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

// MSDN's documentation of ID3D11Device::CreateBlendState claims the maximum number of
// unique blend states an application can create is 4096
const unsigned int RenderStateCache::kMaxBlendStates = 4096;

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

                rtBlend.RenderTargetWriteMask = gl_d3d11::ConvertColorMask(blendState.colorMaskRed,
                                                                           blendState.colorMaskGreen,
                                                                           blendState.colorMaskBlue,
                                                                           blendState.colorMaskAlpha);
            }
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

}
//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// RenderStateCache.h: Defines rx::RenderStateCache, a cache of Direct3D render
// state objects.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_RENDERSTATECACHE_H_
#define LIBANGLE_RENDERER_D3D_D3D11_RENDERSTATECACHE_H_

#include "libANGLE/angletypes.h"
#include "libANGLE/Error.h"
#include "common/angleutils.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"

#include <unordered_map>

namespace gl
{
class Framebuffer;
}

namespace rx
{
class Renderer11;

class RenderStateCache : angle::NonCopyable
{
  public:
    RenderStateCache(Renderer11 *renderer);
    virtual ~RenderStateCache();

    void clear();

    static d3d11::BlendStateKey GetBlendStateKey(const gl::Framebuffer *framebuffer,
                                                 const gl::BlendState &blendState);
    gl::Error getBlendState(const d3d11::BlendStateKey &key, ID3D11BlendState **outBlendState);
    gl::Error getRasterizerState(const gl::RasterizerState &rasterState, bool scissorEnabled, ID3D11RasterizerState **outRasterizerState);
    gl::Error getDepthStencilState(const gl::DepthStencilState &dsState,
                                   ID3D11DepthStencilState **outDSState);
    gl::Error getSamplerState(const gl::SamplerState &samplerState, ID3D11SamplerState **outSamplerState);

  private:
    Renderer11 *mRenderer;
    unsigned long long mCounter;

    // Blend state cache
    static std::size_t HashBlendState(const d3d11::BlendStateKey &blendState);
    static const unsigned int kMaxBlendStates;

    typedef std::size_t (*BlendStateHashFunction)(const d3d11::BlendStateKey &);
    typedef std::pair<d3d11::BlendState, unsigned long long> BlendStateCounterPair;
    typedef std::unordered_map<d3d11::BlendStateKey, BlendStateCounterPair, BlendStateHashFunction>
        BlendStateMap;
    BlendStateMap mBlendStateCache;

    // Rasterizer state cache
    static std::size_t HashRasterizerState(const d3d11::RasterizerStateKey &rasterState);
    static const unsigned int kMaxRasterizerStates;

    typedef std::size_t (*RasterizerStateHashFunction)(const d3d11::RasterizerStateKey &);
    typedef std::pair<d3d11::RasterizerState, unsigned long long> RasterizerStateCounterPair;
    typedef std::unordered_map<d3d11::RasterizerStateKey,
                               RasterizerStateCounterPair,
                               RasterizerStateHashFunction>
        RasterizerStateMap;
    RasterizerStateMap mRasterizerStateCache;

    // Depth stencil state cache
    static std::size_t HashDepthStencilState(const gl::DepthStencilState &dsState);
    static const unsigned int kMaxDepthStencilStates;

    typedef std::size_t (*DepthStencilStateHashFunction)(const gl::DepthStencilState &);
    typedef std::pair<d3d11::DepthStencilState, unsigned long long> DepthStencilStateCounterPair;
    typedef std::unordered_map<gl::DepthStencilState,
                               DepthStencilStateCounterPair,
                               DepthStencilStateHashFunction>
        DepthStencilStateMap;
    DepthStencilStateMap mDepthStencilStateCache;

    // Sample state cache
    static std::size_t HashSamplerState(const gl::SamplerState &samplerState);
    static const unsigned int kMaxSamplerStates;

    typedef std::size_t (*SamplerStateHashFunction)(const gl::SamplerState &);
    typedef std::pair<d3d11::SamplerState, unsigned long long> SamplerStateCounterPair;
    typedef std::unordered_map<gl::SamplerState, SamplerStateCounterPair, SamplerStateHashFunction>
        SamplerStateMap;
    SamplerStateMap mSamplerStateCache;
};

}  // namespace rx

#endif // LIBANGLE_RENDERER_D3D_D3D11_RENDERSTATECACHE_H_

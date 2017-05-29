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
template <typename T>
std::size_t ComputeGenericHash(const T &key);
}  // namespace rx

namespace std
{
template <>
struct hash<rx::d3d11::BlendStateKey>
{
    size_t operator()(const rx::d3d11::BlendStateKey &key) const
    {
        return rx::ComputeGenericHash(key);
    }
};

template <>
struct hash<rx::d3d11::RasterizerStateKey>
{
    size_t operator()(const rx::d3d11::RasterizerStateKey &key) const
    {
        return rx::ComputeGenericHash(key);
    }
};

template <>
struct hash<gl::DepthStencilState>
{
    size_t operator()(const gl::DepthStencilState &key) const
    {
        return rx::ComputeGenericHash(key);
    }
};

template <>
struct hash<gl::SamplerState>
{
    size_t operator()(const gl::SamplerState &key) const { return rx::ComputeGenericHash(key); }
};
}  // namespace std

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
    static const unsigned int kMaxBlendStates;

    typedef std::pair<d3d11::BlendState, unsigned long long> BlendStateCounterPair;
    typedef std::unordered_map<d3d11::BlendStateKey, BlendStateCounterPair> BlendStateMap;
    BlendStateMap mBlendStateCache;

    // Rasterizer state cache
    static const unsigned int kMaxRasterizerStates;

    typedef std::pair<d3d11::RasterizerState, unsigned long long> RasterizerStateCounterPair;
    typedef std::unordered_map<d3d11::RasterizerStateKey, RasterizerStateCounterPair>
        RasterizerStateMap;
    RasterizerStateMap mRasterizerStateCache;

    // Depth stencil state cache
    static const unsigned int kMaxDepthStencilStates;

    typedef std::pair<d3d11::DepthStencilState, unsigned long long> DepthStencilStateCounterPair;
    typedef std::unordered_map<gl::DepthStencilState, DepthStencilStateCounterPair>
        DepthStencilStateMap;
    DepthStencilStateMap mDepthStencilStateCache;

    // Sample state cache
    static const unsigned int kMaxSamplerStates;

    typedef std::pair<d3d11::SamplerState, unsigned long long> SamplerStateCounterPair;
    typedef std::unordered_map<gl::SamplerState, SamplerStateCounterPair> SamplerStateMap;
    SamplerStateMap mSamplerStateCache;
};

}  // namespace rx

#endif // LIBANGLE_RENDERER_D3D_D3D11_RENDERSTATECACHE_H_

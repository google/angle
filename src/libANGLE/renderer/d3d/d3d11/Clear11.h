//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Clear11.h: Framebuffer clear utility class.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_CLEAR11_H_
#define LIBANGLE_RENDERER_D3D_D3D11_CLEAR11_H_

#include <map>
#include <vector>

#include "libANGLE/angletypes.h"
#include "libANGLE/Error.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"

namespace rx
{
class Renderer11;
class RenderTarget11;
struct ClearParameters;

struct MaskedRenderTarget
{
    // Corrected alpha clear value
    float alphaOverride;
    // RenderTarget info
    RenderTarget11 *renderTarget;
};

class Clear11 : angle::NonCopyable
{
  public:
    explicit Clear11(Renderer11 *renderer);
    ~Clear11();

    // Clears the framebuffer with the supplied clear parameters, assumes that the framebuffer is currently applied.
    gl::Error clearFramebuffer(const ClearParameters &clearParams,
                               const gl::FramebufferState &fboData);

  private:

    ID3D11DepthStencilState *getDepthStencilState(const ClearParameters &clearParams);

    struct ClearShader final : public angle::NonCopyable
    {
        ClearShader(const BYTE *vsByteCode,
                    size_t vsSize,
                    const char *vsDebugName,
                    const BYTE *psByteCode,
                    size_t psSize,
                    const char *psDebugName);
        ClearShader(const D3D11_INPUT_ELEMENT_DESC *ilDesc,
                    size_t ilSize,
                    const char *ilDebugName,
                    const BYTE *vsByteCode,
                    size_t vsSize,
                    const char *vsDebugName,
                    const BYTE *psByteCode,
                    size_t psSize,
                    const char *psDebugName);
        ~ClearShader();

        d3d11::LazyInputLayout inputLayout;
        d3d11::LazyShader<ID3D11VertexShader> vertexShader;
        d3d11::LazyShader<ID3D11PixelShader> pixelShader;
    };

    template <unsigned int vsSize, unsigned int psSize>
    static ClearShader CreateClearShader(ID3D11Device *device, DXGI_FORMAT colorType, const BYTE(&vsByteCode)[vsSize], const BYTE(&psByteCode)[psSize]);

    struct ClearDepthStencilInfo
    {
        bool clearDepth;
        bool clearStencil;
        UINT8 stencilWriteMask;
    };
    typedef bool(*ClearDepthStencilInfoComparisonFunction)(const ClearDepthStencilInfo&, const ClearDepthStencilInfo &);
    typedef std::map<ClearDepthStencilInfo, ID3D11DepthStencilState*, ClearDepthStencilInfoComparisonFunction> ClearDepthStencilStateMap;

    Renderer11 *mRenderer;

    ClearShader *mFloatClearShader;
    ClearShader *mUintClearShader;
    ClearShader *mIntClearShader;

    ClearDepthStencilStateMap mClearDepthStencilStates;

    angle::ComPtr<ID3D11Buffer> mVertexBuffer;
    angle::ComPtr<ID3D11Buffer> mColorAndDepthDataBuffer;
    angle::ComPtr<ID3D11RasterizerState> mScissorEnabledRasterizerState;
    angle::ComPtr<ID3D11RasterizerState> mScissorDisabledRasterizerState;
    angle::ComPtr<ID3D11BlendState> mBlendState;
};

}

#endif // LIBANGLE_RENDERER_D3D_D3D11_CLEAR11_H_

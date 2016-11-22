//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Clear11.cpp: Framebuffer clear utility class.

#include "libANGLE/renderer/d3d/d3d11/Clear11.h"

#include <algorithm>

#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"
#include "libANGLE/renderer/d3d/d3d11/RenderTarget11.h"
#include "libANGLE/renderer/d3d/d3d11/formatutils11.h"
#include "third_party/trace_event/trace_event.h"

// Precompiled shaders
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearanytype11vs.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearfloat11_fl9ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearfloat11ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearuint11ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearsint11ps.h"

namespace rx
{
namespace
{

template <typename T>
struct RtvDsvClearInfo
{
    gl::Color<T> clearColor;
    float z;
    float alphas1to7[7];
};

// Helper function to fill RtvDsvClearInfo with a single color
template <typename T>
void ApplyColorAndDepthData(const gl::Color<T> &color, const float depthValue, void *buffer)
{
    static_assert((sizeof(RtvDsvClearInfo<float>) == sizeof(RtvDsvClearInfo<int>)),
                  "Size of rx::RtvDsvClearInfo<float> is not equal to rx::RtvDsvClearInfo<int>");
    RtvDsvClearInfo<T> *rtvDsvData = reinterpret_cast<RtvDsvClearInfo<T> *>(buffer);

    rtvDsvData->clearColor.red   = color.red;
    rtvDsvData->clearColor.green = color.green;
    rtvDsvData->clearColor.blue  = color.blue;
    rtvDsvData->clearColor.alpha = color.alpha;
    rtvDsvData->z                = gl::clamp01(depthValue);
}

// Helper function to fill RtvDsvClearInfo with more than one alpha
void ApplyAdjustedColorAndDepthData(const gl::Color<float> &color,
                                    const std::vector<MaskedRenderTarget> &renderTargets,
                                    const float depthValue,
                                    void *buffer)
{
    static_assert((sizeof(float) * 5 == (offsetof(RtvDsvClearInfo<float>, alphas1to7))),
                  "Unexpected padding in rx::RtvDsvClearInfo between z and alphas1to7 elements");
    static_assert((sizeof(RtvDsvClearInfo<float>) ==
                   (offsetof(RtvDsvClearInfo<float>, alphas1to7) + sizeof(float) * 7)),
                  "Unexpected padding in rx::RtvDsvClearInfo after alphas1to7 element");

    const unsigned int numRtvs = static_cast<unsigned int>(renderTargets.size());

    RtvDsvClearInfo<float> *rtvDsvData = reinterpret_cast<RtvDsvClearInfo<float> *>(buffer);

    rtvDsvData->z = gl::clamp01(depthValue);

    if (numRtvs > 0)
    {
        rtvDsvData->clearColor.red   = color.red;
        rtvDsvData->clearColor.green = color.green;
        rtvDsvData->clearColor.blue  = color.blue;
        rtvDsvData->clearColor.alpha = renderTargets[0].alphaOverride;

        for (unsigned int i = 1; i < numRtvs; i++)
        {
            rtvDsvData->alphas1to7[i] = renderTargets[i].alphaOverride;
        }
    }
}

// Clamps and rounds alpha clear values to either 0.0f or 1.0f
float AdjustAlphaFor1BitOutput(float alpha)
{
    // Some drivers do not correctly handle calling Clear() on formats with a
    // 1-bit alpha component. They can incorrectly round all non-zero values
    // up to 1.0f. Note that WARP does not do this. We should handle the
    // rounding for them instead.
    return (alpha >= 0.5f ? 1.0f : 0.0f);
}

}  // anonymous namespace

Clear11::ClearShader::ClearShader(const BYTE *vsByteCode,
                                  size_t vsSize,
                                  const char *vsDebugName,
                                  const BYTE *psByteCode,
                                  size_t psSize,
                                  const char *psDebugName)
    : inputLayout(nullptr, 0, nullptr, 0, nullptr),
      vertexShader(vsByteCode, vsSize, vsDebugName),
      pixelShader(psByteCode, psSize, psDebugName)
{
}

Clear11::ClearShader::ClearShader(const D3D11_INPUT_ELEMENT_DESC *ilDesc,
                                  size_t ilSize,
                                  const char *ilDebugName,
                                  const BYTE *vsByteCode,
                                  size_t vsSize,
                                  const char *vsDebugName,
                                  const BYTE *psByteCode,
                                  size_t psSize,
                                  const char *psDebugName)
    : inputLayout(ilDesc, ilSize, vsByteCode, vsSize, ilDebugName),
      vertexShader(vsByteCode, vsSize, vsDebugName),
      pixelShader(psByteCode, psSize, psDebugName)
{
}

Clear11::ClearShader::~ClearShader()
{
    inputLayout.release();
    vertexShader.release();
    pixelShader.release();
}

Clear11::Clear11(Renderer11 *renderer)
    : mRenderer(renderer),
      mFloatClearShader(nullptr),
      mUintClearShader(nullptr),
      mIntClearShader(nullptr),
      mClearDepthStencilStates(StructLessThan<ClearDepthStencilInfo>),
      mVertexBuffer(nullptr),
      mColorAndDepthDataBuffer(nullptr),
      mScissorEnabledRasterizerState(nullptr),
      mScissorDisabledRasterizerState(nullptr),
      mBlendState(nullptr)
{
    TRACE_EVENT0("gpu.angle", "Clear11::Clear11");

    HRESULT result;
    ID3D11Device *device = renderer->getDevice();

    D3D11_RASTERIZER_DESC rsDesc;
    rsDesc.FillMode              = D3D11_FILL_SOLID;
    rsDesc.CullMode              = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = FALSE;
    rsDesc.DepthBias             = 0;
    rsDesc.DepthBiasClamp        = 0.0f;
    rsDesc.SlopeScaledDepthBias  = 0.0f;
    rsDesc.DepthClipEnable       = TRUE;
    rsDesc.ScissorEnable         = FALSE;
    rsDesc.MultisampleEnable     = FALSE;
    rsDesc.AntialiasedLineEnable = FALSE;

    result = device->CreateRasterizerState(&rsDesc, mScissorDisabledRasterizerState.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mScissorDisabledRasterizerState,
                        "Clear11 masked clear rasterizer without scissor state");

    rsDesc.ScissorEnable = TRUE;
    result = device->CreateRasterizerState(&rsDesc, mScissorEnabledRasterizerState.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mScissorEnabledRasterizerState,
                        "Clear11 masked clear rasterizer with scissor state");

    D3D11_BLEND_DESC blendDesc                      = {0};
    blendDesc.AlphaToCoverageEnable                 = FALSE;
    blendDesc.IndependentBlendEnable                = FALSE;
    blendDesc.RenderTarget[0].BlendEnable           = TRUE;
    blendDesc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlend              = D3D11_BLEND_BLEND_FACTOR;
    blendDesc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_BLEND_FACTOR;
    blendDesc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_BLEND_FACTOR;
    blendDesc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_BLEND_FACTOR;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    result = device->CreateBlendState(&blendDesc, mBlendState.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mBlendState, "Clear11 masked clear universal blendState");

    // Create constant buffer for color & depth data
    const UINT colorAndDepthDataSize = rx::roundUp<UINT>(sizeof(RtvDsvClearInfo<float>), 16);

    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth           = colorAndDepthDataSize;
    bufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags           = 0;
    bufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&bufferDesc, nullptr, mColorAndDepthDataBuffer.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mColorAndDepthDataBuffer, "Clear11 masked clear constant buffer");

    // Create vertex buffer with clip co-ordinates for a quad that covers the entire surface
    const d3d11::PositionVertex vbData[4] = {{-1.0f, 1.0f, 0.0f, 0.0f},
                                             {-1.0f, -1.0f, 0.0f, 0.0f},
                                             {1.0f, 1.0f, 0.0f, 0.0f},
                                             {1.0f, -1.0f, 0.0f, 0.0f}};
    const UINT vbSize = sizeof(vbData);
    ASSERT((vbSize % 16) == 0);

    D3D11_SUBRESOURCE_DATA initialData;
    initialData.pSysMem          = vbData;
    initialData.SysMemPitch      = vbSize;
    initialData.SysMemSlicePitch = initialData.SysMemPitch;

    bufferDesc.ByteWidth           = vbSize;
    bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
    bufferDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
    bufferDesc.CPUAccessFlags      = 0;
    bufferDesc.MiscFlags           = 0;
    bufferDesc.StructureByteStride = 0;

    result = device->CreateBuffer(&bufferDesc, &initialData, mVertexBuffer.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mVertexBuffer, "Clear11 masked clear vertex buffer");

    const D3D11_INPUT_ELEMENT_DESC ilDesc = {
        "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};
    const size_t ilDescArraySize = 1;

    // TODO (Shahmeer Esmail): As a potential performance optimizations, evaluate use of a single
    // color floatClear shader that can be used where only one RT needs to be cleared or alpha
    // correction isn't required.
    if (mRenderer->getRenderer11DeviceCaps().featureLevel <= D3D_FEATURE_LEVEL_9_3)
    {
        mFloatClearShader =
            new ClearShader(&ilDesc, ilDescArraySize, "Clear11 IL", g_VS_ClearAnyType,
                            ArraySize(g_VS_ClearAnyType), "Clear11 VS", g_PS_ClearFloat_FL9,
                            ArraySize(g_PS_ClearFloat_FL9), "Clear11 Float PS FL93");
    }
    else
    {
        mFloatClearShader = new ClearShader(
            &ilDesc, ilDescArraySize, "Clear11 IL", g_VS_ClearAnyType, ArraySize(g_VS_ClearAnyType),
            "Clear11 VS", g_PS_ClearFloat, ArraySize(g_PS_ClearFloat), "Clear11 Float PS");
    }

    if (renderer->isES3Capable())
    {
        mUintClearShader = new ClearShader(
            &ilDesc, ilDescArraySize, "Clear11 IL", g_VS_ClearAnyType, ArraySize(g_VS_ClearAnyType),
            "Clear11 VS", g_PS_ClearUint, ArraySize(g_PS_ClearUint), "Clear11 UINT PS");
        mIntClearShader = new ClearShader(
            &ilDesc, ilDescArraySize, "Clear11 IL", g_VS_ClearAnyType, ArraySize(g_VS_ClearAnyType),
            "Clear11 VS", g_PS_ClearSint, ArraySize(g_PS_ClearSint), "Clear11 SINT PS");
    }
}

Clear11::~Clear11()
{
    SafeDelete(mFloatClearShader);
    SafeDelete(mUintClearShader);
    SafeDelete(mIntClearShader);

    for (ClearDepthStencilStateMap::iterator i = mClearDepthStencilStates.begin();
         i != mClearDepthStencilStates.end(); i++)
    {
        SafeRelease(i->second);
    }
    mClearDepthStencilStates.clear();
}

gl::Error Clear11::clearFramebuffer(const ClearParameters &clearParams,
                                    const gl::FramebufferState &fboData)
{
    const auto &colorAttachments  = fboData.getColorAttachments();
    const auto &drawBufferStates  = fboData.getDrawBufferStates();
    const auto *depthAttachment   = fboData.getDepthAttachment();
    const auto *stencilAttachment = fboData.getStencilAttachment();

    ASSERT(colorAttachments.size() == drawBufferStates.size());

    // Iterate over the color buffers which require clearing and determine if they can be
    // cleared with ID3D11DeviceContext::ClearRenderTargetView or ID3D11DeviceContext1::ClearView.
    // This requires:
    // 1) The render target is being cleared to a float value (will be cast to integer when clearing
    // integer
    //    render targets as expected but does not work the other way around)
    // 2) The format of the render target has no color channels that are currently masked out.
    // Clear the easy-to-clear buffers on the spot and accumulate the ones that require special
    // work.
    //
    // If these conditions are met, and:
    // - No scissored clear is needed, then clear using ID3D11DeviceContext::ClearRenderTargetView.
    // - A scissored clear is needed then clear using ID3D11DeviceContext1::ClearView if available.
    //   Otherwise draw a quad.
    //
    // Also determine if the depth stencil can be cleared with
    // ID3D11DeviceContext::ClearDepthStencilView
    // by checking if the stencil write mask covers the entire stencil.
    //
    // To clear the remaining buffers, quads must be drawn containing an int, uint or float vertex
    // color
    // attribute.

    gl::Extents framebufferSize;

    const gl::FramebufferAttachment *colorAttachment = fboData.getFirstColorAttachment();
    if (colorAttachment != nullptr)
    {
        framebufferSize = colorAttachment->getSize();
    }
    else if (depthAttachment != nullptr)
    {
        framebufferSize = depthAttachment->getSize();
    }
    else if (stencilAttachment != nullptr)
    {
        framebufferSize = stencilAttachment->getSize();
    }
    else
    {
        UNREACHABLE();
        return gl::Error(GL_INVALID_OPERATION);
    }

    if (clearParams.scissorEnabled && (clearParams.scissor.x >= framebufferSize.width ||
                                       clearParams.scissor.y >= framebufferSize.height ||
                                       clearParams.scissor.x + clearParams.scissor.width <= 0 ||
                                       clearParams.scissor.y + clearParams.scissor.height <= 0))
    {
        // Scissor is enabled and the scissor rectangle is outside the renderbuffer
        return gl::NoError();
    }

    bool needScissoredClear =
        clearParams.scissorEnabled &&
        (clearParams.scissor.x > 0 || clearParams.scissor.y > 0 ||
         clearParams.scissor.x + clearParams.scissor.width < framebufferSize.width ||
         clearParams.scissor.y + clearParams.scissor.height < framebufferSize.height);

    std::vector<MaskedRenderTarget> maskedClearRenderTargets;
    RenderTarget11 *maskedClearDepthStencil = nullptr;

    ID3D11DeviceContext *deviceContext   = mRenderer->getDeviceContext();
    ID3D11DeviceContext1 *deviceContext1 = mRenderer->getDeviceContext1IfSupported();
    ID3D11Device *device                 = mRenderer->getDevice();

    for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < colorAttachments.size();
         colorAttachmentIndex++)
    {
        const gl::FramebufferAttachment &attachment = colorAttachments[colorAttachmentIndex];

        if (clearParams.clearColor[colorAttachmentIndex] && attachment.isAttached() &&
            drawBufferStates[colorAttachmentIndex] != GL_NONE)
        {
            RenderTarget11 *renderTarget = nullptr;
            ANGLE_TRY(attachment.getRenderTarget(&renderTarget));

            const gl::InternalFormat &formatInfo = *attachment.getFormat().info;
            const auto &nativeFormat             = renderTarget->getFormatSet().format();

            if (clearParams.colorClearType == GL_FLOAT &&
                !(formatInfo.componentType == GL_FLOAT ||
                  formatInfo.componentType == GL_UNSIGNED_NORMALIZED ||
                  formatInfo.componentType == GL_SIGNED_NORMALIZED))
            {
                ERR() << "It is undefined behaviour to clear a render buffer which is not "
                         "normalized fixed point or floating-point to floating point values (color "
                         "attachment "
                      << colorAttachmentIndex << " has internal format " << attachment.getFormat()
                      << ").";
            }

            if ((formatInfo.redBits == 0 || !clearParams.colorMaskRed) &&
                (formatInfo.greenBits == 0 || !clearParams.colorMaskGreen) &&
                (formatInfo.blueBits == 0 || !clearParams.colorMaskBlue) &&
                (formatInfo.alphaBits == 0 || !clearParams.colorMaskAlpha))
            {
                // Every channel either does not exist in the render target or is masked out
                continue;
            }
            else if ((!(mRenderer->getRenderer11DeviceCaps().supportsClearView) &&
                      needScissoredClear) ||
                     clearParams.colorClearType != GL_FLOAT ||
                     (formatInfo.redBits > 0 && !clearParams.colorMaskRed) ||
                     (formatInfo.greenBits > 0 && !clearParams.colorMaskGreen) ||
                     (formatInfo.blueBits > 0 && !clearParams.colorMaskBlue) ||
                     (formatInfo.alphaBits > 0 && !clearParams.colorMaskAlpha))
            {
                // A masked clear is required, or a scissored clear is required and
                // ID3D11DeviceContext1::ClearView is unavailable
                MaskedRenderTarget rtData;

                if (clearParams.colorClearType == GL_FLOAT)
                {
                    if ((formatInfo.alphaBits == 0 && nativeFormat.alphaBits > 0))
                    {
                        rtData.alphaOverride = 1.0f;
                    }
                    else if (formatInfo.alphaBits == 1)
                    {
                        rtData.alphaOverride =
                            AdjustAlphaFor1BitOutput(clearParams.colorFClearValue.alpha);
                    }
                    else
                    {
                        rtData.alphaOverride = clearParams.colorFClearValue.alpha;
                    }
                }

                rtData.renderTarget = renderTarget;

                maskedClearRenderTargets.push_back(rtData);
            }
            else
            {
                // ID3D11DeviceContext::ClearRenderTargetView or ID3D11DeviceContext1::ClearView is
                // possible

                ID3D11RenderTargetView *framebufferRTV = renderTarget->getRenderTargetView();
                if (!framebufferRTV)
                {
                    return gl::Error(GL_OUT_OF_MEMORY,
                                     "Internal render target view pointer unexpectedly null.");
                }

                // Check if the actual format has a channel that the internal format does not and
                // set them to the default values
                float clearValues[4] = {
                    ((formatInfo.redBits == 0 && nativeFormat.redBits > 0)
                         ? 0.0f
                         : clearParams.colorFClearValue.red),
                    ((formatInfo.greenBits == 0 && nativeFormat.greenBits > 0)
                         ? 0.0f
                         : clearParams.colorFClearValue.green),
                    ((formatInfo.blueBits == 0 && nativeFormat.blueBits > 0)
                         ? 0.0f
                         : clearParams.colorFClearValue.blue),
                    ((formatInfo.alphaBits == 0 && nativeFormat.alphaBits > 0)
                         ? 1.0f
                         : clearParams.colorFClearValue.alpha),
                };

                if (formatInfo.alphaBits == 1)
                {
                    clearValues[3] = AdjustAlphaFor1BitOutput(clearParams.colorFClearValue.alpha);
                }

                if (needScissoredClear)
                {
                    // We shouldn't reach here if deviceContext1 is unavailable.
                    ASSERT(deviceContext1);

                    D3D11_RECT rect;
                    rect.left   = clearParams.scissor.x;
                    rect.right  = clearParams.scissor.x + clearParams.scissor.width;
                    rect.top    = clearParams.scissor.y;
                    rect.bottom = clearParams.scissor.y + clearParams.scissor.height;

                    deviceContext1->ClearView(framebufferRTV, clearValues, &rect, 1);
                    if (mRenderer->getWorkarounds().callClearTwiceOnSmallTarget)
                    {
                        if (clearParams.scissor.width <= 16 || clearParams.scissor.height <= 16)
                        {
                            deviceContext1->ClearView(framebufferRTV, clearValues, &rect, 1);
                        }
                    }
                }
                else
                {
                    deviceContext->ClearRenderTargetView(framebufferRTV, clearValues);
                    if (mRenderer->getWorkarounds().callClearTwiceOnSmallTarget)
                    {
                        if (framebufferSize.width <= 16 || framebufferSize.height <= 16)
                        {
                            deviceContext->ClearRenderTargetView(framebufferRTV, clearValues);
                        }
                    }
                }
            }
        }
    }

    if (clearParams.clearDepth || clearParams.clearStencil)
    {
        const gl::FramebufferAttachment *attachment =
            (depthAttachment != nullptr) ? depthAttachment : stencilAttachment;
        ASSERT(attachment != nullptr);

        RenderTarget11 *renderTarget = nullptr;
        ANGLE_TRY(attachment->getRenderTarget(&renderTarget));

        const auto &nativeFormat = renderTarget->getFormatSet().format();

        unsigned int stencilUnmasked =
            (stencilAttachment != nullptr) ? (1 << nativeFormat.stencilBits) - 1 : 0;
        bool needMaskedStencilClear =
            clearParams.clearStencil &&
            (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

        if (needScissoredClear || needMaskedStencilClear)
        {
            maskedClearDepthStencil = renderTarget;
        }
        else
        {
            ID3D11DepthStencilView *framebufferDSV = renderTarget->getDepthStencilView();
            if (!framebufferDSV)
            {
                return gl::Error(GL_OUT_OF_MEMORY,
                                 "Internal depth stencil view pointer unexpectedly null.");
            }

            UINT clearFlags = (clearParams.clearDepth ? D3D11_CLEAR_DEPTH : 0) |
                              (clearParams.clearStencil ? D3D11_CLEAR_STENCIL : 0);
            FLOAT depthClear   = gl::clamp01(clearParams.depthClearValue);
            UINT8 stencilClear = clearParams.stencilClearValue & 0xFF;

            deviceContext->ClearDepthStencilView(framebufferDSV, clearFlags, depthClear,
                                                 stencilClear);
        }
    }

    if (maskedClearRenderTargets.empty() && !maskedClearDepthStencil)
    {
        return gl::NoError();
    }

    // To clear the render targets and depth stencil in one pass:
    //
    // Render a quad clipped to the scissor rectangle which draws the clear color and a blend
    // state that will perform the required color masking.
    //
    // The quad's depth is equal to the depth clear value with a depth stencil state that
    // will enable or disable depth test/writes if the depth buffer should be cleared or not.
    //
    // The rasterizer state's stencil is set to always pass or fail based on if the stencil
    // should be cleared or not with a stencil write mask of the stencil clear value.
    //
    // ======================================================================================
    //
    // Luckily, the gl spec (ES 3.0.2 pg 183) states that the results of clearing a render-
    // buffer that is not normalized fixed point or floating point with floating point values
    // are undefined so we can just write floats to them and D3D11 will bit cast them to
    // integers.
    //
    // Also, we don't have to worry about attempting to clear a normalized fixed/floating point
    // buffer with integer values because there is no gl API call which would allow it,
    // glClearBuffer* calls only clear a single renderbuffer at a time which is verified to
    // be a compatible clear type.

    // Bind all the render targets which need clearing
    ASSERT(maskedClearRenderTargets.size() <= mRenderer->getNativeCaps().maxDrawBuffers);
    std::vector<ID3D11RenderTargetView *> rtvs(maskedClearRenderTargets.size());
    for (unsigned int i = 0; i < maskedClearRenderTargets.size(); i++)
    {
        RenderTarget11 *renderTarget = maskedClearRenderTargets[i].renderTarget;
        ID3D11RenderTargetView *rtv  = renderTarget->getRenderTargetView();
        if (!rtv)
        {
            return gl::Error(GL_OUT_OF_MEMORY,
                             "Internal render target view pointer unexpectedly null.");
        }

        rtvs[i] = rtv;
    }
    ID3D11DepthStencilView *dsv =
        maskedClearDepthStencil ? maskedClearDepthStencil->getDepthStencilView() : nullptr;

    const FLOAT blendFactors[4] = {
        clearParams.colorMaskRed ? 1.0f : 0.0f, clearParams.colorMaskGreen ? 1.0f : 0.0f,
        clearParams.colorMaskBlue ? 1.0f : 0.0f, clearParams.colorMaskAlpha ? 1.0f : 0.0f};
    const UINT sampleMask        = 0xFFFFFFFF;

    ID3D11DepthStencilState *dsState = getDepthStencilState(clearParams);
    const UINT stencilClear          = clearParams.stencilClearValue & 0xFF;

    // Set the clear color(s) and depth value
    ClearShader *shader = nullptr;
    D3D11_MAPPED_SUBRESOURCE mappedResource;

    HRESULT result = deviceContext->Map(mColorAndDepthDataBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD,
                                        0, &mappedResource);
    if (FAILED(result))
    {
        return gl::Error(GL_OUT_OF_MEMORY,
                         "Failed to map internal masked clear constant buffer, HRESULT: 0x%X.",
                         result);
    }

    switch (clearParams.colorClearType)
    {
        case GL_FLOAT:
            // TODO (Shahmeer Esmail): Evaluate performance impact of using a single-color clear
            // instead of a multi-color clear
            ApplyAdjustedColorAndDepthData(clearParams.colorFClearValue, maskedClearRenderTargets,
                                           clearParams.depthClearValue, mappedResource.pData);
            shader = mFloatClearShader;
            break;

        case GL_UNSIGNED_INT:
            ApplyColorAndDepthData(clearParams.colorUIClearValue, clearParams.depthClearValue,
                                   mappedResource.pData);
            shader = mUintClearShader;
            break;

        case GL_INT:
            ApplyColorAndDepthData(clearParams.colorIClearValue, clearParams.depthClearValue,
                                   mappedResource.pData);
            shader = mIntClearShader;
            break;

        default:
            UNREACHABLE();
            break;
    }

    deviceContext->Unmap(mColorAndDepthDataBuffer.Get(), 0);

    // Set the viewport to be the same size as the framebuffer
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = static_cast<FLOAT>(framebufferSize.width);
    viewport.Height   = static_cast<FLOAT>(framebufferSize.height);
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    deviceContext->RSSetViewports(1, &viewport);

    if (needScissoredClear)
    {
        D3D11_RECT scissorRect;
        scissorRect.top    = clearParams.scissor.y;
        scissorRect.bottom = clearParams.scissor.y + clearParams.scissor.height;
        scissorRect.left   = clearParams.scissor.x;
        scissorRect.right  = clearParams.scissor.x + clearParams.scissor.width;
        deviceContext->RSSetScissorRects(1, &scissorRect);
    }

    // Set state
    deviceContext->OMSetBlendState(mBlendState.Get(), blendFactors, sampleMask);
    deviceContext->OMSetDepthStencilState(dsState, stencilClear);
    deviceContext->RSSetState(needScissoredClear ? mScissorEnabledRasterizerState.Get()
                                                 : mScissorDisabledRasterizerState.Get());

    // Bind constant buffer
    deviceContext->PSSetConstantBuffers(0, 1, mColorAndDepthDataBuffer.GetAddressOf());

    // Bind shaders
    deviceContext->VSSetShader(shader->vertexShader.resolve(device), nullptr, 0);
    deviceContext->PSSetShader(shader->pixelShader.resolve(device), nullptr, 0);
    deviceContext->GSSetShader(nullptr, nullptr, 0);

    const UINT vertexStride = sizeof(d3d11::PositionVertex);
    const UINT startIdx     = 0;

    deviceContext->IASetInputLayout(shader->inputLayout.resolve(device));
    deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &vertexStride, &startIdx);

    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    // Bind render target(s) and depthStencil buffer
    mRenderer->getStateManager()->setOneTimeRenderTargets(rtvs, dsv);

    // Draw the clear quad
    deviceContext->Draw(4, 0);

    // Clean up
    mRenderer->markAllStateDirty();

    return gl::NoError();
}

ID3D11DepthStencilState *Clear11::getDepthStencilState(const ClearParameters &clearParams)
{
    ClearDepthStencilInfo dsKey = {0};
    dsKey.clearDepth            = clearParams.clearDepth;
    dsKey.clearStencil          = clearParams.clearStencil;
    dsKey.stencilWriteMask      = clearParams.stencilWriteMask & 0xFF;

    ClearDepthStencilStateMap::const_iterator i = mClearDepthStencilStates.find(dsKey);
    if (i != mClearDepthStencilStates.end())
    {
        return i->second;
    }
    else
    {
        D3D11_DEPTH_STENCIL_DESC dsDesc = {0};
        dsDesc.DepthEnable              = dsKey.clearDepth ? TRUE : FALSE;
        dsDesc.DepthWriteMask =
            dsKey.clearDepth ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
        dsDesc.DepthFunc                    = D3D11_COMPARISON_ALWAYS;
        dsDesc.StencilEnable                = dsKey.clearStencil ? TRUE : FALSE;
        dsDesc.StencilReadMask              = 0;
        dsDesc.StencilWriteMask             = dsKey.stencilWriteMask;
        dsDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_REPLACE;
        dsDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
        dsDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_REPLACE;
        dsDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;

        ID3D11Device *device             = mRenderer->getDevice();
        ID3D11DepthStencilState *dsState = nullptr;
        HRESULT result                   = device->CreateDepthStencilState(&dsDesc, &dsState);
        if (FAILED(result) || !dsState)
        {
            ERR() << "Unable to create a ID3D11DepthStencilState, " << gl::FmtHR(result) << ".";
            return nullptr;
        }

        mClearDepthStencilStates[dsKey] = dsState;

        return dsState;
    }
}
}

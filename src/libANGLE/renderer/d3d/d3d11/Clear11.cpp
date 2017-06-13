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
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clear11_fl9vs.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clear11vs.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearfloat11_fl9ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearfloat11ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearuint11ps.h"
#include "libANGLE/renderer/d3d/d3d11/shaders/compiled/clearsint11ps.h"

namespace rx
{

namespace
{

static constexpr uint32_t g_ConstantBufferSize = sizeof(RtvDsvClearInfo<float>);
static constexpr uint32_t g_VertexSize         = sizeof(d3d11::PositionVertex);

// Updates color, depth and alpha components of cached CB if necessary.
// Returns true if any constants are updated, false otherwise.
template <typename T>
bool UpdateDataCache(RtvDsvClearInfo<T> *dataCache,
                     const gl::Color<T> &color,
                     const float *zValue,
                     const uint32_t numRtvs,
                     const uint8_t writeMask)
{
    bool cacheDirty = false;

    if (numRtvs > 0)
    {
        const bool writeRGB = (writeMask & ~D3D11_COLOR_WRITE_ENABLE_ALPHA) != 0;
        if (writeRGB && memcmp(&dataCache->r, &color.red, sizeof(T) * 3) != 0)
        {
            dataCache->r = color.red;
            dataCache->g = color.green;
            dataCache->b = color.blue;
            cacheDirty   = true;
        }

        const bool writeAlpha = (writeMask & D3D11_COLOR_WRITE_ENABLE_ALPHA) != 0;
        if (writeAlpha && (dataCache->a != color.alpha))
        {
            dataCache->a = color.alpha;
            cacheDirty   = true;
        }
    }

    if (zValue)
    {
        const float clampedZValue = gl::clamp01(*zValue);

        if (clampedZValue != dataCache->z)
        {
            dataCache->z = clampedZValue;
            cacheDirty   = true;
        }
    }

    return cacheDirty;
}
}  // anonymous namespace

Clear11::ShaderManager::ShaderManager()
    : mIl9(nullptr),
      mVs9(g_VS_Clear_FL9, ArraySize(g_VS_Clear_FL9), "Clear11 VS FL9"),
      mPsFloat9(g_PS_ClearFloat_FL9, ArraySize(g_PS_ClearFloat_FL9), "Clear11 PS FloatFL9"),
      mVs(g_VS_Clear, ArraySize(g_VS_Clear), "Clear11 VS"),
      mPsFloat(g_PS_ClearFloat, ArraySize(g_PS_ClearFloat), "Clear11 PS Float"),
      mPsUInt(g_PS_ClearUint, ArraySize(g_PS_ClearUint), "Clear11 PS UINT"),
      mPsSInt(g_PS_ClearSint, ArraySize(g_PS_ClearSint), "Clear11 PS SINT")
{
}

Clear11::ShaderManager::~ShaderManager()
{
    mVs9.release();
    mPsFloat9.release();
    mVs.release();
    mPsFloat.release();
    mPsUInt.release();
    mPsSInt.release();
}

void Clear11::ShaderManager::getShadersAndLayout(ID3D11Device *device,
                                                 D3D_FEATURE_LEVEL featureLevel,
                                                 const INT clearType,
                                                 ID3D11InputLayout **il,
                                                 ID3D11VertexShader **vs,
                                                 ID3D11PixelShader **ps)
{
    if (featureLevel <= D3D_FEATURE_LEVEL_9_3)
    {
        ASSERT(clearType == GL_FLOAT);

        *vs = mVs9.resolve(device);

        if (mIl9.Get() == nullptr)
        {
            const D3D11_INPUT_ELEMENT_DESC ilDesc = {
                "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0};

            device->CreateInputLayout(&ilDesc, 1, g_VS_Clear_FL9, ArraySize(g_PS_ClearFloat_FL9),
                                      mIl9.GetAddressOf());
        }

        *il = mIl9.Get();
        *ps = mPsFloat9.resolve(device);
        return;
    }

    *vs = mVs.resolve(device);
    *il = nullptr;

    switch (clearType)
    {
        case GL_FLOAT:
            *ps = mPsFloat.resolve(device);
            break;
        case GL_UNSIGNED_INT:
            *ps = mPsUInt.resolve(device);
            break;
        case GL_INT:
            *ps = mPsSInt.resolve(device);
            break;
        default:
            UNREACHABLE();
            break;
    }
}

Clear11::Clear11(Renderer11 *renderer)
    : mRenderer(renderer),
      mScissorEnabledRasterizerState(nullptr),
      mScissorDisabledRasterizerState(nullptr),
      mShaderManager(),
      mConstantBuffer(nullptr),
      mVertexBuffer(nullptr),
      mShaderData({})
{
    TRACE_EVENT0("gpu.angle", "Clear11::Clear11");

    HRESULT result;
    ID3D11Device *device = renderer->getDevice();

    static_assert((sizeof(RtvDsvClearInfo<float>) == sizeof(RtvDsvClearInfo<int>)),
                  "Size of rx::RtvDsvClearInfo<float> is not equal to rx::RtvDsvClearInfo<int>");

    static_assert(
        (sizeof(RtvDsvClearInfo<float>) == sizeof(RtvDsvClearInfo<uint32_t>)),
        "Size of rx::RtvDsvClearInfo<float> is not equal to rx::RtvDsvClearInfo<uint32_t>");

    static_assert((sizeof(RtvDsvClearInfo<float>) % 16 == 0),
                  "The size of RtvDsvClearInfo<float> should be a multiple of 16bytes.");

    // Create constant buffer for color & depth data

    D3D11_BUFFER_DESC bufferDesc;
    bufferDesc.ByteWidth           = g_ConstantBufferSize;
    bufferDesc.Usage               = D3D11_USAGE_DYNAMIC;
    bufferDesc.BindFlags           = D3D11_BIND_CONSTANT_BUFFER;
    bufferDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    bufferDesc.MiscFlags           = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initialData;
    initialData.pSysMem          = &mShaderData;
    initialData.SysMemPitch      = g_ConstantBufferSize;
    initialData.SysMemSlicePitch = g_ConstantBufferSize;

    result = device->CreateBuffer(&bufferDesc, &initialData, mConstantBuffer.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mConstantBuffer, "Clear11 Constant Buffer");

    const D3D_FEATURE_LEVEL featureLevel = mRenderer->getRenderer11DeviceCaps().featureLevel;

    if (featureLevel <= D3D_FEATURE_LEVEL_9_3)
    {
        // Create vertex buffer with vertices for a quad covering the entire surface

        static_assert((sizeof(d3d11::PositionVertex) % 16) == 0,
                      "d3d11::PositionVertex should be a multiple of 16 bytes");
        const d3d11::PositionVertex vbData[6] = {
            {-1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, -1.0f, 0.0f, 1.0f}, {-1.0f, -1.0f, 0.0f, 1.0f},
            {-1.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f, 0.0f, 1.0f},  {1.0f, -1.0f, 0.0f, 1.0f}};

        const UINT vbSize = sizeof(vbData);

        bufferDesc.ByteWidth           = vbSize;
        bufferDesc.Usage               = D3D11_USAGE_IMMUTABLE;
        bufferDesc.BindFlags           = D3D11_BIND_VERTEX_BUFFER;
        bufferDesc.CPUAccessFlags      = 0;
        bufferDesc.MiscFlags           = 0;
        bufferDesc.StructureByteStride = 0;

        initialData.pSysMem          = vbData;
        initialData.SysMemPitch      = vbSize;
        initialData.SysMemSlicePitch = initialData.SysMemPitch;

        result = device->CreateBuffer(&bufferDesc, &initialData, mVertexBuffer.GetAddressOf());
        ASSERT(SUCCEEDED(result));
        d3d11::SetDebugName(mVertexBuffer, "Clear11 Vertex Buffer");
    }

    // Create Rasterizer States
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
                        "Clear11 Rasterizer State with scissor disabled");

    rsDesc.ScissorEnable = TRUE;
    result = device->CreateRasterizerState(&rsDesc, mScissorEnabledRasterizerState.GetAddressOf());
    ASSERT(SUCCEEDED(result));
    d3d11::SetDebugName(mScissorEnabledRasterizerState,
                        "Clear11 Rasterizer State with scissor enabled");

    // Initialize Depthstencil state with defaults
    mDepthStencilStateKey.depthTest                = false;
    mDepthStencilStateKey.depthMask                = false;
    mDepthStencilStateKey.depthFunc                = GL_ALWAYS;
    mDepthStencilStateKey.stencilWritemask         = static_cast<GLuint>(-1);
    mDepthStencilStateKey.stencilBackWritemask     = static_cast<GLuint>(-1);
    mDepthStencilStateKey.stencilBackMask          = 0;
    mDepthStencilStateKey.stencilTest              = false;
    mDepthStencilStateKey.stencilMask              = 0;
    mDepthStencilStateKey.stencilFail              = GL_REPLACE;
    mDepthStencilStateKey.stencilPassDepthFail     = GL_REPLACE;
    mDepthStencilStateKey.stencilPassDepthPass     = GL_REPLACE;
    mDepthStencilStateKey.stencilFunc              = GL_ALWAYS;
    mDepthStencilStateKey.stencilBackFail          = GL_REPLACE;
    mDepthStencilStateKey.stencilBackPassDepthFail = GL_REPLACE;
    mDepthStencilStateKey.stencilBackPassDepthPass = GL_REPLACE;
    mDepthStencilStateKey.stencilBackFunc          = GL_ALWAYS;

    // Initialize BlendStateKey with defaults
    mBlendStateKey.blendState.blend                 = false;
    mBlendStateKey.blendState.sourceBlendRGB        = GL_ONE;
    mBlendStateKey.blendState.sourceBlendAlpha      = GL_ONE;
    mBlendStateKey.blendState.destBlendRGB          = GL_ZERO;
    mBlendStateKey.blendState.destBlendAlpha        = GL_ZERO;
    mBlendStateKey.blendState.blendEquationRGB      = GL_FUNC_ADD;
    mBlendStateKey.blendState.blendEquationAlpha    = GL_FUNC_ADD;
    mBlendStateKey.blendState.sampleAlphaToCoverage = false;
    mBlendStateKey.blendState.dither                = true;
    mBlendStateKey.mrt                              = false;
    memset(mBlendStateKey.rtvMasks, 0, sizeof(mBlendStateKey.rtvMasks));
}

Clear11::~Clear11()
{
}

gl::Error Clear11::clearFramebuffer(const ClearParameters &clearParams,
                                    const gl::FramebufferState &fboData)
{
    // Iterate over the color buffers which require clearing and determine if they can be
    // cleared with ID3D11DeviceContext::ClearRenderTargetView or ID3D11DeviceContext1::ClearView.
    // This requires:
    // 1) The render target is being cleared to a float value (will be cast to integer when clearing
    // integer render targets as expected but does not work the other way around)
    // 2) The format of the render target has no color channels that are currently masked out.
    // Clear the easy-to-clear buffers on the spot and accumulate the ones that require special
    // work.
    //
    // If these conditions are met, and:
    // - No scissored clear is needed, then clear using ID3D11DeviceContext::ClearRenderTargetView.
    // - A scissored clear is needed then clear using ID3D11DeviceContext1::ClearView if available.
    // Otherwise perform a shader based clear.
    //
    // Also determine if the DSV can be cleared withID3D11DeviceContext::ClearDepthStencilView by
    // checking if the stencil write mask covers the entire stencil.
    //
    // To clear the remaining buffers, a shader based clear is performed:
    // - The appropriate ShaderManagers (VS & PS) for the clearType is set
    // - A CB containing the clear color and Z values is bound
    // - An IL and VB are bound (for FL93 and below)
    // - ScissorRect/Raststate/Viewport set as required
    // - Blendstate set containing appropriate colorMasks
    // - DepthStencilState set with appropriate parameters for a z or stencil clear if required
    // - Color and/or Z buffers to be cleared are bound
    // - Primitive covering entire clear area is drawn

    gl::Extents framebufferSize;

    const auto *depthStencilAttachment = fboData.getDepthOrStencilAttachment();
    if (depthStencilAttachment != nullptr)
    {
        framebufferSize = depthStencilAttachment->getSize();
    }
    else
    {
        const auto colorAttachment = fboData.getFirstColorAttachment();

        if (!colorAttachment)
        {
            UNREACHABLE();
            return gl::InternalError();
        }

        framebufferSize = colorAttachment->getSize();
    }

    bool needScissoredClear = false;

    if (clearParams.scissorEnabled)
    {
        if (clearParams.scissor.x >= framebufferSize.width ||
            clearParams.scissor.y >= framebufferSize.height ||
            clearParams.scissor.x + clearParams.scissor.width <= 0 ||
            clearParams.scissor.y + clearParams.scissor.height <= 0 ||
            clearParams.scissor.width == 0 || clearParams.scissor.height == 0)
        {
            // Scissor rect is outside the renderbuffer or is an empty rect
            return gl::NoError();
        }

        needScissoredClear =
            clearParams.scissor.x > 0 || clearParams.scissor.y > 0 ||
            clearParams.scissor.x + clearParams.scissor.width < framebufferSize.width ||
            clearParams.scissor.y + clearParams.scissor.height < framebufferSize.height;
    }

    ID3D11DeviceContext *deviceContext   = mRenderer->getDeviceContext();
    ID3D11DeviceContext1 *deviceContext1 = mRenderer->getDeviceContext1IfSupported();

    std::array<ID3D11RenderTargetView *, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> rtvs;
    std::array<uint8_t, D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT> rtvMasks = {};

    uint32_t numRtvs = 0;
    const uint8_t colorMask =
        gl_d3d11::ConvertColorMask(clearParams.colorMaskRed, clearParams.colorMaskGreen,
                                   clearParams.colorMaskBlue, clearParams.colorMaskAlpha);

    const auto &colorAttachments = fboData.getColorAttachments();
    const auto &drawBufferStates = fboData.getDrawBufferStates();
    for (size_t colorAttachmentIndex = 0; colorAttachmentIndex < colorAttachments.size();
        colorAttachmentIndex++)
    {
        const gl::FramebufferAttachment &attachment = colorAttachments[colorAttachmentIndex];

        if (clearParams.clearColor[colorAttachmentIndex] && attachment.isAttached() &&
            drawBufferStates[colorAttachmentIndex] != GL_NONE)
        {
            continue;
        }

        RenderTarget11 *renderTarget = nullptr;
        ANGLE_TRY(attachment.getRenderTarget(&renderTarget));

        const gl::InternalFormat &formatInfo = *attachment.getFormat().info;

        if (clearParams.colorType == GL_FLOAT &&
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

        const auto &framebufferRTV = renderTarget->getRenderTargetView();
        ASSERT(framebufferRTV.valid());

        if ((!(mRenderer->getRenderer11DeviceCaps().supportsClearView) && needScissoredClear) ||
            clearParams.colorType != GL_FLOAT ||
            (formatInfo.redBits > 0 && !clearParams.colorMaskRed) ||
            (formatInfo.greenBits > 0 && !clearParams.colorMaskGreen) ||
            (formatInfo.blueBits > 0 && !clearParams.colorMaskBlue) ||
            (formatInfo.alphaBits > 0 && !clearParams.colorMaskAlpha))
        {
            rtvs[numRtvs]     = framebufferRTV.get();
            rtvMasks[numRtvs] = gl_d3d11::GetColorMask(formatInfo) & colorMask;
            numRtvs++;
        }
        else
        {
            // ID3D11DeviceContext::ClearRenderTargetView or ID3D11DeviceContext1::ClearView is
            // possible

            const auto &nativeFormat = renderTarget->getFormatSet().format();

            // Check if the actual format has a channel that the internal format does not and
            // set them to the default values
            float clearValues[4] = {
                ((formatInfo.redBits == 0 && nativeFormat.redBits > 0) ? 0.0f
                                                                       : clearParams.colorF.red),
                ((formatInfo.greenBits == 0 && nativeFormat.greenBits > 0)
                     ? 0.0f
                     : clearParams.colorF.green),
                ((formatInfo.blueBits == 0 && nativeFormat.blueBits > 0) ? 0.0f
                                                                         : clearParams.colorF.blue),
                ((formatInfo.alphaBits == 0 && nativeFormat.alphaBits > 0)
                     ? 1.0f
                     : clearParams.colorF.alpha),
            };

            if (formatInfo.alphaBits == 1)
            {
                // Some drivers do not correctly handle calling Clear() on a format with 1-bit
                // alpha. They can incorrectly round all non-zero values up to 1.0f. Note that
                // WARP does not do this. We should handle the rounding for them instead.
                clearValues[3] = (clearParams.colorF.alpha >= 0.5f) ? 1.0f : 0.0f;
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

                deviceContext1->ClearView(framebufferRTV.get(), clearValues, &rect, 1);
                if (mRenderer->getWorkarounds().callClearTwiceOnSmallTarget)
                {
                    if (clearParams.scissor.width <= 16 || clearParams.scissor.height <= 16)
                    {
                        deviceContext1->ClearView(framebufferRTV.get(), clearValues, &rect, 1);
                    }
                }
            }
            else
            {
                deviceContext->ClearRenderTargetView(framebufferRTV.get(), clearValues);
                if (mRenderer->getWorkarounds().callClearTwiceOnSmallTarget)
                {
                    if (framebufferSize.width <= 16 || framebufferSize.height <= 16)
                    {
                        deviceContext->ClearRenderTargetView(framebufferRTV.get(), clearValues);
                    }
                }
            }
        }
    }

    ID3D11DepthStencilView *dsv = nullptr;

    if (clearParams.clearDepth || clearParams.clearStencil)
    {
        RenderTarget11 *depthStencilRenderTarget = nullptr;

        ASSERT(depthStencilAttachment != nullptr);
        ANGLE_TRY(depthStencilAttachment->getRenderTarget(&depthStencilRenderTarget));

        dsv = depthStencilRenderTarget->getDepthStencilView().get();
        ASSERT(dsv != nullptr);

        const auto &nativeFormat = depthStencilRenderTarget->getFormatSet().format();
        const auto *stencilAttachment = fboData.getStencilAttachment();

        uint32_t stencilUnmasked =
            (stencilAttachment != nullptr) ? (1 << nativeFormat.stencilBits) - 1 : 0;
        bool needMaskedStencilClear =
            clearParams.clearStencil &&
            (clearParams.stencilWriteMask & stencilUnmasked) != stencilUnmasked;

        if (!needScissoredClear && !needMaskedStencilClear)
        {
            const UINT clearFlags = (clearParams.clearDepth ? D3D11_CLEAR_DEPTH : 0) |
                                    (clearParams.clearStencil ? D3D11_CLEAR_STENCIL : 0);
            const FLOAT depthClear   = gl::clamp01(clearParams.depthValue);
            const UINT8 stencilClear = clearParams.stencilValue & 0xFF;

            deviceContext->ClearDepthStencilView(dsv, clearFlags, depthClear, stencilClear);

            dsv = nullptr;
        }
    }

    if (numRtvs == 0 && dsv == nullptr)
    {
        return gl::NoError();
    }

    // Clear the remaining render targets and depth stencil in one pass by rendering a quad:
    //
    // IA/VS: Vertices containing position and color members are passed through to the next stage.
    // The vertex position has XY coordinates equal to clip extents and a Z component equal to the
    // Z clear value. The vertex color contains the clear color.
    //
    // Rasterizer: Viewport scales the VS output over the entire surface and depending on whether
    // or not scissoring is enabled the appropriate scissor rect and rasterizerState with or without
    // the scissor test enabled is set as well.
    //
    // DepthStencilTest: DepthTesting, DepthWrites, StencilMask and StencilWrites will be enabled or
    // disabled or set depending on what the input depthStencil clear parameters are. Since the PS
    // is not writing out depth or rejecting pixels, this should happen prior to the PS stage.
    //
    // PS: Will write out the color values passed through from the previous stage to all outputs.
    //
    // OM: BlendState will perform the required color masking and output to RTV(s).

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

    ASSERT(numRtvs <= mRenderer->getNativeCaps().maxDrawBuffers);

    // Setup BlendStateKey parameters
    mBlendStateKey.blendState.colorMaskRed   = clearParams.colorMaskRed;
    mBlendStateKey.blendState.colorMaskGreen = clearParams.colorMaskGreen;
    mBlendStateKey.blendState.colorMaskBlue  = clearParams.colorMaskBlue;
    mBlendStateKey.blendState.colorMaskAlpha = clearParams.colorMaskAlpha;
    mBlendStateKey.mrt                       = numRtvs > 1;
    memcpy(mBlendStateKey.rtvMasks, &rtvMasks[0], sizeof(mBlendStateKey.rtvMasks));

    // Get BlendState
    ID3D11BlendState *blendState = nullptr;
    ANGLE_TRY(mRenderer->getStateCache().getBlendState(mBlendStateKey, &blendState));

    ID3D11DepthStencilState *dsState = nullptr;
    const float *zValue              = nullptr;

    if (dsv)
    {
        // Setup DepthStencilStateKey
        mDepthStencilStateKey.depthTest        = clearParams.clearDepth;
        mDepthStencilStateKey.depthMask        = clearParams.clearDepth;
        mDepthStencilStateKey.stencilWritemask = clearParams.stencilWriteMask;
        mDepthStencilStateKey.stencilTest      = clearParams.clearStencil;

        // Get DepthStencilState
        ANGLE_TRY(mRenderer->getStateCache().getDepthStencilState(mDepthStencilStateKey, &dsState));
        zValue = clearParams.clearDepth ? &clearParams.depthValue : nullptr;
    }

    bool dirtyCb = false;

    // Compare the input color/z values against the CB cache and update it if necessary
    switch (clearParams.colorType)
    {
        case GL_FLOAT:
            dirtyCb = UpdateDataCache(reinterpret_cast<RtvDsvClearInfo<float> *>(&mShaderData),
                                      clearParams.colorF, zValue, numRtvs, colorMask);
            break;
        case GL_UNSIGNED_INT:
            dirtyCb = UpdateDataCache(reinterpret_cast<RtvDsvClearInfo<uint32_t> *>(&mShaderData),
                                      clearParams.colorUI, zValue, numRtvs, colorMask);
            break;
        case GL_INT:
            dirtyCb = UpdateDataCache(reinterpret_cast<RtvDsvClearInfo<int> *>(&mShaderData),
                                      clearParams.colorI, zValue, numRtvs, colorMask);
            break;
        default:
            UNREACHABLE();
            break;
    }

    if (dirtyCb)
    {
        // Update the constant buffer with the updated cache contents
        // TODO(Shahmeer): Consider using UpdateSubresource1 D3D11_COPY_DISCARD where possible.
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        HRESULT result = deviceContext->Map(mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                                            &mappedResource);
        if (FAILED(result))
        {
            return gl::OutOfMemory() << "Clear11: Failed to map CB, " << result;
        }

        memcpy(mappedResource.pData, &mShaderData, g_ConstantBufferSize);
        deviceContext->Unmap(mConstantBuffer.Get(), 0);
    }

    // Set the viewport to be the same size as the framebuffer
    D3D11_VIEWPORT viewport;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width    = static_cast<FLOAT>(framebufferSize.width);
    viewport.Height   = static_cast<FLOAT>(framebufferSize.height);
    viewport.MinDepth = 0;
    viewport.MaxDepth = 1;
    deviceContext->RSSetViewports(1, &viewport);

    // Apply state
    deviceContext->OMSetBlendState(blendState, nullptr, 0xFFFFFFFF);

    const UINT stencilValue = clearParams.stencilValue & 0xFF;
    deviceContext->OMSetDepthStencilState(dsState, stencilValue);

    if (needScissoredClear)
    {
        const D3D11_RECT scissorRect = {clearParams.scissor.x, clearParams.scissor.y,
                                        clearParams.scissor.x1(), clearParams.scissor.y1()};
        deviceContext->RSSetScissorRects(1, &scissorRect);
        deviceContext->RSSetState(mScissorEnabledRasterizerState.Get());
    }
    else
    {
        deviceContext->RSSetState(mScissorDisabledRasterizerState.Get());
    }

    // Get Shaders
    const D3D_FEATURE_LEVEL fl = mRenderer->getRenderer11DeviceCaps().featureLevel;
    ID3D11Device *device = mRenderer->getDevice();
    ID3D11VertexShader *vs;
    ID3D11InputLayout *il;
    ID3D11PixelShader *ps;

    mShaderManager.getShadersAndLayout(device, fl, clearParams.colorType, &il, &vs, &ps);

    // Apply Shaders
    deviceContext->VSSetShader(vs, nullptr, 0);
    deviceContext->GSSetShader(nullptr, nullptr, 0);
    deviceContext->PSSetShader(ps, nullptr, 0);
    deviceContext->PSSetConstantBuffers(0, 1, mConstantBuffer.GetAddressOf());

    // Bind IL & VB if needed
    deviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_UNKNOWN, 0);
    deviceContext->IASetInputLayout(il);

    if (mVertexBuffer.Get())
    {
        const UINT offset = 0;
        deviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &g_VertexSize,
                                          &offset);
    }
    else
    {
        deviceContext->IASetVertexBuffers(0, 0, nullptr, nullptr, nullptr);
    }

    deviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // Apply render targets
    mRenderer->getStateManager()->setOneTimeRenderTargets(&rtvs[0], numRtvs, dsv);

    // Draw the fullscreen quad
    deviceContext->Draw(6, 0);

    // Clean up
    mRenderer->markAllStateDirty();

    return gl::NoError();
}
}

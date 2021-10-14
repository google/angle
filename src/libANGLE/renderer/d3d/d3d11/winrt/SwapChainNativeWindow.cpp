//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainNativeWindow.cpp: NativeWindow for managing IDXGISwapChain native window types.

#include "libANGLE/renderer/d3d/d3d11/winrt/SwapChainNativeWindow.h"

using namespace Microsoft::WRL;

namespace
{
DXGI_FORMAT GetSwapChainFormat(const ComPtr<IDXGISwapChain> &swapChain)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    HRESULT result            = swapChain->GetDesc(&desc);
    if (SUCCEEDED(result))
    {
        return desc.BufferDesc.Format;
    }
    return DXGI_FORMAT_UNKNOWN;
}

HRESULT GetSwapChainSize(const ComPtr<IDXGISwapChain> &swapChain, Size *windowSize)
{
    DXGI_SWAP_CHAIN_DESC desc = {};
    HRESULT result            = swapChain->GetDesc(&desc);
    if (SUCCEEDED(result))
    {
        *windowSize = {(float)desc.BufferDesc.Width, (float)desc.BufferDesc.Height};
    }
    return result;
}
}  // namespace

namespace rx
{
bool SwapChainNativeWindow::initialize(EGLNativeWindowType window, IPropertySet *propertySet)
{
    mSupportsSwapChainResize = false;
    mSwapChainScale          = 1.0f;

    ComPtr<IInspectable> win = window;
    HRESULT result           = win.As(&mSwapChain);

    if (SUCCEEDED(result))
    {
        Size swapChainSize;
        result = GetSwapChainSize(mSwapChain, &swapChainSize);

        if (SUCCEEDED(result))
        {
            // Update the client rect to account for any swapchain scale factor
            mClientRect = clientRect(swapChainSize);
        }
    }

    if (SUCCEEDED(result))
    {
        mNewClientRect     = mClientRect;
        mClientRectChanged = false;
        return true;
    }

    return false;
}

HRESULT SwapChainNativeWindow::createSwapChain(ID3D11Device *device,
                                               IDXGIFactory2 *factory,
                                               DXGI_FORMAT format,
                                               unsigned int width,
                                               unsigned int height,
                                               bool containsAlpha,
                                               IDXGISwapChain1 **swapChain)
{
    if (swapChain == nullptr || width == 0 || height == 0 ||
        format != GetSwapChainFormat(mSwapChain))
    {
        return E_INVALIDARG;
    }

    return mSwapChain.CopyTo(swapChain);
}

HRESULT SwapChainNativeWindow::scaleSwapChain(const Size &windowSize, const RECT &clientRect)
{
    return S_OK;
}
}  // namespace rx

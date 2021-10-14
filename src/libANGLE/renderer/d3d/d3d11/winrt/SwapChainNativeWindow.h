//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// SwapChainNativeWindow.h: NativeWindow for managing IDXGISwapChain native window types.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_WINRT_SWAPCHAINNATIVEWINDOW_H_
#define LIBANGLE_RENDERER_D3D_D3D11_WINRT_SWAPCHAINNATIVEWINDOW_H_

#include "libANGLE/renderer/d3d/d3d11/winrt/InspectableNativeWindow.h"

#include <memory>

namespace rx
{
class SwapChainNativeWindow : public InspectableNativeWindow,
                              public std::enable_shared_from_this<SwapChainNativeWindow>
{
  public:
    bool initialize(EGLNativeWindowType window, IPropertySet *propertySet) override;
    HRESULT createSwapChain(ID3D11Device *device,
                            IDXGIFactory2 *factory,
                            DXGI_FORMAT format,
                            unsigned int width,
                            unsigned int height,
                            bool containsAlpha,
                            IDXGISwapChain1 **swapChain) override;

  protected:
    HRESULT scaleSwapChain(const Size &windowSize, const RECT &clientRect) override;

  private:
    ComPtr<IMap<HSTRING, IInspectable *>> mPropertyMap;
    ComPtr<IDXGISwapChain> mSwapChain;
};
}  // namespace rx
#endif  // LIBANGLE_RENDERER_D3D_D3D11_WINRT_SWAPCHAINPANELNATIVEWINDOW_H_

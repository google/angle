//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Framebuffer9.cpp: Implements the Framebuffer9 class.

#include "libANGLE/renderer/d3d/d3d9/Framebuffer9.h"
#include "libANGLE/renderer/d3d/d3d9/formatutils9.h"
#include "libANGLE/renderer/d3d/d3d9/TextureStorage9.h"
#include "libANGLE/renderer/d3d/d3d9/Renderer9.h"
#include "libANGLE/renderer/d3d/d3d9/renderer9_utils.h"
#include "libANGLE/renderer/d3d/d3d9/RenderTarget9.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/Texture.h"

namespace rx
{

Framebuffer9::Framebuffer9(Renderer9 *renderer)
    : FramebufferD3D(renderer),
      mRenderer(renderer)
{
    ASSERT(mRenderer != nullptr);
}

Framebuffer9::~Framebuffer9()
{
}

gl::Error Framebuffer9::clear(const gl::State &state, const gl::ClearParameters &clearParams)
{
    const gl::FramebufferAttachment *colorBuffer = mColorBuffers[0];
    const gl::FramebufferAttachment *depthStencilBuffer = (mDepthbuffer != nullptr) ? mDepthbuffer
                                                                                    : mStencilbuffer;

    gl::Error error = mRenderer->applyRenderTarget(colorBuffer, depthStencilBuffer);
    if (error.isError())
    {
        return error;
    }

    float nearZ, farZ;
    state.getDepthRange(&nearZ, &farZ);
    mRenderer->setViewport(state.getViewport(), nearZ, farZ, GL_TRIANGLES, state.getRasterizerState().frontFace, true);

    mRenderer->setScissorRectangle(state.getScissor(), state.isScissorTestEnabled());

    return mRenderer->clear(clearParams, mColorBuffers[0], depthStencilBuffer);
}

gl::Error Framebuffer9::readPixels(const gl::Rectangle &area, GLenum format, GLenum type, size_t outputPitch, const gl::PixelPackState &pack, uint8_t *pixels) const
{
    ASSERT(pack.pixelBuffer.get() == NULL);

    const gl::FramebufferAttachment *colorbuffer = mColorBuffers[0];
    ASSERT(colorbuffer);

    RenderTarget9 *renderTarget = NULL;
    gl::Error error = d3d9::GetAttachmentRenderTarget(colorbuffer, &renderTarget);
    if (error.isError())
    {
        return error;
    }
    ASSERT(renderTarget);

    IDirect3DSurface9 *surface = renderTarget->getSurface();
    ASSERT(surface);

    D3DSURFACE_DESC desc;
    surface->GetDesc(&desc);

    if (desc.MultiSampleType != D3DMULTISAMPLE_NONE)
    {
        UNIMPLEMENTED();   // FIXME: Requires resolve using StretchRect into non-multisampled render target
        SafeRelease(surface);
        return gl::Error(GL_OUT_OF_MEMORY, "ReadPixels is unimplemented for multisampled framebuffer attachments.");
    }

    IDirect3DDevice9 *device = mRenderer->getDevice();
    ASSERT(device);

    HRESULT result;
    IDirect3DSurface9 *systemSurface = NULL;
    bool directToPixels = !pack.reverseRowOrder && pack.alignment <= 4 && mRenderer->getShareHandleSupport() &&
                          area.x == 0 && area.y == 0 &&
                          static_cast<UINT>(area.width) == desc.Width && static_cast<UINT>(area.height) == desc.Height &&
                          desc.Format == D3DFMT_A8R8G8B8 && format == GL_BGRA_EXT && type == GL_UNSIGNED_BYTE;
    if (directToPixels)
    {
        // Use the pixels ptr as a shared handle to write directly into client's memory
        result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                                     D3DPOOL_SYSTEMMEM, &systemSurface, reinterpret_cast<void**>(&pixels));
        if (FAILED(result))
        {
            // Try again without the shared handle
            directToPixels = false;
        }
    }

    if (!directToPixels)
    {
        result = device->CreateOffscreenPlainSurface(desc.Width, desc.Height, desc.Format,
                                                     D3DPOOL_SYSTEMMEM, &systemSurface, NULL);
        if (FAILED(result))
        {
            ASSERT(result == D3DERR_OUTOFVIDEOMEMORY || result == E_OUTOFMEMORY);
            SafeRelease(surface);
            return gl::Error(GL_OUT_OF_MEMORY, "Failed to allocate internal texture for ReadPixels.");
        }
    }

    result = device->GetRenderTargetData(surface, systemSurface);
    SafeRelease(surface);

    if (FAILED(result))
    {
        SafeRelease(systemSurface);

        // It turns out that D3D will sometimes produce more error
        // codes than those documented.
        if (d3d9::isDeviceLostError(result))
        {
            mRenderer->notifyDeviceLost();
        }
        else
        {
            UNREACHABLE();
        }

        return gl::Error(GL_OUT_OF_MEMORY, "Failed to read internal render target data.");
    }

    if (directToPixels)
    {
        SafeRelease(systemSurface);
        return gl::Error(GL_NO_ERROR);
    }

    RECT rect;
    rect.left = gl::clamp(area.x, 0L, static_cast<LONG>(desc.Width));
    rect.top = gl::clamp(area.y, 0L, static_cast<LONG>(desc.Height));
    rect.right = gl::clamp(area.x + area.width, 0L, static_cast<LONG>(desc.Width));
    rect.bottom = gl::clamp(area.y + area.height, 0L, static_cast<LONG>(desc.Height));

    D3DLOCKED_RECT lock;
    result = systemSurface->LockRect(&lock, &rect, D3DLOCK_READONLY);

    if (FAILED(result))
    {
        UNREACHABLE();
        SafeRelease(systemSurface);

        return gl::Error(GL_OUT_OF_MEMORY, "Failed to lock internal render target.");
    }

    uint8_t *source;
    int inputPitch;
    if (pack.reverseRowOrder)
    {
        source = reinterpret_cast<uint8_t*>(lock.pBits) + lock.Pitch * (rect.bottom - rect.top - 1);
        inputPitch = -lock.Pitch;
    }
    else
    {
        source = reinterpret_cast<uint8_t*>(lock.pBits);
        inputPitch = lock.Pitch;
    }

    const d3d9::D3DFormat &d3dFormatInfo = d3d9::GetD3DFormatInfo(desc.Format);
    const gl::InternalFormat &sourceFormatInfo = gl::GetInternalFormatInfo(d3dFormatInfo.internalFormat);
    if (sourceFormatInfo.format == format && sourceFormatInfo.type == type)
    {
        // Direct copy possible
        for (int y = 0; y < rect.bottom - rect.top; y++)
        {
            memcpy(pixels + y * outputPitch, source + y * inputPitch, (rect.right - rect.left) * sourceFormatInfo.pixelBytes);
        }
    }
    else
    {
        const d3d9::D3DFormat &sourceD3DFormatInfo = d3d9::GetD3DFormatInfo(desc.Format);
        ColorCopyFunction fastCopyFunc = sourceD3DFormatInfo.getFastCopyFunction(format, type);

        const gl::FormatType &destFormatTypeInfo = gl::GetFormatTypeInfo(format, type);
        const gl::InternalFormat &destFormatInfo = gl::GetInternalFormatInfo(destFormatTypeInfo.internalFormat);

        if (fastCopyFunc)
        {
            // Fast copy is possible through some special function
            for (int y = 0; y < rect.bottom - rect.top; y++)
            {
                for (int x = 0; x < rect.right - rect.left; x++)
                {
                    uint8_t *dest = pixels + y * outputPitch + x * destFormatInfo.pixelBytes;
                    const uint8_t *src = source + y * inputPitch + x * sourceFormatInfo.pixelBytes;

                    fastCopyFunc(src, dest);
                }
            }
        }
        else
        {
            uint8_t temp[sizeof(gl::ColorF)];
            for (int y = 0; y < rect.bottom - rect.top; y++)
            {
                for (int x = 0; x < rect.right - rect.left; x++)
                {
                    uint8_t *dest = pixels + y * outputPitch + x * destFormatInfo.pixelBytes;
                    const uint8_t *src = source + y * inputPitch + x * sourceFormatInfo.pixelBytes;

                    // readFunc and writeFunc will be using the same type of color, CopyTexImage
                    // will not allow the copy otherwise.
                    sourceD3DFormatInfo.colorReadFunction(src, temp);
                    destFormatTypeInfo.colorWriteFunction(temp, dest);
                }
            }
        }
    }

    systemSurface->UnlockRect();
    SafeRelease(systemSurface);

    return gl::Error(GL_NO_ERROR);
}

}

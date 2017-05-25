//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ResourceManager11:
//   Centralized point of allocation for all D3D11 Resources.

#include "libANGLE/renderer/d3d/d3d11/ResourceManager11.h"

#include "common/debug.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "libANGLE/renderer/d3d/d3d11/formatutils11.h"

namespace rx
{

namespace
{
size_t ComputeMippedMemoryUsage(unsigned int width,
                                unsigned int height,
                                unsigned int depth,
                                size_t pixelSize,
                                unsigned int mipLevels)
{
    size_t sizeSum = 0;

    for (unsigned int level = 0; level < mipLevels; ++level)
    {
        unsigned int mipWidth  = std::max(width >> level, 1u);
        unsigned int mipHeight = std::max(height >> level, 1u);
        unsigned int mipDepth  = std::max(depth >> level, 1u);
        sizeSum += static_cast<size_t>(mipWidth * mipHeight * mipDepth) * pixelSize;
    }

    return sizeSum;
}

size_t ComputeMemoryUsage(const D3D11_TEXTURE2D_DESC *desc)
{
    ASSERT(desc);
    size_t pixelBytes = static_cast<size_t>(d3d11::GetDXGIFormatSizeInfo(desc->Format).pixelBytes);
    return ComputeMippedMemoryUsage(desc->Width, desc->Height, 1, pixelBytes, desc->MipLevels);
}

size_t ComputeMemoryUsage(const D3D11_TEXTURE3D_DESC *desc)
{
    ASSERT(desc);
    size_t pixelBytes = static_cast<size_t>(d3d11::GetDXGIFormatSizeInfo(desc->Format).pixelBytes);
    return ComputeMippedMemoryUsage(desc->Width, desc->Height, desc->Depth, pixelBytes,
                                    desc->MipLevels);
}

size_t ComputeMemoryUsage(const D3D11_BUFFER_DESC *desc)
{
    ASSERT(desc);
    return static_cast<size_t>(desc->ByteWidth);
}

template <typename T>
size_t ComputeMemoryUsage(const T *desc)
{
    return 0;
}

template <ResourceType ResourceT>
size_t ComputeGenericMemoryUsage(ID3D11DeviceChild *genericResource)
{
    auto *typedResource = static_cast<GetD3D11Type<ResourceT> *>(genericResource);
    GetDescType<ResourceT> desc;
    typedResource->GetDesc(&desc);
    return ComputeMemoryUsage(&desc);
}

size_t ComputeGenericMemoryUsage(ResourceType resourceType, ID3D11DeviceChild *resource)
{
    switch (resourceType)
    {
        case ResourceType::Texture2D:
            return ComputeGenericMemoryUsage<ResourceType::Texture2D>(resource);
        case ResourceType::Texture3D:
            return ComputeGenericMemoryUsage<ResourceType::Texture3D>(resource);
        case ResourceType::Buffer:
            return ComputeGenericMemoryUsage<ResourceType::Buffer>(resource);

        default:
            return 0;
    }
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_BLEND_DESC *desc,
                       void * /*initData*/,
                       ID3D11BlendState **blendState)
{
    return device->CreateBlendState(desc, blendState);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_BUFFER_DESC *desc,
                       const D3D11_SUBRESOURCE_DATA *initData,
                       ID3D11Buffer **buffer)
{
    return device->CreateBuffer(desc, initData, buffer);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_DEPTH_STENCIL_DESC *desc,
                       void * /*initData*/,
                       ID3D11DepthStencilState **resourceOut)
{
    return device->CreateDepthStencilState(desc, resourceOut);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_DEPTH_STENCIL_VIEW_DESC *desc,
                       ID3D11Resource *resource,
                       ID3D11DepthStencilView **resourceOut)
{
    return device->CreateDepthStencilView(resource, desc, resourceOut);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_RASTERIZER_DESC *desc,
                       void * /*initData*/,
                       ID3D11RasterizerState **rasterizerState)
{
    return device->CreateRasterizerState(desc, rasterizerState);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_RENDER_TARGET_VIEW_DESC *desc,
                       ID3D11Resource *resource,
                       ID3D11RenderTargetView **renderTargetView)
{
    return device->CreateRenderTargetView(resource, desc, renderTargetView);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_SAMPLER_DESC *desc,
                       void * /*initData*/,
                       ID3D11SamplerState **resourceOut)
{
    return device->CreateSamplerState(desc, resourceOut);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_SHADER_RESOURCE_VIEW_DESC *desc,
                       ID3D11Resource *resource,
                       ID3D11ShaderResourceView **resourceOut)
{
    return device->CreateShaderResourceView(resource, desc, resourceOut);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_TEXTURE2D_DESC *desc,
                       const D3D11_SUBRESOURCE_DATA *initData,
                       ID3D11Texture2D **texture)
{
    return device->CreateTexture2D(desc, initData, texture);
}

HRESULT CreateResource(ID3D11Device *device,
                       const D3D11_TEXTURE3D_DESC *desc,
                       const D3D11_SUBRESOURCE_DATA *initData,
                       ID3D11Texture3D **texture)
{
    return device->CreateTexture3D(desc, initData, texture);
}

#define ANGLE_RESOURCE_STRINGIFY_OP(NAME, RESTYPE, D3D11TYPE, DESCTYPE, INITDATATYPE) #RESTYPE

constexpr std::array<const char *, NumResourceTypes> kResourceTypeNames = {
    {ANGLE_RESOURCE_TYPE_OP(Stringify, ANGLE_RESOURCE_STRINGIFY_OP)}};
}  // anonymous namespace

// ResourceManager11 Implementation.
ResourceManager11::ResourceManager11()
    : mAllocatedResourceCounts({{}}), mAllocatedResourceDeviceMemory({{}})
{
}

ResourceManager11::~ResourceManager11()
{
    for (size_t count : mAllocatedResourceCounts)
    {
        ASSERT(count == 0);
    }

    for (size_t memorySize : mAllocatedResourceDeviceMemory)
    {
        ASSERT(memorySize == 0);
    }
}

template <typename T>
gl::Error ResourceManager11::allocate(Renderer11 *renderer,
                                      const GetDescFromD3D11<T> *desc,
                                      GetInitDataFromD3D11<T> *initData,
                                      Resource11<T> *resourceOut)
{
    ID3D11Device *device = renderer->getDevice();
    T *resource          = nullptr;

    HRESULT hr = CreateResource(device, desc, initData, &resource);

    if (FAILED(hr))
    {
        ASSERT(!resource);
        if (d3d11::isDeviceLostError(hr))
        {
            renderer->notifyDeviceLost();
        }
        return gl::OutOfMemory() << "Error allocating "
                                 << std::string(kResourceTypeNames[ResourceTypeIndex<T>()]) << ". "
                                 << gl::FmtHR(hr);
    }

    ASSERT(resource);
    incrResource(GetResourceTypeFromD3D11<T>(), ComputeMemoryUsage(desc));
    *resourceOut = std::move(Resource11<T>(resource, this));
    return gl::NoError();
}

void ResourceManager11::incrResource(ResourceType resourceType, size_t memorySize)
{
    mAllocatedResourceCounts[ResourceTypeIndex(resourceType)]++;
    mAllocatedResourceDeviceMemory[ResourceTypeIndex(resourceType)] += memorySize;
}

void ResourceManager11::decrResource(ResourceType resourceType, size_t memorySize)
{
    ASSERT(mAllocatedResourceCounts[ResourceTypeIndex(resourceType)] > 0);
    mAllocatedResourceCounts[ResourceTypeIndex(resourceType)]--;
    ASSERT(mAllocatedResourceDeviceMemory[ResourceTypeIndex(resourceType)] >= memorySize);
    mAllocatedResourceDeviceMemory[ResourceTypeIndex(resourceType)] -= memorySize;
}

void ResourceManager11::onReleaseResource(ResourceType resourceType, ID3D11Resource *resource)
{
    ASSERT(resource);
    decrResource(resourceType, ComputeGenericMemoryUsage(resourceType, resource));
}

template <>
void ResourceManager11::onRelease(ID3D11Resource *resource)
{
    // For untyped ID3D11Resource, they must call onReleaseResource.
    UNREACHABLE();
}

template <typename T>
void ResourceManager11::onRelease(T *resource)
{
    ASSERT(resource);

    GetDescFromD3D11<T> desc;
    resource->GetDesc(&desc);
    decrResource(GetResourceTypeFromD3D11<T>(), ComputeMemoryUsage(&desc));
}

#define ANGLE_INSTANTIATE_OP(NAME, RESTYPE, D3D11TYPE, DESCTYPE, INITDATATYPE) \
    \
template gl::Error                                                             \
    ResourceManager11::allocate(\
Renderer11 *,                                                                  \
                                \
const DESCTYPE *,                                                              \
                                \
INITDATATYPE *,                                                                \
                                \
Resource11<D3D11TYPE> *);                                                      \
    \
\
template void                                                                  \
    ResourceManager11::onRelease(D3D11TYPE *);

ANGLE_RESOURCE_TYPE_OP(Instantitate, ANGLE_INSTANTIATE_OP)

}  // namespace rx

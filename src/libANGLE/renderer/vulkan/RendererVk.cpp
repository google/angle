//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererVk.cpp:
//    Implements the class methods for RendererVk.
//

#include "libANGLE/renderer/vulkan/RendererVk.h"

#include "common/debug.h"

namespace rx
{

RendererVk::RendererVk() : mCapsInitialized(false), mInstance(nullptr)
{
}

RendererVk::~RendererVk()
{
    vkDestroyInstance(mInstance, nullptr);
}

vk::Error RendererVk::initialize()
{
    VkApplicationInfo applicationInfo;
    applicationInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pNext              = nullptr;
    applicationInfo.pApplicationName   = "ANGLE";
    applicationInfo.applicationVersion = 1;
    applicationInfo.pEngineName        = "ANGLE";
    applicationInfo.engineVersion      = 1;
    applicationInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo;
    instanceInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext            = nullptr;
    instanceInfo.flags            = 0;
    instanceInfo.pApplicationInfo = &applicationInfo;

    // TODO(jmadill): Layers and extensions.
    instanceInfo.enabledExtensionCount   = 0;
    instanceInfo.ppEnabledExtensionNames = nullptr;
    instanceInfo.enabledLayerCount       = 0;
    instanceInfo.ppEnabledLayerNames     = nullptr;

    ANGLE_VK_TRY(vkCreateInstance(&instanceInfo, nullptr, &mInstance));

    return vk::NoError();
}

void RendererVk::ensureCapsInitialized() const
{
    if (!mCapsInitialized)
    {
        generateCaps(&mNativeCaps, &mNativeTextureCaps, &mNativeExtensions, &mNativeLimitations);
        mCapsInitialized = true;
    }
}

void RendererVk::generateCaps(gl::Caps * /*outCaps*/,
                              gl::TextureCapsMap * /*outTextureCaps*/,
                              gl::Extensions * /*outExtensions*/,
                              gl::Limitations * /* outLimitations */) const
{
    // TODO(jmadill): Caps.
}

const gl::Caps &RendererVk::getNativeCaps() const
{
    ensureCapsInitialized();
    return mNativeCaps;
}

const gl::TextureCapsMap &RendererVk::getNativeTextureCaps() const
{
    ensureCapsInitialized();
    return mNativeTextureCaps;
}

const gl::Extensions &RendererVk::getNativeExtensions() const
{
    ensureCapsInitialized();
    return mNativeExtensions;
}

const gl::Limitations &RendererVk::getNativeLimitations() const
{
    ensureCapsInitialized();
    return mNativeLimitations;
}

}  // namespace rx

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererVk.h:
//    Defines the class interface for RendererVk.
//

#ifndef LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_
#define LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

#include <vulkan/vulkan.h>
#include <memory>

#include "common/PoolAlloc.h"
#include "common/angleutils.h"
#include "libANGLE/BlobCache.h"
#include "libANGLE/Caps.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/QueryVk.h"
#include "libANGLE/renderer/vulkan/UtilsVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders_autogen.h"

namespace egl
{
class Display;
class BlobCache;
}  // namespace egl

namespace rx
{
class DisplayVk;
class FramebufferVk;

namespace vk
{
struct Format;
}

class RendererVk : angle::NonCopyable
{
  public:
    RendererVk();
    ~RendererVk();

    angle::Result initialize(DisplayVk *displayVk,
                             egl::Display *display,
                             const char *wsiExtension,
                             const char *wsiLayer);
    void onDestroy(vk::Context *context);

    void notifyDeviceLost();
    bool isDeviceLost() const;

    std::string getVendorString() const;
    std::string getRendererDescription() const;

    gl::Version getMaxSupportedESVersion() const;

    VkInstance getInstance() const { return mInstance; }
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    const VkPhysicalDeviceProperties &getPhysicalDeviceProperties() const
    {
        return mPhysicalDeviceProperties;
    }
    const VkPhysicalDeviceFeatures &getPhysicalDeviceFeatures() const
    {
        return mPhysicalDeviceFeatures;
    }
    VkQueue getQueue() const { return mQueue; }
    VkDevice getDevice() const { return mDevice; }

    angle::Result selectPresentQueueForSurface(DisplayVk *displayVk,
                                               VkSurfaceKHR surface,
                                               uint32_t *presentQueueOut);

    const gl::Caps &getNativeCaps() const;
    const gl::TextureCapsMap &getNativeTextureCaps() const;
    const gl::Extensions &getNativeExtensions() const;
    const gl::Limitations &getNativeLimitations() const;
    uint32_t getMaxUniformBlocks() const;
    uint32_t getMaxActiveTextures() const;

    uint32_t getQueueFamilyIndex() const { return mCurrentQueueFamilyIndex; }

    const vk::MemoryProperties &getMemoryProperties() const { return mMemoryProperties; }

    // TODO(jmadill): We could pass angle::FormatID here.
    const vk::Format &getFormat(GLenum internalFormat) const
    {
        return mFormatTable[internalFormat];
    }

    const vk::Format &getFormat(angle::FormatID formatID) const { return mFormatTable[formatID]; }

    // Queries the descriptor set layout cache. Creates the layout if not present.
    angle::Result getDescriptorSetLayout(
        vk::Context *context,
        const vk::DescriptorSetLayoutDesc &desc,
        vk::BindingPointer<vk::DescriptorSetLayout> *descriptorSetLayoutOut);

    // Queries the pipeline layout cache. Creates the layout if not present.
    angle::Result getPipelineLayout(vk::Context *context,
                                    const vk::PipelineLayoutDesc &desc,
                                    const vk::DescriptorSetLayoutPointerArray &descriptorSetLayouts,
                                    vk::BindingPointer<vk::PipelineLayout> *pipelineLayoutOut);

    angle::Result syncPipelineCacheVk(DisplayVk *displayVk);

    // Issues a new serial for linked shader modules. Used in the pipeline cache.
    Serial issueShaderSerial();

    const angle::FeaturesVk &getFeatures() const
    {
        ASSERT(mFeaturesInitialized);
        return mFeatures;
    }

    bool isMockICDEnabled() const { return mEnableMockICD; }

    const vk::PipelineCache &getPipelineCache() const { return mPipelineCache; }

    // Query the format properties for select bits (linearTilingFeatures, optimalTilingFeatures and
    // bufferFeatures).  Looks through mandatory features first, and falls back to querying the
    // device (first time only).
    bool hasLinearImageFormatFeatureBits(VkFormat format, const VkFormatFeatureFlags featureBits);
    VkFormatFeatureFlags getImageFormatFeatureBits(VkFormat format,
                                                   const VkFormatFeatureFlags featureBits);
    bool hasImageFormatFeatureBits(VkFormat format, const VkFormatFeatureFlags featureBits);
    bool hasBufferFormatFeatureBits(VkFormat format, const VkFormatFeatureFlags featureBits);

    angle::Result queueSubmit(vk::Context *context,
                              const VkSubmitInfo &submitInfo,
                              const vk::Fence &fence);
    angle::Result queueWaitIdle(vk::Context *context);
    VkResult queuePresent(const VkPresentInfoKHR &presentInfo);

    Serial nextSerial();

    void addGarbage(vk::Shared<vk::Fence> &&fence, std::vector<vk::GarbageObjectBase> &&garbage);
    void addGarbage(std::vector<vk::Shared<vk::Fence>> &&fences,
                    std::vector<vk::GarbageObjectBase> &&garbage);

    static constexpr size_t kMaxExtensionNames = 200;
    using ExtensionNameList = angle::FixedVector<const char *, kMaxExtensionNames>;

  private:
    angle::Result initializeDevice(DisplayVk *displayVk, uint32_t queueFamilyIndex);
    void ensureCapsInitialized() const;

    void initFeatures(const ExtensionNameList &extensions);
    void initPipelineCacheVkKey();
    angle::Result initPipelineCache(DisplayVk *display);

    template <VkFormatFeatureFlags VkFormatProperties::*features>
    VkFormatFeatureFlags getFormatFeatureBits(VkFormat format,
                                              const VkFormatFeatureFlags featureBits);

    template <VkFormatFeatureFlags VkFormatProperties::*features>
    bool hasFormatFeatureBits(VkFormat format, const VkFormatFeatureFlags featureBits);

    angle::Result cleanupGarbage(vk::Context *context, bool block);

    egl::Display *mDisplay;

    mutable bool mCapsInitialized;
    mutable gl::Caps mNativeCaps;
    mutable gl::TextureCapsMap mNativeTextureCaps;
    mutable gl::Extensions mNativeExtensions;
    mutable gl::Limitations mNativeLimitations;
    mutable bool mFeaturesInitialized;
    mutable angle::FeaturesVk mFeatures;

    VkInstance mInstance;
    bool mEnableValidationLayers;
    bool mEnableMockICD;
    VkDebugUtilsMessengerEXT mDebugUtilsMessenger;
    VkDebugReportCallbackEXT mDebugReportCallback;
    VkPhysicalDevice mPhysicalDevice;
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures mPhysicalDeviceFeatures;
    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    VkQueue mQueue;
    uint32_t mCurrentQueueFamilyIndex;
    uint32_t mMaxVertexAttribDivisor;
    VkDevice mDevice;
    AtomicSerialFactory mQueueSerialFactory;
    AtomicSerialFactory mShaderSerialFactory;
    Serial mCurrentQueueSerial;

    bool mDeviceLost;

    using FencedGarbage =
        std::pair<std::vector<vk::Shared<vk::Fence>>, std::vector<vk::GarbageObjectBase>>;
    std::vector<FencedGarbage> mFencedGarbage;

    vk::MemoryProperties mMemoryProperties;
    vk::FormatTable mFormatTable;

    vk::PipelineCache mPipelineCache;
    egl::BlobCache::Key mPipelineCacheVkBlobKey;
    uint32_t mPipelineCacheVkUpdateTimeout;

    // A cache of VkFormatProperties as queried from the device over time.
    std::array<VkFormatProperties, vk::kNumVkFormats> mFormatProperties;

    // ANGLE uses a PipelineLayout cache to store compatible pipeline layouts.
    PipelineLayoutCache mPipelineLayoutCache;

    // DescriptorSetLayouts are also managed in a cache.
    DescriptorSetLayoutCache mDescriptorSetLayoutCache;
};

uint32_t GetUniformBufferDescriptorCount();

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

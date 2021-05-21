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

#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "common/PackedEnums.h"
#include "common/PoolAlloc.h"
#include "common/angleutils.h"
#include "common/vulkan/vk_headers.h"
#include "common/vulkan/vulkan_icd.h"
#include "libANGLE/BlobCache.h"
#include "libANGLE/Caps.h"
#include "libANGLE/WorkerThread.h"
#include "libANGLE/renderer/vulkan/CommandProcessor.h"
#include "libANGLE/renderer/vulkan/DebugAnnotatorVk.h"
#include "libANGLE/renderer/vulkan/QueryVk.h"
#include "libANGLE/renderer/vulkan/ResourceVk.h"
#include "libANGLE/renderer/vulkan/UtilsVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders_autogen.h"
#include "libANGLE/renderer/vulkan/vk_mem_alloc_wrapper.h"

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

static constexpr size_t kMaxExtensionNames = 400;
using ExtensionNameList                    = angle::FixedVector<const char *, kMaxExtensionNames>;

// Process GPU memory reports
class MemoryReport final : angle::NonCopyable
{
  public:
    MemoryReport();
    void processCallback(const VkDeviceMemoryReportCallbackDataEXT &callbackData, bool logCallback);
    void logMemoryReportStats() const;

  private:
    struct MemorySizes
    {
        VkDeviceSize allocatedMemory;
        VkDeviceSize allocatedMemoryMax;
        VkDeviceSize importedMemory;
        VkDeviceSize importedMemoryMax;
    };
    mutable std::mutex mMemoryReportMutex;
    VkDeviceSize mCurrentTotalAllocatedMemory;
    VkDeviceSize mMaxTotalAllocatedMemory;
    angle::HashMap<VkObjectType, MemorySizes> mSizesPerType;
    VkDeviceSize mCurrentTotalImportedMemory;
    VkDeviceSize mMaxTotalImportedMemory;
    angle::HashMap<uint64_t, int> mUniqueIDCounts;
};
}  // namespace vk

// Supports one semaphore from current surface, and one semaphore passed to
// glSignalSemaphoreEXT.
using SignalSemaphoreVector = angle::FixedVector<VkSemaphore, 2>;

inline void CollectGarbage(std::vector<vk::GarbageObject> *garbageOut) {}

template <typename ArgT, typename... ArgsT>
void CollectGarbage(std::vector<vk::GarbageObject> *garbageOut, ArgT object, ArgsT... objectsIn)
{
    if (object->valid())
    {
        garbageOut->emplace_back(vk::GarbageObject::Get(object));
    }
    CollectGarbage(garbageOut, objectsIn...);
}

class WaitableCompressEvent
{
  public:
    WaitableCompressEvent(std::shared_ptr<angle::WaitableEvent> waitableEvent)
        : mWaitableEvent(waitableEvent)
    {}

    virtual ~WaitableCompressEvent() {}

    void wait() { return mWaitableEvent->wait(); }

    bool isReady() { return mWaitableEvent->isReady(); }

    virtual bool getResult() = 0;

  private:
    std::shared_ptr<angle::WaitableEvent> mWaitableEvent;
};

class RendererVk : angle::NonCopyable
{
  public:
    RendererVk();
    ~RendererVk();

    angle::Result initialize(DisplayVk *displayVk,
                             egl::Display *display,
                             const char *wsiExtension,
                             const char *wsiLayer);
    // Reload volk vk* function ptrs if needed for an already initialized RendererVk
    void reloadVolkIfNeeded() const;
    void onDestroy(vk::Context *context);

    void notifyDeviceLost();
    bool isDeviceLost() const;
    bool hasSharedGarbage();
    void releaseSharedResources(vk::ResourceUseList *resourceList);

    std::string getVendorString() const;
    std::string getRendererDescription() const;
    std::string getVersionString() const;

    gl::Version getMaxSupportedESVersion() const;
    gl::Version getMaxConformantESVersion() const;

    VkInstance getInstance() const { return mInstance; }
    VkPhysicalDevice getPhysicalDevice() const { return mPhysicalDevice; }
    const VkPhysicalDeviceProperties &getPhysicalDeviceProperties() const
    {
        return mPhysicalDeviceProperties;
    }
    const VkPhysicalDeviceSubgroupProperties &getPhysicalDeviceSubgroupProperties() const
    {
        return mSubgroupProperties;
    }
    const VkPhysicalDeviceFeatures &getPhysicalDeviceFeatures() const
    {
        return mPhysicalDeviceFeatures;
    }
    VkDevice getDevice() const { return mDevice; }

    const vk::Allocator &getAllocator() const { return mAllocator; }

    angle::Result selectPresentQueueForSurface(DisplayVk *displayVk,
                                               VkSurfaceKHR surface,
                                               uint32_t *presentQueueOut);

    const gl::Caps &getNativeCaps() const;
    const gl::TextureCapsMap &getNativeTextureCaps() const;
    const gl::Extensions &getNativeExtensions() const;
    const gl::Limitations &getNativeLimitations() const;

    uint32_t getQueueFamilyIndex() const { return mCurrentQueueFamilyIndex; }
    const VkQueueFamilyProperties &getQueueFamilyProperties() const
    {
        return mQueueFamilyProperties[mCurrentQueueFamilyIndex];
    }

    const vk::MemoryProperties &getMemoryProperties() const { return mMemoryProperties; }

    const vk::Format &getFormat(GLenum internalFormat) const
    {
        return mFormatTable[internalFormat];
    }

    const vk::Format &getFormat(angle::FormatID formatID) const { return mFormatTable[formatID]; }

    angle::Result getPipelineCacheSize(DisplayVk *displayVk, size_t *pipelineCacheSizeOut);
    angle::Result syncPipelineCacheVk(DisplayVk *displayVk, const gl::Context *context);

    // Issues a new serial for linked shader modules. Used in the pipeline cache.
    Serial issueShaderSerial();

    const angle::FeaturesVk &getFeatures() const { return mFeatures; }
    uint32_t getMaxVertexAttribDivisor() const { return mMaxVertexAttribDivisor; }
    VkDeviceSize getMaxVertexAttribStride() const { return mMaxVertexAttribStride; }

    VkDeviceSize getMinImportedHostPointerAlignment() const
    {
        return mMinImportedHostPointerAlignment;
    }
    uint32_t getDefaultUniformBufferSize() const { return mDefaultUniformBufferSize; }

    bool isMockICDEnabled() const { return mEnabledICD == angle::vk::ICD::Mock; }

    // Query the format properties for select bits (linearTilingFeatures, optimalTilingFeatures and
    // bufferFeatures).  Looks through mandatory features first, and falls back to querying the
    // device (first time only).
    bool hasLinearImageFormatFeatureBits(angle::FormatID format,
                                         const VkFormatFeatureFlags featureBits) const;
    VkFormatFeatureFlags getLinearImageFormatFeatureBits(
        angle::FormatID format,
        const VkFormatFeatureFlags featureBits) const;
    VkFormatFeatureFlags getImageFormatFeatureBits(angle::FormatID format,
                                                   const VkFormatFeatureFlags featureBits) const;
    bool hasImageFormatFeatureBits(angle::FormatID format,
                                   const VkFormatFeatureFlags featureBits) const;
    bool hasBufferFormatFeatureBits(angle::FormatID format,
                                    const VkFormatFeatureFlags featureBits) const;

    ANGLE_INLINE egl::ContextPriority getDriverPriority(egl::ContextPriority priority)
    {
        return mPriorities[priority];
    }

    // This command buffer should be submitted immediately via queueSubmitOneOff.
    angle::Result getCommandBufferOneOff(vk::Context *context,
                                         vk::PrimaryCommandBuffer *commandBufferOut);

    // Fire off a single command buffer immediately with default priority.
    // Command buffer must be allocated with getCommandBufferOneOff and is reclaimed.
    angle::Result queueSubmitOneOff(vk::Context *context,
                                    vk::PrimaryCommandBuffer &&primary,
                                    egl::ContextPriority priority,
                                    const vk::Fence *fence,
                                    vk::SubmitPolicy submitPolicy,
                                    Serial *serialOut);

    template <typename... ArgsT>
    void collectGarbageAndReinit(vk::SharedResourceUse *use, ArgsT... garbageIn)
    {
        std::vector<vk::GarbageObject> sharedGarbage;
        CollectGarbage(&sharedGarbage, garbageIn...);
        if (!sharedGarbage.empty())
        {
            collectGarbage(std::move(*use), std::move(sharedGarbage));
        }
        else
        {
            // Force releasing "use" even if no garbage was created.
            use->release();
        }
        // Keep "use" valid.
        use->init();
    }

    void collectGarbage(vk::SharedResourceUse &&use, std::vector<vk::GarbageObject> &&sharedGarbage)
    {
        if (!sharedGarbage.empty())
        {
            std::lock_guard<std::mutex> lock(mGarbageMutex);
            mSharedGarbage.emplace_back(std::move(use), std::move(sharedGarbage));
        }
    }

    angle::Result getPipelineCache(vk::PipelineCache **pipelineCache);
    void onNewGraphicsPipeline()
    {
        std::lock_guard<std::mutex> lock(mPipelineCacheMutex);
        mPipelineCacheDirty = true;
    }

    void onNewValidationMessage(const std::string &message);
    std::string getAndClearLastValidationMessage(uint32_t *countSinceLastClear);

    uint64_t getMaxFenceWaitTimeNs() const;

    ANGLE_INLINE Serial getCurrentQueueSerial()
    {
        if (mFeatures.asyncCommandQueue.enabled)
        {
            return mCommandProcessor.getCurrentQueueSerial();
        }
        else
        {
            std::lock_guard<std::mutex> lock(mCommandQueueMutex);
            return mCommandQueue.getCurrentQueueSerial();
        }
    }

    ANGLE_INLINE Serial getLastSubmittedQueueSerial()
    {
        if (mFeatures.asyncCommandQueue.enabled)
        {
            return mCommandProcessor.getLastSubmittedQueueSerial();
        }
        else
        {
            std::lock_guard<std::mutex> lock(mCommandQueueMutex);
            return mCommandQueue.getLastSubmittedQueueSerial();
        }
    }

    ANGLE_INLINE Serial getLastCompletedQueueSerial()
    {
        if (mFeatures.asyncCommandQueue.enabled)
        {
            return mCommandProcessor.getLastCompletedQueueSerial();
        }
        else
        {
            std::lock_guard<std::mutex> lock(mCommandQueueMutex);
            return mCommandQueue.getLastCompletedQueueSerial();
        }
    }

    egl::Display *getDisplay() const { return mDisplay; }

    VkResult getLastPresentResult(VkSwapchainKHR swapchain)
    {
        return mCommandProcessor.getLastPresentResult(swapchain);
    }

    bool enableDebugUtils() const { return mEnableDebugUtils; }
    bool angleDebuggerMode() const { return mAngleDebuggerMode; }

    SamplerCache &getSamplerCache() { return mSamplerCache; }
    SamplerYcbcrConversionCache &getYuvConversionCache() { return mYuvConversionCache; }
    vk::ActiveHandleCounter &getActiveHandleCounts() { return mActiveHandleCounts; }

    bool getEnableValidationLayers() const { return mEnableValidationLayers; }

    vk::ResourceSerialFactory &getResourceSerialFactory() { return mResourceSerialFactory; }

    void setGlobalDebugAnnotator();

    void outputVmaStatString();

    bool haveSameFormatFeatureBits(angle::FormatID formatID1, angle::FormatID formatID2) const;

    angle::Result cleanupGarbage(Serial lastCompletedQueueSerial);
    void cleanupCompletedCommandsGarbage();

    angle::Result submitFrame(vk::Context *context,
                              egl::ContextPriority contextPriority,
                              std::vector<VkSemaphore> &&waitSemaphores,
                              std::vector<VkPipelineStageFlags> &&waitSemaphoreStageMasks,
                              const vk::Semaphore *signalSemaphore,
                              std::vector<vk::ResourceUseList> &&resourceUseLists,
                              vk::GarbageList &&currentGarbage,
                              vk::CommandPool *commandPool);

    void handleDeviceLost();
    angle::Result finishToSerial(vk::Context *context, Serial serial);
    angle::Result waitForSerialWithUserTimeout(vk::Context *context,
                                               Serial serial,
                                               uint64_t timeout,
                                               VkResult *result);
    angle::Result finish(vk::Context *context);
    angle::Result checkCompletedCommands(vk::Context *context);

    angle::Result flushRenderPassCommands(vk::Context *context,
                                          const vk::RenderPass &renderPass,
                                          vk::CommandBufferHelper **renderPassCommands);
    angle::Result flushOutsideRPCommands(vk::Context *context,
                                         vk::CommandBufferHelper **outsideRPCommands);

    VkResult queuePresent(vk::Context *context,
                          egl::ContextPriority priority,
                          const VkPresentInfoKHR &presentInfo);

    vk::CommandBufferHelper *getCommandBufferHelper(bool hasRenderPass);
    void recycleCommandBufferHelper(vk::CommandBufferHelper *commandBuffer);

    // Process GPU memory reports
    void processMemoryReportCallback(const VkDeviceMemoryReportCallbackDataEXT &callbackData)
    {
        bool logCallback = getFeatures().logMemoryReportCallbacks.enabled;
        mMemoryReport.processCallback(callbackData, logCallback);
    }

    // Accumulate cache stats for a specific cache
    void accumulateCacheStats(VulkanCacheType cache, const CacheStats &stats)
    {
        mVulkanCacheStats[cache].accumulate(stats);
    }
    // Log cache stats for all caches
    void logCacheStats() const;

    VkPipelineStageFlags getSupportedVulkanPipelineStageMask() const
    {
        return mSupportedVulkanPipelineStageMask;
    }

  private:
    angle::Result initializeDevice(DisplayVk *displayVk, uint32_t queueFamilyIndex);
    void ensureCapsInitialized() const;

    void queryDeviceExtensionFeatures(const vk::ExtensionNameList &deviceExtensionNames);

    void initFeatures(DisplayVk *display, const vk::ExtensionNameList &extensions);
    angle::Result initPipelineCache(DisplayVk *display,
                                    vk::PipelineCache *pipelineCache,
                                    bool *success);

    template <VkFormatFeatureFlags VkFormatProperties::*features>
    VkFormatFeatureFlags getFormatFeatureBits(angle::FormatID formatID,
                                              const VkFormatFeatureFlags featureBits) const;

    template <VkFormatFeatureFlags VkFormatProperties::*features>
    bool hasFormatFeatureBits(angle::FormatID formatID,
                              const VkFormatFeatureFlags featureBits) const;

    egl::Display *mDisplay;

    mutable bool mCapsInitialized;
    mutable gl::Caps mNativeCaps;
    mutable gl::TextureCapsMap mNativeTextureCaps;
    mutable gl::Extensions mNativeExtensions;
    mutable gl::Limitations mNativeLimitations;
    mutable angle::FeaturesVk mFeatures;

    VkInstance mInstance;
    bool mEnableValidationLayers;
    // True if ANGLE is enabling the VK_EXT_debug_utils extension.
    bool mEnableDebugUtils;
    // True if ANGLE should call the vkCmd*DebugUtilsLabelEXT functions in order to communicate to
    // debuggers (e.g. AGI) the OpenGL ES commands that the application uses.  This is independent
    // of mEnableDebugUtils, as an external graphics debugger can enable the VK_EXT_debug_utils
    // extension and cause this to be set true.
    bool mAngleDebuggerMode;
    angle::vk::ICD mEnabledICD;
    VkDebugUtilsMessengerEXT mDebugUtilsMessenger;
    VkDebugReportCallbackEXT mDebugReportCallback;
    VkPhysicalDevice mPhysicalDevice;
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures mPhysicalDeviceFeatures;
    VkPhysicalDeviceLineRasterizationFeaturesEXT mLineRasterizationFeatures;
    VkPhysicalDeviceProvokingVertexFeaturesEXT mProvokingVertexFeatures;
    VkPhysicalDeviceVertexAttributeDivisorFeaturesEXT mVertexAttributeDivisorFeatures;
    VkPhysicalDeviceVertexAttributeDivisorPropertiesEXT mVertexAttributeDivisorProperties;
    VkPhysicalDeviceTransformFeedbackFeaturesEXT mTransformFeedbackFeatures;
    VkPhysicalDeviceIndexTypeUint8FeaturesEXT mIndexTypeUint8Features;
    VkPhysicalDeviceSubgroupProperties mSubgroupProperties;
    VkPhysicalDeviceDeviceMemoryReportFeaturesEXT mMemoryReportFeatures;
    VkDeviceDeviceMemoryReportCreateInfoEXT mMemoryReportCallback;
    VkPhysicalDeviceExternalMemoryHostPropertiesEXT mExternalMemoryHostProperties;
    VkPhysicalDeviceShaderFloat16Int8FeaturesKHR mShaderFloat16Int8Features;
    VkPhysicalDeviceDepthStencilResolvePropertiesKHR mDepthStencilResolveProperties;
    VkPhysicalDeviceMultisampledRenderToSingleSampledFeaturesEXT
        mMultisampledRenderToSingleSampledFeatures;
    VkPhysicalDeviceDriverPropertiesKHR mDriverProperties;
    VkExternalFenceProperties mExternalFenceProperties;
    VkExternalSemaphoreProperties mExternalSemaphoreProperties;
    VkPhysicalDeviceSamplerYcbcrConversionFeatures mSamplerYcbcrConversionFeatures;
    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    angle::PackedEnumMap<egl::ContextPriority, egl::ContextPriority> mPriorities;
    uint32_t mCurrentQueueFamilyIndex;
    uint32_t mMaxVertexAttribDivisor;
    VkDeviceSize mMaxVertexAttribStride;
    VkDeviceSize mMinImportedHostPointerAlignment;
    uint32_t mDefaultUniformBufferSize;
    VkDevice mDevice;
    AtomicSerialFactory mShaderSerialFactory;

    bool mDeviceLost;

    std::mutex mGarbageMutex;
    vk::SharedGarbageList mSharedGarbage;

    vk::MemoryProperties mMemoryProperties;
    vk::FormatTable mFormatTable;

    // All access to the pipeline cache is done through EGL objects so it is thread safe to not use
    // a lock.
    std::mutex mPipelineCacheMutex;
    vk::PipelineCache mPipelineCache;
    uint32_t mPipelineCacheVkUpdateTimeout;
    bool mPipelineCacheDirty;
    bool mPipelineCacheInitialized;

    // A cache of VkFormatProperties as queried from the device over time.
    mutable angle::FormatMap<VkFormatProperties> mFormatProperties;

    // Latest validation data for debug overlay.
    std::string mLastValidationMessage;
    uint32_t mValidationMessageCount;

    DebugAnnotatorVk mAnnotator;

    // How close to VkPhysicalDeviceLimits::maxMemoryAllocationCount we allow ourselves to get
    static constexpr double kPercentMaxMemoryAllocationCount = 0.3;
    // How many objects to garbage collect before issuing a flush()
    uint32_t mGarbageCollectionFlushThreshold;

    // Only used for "one off" command buffers.
    vk::CommandPool mOneOffCommandPool;

    struct PendingOneOffCommands
    {
        Serial serial;
        vk::PrimaryCommandBuffer commandBuffer;
    };
    std::deque<PendingOneOffCommands> mPendingOneOffCommands;

    std::mutex mCommandQueueMutex;
    vk::CommandQueue mCommandQueue;

    // Command buffer pool management.
    std::mutex mCommandBufferHelperFreeListMutex;
    std::vector<vk::CommandBufferHelper *> mCommandBufferHelperFreeList;

    // Async Command Queue
    vk::CommandProcessor mCommandProcessor;

    vk::Allocator mAllocator;
    SamplerCache mSamplerCache;
    SamplerYcbcrConversionCache mYuvConversionCache;
    vk::ActiveHandleCounter mActiveHandleCounts;

    // Tracks resource serials.
    vk::ResourceSerialFactory mResourceSerialFactory;

    // Process GPU memory reports
    vk::MemoryReport mMemoryReport;

    // Stats about all Vulkan object caches
    using VulkanCacheStats = angle::PackedEnumMap<VulkanCacheType, CacheStats>;
    VulkanCacheStats mVulkanCacheStats;

    // A mask to filter out Vulkan pipeline stages that are not supported, applied in situations
    // where multiple stages are prespecified (for example with image layout transitions):
    //
    // - Excludes GEOMETRY if geometry shaders are not supported.
    // - Excludes TESSELLATION_CONTROL and TESSELLATION_EVALUATION if tessellation shaders are not
    //   supported.
    //
    // Note that this mask can have bits set that don't correspond to valid stages, so it's strictly
    // only useful for masking out unsupported stages in an otherwise valid set of stages.
    VkPipelineStageFlags mSupportedVulkanPipelineStageMask;

    // Use thread pool to compress cache data.
    std::shared_ptr<rx::WaitableCompressEvent> mCompressEvent;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

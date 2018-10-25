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

#include "common/angleutils.h"
#include "libANGLE/BlobCache.h"
#include "libANGLE/Caps.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/FeaturesVk.h"
#include "libANGLE/renderer/vulkan/QueryVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_internal_shaders.h"

namespace egl
{
class AttributeMap;
class BlobCache;
}

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
                             const egl::AttributeMap &attribs,
                             const char *wsiName);
    void onDestroy(vk::Context *context);

    void markDeviceLost();
    bool isDeviceLost() const;

    std::string getVendorString() const;
    std::string getRendererDescription() const;

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

    angle::Result finish(vk::Context *context);
    angle::Result flush(vk::Context *context);

    const vk::CommandPool &getCommandPool() const;

    const gl::Caps &getNativeCaps() const;
    const gl::TextureCapsMap &getNativeTextureCaps() const;
    const gl::Extensions &getNativeExtensions() const;
    const gl::Limitations &getNativeLimitations() const;
    uint32_t getMaxActiveTextures();

    Serial getCurrentQueueSerial() const { return mCurrentQueueSerial; }
    Serial getLastSubmittedQueueSerial() const { return mLastSubmittedQueueSerial; }
    Serial getLastCompletedQueueSerial() const { return mLastCompletedQueueSerial; }

    bool isSerialInUse(Serial serial) const;

    template <typename T>
    void releaseObject(Serial resourceSerial, T *object)
    {
        if (!isSerialInUse(resourceSerial))
        {
            object->destroy(mDevice);
        }
        else
        {
            object->dumpResources(resourceSerial, &mGarbage);
        }
    }

    // Check to see which batches have finished completion (forward progress for
    // mLastCompletedQueueSerial, for example for when the application busy waits on a query
    // result).
    angle::Result checkCompletedCommands(vk::Context *context);
    // Wait for completion of batches until (at least) batch with given serial is finished.
    angle::Result finishToSerial(vk::Context *context, Serial serial);

    uint32_t getQueueFamilyIndex() const { return mCurrentQueueFamilyIndex; }

    const vk::MemoryProperties &getMemoryProperties() const { return mMemoryProperties; }

    // TODO(jmadill): We could pass angle::FormatID here.
    const vk::Format &getFormat(GLenum internalFormat) const
    {
        return mFormatTable[internalFormat];
    }

    const vk::Format &getFormat(angle::FormatID formatID) const { return mFormatTable[formatID]; }

    angle::Result getCompatibleRenderPass(vk::Context *context,
                                          const vk::RenderPassDesc &desc,
                                          vk::RenderPass **renderPassOut);
    angle::Result getRenderPassWithOps(vk::Context *context,
                                       const vk::RenderPassDesc &desc,
                                       const vk::AttachmentOpsArray &ops,
                                       vk::RenderPass **renderPassOut);

    // For getting a vk::Pipeline and checking the pipeline cache.
    angle::Result getPipeline(vk::Context *context,
                              const vk::ShaderAndSerial &vertexShader,
                              const vk::ShaderAndSerial &fragmentShader,
                              const vk::PipelineLayout &pipelineLayout,
                              const vk::PipelineDesc &pipelineDesc,
                              const gl::AttributesMask &activeAttribLocationsMask,
                              vk::PipelineAndSerial **pipelineOut);

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

    vk::DynamicSemaphorePool *getDynamicSemaphorePool() { return &mSubmitSemaphorePool; }

    // Request a semaphore, that is expected to be signaled externally.  The next submission will
    // wait on it.
    angle::Result allocateSubmitWaitSemaphore(vk::Context *context,
                                              const vk::Semaphore **outSemaphore);
    // Get the last signaled semaphore to wait on externally.  The semaphore will not be waited on
    // by next submission.
    const vk::Semaphore *getSubmitLastSignaledSemaphore(vk::Context *context);

    // This should only be called from ResourceVk.
    // TODO(jmadill): Keep in ContextVk to enable threaded rendering.
    vk::CommandGraph *getCommandGraph();

    // Issues a new serial for linked shader modules. Used in the pipeline cache.
    Serial issueShaderSerial();

    vk::ShaderLibrary *getShaderLibrary();
    const FeaturesVk &getFeatures() const { return mFeatures; }

    angle::Result getTimestamp(vk::Context *context, uint64_t *timestampOut);

    // Create Begin/End/Instant GPU trace events, which take their timestamps from GPU queries.
    // The events are queued until the query results are available.  Possible values for `phase`
    // are TRACE_EVENT_PHASE_*
    ANGLE_INLINE angle::Result traceGpuEvent(vk::Context *context,
                                             vk::CommandBuffer *commandBuffer,
                                             char phase,
                                             const char *name)
    {
        if (mGpuEventsEnabled)
            return traceGpuEventImpl(context, commandBuffer, phase, name);
        return angle::Result::Continue();
    }

  private:
    // Number of semaphores for external entities to renderer to issue a wait, such as surface's
    // image acquire.
    static constexpr size_t kMaxExternalSemaphores = 8;
    // Total possible number of semaphores a submission can wait on.  +1 is for the semaphore
    // signaled in the last submission.
    static constexpr size_t kMaxWaitSemaphores = kMaxExternalSemaphores + 1;

    angle::Result initializeDevice(DisplayVk *displayVk, uint32_t queueFamilyIndex);
    void ensureCapsInitialized() const;
    void getSubmitWaitSemaphores(
        vk::Context *context,
        angle::FixedVector<VkSemaphore, kMaxWaitSemaphores> *waitSemaphores,
        angle::FixedVector<VkPipelineStageFlags, kMaxWaitSemaphores> *waitStageMasks);
    angle::Result submitFrame(vk::Context *context,
                              const VkSubmitInfo &submitInfo,
                              vk::CommandBuffer &&commandBuffer);
    void freeAllInFlightResources();
    angle::Result flushCommandGraph(vk::Context *context, vk::CommandBuffer *commandBatch);
    void initFeatures();
    void initPipelineCacheVkKey();
    angle::Result initPipelineCacheVk(DisplayVk *display);

    angle::Result synchronizeCpuGpuTime(vk::Context *context);
    angle::Result traceGpuEventImpl(vk::Context *context,
                                    vk::CommandBuffer *commandBuffer,
                                    char phase,
                                    const char *name);
    angle::Result checkCompletedGpuEvents(vk::Context *context);
    void flushGpuEvents(double nextSyncGpuTimestampS, double nextSyncCpuTimestampS);

    mutable bool mCapsInitialized;
    mutable gl::Caps mNativeCaps;
    mutable gl::TextureCapsMap mNativeTextureCaps;
    mutable gl::Extensions mNativeExtensions;
    mutable gl::Limitations mNativeLimitations;
    mutable FeaturesVk mFeatures;

    VkInstance mInstance;
    bool mEnableValidationLayers;
    VkDebugReportCallbackEXT mDebugReportCallback;
    VkPhysicalDevice mPhysicalDevice;
    VkPhysicalDeviceProperties mPhysicalDeviceProperties;
    VkPhysicalDeviceFeatures mPhysicalDeviceFeatures;
    std::vector<VkQueueFamilyProperties> mQueueFamilyProperties;
    VkQueue mQueue;
    uint32_t mCurrentQueueFamilyIndex;
    VkDevice mDevice;
    vk::CommandPool mCommandPool;
    SerialFactory mQueueSerialFactory;
    SerialFactory mShaderSerialFactory;
    Serial mLastCompletedQueueSerial;
    Serial mLastSubmittedQueueSerial;
    Serial mCurrentQueueSerial;

    bool mDeviceLost;

    struct CommandBatch final : angle::NonCopyable
    {
        CommandBatch();
        ~CommandBatch();
        CommandBatch(CommandBatch &&other);
        CommandBatch &operator=(CommandBatch &&other);

        void destroy(VkDevice device);

        vk::CommandPool commandPool;
        vk::Fence fence;
        Serial serial;
    };

    std::vector<CommandBatch> mInFlightCommands;
    std::vector<vk::GarbageObject> mGarbage;
    vk::MemoryProperties mMemoryProperties;
    vk::FormatTable mFormatTable;

    RenderPassCache mRenderPassCache;
    PipelineCache mPipelineCache;

    vk::PipelineCache mPipelineCacheVk;
    egl::BlobCache::Key mPipelineCacheVkBlobKey;
    uint32_t mPipelineCacheVkUpdateTimeout;

    // mSubmitWaitSemaphores is a list of specifically requested semaphores to be waited on before a
    // command buffer submission, for example, semaphores signaled by vkAcquireNextImageKHR.
    // After first use, the list is automatically cleared.  This is a vector to support concurrent
    // rendering to multiple surfaces.
    //
    // Note that with multiple contexts present, this may result in a context waiting on image
    // acquisition even if it doesn't render to that surface.  If CommandGraphs are separated by
    // context or share group for example, this could be moved to the one that actually uses the
    // image.
    angle::FixedVector<vk::SemaphoreHelper, kMaxExternalSemaphores> mSubmitWaitSemaphores;
    // mSubmitLastSignaledSemaphore shows which semaphore was last signaled by submission.  This can
    // be set to nullptr if retrieved to be waited on outside RendererVk, such
    // as by the surface before presentation.  Each submission waits on the
    // previously signaled semaphore (as well as any in mSubmitWaitSemaphores)
    // and allocates a new semaphore to signal.
    vk::SemaphoreHelper mSubmitLastSignaledSemaphore;

    // A pool of semaphores used to support the aforementioned mid-frame submissions.
    vk::DynamicSemaphorePool mSubmitSemaphorePool;

    // See CommandGraph.h for a desription of the Command Graph.
    vk::CommandGraph mCommandGraph;

    // ANGLE uses a PipelineLayout cache to store compatible pipeline layouts.
    PipelineLayoutCache mPipelineLayoutCache;

    // DescriptorSetLayouts are also managed in a cache.
    DescriptorSetLayoutCache mDescriptorSetLayoutCache;

    // Internal shader library.
    vk::ShaderLibrary mShaderLibrary;

    // The GpuEventQuery struct holds together a timestamp query and enough data to create a
    // trace event based on that. Use traceGpuEvent to insert such queries.  They will be readback
    // when the results are available, without inserting a GPU bubble.
    //
    // - eventName will be the reported name of the event
    // - phase is either 'B' (duration begin), 'E' (duration end) or 'i' (instant // event).
    //   See Google's "Trace Event Format":
    //   https://docs.google.com/document/d/1CvAClvFfyA5R-PhYUmn5OOQtYMH4h6I0nSsKchNAySU
    // - serial is the serial of the batch the query was submitted on.  Until the batch is
    //   submitted, the query is not checked to avoid incuring a flush.
    struct GpuEventQuery final
    {
        const char *name;
        char phase;

        uint32_t queryIndex;
        size_t queryPoolIndex;

        Serial serial;
    };

    // Once a query result is available, the timestamp is read and a GpuEvent object is kept until
    // the next clock sync, at which point the clock drift is compensated in the results before
    // handing them off to the application.
    struct GpuEvent final
    {
        uint64_t gpuTimestampCycles;
        const char *name;
        char phase;
    };

    bool mGpuEventsEnabled;
    vk::DynamicQueryPool mGpuEventQueryPool;
    // A list of queries that have yet to be turned into an event (their result is not yet
    // available).
    std::vector<GpuEventQuery> mInFlightGpuEventQueries;
    // A list of gpu events since the last clock sync.
    std::vector<GpuEvent> mGpuEvents;

    // Hold information from the last gpu clock sync for future gpu-to-cpu timestamp conversions.
    struct GpuClockSyncInfo
    {
        double gpuTimestampS;
        double cpuTimestampS;
    };
    GpuClockSyncInfo mGpuClockSync;

    // The very first timestamp queried for a GPU event is used as origin, so event timestamps would
    // have a value close to zero, to avoid losing 12 bits when converting these 64 bit values to
    // double.
    uint64_t mGpuEventTimestampOrigin;
};

uint32_t GetUniformBufferDescriptorCount();

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

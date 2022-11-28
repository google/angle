//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandProcessor.h:
//    A class to process and submit Vulkan command buffers that can be
//    used in an asynchronous worker thread.
//

#ifndef LIBANGLE_RENDERER_VULKAN_COMMAND_PROCESSOR_H_
#define LIBANGLE_RENDERER_VULKAN_COMMAND_PROCESSOR_H_

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "common/vulkan/vk_headers.h"
#include "libANGLE/renderer/vulkan/PersistentCommandPool.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
class RendererVk;
class CommandProcessor;

namespace vk
{
enum class SubmitPolicy
{
    AllowDeferred,
    EnsureSubmitted,
};

class FenceRecycler
{
  public:
    FenceRecycler() {}
    ~FenceRecycler() {}
    void destroy(Context *context);

    angle::Result newSharedFence(Context *context, Shared<Fence> *sharedFenceOut);
    inline void resetSharedFence(Shared<Fence> *sharedFenceIn)
    {
        std::lock_guard<std::mutex> lock(mMutex);
        sharedFenceIn->resetAndRecycle(&mRecyler);
    }

  private:
    std::mutex mMutex;
    Recycler<Fence> mRecyler;
};

enum class CustomTask
{
    Invalid = 0,
    // Process SecondaryCommandBuffer commands into the primary CommandBuffer.
    ProcessOutsideRenderPassCommands,
    ProcessRenderPassCommands,
    // End the current command buffer and submit commands to the queue
    FlushAndQueueSubmit,
    // Submit custom command buffer, excludes some state management
    OneOffQueueSubmit,
    // Execute QueuePresent
    Present,
    // do cleanup processing on completed commands
    // TODO: https://issuetracker.google.com/170312581 - should be able to remove
    // checkCompletedCommands command with fence refactor.
    CheckCompletedCommands,
    // Exit the command processor thread
    Exit,
};

// CommandProcessorTask interface
class CommandProcessorTask
{
  public:
    CommandProcessorTask() { initTask(); }

    void initTask();

    void initTask(CustomTask command) { mTask = command; }

    void initOutsideRenderPassProcessCommands(bool hasProtectedContent,
                                              OutsideRenderPassCommandBufferHelper *commandBuffer);

    void initRenderPassProcessCommands(bool hasProtectedContent,
                                       RenderPassCommandBufferHelper *commandBuffer,
                                       const RenderPass *renderPass);

    void initPresent(egl::ContextPriority priority, const VkPresentInfoKHR &presentInfo);

    void initFlushAndQueueSubmit(const std::vector<VkSemaphore> &waitSemaphores,
                                 const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
                                 const VkSemaphore semaphore,
                                 bool hasProtectedContent,
                                 egl::ContextPriority priority,
                                 SecondaryCommandPools *commandPools,
                                 SecondaryCommandBufferList &&commandBuffersToReset,
                                 const QueueSerial &submitQueueSerial);

    void initOneOffQueueSubmit(VkCommandBuffer commandBufferHandle,
                               bool hasProtectedContent,
                               egl::ContextPriority priority,
                               const Semaphore *waitSemaphore,
                               VkPipelineStageFlags waitSemaphoreStageMask,
                               const Fence *fence,
                               const QueueSerial &submitQueueSerial);

    CommandProcessorTask &operator=(CommandProcessorTask &&rhs);

    CommandProcessorTask(CommandProcessorTask &&other) : CommandProcessorTask()
    {
        *this = std::move(other);
    }

    const QueueSerial &getSubmitQueueSerial() const { return mSubmitQueueSerial; }
    CustomTask getTaskCommand() { return mTask; }
    std::vector<VkSemaphore> &getWaitSemaphores() { return mWaitSemaphores; }
    std::vector<VkPipelineStageFlags> &getWaitSemaphoreStageMasks()
    {
        return mWaitSemaphoreStageMasks;
    }
    VkSemaphore getSemaphore() { return mSemaphore; }
    SecondaryCommandBufferList &&getCommandBuffersToReset()
    {
        return std::move(mCommandBuffersToReset);
    }
    egl::ContextPriority getPriority() const { return mPriority; }
    bool hasProtectedContent() const { return mHasProtectedContent; }
    VkCommandBuffer getOneOffCommandBufferVk() const { return mOneOffCommandBufferVk; }
    const Semaphore *getOneOffWaitSemaphore() { return mOneOffWaitSemaphore; }
    VkPipelineStageFlags getOneOffWaitSemaphoreStageMask() { return mOneOffWaitSemaphoreStageMask; }
    const Fence *getOneOffFence() { return mOneOffFence; }
    const VkPresentInfoKHR &getPresentInfo() const { return mPresentInfo; }
    const RenderPass *getRenderPass() const { return mRenderPass; }
    OutsideRenderPassCommandBufferHelper *getOutsideRenderPassCommandBuffer() const
    {
        return mOutsideRenderPassCommandBuffer;
    }
    RenderPassCommandBufferHelper *getRenderPassCommandBuffer() const
    {
        return mRenderPassCommandBuffer;
    }
    SecondaryCommandPools *getCommandPools() const { return mCommandPools; }

  private:
    void copyPresentInfo(const VkPresentInfoKHR &other);

    CustomTask mTask;

    // ProcessCommands
    OutsideRenderPassCommandBufferHelper *mOutsideRenderPassCommandBuffer;
    RenderPassCommandBufferHelper *mRenderPassCommandBuffer;
    const RenderPass *mRenderPass;

    // Flush data
    std::vector<VkSemaphore> mWaitSemaphores;
    std::vector<VkPipelineStageFlags> mWaitSemaphoreStageMasks;
    VkSemaphore mSemaphore;
    SecondaryCommandPools *mCommandPools;
    SecondaryCommandBufferList mCommandBuffersToReset;

    // Flush command data
    QueueSerial mSubmitQueueSerial;

    // Present command data
    VkPresentInfoKHR mPresentInfo;
    VkSwapchainKHR mSwapchain;
    VkSemaphore mWaitSemaphore;
    uint32_t mImageIndex;
    // Used by Present if supportsIncrementalPresent is enabled
    VkPresentRegionKHR mPresentRegion;
    VkPresentRegionsKHR mPresentRegions;
    std::vector<VkRectLayerKHR> mRects;

    VkSwapchainPresentFenceInfoEXT mPresentFenceInfo;
    VkFence mPresentFence;

    // Used by OneOffQueueSubmit
    VkCommandBuffer mOneOffCommandBufferVk;
    const Semaphore *mOneOffWaitSemaphore;
    VkPipelineStageFlags mOneOffWaitSemaphoreStageMask;
    const Fence *mOneOffFence;

    // Flush, Present & QueueWaitIdle data
    egl::ContextPriority mPriority;
    bool mHasProtectedContent;
};

struct CommandBatch final : angle::NonCopyable
{
    CommandBatch();
    ~CommandBatch();
    CommandBatch(CommandBatch &&other);
    CommandBatch &operator=(CommandBatch &&other);

    void destroy(VkDevice device);
    void resetSecondaryCommandBuffers(VkDevice device);

    PrimaryCommandBuffer primaryCommands;
    // commandPools is for secondary CommandBuffer allocation
    SecondaryCommandPools *commandPools;
    SecondaryCommandBufferList commandBuffersToReset;
    Shared<Fence> fence;
    QueueSerial queueSerial;
    bool hasProtectedContent;
};

class DeviceQueueMap;

class QueueFamily final : angle::NonCopyable
{
  public:
    static const uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

    static uint32_t FindIndex(const std::vector<VkQueueFamilyProperties> &queueFamilyProperties,
                              VkQueueFlags flags,
                              int32_t matchNumber,  // 0 = first match, 1 = second match ...
                              uint32_t *matchCount);
    static const uint32_t kQueueCount = static_cast<uint32_t>(egl::ContextPriority::EnumCount);
    static const float kQueuePriorities[static_cast<uint32_t>(egl::ContextPriority::EnumCount)];

    QueueFamily() : mProperties{}, mIndex(kInvalidIndex) {}
    ~QueueFamily() {}

    void initialize(const VkQueueFamilyProperties &queueFamilyProperties, uint32_t index);
    bool valid() const { return (mIndex != kInvalidIndex); }
    uint32_t getIndex() const { return mIndex; }
    const VkQueueFamilyProperties *getProperties() const { return &mProperties; }
    bool isGraphics() const { return ((mProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) > 0); }
    bool isCompute() const { return ((mProperties.queueFlags & VK_QUEUE_COMPUTE_BIT) > 0); }
    bool supportsProtected() const
    {
        return ((mProperties.queueFlags & VK_QUEUE_PROTECTED_BIT) > 0);
    }
    uint32_t getDeviceQueueCount() const { return mProperties.queueCount; }

    DeviceQueueMap initializeQueueMap(VkDevice device,
                                      bool makeProtected,
                                      uint32_t queueIndex,
                                      uint32_t queueCount);

  private:
    VkQueueFamilyProperties mProperties;
    uint32_t mIndex;

    void getDeviceQueue(VkDevice device, bool makeProtected, uint32_t queueIndex, VkQueue *queue);
};

class DeviceQueueMap : public angle::PackedEnumMap<egl::ContextPriority, VkQueue>
{
    friend QueueFamily;

  public:
    DeviceQueueMap() : mIndex(QueueFamily::kInvalidIndex), mIsProtected(false) {}
    DeviceQueueMap(uint32_t queueFamilyIndex, bool isProtected)
        : mIndex(queueFamilyIndex), mIsProtected(isProtected)
    {}
    DeviceQueueMap(const DeviceQueueMap &other) = default;
    ~DeviceQueueMap();
    DeviceQueueMap &operator=(const DeviceQueueMap &other);

    bool valid() const { return (mIndex != QueueFamily::kInvalidIndex); }
    uint32_t getIndex() const { return mIndex; }
    bool isProtected() const { return mIsProtected; }
    egl::ContextPriority getDevicePriority(egl::ContextPriority priority) const;

  private:
    uint32_t mIndex;
    bool mIsProtected;
    angle::PackedEnumMap<egl::ContextPriority, egl::ContextPriority> mPriorities;
};

class CommandQueueInterface : angle::NonCopyable
{
  public:
    virtual ~CommandQueueInterface() {}

    virtual angle::Result init(Context *context, const DeviceQueueMap &queueMap) = 0;
    virtual void destroy(Context *context)                                       = 0;

    virtual void handleDeviceLost(RendererVk *renderer) = 0;

    // Wait until the desired serial has been completed.
    virtual angle::Result finishResourceUse(Context *context,
                                            const ResourceUse &use,
                                            uint64_t timeout)          = 0;
    virtual angle::Result waitIdle(Context *context, uint64_t timeout) = 0;
    virtual angle::Result submitCommands(
        Context *context,
        bool hasProtectedContent,
        egl::ContextPriority priority,
        const std::vector<VkSemaphore> &waitSemaphores,
        const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
        const VkSemaphore signalSemaphore,
        SecondaryCommandBufferList &&commandBuffersToReset,
        SecondaryCommandPools *commandPools,
        const QueueSerial &submitQueueSerial)                                     = 0;
    virtual angle::Result queueSubmitOneOff(Context *context,
                                            bool hasProtectedContent,
                                            egl::ContextPriority contextPriority,
                                            VkCommandBuffer commandBufferHandle,
                                            const Semaphore *waitSemaphore,
                                            VkPipelineStageFlags waitSemaphoreStageMask,
                                            const Fence *fence,
                                            SubmitPolicy submitPolicy,
                                            const QueueSerial &submitQueueSerial) = 0;
    virtual VkResult queuePresent(egl::ContextPriority contextPriority,
                                  const VkPresentInfoKHR &presentInfo)            = 0;

    virtual angle::Result waitForResourceUseToFinishWithUserTimeout(Context *context,
                                                                    const ResourceUse &use,
                                                                    uint64_t timeout,
                                                                    VkResult *result) = 0;

    // Check to see which batches have finished completion (forward progress for
    // the last completed serial, for example for when the application busy waits on a query
    // result). It would be nice if we didn't have to expose this for QueryVk::getResult.
    virtual angle::Result checkCompletedCommands(Context *context) = 0;

    virtual angle::Result flushOutsideRPCommands(
        Context *context,
        bool hasProtectedContent,
        OutsideRenderPassCommandBufferHelper **outsideRPCommands) = 0;
    virtual angle::Result flushRenderPassCommands(
        Context *context,
        bool hasProtectedContent,
        const RenderPass &renderPass,
        RenderPassCommandBufferHelper **renderPassCommands) = 0;

    // For correct synchronization with external, in particular when asked to signal an external
    // semaphore, we need to ensure that there are no pending submissions.
    virtual angle::Result ensureNoPendingWork(Context *context) = 0;

    virtual bool isBusy() const = 0;
};

class CommandQueue final : public CommandQueueInterface
{
  public:
    CommandQueue();
    ~CommandQueue() override;

    angle::Result init(Context *context, const DeviceQueueMap &queueMap) override;
    void destroy(Context *context) override;

    void handleDeviceLost(RendererVk *renderer) override;

    angle::Result finishQueueSerial(Context *context,
                                    const QueueSerial &queueSerial,
                                    uint64_t timeout);
    angle::Result finishResourceUse(Context *context,
                                    const ResourceUse &use,
                                    uint64_t timeout) override;
    angle::Result waitIdle(Context *context, uint64_t timeout) override;

    angle::Result submitCommands(Context *context,
                                 bool hasProtectedContent,
                                 egl::ContextPriority priority,
                                 const std::vector<VkSemaphore> &waitSemaphores,
                                 const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
                                 const VkSemaphore signalSemaphore,
                                 SecondaryCommandBufferList &&commandBuffersToReset,
                                 SecondaryCommandPools *commandPools,
                                 const QueueSerial &submitQueueSerial) override;

    angle::Result queueSubmitOneOff(Context *context,
                                    bool hasProtectedContent,
                                    egl::ContextPriority contextPriority,
                                    VkCommandBuffer commandBufferHandle,
                                    const Semaphore *waitSemaphore,
                                    VkPipelineStageFlags waitSemaphoreStageMask,
                                    const Fence *fence,
                                    SubmitPolicy submitPolicy,
                                    const QueueSerial &submitQueueSerial) override;

    VkResult queuePresent(egl::ContextPriority contextPriority,
                          const VkPresentInfoKHR &presentInfo) override;

    angle::Result waitForResourceUseToFinishWithUserTimeout(Context *context,
                                                            const ResourceUse &use,
                                                            uint64_t timeout,
                                                            VkResult *result) override;

    angle::Result checkCompletedCommands(Context *context) override;

    angle::Result flushOutsideRPCommands(
        Context *context,
        bool hasProtectedContent,
        OutsideRenderPassCommandBufferHelper **outsideRPCommands) override;
    angle::Result flushRenderPassCommands(
        Context *context,
        bool hasProtectedContent,
        const RenderPass &renderPass,
        RenderPassCommandBufferHelper **renderPassCommands) override;

    angle::Result ensureNoPendingWork(Context *context) override { return angle::Result::Continue; }

    bool isBusy() const override;

    angle::Result queueSubmit(Context *context,
                              egl::ContextPriority contextPriority,
                              const VkSubmitInfo &submitInfo,
                              const Fence *fence,
                              const QueueSerial &submitQueueSerial);

    egl::ContextPriority getDriverPriority(egl::ContextPriority priority)
    {
        return mQueueMap.getDevicePriority(priority);
    }
    uint32_t getDeviceQueueIndex() const { return mQueueMap.getIndex(); }

    VkQueue getQueue(egl::ContextPriority priority) { return mQueueMap[priority]; }

    const angle::VulkanPerfCounters &getPerfCounters() const { return mPerfCounters; }
    void resetPerFramePerfCounters();

    // The ResourceUse still have unfinished queue serial by ANGLE or vulkan.
    bool hasUnfinishedUse(const ResourceUse &use) const;
    // The ResourceUse still have queue serial not yet submitted to vulkan.
    bool hasUnsubmittedUse(const ResourceUse &use) const;
    Serial getLastSubmittedSerial(SerialIndex index) const { return mLastSubmittedSerials[index]; }

  private:
    void releaseToCommandBatch(bool hasProtectedContent,
                               PrimaryCommandBuffer &&commandBuffer,
                               SecondaryCommandPools *commandPools,
                               CommandBatch *batch);
    angle::Result retireFinishedCommands(Context *context, size_t finishedCount);
    angle::Result retireFinishedCommandsAndCleanupGarbage(Context *context, size_t finishedCount);
    angle::Result ensurePrimaryCommandBufferValid(Context *context, bool hasProtectedContent);

    size_t getBatchCountUpToSerials(RendererVk *renderer,
                                    const Serials &serials,
                                    Shared<Fence> **fenceToWaitOnOut);

    // For validation only. Should only be called with ASSERT macro.
    bool allInFlightCommandsAreAfterSerials(const Serials &serials);

    PrimaryCommandBuffer &getCommandBuffer(bool hasProtectedContent)
    {
        if (hasProtectedContent)
        {
            return mProtectedPrimaryCommands;
        }
        else
        {
            return mPrimaryCommands;
        }
    }

    PersistentCommandPool &getCommandPool(bool hasProtectedContent)
    {
        if (hasProtectedContent)
        {
            return mProtectedPrimaryCommandPool;
        }
        else
        {
            return mPrimaryCommandPool;
        }
    }

    std::vector<CommandBatch> mInFlightCommands;

    // Keeps a free list of reusable primary command buffers.
    PrimaryCommandBuffer mPrimaryCommands;
    PersistentCommandPool mPrimaryCommandPool;
    PrimaryCommandBuffer mProtectedPrimaryCommands;
    PersistentCommandPool mProtectedPrimaryCommandPool;

    // Queue serial management.
    AtomicQueueSerialFixedArray mLastSubmittedSerials;
    // This queue serial can be read/write from different threads, so we need to use atomic
    // operations to access the underline value. Since we only do load/store on this value, it
    // should be just a normal uint64_t load/store on most platforms.
    AtomicQueueSerialFixedArray mLastCompletedSerials;

    // QueueMap
    DeviceQueueMap mQueueMap;

    FenceRecycler mFenceRecycler;

    angle::VulkanPerfCounters mPerfCounters;
};

// CommandProcessor is used to dispatch work to the GPU when the asyncCommandQueue feature is
// enabled. Issuing the |destroy| command will cause the worker thread to clean up it's resources
// and shut down. This command is sent when the renderer instance shuts down. Tasks are defined by
// the CommandQueue interface.

class CommandProcessor final : public Context, public CommandQueueInterface
{
  public:
    CommandProcessor(RendererVk *renderer);
    ~CommandProcessor() override;

    VkResult getLastPresentResult(VkSwapchainKHR swapchain)
    {
        return getLastAndClearPresentResult(swapchain);
    }

    // Context
    void handleError(VkResult result,
                     const char *file,
                     const char *function,
                     unsigned int line) override;

    // CommandQueueInterface
    angle::Result init(Context *context, const DeviceQueueMap &queueMap) override;

    void destroy(Context *context) override;

    void handleDeviceLost(RendererVk *renderer) override;

    angle::Result finishQueueSerial(Context *context,
                                    const QueueSerial &queueSerial,
                                    uint64_t timeout);
    angle::Result finishResourceUse(Context *context,
                                    const ResourceUse &use,
                                    uint64_t timeout) override;

    angle::Result waitIdle(Context *context, uint64_t timeout) override;

    angle::Result submitCommands(Context *context,
                                 bool hasProtectedContent,
                                 egl::ContextPriority priority,
                                 const std::vector<VkSemaphore> &waitSemaphores,
                                 const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
                                 const VkSemaphore signalSemaphore,
                                 SecondaryCommandBufferList &&commandBuffersToReset,
                                 SecondaryCommandPools *commandPools,
                                 const QueueSerial &submitQueueSerial) override;

    angle::Result queueSubmitOneOff(Context *context,
                                    bool hasProtectedContent,
                                    egl::ContextPriority contextPriority,
                                    VkCommandBuffer commandBufferHandle,
                                    const Semaphore *waitSemaphore,
                                    VkPipelineStageFlags waitSemaphoreStageMask,
                                    const Fence *fence,
                                    SubmitPolicy submitPolicy,
                                    const QueueSerial &submitQueueSerial) override;
    VkResult queuePresent(egl::ContextPriority contextPriority,
                          const VkPresentInfoKHR &presentInfo) override;

    angle::Result waitForResourceUseToFinishWithUserTimeout(Context *context,
                                                            const ResourceUse &use,
                                                            uint64_t timeout,
                                                            VkResult *result) override;

    angle::Result checkCompletedCommands(Context *context) override;

    angle::Result flushOutsideRPCommands(
        Context *context,
        bool hasProtectedContent,
        OutsideRenderPassCommandBufferHelper **outsideRPCommands) override;
    angle::Result flushRenderPassCommands(
        Context *context,
        bool hasProtectedContent,
        const RenderPass &renderPass,
        RenderPassCommandBufferHelper **renderPassCommands) override;

    angle::Result ensureNoPendingWork(Context *context) override;

    bool isBusy() const override;

    egl::ContextPriority getDriverPriority(egl::ContextPriority priority)
    {
        return mCommandQueue.getDriverPriority(priority);
    }
    uint32_t getDeviceQueueIndex() const { return mCommandQueue.getDeviceQueueIndex(); }
    VkQueue getQueue(egl::ContextPriority priority) { return mCommandQueue.getQueue(priority); }

    // Note that due to inheritance from Context, this class has a set of perf counters as well,
    // but currently only the counters in the member command queue are of interest.
    const angle::VulkanPerfCounters &getPerfCounters() const
    {
        return mCommandQueue.getPerfCounters();
    }
    void resetPerFramePerfCounters() { mCommandQueue.resetPerFramePerfCounters(); }

    ANGLE_INLINE bool hasUnfinishedUse(const ResourceUse &use) const
    {
        return mCommandQueue.hasUnfinishedUse(use);
    }

    bool hasUnsubmittedUse(const ResourceUse &use) const;
    Serial getLastSubmittedSerial(SerialIndex index) const { return mLastSubmittedSerials[index]; }

  private:
    bool hasPendingError() const
    {
        std::lock_guard<std::mutex> queueLock(mErrorMutex);
        return !mErrors.empty();
    }
    angle::Result checkAndPopPendingError(Context *errorHandlingContext);

    // Entry point for command processor thread, calls processTasksImpl to do the
    // work. called by RendererVk::initializeDevice on main thread
    void processTasks();

    // Called asynchronously from main thread to queue work that is then processed by the worker
    // thread
    void queueCommand(CommandProcessorTask &&task);

    // Command processor thread, called by processTasks. The loop waits for work to
    // be submitted from a separate thread.
    angle::Result processTasksImpl(bool *exitThread);

    // Command processor thread, process a task
    angle::Result processTask(CommandProcessorTask *task);

    VkResult getLastAndClearPresentResult(VkSwapchainKHR swapchain);
    VkResult present(egl::ContextPriority priority, const VkPresentInfoKHR &presentInfo);

    // Used by main thread to wait for worker thread to complete all outstanding work.
    angle::Result waitForWorkComplete(Context *context);

    std::queue<CommandProcessorTask> mTasks;
    mutable std::mutex mWorkerMutex;
    // Signal worker thread when work is available
    std::condition_variable mWorkAvailableCondition;
    // Signal main thread when all work completed
    mutable std::condition_variable mWorkerIdleCondition;
    // Track worker thread Idle state for assertion purposes
    bool mWorkerThreadIdle;
    CommandQueue mCommandQueue;

    // Tracks last serial that was submitted to command processor. Note: this maybe different from
    // mLastSubmittedQueueSerial in CommandQueue since submission from CommandProcessor to
    // CommandQueue occur in a separate thread.
    AtomicQueueSerialFixedArray mLastSubmittedSerials;

    mutable std::mutex mErrorMutex;
    std::queue<Error> mErrors;

    // Track present info
    std::mutex mSwapchainStatusMutex;
    std::condition_variable mSwapchainStatusCondition;
    std::map<VkSwapchainKHR, VkResult> mSwapchainStatus;

    // Command queue worker thread.
    std::thread mTaskThread;
};

}  // namespace vk

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_PROCESSOR_H_

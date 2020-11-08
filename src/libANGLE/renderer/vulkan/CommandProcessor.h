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
// CommandProcessor is used to dispatch work to the GPU when commandProcessor feature is true.
// If asynchronousCommandProcessing is enabled the work will be queued and handled by a worker
// thread asynchronous to the context. Issuing the CustomTask::Exit command will cause the worker
// thread to clean up it's resources and shut down. This command is sent when the renderer instance
// shuts down. Custom tasks are:

enum CustomTask
{
    Invalid = 0,
    // Process SecondaryCommandBuffer commands into the primary CommandBuffer.
    ProcessCommands,
    // End the current command buffer and submit commands to the queue
    FlushAndQueueSubmit,
    // Submit custom command buffer, excludes some state management
    OneOffQueueSubmit,
    // Finish queue commands up to given serial value, process garbage
    FinishToSerial,
    // Execute QueuePresent
    Present,
    // do cleanup processing on completed commands
    // TODO: https://issuetracker.google.com/170312581 - should be able to remove
    // checkCompletedCommands command with fence refactor.
    CheckCompletedCommands,
    // Exit the command processor thread
    Exit,
};

class CommandProcessorTask
{
  public:
    CommandProcessorTask() { initTask(); }

    void initTask();

    void initTask(CustomTask command) { mTask = command; }

    void initProcessCommands(CommandBufferHelper *commandBuffer, const RenderPass *renderPass);

    void initPresent(egl::ContextPriority priority, const VkPresentInfoKHR &presentInfo);

    void initFinishToSerial(Serial serial);

    void initFlushAndQueueSubmit(std::vector<VkSemaphore> &&waitSemaphores,
                                 std::vector<VkPipelineStageFlags> &&waitSemaphoreStageMasks,
                                 const Semaphore *semaphore,
                                 egl::ContextPriority priority,
                                 GarbageList &&currentGarbage,
                                 Serial submitQueueSerial);

    void initOneOffQueueSubmit(VkCommandBuffer oneOffCommandBufferVk,
                               egl::ContextPriority priority,
                               const Fence *fence,
                               Serial submitQueueSerial);

    CommandProcessorTask &operator=(CommandProcessorTask &&rhs);

    CommandProcessorTask(CommandProcessorTask &&other) : CommandProcessorTask()
    {
        *this = std::move(other);
    }

    void setQueueSerial(Serial serial) { mSerial = serial; }
    Serial getQueueSerial() const { return mSerial; }
    CustomTask getTaskCommand() { return mTask; }
    std::vector<VkSemaphore> &getWaitSemaphores() { return mWaitSemaphores; }
    std::vector<VkPipelineStageFlags> &getWaitSemaphoreStageMasks()
    {
        return mWaitSemaphoreStageMasks;
    }
    const Semaphore *getSemaphore() { return mSemaphore; }
    GarbageList &getGarbage() { return mGarbage; }
    egl::ContextPriority getPriority() const { return mPriority; }
    const VkCommandBuffer &getOneOffCommandBufferVk() const { return mOneOffCommandBufferVk; }
    const Fence *getOneOffFence() { return mOneOffFence; }
    const VkPresentInfoKHR &getPresentInfo() const { return mPresentInfo; }
    const RenderPass *getRenderPass() const { return mRenderPass; }
    CommandBufferHelper *getCommandBuffer() const { return mCommandBuffer; }

  private:
    void copyPresentInfo(const VkPresentInfoKHR &other);

    CustomTask mTask;

    // ProcessCommands
    const RenderPass *mRenderPass;
    CommandBufferHelper *mCommandBuffer;

    // Flush data
    std::vector<VkSemaphore> mWaitSemaphores;
    std::vector<VkPipelineStageFlags> mWaitSemaphoreStageMasks;
    const Semaphore *mSemaphore;
    GarbageList mGarbage;

    // FinishToSerial & Flush command data
    Serial mSerial;

    // Present command data
    VkPresentInfoKHR mPresentInfo;
    VkSwapchainKHR mSwapchain;
    VkSemaphore mWaitSemaphore;
    uint32_t mImageIndex;
    // Used by Present if supportsIncrementalPresent is enabled
    VkPresentRegionKHR mPresentRegion;
    VkPresentRegionsKHR mPresentRegions;
    std::vector<VkRectLayerKHR> mRects;

    // Used by OneOffQueueSubmit
    VkCommandBuffer mOneOffCommandBufferVk;
    const Fence *mOneOffFence;

    // Flush, Present & QueueWaitIdle data
    egl::ContextPriority mPriority;
};

struct CommandBatch final : angle::NonCopyable
{
    CommandBatch();
    ~CommandBatch();
    CommandBatch(CommandBatch &&other);
    CommandBatch &operator=(CommandBatch &&other);

    void destroy(VkDevice device);

    PrimaryCommandBuffer primaryCommands;
    // commandPool is for secondary CommandBuffer allocation
    CommandPool commandPool;
    Shared<Fence> fence;
    Serial serial;
};

using DeviceQueueMap = angle::PackedEnumMap<egl::ContextPriority, VkQueue>;

class CommandQueue final : angle::NonCopyable
{
  public:
    CommandQueue();
    ~CommandQueue();

    angle::Result init(Context *context, const DeviceQueueMap &queueMap);
    void destroy(RendererVk *renderer);
    void handleDeviceLost(RendererVk *renderer);

    void clearAllGarbage(RendererVk *renderer);

    angle::Result finishToSerial(Context *context, Serial finishSerial, uint64_t timeout);

    Serial reserveSubmitSerial();

    angle::Result submitFrame(Context *context,
                              egl::ContextPriority priority,
                              const std::vector<VkSemaphore> &waitSemaphores,
                              const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
                              const Semaphore *signalSemaphore,
                              Shared<Fence> &&sharedFence,
                              GarbageList &&currentGarbage,
                              CommandPool *commandPool,
                              Serial submitQueueSerial);

    angle::Result queueSubmit(Context *context,
                              egl::ContextPriority contextPriority,
                              const VkSubmitInfo &submitInfo,
                              const Fence *fence,
                              Serial submitQueueSerial);
    VkResult queuePresent(egl::ContextPriority contextPriority,
                          const VkPresentInfoKHR &presentInfo);

    angle::Result waitForSerialWithUserTimeout(vk::Context *context,
                                               Serial serial,
                                               uint64_t timeout,
                                               VkResult *result);

    // Check to see which batches have finished completion (forward progress for
    // mLastCompletedQueueSerial, for example for when the application busy waits on a query
    // result). It would be nice if we didn't have to expose this for QuerygetResult.
    angle::Result checkCompletedCommands(Context *context);

    angle::Result flushOutsideRPCommands(Context *context, CommandBufferHelper *outsideRPCommands);
    angle::Result flushRenderPassCommands(Context *context,
                                          const RenderPass &renderPass,
                                          CommandBufferHelper *renderPassCommands);

    ANGLE_INLINE Serial getLastSubmittedQueueSerial() const { return mLastSubmittedQueueSerial; }
    ANGLE_INLINE Serial getLastCompletedQueueSerial() const { return mLastCompletedQueueSerial; }
    ANGLE_INLINE Serial getCurrentQueueSerial() const { return mCurrentQueueSerial; }

  private:
    angle::Result releaseToCommandBatch(Context *context,
                                        PrimaryCommandBuffer &&commandBuffer,
                                        CommandPool *commandPool,
                                        CommandBatch *batch);
    angle::Result retireFinishedCommands(Context *context, size_t finishedCount);
    angle::Result ensurePrimaryCommandBufferValid(Context *context);

    bool allInFlightCommandsAreAfterSerial(Serial serial) const;

    GarbageQueue mGarbageQueue;
    std::vector<CommandBatch> mInFlightCommands;

    // Keeps a free list of reusable primary command buffers.
    PrimaryCommandBuffer mPrimaryCommands;
    PersistentCommandPool mPrimaryCommandPool;

    // Queue serial management.
    AtomicSerialFactory mQueueSerialFactory;
    Serial mLastCompletedQueueSerial;
    Serial mLastSubmittedQueueSerial;
    Serial mCurrentQueueSerial;

    // Devices queues.
    DeviceQueueMap mQueues;
};

// TODO(jmadill): Give this the same API as CommandQueue. b/172704839
class CommandProcessor : public Context
{
  public:
    CommandProcessor(RendererVk *renderer);
    ~CommandProcessor() override;

    void handleError(VkResult result,
                     const char *file,
                     const char *function,
                     unsigned int line) override;

    // Entry point for command processor thread, calls processTasksImpl to do the
    // work. called by Rendererinitialization on main thread
    void processTasks(const DeviceQueueMap &queueMap);

    // Called asynchronously from main thread to queue work that is then processed by the worker
    // thread
    void queueCommand(Context *context, CommandProcessorTask *task);

    void checkCompletedCommands(Context *context);

    // Used by main thread to wait for worker thread to complete all outstanding work.
    void waitForWorkComplete(Context *context);

    Serial getLastCompletedQueueSerial();
    Serial getLastSubmittedQueueSerial();
    Serial getCurrentQueueSerial();
    Serial reserveSubmitSerial();

    // Wait until desired serial has been processed.
    void finishToSerial(Context *context, Serial serial);

    void handleDeviceLost();

    bool hasPendingError() const
    {
        std::lock_guard<std::mutex> queueLock(mErrorMutex);
        return !mErrors.empty();
    }
    Error getAndClearPendingError();

    // Stop the command processor thread
    void shutdown(std::thread *commandProcessorThread);

    void finishAllWork(Context *context);

    VkResult getLastPresentResult(VkSwapchainKHR swapchain)
    {
        return getLastAndClearPresentResult(swapchain);
    }

  private:
    // Command processor thread, called by processTasks. The loop waits for work to
    // be submitted from a separate thread.
    angle::Result processTasksImpl(bool *exitThread);

    // Command processor thread, process a task
    angle::Result processTask(CommandProcessorTask *task);

    VkResult getLastAndClearPresentResult(VkSwapchainKHR swapchain);
    VkResult present(egl::ContextPriority priority, const VkPresentInfoKHR &presentInfo);

    std::queue<CommandProcessorTask> mTasks;
    mutable std::mutex mWorkerMutex;
    // Signal worker thread when work is available
    std::condition_variable mWorkAvailableCondition;
    // Signal main thread when all work completed
    mutable std::condition_variable mWorkerIdleCondition;
    // Track worker thread Idle state for assertion purposes
    bool mWorkerThreadIdle;
    // Command pool to allocate processor thread primary command buffers from
    CommandPool mCommandPool;
    CommandQueue mCommandQueue;

    std::mutex mQueueSerialMutex;

    mutable std::mutex mErrorMutex;
    std::queue<Error> mErrors;

    // Track present info
    std::mutex mSwapchainStatusMutex;
    std::condition_variable mSwapchainStatusCondition;
    std::map<VkSwapchainKHR, VkResult> mSwapchainStatus;
};

}  // namespace vk

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_PROCESSOR_H_

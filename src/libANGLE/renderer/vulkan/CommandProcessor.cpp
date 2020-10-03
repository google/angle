//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandProcessor.cpp:
//    Implements the class methods for CommandProcessor.
//

#include "libANGLE/renderer/vulkan/CommandProcessor.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/trace.h"

namespace rx
{
namespace
{
constexpr size_t kInFlightCommandsLimit = 100u;
constexpr bool kOutputVmaStatsString    = false;

void InitializeSubmitInfo(VkSubmitInfo *submitInfo,
                          const vk::PrimaryCommandBuffer &commandBuffer,
                          const std::vector<VkSemaphore> &waitSemaphores,
                          std::vector<VkPipelineStageFlags> *waitSemaphoreStageMasks,
                          const vk::Semaphore *signalSemaphore)
{
    // Verify that the submitInfo has been zero'd out.
    ASSERT(submitInfo->signalSemaphoreCount == 0);

    submitInfo->sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo->commandBufferCount = commandBuffer.valid() ? 1 : 0;
    submitInfo->pCommandBuffers    = commandBuffer.ptr();

    if (waitSemaphoreStageMasks->size() < waitSemaphores.size())
    {
        waitSemaphoreStageMasks->resize(waitSemaphores.size(), VK_PIPELINE_STAGE_ALL_COMMANDS_BIT);
    }

    submitInfo->waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo->pWaitSemaphores    = waitSemaphores.data();
    submitInfo->pWaitDstStageMask  = waitSemaphoreStageMasks->data();

    if (signalSemaphore)
    {
        submitInfo->signalSemaphoreCount = 1;
        submitInfo->pSignalSemaphores    = signalSemaphore->ptr();
    }
    else
    {
        submitInfo->signalSemaphoreCount = 0;
        submitInfo->pSignalSemaphores    = nullptr;
    }
}
}  // namespace

namespace vk
{
void CommandProcessorTask::initTask()
{
    mTask                  = CustomTask::Invalid;
    mContextVk             = nullptr;
    mRenderPass            = nullptr;
    mCommandBuffer         = nullptr;
    mSemaphore             = nullptr;
    mOneOffFence           = nullptr;
    mOneOffCommandBufferVk = VK_NULL_HANDLE;
}

// CommandProcessorTask implementation
void CommandProcessorTask::initProcessCommands(ContextVk *contextVk,
                                               CommandBufferHelper *commandBuffer,
                                               vk::RenderPass *renderPass)
{
    mTask          = vk::CustomTask::ProcessCommands;
    mContextVk     = contextVk;
    mCommandBuffer = commandBuffer;
    mRenderPass    = renderPass;
}

void CommandProcessorTask::initPresent(egl::ContextPriority priority, VkPresentInfoKHR presentInfo)
{
    mTask        = vk::CustomTask::Present;
    mPresentInfo = presentInfo;
    mPriority    = priority;
}

void CommandProcessorTask::initFinishToSerial(Serial serial)
{
    // Note: sometimes the serial is not valid and that's okay, the finish will early exit in the
    // TaskProcessor::finishToSerial
    mTask   = vk::CustomTask::FinishToSerial;
    mSerial = serial;
}

void CommandProcessorTask::initFlushAndQueueSubmit(
    std::vector<VkSemaphore> &&waitSemaphores,
    std::vector<VkPipelineStageFlags> &&waitSemaphoreStageMasks,
    const vk::Semaphore *semaphore,
    egl::ContextPriority priority,
    vk::GarbageList &&currentGarbage,
    vk::ResourceUseList &&currentResources)
{
    mTask                    = vk::CustomTask::FlushAndQueueSubmit;
    mWaitSemaphores          = std::move(waitSemaphores);
    mWaitSemaphoreStageMasks = std::move(waitSemaphoreStageMasks);
    mSemaphore               = semaphore;
    mGarbage                 = std::move(currentGarbage);
    mResourceUseList         = std::move(currentResources);
    mPriority                = priority;
}

void CommandProcessorTask::initOneOffQueueSubmit(VkCommandBuffer oneOffCommandBufferVk,
                                                 egl::ContextPriority priority,
                                                 const vk::Fence *fence)
{
    mTask                  = vk::CustomTask::OneOffQueueSubmit;
    mOneOffCommandBufferVk = oneOffCommandBufferVk;
    mOneOffFence           = fence;
    mPriority              = priority;
}

CommandProcessorTask &CommandProcessorTask::operator=(CommandProcessorTask &&rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    mContextVk     = rhs.mContextVk;
    mRenderPass    = rhs.mRenderPass;
    mCommandBuffer = rhs.mCommandBuffer;
    std::swap(mTask, rhs.mTask);
    std::swap(mWaitSemaphores, rhs.mWaitSemaphores);
    std::swap(mWaitSemaphoreStageMasks, rhs.mWaitSemaphoreStageMasks);
    mSemaphore   = rhs.mSemaphore;
    mOneOffFence = rhs.mOneOffFence;
    std::swap(mGarbage, rhs.mGarbage);
    std::swap(mSerial, rhs.mSerial);
    std::swap(mPresentInfo, rhs.mPresentInfo);
    std::swap(mPriority, rhs.mPriority);
    std::swap(mResourceUseList, rhs.mResourceUseList);
    mOneOffCommandBufferVk = rhs.mOneOffCommandBufferVk;

    // clear rhs now that everything has moved.
    rhs.initTask();

    return *this;
}

// CommandBatch implementation.
CommandBatch::CommandBatch() = default;

CommandBatch::~CommandBatch() = default;

CommandBatch::CommandBatch(CommandBatch &&other)
{
    *this = std::move(other);
}

CommandBatch &CommandBatch::operator=(CommandBatch &&other)
{
    std::swap(primaryCommands, other.primaryCommands);
    std::swap(commandPool, other.commandPool);
    std::swap(fence, other.fence);
    std::swap(serial, other.serial);
    return *this;
}

void CommandBatch::destroy(VkDevice device)
{
    primaryCommands.destroy(device);
    commandPool.destroy(device);
    fence.reset(device);
}

// TaskProcessor implementation.
TaskProcessor::TaskProcessor() = default;

TaskProcessor::~TaskProcessor() = default;

void TaskProcessor::destroy(VkDevice device)
{
    mPrimaryCommandPool.destroy(device);
    ASSERT(mInFlightCommands.empty() && mGarbageQueue.empty());
}

angle::Result TaskProcessor::init(vk::Context *context, std::thread::id threadId)
{
    mThreadId = threadId;

    // Initialize the command pool now that we know the queue family index.
    ANGLE_TRY(mPrimaryCommandPool.init(context, context->getRenderer()->getQueueFamilyIndex()));

    return angle::Result::Continue;
}

angle::Result TaskProcessor::checkCompletedCommands(vk::Context *context)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "TaskProcessor::checkCompletedCommands");
    VkDevice device        = context->getDevice();
    RendererVk *rendererVk = context->getRenderer();

    int finishedCount = 0;

    for (vk::CommandBatch &batch : mInFlightCommands)
    {
        VkResult result = batch.fence.get().getStatus(device);
        if (result == VK_NOT_READY)
        {
            break;
        }
        ANGLE_VK_TRY(context, result);

        rendererVk->onCompletedSerial(batch.serial);

        rendererVk->resetSharedFence(&batch.fence);

        ANGLE_TRACE_EVENT0("gpu.angle", "command buffer recycling");
        batch.commandPool.destroy(device);
        ANGLE_TRY(releasePrimaryCommandBuffer(context, std::move(batch.primaryCommands)));
        ++finishedCount;
    }

    if (finishedCount > 0)
    {
        auto beginIter = mInFlightCommands.begin();
        mInFlightCommands.erase(beginIter, beginIter + finishedCount);
    }

    Serial lastCompleted = rendererVk->getLastCompletedQueueSerial();

    size_t freeIndex = 0;
    for (; freeIndex < mGarbageQueue.size(); ++freeIndex)
    {
        vk::GarbageAndSerial &garbageList = mGarbageQueue[freeIndex];
        if (garbageList.getSerial() <= lastCompleted)
        {
            for (vk::GarbageObject &garbage : garbageList.get())
            {
                garbage.destroy(rendererVk);
            }
        }
        else
        {
            break;
        }
    }

    // Remove the entries from the garbage list - they should be ready to go.
    if (freeIndex > 0)
    {
        mGarbageQueue.erase(mGarbageQueue.begin(), mGarbageQueue.begin() + freeIndex);
    }

    return angle::Result::Continue;
}

angle::Result TaskProcessor::releaseToCommandBatch(vk::Context *context,
                                                   vk::PrimaryCommandBuffer &&commandBuffer,
                                                   vk::CommandPool *commandPool,
                                                   vk::CommandBatch *batch)
{
    batch->primaryCommands = std::move(commandBuffer);

    if (commandPool->valid())
    {
        batch->commandPool = std::move(*commandPool);
        // Recreate CommandPool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex        = context->getRenderer()->getQueueFamilyIndex();

        ANGLE_VK_TRY(context, commandPool->init(context->getDevice(), poolInfo));
    }

    return angle::Result::Continue;
}

angle::Result TaskProcessor::allocatePrimaryCommandBuffer(
    vk::Context *context,
    vk::PrimaryCommandBuffer *commandBufferOut)
{
    return mPrimaryCommandPool.allocate(context, commandBufferOut);
}

angle::Result TaskProcessor::releasePrimaryCommandBuffer(vk::Context *context,
                                                         vk::PrimaryCommandBuffer &&commandBuffer)
{
    ASSERT(mPrimaryCommandPool.valid());
    return mPrimaryCommandPool.collect(context, std::move(commandBuffer));
}

void TaskProcessor::handleDeviceLost(vk::Context *context)
{
    VkDevice device = context->getDevice();

    for (vk::CommandBatch &batch : mInFlightCommands)
    {
        // On device loss we need to wait for fence to be signaled before destroying it
        VkResult status =
            batch.fence.get().wait(device, context->getRenderer()->getMaxFenceWaitTimeNs());
        // If the wait times out, it is probably not possible to recover from lost device
        ASSERT(status == VK_SUCCESS || status == VK_ERROR_DEVICE_LOST);

        // On device lost, here simply destroy the CommandBuffer, it will be fully cleared later by
        // CommandPool::destroy
        batch.primaryCommands.destroy(device);

        batch.commandPool.destroy(device);
        batch.fence.reset(device);
    }
    mInFlightCommands.clear();
}

// If there are any inflight commands worker will look for fence that corresponds to the request
// serial or the last available fence and wait on that fence. Will then do necessary cleanup work.
// This can cause the worker thread to block.
// TODO: https://issuetracker.google.com/issues/170312581 - A more optimal solution might be to do
// the wait in CommandProcessor rather than the worker thread. That would require protecting access
// to mInFlightCommands
angle::Result TaskProcessor::finishToSerial(vk::Context *context, Serial serial)
{
    RendererVk *rendererVk = context->getRenderer();
    uint64_t timeout       = rendererVk->getMaxFenceWaitTimeNs();

    if (mInFlightCommands.empty())
    {
        // No outstanding work, nothing to wait for.
        return angle::Result::Continue;
    }

    // Find the first batch with serial equal to or bigger than given serial (note that
    // the batch serials are unique, otherwise upper-bound would have been necessary).
    size_t batchIndex = mInFlightCommands.size() - 1;
    for (size_t i = 0; i < mInFlightCommands.size(); ++i)
    {
        if (mInFlightCommands[i].serial >= serial)
        {
            batchIndex = i;
            break;
        }
    }
    const vk::CommandBatch &batch = mInFlightCommands[batchIndex];

    // Wait for it finish
    VkDevice device = context->getDevice();
    ANGLE_VK_TRY(context, batch.fence.get().wait(device, timeout));

    // Clean up finished batches.
    return checkCompletedCommands(context);
}

VkResult TaskProcessor::present(VkQueue queue, const VkPresentInfoKHR &presentInfo)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "vkQueuePresentKHR");
    return vkQueuePresentKHR(queue, &presentInfo);
}

angle::Result TaskProcessor::submitFrame(vk::Context *context,
                                         VkQueue queue,
                                         const VkSubmitInfo &submitInfo,
                                         const vk::Shared<vk::Fence> &sharedFence,
                                         vk::GarbageList *currentGarbage,
                                         vk::CommandPool *commandPool,
                                         vk::PrimaryCommandBuffer &&commandBuffer,
                                         const Serial &queueSerial)
{
    ASSERT(std::this_thread::get_id() == mThreadId);
    ANGLE_TRACE_EVENT0("gpu.angle", "TaskProcessor::submitFrame");

    VkDevice device = context->getDevice();

    vk::DeviceScoped<vk::CommandBatch> scopedBatch(device);
    vk::CommandBatch &batch = scopedBatch.get();
    batch.fence.copy(device, sharedFence);
    batch.serial = queueSerial;

    ANGLE_TRY(queueSubmit(context, queue, submitInfo, &batch.fence.get()));

    if (!currentGarbage->empty())
    {
        mGarbageQueue.emplace_back(std::move(*currentGarbage), queueSerial);
    }

    // Store the primary CommandBuffer and command pool used for secondary CommandBuffers
    // in the in-flight list.
    ANGLE_TRY(releaseToCommandBatch(context, std::move(commandBuffer), commandPool, &batch));

    mInFlightCommands.emplace_back(scopedBatch.release());

    ANGLE_TRY(checkCompletedCommands(context));

    // CPU should be throttled to avoid mInFlightCommands from growing too fast. Important for
    // off-screen scenarios.
    while (mInFlightCommands.size() > kInFlightCommandsLimit)
    {
        ANGLE_TRY(finishToSerial(context, mInFlightCommands[0].serial));
    }

    return angle::Result::Continue;
}

vk::Shared<vk::Fence> TaskProcessor::getLastSubmittedFenceWithLock(VkDevice device) const
{
    vk::Shared<vk::Fence> fence;
    // Note: this must be called when the work queue is empty and while holding mWorkerMutex to
    // ensure that worker isn't touching mInFlightCommands
    if (!mInFlightCommands.empty())
    {
        fence.copy(device, mInFlightCommands.back().fence);
    }

    return fence;
}

angle::Result TaskProcessor::queueSubmit(vk::Context *context,
                                         VkQueue queue,
                                         const VkSubmitInfo &submitInfo,
                                         const vk::Fence *fence)
{
    ASSERT(std::this_thread::get_id() == mThreadId);
    if (kOutputVmaStatsString)
    {
        context->getRenderer()->outputVmaStatString();
    }

    // Don't need a QueueMutex since all queue accesses are serialized through the worker.
    VkFence handle = fence ? fence->getHandle() : VK_NULL_HANDLE;
    ANGLE_VK_TRY(context, vkQueueSubmit(queue, 1, &submitInfo, handle));

    return angle::Result::Continue;
}

void CommandProcessor::handleError(VkResult errorCode,
                                   const char *file,
                                   const char *function,
                                   unsigned int line)
{
    ASSERT(errorCode != VK_SUCCESS);

    std::stringstream errorStream;
    errorStream << "Internal Vulkan error (" << errorCode << "): " << VulkanResultString(errorCode)
                << ".";

    if (errorCode == VK_ERROR_DEVICE_LOST)
    {
        WARN() << errorStream.str();
        handleDeviceLost();
    }

    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    vk::Error error = {errorCode, file, function, line};
    mErrors.emplace(error);
}

CommandProcessor::CommandProcessor(RendererVk *renderer)
    : vk::Context(renderer),
      mWorkerThreadIdle(false),
      mCommandProcessorLastSubmittedSerial(mQueueSerialFactory.generate()),
      mCommandProcessorCurrentQueueSerial(mQueueSerialFactory.generate())
{
    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    while (!mErrors.empty())
    {
        mErrors.pop();
    }
}

CommandProcessor::~CommandProcessor() = default;

bool CommandProcessor::isRobustResourceInitEnabled() const
{
    // Unused for worker thread, just return false.
    return false;
}

vk::Error CommandProcessor::getAndClearPendingError()
{
    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    vk::Error tmpError({VK_SUCCESS, nullptr, nullptr, 0});
    if (!mErrors.empty())
    {
        tmpError = mErrors.front();
        mErrors.pop();
    }
    return tmpError;
}

void CommandProcessor::queueCommand(vk::Context *context, vk::CommandProcessorTask *task)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::queueCommand");
    {
        // Grab the worker mutex so that we put things on the queue in the same order as we give out
        // serials.
        std::lock_guard<std::mutex> queueLock(mWorkerMutex);

        if (task->getTaskCommand() == vk::CustomTask::FlushAndQueueSubmit ||
            task->getTaskCommand() == vk::CustomTask::OneOffQueueSubmit)
        {
            std::lock_guard<std::mutex> lock(mCommandProcessorQueueSerialMutex);
            // Flush submits work, so give it the current serial and generate a new one.
            Serial queueSerial = mCommandProcessorCurrentQueueSerial;
            task->setQueueSerial(queueSerial);
            mCommandProcessorLastSubmittedSerial = mCommandProcessorCurrentQueueSerial;
            mCommandProcessorCurrentQueueSerial  = mQueueSerialFactory.generate();

            task->getResourceUseList().releaseResourceUsesAndUpdateSerials(queueSerial);
        }

        mTasks.emplace(std::move(*task));
        mWorkAvailableCondition.notify_one();
    }

    if (getRenderer()->getFeatures().asynchronousCommandProcessing.enabled)
    {
        return;
    }

    // parallel task processing disabled so wait for work to complete.
    waitForWorkComplete(context);
}

void CommandProcessor::processTasks()
{

    while (true)
    {
        bool exitThread      = false;
        angle::Result result = processTasksImpl(&exitThread);
        if (exitThread)
        {
            // We are doing a controlled exit of the thread, break out of the while loop.
            break;
        }
        if (result != angle::Result::Continue)
        {
            // TODO: https://issuetracker.google.com/issues/170311829 - follow-up on error handling
            // ContextVk::commandProcessorSyncErrorsAndQueueCommand and WindowSurfaceVk::destroy
            // do error processing, is anything required here? Don't think so, mostly need to
            // continue the worker thread until it's been told to exit.
            UNREACHABLE();
        }
    }
}

angle::Result CommandProcessor::processTasksImpl(bool *exitThread)
{
    // Initialization prior to work thread loop
    ANGLE_TRY(mTaskProcessor.init(this, std::this_thread::get_id()));
    // Allocate and begin primary command buffer
    ANGLE_TRY(mTaskProcessor.allocatePrimaryCommandBuffer(this, &mPrimaryCommandBuffer));
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo         = nullptr;

    ANGLE_VK_TRY(this, mPrimaryCommandBuffer.begin(beginInfo));

    while (true)
    {
        std::unique_lock<std::mutex> lock(mWorkerMutex);
        if (mTasks.empty())
        {
            mWorkerThreadIdle = true;
            mWorkerIdleCondition.notify_all();
            // Only wake if notified and command queue is not empty
            mWorkAvailableCondition.wait(lock, [this] { return !mTasks.empty(); });
        }
        mWorkerThreadIdle = false;
        vk::CommandProcessorTask task(std::move(mTasks.front()));
        mTasks.pop();
        lock.unlock();

        switch (task.getTaskCommand())
        {
            case vk::CustomTask::Exit:
            {
                ANGLE_TRY(mTaskProcessor.finishToSerial(this, Serial::Infinite()));
                *exitThread = true;
                // Shutting down so cleanup
                mTaskProcessor.destroy(mRenderer->getDevice());
                mCommandPool.destroy(mRenderer->getDevice());
                mPrimaryCommandBuffer.destroy(mRenderer->getDevice());
                mWorkerThreadIdle = true;
                mWorkerIdleCondition.notify_one();
                return angle::Result::Continue;
            }
            default:
                ANGLE_TRY(processTask(&task));
                break;
        }
    }

    UNREACHABLE();
    return angle::Result::Stop;
}

angle::Result CommandProcessor::processTask(vk::CommandProcessorTask *task)
{
    switch (task->getTaskCommand())
    {
        case vk::CustomTask::FlushAndQueueSubmit:
        {
            // End command buffer
            ANGLE_VK_TRY(this, mPrimaryCommandBuffer.end());
            // 1. Create submitInfo
            VkSubmitInfo submitInfo = {};
            InitializeSubmitInfo(&submitInfo, mPrimaryCommandBuffer, task->getWaitSemaphores(),
                                 &task->getWaitSemaphoreStageMasks(), task->getSemaphore());

            // 2. Get shared submit fence. It's possible there are other users of this fence that
            // must wait for the work to be submitted before waiting on the fence. Reset the fence
            // immediately so we are sure to get a fresh one next time.
            vk::Shared<vk::Fence> fence;
            ANGLE_TRY(mRenderer->getNextSubmitFence(&fence, true));

            // 3. Call submitFrame()
            ANGLE_TRY(mTaskProcessor.submitFrame(
                this, getRenderer()->getVkQueue(task->getPriority()), submitInfo, fence,
                &task->getGarbage(), &mCommandPool, std::move(mPrimaryCommandBuffer),
                task->getQueueSerial()));
            // 4. Allocate & begin new primary command buffer
            ANGLE_TRY(mTaskProcessor.allocatePrimaryCommandBuffer(this, &mPrimaryCommandBuffer));

            VkCommandBufferBeginInfo beginInfo = {};
            beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginInfo.pInheritanceInfo         = nullptr;
            ANGLE_VK_TRY(this, mPrimaryCommandBuffer.begin(beginInfo));

            // Free this local reference
            getRenderer()->resetSharedFence(&fence);

            ASSERT(task->getGarbage().empty());
            break;
        }
        case vk::CustomTask::OneOffQueueSubmit:
        {
            VkSubmitInfo submitInfo = {};
            submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            if (task->getOneOffCommandBufferVk() != VK_NULL_HANDLE)
            {
                submitInfo.commandBufferCount = 1;
                submitInfo.pCommandBuffers    = &task->getOneOffCommandBufferVk();
            }

            // TODO: https://issuetracker.google.com/issues/170328907 - vkQueueSubmit should be
            // owned by TaskProcessor to ensure proper synchronization
            ANGLE_TRY(mTaskProcessor.queueSubmit(this,
                                                 getRenderer()->getVkQueue(task->getPriority()),
                                                 submitInfo, task->getOneOffFence()));
            ANGLE_TRY(mTaskProcessor.checkCompletedCommands(this));
            break;
        }
        case vk::CustomTask::FinishToSerial:
        {
            ANGLE_TRY(mTaskProcessor.finishToSerial(this, task->getQueueSerial()));
            break;
        }
        case vk::CustomTask::Present:
        {
            VkResult result = mTaskProcessor.present(getRenderer()->getVkQueue(task->getPriority()),
                                                     task->getPresentInfo());
            if (ANGLE_UNLIKELY(result != VK_SUCCESS))
            {
                // Save the error so that we can handle it (e.g. VK_OUT_OF_DATE)
                // Don't leave processing loop, don't consider errors from present to be fatal.
                // TODO: https://issuetracker.google.com/issues/170329600 - This needs to improve to
                // properly parallelize present
                handleError(result, __FILE__, __FUNCTION__, __LINE__);
            }
            break;
        }
        case vk::CustomTask::ProcessCommands:
        {
            ASSERT(!task->getCommandBuffer()->empty());
            ANGLE_TRY(task->getCommandBuffer()->flushToPrimary(
                getRenderer()->getFeatures(), &mPrimaryCommandBuffer, task->getRenderPass()));
            ASSERT(task->getCommandBuffer()->empty());
            task->getCommandBuffer()->releaseToContextQueue(task->getContextVk());
            break;
        }
        default:
            UNREACHABLE();
            break;
    }

    return angle::Result::Continue;
}

void CommandProcessor::waitForWorkComplete(vk::Context *context)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::waitForWorkerThreadIdle");
    std::unique_lock<std::mutex> lock(mWorkerMutex);
    mWorkerIdleCondition.wait(lock, [this] { return (mTasks.empty() && mWorkerThreadIdle); });
    // Worker thread is idle and command queue is empty so good to continue

    if (!context)
    {
        return;
    }

    // Sync any errors to the context
    while (hasPendingError())
    {
        vk::Error workerError = getAndClearPendingError();
        if (workerError.mErrorCode != VK_SUCCESS)
        {
            context->handleError(workerError.mErrorCode, workerError.mFile, workerError.mFunction,
                                 workerError.mLine);
        }
    }
}

void CommandProcessor::shutdown(std::thread *commandProcessorThread)
{
    vk::CommandProcessorTask endTask;
    endTask.initTask(vk::CustomTask::Exit);
    queueCommand(nullptr, &endTask);
    waitForWorkComplete(nullptr);
    if (commandProcessorThread->joinable())
    {
        commandProcessorThread->join();
    }
}

// Return the fence for the last submit. This may mean waiting on the worker to process tasks to
// actually get to the last submit
vk::Shared<vk::Fence> CommandProcessor::getLastSubmittedFence() const
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::getLastSubmittedFence");
    std::unique_lock<std::mutex> lock(mWorkerMutex);
    mWorkerIdleCondition.wait(lock, [this] { return (mTasks.empty() && mWorkerThreadIdle); });

    // Worker thread is idle and command queue is empty so good to continue

    return mTaskProcessor.getLastSubmittedFenceWithLock(getDevice());
}

Serial CommandProcessor::getLastSubmittedSerial()
{
    std::lock_guard<std::mutex> lock(mCommandProcessorQueueSerialMutex);
    return mCommandProcessorLastSubmittedSerial;
}

Serial CommandProcessor::getCurrentQueueSerial()
{
    std::lock_guard<std::mutex> lock(mCommandProcessorQueueSerialMutex);
    return mCommandProcessorCurrentQueueSerial;
}

// Wait until all commands up to and including serial have been processed
void CommandProcessor::finishToSerial(vk::Context *context, Serial serial)
{
    vk::CommandProcessorTask finishToSerial;
    finishToSerial.initFinishToSerial(serial);
    queueCommand(context, &finishToSerial);

    // Wait until the worker is idle. At that point we know that the finishToSerial command has
    // completed executing, including any associated state cleanup.
    waitForWorkComplete(context);
}

void CommandProcessor::handleDeviceLost()
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::handleDeviceLost");
    std::unique_lock<std::mutex> lock(mWorkerMutex);
    mWorkerIdleCondition.wait(lock, [this] { return (mTasks.empty() && mWorkerThreadIdle); });

    // Worker thread is idle and command queue is empty so good to continue
    mTaskProcessor.handleDeviceLost(this);
}

void CommandProcessor::finishAllWork(vk::Context *context)
{
    // Wait for GPU work to finish
    finishToSerial(context, Serial::Infinite());
}

}  // namespace vk

}  // namespace rx

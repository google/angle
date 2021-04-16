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
namespace vk
{
namespace
{
constexpr size_t kInFlightCommandsLimit = 100u;
constexpr bool kOutputVmaStatsString    = false;

void InitializeSubmitInfo(VkSubmitInfo *submitInfo,
                          const vk::PrimaryCommandBuffer &commandBuffer,
                          const std::vector<VkSemaphore> &waitSemaphores,
                          const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
                          const vk::Semaphore *signalSemaphore)
{
    // Verify that the submitInfo has been zero'd out.
    ASSERT(submitInfo->signalSemaphoreCount == 0);
    ASSERT(waitSemaphores.size() == waitSemaphoreStageMasks.size());
    submitInfo->sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo->commandBufferCount = commandBuffer.valid() ? 1 : 0;
    submitInfo->pCommandBuffers    = commandBuffer.ptr();
    submitInfo->waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
    submitInfo->pWaitSemaphores    = waitSemaphores.data();
    submitInfo->pWaitDstStageMask  = waitSemaphoreStageMasks.data();

    if (signalSemaphore)
    {
        submitInfo->signalSemaphoreCount = 1;
        submitInfo->pSignalSemaphores    = signalSemaphore->ptr();
    }
}

bool CommandsHaveValidOrdering(const std::vector<vk::CommandBatch> &commands)
{
    Serial currentSerial;
    for (const vk::CommandBatch &commands : commands)
    {
        if (commands.serial <= currentSerial)
        {
            return false;
        }
        currentSerial = commands.serial;
    }

    return true;
}
}  // namespace

angle::Result FenceRecycler::newSharedFence(vk::Context *context,
                                            vk::Shared<vk::Fence> *sharedFenceOut)
{
    bool gotRecycledFence = false;
    vk::Fence fence;
    {
        std::lock_guard<std::mutex> lock(mMutex);
        if (!mRecyler.empty())
        {
            mRecyler.fetch(&fence);
            gotRecycledFence = true;
        }
    }

    VkDevice device(context->getDevice());
    if (gotRecycledFence)
    {
        ANGLE_VK_TRY(context, fence.reset(device));
    }
    else
    {
        VkFenceCreateInfo fenceCreateInfo = {};
        fenceCreateInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceCreateInfo.flags             = 0;
        ANGLE_VK_TRY(context, fence.init(device, fenceCreateInfo));
    }
    sharedFenceOut->assign(device, std::move(fence));
    return angle::Result::Continue;
}

void FenceRecycler::destroy(vk::Context *context)
{
    std::lock_guard<std::mutex> lock(mMutex);
    mRecyler.destroy(context->getDevice());
}

// CommandProcessorTask implementation
void CommandProcessorTask::initTask()
{
    mTask                        = CustomTask::Invalid;
    mRenderPass                  = nullptr;
    mCommandBuffer               = nullptr;
    mSemaphore                   = nullptr;
    mOneOffFence                 = nullptr;
    mPresentInfo                 = {};
    mPresentInfo.pResults        = nullptr;
    mPresentInfo.pSwapchains     = nullptr;
    mPresentInfo.pImageIndices   = nullptr;
    mPresentInfo.pNext           = nullptr;
    mPresentInfo.pWaitSemaphores = nullptr;
    mOneOffCommandBufferVk       = VK_NULL_HANDLE;
}

void CommandProcessorTask::initProcessCommands(CommandBufferHelper *commandBuffer,
                                               const RenderPass *renderPass)
{
    mTask          = CustomTask::ProcessCommands;
    mCommandBuffer = commandBuffer;
    mRenderPass    = renderPass;
}

void CommandProcessorTask::copyPresentInfo(const VkPresentInfoKHR &other)
{
    if (other.sType == 0)
    {
        return;
    }

    mPresentInfo.sType = other.sType;
    mPresentInfo.pNext = other.pNext;

    if (other.swapchainCount > 0)
    {
        ASSERT(other.swapchainCount == 1);
        mPresentInfo.swapchainCount = 1;
        mSwapchain                  = other.pSwapchains[0];
        mPresentInfo.pSwapchains    = &mSwapchain;
        mImageIndex                 = other.pImageIndices[0];
        mPresentInfo.pImageIndices  = &mImageIndex;
    }

    if (other.waitSemaphoreCount > 0)
    {
        ASSERT(other.waitSemaphoreCount == 1);
        mPresentInfo.waitSemaphoreCount = 1;
        mWaitSemaphore                  = other.pWaitSemaphores[0];
        mPresentInfo.pWaitSemaphores    = &mWaitSemaphore;
    }

    mPresentInfo.pResults = other.pResults;

    void *pNext = const_cast<void *>(other.pNext);
    while (pNext != nullptr)
    {
        VkStructureType sType = *reinterpret_cast<VkStructureType *>(pNext);
        switch (sType)
        {
            case VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR:
            {
                const VkPresentRegionsKHR *presentRegions =
                    reinterpret_cast<VkPresentRegionsKHR *>(pNext);
                mPresentRegion = *presentRegions->pRegions;
                mRects.resize(mPresentRegion.rectangleCount);
                for (uint32_t i = 0; i < mPresentRegion.rectangleCount; i++)
                {
                    mRects[i] = presentRegions->pRegions->pRectangles[i];
                }
                mPresentRegion.pRectangles = mRects.data();

                mPresentRegions.sType          = VK_STRUCTURE_TYPE_PRESENT_REGIONS_KHR;
                mPresentRegions.pNext          = presentRegions->pNext;
                mPresentRegions.swapchainCount = 1;
                mPresentRegions.pRegions       = &mPresentRegion;
                mPresentInfo.pNext             = &mPresentRegions;
                pNext                          = const_cast<void *>(presentRegions->pNext);
                break;
            }
            default:
                ERR() << "Unknown sType: " << sType << " in VkPresentInfoKHR.pNext chain";
                UNREACHABLE();
                break;
        }
    }
}

void CommandProcessorTask::initPresent(egl::ContextPriority priority,
                                       const VkPresentInfoKHR &presentInfo)
{
    mTask     = CustomTask::Present;
    mPriority = priority;
    copyPresentInfo(presentInfo);
}

void CommandProcessorTask::initFinishToSerial(Serial serial)
{
    // Note: sometimes the serial is not valid and that's okay, the finish will early exit in the
    // TaskProcessor::finishToSerial
    mTask   = CustomTask::FinishToSerial;
    mSerial = serial;
}

void CommandProcessorTask::initFlushAndQueueSubmit(
    const std::vector<VkSemaphore> &waitSemaphores,
    const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
    const Semaphore *semaphore,
    egl::ContextPriority priority,
    GarbageList &&currentGarbage,
    Serial submitQueueSerial)
{
    mTask                    = CustomTask::FlushAndQueueSubmit;
    mWaitSemaphores          = waitSemaphores;
    mWaitSemaphoreStageMasks = waitSemaphoreStageMasks;
    mSemaphore               = semaphore;
    mGarbage                 = std::move(currentGarbage);
    mPriority                = priority;
    mSerial                  = submitQueueSerial;
}

void CommandProcessorTask::initOneOffQueueSubmit(VkCommandBuffer commandBufferHandle,
                                                 egl::ContextPriority priority,
                                                 const Fence *fence,
                                                 Serial submitQueueSerial)
{
    mTask                  = CustomTask::OneOffQueueSubmit;
    mOneOffCommandBufferVk = commandBufferHandle;
    mOneOffFence           = fence;
    mPriority              = priority;
    mSerial                = submitQueueSerial;
}

CommandProcessorTask &CommandProcessorTask::operator=(CommandProcessorTask &&rhs)
{
    if (this == &rhs)
    {
        return *this;
    }

    std::swap(mRenderPass, rhs.mRenderPass);
    std::swap(mCommandBuffer, rhs.mCommandBuffer);
    std::swap(mTask, rhs.mTask);
    std::swap(mWaitSemaphores, rhs.mWaitSemaphores);
    std::swap(mWaitSemaphoreStageMasks, rhs.mWaitSemaphoreStageMasks);
    std::swap(mSemaphore, rhs.mSemaphore);
    std::swap(mOneOffFence, rhs.mOneOffFence);
    std::swap(mGarbage, rhs.mGarbage);
    std::swap(mSerial, rhs.mSerial);
    std::swap(mPriority, rhs.mPriority);
    std::swap(mOneOffCommandBufferVk, rhs.mOneOffCommandBufferVk);

    copyPresentInfo(rhs.mPresentInfo);

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

// CommandProcessor implementation.
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
        handleDeviceLost(mRenderer);
    }

    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    Error error = {errorCode, file, function, line};
    mErrors.emplace(error);
}

CommandProcessor::CommandProcessor(RendererVk *renderer)
    : Context(renderer), mWorkerThreadIdle(false)
{
    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    while (!mErrors.empty())
    {
        mErrors.pop();
    }
}

CommandProcessor::~CommandProcessor() = default;

angle::Result CommandProcessor::checkAndPopPendingError(Context *errorHandlingContext)
{
    std::lock_guard<std::mutex> queueLock(mErrorMutex);
    if (mErrors.empty())
    {
        return angle::Result::Continue;
    }
    else
    {
        Error err = mErrors.front();
        mErrors.pop();
        errorHandlingContext->handleError(err.errorCode, err.file, err.function, err.line);
        return angle::Result::Stop;
    }
}

void CommandProcessor::queueCommand(CommandProcessorTask &&task)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::queueCommand");
    // Grab the worker mutex so that we put things on the queue in the same order as we give out
    // serials.
    std::lock_guard<std::mutex> queueLock(mWorkerMutex);

    mTasks.emplace(std::move(task));
    mWorkAvailableCondition.notify_one();
}

void CommandProcessor::processTasks(const DeviceQueueMap &queueMap)
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
        CommandProcessorTask task(std::move(mTasks.front()));
        mTasks.pop();
        lock.unlock();

        ANGLE_TRY(processTask(&task));
        if (task.getTaskCommand() == CustomTask::Exit)
        {

            *exitThread = true;
            lock.lock();
            mWorkerThreadIdle = true;
            mWorkerIdleCondition.notify_one();
            return angle::Result::Continue;
        }
    }

    UNREACHABLE();
    return angle::Result::Stop;
}

angle::Result CommandProcessor::processTask(CommandProcessorTask *task)
{
    switch (task->getTaskCommand())
    {
        case CustomTask::Exit:
        {
            ANGLE_TRY(mCommandQueue.finishToSerial(this, Serial::Infinite(),
                                                   mRenderer->getMaxFenceWaitTimeNs()));
            // Shutting down so cleanup
            mCommandQueue.destroy(this);
            mCommandPool.destroy(mRenderer->getDevice());
            break;
        }
        case CustomTask::FlushAndQueueSubmit:
        {
            ANGLE_TRACE_EVENT0("gpu.angle", "processTask::FlushAndQueueSubmit");
            // End command buffer

            // Call submitFrame()
            ANGLE_TRY(mCommandQueue.submitFrame(
                this, task->getPriority(), task->getWaitSemaphores(),
                task->getWaitSemaphoreStageMasks(), task->getSemaphore(),
                std::move(task->getGarbage()), &mCommandPool, task->getQueueSerial()));

            ASSERT(task->getGarbage().empty());
            break;
        }
        case CustomTask::OneOffQueueSubmit:
        {
            ANGLE_TRACE_EVENT0("gpu.angle", "processTask::OneOffQueueSubmit");

            ANGLE_TRY(mCommandQueue.queueSubmitOneOff(
                this, task->getPriority(), task->getOneOffCommandBufferVk(), task->getOneOffFence(),
                SubmitPolicy::EnsureSubmitted, task->getQueueSerial()));
            ANGLE_TRY(mCommandQueue.checkCompletedCommands(this));
            break;
        }
        case CustomTask::FinishToSerial:
        {
            ANGLE_TRY(mCommandQueue.finishToSerial(this, task->getQueueSerial(),
                                                   mRenderer->getMaxFenceWaitTimeNs()));
            break;
        }
        case CustomTask::Present:
        {
            VkResult result = present(task->getPriority(), task->getPresentInfo());
            if (ANGLE_UNLIKELY(result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR))
            {
                // We get to ignore these as they are not fatal
            }
            else if (ANGLE_UNLIKELY(result != VK_SUCCESS))
            {
                // Save the error so that we can handle it.
                // Don't leave processing loop, don't consider errors from present to be fatal.
                // TODO: https://issuetracker.google.com/issues/170329600 - This needs to improve to
                // properly parallelize present
                handleError(result, __FILE__, __FUNCTION__, __LINE__);
            }
            break;
        }
        case CustomTask::ProcessCommands:
        {
            ASSERT(!task->getCommandBuffer()->empty());

            CommandBufferHelper *commandBuffer = task->getCommandBuffer();
            if (task->getRenderPass())
            {
                ANGLE_TRY(mCommandQueue.flushRenderPassCommands(this, *task->getRenderPass(),
                                                                &commandBuffer));
            }
            else
            {
                ANGLE_TRY(mCommandQueue.flushOutsideRPCommands(this, &commandBuffer));
            }
            ASSERT(task->getCommandBuffer()->empty());
            mRenderer->recycleCommandBufferHelper(task->getCommandBuffer());
            break;
        }
        case CustomTask::CheckCompletedCommands:
        {
            ANGLE_TRY(mCommandQueue.checkCompletedCommands(this));
            break;
        }
        default:
            UNREACHABLE();
            break;
    }

    return angle::Result::Continue;
}

angle::Result CommandProcessor::checkCompletedCommands(Context *context)
{
    ANGLE_TRY(checkAndPopPendingError(context));

    CommandProcessorTask checkCompletedTask;
    checkCompletedTask.initTask(CustomTask::CheckCompletedCommands);
    queueCommand(std::move(checkCompletedTask));

    return angle::Result::Continue;
}

angle::Result CommandProcessor::waitForWorkComplete(Context *context)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::waitForWorkComplete");
    std::unique_lock<std::mutex> lock(mWorkerMutex);
    mWorkerIdleCondition.wait(lock, [this] { return (mTasks.empty() && mWorkerThreadIdle); });
    // Worker thread is idle and command queue is empty so good to continue

    // Sync any errors to the context
    bool shouldStop = hasPendingError();
    while (hasPendingError())
    {
        (void)checkAndPopPendingError(context);
    }
    return shouldStop ? angle::Result::Stop : angle::Result::Continue;
}

angle::Result CommandProcessor::init(Context *context, const DeviceQueueMap &queueMap)
{
    ANGLE_TRY(mCommandQueue.init(context, queueMap));

    mTaskThread = std::thread(&CommandProcessor::processTasks, this, queueMap);

    return angle::Result::Continue;
}

void CommandProcessor::destroy(Context *context)
{
    CommandProcessorTask endTask;
    endTask.initTask(CustomTask::Exit);
    queueCommand(std::move(endTask));
    (void)waitForWorkComplete(context);
    if (mTaskThread.joinable())
    {
        mTaskThread.join();
    }
}

Serial CommandProcessor::getLastCompletedQueueSerial() const
{
    std::lock_guard<std::mutex> lock(mQueueSerialMutex);
    return mCommandQueue.getLastCompletedQueueSerial();
}

Serial CommandProcessor::getLastSubmittedQueueSerial() const
{
    std::lock_guard<std::mutex> lock(mQueueSerialMutex);
    return mCommandQueue.getLastSubmittedQueueSerial();
}

Serial CommandProcessor::getCurrentQueueSerial() const
{
    std::lock_guard<std::mutex> lock(mQueueSerialMutex);
    return mCommandQueue.getCurrentQueueSerial();
}

Serial CommandProcessor::reserveSubmitSerial()
{
    std::lock_guard<std::mutex> lock(mQueueSerialMutex);
    return mCommandQueue.reserveSubmitSerial();
}

// Wait until all commands up to and including serial have been processed
angle::Result CommandProcessor::finishToSerial(Context *context, Serial serial, uint64_t timeout)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::finishToSerial");

    ANGLE_TRY(checkAndPopPendingError(context));

    CommandProcessorTask task;
    task.initFinishToSerial(serial);
    queueCommand(std::move(task));

    // Wait until the worker is idle. At that point we know that the finishToSerial command has
    // completed executing, including any associated state cleanup.
    return waitForWorkComplete(context);
}

void CommandProcessor::handleDeviceLost(RendererVk *renderer)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::handleDeviceLost");
    std::unique_lock<std::mutex> lock(mWorkerMutex);
    mWorkerIdleCondition.wait(lock, [this] { return (mTasks.empty() && mWorkerThreadIdle); });

    // Worker thread is idle and command queue is empty so good to continue
    mCommandQueue.handleDeviceLost(renderer);
}

angle::Result CommandProcessor::finishAllWork(Context *context)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::finishAllWork");
    // Wait for GPU work to finish
    return finishToSerial(context, Serial::Infinite(), mRenderer->getMaxFenceWaitTimeNs());
}

VkResult CommandProcessor::getLastAndClearPresentResult(VkSwapchainKHR swapchain)
{
    std::unique_lock<std::mutex> lock(mSwapchainStatusMutex);
    if (mSwapchainStatus.find(swapchain) == mSwapchainStatus.end())
    {
        // Wake when required swapchain status becomes available
        mSwapchainStatusCondition.wait(lock, [this, swapchain] {
            return mSwapchainStatus.find(swapchain) != mSwapchainStatus.end();
        });
    }
    VkResult result = mSwapchainStatus[swapchain];
    mSwapchainStatus.erase(swapchain);
    return result;
}

VkResult CommandProcessor::present(egl::ContextPriority priority,
                                   const VkPresentInfoKHR &presentInfo)
{
    std::lock_guard<std::mutex> lock(mSwapchainStatusMutex);
    ANGLE_TRACE_EVENT0("gpu.angle", "vkQueuePresentKHR");
    VkResult result = mCommandQueue.queuePresent(priority, presentInfo);

    // Verify that we are presenting one and only one swapchain
    ASSERT(presentInfo.swapchainCount == 1);
    ASSERT(presentInfo.pResults == nullptr);
    mSwapchainStatus[presentInfo.pSwapchains[0]] = result;

    mSwapchainStatusCondition.notify_all();

    return result;
}

angle::Result CommandProcessor::submitFrame(
    Context *context,
    egl::ContextPriority priority,
    const std::vector<VkSemaphore> &waitSemaphores,
    const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
    const Semaphore *signalSemaphore,
    GarbageList &&currentGarbage,
    CommandPool *commandPool,
    Serial submitQueueSerial)
{
    ANGLE_TRY(checkAndPopPendingError(context));

    CommandProcessorTask task;
    task.initFlushAndQueueSubmit(waitSemaphores, waitSemaphoreStageMasks, signalSemaphore, priority,
                                 std::move(currentGarbage), submitQueueSerial);

    queueCommand(std::move(task));

    return angle::Result::Continue;
}

angle::Result CommandProcessor::queueSubmitOneOff(Context *context,
                                                  egl::ContextPriority contextPriority,
                                                  VkCommandBuffer commandBufferHandle,
                                                  const Fence *fence,
                                                  SubmitPolicy submitPolicy,
                                                  Serial submitQueueSerial)
{
    ANGLE_TRY(checkAndPopPendingError(context));

    CommandProcessorTask task;
    task.initOneOffQueueSubmit(commandBufferHandle, contextPriority, fence, submitQueueSerial);
    queueCommand(std::move(task));
    if (submitPolicy == SubmitPolicy::EnsureSubmitted)
    {
        // Caller has synchronization requirement to have work in GPU pipe when returning from this
        // function.
        ANGLE_TRY(waitForWorkComplete(context));
    }

    return angle::Result::Continue;
}

VkResult CommandProcessor::queuePresent(egl::ContextPriority contextPriority,
                                        const VkPresentInfoKHR &presentInfo)
{
    CommandProcessorTask task;
    task.initPresent(contextPriority, presentInfo);

    ANGLE_TRACE_EVENT0("gpu.angle", "CommandProcessor::queuePresent");
    queueCommand(std::move(task));

    // Always return success, when we call acquireNextImage we'll check the return code. This
    // allows the app to continue working until we really need to know the return code from
    // present.
    return VK_SUCCESS;
}

angle::Result CommandProcessor::waitForSerialWithUserTimeout(vk::Context *context,
                                                             Serial serial,
                                                             uint64_t timeout,
                                                             VkResult *result)
{
    // If finishToSerial times out we generate an error. Therefore we a large timeout.
    // TODO: https://issuetracker.google.com/170312581 - Wait with timeout.
    return finishToSerial(context, serial, mRenderer->getMaxFenceWaitTimeNs());
}

angle::Result CommandProcessor::flushOutsideRPCommands(Context *context,
                                                       CommandBufferHelper **outsideRPCommands)
{
    ANGLE_TRY(checkAndPopPendingError(context));

    (*outsideRPCommands)->markClosed();
    CommandProcessorTask task;
    task.initProcessCommands(*outsideRPCommands, nullptr);
    queueCommand(std::move(task));
    *outsideRPCommands = mRenderer->getCommandBufferHelper(false);

    return angle::Result::Continue;
}

angle::Result CommandProcessor::flushRenderPassCommands(Context *context,
                                                        const RenderPass &renderPass,
                                                        CommandBufferHelper **renderPassCommands)
{
    ANGLE_TRY(checkAndPopPendingError(context));

    (*renderPassCommands)->markClosed();
    CommandProcessorTask task;
    task.initProcessCommands(*renderPassCommands, &renderPass);
    queueCommand(std::move(task));
    *renderPassCommands = mRenderer->getCommandBufferHelper(true);

    return angle::Result::Continue;
}

// CommandQueue implementation.
CommandQueue::CommandQueue() : mCurrentQueueSerial(mQueueSerialFactory.generate()) {}

CommandQueue::~CommandQueue() = default;

void CommandQueue::destroy(Context *context)
{
    // Force all commands to finish by flushing all queues.
    for (VkQueue queue : mQueues)
    {
        if (queue != VK_NULL_HANDLE)
        {
            vkQueueWaitIdle(queue);
        }
    }

    RendererVk *renderer = context->getRenderer();

    mLastCompletedQueueSerial = Serial::Infinite();
    (void)clearAllGarbage(renderer);

    mPrimaryCommands.destroy(renderer->getDevice());
    mPrimaryCommandPool.destroy(renderer->getDevice());
    mFenceRecycler.destroy(context);

    ASSERT(mInFlightCommands.empty() && mGarbageQueue.empty());
}

angle::Result CommandQueue::init(Context *context, const DeviceQueueMap &queueMap)
{
    RendererVk *renderer = context->getRenderer();

    // Initialize the command pool now that we know the queue family index.
    uint32_t queueFamilyIndex = renderer->getQueueFamilyIndex();
    ANGLE_TRY(mPrimaryCommandPool.init(context, queueFamilyIndex));

    mQueues = queueMap;

    return angle::Result::Continue;
}

angle::Result CommandQueue::checkCompletedCommands(Context *context)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::checkCompletedCommandsNoLock");
    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    int finishedCount = 0;

    for (CommandBatch &batch : mInFlightCommands)
    {
        VkResult result = batch.fence.get().getStatus(device);
        if (result == VK_NOT_READY)
        {
            break;
        }
        ANGLE_VK_TRY(context, result);
        ++finishedCount;
    }

    if (finishedCount == 0)
    {
        return angle::Result::Continue;
    }

    return retireFinishedCommands(context, finishedCount);
}

angle::Result CommandQueue::retireFinishedCommands(Context *context, size_t finishedCount)
{
    ASSERT(finishedCount > 0);

    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    for (size_t commandIndex = 0; commandIndex < finishedCount; ++commandIndex)
    {
        CommandBatch &batch = mInFlightCommands[commandIndex];

        mLastCompletedQueueSerial = batch.serial;
        mFenceRecycler.resetSharedFence(&batch.fence);
        ANGLE_TRACE_EVENT0("gpu.angle", "command buffer recycling");
        batch.commandPool.destroy(device);
        ANGLE_TRY(mPrimaryCommandPool.collect(context, std::move(batch.primaryCommands)));
    }

    if (finishedCount > 0)
    {
        auto beginIter = mInFlightCommands.begin();
        mInFlightCommands.erase(beginIter, beginIter + finishedCount);
    }

    size_t freeIndex = 0;
    for (; freeIndex < mGarbageQueue.size(); ++freeIndex)
    {
        GarbageAndSerial &garbageList = mGarbageQueue[freeIndex];
        if (garbageList.getSerial() < mLastCompletedQueueSerial)
        {
            for (GarbageObject &garbage : garbageList.get())
            {
                garbage.destroy(renderer);
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

angle::Result CommandQueue::releaseToCommandBatch(Context *context,
                                                  PrimaryCommandBuffer &&commandBuffer,
                                                  CommandPool *commandPool,
                                                  CommandBatch *batch)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::releaseToCommandBatch");

    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    batch->primaryCommands = std::move(commandBuffer);

    if (commandPool->valid())
    {
        batch->commandPool = std::move(*commandPool);
        // Recreate CommandPool
        VkCommandPoolCreateInfo poolInfo = {};
        poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
        poolInfo.queueFamilyIndex        = renderer->getQueueFamilyIndex();

        ANGLE_VK_TRY(context, commandPool->init(device, poolInfo));
    }

    return angle::Result::Continue;
}

void CommandQueue::clearAllGarbage(RendererVk *renderer)
{
    for (GarbageAndSerial &garbageList : mGarbageQueue)
    {
        for (GarbageObject &garbage : garbageList.get())
        {
            garbage.destroy(renderer);
        }
    }
    mGarbageQueue.clear();
}

void CommandQueue::handleDeviceLost(RendererVk *renderer)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::handleDeviceLost");

    VkDevice device = renderer->getDevice();

    for (CommandBatch &batch : mInFlightCommands)
    {
        // On device loss we need to wait for fence to be signaled before destroying it
        VkResult status = batch.fence.get().wait(device, renderer->getMaxFenceWaitTimeNs());
        // If the wait times out, it is probably not possible to recover from lost device
        ASSERT(status == VK_SUCCESS || status == VK_ERROR_DEVICE_LOST);

        // On device lost, here simply destroy the CommandBuffer, it will fully cleared later
        // by CommandPool::destroy
        batch.primaryCommands.destroy(device);

        batch.commandPool.destroy(device);
        batch.fence.reset(device);
    }
    mInFlightCommands.clear();
}

bool CommandQueue::allInFlightCommandsAreAfterSerial(Serial serial) const
{
    return mInFlightCommands.empty() || mInFlightCommands[0].serial > serial;
}

angle::Result CommandQueue::finishToSerial(Context *context, Serial finishSerial, uint64_t timeout)
{
    if (mInFlightCommands.empty())
    {
        return angle::Result::Continue;
    }

    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::finishToSerial");

    // Find the serial in the the list. The serials should be in order.
    ASSERT(CommandsHaveValidOrdering(mInFlightCommands));

    size_t finishedCount = 0;
    while (finishedCount < mInFlightCommands.size() &&
           mInFlightCommands[finishedCount].serial <= finishSerial)
    {
        finishedCount++;
    }

    if (finishedCount == 0)
    {
        return angle::Result::Continue;
    }

    const CommandBatch &batch = mInFlightCommands[finishedCount - 1];

    // Wait for it finish
    VkDevice device = context->getDevice();
    VkResult status = batch.fence.get().wait(device, timeout);

    ANGLE_VK_TRY(context, status);

    // Clean up finished batches.
    ANGLE_TRY(retireFinishedCommands(context, finishedCount));
    ASSERT(allInFlightCommandsAreAfterSerial(finishSerial));

    return angle::Result::Continue;
}

Serial CommandQueue::reserveSubmitSerial()
{
    Serial returnSerial = mCurrentQueueSerial;
    mCurrentQueueSerial = mQueueSerialFactory.generate();
    return returnSerial;
}

angle::Result CommandQueue::submitFrame(
    Context *context,
    egl::ContextPriority priority,
    const std::vector<VkSemaphore> &waitSemaphores,
    const std::vector<VkPipelineStageFlags> &waitSemaphoreStageMasks,
    const Semaphore *signalSemaphore,
    GarbageList &&currentGarbage,
    CommandPool *commandPool,
    Serial submitQueueSerial)
{
    // Start an empty primary buffer if we have an empty submit.
    ANGLE_TRY(ensurePrimaryCommandBufferValid(context));
    ANGLE_VK_TRY(context, mPrimaryCommands.end());

    VkSubmitInfo submitInfo = {};
    InitializeSubmitInfo(&submitInfo, mPrimaryCommands, waitSemaphores, waitSemaphoreStageMasks,
                         signalSemaphore);

    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::submitFrame");

    RendererVk *renderer = context->getRenderer();
    VkDevice device      = renderer->getDevice();

    DeviceScoped<CommandBatch> scopedBatch(device);
    CommandBatch &batch = scopedBatch.get();

    ANGLE_TRY(mFenceRecycler.newSharedFence(context, &batch.fence));
    batch.serial = submitQueueSerial;

    ANGLE_TRY(queueSubmit(context, priority, submitInfo, &batch.fence.get(), batch.serial));

    if (!currentGarbage.empty())
    {
        mGarbageQueue.emplace_back(std::move(currentGarbage), batch.serial);
    }

    // Store the primary CommandBuffer and command pool used for secondary CommandBuffers
    // in the in-flight list.
    ANGLE_TRY(releaseToCommandBatch(context, std::move(mPrimaryCommands), commandPool, &batch));

    mInFlightCommands.emplace_back(scopedBatch.release());

    ANGLE_TRY(checkCompletedCommands(context));

    // CPU should be throttled to avoid mInFlightCommands from growing too fast. Important for
    // off-screen scenarios.
    if (mInFlightCommands.size() > kInFlightCommandsLimit)
    {
        size_t numCommandsToFinish = mInFlightCommands.size() - kInFlightCommandsLimit;
        Serial finishSerial        = mInFlightCommands[numCommandsToFinish].serial;
        ANGLE_TRY(finishToSerial(context, finishSerial, renderer->getMaxFenceWaitTimeNs()));
    }

    return angle::Result::Continue;
}

angle::Result CommandQueue::waitForSerialWithUserTimeout(vk::Context *context,
                                                         Serial serial,
                                                         uint64_t timeout,
                                                         VkResult *result)
{
    // No in-flight work. This indicates the serial is already complete.
    if (mInFlightCommands.empty())
    {
        *result = VK_SUCCESS;
        return angle::Result::Continue;
    }

    // Serial is already complete.
    if (serial < mInFlightCommands[0].serial)
    {
        *result = VK_SUCCESS;
        return angle::Result::Continue;
    }

    size_t batchIndex = 0;
    while (batchIndex != mInFlightCommands.size() && mInFlightCommands[batchIndex].serial < serial)
    {
        batchIndex++;
    }

    // Serial is not yet submitted. This is undefined behaviour, so we can do anything.
    if (batchIndex >= mInFlightCommands.size())
    {
        WARN() << "Waiting on an unsubmitted serial.";
        *result = VK_TIMEOUT;
        return angle::Result::Continue;
    }

    ASSERT(serial == mInFlightCommands[batchIndex].serial);

    vk::Fence &fence = mInFlightCommands[batchIndex].fence.get();
    ASSERT(fence.valid());
    *result = fence.wait(context->getDevice(), timeout);

    // Don't trigger an error on timeout.
    if (*result != VK_TIMEOUT)
    {
        ANGLE_VK_TRY(context, *result);
    }

    return angle::Result::Continue;
}

angle::Result CommandQueue::ensurePrimaryCommandBufferValid(Context *context)
{
    if (mPrimaryCommands.valid())
    {
        return angle::Result::Continue;
    }

    ANGLE_TRY(mPrimaryCommandPool.allocate(context, &mPrimaryCommands));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo         = nullptr;
    ANGLE_VK_TRY(context, mPrimaryCommands.begin(beginInfo));

    return angle::Result::Continue;
}

angle::Result CommandQueue::flushOutsideRPCommands(Context *context,
                                                   CommandBufferHelper **outsideRPCommands)
{
    ANGLE_TRY(ensurePrimaryCommandBufferValid(context));
    return (*outsideRPCommands)
        ->flushToPrimary(context->getRenderer()->getFeatures(), &mPrimaryCommands, nullptr);
}

angle::Result CommandQueue::flushRenderPassCommands(Context *context,
                                                    const RenderPass &renderPass,
                                                    CommandBufferHelper **renderPassCommands)
{
    ANGLE_TRY(ensurePrimaryCommandBufferValid(context));
    return (*renderPassCommands)
        ->flushToPrimary(context->getRenderer()->getFeatures(), &mPrimaryCommands, &renderPass);
}

angle::Result CommandQueue::queueSubmitOneOff(Context *context,
                                              egl::ContextPriority contextPriority,
                                              VkCommandBuffer commandBufferHandle,
                                              const Fence *fence,
                                              SubmitPolicy submitPolicy,
                                              Serial submitQueueSerial)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    if (commandBufferHandle != VK_NULL_HANDLE)
    {
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers    = &commandBufferHandle;
    }

    return queueSubmit(context, contextPriority, submitInfo, fence, submitQueueSerial);
}

angle::Result CommandQueue::queueSubmit(Context *context,
                                        egl::ContextPriority contextPriority,
                                        const VkSubmitInfo &submitInfo,
                                        const Fence *fence,
                                        Serial submitQueueSerial)
{
    ANGLE_TRACE_EVENT0("gpu.angle", "CommandQueue::queueSubmit");

    RendererVk *renderer = context->getRenderer();

    if (kOutputVmaStatsString)
    {
        renderer->outputVmaStatString();
    }

    VkFence fenceHandle = fence ? fence->getHandle() : VK_NULL_HANDLE;
    ANGLE_VK_TRY(context, vkQueueSubmit(mQueues[contextPriority], 1, &submitInfo, fenceHandle));
    mLastSubmittedQueueSerial = submitQueueSerial;

    // Now that we've submitted work, clean up RendererVk garbage
    return renderer->cleanupGarbage(mLastCompletedQueueSerial);
}

VkResult CommandQueue::queuePresent(egl::ContextPriority contextPriority,
                                    const VkPresentInfoKHR &presentInfo)
{
    return vkQueuePresentKHR(mQueues[contextPriority], &presentInfo);
}

Serial CommandQueue::getLastSubmittedQueueSerial() const
{
    return mLastSubmittedQueueSerial;
}

Serial CommandQueue::getLastCompletedQueueSerial() const
{
    return mLastCompletedQueueSerial;
}

Serial CommandQueue::getCurrentQueueSerial() const
{
    return mCurrentQueueSerial;
}
}  // namespace vk
}  // namespace rx

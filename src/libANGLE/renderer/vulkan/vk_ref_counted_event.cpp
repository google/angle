//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RefCountedEvent:
//    Manages reference count of VkEvent and its associated functions.
//

#include "libANGLE/renderer/vulkan/vk_ref_counted_event.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"
#include "libANGLE/renderer/vulkan/vk_renderer.h"

namespace rx
{
namespace vk
{
bool RefCountedEvent::init(Context *context, ImageLayout layout)
{
    ASSERT(mHandle == nullptr);
    ASSERT(layout != ImageLayout::Undefined);

    // First try with recycler. We must issue VkCmdResetEvent before VkCmdSetEvent
    if (context->getRefCountedEventsGarbageRecycler()->fetch(this) ||
        context->getRenderer()->getRefCountedEventRecycler()->fetch(this))
    {
        mHandle->get().needsReset = true;
    }
    else
    {
        // If failed to fetch from recycler, then create a new event.
        mHandle                      = new RefCounted<EventAndLayout>;
        VkEventCreateInfo createInfo = {};
        createInfo.sType             = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
        // Use device only for performance reasons.
        createInfo.flags = context->getFeatures().supportsSynchronization2.enabled
                               ? VK_EVENT_CREATE_DEVICE_ONLY_BIT_KHR
                               : 0;
        VkResult result  = mHandle->get().event.init(context->getDevice(), createInfo);
        if (result != VK_SUCCESS)
        {
            WARN() << "event.init failed. Clean up garbage and retry again";
            // Proactively clean up garbage and retry
            context->getRefCountedEventsGarbageRecycler()->cleanup(context->getRenderer());
            result = mHandle->get().event.init(context->getDevice(), createInfo);
            if (result != VK_SUCCESS)
            {
                // Drivers usually can allocate huge amount of VkEvents, and we should never use
                // that many VkEvents under normal situation. If we failed to allocate, there is a
                // high chance that we may have a leak somewhere. This macro should help us catch
                // such potential bugs in the bots if that happens.
                UNREACHABLE();
                // If still fail to create, we just return. An invalid event will trigger
                // pipelineBarrier code path
                return false;
            }
        }
        mHandle->get().needsReset = false;
    }

    mHandle->addRef();
    mHandle->get().imageLayout = layout;
    return true;
}

void RefCountedEvent::release(Context *context)
{
    if (mHandle != nullptr)
    {
        releaseImpl(context->getRenderer(), context->getRefCountedEventsGarbageRecycler());
    }
}

void RefCountedEvent::release(Renderer *renderer)
{
    if (mHandle != nullptr)
    {
        releaseImpl(renderer, renderer->getRefCountedEventRecycler());
    }
}

template <typename RecyclerT>
void RefCountedEvent::releaseImpl(Renderer *renderer, RecyclerT *recycler)
{
    ASSERT(mHandle != nullptr);
    // This should never called from async submission thread since the refcount is not atomic. It is
    // expected only called under context share lock.
    ASSERT(std::this_thread::get_id() != renderer->getCommandProcessorThreadId());

    const bool isLastReference = mHandle->getAndReleaseRef() == 1;
    if (isLastReference)
    {
        // When async submission is enabled, recycler will be null when release call comes from
        // CommandProcessor. But in that case it will not be the last reference since garbage
        // collector should have one reference count and will never release that reference count
        // until GPU finished.
        ASSERT(recycler != nullptr);
        recycler->recycle(std::move(*this));
        ASSERT(mHandle == nullptr);
    }
    else
    {
        mHandle = nullptr;
    }
}

void RefCountedEvent::destroy(VkDevice device)
{
    ASSERT(mHandle != nullptr);
    ASSERT(!mHandle->isReferenced());
    mHandle->get().event.destroy(device);
    SafeDelete(mHandle);
}

// RefCountedEventsGarbage implementation.
bool RefCountedEventsGarbage::releaseIfComplete(Renderer *renderer,
                                                RefCountedEventsGarbageRecycler *recycler)
{
    if (!renderer->hasQueueSerialFinished(mQueueSerial))
    {
        return false;
    }

    while (!mRefCountedEvents.empty())
    {
        ASSERT(mRefCountedEvents.back().valid());
        mRefCountedEvents.back().releaseImpl(renderer, recycler);
        ASSERT(!mRefCountedEvents.back().valid());
        mRefCountedEvents.pop_back();
    }
    return true;
}

void RefCountedEventsGarbage::destroy(Renderer *renderer)
{
    ASSERT(renderer->hasQueueSerialFinished(mQueueSerial));
    while (!mRefCountedEvents.empty())
    {
        ASSERT(mRefCountedEvents.back().valid());
        mRefCountedEvents.back().release(renderer);
        mRefCountedEvents.pop_back();
    }
}

// RefCountedEventsGarbageRecycler implementation.
RefCountedEventsGarbageRecycler::~RefCountedEventsGarbageRecycler()
{
    ASSERT(mFreeStack.empty());
    ASSERT(mGarbageQueue.empty());
}

void RefCountedEventsGarbageRecycler::destroy(Renderer *renderer)
{
    while (!mGarbageQueue.empty())
    {
        mGarbageQueue.front().destroy(renderer);
        mGarbageQueue.pop();
    }

    mFreeStack.destroy(renderer->getDevice());
}

void RefCountedEventsGarbageRecycler::cleanup(Renderer *renderer)
{
    // Destroy free stack first. The garbage clean up process will add more events to the free
    // stack. If everything is stable between each frame, grabage should release enough events to
    // recycler for next frame's needs.
    mFreeStack.destroy(renderer->getDevice());

    while (!mGarbageQueue.empty())
    {
        size_t count  = mGarbageQueue.front().size();
        bool released = mGarbageQueue.front().releaseIfComplete(renderer, this);
        if (released)
        {
            mGarbageCount -= count;
            mGarbageQueue.pop();
        }
        else
        {
            break;
        }
    }
}

bool RefCountedEventsGarbageRecycler::fetch(RefCountedEvent *outObject)
{
    if (!mFreeStack.empty())
    {
        mFreeStack.fetch(outObject);
        ASSERT(outObject->valid());
        ASSERT(!outObject->mHandle->isReferenced());
        return true;
    }
    return false;
}

// EventBarrier implementation.
bool EventBarrier::hasEvent(const VkEvent &event) const
{
    for (const VkEvent &existingEvent : mEvents)
    {
        if (existingEvent == event)
        {
            return true;
        }
    }
    return false;
}

void EventBarrier::addDiagnosticsString(std::ostringstream &out) const
{
    if (mMemoryBarrierSrcAccess != 0 || mMemoryBarrierDstAccess != 0)
    {
        out << "Src: 0x" << std::hex << mMemoryBarrierSrcAccess << " &rarr; Dst: 0x" << std::hex
            << mMemoryBarrierDstAccess << std::endl;
    }
}

void EventBarrier::execute(PrimaryCommandBuffer *primary)
{
    if (isEmpty())
    {
        return;
    }

    // Issue vkCmdWaitEvents call
    VkMemoryBarrier memoryBarrier = {};
    uint32_t memoryBarrierCount   = 0;
    if (mMemoryBarrierDstAccess != 0)
    {
        memoryBarrier.sType         = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        memoryBarrier.srcAccessMask = mMemoryBarrierSrcAccess;
        memoryBarrier.dstAccessMask = mMemoryBarrierDstAccess;
        memoryBarrierCount++;
    }

    primary->waitEvents(static_cast<uint32_t>(mEvents.size()), mEvents.data(), mSrcStageMask,
                        mDstStageMask, memoryBarrierCount, &memoryBarrier, 0, nullptr,
                        static_cast<uint32_t>(mImageMemoryBarriers.size()),
                        mImageMemoryBarriers.data());

    reset();
}

// EventBarrierArray implementation.
void EventBarrierArray::addMemoryEvent(Context *context,
                                       const RefCountedEvent &waitEvent,
                                       VkPipelineStageFlags dstStageMask,
                                       VkAccessFlags dstAccess)
{
    ASSERT(waitEvent.valid());

    for (EventBarrier &barrier : mBarriers)
    {
        // If the event is already in the waiting list, just add the new stageMask to the
        // dstStageMask. Otherwise we will end up with two waitEvent calls that wait for the same
        // VkEvent but for different dstStage and confuses VVL.
        if (barrier.hasEvent(waitEvent.getEvent().getHandle()))
        {
            barrier.addAdditionalStageAccess(dstStageMask, dstAccess);
            return;
        }
    }

    VkAccessFlags accessMask;
    VkPipelineStageFlags stageFlags = GetRefCountedEventStageMask(context, waitEvent, &accessMask);
    // Since this is used with WAW without layout change, dstStageMask should be the same as event's
    // stageMask. Otherwise you should get into addImageEvent.
    ASSERT(stageFlags == dstStageMask && accessMask == dstAccess);
    mBarriers.emplace_back(stageFlags, dstStageMask, accessMask, dstAccess,
                           waitEvent.getEvent().getHandle());
}

void EventBarrierArray::addImageEvent(Context *context,
                                      const RefCountedEvent &waitEvent,
                                      VkPipelineStageFlags dstStageMask,
                                      const VkImageMemoryBarrier &imageMemoryBarrier)
{
    ASSERT(waitEvent.valid());
    VkPipelineStageFlags srcStageFlags = GetRefCountedEventStageMask(context, waitEvent);

    mBarriers.emplace_back();
    EventBarrier &barrier = mBarriers.back();
    // VkCmdWaitEvent must uses the same stageMask as VkCmdSetEvent due to
    // VUID-vkCmdWaitEvents-srcStageMask-01158 requirements.
    barrier.mSrcStageMask = srcStageFlags;
    // If there is an event, we use the waitEvent to do layout change.
    barrier.mEvents.emplace_back(waitEvent.getEvent().getHandle());
    barrier.mDstStageMask = dstStageMask;
    barrier.mImageMemoryBarriers.emplace_back(imageMemoryBarrier);
}

void EventBarrierArray::execute(Renderer *renderer, PrimaryCommandBuffer *primary)
{
    if (mBarriers.empty())
    {
        return;
    }

    for (EventBarrier &barrier : mBarriers)
    {
        barrier.execute(primary);
    }
    mBarriers.clear();
}

void EventBarrierArray::addDiagnosticsString(std::ostringstream &out) const
{
    out << "Event Barrier: ";
    for (const EventBarrier &barrier : mBarriers)
    {
        barrier.addDiagnosticsString(out);
    }
    out << "\\l";
}
}  // namespace vk
}  // namespace rx

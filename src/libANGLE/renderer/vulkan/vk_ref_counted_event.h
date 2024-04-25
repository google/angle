//
// Copyright 2024 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RefCountedEvent:
//    Manages reference count of VkEvent and its associated functions.
//

#ifndef LIBANGLE_RENDERER_VULKAN_REFCOUNTED_EVENT_H_
#define LIBANGLE_RENDERER_VULKAN_REFCOUNTED_EVENT_H_

#include <atomic>
#include <limits>
#include <queue>

#include "common/PackedEnums.h"
#include "common/debug.h"
#include "libANGLE/renderer/serial_utils.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"
#include "libANGLE/renderer/vulkan/vk_wrapper.h"

namespace rx
{
namespace vk
{
enum class ImageLayout;

// There are two ways to implement a barrier: Using VkCmdPipelineBarrier or VkCmdWaitEvents. The
// BarrierType enum will be passed around to indicate which barrier caller want to use.
enum class BarrierType
{
    Pipeline,
    Event,
};

// VkCmdWaitEvents requires srcStageMask must be the bitwise OR of the stageMask parameter used in
// previous calls to vkCmdSetEvent (See VUID-vkCmdWaitEvents-srcStageMask-01158). This mean we must
// keep the record of what stageMask each event has been used in VkCmdSetEvent call so that we can
// retrieve that information when we need to wait for the event. Instead of keeping just stageMask
// here, we keep the ImageLayout for now which gives us more information for debugging.
struct EventAndLayout
{
    bool valid() const { return event.valid(); }
    Event event;
    ImageLayout imageLayout;
};

// The VkCmdSetEvent is called after VkCmdEndRenderPass and all images that used at the given
// pipeline stage (i.e, they have the same stageMask) will be tracked by the same event. This means
// there will be multiple objects pointing to the same event. Events are thus reference counted so
// that we do not destroy it while other objects still referencing to it.
using RefCountedEventAndLayoutHandle = AtomicRefCounted<EventAndLayout> *;

void ReleaseRefcountedEvent(VkDevice device, RefCountedEventAndLayoutHandle atomicRefCountedEvent);

// Wrapper for RefCountedEventAndLayoutHandle.
class RefCountedEvent final : public WrappedObject<RefCountedEvent, RefCountedEventAndLayoutHandle>
{
  public:
    RefCountedEvent() = default;

    // Move constructor moves reference of the underline object from other to this.
    RefCountedEvent(RefCountedEvent &&other)
    {
        mHandle       = other.mHandle;
        other.mHandle = nullptr;
    }

    // Copy constructor adds reference to the underline object.
    RefCountedEvent(const RefCountedEvent &other)
    {
        mHandle = other.mHandle;
        if (mHandle != nullptr)
        {
            mHandle->addRef();
        }
    }

    // Move assignment moves reference of the underline object from other to this.
    RefCountedEvent &operator=(RefCountedEvent &&other)
    {
        ASSERT(!valid());
        std::swap(mHandle, other.mHandle);
        return *this;
    }

    // Copy assignment adds reference to the underline object.
    RefCountedEvent &operator=(const RefCountedEvent &other)
    {
        ASSERT(!valid());
        ASSERT(other.valid());
        mHandle = other.mHandle;
        mHandle->addRef();
        return *this;
    }

    // Returns true if both points to the same underline object.
    bool operator==(const RefCountedEvent &other) const { return mHandle == other.mHandle; }

    // Create VkEvent and associated it with given layout
    void init(Context *context, ImageLayout layout);

    // Release one reference count to the underline Event object and destroy if this is the
    // very last reference.
    void release(VkDevice device)
    {
        if (!valid())
        {
            return;
        }
        ReleaseRefcountedEvent(device, mHandle);
        mHandle = nullptr;
    }

    bool valid() const { return mHandle != nullptr; }

    // Returns the underlying Event object
    const Event &getEvent() const
    {
        ASSERT(valid());
        return mHandle->get().event;
    }

    // Returns the ImageLayout associated with the event.
    ImageLayout getImageLayout() const
    {
        ASSERT(valid());
        return mHandle->get().imageLayout;
    }
};

template <>
struct HandleTypeHelper<RefCountedEvent>
{
    constexpr static HandleType kHandleType = HandleType::RefCountedEvent;
};

// This class tracks a vector of RefcountedEvent garbage. For performance reason, instead of
// individually tracking each VkEvent garbage, we collect all events that are accessed in the
// CommandBufferHelper into this class. After we submit the command buffer, we treat this vector of
// events as one garbage object and add it to renderer's garbage list. The garbage clean up will
// decrement the refCount and destroy event only when last refCount goes away. Basically all GPU
// usage will use one refCount and that refCount ensures we never destroy event until GPU is
// finished.
class RefCountedEventGarbageObjects final
{
  public:
    // Move event to the garbage list
    void add(RefCountedEvent *event);
    // Move the vector of events to the garbage list
    void add(std::vector<RefCountedEvent> *events);

    bool empty() const { return mGarbageObjects.empty(); }

    GarbageObjects &&release() { return std::move(mGarbageObjects); }

  private:
    GarbageObjects mGarbageObjects;
};
}  // namespace vk
}  // namespace rx
#endif  // LIBANGLE_RENDERER_VULKAN_REFCOUNTED_EVENT_H_

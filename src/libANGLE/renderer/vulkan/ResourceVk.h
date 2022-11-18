//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ResourceVk:
//    Resource lifetime tracking in the Vulkan back-end.
//

#ifndef LIBANGLE_RENDERER_VULKAN_RESOURCEVK_H_
#define LIBANGLE_RENDERER_VULKAN_RESOURCEVK_H_

#include "libANGLE/HandleAllocator.h"
#include "libANGLE/renderer/vulkan/vk_utils.h"

#include <queue>

namespace rx
{
namespace vk
{
// We expect almost all reasonable usage case should have at most 4 current contexts now. When
// exceeded, it should still work, but storage will grow.
static constexpr size_t kMaxFastQueueSerials = 4;
// Serials is an array of queue serials, which when paired with the index of the serials in the
// array result in QueueSerials. The array may expand if needed. Since it owned by Resource object
// which is protected by shared lock, it is safe to reallocate storage if needed. When it passes to
// renderer at garbage collection time, we will make a copy. The array size is expected to be small.
// But in future if we run into situation that array size is too big, we can change to packed array
// of QueueSerials.
using Serials = angle::FastVector<Serial, kMaxFastQueueSerials>;

// Tracks how a resource is used by ANGLE and by a VkQueue. The serial indicates the most recent use
// of a resource in the VkQueue. We use the monotonically incrementing serial number to determine if
// a resource is currently in use.
class ResourceUse final
{
  public:
    ResourceUse()  = default;
    ~ResourceUse() = default;

    ResourceUse(const QueueSerial &queueSerial) { setQueueSerial(queueSerial); }
    ResourceUse(const Serials &otherSerials) { mSerials = otherSerials; }

    // Copy constructor
    ResourceUse(const ResourceUse &other) : mSerials(other.mSerials) {}
    ResourceUse &operator=(const ResourceUse &other)
    {
        mSerials = other.mSerials;
        return *this;
    }

    // Move constructor
    ResourceUse(ResourceUse &&other) : mSerials(other.mSerials) { other.mSerials.clear(); }
    ResourceUse &operator=(ResourceUse &&other)
    {
        mSerials = other.mSerials;
        other.mSerials.clear();
        return *this;
    }

    ANGLE_INLINE bool valid() const { return mSerials.size() > 0; }

    ANGLE_INLINE void reset() { mSerials.clear(); }

    ANGLE_INLINE const Serials &getSerials() const { return mSerials; }

    ANGLE_INLINE void setSerial(SerialIndex index, Serial serial)
    {
        ASSERT(index != kInvalidQueueSerialIndex);
        ASSERT(serial.valid());
        if (mSerials.size() <= index)
        {
            mSerials.resize(index + 1);
        }
        mSerials[index] = serial;
    }

    ANGLE_INLINE void setQueueSerial(const QueueSerial &queueSerial)
    {
        setSerial(queueSerial.getIndex(), queueSerial.getSerial());
    }

    // Returns true if there is at least one serial is greater than
    bool operator>(const AtomicQueueSerialFixedArray &serials) const
    {
        ASSERT(mSerials.size() <= serials.size());
        for (SerialIndex i = 0; i < mSerials.size(); ++i)
        {
            if (mSerials[i] > serials[i])
            {
                return true;
            }
        }
        return false;
    }

    // Returns true if it contains a serial that is greater than
    bool operator>(const QueueSerial &queuSerial) const
    {
        return mSerials.size() > queuSerial.getIndex() &&
               mSerials[queuSerial.getIndex()] > queuSerial.getSerial();
    }

    ANGLE_INLINE bool usedByCommandBuffer(const QueueSerial &commandBufferQueueSerial) const
    {
        // Return true if we have the exact queue serial in the array.
        return commandBufferQueueSerial.valid() &&
               mSerials.size() > commandBufferQueueSerial.getIndex() &&
               mSerials[commandBufferQueueSerial.getIndex()] ==
                   commandBufferQueueSerial.getSerial();
    }

  private:
    // The most recent time of use in a VkQueue.
    Serials mSerials;
};

class SharedGarbage
{
  public:
    SharedGarbage();
    SharedGarbage(SharedGarbage &&other);
    SharedGarbage(const ResourceUse &use, GarbageList &&garbage);
    ~SharedGarbage();
    SharedGarbage &operator=(SharedGarbage &&rhs);

    bool destroyIfComplete(RendererVk *renderer);
    bool hasUnsubmittedUse(RendererVk *renderer) const;

  private:
    ResourceUse mLifetime;
    GarbageList mGarbage;
};

using SharedGarbageList = std::queue<SharedGarbage>;

// This is a helper class for back-end objects used in Vk command buffers. They keep a record
// of their use in ANGLE and VkQueues via ResourceUse.
class Resource : angle::NonCopyable
{
  public:
    virtual ~Resource();

    // Determine if the driver has finished execution with this resource.
    bool usedInRunningCommands(RendererVk *renderer) const;

    // Returns true if the resource is in use by ANGLE or the driver.
    bool isCurrentlyInUse(RendererVk *renderer) const;

    // Ensures the driver is caught up to this resource and it is only in use by ANGLE.
    angle::Result finishRunningCommands(ContextVk *contextVk);

    // Complete all recorded and in-flight commands involving this resource
    angle::Result waitForIdle(ContextVk *contextVk,
                              const char *debugMessage,
                              RenderPassClosureReason reason);

    // Adds the resource to the list and also records command buffer use.
    void retainCommands(const QueueSerial &queueSerial);

    // Check if this resource is used by a command buffer.
    bool usedByCommandBuffer(const QueueSerial &commandBufferQueueSerial) const
    {
        return mUse.usedByCommandBuffer(commandBufferQueueSerial);
    }

    const ResourceUse &getResourceUse() const { return mUse; }

  protected:
    Resource();
    Resource(Resource &&other);
    Resource &operator=(Resource &&rhs);

    // Current resource lifetime.
    ResourceUse mUse;
};

ANGLE_INLINE void Resource::retainCommands(const QueueSerial &queueSerial)
{
    mUse.setQueueSerial(queueSerial);
}

// Similar to |Resource| above, this tracks object usage. This includes additional granularity to
// track whether an object is used for read-only or read/write access.
class ReadWriteResource : public angle::NonCopyable
{
  public:
    virtual ~ReadWriteResource();

    // Determine if the driver has finished execution with this resource.
    bool usedInRunningCommands(RendererVk *renderer) const;

    // Returns true if the resource is in use by ANGLE or the driver.
    bool isCurrentlyInUse(RendererVk *renderer) const;
    bool isCurrentlyInUseForWrite(RendererVk *renderer) const;

    // Ensures the driver is caught up to this resource and it is only in use by ANGLE.
    angle::Result finishRunningCommands(ContextVk *contextVk);

    // Ensures the GPU write commands is completed.
    angle::Result finishGPUWriteCommands(ContextVk *contextVk);

    // Complete all recorded and in-flight commands involving this resource
    angle::Result waitForIdle(ContextVk *contextVk,
                              const char *debugMessage,
                              RenderPassClosureReason reason);

    // Adds the resource to a resource use list.
    void retainReadOnly(const QueueSerial &queueSerial);
    void retainReadWrite(const QueueSerial &queueSerial);

    // Check if this resource is used by a command buffer.
    bool usedByCommandBuffer(const QueueSerial &commandBufferQueueSerial) const;
    bool writtenByCommandBuffer(const QueueSerial &commandBufferQueueSerial) const;

    const ResourceUse &getResourceUse() const { return mReadOnlyUse; }
    const ResourceUse &getWriteResourceUse() const { return mReadWriteUse; }

  protected:
    ReadWriteResource();
    ReadWriteResource(ReadWriteResource &&other);
    ReadWriteResource &operator=(ReadWriteResource &&other);

    // Track any use of the object. Always updated on every retain call.
    ResourceUse mReadOnlyUse;
    // Track read/write use of the object. Only updated for retainReadWrite().
    ResourceUse mReadWriteUse;
};

ANGLE_INLINE void ReadWriteResource::retainReadOnly(const QueueSerial &queueSerial)
{
    mReadOnlyUse.setQueueSerial(queueSerial);
}

ANGLE_INLINE void ReadWriteResource::retainReadWrite(const QueueSerial &queueSerial)
{
    mReadOnlyUse.setQueueSerial(queueSerial);
    mReadWriteUse.setQueueSerial(queueSerial);
}

ANGLE_INLINE bool ReadWriteResource::usedByCommandBuffer(
    const QueueSerial &commandBufferQueueSerial) const
{
    return mReadOnlyUse.usedByCommandBuffer(commandBufferQueueSerial);
}

ANGLE_INLINE bool ReadWriteResource::writtenByCommandBuffer(
    const QueueSerial &commandBufferQueueSerial) const
{
    return mReadWriteUse.usedByCommandBuffer(commandBufferQueueSerial);
}
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RESOURCEVK_H_

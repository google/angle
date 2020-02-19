//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandGraph:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#ifndef LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_
#define LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_

#include "libANGLE/renderer/vulkan/SecondaryCommandBuffer.h"
#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{

namespace vk
{
class CommandGraph;

// Receives notifications when a render pass command buffer is no longer able to record. Can be
// used with inheritance. Faster than using an interface class since it has inlined methods. Could
// be used with composition by adding a getCommandBuffer method.
class RenderPassOwner
{
  public:
    RenderPassOwner() = default;
    virtual ~RenderPassOwner() {}

    ANGLE_INLINE void onRenderPassFinished() { mRenderPassCommandBuffer = nullptr; }

  protected:
    CommandBuffer *mRenderPassCommandBuffer = nullptr;
};

// Tracks how a resource is used in a command graph and in a VkQueue. The reference count indicates
// the number of times a resource is used in the graph. The serial indicates the last current use
// of a resource in the VkQueue. The reference count and serial together can determine if a
// resource is in use.
struct ResourceUse
{
    ResourceUse() = default;

    uint32_t counter = 0;
    Serial serial;
};

class SharedResourceUse final : angle::NonCopyable
{
  public:
    SharedResourceUse() : mUse(nullptr) {}
    ~SharedResourceUse() { ASSERT(!valid()); }
    SharedResourceUse(SharedResourceUse &&rhs) : mUse(rhs.mUse) { rhs.mUse = nullptr; }
    SharedResourceUse &operator=(SharedResourceUse &&rhs)
    {
        std::swap(mUse, rhs.mUse);
        return *this;
    }

    ANGLE_INLINE bool valid() const { return mUse != nullptr; }

    void init()
    {
        ASSERT(!mUse);
        mUse = new ResourceUse;
        mUse->counter++;
    }

    // Specifically for use with command buffers that are used as one-offs.
    void updateSerialOneOff(Serial serial) { mUse->serial = serial; }

    ANGLE_INLINE void release()
    {
        ASSERT(valid());
        ASSERT(mUse->counter > 0);
        if (--mUse->counter == 0)
        {
            delete mUse;
        }
        mUse = nullptr;
    }

    ANGLE_INLINE void releaseAndUpdateSerial(Serial serial)
    {
        ASSERT(valid());
        ASSERT(mUse->counter > 0);
        ASSERT(mUse->serial <= serial);
        mUse->serial = serial;
        release();
    }

    ANGLE_INLINE void set(const SharedResourceUse &rhs)
    {
        ASSERT(rhs.valid());
        ASSERT(!valid());
        ASSERT(rhs.mUse->counter < std::numeric_limits<uint32_t>::max());
        mUse = rhs.mUse;
        mUse->counter++;
    }

    // The base counter value for a live resource is "1". Any value greater than one indicates
    // the resource is in use by a vk::CommandGraph.
    ANGLE_INLINE bool hasRecordedCommands() const
    {
        ASSERT(valid());
        return mUse->counter > 1;
    }

    ANGLE_INLINE bool hasRunningCommands(Serial lastCompletedSerial) const
    {
        ASSERT(valid());
        return mUse->serial > lastCompletedSerial;
    }

    ANGLE_INLINE bool isCurrentlyInUse(Serial lastCompletedSerial) const
    {
        return hasRecordedCommands() || hasRunningCommands(lastCompletedSerial);
    }

    ANGLE_INLINE Serial getSerial() const
    {
        ASSERT(valid());
        return mUse->serial;
    }

  private:
    ResourceUse *mUse;
};

class SharedGarbage
{
  public:
    SharedGarbage();
    SharedGarbage(SharedGarbage &&other);
    SharedGarbage(SharedResourceUse &&use, std::vector<GarbageObject> &&garbage);
    ~SharedGarbage();
    SharedGarbage &operator=(SharedGarbage &&rhs);

    bool destroyIfComplete(VkDevice device, Serial completedSerial);

  private:
    SharedResourceUse mLifetime;
    std::vector<GarbageObject> mGarbage;
};

using SharedGarbageList = std::vector<SharedGarbage>;

// Mixin to abstract away the resource use tracking.
class ResourceUseList final : angle::NonCopyable
{
  public:
    ResourceUseList();
    virtual ~ResourceUseList();

    void add(const SharedResourceUse &resourceUse);

    void releaseResourceUses();
    void releaseResourceUsesAndUpdateSerials(Serial serial);

  private:
    std::vector<SharedResourceUse> mResourceUses;
};

// ResourceUser inlines.
ANGLE_INLINE void ResourceUseList::add(const SharedResourceUse &resourceUse)
{
    // Disabled the assert because of difficulties with ImageView references.
    // TODO(jmadill): Clean up with graph redesign. http://anglebug.com/4029
    // ASSERT(!empty());
    SharedResourceUse newUse;
    newUse.set(resourceUse);
    mResourceUses.emplace_back(std::move(newUse));
}

// This is a helper class for back-end objects used in Vk command buffers. It records a serial
// at command recording times indicating an order in the queue. We use Fences to detect when
// commands finish, and then release any unreferenced and deleted resources based on the stored
// queue serial in a special 'garbage' queue. Resources also track current read and write
// dependencies. Only one command buffer node can be writing to the Resource at a time, but many
// can be reading from it. Together the dependencies will form a command graph at submission time.
class CommandGraphResource : angle::NonCopyable
{
  public:
    virtual ~CommandGraphResource();

    // Returns true if the resource has commands in the graph.  This is used to know if a flush
    // should be performed, e.g. if we need to wait for the GPU to finish with the resource.
    bool hasRecordedCommands() const { return mUse.hasRecordedCommands(); }

    // Determine if the driver has finished execution with this resource.
    bool hasRunningCommands(Serial lastCompletedSerial) const
    {
        return mUse.hasRunningCommands(lastCompletedSerial);
    }

    // Returns true if the resource is in use by ANGLE or the driver.
    bool isCurrentlyInUse(Serial lastCompletedSerial) const
    {
        return mUse.isCurrentlyInUse(lastCompletedSerial);
    }

    // Ensures the driver is caught up to this resource and it is only in use by ANGLE.
    angle::Result finishRunningCommands(ContextVk *contextVk);

    // Updates the in-use serial tracked for this resource. Will clear dependencies if the resource
    // was not used in this set of command nodes.
    // TODO(jmadill): Merge and rename. http://anglebug.com/4029
    void onResourceAccess(ResourceUseList *resourceUseList);

    // If a resource is recreated, as in released and reinitialized, the next access to the
    // resource will not create an edge from its last node and will create a new independent node.
    // This is because mUse is reset and the graph believes it's an entirely new resource.  In very
    // particular cases, such as recreating an image with full mipchain or adding STORAGE_IMAGE flag
    // to its uses, this function is used to preserve the link between the previous and new
    // nodes allocated for this resource.
    // TODO(jmadill): Merge and rename. http://anglebug.com/4029
    void onResourceRecreated(ResourceUseList *resourceUseList);

  protected:
    CommandGraphResource();

    // Current resource lifetime.
    SharedResourceUse mUse;
};

ANGLE_INLINE void CommandGraphResource::onResourceRecreated(ResourceUseList *resourceUseList)
{
    // Store reference in resource list.
    resourceUseList->add(mUse);
}

ANGLE_INLINE void CommandGraphResource::onResourceAccess(ResourceUseList *resourceUseList)
{
    // Store reference in resource list.
    resourceUseList->add(mUse);
}
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_

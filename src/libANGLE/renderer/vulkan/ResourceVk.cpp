//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Resource:
//    Resource lifetime tracking in the Vulkan back-end.
//

#include "libANGLE/renderer/vulkan/ResourceVk.h"

#include "libANGLE/renderer/vulkan/ContextVk.h"

namespace rx
{
namespace vk
{
// Resource implementation.
Resource::Resource()
{
    mUse.init();
}

Resource::Resource(Resource &&other) : Resource()
{
    mUse = std::move(other.mUse);
}

Resource::~Resource()
{
    mUse.release();
}

angle::Result Resource::finishRunningCommands(ContextVk *contextVk)
{
    return contextVk->finishToSerial(mUse.getSerial());
}

angle::Result Resource::waitForIdle(ContextVk *contextVk, const char *debugMessage)
{
    // If there are pending commands for the resource, flush them.
    if (usedInRecordedCommands())
    {
        ANGLE_TRY(contextVk->flushImpl(nullptr));
    }

    // Make sure the driver is done with the resource.
    if (usedInRunningCommands(contextVk->getLastCompletedQueueSerial()))
    {
        if (debugMessage)
        {
            ANGLE_PERF_WARNING(contextVk->getDebug(), GL_DEBUG_SEVERITY_HIGH, debugMessage);
        }
        ANGLE_TRY(finishRunningCommands(contextVk));
    }

    ASSERT(!isCurrentlyInUse(contextVk->getLastCompletedQueueSerial()));

    return angle::Result::Continue;
}

// SharedResourceUsePool implementation.
SharedResourceUsePool::SharedResourceUsePool() {}

SharedResourceUsePool::~SharedResourceUsePool() = default;

void SharedResourceUsePool::ensureCapacity()
{
    // Allocate a SharedResourceUse block
    constexpr size_t kSharedResourceUseBlockSize = 2048;
    size_t newSize                               = (mSharedResourceUsePool.empty())
                         ? kSharedResourceUseBlockSize
                         : mSharedResourceUsePool.back().capacity() * 2;
    SharedResourceUseBlock sharedResourceUseBlock;
    sharedResourceUseBlock.resize(newSize);

    // Append it to the SharedResourceUse pool
    mSharedResourceUsePool.emplace_back(std::move(sharedResourceUseBlock));

    // Add the newly allocated SharedResourceUse to the free list
    mSharedResourceUseFreeList.reserve(newSize);
    SharedResourceUseBlock &newSharedResourceUseBlock = mSharedResourceUsePool.back();
    for (SharedResourceUse &use : newSharedResourceUseBlock)
    {
        mSharedResourceUseFreeList.push_back(&use);
    }
}

// SharedGarbage implementation.
SharedGarbage::SharedGarbage() = default;

SharedGarbage::SharedGarbage(SharedGarbage &&other)
{
    *this = std::move(other);
}

SharedGarbage::SharedGarbage(SharedResourceUse &&use, std::vector<GarbageObject> &&garbage)
    : mLifetime(std::move(use)), mGarbage(std::move(garbage))
{}

SharedGarbage::~SharedGarbage() = default;

SharedGarbage &SharedGarbage::operator=(SharedGarbage &&rhs)
{
    std::swap(mLifetime, rhs.mLifetime);
    std::swap(mGarbage, rhs.mGarbage);
    return *this;
}

bool SharedGarbage::destroyIfComplete(RendererVk *renderer, Serial completedSerial)
{
    if (mLifetime.isCurrentlyInUse(completedSerial))
        return false;

    for (GarbageObject &object : mGarbage)
    {
        object.destroy(renderer);
    }

    mLifetime.release();

    return true;
}

// ResourceUseList implementation.
ResourceUseList::ResourceUseList()
{
    constexpr size_t kDefaultResourceUseCount = 4096;
    mResourceUses.reserve(kDefaultResourceUseCount);
}

ResourceUseList::~ResourceUseList()
{
    ASSERT(mResourceUses.empty());
}

void ResourceUseList::releaseResourceUses(SharedResourceUsePool *sharedResourceUsePool)
{
    for (SharedResourceUse *use : mResourceUses)
    {
        use->release();
        sharedResourceUsePool->releaseSharedResouceUse(use);
    }
    mResourceUses.clear();
}

void ResourceUseList::releaseResourceUsesAndUpdateSerials(
    Serial serial,
    SharedResourceUsePool *sharedResourceUsePool)
{
    for (SharedResourceUse *use : mResourceUses)
    {
        use->releaseAndUpdateSerial(serial);
        sharedResourceUsePool->releaseSharedResouceUse(use);
    }
    mResourceUses.clear();
}
}  // namespace vk
}  // namespace rx

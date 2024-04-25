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

void ReleaseRefcountedEvent(VkDevice device, RefCountedEventAndLayoutHandle atomicRefCountedEvent)
{
    const bool isLastReference = atomicRefCountedEvent->getAndReleaseRef() == 1;
    if (isLastReference)
    {
        atomicRefCountedEvent->get().event.destroy(device);
        SafeDelete(atomicRefCountedEvent);
    }
}

void RefCountedEvent::init(Context *context, ImageLayout layout)
{
    ASSERT(mHandle == nullptr);
    ASSERT(layout != ImageLayout::Undefined);

    mHandle                      = new AtomicRefCounted<EventAndLayout>;
    VkEventCreateInfo createInfo = {};
    createInfo.sType             = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
    createInfo.flags             = 0;
    mHandle->get().event.init(context->getDevice(), createInfo);
    mHandle->addRef();
    mHandle->get().imageLayout = layout;
}

// RefCountedEventGarbageObjects implementation
void RefCountedEventGarbageObjects::add(RefCountedEvent *event)
{
    mGarbageObjects.emplace_back(GetGarbage(event));
}

void RefCountedEventGarbageObjects::add(std::vector<RefCountedEvent> *events)
{
    while (!events->empty())
    {
        mGarbageObjects.emplace_back(GetGarbage(&events->back()));
        events->pop_back();
    }
}
}  // namespace vk
}  // namespace rx

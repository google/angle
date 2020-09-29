//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DebugAnnotatorVk.cpp: Vulkan helpers for adding trace annotations.
//

#include "libANGLE/renderer/vulkan/DebugAnnotatorVk.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"

namespace rx
{

DebugAnnotatorVk::DebugAnnotatorVk() {}

DebugAnnotatorVk::~DebugAnnotatorVk() {}

void DebugAnnotatorVk::beginEvent(gl::Context *context,
                                  const char *eventName,
                                  const char *eventMessage)
{
    angle::LoggingAnnotator::beginEvent(context, eventName, eventMessage);
    if (context)
    {
        ContextVk *contextVk = vk::GetImpl(static_cast<gl::Context *>(context));
        contextVk->logEvent(eventMessage);
    }
}

void DebugAnnotatorVk::endEvent(const char *eventName)
{
    angle::LoggingAnnotator::endEvent(eventName);
}

bool DebugAnnotatorVk::getStatus()
{
    return true;
}

}  // namespace rx

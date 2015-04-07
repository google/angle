// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "common/event_tracer.h"

#include "common/debug.h"
#include "platform/Platform.h"

namespace gl
{

GetCategoryEnabledFlagFunc g_getCategoryEnabledFlag;
AddTraceEventFunc g_addTraceEvent;

}  // namespace gl

namespace gl
{

const unsigned char *TraceGetTraceCategoryEnabledFlag(const char *name)
{
    angle::Platform *platform = ANGLEPlatformCurrent();
    ASSERT(platform);

    // TODO(jmadill): only use platform once it's working
    const unsigned char *categoryEnabledFlag = platform->getTraceCategoryEnabledFlag(name);
    if (categoryEnabledFlag != nullptr)
    {
        return categoryEnabledFlag;
    }

    if (g_getCategoryEnabledFlag)
    {
        return g_getCategoryEnabledFlag(name);
    }
    static unsigned char disabled = 0;
    return &disabled;
}

void TraceAddTraceEvent(char phase, const unsigned char* categoryGroupEnabled, const char* name, unsigned long long id,
                        int numArgs, const char** argNames, const unsigned char* argTypes,
                        const unsigned long long* argValues, unsigned char flags)
{
    angle::Platform *platform = ANGLEPlatformCurrent();
    ASSERT(platform);

    // TODO(jmadill): only use platform once it's working
    double timestamp = platform->monotonicallyIncreasingTime();

    if (timestamp != 0)
    {
        angle::Platform::TraceEventHandle handle =
            platform->addTraceEvent(phase,
                                    categoryGroupEnabled,
                                    name,
                                    id,
                                    timestamp,
                                    numArgs,
                                    argNames,
                                    argTypes,
                                    argValues,
                                    flags);
        ASSERT(handle != 0);
        UNUSED_ASSERTION_VARIABLE(handle);
    }
    else if (g_addTraceEvent)
    {
        g_addTraceEvent(phase, categoryGroupEnabled, name, id, numArgs, argNames, argTypes, argValues, flags);
    }
}

}  // namespace gl

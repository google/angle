//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Platform.cpp: Implementation methods for angle::Platform.

#include <platform/Platform.h>

#include <cstring>

#include "common/debug.h"

namespace
{
angle::Platform *currentPlatform = nullptr;

// TODO(jmadill): Make methods owned by egl::Display.
angle::PlatformMethods g_platformMethods;

// TODO(jmadill): Remove all the Class_ methods once we switch Chromium to the new impl.
double Class_currentTime(angle::PlatformMethods *platform)
{
    return currentPlatform->currentTime();
}

double Class_monotonicallyIncreasingTime(angle::PlatformMethods *platform)
{
    return currentPlatform->monotonicallyIncreasingTime();
}

void Class_logError(angle::PlatformMethods *platform, const char *errorMessage)
{
    currentPlatform->logError(errorMessage);
}

void Class_logWarning(angle::PlatformMethods *platform, const char *warningMessage)
{
    currentPlatform->logWarning(warningMessage);
}

void Class_logInfo(angle::PlatformMethods *platform, const char *infoMessage)
{
    currentPlatform->logInfo(infoMessage);
}

const unsigned char *Class_getTraceCategoryEnabledFlag(angle::PlatformMethods *platform,
                                                       const char *categoryName)
{
    return currentPlatform->getTraceCategoryEnabledFlag(categoryName);
}

angle::TraceEventHandle Class_addTraceEvent(angle::PlatformMethods *platform,
                                            char phase,
                                            const unsigned char *categoryEnabledFlag,
                                            const char *name,
                                            unsigned long long id,
                                            double timestamp,
                                            int numArgs,
                                            const char **argNames,
                                            const unsigned char *argTypes,
                                            const unsigned long long *argValues,
                                            unsigned char flags)
{
    return currentPlatform->addTraceEvent(phase, categoryEnabledFlag, name, id, timestamp, numArgs,
                                          argNames, argTypes, argValues, flags);
}

void Class_updateTraceEventDuration(angle::PlatformMethods *platform,
                                    const unsigned char *categoryEnabledFlag,
                                    const char *name,
                                    angle::TraceEventHandle eventHandle)
{
    currentPlatform->updateTraceEventDuration(categoryEnabledFlag, name, eventHandle);
}

void Class_histogramCustomCounts(angle::PlatformMethods *platform,
                                 const char *name,
                                 int sample,
                                 int min,
                                 int max,
                                 int bucketCount)
{
    currentPlatform->histogramCustomCounts(name, sample, min, max, bucketCount);
}

void Class_histogramEnumeration(angle::PlatformMethods *platform,
                                const char *name,
                                int sample,
                                int boundaryValue)
{
    currentPlatform->histogramEnumeration(name, sample, boundaryValue);
}

void Class_histogramSparse(angle::PlatformMethods *platform, const char *name, int sample)
{
    currentPlatform->histogramSparse(name, sample);
}

void Class_histogramBoolean(angle::PlatformMethods *platform, const char *name, bool sample)
{
    currentPlatform->histogramBoolean(name, sample);
}

void Class_overrideWorkaroundsD3D(angle::PlatformMethods *platform,
                                  angle::WorkaroundsD3D *workaroundsD3D)
{
    currentPlatform->overrideWorkaroundsD3D(workaroundsD3D);
}

}  // anonymous namespace

angle::PlatformMethods *ANGLEPlatformCurrent()
{
    return &g_platformMethods;
}

void ANGLE_APIENTRY ANGLEPlatformInitialize(angle::Platform *platformImpl)
{
    ASSERT(platformImpl != nullptr);
    currentPlatform = platformImpl;

    // TODO(jmadill): Migrate to platform methods.
    g_platformMethods.addTraceEvent               = Class_addTraceEvent;
    g_platformMethods.currentTime                 = Class_currentTime;
    g_platformMethods.getTraceCategoryEnabledFlag = Class_getTraceCategoryEnabledFlag;
    g_platformMethods.histogramBoolean            = Class_histogramBoolean;
    g_platformMethods.histogramCustomCounts       = Class_histogramCustomCounts;
    g_platformMethods.histogramEnumeration        = Class_histogramEnumeration;
    g_platformMethods.histogramSparse             = Class_histogramSparse;
    g_platformMethods.logError                    = Class_logError;
    g_platformMethods.logInfo                     = Class_logInfo;
    g_platformMethods.logWarning                  = Class_logWarning;
    g_platformMethods.monotonicallyIncreasingTime = Class_monotonicallyIncreasingTime;
    g_platformMethods.overrideWorkaroundsD3D      = Class_overrideWorkaroundsD3D;
    g_platformMethods.updateTraceEventDuration    = Class_updateTraceEventDuration;
}

void ANGLE_APIENTRY ANGLEPlatformShutdown()
{
    currentPlatform = nullptr;
    g_platformMethods = angle::PlatformMethods();
}

bool ANGLE_APIENTRY ANGLEGetDisplayPlatform(angle::EGLDisplayType display,
                                            const char *const methodNames[],
                                            unsigned int methodNameCount,
                                            uintptr_t context,
                                            angle::PlatformMethods **platformMethodsOut)
{
    // We allow for a lower input count of impl platform methods if the subset is correct.
    if (methodNameCount > angle::g_NumPlatformMethods)
    {
        ERR() << "Invalid platform method count: " << methodNameCount << ", expected "
              << angle::g_NumPlatformMethods << ".";
        return false;
    }

    for (unsigned int nameIndex = 0; nameIndex < methodNameCount; ++nameIndex)
    {
        const char *expectedName = angle::g_PlatformMethodNames[nameIndex];
        const char *actualName   = methodNames[nameIndex];
        if (strcmp(expectedName, actualName) != 0)
        {
            ERR() << "Invalid platform method name: " << actualName << ", expected " << expectedName
                  << ".";
            return false;
        }
    }

    // TODO(jmadill): Store platform methods in display.
    g_platformMethods.context = context;
    *platformMethodsOut       = &g_platformMethods;
    return true;
}

void ANGLE_APIENTRY ANGLEResetDisplayPlatform(angle::EGLDisplayType display)
{
    // TODO(jmadill): Store platform methods in display.
    g_platformMethods         = angle::PlatformMethods();
    g_platformMethods.context = 0;
}

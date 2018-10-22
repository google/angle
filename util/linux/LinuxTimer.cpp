//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// LinuxTimer.cpp: Implementation of a high precision timer class on Linux

#include "linux/LinuxTimer.h"
#include <iostream>

LinuxTimer::LinuxTimer()
    : mRunning(false)
{
}

namespace
{
uint64_t getCurrentTimeNs()
{
    struct timespec currentTime;
    clock_gettime(CLOCK_MONOTONIC, &currentTime);
    return currentTime.tv_sec * 1'000'000'000llu + currentTime.tv_nsec;
}
}  // anonymous namespace

void LinuxTimer::start()
{
    mStartTimeNs = getCurrentTimeNs();
    mRunning = true;
}

void LinuxTimer::stop()
{
    mStopTimeNs = getCurrentTimeNs();
    mRunning = false;
}

double LinuxTimer::getElapsedTime() const
{
    uint64_t endTimeNs;
    if (mRunning)
    {
        endTimeNs = getCurrentTimeNs();
    }
    else
    {
        endTimeNs = mStopTimeNs;
    }

    return (endTimeNs - mStartTimeNs) * 1e-9;
}

double LinuxTimer::getAbsoluteTime()
{
    return getCurrentTimeNs() * 1e-9;
}

Timer *CreateTimer()
{
    return new LinuxTimer();
}

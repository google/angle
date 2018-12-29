//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// LinuxTimer.h: Definition of a high precision timer class on Linux

#ifndef UTIL_LINUX_TIMER_H
#define UTIL_LINUX_TIMER_H

#include <stdint.h>
#include <time.h>

#include "Timer.h"

class LinuxTimer : public Timer
{
  public:
    LinuxTimer();

    void start() override;
    void stop() override;
    double getElapsedTime() const override;

    double getAbsoluteTime() override;

  private:
    bool mRunning;
    uint64_t mStartTimeNs;
    uint64_t mStopTimeNs;
};

#endif // UTIL_LINUX_TIMER_H

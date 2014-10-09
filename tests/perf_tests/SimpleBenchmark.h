//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef SAMPLE_UTIL_SIMPLE_BENCHMARK_H
#define SAMPLE_UTIL_SIMPLE_BENCHMARK_H

#include <memory>
#include <vector>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <string>

#include "shared_utils.h"

#include "OSWindow.h"
#include "EGLWindow.h"
#include "Timer.h"

class Event;

// Base class
struct BenchmarkParams
{
    EGLint requestedRenderer;

    virtual std::string suffix() const;
};

class SimpleBenchmark
{
  public:
    SimpleBenchmark(const std::string &name, size_t width, size_t height,
                    EGLint glesMajorVersion, const BenchmarkParams &params);

    virtual ~SimpleBenchmark() { };

    virtual bool initializeBenchmark() { return true; }
    virtual void destroyBenchmark() { }

    virtual void stepBenchmark(float dt, double totalTime) { }

    virtual void beginDrawBenchmark() { }
    virtual void drawBenchmark() = 0;
    virtual void endDrawBenchmark() { }

    int run();
    bool popEvent(Event *event);

    OSWindow *getWindow();

  protected:
    void printResult(const std::string &trace, double value, const std::string &units, bool important) const;
    void printResult(const std::string &trace, size_t value, const std::string &units, bool important) const;

    unsigned int mDrawIterations;
    double mRunTimeSeconds;
    int mNumFrames;

  private:
    DISALLOW_COPY_AND_ASSIGN(SimpleBenchmark);

    bool initialize();
    void destroy();

    void step(float dt, double totalTime);
    void draw();

    std::string mName;
    bool mRunning;
    std::string mSuffix;

    std::unique_ptr<EGLWindow> mEGLWindow;
    std::unique_ptr<OSWindow> mOSWindow;
    std::unique_ptr<Timer> mTimer;
};

template <typename BenchmarkT, typename ParamsT>
inline int RunBenchmarks(const std::vector<ParamsT> &benchmarks)
{
    int result;

    for (size_t benchIndex = 0; benchIndex < benchmarks.size(); benchIndex++)
    {
        BenchmarkT benchmark(benchmarks[benchIndex]);
        result = benchmark.run();
        if (result != 0) { return result; }
    }

    return 0;
}

#endif // SAMPLE_UTIL_SIMPLE_BENCHMARK_H

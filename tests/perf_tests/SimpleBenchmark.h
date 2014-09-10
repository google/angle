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

class SimpleBenchmark
{
  public:
    SimpleBenchmark(const std::string &name, size_t width, size_t height,
                    EGLint glesMajorVersion = 2,
                    EGLint requestedRenderer = EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE);

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

    std::unique_ptr<EGLWindow> mEGLWindow;
    std::unique_ptr<OSWindow> mOSWindow;
    std::unique_ptr<Timer> mTimer;
};

// Base class
struct BenchmarkParams
{
    EGLint requestedRenderer;

    virtual std::string name() const
    {
        switch (requestedRenderer)
        {
          case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE: return "D3D11";
          case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE: return "D3D9";
          default: return "Unknown Renderer";
        }
    }
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

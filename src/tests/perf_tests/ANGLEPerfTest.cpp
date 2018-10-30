//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ANGLEPerfTests:
//   Base class for google test performance tests
//

#include "ANGLEPerfTest.h"

#include "third_party/perf/perf_test.h"
#include "third_party/trace_event/trace_event.h"
#include "system_utils.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

#include <json/json.h>

namespace
{
constexpr size_t kInitialTraceEventBufferSize = 50000;
constexpr size_t kWarmupIterations            = 3;
constexpr double kMicroSecondsPerSecond       = 1e6;
constexpr double kNanoSecondsPerSecond        = 1e9;

struct TraceCategory
{
    unsigned char enabled;
    const char *name;
};

constexpr TraceCategory gTraceCategories[2] = {
    {1, "gpu.angle"},
    {1, "gpu.angle.gpu"},
};

void EmptyPlatformMethod(angle::PlatformMethods *, const char *)
{
}

void OverrideWorkaroundsD3D(angle::PlatformMethods *platform, angle::WorkaroundsD3D *workaroundsD3D)
{
    auto *angleRenderTest = static_cast<ANGLERenderTest *>(platform->context);
    angleRenderTest->overrideWorkaroundsD3D(workaroundsD3D);
}

angle::TraceEventHandle AddTraceEvent(angle::PlatformMethods *platform,
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
    // Discover the category name based on categoryEnabledFlag.  This flag comes from the first
    // parameter of TraceCategory, and corresponds to one of the entries in gTraceCategories.
    static_assert(offsetof(TraceCategory, enabled) == 0,
                  "|enabled| must be the first field of the TraceCategory class.");
    const TraceCategory *category = reinterpret_cast<const TraceCategory *>(categoryEnabledFlag);
    ptrdiff_t categoryIndex       = category - gTraceCategories;
    ASSERT(categoryIndex >= 0 && static_cast<size_t>(categoryIndex) < ArraySize(gTraceCategories));

    ANGLERenderTest *renderTest     = static_cast<ANGLERenderTest *>(platform->context);
    std::vector<TraceEvent> &buffer = renderTest->getTraceEventBuffer();
    buffer.emplace_back(phase, category->name, name, timestamp);
    return buffer.size();
}

const unsigned char *GetTraceCategoryEnabledFlag(angle::PlatformMethods *platform,
                                                 const char *categoryName)
{
    for (const TraceCategory &category : gTraceCategories)
    {
        if (strcmp(category.name, categoryName) == 0)
        {
            return &category.enabled;
        }
    }

    constexpr static unsigned char kZero = 0;
    return &kZero;
}

void UpdateTraceEventDuration(angle::PlatformMethods *platform,
                              const unsigned char *categoryEnabledFlag,
                              const char *name,
                              angle::TraceEventHandle eventHandle)
{
    // Not implemented.
}

double MonotonicallyIncreasingTime(angle::PlatformMethods *platform)
{
    ANGLERenderTest *renderTest = static_cast<ANGLERenderTest *>(platform->context);
    // Move the time origin to the first call to this function, to avoid generating unnecessarily
    // large timestamps.
    static double origin = renderTest->getTimer()->getAbsoluteTime();
    return renderTest->getTimer()->getAbsoluteTime() - origin;
}

void DumpTraceEventsToJSONFile(const std::vector<TraceEvent> &traceEvents,
                               const char *outputFileName)
{
    Json::Value eventsValue(Json::arrayValue);

    for (const TraceEvent &traceEvent : traceEvents)
    {
        Json::Value value(Json::objectValue);

        std::stringstream phaseName;
        phaseName << traceEvent.phase;

        unsigned long long microseconds =
            static_cast<unsigned long long>(traceEvent.timestamp * 1000.0 * 1000.0);

        value["name"] = traceEvent.name;
        value["cat"]  = traceEvent.categoryName;
        value["ph"]   = phaseName.str();
        value["ts"]   = microseconds;
        value["pid"]  = "ANGLE";
        value["tid"]  = strcmp(traceEvent.categoryName, "gpu.angle.gpu") == 0 ? "GPU" : "CPU";

        eventsValue.append(value);
    }

    Json::Value root(Json::objectValue);
    root["traceEvents"] = eventsValue;

    std::ofstream outFile;
    outFile.open(outputFileName);

    Json::StyledWriter styledWrite;
    outFile << styledWrite.write(root);

    outFile.close();
}
}  // anonymous namespace

bool g_OnlyOneRunFrame = false;
bool gEnableTrace      = false;
const char *gTraceFile = "ANGLETrace.json";

ANGLEPerfTest::ANGLEPerfTest(const std::string &name,
                             const std::string &suffix,
                             unsigned int iterationsPerStep)
    : mName(name),
      mSuffix(suffix),
      mTimer(CreateTimer()),
      mRunTimeSeconds(2.0),
      mSkipTest(false),
      mNumStepsPerformed(0),
      mIterationsPerStep(iterationsPerStep),
      mRunning(true)
{
}

ANGLEPerfTest::~ANGLEPerfTest()
{
    SafeDelete(mTimer);
}

void ANGLEPerfTest::run()
{
    if (mSkipTest)
    {
        return;
    }

    mTimer->start();
    while (mRunning)
    {
        step();
        if (mRunning)
        {
            ++mNumStepsPerformed;
        }
        if (mTimer->getElapsedTime() > mRunTimeSeconds || g_OnlyOneRunFrame)
        {
            mRunning = false;
        }
    }
    finishTest();
    mTimer->stop();
}

void ANGLEPerfTest::printResult(const std::string &trace, double value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::printResult(const std::string &trace, size_t value, const std::string &units, bool important) const
{
    perf_test::PrintResult(mName, mSuffix, trace, value, units, important);
}

void ANGLEPerfTest::SetUp()
{
}

void ANGLEPerfTest::TearDown()
{
    if (mSkipTest)
    {
        return;
    }

    double elapsedTimeSeconds = mTimer->getElapsedTime();

    double secondsPerStep      = elapsedTimeSeconds / static_cast<double>(mNumStepsPerformed);
    double secondsPerIteration = secondsPerStep / static_cast<double>(mIterationsPerStep);

    // Give the result a different name to ensure separate graphs if we transition.
    if (secondsPerIteration > 1e-3)
    {
        double microSecondsPerIteration = secondsPerIteration * kMicroSecondsPerSecond;
        printResult("microSecPerIteration", microSecondsPerIteration, "us", true);
    }
    else
    {
        double nanoSecPerIteration = secondsPerIteration * kNanoSecondsPerSecond;
        printResult("nanoSecPerIteration", nanoSecPerIteration, "ns", true);
    }

    double relativeScore = static_cast<double>(mNumStepsPerformed) / elapsedTimeSeconds;
    printResult("score", static_cast<size_t>(std::round(relativeScore)), "score", true);
}

double ANGLEPerfTest::normalizedTime(size_t value) const
{
    return static_cast<double>(value) / static_cast<double>(mNumStepsPerformed);
}

std::string RenderTestParams::suffix() const
{
    switch (getRenderer())
    {
        case EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE:
            return "_d3d11";
        case EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE:
            return "_d3d9";
        case EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE:
            return "_gl";
        case EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE:
            return "_gles";
        case EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE:
            return "_default";
        case EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE:
            return "_vulkan";
        default:
            assert(0);
            return "_unk";
    }
}

ANGLERenderTest::ANGLERenderTest(const std::string &name, const RenderTestParams &testParams)
    : ANGLEPerfTest(name,
                    testParams.suffix(),
                    g_OnlyOneRunFrame ? 1 : testParams.iterationsPerStep),
      mTestParams(testParams),
      mEGLWindow(createEGLWindow(testParams)),
      mOSWindow(nullptr)
{
    // Force fast tests to make sure our slowest bots don't time out.
    if (g_OnlyOneRunFrame)
    {
        const_cast<RenderTestParams &>(testParams).iterationsPerStep = 1;
    }

    // Try to ensure we don't trigger allocation during execution.
    mTraceEventBuffer.reserve(kInitialTraceEventBufferSize);
}

ANGLERenderTest::~ANGLERenderTest()
{
    SafeDelete(mOSWindow);
    SafeDelete(mEGLWindow);
}

void ANGLERenderTest::addExtensionPrerequisite(const char *extensionName)
{
    mExtensionPrerequisites.push_back(extensionName);
}

void ANGLERenderTest::SetUp()
{
    ANGLEPerfTest::SetUp();

    // Set a consistent CPU core affinity and high priority.
    angle::StabilizeCPUForBenchmarking();

    mOSWindow = CreateOSWindow();
    ASSERT(mEGLWindow != nullptr);
    mEGLWindow->setSwapInterval(0);

    mPlatformMethods.overrideWorkaroundsD3D = OverrideWorkaroundsD3D;
    mPlatformMethods.logError               = EmptyPlatformMethod;
    mPlatformMethods.logWarning             = EmptyPlatformMethod;
    mPlatformMethods.logInfo                = EmptyPlatformMethod;
    mPlatformMethods.addTraceEvent               = AddTraceEvent;
    mPlatformMethods.getTraceCategoryEnabledFlag = GetTraceCategoryEnabledFlag;
    mPlatformMethods.updateTraceEventDuration    = UpdateTraceEventDuration;
    mPlatformMethods.monotonicallyIncreasingTime = MonotonicallyIncreasingTime;
    mPlatformMethods.context                = this;
    mEGLWindow->setPlatformMethods(&mPlatformMethods);

    if (!mOSWindow->initialize(mName, mTestParams.windowWidth, mTestParams.windowHeight))
    {
        FAIL() << "Failed initializing OSWindow";
        return;
    }

    if (!mEGLWindow->initializeGL(mOSWindow))
    {
        FAIL() << "Failed initializing EGLWindow";
        return;
    }

    if (!areExtensionPrerequisitesFulfilled())
    {
        mSkipTest = true;
    }

    if (mSkipTest)
    {
        return;
    }

    initializeBenchmark();

    if (mTestParams.iterationsPerStep == 0)
    {
        FAIL() << "Please initialize 'iterationsPerStep'.";
        abortTest();
        return;
    }

    // Warm up the benchmark to reduce variance.
    if (!g_OnlyOneRunFrame)
    {
        for (size_t iteration = 0; iteration < kWarmupIterations; ++iteration)
        {
            drawBenchmark();
        }
    }
}

void ANGLERenderTest::TearDown()
{
    destroyBenchmark();

    mEGLWindow->destroyGL();
    mOSWindow->destroy();

    // Dump trace events to json file.
    if (gEnableTrace)
    {
        DumpTraceEventsToJSONFile(mTraceEventBuffer, gTraceFile);
    }

    ANGLEPerfTest::TearDown();
}

void ANGLERenderTest::step()
{
    // Clear events that the application did not process from this frame
    Event event;
    bool closed = false;
    while (popEvent(&event))
    {
        // If the application did not catch a close event, close now
        if (event.Type == Event::EVENT_CLOSED)
        {
            closed = true;
        }
    }

    if (closed)
    {
        abortTest();
    }
    else
    {
        drawBenchmark();
        // Swap is needed so that the GPU driver will occasionally flush its internal command queue
        // to the GPU. The null device benchmarks are only testing CPU overhead, so they don't need
        // to swap.
        if (mTestParams.eglParameters.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE)
        {
            mEGLWindow->swap();
        }
        mOSWindow->messageLoop();
    }
}

void ANGLERenderTest::finishTest()
{
    if (mTestParams.eglParameters.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE)
    {
        glFinish();
    }
}

bool ANGLERenderTest::popEvent(Event *event)
{
    return mOSWindow->popEvent(event);
}

OSWindow *ANGLERenderTest::getWindow()
{
    return mOSWindow;
}

bool ANGLERenderTest::areExtensionPrerequisitesFulfilled() const
{
    for (const char *extension : mExtensionPrerequisites)
    {
        if (!CheckExtensionExists(reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS)),
                                  extension))
        {
            std::cout << "Test skipped due to missing extension: " << extension << std::endl;
            return false;
        }
    }
    return true;
}

void ANGLERenderTest::setWebGLCompatibilityEnabled(bool webglCompatibility)
{
    mEGLWindow->setWebGLCompatibilityEnabled(webglCompatibility);
}

void ANGLERenderTest::setRobustResourceInit(bool enabled)
{
    mEGLWindow->setRobustResourceInit(enabled);
}

std::vector<TraceEvent> &ANGLERenderTest::getTraceEventBuffer()
{
    return mTraceEventBuffer;
}

// static
EGLWindow *ANGLERenderTest::createEGLWindow(const RenderTestParams &testParams)
{
    return new EGLWindow(testParams.majorVersion, testParams.minorVersion,
                         testParams.eglParameters);
}

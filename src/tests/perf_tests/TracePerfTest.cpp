//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// TracePerf:
//   Performance test for ANGLE replaying traces.
//

#include <gtest/gtest.h>
#include "common/PackedEnums.h"
#include "common/system_utils.h"
#include "tests/perf_tests/ANGLEPerfTest.h"
#include "tests/perf_tests/DrawCallPerfParams.h"
#include "util/egl_loader_autogen.h"

#include "restricted_traces/trex_1300_1310/trex_1300_1310_capture_context1.h"
#include "restricted_traces/trex_200_210/trex_200_210_capture_context1.h"
#include "restricted_traces/trex_800_810/trex_800_810_capture_context1.h"
#include "restricted_traces/trex_900_910/trex_900_910_capture_context1.h"

#include <cassert>
#include <functional>
#include <sstream>

using namespace angle;
using namespace egl_platform;

namespace
{
void FramebufferChangeCallback(void *userData, GLenum target, GLuint framebuffer);

enum class TracePerfTestID
{
    TRex200,
    TRex800,
    TRex900,
    TRex1300,
    InvalidEnum,
};

struct TracePerfParams final : public RenderTestParams
{
    // Common default options
    TracePerfParams()
    {
        majorVersion = 2;
        minorVersion = 0;
        windowWidth  = 1920;
        windowHeight = 1080;
        trackGpuTime = true;

        // Display the frame after every drawBenchmark invocation
        iterationsPerStep = 1;
    }

    std::string story() const override
    {
        std::stringstream strstr;

        strstr << RenderTestParams::story();

        switch (testID)
        {
            case TracePerfTestID::TRex200:
                strstr << "_trex_200";
                break;
            case TracePerfTestID::TRex800:
                strstr << "_trex_800";
                break;
            case TracePerfTestID::TRex900:
                strstr << "_trex_900";
                break;
            case TracePerfTestID::TRex1300:
                strstr << "_trex_1300";
                break;
            default:
                assert(0);
                break;
        }

        return strstr.str();
    }

    TracePerfTestID testID;
};

std::ostream &operator<<(std::ostream &os, const TracePerfParams &params)
{
    os << params.backendAndStory().substr(1);
    return os;
}

class TracePerfTest : public ANGLERenderTest, public ::testing::WithParamInterface<TracePerfParams>
{
  public:
    TracePerfTest();

    void initializeBenchmark() override;
    void destroyBenchmark() override;
    void drawBenchmark() override;

    void onFramebufferChange(GLenum target, GLuint framebuffer);

    uint32_t mStartFrame;
    uint32_t mEndFrame;
    std::function<void(uint32_t)> mReplayFunc;

    double getHostTimeFromGLTime(GLint64 glTime);

  private:
    struct QueryInfo
    {
        GLuint beginTimestampQuery;
        GLuint endTimestampQuery;
        GLuint framebuffer;
    };

    struct TimeSample
    {
        GLint64 glTime;
        double hostTime;
    };

    void sampleTime();

    // For tracking RenderPass/FBO change timing.
    QueryInfo mCurrentQuery = {};
    std::vector<QueryInfo> mRunningQueries;
    std::vector<TimeSample> mTimeline;
};

TracePerfTest::TracePerfTest()
    : ANGLERenderTest("TracePerf", GetParam()), mStartFrame(0), mEndFrame(0)
{}

void TracePerfTest::initializeBenchmark()
{
    const auto &params = GetParam();

    // TODO: Note the start and end frames in the trace
    //       i.e. mStartFrame = trex_200_210::kReplayFrameStart
    switch (params.testID)
    {
        // For each case, bootstrap the trace
        case TracePerfTestID::TRex200:
            mStartFrame = 200;
            mEndFrame   = 210;
            mReplayFunc = trex_200_210::ReplayContext1Frame;
            trex_200_210::SetBinaryDataDir(ANGLE_TRACE_DATA_DIR_trex_200_210);
            trex_200_210::SetupContext1Replay();
            break;
        case TracePerfTestID::TRex800:
            mStartFrame = 800;
            mEndFrame   = 810;
            mReplayFunc = trex_800_810::ReplayContext1Frame;
            trex_800_810::SetBinaryDataDir(ANGLE_TRACE_DATA_DIR_trex_800_810);
            trex_800_810::SetupContext1Replay();
            break;
        case TracePerfTestID::TRex900:
            mStartFrame = 900;
            mEndFrame   = 910;
            mReplayFunc = trex_900_910::ReplayContext1Frame;
            trex_900_910::SetBinaryDataDir(ANGLE_TRACE_DATA_DIR_trex_900_910);
            trex_900_910::SetupContext1Replay();
            break;
        case TracePerfTestID::TRex1300:
            mStartFrame = 1300;
            mEndFrame   = 1310;
            mReplayFunc = trex_1300_1310::ReplayContext1Frame;
            trex_1300_1310::SetBinaryDataDir(ANGLE_TRACE_DATA_DIR_trex_1300_1310);
            trex_1300_1310::SetupContext1Replay();
            break;
        default:
            assert(0);
            break;
    }

    ASSERT_TRUE(mEndFrame > mStartFrame);

    getWindow()->setVisible(true);
}

void TracePerfTest::destroyBenchmark() {}

void TracePerfTest::sampleTime()
{
    if (mIsTimestampQueryAvailable)
    {
        GLint64 glTime;
        // glGetInteger64vEXT is exported by newer versions of the timer query extensions.
        // Unfortunately only the core EP is exposed by some desktop drivers (e.g. NVIDIA).
        if (glGetInteger64vEXT)
        {
            glGetInteger64vEXT(GL_TIMESTAMP_EXT, &glTime);
        }
        else
        {
            glGetInteger64v(GL_TIMESTAMP_EXT, &glTime);
        }
        mTimeline.push_back({glTime, angle::GetHostTimeSeconds()});
    }
}

void TracePerfTest::drawBenchmark()
{
    // Add a time sample from GL and the host.
    sampleTime();

    startGpuTimer();

    for (uint32_t frame = mStartFrame; frame < mEndFrame; ++frame)
    {
        char frameName[32];
        sprintf(frameName, "Frame %u", frame);
        beginInternalTraceEvent(frameName);

        mReplayFunc(frame);
        getGLWindow()->swap();

        endInternalTraceEvent(frameName);
    }

    // Process any running queries once per iteration.
    for (size_t queryIndex = 0; queryIndex < mRunningQueries.size();)
    {
        const QueryInfo &query = mRunningQueries[queryIndex];

        GLuint endResultAvailable = 0;
        glGetQueryObjectuivEXT(query.endTimestampQuery, GL_QUERY_RESULT_AVAILABLE,
                               &endResultAvailable);

        if (endResultAvailable == GL_TRUE)
        {
            char fboName[32];
            sprintf(fboName, "FBO %u", query.framebuffer);

            GLint64 beginTimestamp = 0;
            glGetQueryObjecti64vEXT(query.beginTimestampQuery, GL_QUERY_RESULT, &beginTimestamp);
            glDeleteQueriesEXT(1, &query.beginTimestampQuery);
            double beginHostTime = getHostTimeFromGLTime(beginTimestamp);
            beginGLTraceEvent(fboName, beginHostTime);

            GLint64 endTimestamp = 0;
            glGetQueryObjecti64vEXT(query.endTimestampQuery, GL_QUERY_RESULT, &endTimestamp);
            glDeleteQueriesEXT(1, &query.endTimestampQuery);
            double endHostTime = getHostTimeFromGLTime(endTimestamp);
            endGLTraceEvent(fboName, endHostTime);

            mRunningQueries.erase(mRunningQueries.begin() + queryIndex);
        }
        else
        {
            queryIndex++;
        }
    }

    stopGpuTimer();
}

// Converts a GL timestamp into a host-side CPU time aligned with "GetHostTimeSeconds".
// This check is necessary to line up sampled trace events in a consistent timeline.
// Uses a linear interpolation from a series of samples. We do a blocking call to sample
// both host and GL time once per swap. We then find the two closest GL timestamps and
// interpolate the host times between them to compute our result. If we are past the last
// GL timestamp we sample a new data point pair.
double TracePerfTest::getHostTimeFromGLTime(GLint64 glTime)
{
    // Find two samples to do a lerp.
    size_t firstSampleIndex = mTimeline.size() - 1;
    while (firstSampleIndex > 0)
    {
        if (mTimeline[firstSampleIndex].glTime < glTime)
        {
            break;
        }
        firstSampleIndex--;
    }

    // Add an extra sample if we're missing an ending sample.
    if (firstSampleIndex == mTimeline.size() - 1)
    {
        sampleTime();
    }

    const TimeSample &start = mTimeline[firstSampleIndex];
    const TimeSample &end   = mTimeline[firstSampleIndex + 1];

    // Note: we have observed in some odd cases later timestamps producing values that are
    // smaller than preceding timestamps. This bears further investigation.

    // Compute the scaling factor for the lerp.
    double glDelta = static_cast<double>(glTime - start.glTime);
    double glRange = static_cast<double>(end.glTime - start.glTime);
    double t       = glDelta / glRange;

    // Lerp(t1, t2, t)
    double hostRange = end.hostTime - start.hostTime;
    return mTimeline[firstSampleIndex].hostTime + hostRange * t;
}

// Callback from the perf tests.
void TracePerfTest::onFramebufferChange(GLenum target, GLuint framebuffer)
{
    if (!mIsTimestampQueryAvailable)
        return;

    if (target != GL_FRAMEBUFFER && target != GL_DRAW_FRAMEBUFFER)
        return;

    // We have at most one active timestamp query at a time. This code will end the current query
    // and immediately start a new one.
    if (mCurrentQuery.beginTimestampQuery != 0)
    {
        glGenQueriesEXT(1, &mCurrentQuery.endTimestampQuery);
        glQueryCounterEXT(mCurrentQuery.endTimestampQuery, GL_TIMESTAMP_EXT);
        mRunningQueries.push_back(mCurrentQuery);
        mCurrentQuery = {};
    }

    ASSERT(mCurrentQuery.beginTimestampQuery == 0);

    glGenQueriesEXT(1, &mCurrentQuery.beginTimestampQuery);
    glQueryCounterEXT(mCurrentQuery.beginTimestampQuery, GL_TIMESTAMP_EXT);
    mCurrentQuery.framebuffer = framebuffer;
}

ANGLE_MAYBE_UNUSED void FramebufferChangeCallback(void *userData, GLenum target, GLuint framebuffer)
{
    reinterpret_cast<TracePerfTest *>(userData)->onFramebufferChange(target, framebuffer);
}

TEST_P(TracePerfTest, Run)
{
    run();
}

TracePerfParams CombineTestID(const TracePerfParams &in, TracePerfTestID id)
{
    TracePerfParams out = in;
    out.testID          = id;
    return out;
}

using namespace params;
using P = TracePerfParams;

std::vector<P> gTestsWithID = CombineWithValues({P()}, AllEnums<TracePerfTestID>(), CombineTestID);
std::vector<P> gTestsWithRenderer = CombineWithFuncs(gTestsWithID, {GL<P>, Vulkan<P>});
ANGLE_INSTANTIATE_TEST_ARRAY(TracePerfTest, gTestsWithRenderer);

}  // anonymous namespace

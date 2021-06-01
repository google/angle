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
#include "tests/perf_tests/ANGLEPerfTestArgs.h"
#include "tests/perf_tests/DrawCallPerfParams.h"
#include "util/egl_loader_autogen.h"
#include "util/frame_capture_test_utils.h"
#include "util/png_utils.h"
#include "util/test_utils.h"

#include "restricted_traces/restricted_traces_autogen.h"

#include <cassert>
#include <functional>
#include <sstream>

// When --minimize-gpu-work is specified, we want to reduce GPU work to minimum and lift up the CPU
// overhead to surface so that we can see how much CPU overhead each driver has for each app trace.
// On some driver(s) the bufferSubData/texSubImage calls end up dominating the frame time when the
// actual GPU work is minimized. Even reducing the texSubImage calls to only update 1x1 area is not
// enough. The driver may be implementing copy on write by cloning the entire texture to another
// memory storage for texSubImage call. While this information is also important for performance,
// they should be evaluated separately in real app usage scenario, or write stand alone tests for
// these. For the purpose of CPU overhead and avoid data copy to dominate the trace, I am using this
// flag to noop the texSubImage and bufferSubData call when --minimize-gpu-work is specified. Feel
// free to disable this when you have other needs. Or it can be turned to another run time option
// when desired.
#define NOOP_SUBDATA_SUBIMAGE_FOR_MINIMIZE_GPU_WORK

using namespace angle;
using namespace egl_platform;

namespace
{
struct TracePerfParams final : public RenderTestParams
{
    // Common default options
    TracePerfParams()
    {
        // Display the frame after every drawBenchmark invocation
        iterationsPerStep = 1;
    }

    std::string story() const override
    {
        std::stringstream strstr;
        strstr << RenderTestParams::story() << "_" << GetTraceInfo(testID).name;
        return strstr.str();
    }

    RestrictedTraceID testID;
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

    void onReplayFramebufferChange(GLenum target, GLuint framebuffer);
    void onReplayInvalidateFramebuffer(GLenum target,
                                       GLsizei numAttachments,
                                       const GLenum *attachments);
    void onReplayInvalidateSubFramebuffer(GLenum target,
                                          GLsizei numAttachments,
                                          const GLenum *attachments,
                                          GLint x,
                                          GLint y,
                                          GLsizei width,
                                          GLsizei height);
    void onReplayDrawBuffers(GLsizei n, const GLenum *bufs);
    void onReplayReadBuffer(GLenum src);
    void onReplayDiscardFramebufferEXT(GLenum target,
                                       GLsizei numAttachments,
                                       const GLenum *attachments);

    bool isDefaultFramebuffer(GLenum target) const;

    uint32_t mStartFrame;
    uint32_t mEndFrame;

    double getHostTimeFromGLTime(GLint64 glTime);

    int getStepAlignment() const override
    {
        // Align step counts to the number of frames in a trace.
        const TraceInfo &traceInfo = GetTraceInfo(GetParam().testID);
        return static_cast<int>(traceInfo.endFrame - traceInfo.startFrame + 1);
    }

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
    void saveScreenshot(const std::string &screenshotName) override;
    void swap();

    // For tracking RenderPass/FBO change timing.
    QueryInfo mCurrentQuery = {};
    std::vector<QueryInfo> mRunningQueries;
    std::vector<TimeSample> mTimeline;

    std::string mStartingDirectory;
    bool mUseTimestampQueries                                           = false;
    static constexpr int mMaxOffscreenBufferCount                       = 2;
    std::array<GLuint, mMaxOffscreenBufferCount> mOffscreenFramebuffers = {0, 0};
    std::array<GLuint, mMaxOffscreenBufferCount> mOffscreenTextures     = {0, 0};
    GLuint mOffscreenDepthStencil                                       = 0;
    int mWindowWidth                                                    = 0;
    int mWindowHeight                                                   = 0;
    GLuint mDrawFramebufferBinding                                      = 0;
    GLuint mReadFramebufferBinding                                      = 0;
    uint32_t mCurrentFrame                                              = 0;
    uint32_t mOffscreenFrameCount                                       = 0;
    uint32_t mTotalFrameCount                                           = 0;
    bool mScreenshotSaved                                               = false;
    std::unique_ptr<TraceLibrary> mTraceLibrary;
};

class TracePerfTest;
TracePerfTest *gCurrentTracePerfTest = nullptr;

// Don't forget to include KHRONOS_APIENTRY in override methods. Neccessary on Win/x86.
void KHRONOS_APIENTRY BindFramebufferProc(GLenum target, GLuint framebuffer)
{
    gCurrentTracePerfTest->onReplayFramebufferChange(target, framebuffer);
}

void KHRONOS_APIENTRY InvalidateFramebufferProc(GLenum target,
                                                GLsizei numAttachments,
                                                const GLenum *attachments)
{
    gCurrentTracePerfTest->onReplayInvalidateFramebuffer(target, numAttachments, attachments);
}

void KHRONOS_APIENTRY InvalidateSubFramebufferProc(GLenum target,
                                                   GLsizei numAttachments,
                                                   const GLenum *attachments,
                                                   GLint x,
                                                   GLint y,
                                                   GLsizei width,
                                                   GLsizei height)
{
    gCurrentTracePerfTest->onReplayInvalidateSubFramebuffer(target, numAttachments, attachments, x,
                                                            y, width, height);
}

void KHRONOS_APIENTRY DrawBuffersProc(GLsizei n, const GLenum *bufs)
{
    gCurrentTracePerfTest->onReplayDrawBuffers(n, bufs);
}

void KHRONOS_APIENTRY ReadBufferProc(GLenum src)
{
    gCurrentTracePerfTest->onReplayReadBuffer(src);
}

void KHRONOS_APIENTRY DiscardFramebufferEXTProc(GLenum target,
                                                GLsizei numAttachments,
                                                const GLenum *attachments)
{
    gCurrentTracePerfTest->onReplayDiscardFramebufferEXT(target, numAttachments, attachments);
}

void KHRONOS_APIENTRY ViewportMinimizedProc(GLint x, GLint y, GLsizei width, GLsizei height)
{
    glViewport(x, y, 1, 1);
}

void KHRONOS_APIENTRY ScissorMinimizedProc(GLint x, GLint y, GLsizei width, GLsizei height)
{
    glScissor(x, y, 1, 1);
}

// Interpose the calls that generate actual GPU work
void KHRONOS_APIENTRY DrawElementsMinimizedProc(GLenum mode,
                                                GLsizei count,
                                                GLenum type,
                                                const void *indices)
{
    glDrawElements(GL_POINTS, 1, type, indices);
}

void KHRONOS_APIENTRY DrawElementsIndirectMinimizedProc(GLenum mode,
                                                        GLenum type,
                                                        const void *indirect)
{
    glDrawElementsInstancedBaseVertex(GL_POINTS, 1, type, 0, 1, 0);
}

void KHRONOS_APIENTRY DrawElementsInstancedMinimizedProc(GLenum mode,
                                                         GLsizei count,
                                                         GLenum type,
                                                         const void *indices,
                                                         GLsizei instancecount)
{
    glDrawElementsInstanced(GL_POINTS, 1, type, indices, 1);
}

void KHRONOS_APIENTRY DrawElementsBaseVertexMinimizedProc(GLenum mode,
                                                          GLsizei count,
                                                          GLenum type,
                                                          const void *indices,
                                                          GLint basevertex)
{
    glDrawElementsBaseVertex(GL_POINTS, 1, type, indices, basevertex);
}

void KHRONOS_APIENTRY DrawElementsInstancedBaseVertexMinimizedProc(GLenum mode,
                                                                   GLsizei count,
                                                                   GLenum type,
                                                                   const void *indices,
                                                                   GLsizei instancecount,
                                                                   GLint basevertex)
{
    glDrawElementsInstancedBaseVertex(GL_POINTS, 1, type, indices, 1, basevertex);
}

void KHRONOS_APIENTRY DrawRangeElementsMinimizedProc(GLenum mode,
                                                     GLuint start,
                                                     GLuint end,
                                                     GLsizei count,
                                                     GLenum type,
                                                     const void *indices)
{
    glDrawRangeElements(GL_POINTS, start, end, 1, type, indices);
}

void KHRONOS_APIENTRY DrawArraysMinimizedProc(GLenum mode, GLint first, GLsizei count)
{
    glDrawArrays(GL_POINTS, first, 1);
}

void KHRONOS_APIENTRY DrawArraysInstancedMinimizedProc(GLenum mode,
                                                       GLint first,
                                                       GLsizei count,
                                                       GLsizei instancecount)
{
    glDrawArraysInstanced(GL_POINTS, first, 1, 1);
}

void KHRONOS_APIENTRY DrawArraysIndirectMinimizedProc(GLenum mode, const void *indirect)
{
    glDrawArraysInstanced(GL_POINTS, 0, 1, 1);
}

void KHRONOS_APIENTRY DispatchComputeMinimizedProc(GLuint num_groups_x,
                                                   GLuint num_groups_y,
                                                   GLuint num_groups_z)
{
    glDispatchCompute(1, 1, 1);
}

void KHRONOS_APIENTRY DispatchComputeIndirectMinimizedProc(GLintptr indirect)
{
    glDispatchCompute(1, 1, 1);
}

// Interpose the calls that generate data copying work
void KHRONOS_APIENTRY BufferDataMinimizedProc(GLenum target,
                                              GLsizeiptr size,
                                              const void *data,
                                              GLenum usage)
{
    glBufferData(target, size, nullptr, usage);
}

void KHRONOS_APIENTRY BufferSubDataMinimizedProc(GLenum target,
                                                 GLintptr offset,
                                                 GLsizeiptr size,
                                                 const void *data)
{
#if !defined(NOOP_SUBDATA_SUBIMAGE_FOR_MINIMIZE_GPU_WORK)
    glBufferSubData(target, offset, 1, data);
#endif
}

void *KHRONOS_APIENTRY MapBufferRangeMinimizedProc(GLenum target,
                                                   GLintptr offset,
                                                   GLsizeiptr length,
                                                   GLbitfield access)
{
    access |= GL_MAP_UNSYNCHRONIZED_BIT;
    return glMapBufferRange(target, offset, length, access);
}

void KHRONOS_APIENTRY TexImage2DMinimizedProc(GLenum target,
                                              GLint level,
                                              GLint internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLint border,
                                              GLenum format,
                                              GLenum type,
                                              const void *pixels)
{
    GLint unpackBuffer = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
    if (unpackBuffer)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    glTexImage2D(target, level, internalformat, width, height, border, format, type, nullptr);
    if (unpackBuffer)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
    }
}

void KHRONOS_APIENTRY TexSubImage2DMinimizedProc(GLenum target,
                                                 GLint level,
                                                 GLint xoffset,
                                                 GLint yoffset,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLenum format,
                                                 GLenum type,
                                                 const void *pixels)
{
#if !defined(NOOP_SUBDATA_SUBIMAGE_FOR_MINIMIZE_GPU_WORK)
    glTexSubImage2D(target, level, xoffset, yoffset, 1, 1, format, type, pixels);
#endif
}

void KHRONOS_APIENTRY TexImage3DMinimizedProc(GLenum target,
                                              GLint level,
                                              GLint internalformat,
                                              GLsizei width,
                                              GLsizei height,
                                              GLsizei depth,
                                              GLint border,
                                              GLenum format,
                                              GLenum type,
                                              const void *pixels)
{
    GLint unpackBuffer = 0;
    glGetIntegerv(GL_PIXEL_UNPACK_BUFFER_BINDING, &unpackBuffer);
    if (unpackBuffer)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    }
    glTexImage3D(target, level, internalformat, width, height, depth, border, format, type,
                 nullptr);
    if (unpackBuffer)
    {
        glBindBuffer(GL_PIXEL_UNPACK_BUFFER, unpackBuffer);
    }
}

void KHRONOS_APIENTRY TexSubImage3DMinimizedProc(GLenum target,
                                                 GLint level,
                                                 GLint xoffset,
                                                 GLint yoffset,
                                                 GLint zoffset,
                                                 GLsizei width,
                                                 GLsizei height,
                                                 GLsizei depth,
                                                 GLenum format,
                                                 GLenum type,
                                                 const void *pixels)
{
#if !defined(NOOP_SUBDATA_SUBIMAGE_FOR_MINIMIZE_GPU_WORK)
    glTexSubImage3D(target, level, xoffset, yoffset, zoffset, 1, 1, 1, format, type, pixels);
#endif
}

void KHRONOS_APIENTRY GenerateMipmapMinimizedProc(GLenum target)
{
    // Noop it for now. There is a risk that this will leave an incomplete mipmap chain and cause
    // other issues. If this turns out to be a real issue with app traces, we can turn this into a
    // glTexImage2D call for each generated level.
}

void KHRONOS_APIENTRY BlitFramebufferMinimizedProc(GLint srcX0,
                                                   GLint srcY0,
                                                   GLint srcX1,
                                                   GLint srcY1,
                                                   GLint dstX0,
                                                   GLint dstY0,
                                                   GLint dstX1,
                                                   GLint dstY1,
                                                   GLbitfield mask,
                                                   GLenum filter)
{
    glBlitFramebuffer(srcX0, srcY0, srcX0 + 1, srcY0 + 1, dstX0, dstY0, dstX0 + 1, dstY0 + 1, mask,
                      filter);
}

void KHRONOS_APIENTRY ReadPixelsMinimizedProc(GLint x,
                                              GLint y,
                                              GLsizei width,
                                              GLsizei height,
                                              GLenum format,
                                              GLenum type,
                                              void *pixels)
{
    glReadPixels(x, y, 1, 1, format, type, pixels);
}

void KHRONOS_APIENTRY BeginTransformFeedbackMinimizedProc(GLenum primitiveMode)
{
    glBeginTransformFeedback(GL_POINTS);
}

angle::GenericProc KHRONOS_APIENTRY TraceLoadProc(const char *procName)
{
    if (strcmp(procName, "glBindFramebuffer") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(BindFramebufferProc);
    }
    if (strcmp(procName, "glInvalidateFramebuffer") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(InvalidateFramebufferProc);
    }
    if (strcmp(procName, "glInvalidateSubFramebuffer") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(InvalidateSubFramebufferProc);
    }
    if (strcmp(procName, "glDrawBuffers") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(DrawBuffersProc);
    }
    if (strcmp(procName, "glReadBuffer") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(ReadBufferProc);
    }
    if (strcmp(procName, "glDiscardFramebufferEXT") == 0)
    {
        return reinterpret_cast<angle::GenericProc>(DiscardFramebufferEXTProc);
    }

    if (gMinimizeGPUWork)
    {
        if (strcmp(procName, "glViewport") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(ViewportMinimizedProc);
        }

        if (strcmp(procName, "glScissor") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(ScissorMinimizedProc);
        }

        // Interpose the calls that generate actual GPU work
        if (strcmp(procName, "glDrawElements") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawElementsMinimizedProc);
        }
        if (strcmp(procName, "glDrawElementsIndirect") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawElementsIndirectMinimizedProc);
        }
        if (strcmp(procName, "glDrawElementsInstanced") == 0 ||
            strcmp(procName, "glDrawElementsInstancedEXT") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawElementsInstancedMinimizedProc);
        }
        if (strcmp(procName, "glDrawElementsBaseVertex") == 0 ||
            strcmp(procName, "glDrawElementsBaseVertexEXT") == 0 ||
            strcmp(procName, "glDrawElementsBaseVertexOES") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawElementsBaseVertexMinimizedProc);
        }
        if (strcmp(procName, "glDrawElementsInstancedBaseVertex") == 0 ||
            strcmp(procName, "glDrawElementsInstancedBaseVertexEXT") == 0 ||
            strcmp(procName, "glDrawElementsInstancedBaseVertexOES") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(
                DrawElementsInstancedBaseVertexMinimizedProc);
        }
        if (strcmp(procName, "glDrawRangeElements") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawRangeElementsMinimizedProc);
        }
        if (strcmp(procName, "glDrawArrays") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawArraysMinimizedProc);
        }
        if (strcmp(procName, "glDrawArraysInstanced") == 0 ||
            strcmp(procName, "glDrawArraysInstancedEXT") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawArraysInstancedMinimizedProc);
        }
        if (strcmp(procName, "glDrawArraysIndirect") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DrawArraysIndirectMinimizedProc);
        }
        if (strcmp(procName, "glDispatchCompute") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DispatchComputeMinimizedProc);
        }
        if (strcmp(procName, "glDispatchComputeIndirect") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(DispatchComputeIndirectMinimizedProc);
        }

        // Interpose the calls that generate data copying work
        if (strcmp(procName, "glBufferData") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(BufferDataMinimizedProc);
        }
        if (strcmp(procName, "glBufferSubData") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(BufferSubDataMinimizedProc);
        }
        if (strcmp(procName, "glMapBufferRange") == 0 ||
            strcmp(procName, "glMapBufferRangeEXT") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(MapBufferRangeMinimizedProc);
        }
        if (strcmp(procName, "glTexImage2D") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(TexImage2DMinimizedProc);
        }
        if (strcmp(procName, "glTexImage3D") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(TexImage3DMinimizedProc);
        }
        if (strcmp(procName, "glTexSubImage2D") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(TexSubImage2DMinimizedProc);
        }
        if (strcmp(procName, "glTexSubImage3D") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(TexSubImage3DMinimizedProc);
        }
        if (strcmp(procName, "glGenerateMipmap") == 0 ||
            strcmp(procName, "glGenerateMipmapOES") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(GenerateMipmapMinimizedProc);
        }
        if (strcmp(procName, "glBlitFramebuffer") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(BlitFramebufferMinimizedProc);
        }
        if (strcmp(procName, "glReadPixels") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(ReadPixelsMinimizedProc);
        }
        if (strcmp(procName, "glBeginTransformFeedback") == 0)
        {
            return reinterpret_cast<angle::GenericProc>(BeginTransformFeedbackMinimizedProc);
        }
    }

    return gCurrentTracePerfTest->getGLWindow()->getProcAddress(procName);
}

TracePerfTest::TracePerfTest()
    : ANGLERenderTest("TracePerf", GetParam(), "ms"), mStartFrame(0), mEndFrame(0)
{
    const TracePerfParams &param = GetParam();

    // TODO: http://anglebug.com/4533 This fails after the upgrade to the 26.20.100.7870 driver.
    if (IsWindows() && IsIntel() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE &&
        param.testID == RestrictedTraceID::manhattan_10)
    {
        mSkipTest = true;
    }

    // TODO: http://anglebug.com/4731 Fails on older Intel drivers. Passes in newer.
    if (IsWindows() && IsIntel() && param.driver != GLESDriverType::AngleEGL &&
        param.testID == RestrictedTraceID::angry_birds_2_1500)
    {
        mSkipTest = true;
    }

    if (param.surfaceType != SurfaceType::Window && !gEnableAllTraceTests)
    {
        printf("Test skipped. Use --enable-all-trace-tests to run.\n");
        mSkipTest = true;
    }

    if (param.eglParameters.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE &&
        !gEnableAllTraceTests)
    {
        printf("Test skipped. Use --enable-all-trace-tests to run.\n");
        mSkipTest = true;
    }

    if (param.testID == RestrictedTraceID::cod_mobile)
    {
        // TODO: http://anglebug.com/4967 Vulkan: GL_EXT_color_buffer_float not supported on Pixel 2
        // The COD:Mobile trace uses a framebuffer attachment with:
        //   format = GL_RGB
        //   type = GL_UNSIGNED_INT_10F_11F_11F_REV
        // That combination is only renderable if GL_EXT_color_buffer_float is supported.
        // It happens to not be supported on Pixel 2's Vulkan driver.
        addExtensionPrerequisite("GL_EXT_color_buffer_float");

        // TODO: http://anglebug.com/4731 This extension is missing on older Intel drivers.
        addExtensionPrerequisite("GL_OES_EGL_image_external");
    }

    if (param.testID == RestrictedTraceID::brawl_stars)
    {
        addExtensionPrerequisite("GL_EXT_shadow_samplers");
    }

    if (param.testID == RestrictedTraceID::free_fire)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
    }

    if (param.testID == RestrictedTraceID::marvel_contest_of_champions)
    {
        addExtensionPrerequisite("GL_EXT_color_buffer_half_float");
    }

    if (param.testID == RestrictedTraceID::world_of_tanks_blitz)
    {
        addExtensionPrerequisite("GL_EXT_disjoint_timer_query");
    }

    if (param.testID == RestrictedTraceID::dragon_ball_legends)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::lego_legacy)
    {
        addExtensionPrerequisite("GL_EXT_shadow_samplers");
    }

    if (param.testID == RestrictedTraceID::world_war_doh)
    {
        // Linux+Nvidia doesn't support GL_KHR_texture_compression_astc_ldr (possibly others also)
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::saint_seiya_awakening)
    {
        addExtensionPrerequisite("GL_EXT_shadow_samplers");

        // TODO(https://anglebug.com/5517) Linux+Intel generates "Framebuffer is incomplete" errors.
        if (IsLinux() && IsIntel() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::magic_tiles_3)
    {
        // Linux+Nvidia doesn't support GL_KHR_texture_compression_astc_ldr (possibly others also)
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::real_gangster_crime)
    {
        // Linux+Nvidia doesn't support GL_KHR_texture_compression_astc_ldr (possibly others also)
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");

        // Intel doesn't support external images.
        addExtensionPrerequisite("GL_OES_EGL_image_external");

        // Failing on Linux Intel and AMD due to invalid enum. http://anglebug.com/5822
        if (IsLinux() && (IsIntel() || IsAMD()) && param.driver != GLESDriverType::AngleEGL)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::asphalt_8)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::hearthstone)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::efootball_pes_2021)
    {
        // TODO(https://anglebug.com/5517) Linux+Intel and Pixel 2 generate "Framebuffer is
        // incomplete" errors with the Vulkan backend.
        if (param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE &&
            ((IsLinux() && IsIntel()) || IsPixel2()))
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::manhattan_31)
    {
        // TODO: http://anglebug.com/5591 Trace crashes on Pixel 2 in vulkan driver
        if (IsPixel2() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::idle_heroes)
    {
        // TODO: http://anglebug.com/5591 Trace crashes on Pixel 2
        if (IsPixel2())
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::shadow_fight_2)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::rise_of_kingdoms)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
    }

    if (param.testID == RestrictedTraceID::happy_color)
    {
        if (IsWindows() && IsAMD() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::bus_simulator_indonesia)
    {
        // TODO(https://anglebug.com/5629) Linux+(Intel|AMD) native GLES generates
        // GL_INVALID_OPERATION
        if (IsLinux() && (IsIntel() || IsAMD()) &&
            param.getRenderer() != EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::messenger_lite)
    {
        // TODO: https://anglebug.com/5663 Incorrect pixels on Nvidia Windows for first frame
        if (IsWindows() && IsNVIDIA() &&
            param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE &&
            param.getDeviceType() != EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::among_us)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::car_parking_multiplayer)
    {
        // TODO: https://anglebug.com/5613 Nvidia native driver spews undefined behavior warnings
        if (IsNVIDIA() && param.getRenderer() != EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
        // TODO: https://anglebug.com/5724 Device lost on Win Intel
        if (IsWindows() && IsIntel() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::fifa_mobile)
    {
        // TODO: http://anglebug.com/5875 Intel Windows Vulkan flakily renders entirely black
        if (IsWindows() && IsIntel() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::rope_hero_vice_town)
    {
        // TODO: http://anglebug.com/5716 Trace crashes on Pixel 2 in vulkan driver
        if (IsPixel2() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::extreme_car_driving_simulator)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    if (param.testID == RestrictedTraceID::lineage_m)
    {
        // TODO: http://anglebug.com/5748 Vulkan device is lost on Nvidia Linux
        if (IsLinux() && IsNVIDIA() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::plants_vs_zombies_2)
    {
        // TODO: http://crbug.com/1187752 Corrupted image
        if (IsWindows() && IsAMD() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::junes_journey)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
    }

    if (param.testID == RestrictedTraceID::ragnarok_m_eternal_love)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");

        // TODO: http://anglebug.com/5772 Pixel 2 errors with "Framebuffer is incomplete" on Vulkan
        if (IsPixel2() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::real_cricket_20)
    {
        // TODO: http://anglebug.com/5777 ARM doesn't have enough VS storage blocks
        if (IsAndroid() && IsARM())
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::league_of_legends_wild_rift)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");

        // TODO: http://anglebug.com/5815 Trace is crashing on Intel Linux
        if (IsLinux() && IsIntel() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::aztec_ruins)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");

        // TODO: http://anglebug.com/5553 Pixel 2 errors with "Framebuffer is incomplete" on Vulkan
        if (IsPixel2() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::dragon_raja)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");

        // TODO: http://anglebug.com/5807 Intel Linux and Pixel 2 error with "Framebuffer is
        // incomplete" on Vulkan
        if (((IsLinux() && IsIntel()) || IsPixel2()) &&
            param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    // Adreno gives a driver error with empty/small draw calls. http://anglebug.com/5823
    if (param.testID == RestrictedTraceID::hill_climb_racing)
    {
        if (IsAndroid() && (IsPixel2() || IsPixel4()) && param.driver == GLESDriverType::SystemEGL)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::avakin_life)
    {
        addExtensionPrerequisite("GL_OES_EGL_image_external");
    }

    if (param.testID == RestrictedTraceID::professional_baseball_spirits)
    {
        // TODO(https://anglebug.com/5827) Linux+Mesa/RADV Vulkan generates
        // GL_INVALID_FRAMEBUFFER_OPERATION.
        // Mesa versions below 20.3.5 produce the same issue on Linux+Mesa/Intel Vulkan
        if (IsLinux() && (IsAMD() || IsIntel()) &&
            param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE &&
            param.eglParameters.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::call_break_offline_card_game)
    {
        // TODO: http://anglebug.com/5837 Intel Linux Vulkan errors with "Framebuffer is incomplete"
        if ((IsLinux() && IsIntel()) && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::slingshot_test1 ||
        param.testID == RestrictedTraceID::slingshot_test2)
    {
        // TODO: http://anglebug.com/5877 Trace crashes on Pixel 2 in vulkan driver
        if (IsPixel2() && param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::ludo_king)
    {
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");
    }

    // TODO: http://anglebug.com/5943 GL_INVALID_ENUM on Windows/Intel.
    if (param.testID == RestrictedTraceID::summoners_war)
    {
        if (IsWindows() && IsIntel() && param.driver != GLESDriverType::AngleEGL)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::pokemon_go)
    {
        addExtensionPrerequisite("GL_EXT_texture_cube_map_array");
        addExtensionPrerequisite("GL_KHR_texture_compression_astc_ldr");

        // TODO: http://anglebug.com/5989 Intel Linux crashing on teardown
        // TODO: http://anglebug.com/5994 Intel Windows timing out periodically
        if ((IsLinux() || IsWindows()) && IsIntel() &&
            param.getRenderer() == EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE)
        {
            mSkipTest = true;
        }
    }

    if (param.testID == RestrictedTraceID::cookie_run_kingdom)
    {
        addExtensionPrerequisite("GL_EXT_texture_cube_map_array");
        addExtensionPrerequisite("GL_OES_EGL_image_external");

        // TODO: http://anglebug.com/6017 ARM doesn't have enough VS storage blocks
        if (IsAndroid() && IsARM())
        {
            mSkipTest = true;
        }
    }

    // We already swap in TracePerfTest::drawBenchmark, no need to swap again in the harness.
    disableTestHarnessSwap();

    gCurrentTracePerfTest = this;
}

void TracePerfTest::initializeBenchmark()
{
    const auto &params         = GetParam();
    const TraceInfo &traceInfo = GetTraceInfo(params.testID);

    mStartingDirectory = angle::GetCWD().value();

    std::stringstream traceNameStr;
    traceNameStr << "angle_restricted_trace_" << traceInfo.name;
    std::string traceName = traceNameStr.str();
    mTraceLibrary.reset(new TraceLibrary(traceName.c_str()));

    // To load the trace data path correctly we set the CWD to the executable dir.
    if (!IsAndroid())
    {
        std::string exeDir = angle::GetExecutableDirectory();
        angle::SetCWD(exeDir.c_str());
    }

    trace_angle::LoadGLES(TraceLoadProc);

    if (!mTraceLibrary->valid())
    {
        ERR() << "Could not load trace library.";
        mSkipTest = true;
        return;
    }

    mStartFrame = traceInfo.startFrame;
    mEndFrame   = traceInfo.endFrame;
    mTraceLibrary->setBinaryDataDecompressCallback(DecompressBinaryData);

    std::string relativeTestDataDir = std::string("src/tests/restricted_traces/") + traceInfo.name;

    constexpr size_t kMaxDataDirLen = 1000;
    char testDataDir[kMaxDataDirLen];
    if (!angle::FindTestDataPath(relativeTestDataDir.c_str(), testDataDir, kMaxDataDirLen))
    {
        ERR() << "Could not find test data folder.";
        mSkipTest = true;
        return;
    }

    mTraceLibrary->setBinaryDataDir(testDataDir);

    if (gMinimizeGPUWork)
    {
        // Shrink the offscreen window to 1x1.
        mWindowWidth  = 1;
        mWindowHeight = 1;
    }
    else
    {
        mWindowWidth  = mTestParams.windowWidth;
        mWindowHeight = mTestParams.windowHeight;
    }
    mCurrentFrame = mStartFrame;

    if (IsAndroid())
    {
        // On Android, set the orientation used by the app, based on width/height
        getWindow()->setOrientation(mTestParams.windowWidth, mTestParams.windowHeight);
    }

    // If we're rendering offscreen we set up a default backbuffer.
    if (params.surfaceType == SurfaceType::Offscreen)
    {
        if (!IsAndroid())
        {
            mWindowWidth *= 4;
            mWindowHeight *= 4;
        }

        glGenRenderbuffers(1, &mOffscreenDepthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, mOffscreenDepthStencil);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mWindowWidth, mWindowHeight);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);

        glGenFramebuffers(mMaxOffscreenBufferCount, mOffscreenFramebuffers.data());
        glGenTextures(mMaxOffscreenBufferCount, mOffscreenTextures.data());
        for (int i = 0; i < mMaxOffscreenBufferCount; i++)
        {
            glBindFramebuffer(GL_FRAMEBUFFER, mOffscreenFramebuffers[i]);

            // Hard-code RGBA8/D24S8. This should be specified in the trace info.
            glBindTexture(GL_TEXTURE_2D, mOffscreenTextures[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, mWindowWidth, mWindowHeight, 0, GL_RGBA,
                         GL_UNSIGNED_BYTE, nullptr);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                   mOffscreenTextures[i], 0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                                      mOffscreenDepthStencil);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                                      mOffscreenDepthStencil);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }

    // Potentially slow. Can load a lot of resources.
    mTraceLibrary->setupReplay();

    glFinish();

    ASSERT_TRUE(mEndFrame > mStartFrame);

    getWindow()->ignoreSizeEvents();
    getWindow()->setVisible(true);

    // If we're re-tracing, trigger capture start after setup. This ensures the Setup function gets
    // recaptured into another Setup function and not merged with the first frame.
    if (angle::gRetraceMode)
    {
        angle::SetEnvironmentVar("ANGLE_CAPTURE_TRIGGER", "0");
        getGLWindow()->swap();
    }
}

#undef TRACE_TEST_CASE

void TracePerfTest::destroyBenchmark()
{
    const auto &params = GetParam();
    if (params.surfaceType == SurfaceType::Offscreen)
    {
        glDeleteTextures(mMaxOffscreenBufferCount, mOffscreenTextures.data());
        mOffscreenTextures.fill(0);

        glDeleteRenderbuffers(1, &mOffscreenDepthStencil);
        mOffscreenDepthStencil = 0;

        glDeleteFramebuffers(mMaxOffscreenBufferCount, mOffscreenFramebuffers.data());
        mOffscreenFramebuffers.fill(0);
    }

    mTraceLibrary->finishReplay();
    mTraceLibrary.reset(nullptr);

    // In order for the next test to load, restore the working directory
    angle::SetCWD(mStartingDirectory.c_str());
}

void TracePerfTest::sampleTime()
{
    if (mUseTimestampQueries)
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
    constexpr uint32_t kFramesPerX  = 6;
    constexpr uint32_t kFramesPerY  = 4;
    constexpr uint32_t kFramesPerXY = kFramesPerY * kFramesPerX;

    const uint32_t kOffscreenOffsetX =
        static_cast<uint32_t>(static_cast<double>(mTestParams.windowWidth) / 3.0f);
    const uint32_t kOffscreenOffsetY =
        static_cast<uint32_t>(static_cast<double>(mTestParams.windowHeight) / 3.0f);
    const uint32_t kOffscreenWidth  = kOffscreenOffsetX;
    const uint32_t kOffscreenHeight = kOffscreenOffsetY;

    const uint32_t kOffscreenFrameWidth = static_cast<uint32_t>(
        static_cast<double>(kOffscreenWidth / static_cast<double>(kFramesPerX)));
    const uint32_t kOffscreenFrameHeight = static_cast<uint32_t>(
        static_cast<double>(kOffscreenHeight / static_cast<double>(kFramesPerY)));

    const TracePerfParams &params = GetParam();

    // Add a time sample from GL and the host.
    if (mCurrentFrame == mStartFrame)
    {
        sampleTime();
    }

    if (params.surfaceType == SurfaceType::Offscreen)
    {
        // Some driver (ARM and ANGLE) try to nop or defer the glFlush if it is called within the
        // renderpass to avoid breaking renderpass (performance reason). For app traces that does
        // not use any FBO, when we run in the offscreen mode, there is no frame boundary and
        // glFlush call we issued at end of frame will get skipped. To overcome this (and also
        // matches what onscreen double buffering behavior as well), we use two offscreen FBOs and
        // ping pong between them for each frame.
        glBindFramebuffer(GL_FRAMEBUFFER,
                          mOffscreenFramebuffers[mTotalFrameCount % mMaxOffscreenBufferCount]);
    }

    char frameName[32];
    sprintf(frameName, "Frame %u", mCurrentFrame);
    beginInternalTraceEvent(frameName);

    startGpuTimer();
    mTraceLibrary->replayFrame(mCurrentFrame);
    stopGpuTimer();

    if (params.surfaceType == SurfaceType::Offscreen)
    {
        if (gMinimizeGPUWork)
        {
            // To keep GPU work minimum, we skip the blit.
            glFlush();
            mOffscreenFrameCount++;
        }
        else
        {
            GLint currentDrawFBO, currentReadFBO;
            glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &currentDrawFBO);
            glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &currentReadFBO);

            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            glBindFramebuffer(
                GL_READ_FRAMEBUFFER,
                mOffscreenFramebuffers[mOffscreenFrameCount % mMaxOffscreenBufferCount]);

            uint32_t frameX  = (mOffscreenFrameCount % kFramesPerXY) % kFramesPerX;
            uint32_t frameY  = (mOffscreenFrameCount % kFramesPerXY) / kFramesPerX;
            uint32_t windowX = kOffscreenOffsetX + frameX * kOffscreenFrameWidth;
            uint32_t windowY = kOffscreenOffsetY + frameY * kOffscreenFrameHeight;

            if (gVerboseLogging)
            {
                printf("Frame %d: x %d y %d (screen x %d, screen y %d)\n", mOffscreenFrameCount,
                       frameX, frameY, windowX, windowY);
            }

            GLboolean scissorTest = GL_FALSE;
            glGetBooleanv(GL_SCISSOR_TEST, &scissorTest);

            if (scissorTest)
            {
                glDisable(GL_SCISSOR_TEST);
            }

            glBlitFramebuffer(0, 0, mWindowWidth, mWindowHeight, windowX, windowY,
                              windowX + kOffscreenFrameWidth, windowY + kOffscreenFrameHeight,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);

            if (frameX == kFramesPerX - 1 && frameY == kFramesPerY - 1)
            {
                swap();
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glClear(GL_COLOR_BUFFER_BIT);
                mOffscreenFrameCount = 0;
            }
            else
            {
                glFlush();
                mOffscreenFrameCount++;
            }

            if (scissorTest)
            {
                glEnable(GL_SCISSOR_TEST);
            }
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, currentDrawFBO);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, currentReadFBO);
        }

        mTotalFrameCount++;
    }
    else
    {
        swap();
    }

    endInternalTraceEvent(frameName);

    if (mCurrentFrame == mEndFrame)
    {
        mTraceLibrary->resetReplay();
        mCurrentFrame = mStartFrame;
    }
    else
    {
        mCurrentFrame++;
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

// Triggered when the replay calls glBindFramebuffer.
void TracePerfTest::onReplayFramebufferChange(GLenum target, GLuint framebuffer)
{
    if (framebuffer == 0 && GetParam().surfaceType == SurfaceType::Offscreen)
    {
        glBindFramebuffer(target,
                          mOffscreenFramebuffers[mTotalFrameCount % mMaxOffscreenBufferCount]);
    }
    else
    {
        glBindFramebuffer(target, framebuffer);
    }

    switch (target)
    {
        case GL_FRAMEBUFFER:
            mDrawFramebufferBinding = framebuffer;
            mReadFramebufferBinding = framebuffer;
            break;
        case GL_DRAW_FRAMEBUFFER:
            mDrawFramebufferBinding = framebuffer;
            break;
        case GL_READ_FRAMEBUFFER:
            mReadFramebufferBinding = framebuffer;
            return;

        default:
            UNREACHABLE();
            break;
    }

    if (!mUseTimestampQueries)
        return;

    // We have at most one active timestamp query at a time. This code will end the current
    // query and immediately start a new one.
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

bool TracePerfTest::isDefaultFramebuffer(GLenum target) const
{
    switch (target)
    {
        case GL_FRAMEBUFFER:
        case GL_DRAW_FRAMEBUFFER:
            return (mDrawFramebufferBinding == 0);

        case GL_READ_FRAMEBUFFER:
            return (mReadFramebufferBinding == 0);

        default:
            UNREACHABLE();
            return false;
    }
}

GLenum ConvertDefaultFramebufferEnum(GLenum value)
{
    switch (value)
    {
        case GL_NONE:
            return GL_NONE;
        case GL_BACK:
        case GL_COLOR:
            return GL_COLOR_ATTACHMENT0;
        case GL_DEPTH:
            return GL_DEPTH_ATTACHMENT;
        case GL_STENCIL:
            return GL_STENCIL_ATTACHMENT;
        case GL_DEPTH_STENCIL:
            return GL_DEPTH_STENCIL_ATTACHMENT;
        default:
            UNREACHABLE();
            return GL_NONE;
    }
}

std::vector<GLenum> ConvertDefaultFramebufferEnums(GLsizei numAttachments,
                                                   const GLenum *attachments)
{
    std::vector<GLenum> translatedAttachments;
    for (GLsizei attachmentIndex = 0; attachmentIndex < numAttachments; ++attachmentIndex)
    {
        GLenum converted = ConvertDefaultFramebufferEnum(attachments[attachmentIndex]);
        translatedAttachments.push_back(converted);
    }
    return translatedAttachments;
}

// Needs special handling to treat the 0 framebuffer in offscreen mode.
void TracePerfTest::onReplayInvalidateFramebuffer(GLenum target,
                                                  GLsizei numAttachments,
                                                  const GLenum *attachments)
{
    if (GetParam().surfaceType != SurfaceType::Offscreen || !isDefaultFramebuffer(target))
    {
        glInvalidateFramebuffer(target, numAttachments, attachments);
    }
    else
    {
        std::vector<GLenum> translatedAttachments =
            ConvertDefaultFramebufferEnums(numAttachments, attachments);
        glInvalidateFramebuffer(target, numAttachments, translatedAttachments.data());
    }
}

void TracePerfTest::onReplayInvalidateSubFramebuffer(GLenum target,
                                                     GLsizei numAttachments,
                                                     const GLenum *attachments,
                                                     GLint x,
                                                     GLint y,
                                                     GLsizei width,
                                                     GLsizei height)
{
    if (GetParam().surfaceType != SurfaceType::Offscreen || !isDefaultFramebuffer(target))
    {
        glInvalidateSubFramebuffer(target, numAttachments, attachments, x, y, width, height);
    }
    else
    {
        std::vector<GLenum> translatedAttachments =
            ConvertDefaultFramebufferEnums(numAttachments, attachments);
        glInvalidateSubFramebuffer(target, numAttachments, translatedAttachments.data(), x, y,
                                   width, height);
    }
}

void TracePerfTest::onReplayDrawBuffers(GLsizei n, const GLenum *bufs)
{
    if (GetParam().surfaceType != SurfaceType::Offscreen ||
        !isDefaultFramebuffer(GL_DRAW_FRAMEBUFFER))
    {
        glDrawBuffers(n, bufs);
    }
    else
    {
        std::vector<GLenum> translatedBufs = ConvertDefaultFramebufferEnums(n, bufs);
        glDrawBuffers(n, translatedBufs.data());
    }
}

void TracePerfTest::onReplayReadBuffer(GLenum src)
{
    if (GetParam().surfaceType != SurfaceType::Offscreen ||
        !isDefaultFramebuffer(GL_READ_FRAMEBUFFER))
    {
        glReadBuffer(src);
    }
    else
    {
        GLenum translated = ConvertDefaultFramebufferEnum(src);
        glReadBuffer(translated);
    }
}

void TracePerfTest::onReplayDiscardFramebufferEXT(GLenum target,
                                                  GLsizei numAttachments,
                                                  const GLenum *attachments)
{
    if (GetParam().surfaceType != SurfaceType::Offscreen || !isDefaultFramebuffer(target))
    {
        glDiscardFramebufferEXT(target, numAttachments, attachments);
    }
    else
    {
        std::vector<GLenum> translatedAttachments =
            ConvertDefaultFramebufferEnums(numAttachments, attachments);
        glDiscardFramebufferEXT(target, numAttachments, translatedAttachments.data());
    }
}

void TracePerfTest::swap()
{
    // Capture a screenshot if enabled.
    if (gScreenShotDir != nullptr && !mScreenshotSaved)
    {
        std::stringstream screenshotNameStr;
        screenshotNameStr << gScreenShotDir << GetPathSeparator() << "angle" << mBackend << "_"
                          << mStory << ".png";
        std::string screenshotName = screenshotNameStr.str();
        saveScreenshot(screenshotName);
        mScreenshotSaved = true;
    }

    getGLWindow()->swap();
}

void TracePerfTest::saveScreenshot(const std::string &screenshotName)
{
    // The frame is already rendered and is waiting in the default framebuffer.

    // RGBA 4-byte data.
    uint32_t pixelCount = mTestParams.windowWidth * mTestParams.windowHeight;
    std::vector<uint8_t> pixelData(pixelCount * 4);

    // Only unbind the framebuffer on context versions where it's available.
    const TraceInfo &traceInfo = GetTraceInfo(GetParam().testID);
    if (traceInfo.contextClientMajorVersion > 1)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    glReadPixels(0, 0, mTestParams.windowWidth, mTestParams.windowHeight, GL_RGBA, GL_UNSIGNED_BYTE,
                 pixelData.data());

    // Convert to RGB and flip y.
    std::vector<uint8_t> rgbData(pixelCount * 3);
    for (EGLint y = 0; y < mTestParams.windowHeight; ++y)
    {
        for (EGLint x = 0; x < mTestParams.windowWidth; ++x)
        {
            EGLint srcPixel = x + y * mTestParams.windowWidth;
            EGLint dstPixel = x + (mTestParams.windowHeight - y - 1) * mTestParams.windowWidth;
            memcpy(&rgbData[dstPixel * 3], &pixelData[srcPixel * 4], 3);
        }
    }

    if (!angle::SavePNGRGB(screenshotName.c_str(), "ANGLE Screenshot", mTestParams.windowWidth,
                           mTestParams.windowHeight, rgbData))
    {
        FAIL() << "Error saving screenshot: " << screenshotName;
    }
    else
    {
        printf("Saved screenshot: '%s'\n", screenshotName.c_str());
    }
}

TEST_P(TracePerfTest, Run)
{
    run();
}

TracePerfParams CombineTestID(const TracePerfParams &in, RestrictedTraceID id)
{
    const TraceInfo &traceInfo = GetTraceInfo(id);

    TracePerfParams out = in;
    out.testID          = id;
    out.majorVersion    = traceInfo.contextClientMajorVersion;
    out.minorVersion    = traceInfo.contextClientMinorVersion;
    out.windowWidth     = traceInfo.drawSurfaceWidth;
    out.windowHeight    = traceInfo.drawSurfaceHeight;
    return out;
}

bool NoAndroidMockICD(const TracePerfParams &in)
{
    return in.eglParameters.deviceType != EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE || !IsAndroid();
}

TracePerfParams CombineWithSurfaceType(const TracePerfParams &in, SurfaceType surfaceType)
{
    TracePerfParams out = in;
    out.surfaceType     = surfaceType;

    if (!IsAndroid() && surfaceType == SurfaceType::Offscreen)
    {
        out.windowWidth /= 4;
        out.windowHeight /= 4;
    }

    // We track GPU time only in frame-rate-limited cases.
    out.trackGpuTime = surfaceType == SurfaceType::WindowWithVSync;

    return out;
}

using namespace params;
using P = TracePerfParams;

std::vector<P> gTestsWithID =
    CombineWithValues({P()}, AllEnums<RestrictedTraceID>(), CombineTestID);
std::vector<P> gTestsWithSurfaceType =
    CombineWithValues(gTestsWithID,
                      {SurfaceType::Offscreen, SurfaceType::Window, SurfaceType::WindowWithVSync},
                      CombineWithSurfaceType);
std::vector<P> gTestsWithRenderer =
    CombineWithFuncs(gTestsWithSurfaceType,
                     {Vulkan<P>, VulkanMockICD<P>, VulkanSwiftShader<P>, Native<P>});
std::vector<P> gTestsWithoutMockICD = FilterWithFunc(gTestsWithRenderer, NoAndroidMockICD);
ANGLE_INSTANTIATE_TEST_ARRAY(TracePerfTest, gTestsWithoutMockICD);

}  // anonymous namespace

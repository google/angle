//
// Copyright 2026 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/unsafe_buffers.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/renderer/gl/FramebufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/QueryGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"
#include "libGLESv2/global_state.h"
#include "tests/angle_unittests_utils.h"
#include "tests/test_utils/angle_test_instantiate.h"

namespace rx
{
namespace
{

enum class DriverCallType
{
    BindFramebuffer,
    DeleteFramebuffer,
    FlushSync,
    Finish,
};

struct DriverCall
{
    DriverCallType type = DriverCallType::BindFramebuffer;
    GLenum target       = 0;
    GLuint framebuffer  = 0;
};

struct MockDriverState
{
    GLboolean blendEnabled             = GL_FALSE;
    std::array<GLboolean, 4> colorMask = {GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE};
    GLuint readFramebuffer             = 0;
    GLuint drawFramebuffer             = 0;
    int enableBlendCallCount           = 0;
    int disableBlendCallCount          = 0;
    int colorMaskCallCount             = 0;
    int fenceSyncCallCount             = 0;
    int deleteSyncCallCount            = 0;
    std::vector<DriverCall> calls;
};

MockDriverState *gDriverState = nullptr;

void GL_APIENTRY StubBeginQuery(GLenum, GLuint) {}
void GL_APIENTRY StubEndQuery(GLenum) {}
GLenum GL_APIENTRY StubGetError()
{
    return GL_NO_ERROR;
}
void GL_APIENTRY StubGetIntegerv(GLenum pname, GLint *data)
{
    if (pname == GL_VIEWPORT || pname == GL_SCISSOR_BOX)
    {
        // SAFETY: Spec requires 4 GLints.
        ANGLE_UNSAFE_BUFFERS({
            data[0] = 0;
            data[1] = 0;
            data[2] = 0;
            data[3] = 0;
        });
    }
    else if (pname == GL_ACTIVE_TEXTURE)
    {
        *data = GL_TEXTURE0;
    }
    else if (pname == GL_COLOR_WRITEMASK && gDriverState != nullptr)
    {
        // SAFETY: Spec requires 4 GLints.
        ANGLE_UNSAFE_BUFFERS({
            data[0] = gDriverState->colorMask[0];
            data[1] = gDriverState->colorMask[1];
            data[2] = gDriverState->colorMask[2];
            data[3] = gDriverState->colorMask[3];
        });
    }
    else if (pname == GL_READ_FRAMEBUFFER_BINDING && gDriverState != nullptr)
    {
        *data = gDriverState->readFramebuffer;
    }
    else if ((pname == GL_DRAW_FRAMEBUFFER_BINDING || pname == GL_FRAMEBUFFER_BINDING) &&
             gDriverState != nullptr)
    {
        *data = gDriverState->drawFramebuffer;
    }
}
void GL_APIENTRY StubGetFloatv(GLenum, GLfloat *) {}
void GL_APIENTRY StubGetBooleanv(GLenum, GLboolean *) {}
GLboolean GL_APIENTRY StubIsEnabled(GLenum cap)
{
    if (cap == GL_BLEND && gDriverState != nullptr)
    {
        return gDriverState->blendEnabled;
    }
    return GL_FALSE;
}
void GL_APIENTRY StubEnable(GLenum cap)
{
    if (cap == GL_BLEND && gDriverState != nullptr)
    {
        gDriverState->enableBlendCallCount++;
    }
}
void GL_APIENTRY StubDisable(GLenum cap)
{
    if (cap == GL_BLEND && gDriverState != nullptr)
    {
        gDriverState->disableBlendCallCount++;
    }
}
void GL_APIENTRY StubColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    if (gDriverState != nullptr)
    {
        gDriverState->colorMaskCallCount++;
    }
}
void GL_APIENTRY StubGenFramebuffers(GLsizei, GLuint *framebuffers)
{
    *framebuffers = 1;
}
void GL_APIENTRY StubBindFramebuffer(GLenum target, GLuint framebuffer)
{
    if (gDriverState == nullptr)
    {
        return;
    }
    if (target == GL_READ_FRAMEBUFFER || target == GL_FRAMEBUFFER)
    {
        gDriverState->readFramebuffer = framebuffer;
    }
    if (target == GL_DRAW_FRAMEBUFFER || target == GL_FRAMEBUFFER)
    {
        gDriverState->drawFramebuffer = framebuffer;
    }
    gDriverState->calls.push_back({DriverCallType::BindFramebuffer, target, framebuffer});
}
void GL_APIENTRY StubDeleteFramebuffers(GLsizei count, const GLuint *framebuffers)
{
    if (gDriverState != nullptr && count > 0 && framebuffers != nullptr)
    {
        gDriverState->calls.push_back({DriverCallType::DeleteFramebuffer, 0, *framebuffers});
    }
}
GLsync GL_APIENTRY StubFenceSync(GLenum, GLbitfield)
{
    if (gDriverState != nullptr)
    {
        gDriverState->fenceSyncCallCount++;
    }
    static int sDummySync = 0;
    return reinterpret_cast<GLsync>(&sDummySync);
}
GLenum GL_APIENTRY StubClientWaitSync(GLsync, GLbitfield flags, GLuint64 timeout)
{
    if (gDriverState != nullptr && (flags & GL_SYNC_FLUSH_COMMANDS_BIT) != 0 && timeout == 0)
    {
        gDriverState->calls.push_back(
            {DriverCallType::FlushSync, 0, gDriverState->drawFramebuffer});
    }
    return GL_ALREADY_SIGNALED;
}
void GL_APIENTRY StubDeleteSync(GLsync)
{
    if (gDriverState != nullptr)
    {
        gDriverState->deleteSyncCallCount++;
    }
}
void GL_APIENTRY StubFinish()
{
    if (gDriverState != nullptr)
    {
        gDriverState->calls.push_back({DriverCallType::Finish, 0, gDriverState->drawFramebuffer});
    }
}
void GL_APIENTRY StubGenRenderbuffers(GLsizei, GLuint *renderbuffers)
{
    *renderbuffers = 1;
}
void GL_APIENTRY StubBindRenderbuffer(GLenum, GLuint) {}
void GL_APIENTRY StubDeleteRenderbuffers(GLsizei, const GLuint *) {}
void GL_APIENTRY StubRenderbufferStorage(GLenum, GLenum, GLsizei, GLsizei) {}
void GL_APIENTRY StubFramebufferRenderbuffer(GLenum, GLenum, GLenum, GLuint) {}
GLenum GL_APIENTRY StubCheckFramebufferStatus(GLenum)
{
    return GL_FRAMEBUFFER_COMPLETE;
}
void GL_APIENTRY StubActiveTexture(GLenum) {}
void GL_APIENTRY StubBindVertexArray(GLuint) {}
void GL_APIENTRY StubGetVertexAttribiv(GLuint, GLenum, GLint *) {}
void GL_APIENTRY StubGetVertexAttribfv(GLuint, GLenum, GLfloat *) {}
void GL_APIENTRY StubGetVertexAttribPointerv(GLuint, GLenum, void **) {}

// Minimal FunctionsGL that never calls into a real driver.
class TestFunctionsGL : public FunctionsGL
{
  public:
    TestFunctionsGL()
    {
        standard = STANDARD_GL_ES;
        version  = gl::Version(3, 0);
        profile  = 0;

        beginQuery              = StubBeginQuery;
        endQuery                = StubEndQuery;
        getError                = StubGetError;
        getIntegerv             = StubGetIntegerv;
        getFloatv               = StubGetFloatv;
        getBooleanv             = StubGetBooleanv;
        isEnabled               = StubIsEnabled;
        enable                  = StubEnable;
        disable                 = StubDisable;
        colorMask               = StubColorMask;
        genFramebuffers         = StubGenFramebuffers;
        bindFramebuffer         = StubBindFramebuffer;
        deleteFramebuffers      = StubDeleteFramebuffers;
        fenceSync               = StubFenceSync;
        clientWaitSync          = StubClientWaitSync;
        deleteSync              = StubDeleteSync;
        finish                  = StubFinish;
        genRenderbuffers        = StubGenRenderbuffers;
        bindRenderbuffer        = StubBindRenderbuffer;
        deleteRenderbuffers     = StubDeleteRenderbuffers;
        renderbufferStorage     = StubRenderbufferStorage;
        framebufferRenderbuffer = StubFramebufferRenderbuffer;
        checkFramebufferStatus  = StubCheckFramebufferStatus;
        activeTexture           = StubActiveTexture;
        bindVertexArray         = StubBindVertexArray;
        getVertexAttribiv       = StubGetVertexAttribiv;
        getVertexAttribfv       = StubGetVertexAttribfv;
        getVertexAttribPointerv = StubGetVertexAttribPointerv;
    }

  private:
    void *loadProcAddress(const std::string &function) const override { return nullptr; }
};

// QueryGL implementation that records pause/resume calls and can be forced to
// fail resume() so that error-propagation paths in StateManagerGL are covered.
class TestQueryGL : public QueryGL
{
  public:
    explicit TestQueryGL(gl::QueryType type) : QueryGL(type) {}

    angle::Result begin(const gl::Context *) override { return angle::Result::Continue; }
    angle::Result end(const gl::Context *) override { return angle::Result::Continue; }
    angle::Result queryCounter(const gl::Context *) override { return angle::Result::Continue; }
    angle::Result getResult(const gl::Context *, GLint *) override
    {
        return angle::Result::Continue;
    }
    angle::Result getResult(const gl::Context *, GLuint *) override
    {
        return angle::Result::Continue;
    }
    angle::Result getResult(const gl::Context *, GLint64 *) override
    {
        return angle::Result::Continue;
    }
    angle::Result getResult(const gl::Context *, GLuint64 *) override
    {
        return angle::Result::Continue;
    }
    angle::Result isResultAvailable(const gl::Context *, bool *) override
    {
        return angle::Result::Continue;
    }

    angle::Result pause(const gl::Context *) override
    {
        mPauseCount++;
        return angle::Result::Continue;
    }
    angle::Result resume(const gl::Context *) override
    {
        mResumeCount++;
        return mResumeResult;
    }

    void setResumeResult(angle::Result result) { mResumeResult = result; }
    int pauseCount() const { return mPauseCount; }
    int resumeCount() const { return mResumeCount; }

  private:
    angle::Result mResumeResult = angle::Result::Continue;
    int mPauseCount             = 0;
    int mResumeCount            = 0;
};

gl::Caps MakeMinimalCaps()
{
    gl::Caps caps;
    caps.maxDrawBuffers          = 1;
    caps.maxVertexAttributes     = 1;
    caps.maxVertexAttribBindings = 1;
    return caps;
}

class StateManagerGLTest : public ::testing::Test
{
  protected:
    StateManagerGLTest()
        : mCaps(MakeMinimalCaps()), mStateManager(&mFunctions, mCaps, mExtensions, mFeatures)
    {
        gDriverState = &mDriverState;
    }

    ~StateManagerGLTest() override { gDriverState = nullptr; }

    StateManagerGL &stateManager() { return mStateManager; }
    MockDriverState &driverState() { return mDriverState; }
    TestFunctionsGL &mutableFunctions() { return mFunctions; }
    const TestFunctionsGL &functions() const { return mFunctions; }
    angle::FeaturesGL &features() { return mFeatures; }

  private:
    MockDriverState mDriverState;
    TestFunctionsGL mFunctions;
    gl::Caps mCaps;
    gl::Extensions mExtensions;
    angle::FeaturesGL mFeatures;
    StateManagerGL mStateManager;
};

// When resume() reports an error for a paused query, StateManagerGL must still
// drop its reference so that a later resumeAllQueries() call does not touch the
// query again after it may have been destroyed.
TEST_F(StateManagerGLTest, ResumeAllQueriesClearsPausedQueryOnError)
{
    TestQueryGL query(gl::QueryType::AnySamples);

    stateManager().beginQuery(gl::QueryType::AnySamples, &query, 1);
    EXPECT_EQ(angle::Result::Continue, stateManager().pauseAllQueries(nullptr));
    EXPECT_EQ(1, query.pauseCount());

    query.setResumeResult(angle::Result::Stop);
    EXPECT_EQ(angle::Result::Stop, stateManager().resumeAllQueries(nullptr));
    EXPECT_EQ(1, query.resumeCount());

    // The query is no longer tracked; a subsequent resume must not reach it.
    query.setResumeResult(angle::Result::Continue);
    EXPECT_EQ(angle::Result::Continue, stateManager().resumeAllQueries(nullptr));
    EXPECT_EQ(1, query.resumeCount());
}

// If one paused query fails to resume, StateManagerGL must still drop its
// references to every other paused query rather than leaving them behind.
TEST_F(StateManagerGLTest, ResumeAllQueriesClearsAllPausedQueriesOnError)
{
    TestQueryGL failingQuery(gl::QueryType::AnySamples);
    TestQueryGL otherQuery(gl::QueryType::TransformFeedbackPrimitivesWritten);

    stateManager().beginQuery(failingQuery.getType(), &failingQuery, 1);
    stateManager().beginQuery(otherQuery.getType(), &otherQuery, 2);
    EXPECT_EQ(angle::Result::Continue, stateManager().pauseAllQueries(nullptr));

    failingQuery.setResumeResult(angle::Result::Stop);
    EXPECT_EQ(angle::Result::Stop, stateManager().resumeAllQueries(nullptr));

    // Neither query should be reached by a subsequent resume.
    int failingResumeCount = failingQuery.resumeCount();
    int otherResumeCount   = otherQuery.resumeCount();
    EXPECT_EQ(angle::Result::Continue, stateManager().resumeAllQueries(nullptr));
    EXPECT_EQ(failingResumeCount, failingQuery.resumeCount());
    EXPECT_EQ(otherResumeCount, otherQuery.resumeCount());
}

// resumeQuery() must clear its paused-query reference even when resume() fails.
TEST_F(StateManagerGLTest, ResumeQueryClearsPausedQueryOnError)
{
    TestQueryGL query(gl::QueryType::AnySamples);

    stateManager().beginQuery(gl::QueryType::AnySamples, &query, 1);
    EXPECT_EQ(angle::Result::Continue,
              stateManager().pauseQuery(nullptr, gl::QueryType::AnySamples));

    query.setResumeResult(angle::Result::Stop);
    EXPECT_EQ(angle::Result::Stop, stateManager().resumeQuery(nullptr, gl::QueryType::AnySamples));
    EXPECT_EQ(1, query.resumeCount());

    query.setResumeResult(angle::Result::Continue);
    EXPECT_EQ(angle::Result::Continue,
              stateManager().resumeQuery(nullptr, gl::QueryType::AnySamples));
    EXPECT_EQ(1, query.resumeCount());
}

class TestFboFactory : public NullFactory
{
  public:
    TestFboFactory(const FunctionsGL *functions, StateManagerGL *stateManager)
        : mFunctions(functions), mStateManager(stateManager)
    {}

    FramebufferImpl *createFramebuffer(const gl::FramebufferState &data) override
    {
        return new FramebufferGL(data, 0, false, mFunctions, mStateManager);
    }

    MemoryObjectImpl *createMemoryObject() override { return nullptr; }

  private:
    const FunctionsGL *mFunctions = nullptr;
    StateManagerGL *mStateManager = nullptr;
};

class ScopedTestContext
{
  public:
    ScopedTestContext(const FunctionsGL *functions, StateManagerGL *stateManager)
        : mFboFactory(functions, stateManager)
    {
        egl::AttributeMap displayAttribs;
        displayAttribs.insert(EGL_PLATFORM_ANGLE_TYPE_ANGLE, EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE);
        mDisplay = egl::Display::GetDisplayFromNativeDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                                                             EGL_DEFAULT_DISPLAY, displayAttribs);
        if (!mDisplay)
        {
            return;
        }
        egl::ScopedDisplayMutexLock lock(*mDisplay);
        EXPECT_EQ(EGL_SUCCESS, mDisplay->initialize().getCode());

        egl::AttributeMap contextAttribs;
        contextAttribs.insert(EGL_CONTEXT_CLIENT_VERSION, 3);
        EXPECT_EQ(EGL_SUCCESS,
                  mDisplay->createContext(nullptr, nullptr, contextAttribs, &mContext).getCode());
        EXPECT_NE(nullptr, mContext);

        mDefaultFbo = new gl::Framebuffer(mContext, &mFboFactory);
        const_cast<gl::State &>(mContext->getState()).setDrawFramebufferBinding(mDefaultFbo);
    }

    ~ScopedTestContext()
    {
        if (!mDisplay)
        {
            return;
        }
        egl::ScopedDisplayMutexLock lock(*mDisplay);
        const_cast<gl::State &>(mContext->getState()).setDrawFramebufferBinding(nullptr);
        mDefaultFbo->onDestroy(mContext);
        delete mDefaultFbo;
        // Mimic how the EGL entry point API does, which always holds reference count before calling
        // into Display::destroyContext.
        mContext->addRef();
        EXPECT_EQ(EGL_SUCCESS, mDisplay->destroyContext(mContext).getCode());
        EXPECT_EQ(EGL_SUCCESS,
                  mDisplay->terminate(egl::GetCurrentThread(), egl::Display::TerminateReason::Api)
                      .getCode());
    }

    gl::Context *context() const { return mContext; }

  private:
    TestFboFactory mFboFactory;
    egl::Display *mDisplay       = nullptr;
    gl::Context *mContext        = nullptr;
    gl::Framebuffer *mDefaultFbo = nullptr;
};

// Verify that when the native driver has blend enabled while StateManagerGL
// tracks blend as disabled, syncFromNativeContext sets DIRTY_BIT_BLEND_ENABLED
// (and not DIRTY_BIT_COLOR_MASK) so a subsequent syncState re-applies blend
// enable state without spuriously touching color mask.
TEST_F(StateManagerGLTest, SyncFromNativeContextBlendEnabled)
{
    ScopedTestContext testContext(&functions(), &stateManager());
    ANGLE_SKIP_TEST_IF(!testContext.context());  // Skip if GL not available.
    gl::Context *context = testContext.context();

    // Native driver has GL_BLEND enabled while frontend context has blend disabled.
    driverState().blendEnabled = GL_TRUE;

    std::unique_ptr<ContextStateGL> nativeState = stateManager().createContextStateGL();
    EXPECT_EQ(angle::Result::Continue,
              stateManager().syncFromNativeContext(context, nativeState.get()));

    driverState().enableBlendCallCount  = 0;
    driverState().disableBlendCallCount = 0;
    driverState().colorMaskCallCount    = 0;

    EXPECT_EQ(
        angle::Result::Continue,
        stateManager().syncState(context, gl::state::DirtyBits(), gl::state::DirtyBits().set(),
                                 gl::state::ExtendedDirtyBits(), gl::state::ExtendedDirtyBits()));

    EXPECT_EQ(1, driverState().disableBlendCallCount);
    EXPECT_EQ(0, driverState().enableBlendCallCount);
    EXPECT_EQ(0, driverState().colorMaskCallCount);
}

// Verify that when the native driver has a different color mask than
// StateManagerGL, syncFromNativeContext sets DIRTY_BIT_COLOR_MASK (and not
// DIRTY_BIT_BLEND_ENABLED) so a subsequent syncState re-applies color mask
// without spuriously touching blend enable state.
TEST_F(StateManagerGLTest, SyncFromNativeContextColorMask)
{
    ScopedTestContext testContext(&functions(), &stateManager());
    ANGLE_SKIP_TEST_IF(!testContext.context());  // Skip if GL not available.
    gl::Context *context = testContext.context();

    // Native driver has red write disabled while frontend context has RGBA all enabled.
    driverState().colorMask = {GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE};

    std::unique_ptr<ContextStateGL> nativeState = stateManager().createContextStateGL();
    EXPECT_EQ(angle::Result::Continue,
              stateManager().syncFromNativeContext(context, nativeState.get()));

    driverState().enableBlendCallCount  = 0;
    driverState().disableBlendCallCount = 0;
    driverState().colorMaskCallCount    = 0;

    EXPECT_EQ(
        angle::Result::Continue,
        stateManager().syncState(context, gl::state::DirtyBits(), gl::state::DirtyBits().set(),
                                 gl::state::ExtendedDirtyBits(), gl::state::ExtendedDirtyBits()));

    EXPECT_EQ(1, driverState().colorMaskCallCount);
    EXPECT_EQ(0, driverState().enableBlendCallCount);
    EXPECT_EQ(0, driverState().disableBlendCallCount);
}

// Verify that when a non-default framebuffer is bound in the native driver on
// the first call to syncFromNativeContext, lazy creation of the placeholder
// framebuffer does not clobber the native draw framebuffer binding before it
// is queried.
TEST_F(StateManagerGLTest, SyncFromNativeContextNonDefaultFramebuffer)
{
    constexpr GLuint kNativeFbo   = 42;
    driverState().readFramebuffer = kNativeFbo;
    driverState().drawFramebuffer = kNativeFbo;

    std::unique_ptr<ContextStateGL> nativeState = stateManager().createContextStateGL();
    EXPECT_EQ(angle::Result::Continue,
              stateManager().syncFromNativeContext(nullptr, nativeState.get()));

    EXPECT_EQ(kNativeFbo, nativeState->framebuffers[angle::FramebufferBindingRead]);
    EXPECT_EQ(kNativeFbo, nativeState->framebuffers[angle::FramebufferBindingDraw]);
    EXPECT_EQ(kNativeFbo, stateManager().getFramebufferID(angle::FramebufferBindingRead));
    EXPECT_EQ(kNativeFbo, stateManager().getFramebufferID(angle::FramebufferBindingDraw));
    EXPECT_EQ(kNativeFbo, driverState().readFramebuffer);
    EXPECT_EQ(kNativeFbo, driverState().drawFramebuffer);
}

// Verify that when flushBeforeDrawFramebufferChange is enabled, switching away from a
// non-default draw framebuffer flushes pending commands while the outgoing draw framebuffer is
// still bound, while leaving default framebuffer 0, redundant binds, and read-only changes do
// not flush.
TEST_F(StateManagerGLTest, FlushBeforeDrawFramebufferChangeOrdering)
{
    ASSERT_TRUE(stateManager().getHasSeparateFramebufferBindings());
    features().flushBeforeDrawFramebufferChange.enabled = true;
    driverState().calls.clear();

    // 0 -> 10 on GL_FRAMEBUFFER: leaving default FBO 0 does not flush.
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 10);
    ASSERT_EQ(1u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[0].type);
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER), driverState().calls[0].target);
    EXPECT_EQ(10u, driverState().calls[0].framebuffer);

    // Redundant bind to 10: no flush and no native bind.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 10);
    EXPECT_TRUE(driverState().calls.empty());

    // 10 -> 20 on GL_FRAMEBUFFER: flush while 10 is still bound, then bind 20.
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 20);
    ASSERT_EQ(2u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::FlushSync, driverState().calls[0].type);
    EXPECT_EQ(10u, driverState().calls[0].framebuffer);
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[1].type);
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER), driverState().calls[1].target);
    EXPECT_EQ(20u, driverState().calls[1].framebuffer);
    EXPECT_EQ(1, driverState().fenceSyncCallCount);
    EXPECT_EQ(1, driverState().deleteSyncCallCount);

    // Changing only GL_READ_FRAMEBUFFER (20 -> 30) while draw remains 20: bind read, no flush.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_READ_FRAMEBUFFER, 30);
    ASSERT_EQ(1u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[0].type);
    EXPECT_EQ(static_cast<GLenum>(GL_READ_FRAMEBUFFER), driverState().calls[0].target);
    EXPECT_EQ(30u, driverState().calls[0].framebuffer);

    // GL_FRAMEBUFFER bind to 20 when Read == 30 and Draw == 20: draw FBO is unchanged, no flush.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 20);
    ASSERT_EQ(1u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[0].type);
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER), driverState().calls[0].target);
    EXPECT_EQ(20u, driverState().calls[0].framebuffer);

    // GL_DRAW_FRAMEBUFFER transition 20 -> 30: flush while 20 is still bound, then bind 30,
    // preserving Read == 20.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_DRAW_FRAMEBUFFER, 30);
    ASSERT_EQ(2u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::FlushSync, driverState().calls[0].type);
    EXPECT_EQ(20u, driverState().calls[0].framebuffer);
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[1].type);
    EXPECT_EQ(static_cast<GLenum>(GL_DRAW_FRAMEBUFFER), driverState().calls[1].target);
    EXPECT_EQ(30u, driverState().calls[1].framebuffer);
    EXPECT_EQ(20u, driverState().readFramebuffer);
    EXPECT_EQ(2, driverState().fenceSyncCallCount);
    EXPECT_EQ(2, driverState().deleteSyncCallCount);

    // GL_FRAMEBUFFER transition 30 -> 0: flush while 30 is still bound, then bind 0.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 0);
    ASSERT_EQ(2u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::FlushSync, driverState().calls[0].type);
    EXPECT_EQ(30u, driverState().calls[0].framebuffer);
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[1].type);
    EXPECT_EQ(static_cast<GLenum>(GL_FRAMEBUFFER), driverState().calls[1].target);
    EXPECT_EQ(0u, driverState().calls[1].framebuffer);
}

// Verify deleting a currently bound non-default draw framebuffer flushes pending work before
// unbinding and deleting it, without issuing a second flush when binding a new FBO from 0.
TEST_F(StateManagerGLTest, FlushBeforeDrawFramebufferChangeOnDeleteBoundDrawFramebuffer)
{
    features().flushBeforeDrawFramebufferChange.enabled = true;
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 15);
    driverState().calls.clear();

    stateManager().deleteFramebuffer(15);
    ASSERT_EQ(4u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[0].type);
    EXPECT_EQ(static_cast<GLenum>(GL_READ_FRAMEBUFFER), driverState().calls[0].target);
    EXPECT_EQ(0u, driverState().calls[0].framebuffer);
    EXPECT_EQ(DriverCallType::FlushSync, driverState().calls[1].type);
    EXPECT_EQ(15u, driverState().calls[1].framebuffer);
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[2].type);
    EXPECT_EQ(static_cast<GLenum>(GL_DRAW_FRAMEBUFFER), driverState().calls[2].target);
    EXPECT_EQ(0u, driverState().calls[2].framebuffer);
    EXPECT_EQ(DriverCallType::DeleteFramebuffer, driverState().calls[3].type);
    EXPECT_EQ(15u, driverState().calls[3].framebuffer);

    // Subsequent 0 -> 25 bind does not double-flush.
    driverState().calls.clear();
    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 25);
    ASSERT_EQ(1u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[0].type);
    EXPECT_EQ(25u, driverState().calls[0].framebuffer);
}

// Verify finish() fallback in forcefullyFlush() when sync entry points are unavailable.
TEST_F(StateManagerGLTest, FlushBeforeDrawFramebufferChangeFinishFallback)
{
    features().flushBeforeDrawFramebufferChange.enabled = true;
    mutableFunctions().fenceSync                        = nullptr;

    stateManager().bindFramebuffer(GL_FRAMEBUFFER, 10);
    driverState().calls.clear();

    stateManager().bindFramebuffer(GL_DRAW_FRAMEBUFFER, 20);
    ASSERT_EQ(2u, driverState().calls.size());
    EXPECT_EQ(DriverCallType::Finish, driverState().calls[0].type);
    EXPECT_EQ(10u, driverState().calls[0].framebuffer);
    EXPECT_EQ(DriverCallType::BindFramebuffer, driverState().calls[1].type);
    EXPECT_EQ(20u, driverState().calls[1].framebuffer);
}

}  // namespace
}  // namespace rx

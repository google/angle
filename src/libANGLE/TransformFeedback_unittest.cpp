//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "libANGLE/TransformFeedback.h"
#include "libANGLE/renderer/TransformFeedbackImpl.h"

namespace {

class MockTransformFeedbackImpl : public rx::TransformFeedbackImpl
{
  public:
    virtual ~MockTransformFeedbackImpl() { destroy(); }

    MOCK_METHOD1(begin, void(GLenum primitiveMode));
    MOCK_METHOD0(end, void());
    MOCK_METHOD0(pause, void());
    MOCK_METHOD0(resume, void());

    MOCK_METHOD0(destroy, void());
};

class TransformFeedbackTest : public testing::Test
{
  protected:
    virtual void SetUp()
    {
        mImpl = new MockTransformFeedbackImpl;
        EXPECT_CALL(*mImpl, destroy());
        mFeedback = new gl::TransformFeedback(mImpl, 1);
        mFeedback->addRef();
    }

    virtual void TearDown()
    {
        mFeedback->release();
    }

    MockTransformFeedbackImpl* mImpl;
    gl::TransformFeedback* mFeedback;
};

TEST_F(TransformFeedbackTest, DestructionDeletesImpl)
{
    MockTransformFeedbackImpl* impl = new MockTransformFeedbackImpl;
    EXPECT_CALL(*impl, destroy()).Times(1).RetiresOnSaturation();

    gl::TransformFeedback* feedback = new gl::TransformFeedback(impl, 1);
    feedback->addRef();
    feedback->release();

    // Only needed because the mock is leaked if bugs are present,
    // which logs an error, but does not cause the test to fail.
    // Ordinarily mocks are verified when destroyed.
    testing::Mock::VerifyAndClear(impl);
}

TEST_F(TransformFeedbackTest, SideEffectsOfStartAndStop)
{
    testing::InSequence seq;

    EXPECT_FALSE(mFeedback->isActive());
    EXPECT_CALL(*mImpl, begin(GL_TRIANGLES));
    mFeedback->begin(GL_TRIANGLES);
    EXPECT_TRUE(mFeedback->isActive());
    EXPECT_EQ(static_cast<GLenum>(GL_TRIANGLES), mFeedback->getPrimitiveMode());
    EXPECT_CALL(*mImpl, end());
    mFeedback->end();
    EXPECT_FALSE(mFeedback->isActive());
}

TEST_F(TransformFeedbackTest, SideEffectsOfPauseAndResume)
{
    testing::InSequence seq;

    EXPECT_FALSE(mFeedback->isActive());
    EXPECT_CALL(*mImpl, begin(GL_TRIANGLES));
    mFeedback->begin(GL_TRIANGLES);
    EXPECT_FALSE(mFeedback->isPaused());
    EXPECT_CALL(*mImpl, pause());
    mFeedback->pause();
    EXPECT_TRUE(mFeedback->isPaused());
    EXPECT_CALL(*mImpl, resume());
    mFeedback->resume();
    EXPECT_FALSE(mFeedback->isPaused());
    EXPECT_CALL(*mImpl, end());
    mFeedback->end();
}

} // namespace

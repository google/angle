//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// IndexDataManagerPerfTest:
//   Performance test for index buffer management.
//

#include "ANGLEPerfTest.h"

#include <gmock/gmock.h>

#include "libANGLE/renderer/d3d/BufferD3D.h"
#include "libANGLE/renderer/d3d/IndexBuffer.h"
#include "libANGLE/renderer/d3d/IndexDataManager.h"

using namespace testing;

namespace
{

class MockIndexBuffer : public rx::IndexBuffer
{
  public:
    MockIndexBuffer(unsigned int bufferSize, GLenum indexType)
        : mBufferSize(bufferSize),
          mIndexType(indexType)
    {
    }

    MOCK_METHOD3(initialize, gl::Error(unsigned int, GLenum, bool));
    MOCK_METHOD3(mapBuffer, gl::Error(unsigned int, unsigned int, void**));
    MOCK_METHOD0(unmapBuffer, gl::Error());
    MOCK_METHOD0(discard, gl::Error());
    MOCK_METHOD2(setSize, gl::Error(unsigned int, GLenum));

    // inlined for speed
    GLenum getIndexType() const override { return mIndexType; }
    unsigned int getBufferSize() const override { return mBufferSize; }

  private:
    unsigned int mBufferSize;
    GLenum mIndexType;
};

class MockBufferFactoryD3D : public rx::BufferFactoryD3D
{
  public:
    MockBufferFactoryD3D(unsigned int bufferSize, GLenum indexType)
        : mBufferSize(bufferSize),
          mIndexType(indexType)
    {
    }

    MOCK_METHOD0(createVertexBuffer, rx::VertexBuffer*());
    MOCK_CONST_METHOD1(getVertexConversionType, rx::VertexConversionType(const gl::VertexFormat &));
    MOCK_CONST_METHOD1(getVertexComponentType, GLenum(const gl::VertexFormat &));

    // Dependency injection
    rx::IndexBuffer* createIndexBuffer() override
    {
        return new MockIndexBuffer(mBufferSize, mIndexType);
    }

  private:
    unsigned int mBufferSize;
    GLenum mIndexType;
};

class MockBufferD3D : public rx::BufferD3D
{
  public:
    MockBufferD3D(rx::BufferFactoryD3D *factory, size_t bufferSize)
        : BufferD3D(factory),
          mBufferSize(bufferSize)
    {
    }

    // BufferImpl
    MOCK_METHOD3(setData, gl::Error(const void*, size_t, GLenum));
    MOCK_METHOD3(setSubData, gl::Error(const void*, size_t, size_t));
    MOCK_METHOD4(copySubData, gl::Error(BufferImpl*, GLintptr, GLintptr, GLsizeiptr));
    MOCK_METHOD2(map, gl::Error(GLenum, GLvoid **));
    MOCK_METHOD4(mapRange, gl::Error(size_t, size_t, GLbitfield, GLvoid **));
    MOCK_METHOD1(unmap, gl::Error(GLboolean *));

    // BufferD3D
    MOCK_METHOD0(markTransformFeedbackUsage, void());

    // inlined for speed
    bool supportsDirectBinding() const override { return false; }
    size_t getSize() const override { return mBufferSize; }
    gl::Error getData(const uint8_t **) override { return gl::Error(GL_NO_ERROR); }

  private:
    size_t mBufferSize;
};

class IndexDataManagerPerfTest : public ANGLEPerfTest
{
  public:
    IndexDataManagerPerfTest();

    void step(float dt, double totalTime) override;

    rx::IndexDataManager mIndexDataManager;
    GLsizei mIndexCount;
    unsigned int mBufferSize;
    MockBufferFactoryD3D mMockFactory;
    gl::Buffer mIndexBuffer;
    std::vector<GLshort> mIndexData;
};

MockBufferD3D *InitMockBufferD3D(MockBufferFactoryD3D *mockFactory, unsigned int bufferSize)
{
    MockBufferD3D *mockBufferD3D = new MockBufferD3D(mockFactory, static_cast<size_t>(bufferSize));

    EXPECT_CALL(*mockFactory, createVertexBuffer()).WillOnce(Return(nullptr)).RetiresOnSaturation();
    mockBufferD3D->initializeStaticData();

    return mockBufferD3D;
}

IndexDataManagerPerfTest::IndexDataManagerPerfTest()
    : ANGLEPerfTest("IndexDataManger", "_run"),
      mIndexDataManager(&mMockFactory, rx::RENDERER_D3D11),
      mIndexCount(4000),
      mBufferSize(mIndexCount * 2),
      mMockFactory(mBufferSize, GL_UNSIGNED_SHORT),
      mIndexBuffer(InitMockBufferD3D(&mMockFactory, mBufferSize), 1),
      mIndexData(mIndexCount)
{
    for (GLsizei index = 0; index < mIndexCount; ++index)
    {
        mIndexData[index] = static_cast<GLshort>(index);
    }
}

void IndexDataManagerPerfTest::step(float dt, double totalTime)
{
    rx::TranslatedIndexData translatedIndexData;

    for (unsigned int iteration = 0; iteration < 100; ++iteration)
    {
        if (!mIndexBuffer.getIndexRangeCache()->findRange(GL_UNSIGNED_SHORT, 0, mIndexCount, &translatedIndexData.indexRange))
        {
            translatedIndexData.indexRange = rx::IndexRangeCache::ComputeRange(GL_UNSIGNED_SHORT, &mIndexData[0], mIndexCount);
            mIndexBuffer.getIndexRangeCache()->addRange(GL_UNSIGNED_SHORT, 0, mIndexCount, translatedIndexData.indexRange);
        }

        mIndexDataManager.prepareIndexData(GL_UNSIGNED_SHORT, mIndexCount, &mIndexBuffer, nullptr, &translatedIndexData);
    }

    if (mTimer->getElapsedTime() >= 5.0)
    {
        mRunning = false;
    }
}

TEST_F(IndexDataManagerPerfTest, Run)
{
    run();
}

}

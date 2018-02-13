//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// AttributeLayoutTest:
//   Test various layouts of vertex attribute data:
//   - in memory, in buffer object, or combination of both
//   - float, integer, or combination of both
//   - sequential or interleaved

#include <array>
#include <vector>

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

using namespace angle;

namespace
{

// Test will draw these four triangles.
// clang-format off
const GLfloat triangleData[] = {
    // xy       rgb
    0,0,        1,1,0,
    -1,+1,      1,1,0,
    +1,+1,      1,1,0,

    0,0,        0,1,0,
    +1,+1,      0,1,0,
    +1,-1,      0,1,0,

    0,0,        0,1,1,
    +1,-1,      0,1,1,
    -1,-1,      0,1,1,

    0,0,        1,0,1,
    -1,-1,      1,0,1,
    -1,+1,      1,0,1,
};
// clang-format on

constexpr size_t kNumVertices = ArraySize(triangleData) / 5;

// A container for one or more vertex attributes.
class Container
{
  public:
    static constexpr size_t kSize = 1024;

    void open(void) { memset(mMemory, 0xff, kSize); }

    void fill(size_t numItem, size_t itemSize, const char *src, unsigned offset, unsigned stride)
    {
        while (numItem--)
        {
            ASSERT(offset + itemSize <= kSize);
            memcpy(mMemory + offset, src, itemSize);
            src += itemSize;
            offset += stride;
        }
    }

    virtual void close(void) {}
    virtual ~Container() {}
    virtual const char *getAddress() = 0;
    virtual GLuint getBuffer()       = 0;

  protected:
    char mMemory[kSize];
};

// Vertex attribute data in client memory.
class Memory : public Container
{
  public:
    const char *getAddress() override { return mMemory; }
    GLuint getBuffer() override { return 0; }
};

// Vertex attribute data in buffer object.
class Buffer : public Container
{
  public:
    void close(void) override
    {
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(mMemory), mMemory, GL_STATIC_DRAW);
    }

    const char *getAddress() override { return nullptr; }
    GLuint getBuffer() override { return mBuffer; }

  protected:
    GLBuffer mBuffer;
};

// clang-format off
template<class Type> struct GLType {};
template<> struct GLType<GLbyte  > { static constexpr GLenum kGLType = GL_BYTE; };
template<> struct GLType<GLubyte > { static constexpr GLenum kGLType = GL_UNSIGNED_BYTE; };
template<> struct GLType<GLshort > { static constexpr GLenum kGLType = GL_SHORT; };
template<> struct GLType<GLushort> { static constexpr GLenum kGLType = GL_UNSIGNED_SHORT; };
template<> struct GLType<GLfloat > { static constexpr GLenum kGLType = GL_FLOAT; };
// clang-format on

// Encapsulates the data for one vertex attribute, where it lives, and how it is layed out.
class Attrib
{
  public:
    template <class T>
    Attrib(std::shared_ptr<Container> container, unsigned offset, unsigned stride, const T &data)
        : mContainer(container),
          mOffset(offset),
          mStride(stride),
          mData(reinterpret_cast<const char *>(data.data())),
          mDimension(data.size() / kNumVertices),
          mAttribSize(mDimension * sizeof(typename T::value_type)),
          mGLType(GLType<typename T::value_type>::kGLType)
    {
        // Compiler complains about unused variable without these.
        (void)GLType<GLbyte>::kGLType;
        (void)GLType<GLubyte>::kGLType;
        (void)GLType<GLshort>::kGLType;
        (void)GLType<GLushort>::kGLType;
        (void)GLType<GLfloat>::kGLType;
    }

    void openContainer(void) const { mContainer->open(); }
    void fillContainer(void) const
    {
        mContainer->fill(kNumVertices, mAttribSize, mData, mOffset, mStride);
    }
    void closeContainer(void) const { mContainer->close(); }

    void enable(unsigned index) const
    {
        glBindBuffer(GL_ARRAY_BUFFER, mContainer->getBuffer());
        glVertexAttribPointer(index, static_cast<int>(mDimension), mGLType, GL_FALSE, mStride,
                              mContainer->getAddress() + mOffset);
        glEnableVertexAttribArray(index);
    }

    bool inClientMemory(void) const { return mContainer->getAddress() != nullptr; }

  protected:
    std::shared_ptr<Container> mContainer;
    unsigned mOffset;
    unsigned mStride;
    const char *mData;
    size_t mDimension;
    size_t mAttribSize;
    GLenum mGLType;
};

typedef std::vector<Attrib> TestCase;

void PrepareTestCase(const TestCase &tc)
{
    for (const Attrib &a : tc)
    {
        a.openContainer();
    }
    for (const Attrib &a : tc)
    {
        a.fillContainer();
    }
    for (const Attrib &a : tc)
    {
        a.closeContainer();
    }
    unsigned i = 0;
    for (const Attrib &a : tc)
    {
        a.enable(i++);
    }
}

template <class Type, size_t Dimension>
using VertexData = std::array<Type, Dimension * kNumVertices>;

class AttributeLayoutTest : public ANGLETest
{
  protected:
    AttributeLayoutTest() : mProgram(0)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    void PrepareVertexData(void);
    void GetTestCases(void);

    void SetUp() override
    {
        ANGLETest::SetUp();

        glClearColor(.2f, .2f, .2f, .0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDisable(GL_DEPTH_TEST);

        const std::string vertexSource =
            "attribute mediump vec2 coord;\n"
            "attribute mediump vec3 color;\n"
            "varying mediump vec3 vcolor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_Position = vec4(coord, 0, 1);\n"
            "    vcolor = color;\n"
            "}\n";

        const std::string fragmentSource =
            "varying mediump vec3 vcolor;\n"
            "void main(void)\n"
            "{\n"
            "    gl_FragColor = vec4(vcolor, 0);\n"
            "}\n";

        mProgram = CompileProgram(vertexSource, fragmentSource);
        ASSERT_NE(0u, mProgram);
        glUseProgram(mProgram);

        glGenBuffers(1, &mIndexBuffer);

        PrepareVertexData();
        GetTestCases();
    }

    void TearDown() override
    {
        mTestCases.clear();
        glDeleteProgram(mProgram);
        glDeleteBuffers(1, &mIndexBuffer);
        ANGLETest::TearDown();
    }

    virtual bool Skip(const TestCase &) { return false; }
    virtual void Draw(int firstVertex, unsigned vertexCount, const GLushort *indices) = 0;

    void Run(bool drawFirstTriangle)
    {
        glViewport(0, 0, getWindowWidth(), getWindowHeight());
        glUseProgram(mProgram);

        for (unsigned i = 0; i < mTestCases.size(); ++i)
        {
            if (Skip(mTestCases[i]))
                continue;

            PrepareTestCase(mTestCases[i]);

            glClear(GL_COLOR_BUFFER_BIT);

            std::string testCase;
            if (drawFirstTriangle)
            {
                Draw(0, kNumVertices, mIndices);
                testCase = "draw";
            }
            else
            {
                Draw(3, kNumVertices - 3, mIndices + 3);
                testCase = "skip";
            }

            testCase += " first triangle case ";
            int w = getWindowWidth() / 4;
            int h = getWindowHeight() / 4;
            if (drawFirstTriangle)
            {
                EXPECT_PIXEL_EQ(w * 2, h * 3, 255, 255, 0, 0) << testCase << i;
            }
            else
            {
                EXPECT_PIXEL_EQ(w * 2, h * 3, 51, 51, 51, 0) << testCase << i;
            }
            EXPECT_PIXEL_EQ(w * 3, h * 2, 0, 255, 0, 0) << testCase << i;
            EXPECT_PIXEL_EQ(w * 2, h * 1, 0, 255, 255, 0) << testCase << i;
            EXPECT_PIXEL_EQ(w * 1, h * 2, 255, 0, 255, 0) << testCase << i;
        }
    }

    static const GLushort mIndices[kNumVertices];

    GLuint mProgram;
    GLuint mIndexBuffer;

    std::vector<TestCase> mTestCases;

    VertexData<GLfloat, 2> mCoord;
    VertexData<GLfloat, 3> mColor;
    VertexData<GLbyte, 3> mBColor;
};
const GLushort AttributeLayoutTest::mIndices[kNumVertices] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

void AttributeLayoutTest::PrepareVertexData(void)
{
    mCoord.fill(0);
    mColor.fill(0);
    mBColor.fill(0);

    for (unsigned i = 0; i < kNumVertices; ++i)
    {
        GLfloat x = triangleData[i * 5 + 0];
        GLfloat y = triangleData[i * 5 + 1];
        GLfloat r = triangleData[i * 5 + 2];
        GLfloat g = triangleData[i * 5 + 3];
        GLfloat b = triangleData[i * 5 + 4];

        mCoord[i * 2 + 0] = x;
        mCoord[i * 2 + 1] = y;

        mColor[i * 3 + 0] = r;
        mColor[i * 3 + 1] = g;
        mColor[i * 3 + 2] = b;

        mBColor[i * 3 + 0] = r;
        mBColor[i * 3 + 1] = g;
        mBColor[i * 3 + 2] = b;
    }
}

void AttributeLayoutTest::GetTestCases(void)
{
    std::shared_ptr<Container> M0 = std::make_shared<Memory>();
    std::shared_ptr<Container> M1 = std::make_shared<Memory>();
    std::shared_ptr<Container> B0 = std::make_shared<Buffer>();
    std::shared_ptr<Container> B1 = std::make_shared<Buffer>();

    // 0. two buffers
    mTestCases.push_back({Attrib(B0, 0, 8, mCoord), Attrib(B1, 0, 12, mColor)});

    // 1. two memory
    mTestCases.push_back({Attrib(M0, 0, 8, mCoord), Attrib(M1, 0, 12, mColor)});

    // 2. one memory, sequential
    mTestCases.push_back({Attrib(M0, 0, 8, mCoord), Attrib(M0, 96, 12, mColor)});

    // 3. one memory, interleaved
    mTestCases.push_back({Attrib(M0, 0, 20, mCoord), Attrib(M0, 8, 20, mColor)});

    // 4. buffer and memory
    mTestCases.push_back({Attrib(B0, 0, 8, mCoord), Attrib(M0, 0, 12, mColor)});

    // 5. stride != size
    mTestCases.push_back({Attrib(B0, 0, 16, mCoord), Attrib(B1, 0, 12, mColor)});

    if (IsVulkan())
    {
        std::cout << "cases skipped on Vulkan: integer data, non-zero buffer offsets" << std::endl;
        return;
    }

    // 6. one buffer, sequential
    mTestCases.push_back({Attrib(B0, 0, 8, mCoord), Attrib(B0, 96, 12, mColor)});

    // 7. one buffer, interleaved
    mTestCases.push_back({Attrib(B0, 0, 20, mCoord), Attrib(B0, 8, 20, mColor)});

    // 8. memory and buffer, float and integer
    mTestCases.push_back({Attrib(M0, 0, 8, mCoord), Attrib(B0, 0, 12, mBColor)});

    // 9. buffer and memory, unusual offset and stride
    mTestCases.push_back({Attrib(B0, 11, 13, mCoord), Attrib(M0, 23, 17, mColor)});
}

class AttributeLayoutNonIndexed : public AttributeLayoutTest
{
    void Draw(int firstVertex, unsigned vertexCount, const GLushort *indices) override
    {
        glDrawArrays(GL_TRIANGLES, firstVertex, vertexCount);
    }
};

class AttributeLayoutMemoryIndexed : public AttributeLayoutTest
{
    void Draw(int firstVertex, unsigned vertexCount, const GLushort *indices) override
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, indices);
    }
};

class AttributeLayoutBufferIndexed : public AttributeLayoutTest
{
    void Draw(int firstVertex, unsigned vertexCount, const GLushort *indices) override
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIndexBuffer);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(*mIndices) * vertexCount, indices,
                     GL_STATIC_DRAW);
        glDrawElements(GL_TRIANGLES, vertexCount, GL_UNSIGNED_SHORT, nullptr);
    }
};

TEST_P(AttributeLayoutNonIndexed, Test)
{
    Run(true);

    if (IsWindows() && IsAMD() && IsOpenGL())
    {
        std::cout << "test skipped on Windows ATI OpenGL: non-indexed non-zero vertex start"
                  << std::endl;
        return;
    }

    Run(false);
}

TEST_P(AttributeLayoutMemoryIndexed, Test)
{
    Run(true);

    if (IsWindows() && IsAMD() && (IsOpenGL() || GetParam() == ES2_D3D11_FL9_3()))
    {
        std::cout << "test skipped on Windows ATI OpenGL and D3D11_9_3: indexed non-zero vertex start"
                  << std::endl;
        return;
    }

    Run(false);
}

TEST_P(AttributeLayoutBufferIndexed, Test)
{
    Run(true);

    if (IsWindows() && IsAMD() && (IsOpenGL() || GetParam() == ES2_D3D11_FL9_3()))
    {
        std::cout << "test skipped on Windows ATI OpenGL and D3D11_9_3: indexed non-zero vertex start"
                  << std::endl;
        return;
    }

    Run(false);
}

#define PARAMS                                                                            \
    ES2_VULKAN(), ES2_OPENGL(), ES2_D3D9(), ES2_D3D11(), ES2_D3D11_FL9_3(), ES3_OPENGL(), \
        ES2_OPENGLES(), ES3_OPENGLES()

ANGLE_INSTANTIATE_TEST(AttributeLayoutNonIndexed, PARAMS);
ANGLE_INSTANTIATE_TEST(AttributeLayoutMemoryIndexed, PARAMS);
ANGLE_INSTANTIATE_TEST(AttributeLayoutBufferIndexed, PARAMS);

}  // anonymous namespace

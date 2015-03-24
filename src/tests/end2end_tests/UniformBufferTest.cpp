#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
ANGLE_TYPED_TEST_CASE(UniformBufferTest, ES3_D3D11_FL11_1, ES3_D3D11_FL11_1_REFERENCE);

template<typename T>
class UniformBufferTest : public ANGLETest
{
  protected:
    UniformBufferTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetPlatform())
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource = SHADER_SOURCE
        (   #version 300 es\n
            in vec4 position;
            void main()
            {
                gl_Position = position;
            }
        );
        const std::string fragmentShaderSource = SHADER_SOURCE
        (   #version 300 es\n
            precision highp float;
            uniform uni {
                vec4 color;
            };

            out vec4 fragColor;

            void main()
            {
                fragColor = color;
            }
        );

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        ASSERT_NE(mProgram, 0u);

        mUniformBufferIndex = glGetUniformBlockIndex(mProgram, "uni");
        ASSERT_NE(mUniformBufferIndex, -1);

        glGenBuffers(1, &mUniformBuffer);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteBuffers(1, &mUniformBuffer);
        glDeleteProgram(mProgram);
        ANGLETest::TearDown();
    }

    GLuint mProgram;
    GLint mUniformBufferIndex;
    GLuint mUniformBuffer;
};

// Test that using a UBO with a non-zero offset and size actually works.
// The first step of this test renders a color from a UBO with a zero offset.
// The second step renders a color from a UBO with a non-zero offset.
TYPED_TEST(UniformBufferTest, UniformBufferRange)
{
    int px = getWindowWidth() / 2;
    int py = getWindowHeight() / 2;

    // Query the uniform buffer alignment requirement
    GLint alignment;
    glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);

    GLint64 maxUniformBlockSize;
    glGetInteger64v(GL_MAX_UNIFORM_BLOCK_SIZE, &maxUniformBlockSize);
    if (alignment >= maxUniformBlockSize)
    {
        // ANGLE doesn't implement UBO offsets for this platform.
        // Ignore the test case.
        return;
    }

    ASSERT_GL_NO_ERROR();

    // Let's create a buffer which contains two vec4.
    GLuint vec4Size = 4 * sizeof(float);
    GLuint stride = 0;
    do
    {
        stride += alignment;
    }
    while (stride < vec4Size);

    std::vector<char> v(2 * stride);
    float *first = reinterpret_cast<float*>(v.data());
    float *second = reinterpret_cast<float*>(v.data() + stride);

    first[0] = 10.f / 255.f;
    first[1] = 20.f / 255.f;
    first[2] = 30.f / 255.f;
    first[3] = 40.f / 255.f;

    second[0] = 110.f / 255.f;
    second[1] = 120.f / 255.f;
    second[2] = 130.f / 255.f;
    second[3] = 140.f / 255.f;

    glBindBuffer(GL_UNIFORM_BUFFER, mUniformBuffer);
    // We use on purpose a size which is not a multiple of the alignment.
    glBufferData(GL_UNIFORM_BUFFER, stride + vec4Size, v.data(), GL_STATIC_DRAW);

    glUniformBlockBinding(mProgram, mUniformBufferIndex, 0);

    EXPECT_GL_NO_ERROR();

    // Bind the first part of the uniform buffer and draw
    // Use a size which is smaller than the alignment to check
    // to check that this case is handle correctly in the conversion to 11.1.
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUniformBuffer, 0, vec4Size);
    drawQuad(mProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(px, py, 10, 20, 30, 40);

    // Bind the second part of the uniform buffer and draw
    // Furthermore the D3D11.1 backend will internally round the vec4Size (16 bytes) to a stride (256 bytes)
    // hence it will try to map the range [stride, 2 * stride] which is
    // out-of-bound of the buffer bufferSize = stride + vec4Size < 2 * stride.
    // Ensure that this behaviour works.
    glBindBufferRange(GL_UNIFORM_BUFFER, 0, mUniformBuffer, stride, vec4Size);
    drawQuad(mProgram, "position", 0.5f);
    EXPECT_GL_NO_ERROR();
    EXPECT_PIXEL_EQ(px, py, 110, 120, 130, 140);
}

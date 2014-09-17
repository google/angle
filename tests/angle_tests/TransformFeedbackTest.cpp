#include "ANGLETest.h"

class TransformFeedbackTest : public ANGLETest
{
  protected:
      TransformFeedbackTest()
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setClientVersion(3);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        const std::string vertexShaderSource = SHADER_SOURCE
        (
            precision highp float;
            attribute vec4 position;

            void main()
            {
                gl_Position = position;
            }
        );

        const std::string fragmentShaderSource = SHADER_SOURCE
        (
            precision highp float;

            void main()
            {
                gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);
            }
        );

        mProgram = CompileProgram(vertexShaderSource, fragmentShaderSource);
        if (mProgram == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glGenBuffers(1, &mTransformFeedbackBuffer);
        mTransformFeedbackBufferSize = 1 << 24; // ~16MB
        glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, mTransformFeedbackBuffer);
        glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER, mTransformFeedbackBufferSize, NULL, GL_STATIC_DRAW);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteProgram(mProgram);
        glDeleteBuffers(1, &mTransformFeedbackBuffer);
        ANGLETest::TearDown();
    }

    GLuint mProgram;

    size_t mTransformFeedbackBufferSize;
    GLuint mTransformFeedbackBuffer;
};

TEST_F(TransformFeedbackTest, ZeroSizedViewport)
{
    // Set the program's transform feedback varyings (just gl_Position)
    const GLchar* transformFeedbackVaryings[] =
    {
        "gl_Position"
    };
    glTransformFeedbackVaryings(mProgram, ArraySize(transformFeedbackVaryings), transformFeedbackVaryings, GL_INTERLEAVED_ATTRIBS);
    glLinkProgram(mProgram);

    // Re-link the program
    GLint linkStatus;
    glGetProgramiv(mProgram, GL_LINK_STATUS, &linkStatus);
    ASSERT_NE(linkStatus, 0);

    glUseProgram(mProgram);

    // Bind the buffer for transform feedback output and start transform feedback
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, mTransformFeedbackBuffer);
    glBeginTransformFeedback(GL_TRIANGLES);

    // Create a query to check how many primitives were written
    GLuint primitivesWrittenQuery = 0;
    glGenQueries(1, &primitivesWrittenQuery);
    glBeginQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN, primitivesWrittenQuery);

    // Set a viewport that would result in no pixels being written to the framebuffer and draw
    // a quad
    glViewport(0, 0, 0, 0);

    drawQuad(mProgram, "position", 0.5f);

    // End the query and transform feedkback
    glEndQuery(GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN);
    glEndTransformFeedback();

    // Check how many primitives were written and verify that some were written even if
    // no pixels were rendered
    GLuint primitivesWritten = 0;
    glGetQueryObjectuiv(primitivesWrittenQuery, GL_QUERY_RESULT_EXT, &primitivesWritten);
    EXPECT_GL_NO_ERROR();

    EXPECT_EQ(primitivesWritten, 2);
}

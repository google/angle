#include "ANGLETest.h"

class DrawBuffersTest : public ANGLETest
{
  protected:
    DrawBuffersTest(int clientVersion)
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
        setClientVersion(clientVersion);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();

        glGenFramebuffers(1, &mFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, mFBO);

        glGenTextures(4, mTextures);

        for (size_t texIndex = 0; texIndex < ArraySize(mTextures); texIndex++)
        {
            glBindTexture(GL_TEXTURE_2D, mTextures[texIndex]);
            glTexStorage2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, getWindowWidth(), getWindowHeight());
        }

        GLfloat data[] =
        {
            -1.0f, 1.0f,
            -1.0f, -2.0f,
            2.0f, 1.0f
        };

        glGenBuffers(1, &mBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6, data, GL_STATIC_DRAW);

        GLint maxDrawBuffers;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &maxDrawBuffers);
        ASSERT_EQ(maxDrawBuffers, 8);

        ASSERT_GL_NO_ERROR();
    }

    virtual void TearDown()
    {
        glDeleteFramebuffers(1, &mFBO);
        glDeleteTextures(4, mTextures);
        glDeleteBuffers(1, &mBuffer);

        ANGLETest::TearDown();
    }

    void setupMRTProgramESSL3(bool bufferEnabled[8], GLuint *programOut)
    {
        const std::string vertexShaderSource =
            "#version 300 es\n"
            "in vec4 position;\n"
            "void main() {\n"
            "    gl_Position = position;\n"
            "}\n";

        std::stringstream strstr;

        strstr << "#version 300 es\n"
                  "precision highp float;\n";

        for (unsigned int index = 0; index < 8; index++)
        {
            if (bufferEnabled[index])
            {
                strstr << "layout(location = " << index << ") "
                          "out vec4 value" << index << ";\n";
            }
        }

        strstr << "void main()\n"
                  "{\n";

        for (unsigned int index = 0; index < 8; index++)
        {
            if (bufferEnabled[index])
            {
                unsigned int r = (index + 1) & 1;
                unsigned int g = (index + 1) & 2;
                unsigned int b = (index + 1) & 4;

                strstr << "    value" << index << " = vec4("
                       << r << ".0, " << g << ".0, "
                       << b << ".0, 1.0);\n";
            }
        }

        strstr << "}\n";

        *programOut = CompileProgram(vertexShaderSource, strstr.str());
        if (*programOut == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glUseProgram(*programOut);

        GLint location = glGetAttribLocation(*programOut, "position");
        ASSERT_NE(location, -1);
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 8, NULL);
        glEnableVertexAttribArray(location);
    }

    void setupMRTProgramESSL1(bool bufferEnabled[8], GLuint *programOut)
    {
        const std::string vertexShaderSource =
            "attribute vec4 position;\n"
            "void main() {\n"
            "    gl_Position = position;\n"
            "}\n";

        std::stringstream strstr;

        strstr << "#extension GL_EXT_draw_buffers : enable\n"
                  "precision highp float;\n"
                  "void main()\n"
                  "{\n";

        for (unsigned int index = 0; index < 8; index++)
        {
            if (bufferEnabled[index])
            {
                unsigned int r = (index + 1) & 1;
                unsigned int g = (index + 1) & 2;
                unsigned int b = (index + 1) & 4;

                strstr << "    gl_FragData[" << index << "] = vec4("
                    << r << ".0, " << g << ".0, "
                    << b << ".0, 1.0);\n";
            }
        }

        strstr << "}\n";

        *programOut = CompileProgram(vertexShaderSource, strstr.str());
        if (*programOut == 0)
        {
            FAIL() << "shader compilation failed.";
        }

        glUseProgram(*programOut);

        GLint location = glGetAttribLocation(*programOut, "position");
        ASSERT_NE(location, -1);
        glBindBuffer(GL_ARRAY_BUFFER, mBuffer);
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 8, NULL);
        glEnableVertexAttribArray(location);
    }

    void setupMRTProgram(bool bufferEnabled[8], GLuint *programOut)
    {
        if (getClientVersion() == 3)
        {
            setupMRTProgramESSL3(bufferEnabled, programOut);
        }
        else
        {
            ASSERT_EQ(getClientVersion(), 2);
            setupMRTProgramESSL1(bufferEnabled, programOut);
        }
    }

    void verifyAttachment(unsigned int index, GLuint textureName)
    {
        for (unsigned int colorAttachment = 0; colorAttachment < 8; colorAttachment++)
        {
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + colorAttachment, GL_TEXTURE_2D, 0, 0);
        }

        glBindTexture(GL_TEXTURE_2D, textureName);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureName, 0);

        unsigned int r = (((index + 1) & 1) > 0) ? 255 : 0;
        unsigned int g = (((index + 1) & 2) > 0) ? 255 : 0;
        unsigned int b = (((index + 1) & 4) > 0) ? 255 : 0;

        EXPECT_PIXEL_EQ(getWindowWidth() / 2, getWindowHeight() / 2, r, g, b, 255);
    }

    void gapsTest()
    {
        glBindTexture(GL_TEXTURE_2D, mTextures[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, mTextures[0], 0);

        bool flags[8] = { false, true };

        GLuint program;
        setupMRTProgram(flags, &program);

        const GLenum bufs[] =
        {
            GL_NONE,
            GL_COLOR_ATTACHMENT1
        };
        glUseProgram(program);
        glDrawBuffersEXT(2, bufs);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        verifyAttachment(1, mTextures[0]);

        glDeleteProgram(program);
    }

    void firstAndLastTest()
    {
        glBindTexture(GL_TEXTURE_2D, mTextures[0]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mTextures[0], 0);

        glBindTexture(GL_TEXTURE_2D, mTextures[1]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, mTextures[1], 0);

        bool flags[8] = { true, false, false, true };

        GLuint program;
        setupMRTProgram(flags, &program);

        const GLenum bufs[] =
        {
            GL_COLOR_ATTACHMENT0,
            GL_NONE,
            GL_NONE,
            GL_COLOR_ATTACHMENT3
        };

        glUseProgram(program);
        glDrawBuffersEXT(4, bufs);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        verifyAttachment(0, mTextures[0]);
        verifyAttachment(3, mTextures[1]);

        EXPECT_GL_NO_ERROR();

        glDeleteProgram(program);
    }

    void firstHalfNULLTest()
    {
        bool flags[8] = { false };
        GLenum bufs[8] = { GL_NONE };

        for (unsigned int texIndex = 0; texIndex < 4; texIndex++)
        {
            glBindTexture(GL_TEXTURE_2D, mTextures[texIndex]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4 + texIndex, GL_TEXTURE_2D, mTextures[texIndex], 0);
            flags[texIndex + 4] = true;
            bufs[texIndex + 4] = GL_COLOR_ATTACHMENT4 + texIndex;
        }

        GLuint program;
        setupMRTProgram(flags, &program);

        glUseProgram(program);
        glDrawBuffersEXT(8, bufs);
        glDrawArrays(GL_TRIANGLES, 0, 3);

        for (unsigned int texIndex = 0; texIndex < 4; texIndex++)
        {
            verifyAttachment(texIndex + 4, mTextures[texIndex]);
        }

        EXPECT_GL_NO_ERROR();

        glDeleteProgram(program);
    }

    GLuint mFBO;
    GLuint mTextures[4];
    GLuint mBuffer;
};

class DrawBuffersTestESSL3 : public DrawBuffersTest
{
  protected:
    DrawBuffersTestESSL3()
        : DrawBuffersTest(3)
    {}
};

class DrawBuffersTestESSL1 : public DrawBuffersTest
{
  protected:
    DrawBuffersTestESSL1()
        : DrawBuffersTest(2)
    {}
};

TEST_F(DrawBuffersTestESSL3, Gaps)
{
    gapsTest();
}

TEST_F(DrawBuffersTestESSL1, Gaps)
{
    gapsTest();
}

TEST_F(DrawBuffersTestESSL3, FirstAndLast)
{
    firstAndLastTest();
}

TEST_F(DrawBuffersTestESSL1, FirstAndLast)
{
    firstAndLastTest();
}

TEST_F(DrawBuffersTestESSL3, FirstHalfNULL)
{
    firstHalfNULLTest();
}

TEST_F(DrawBuffersTestESSL1, FirstHalfNULL)
{
    firstHalfNULLTest();
}

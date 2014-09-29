#include "ANGLETest.h"

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
typedef ::testing::Types<TFT<Gles::Two, Rend::D3D11>, TFT<Gles::Two, Rend::D3D9>> TestFixtureTypes;
TYPED_TEST_CASE(DepthStencilFormatsTest, TestFixtureTypes);

template<typename T>
class DepthStencilFormatsTest : public ANGLETest
{
protected:
    DepthStencilFormatsTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetRequestedRenderer())
    {
        setWindowWidth(128);
        setWindowHeight(128);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }

    bool checkTexImageFormatSupport(GLenum format, GLenum type)
    {
        EXPECT_GL_NO_ERROR();

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, format, 1, 1, 0, format, type, NULL);
        glDeleteTextures(1, &tex);

        return (glGetError() == GL_NO_ERROR);
    }

    bool checkTexStorageFormatSupport(GLenum internalFormat)
    {
        EXPECT_GL_NO_ERROR();

        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexStorage2DEXT(GL_TEXTURE_2D, 1, internalFormat, 1, 1);
        glDeleteTextures(1, &tex);

        return (glGetError() == GL_NO_ERROR);
    }

    bool checkRenderbufferFormatSupport(GLenum internalFormat)
    {
        EXPECT_GL_NO_ERROR();

        GLuint rb = 0;
        glGenRenderbuffers(1, &rb);
        glBindRenderbuffer(GL_RENDERBUFFER, rb);
        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, 1, 1);
        glDeleteRenderbuffers(1, &rb);

        return (glGetError() == GL_NO_ERROR);
    }

    virtual void SetUp()
    {
        ANGLETest::SetUp();
    }

    virtual void TearDown()
    {
        ANGLETest::TearDown();
    }
};

TYPED_TEST(DepthStencilFormatsTest, DepthTexture)
{
    bool shouldHaveTextureSupport = extensionEnabled("GL_ANGLE_depth_texture");
    EXPECT_EQ(shouldHaveTextureSupport, checkTexImageFormatSupport(GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT));
    EXPECT_EQ(shouldHaveTextureSupport, checkTexImageFormatSupport(GL_DEPTH_COMPONENT, GL_UNSIGNED_INT));

    if (extensionEnabled("GL_EXT_texture_storage"))
    {
        EXPECT_EQ(shouldHaveTextureSupport, checkTexStorageFormatSupport(GL_DEPTH_COMPONENT16));
        EXPECT_EQ(shouldHaveTextureSupport, checkTexStorageFormatSupport(GL_DEPTH_COMPONENT32_OES));
    }
}

TYPED_TEST(DepthStencilFormatsTest, PackedDepthStencil)
{
    // Expected to fail in D3D9 if GL_OES_packed_depth_stencil is not present.
    // Expected to fail in D3D11 if GL_OES_packed_depth_stencil or GL_ANGLE_depth_texture is not present.

    bool shouldHaveRenderbufferSupport = extensionEnabled("GL_OES_packed_depth_stencil");
    EXPECT_EQ(shouldHaveRenderbufferSupport, checkRenderbufferFormatSupport(GL_DEPTH24_STENCIL8_OES));

    bool shouldHaveTextureSupport = extensionEnabled("GL_OES_packed_depth_stencil") &&
                                    extensionEnabled("GL_ANGLE_depth_texture");
    EXPECT_EQ(shouldHaveTextureSupport, checkTexImageFormatSupport(GL_DEPTH_STENCIL_OES, GL_UNSIGNED_INT_24_8_OES));

    if (extensionEnabled("GL_EXT_texture_storage"))
    {
        EXPECT_EQ(shouldHaveTextureSupport, checkTexStorageFormatSupport(GL_DEPTH24_STENCIL8_OES));
    }
}

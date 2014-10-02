#include "ANGLETest.h"

// These tests are designed to ensure that the various configurations of the test fixtures work as expected.
// If one of these tests fails, then it is likely that some of the other tests are being configured incorrectly.
// For example, they might be using the D3D11 renderer when the test is meant to be using the D3D9 renderer.

// Use this to select which configurations (e.g. which renderer, which GLES major version) these tests should be run against.
typedef ::testing::Types<TFT<Gles::Three, Rend::D3D11>, TFT<Gles::Two, Rend::D3D11>,
                         TFT<Gles::Three, Rend::WARP>,  TFT<Gles::Two, Rend::WARP>,
                                                        TFT<Gles::Two, Rend::D3D9>  > TestFixtureTypes;
TYPED_TEST_CASE(RendererTest, TestFixtureTypes);

template<typename T>
class RendererTest : public ANGLETest
{
  protected:
    RendererTest() : ANGLETest(T::GetGlesMajorVersion(), T::GetRequestedRenderer())
    {
        setWindowWidth(128);
        setWindowHeight(128);
    }

    T fixtureType;
};

TYPED_TEST(RendererTest, RequestedRendererCreated)
{
    std::string rendererString = std::string(reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    std::transform(rendererString.begin(), rendererString.end(), rendererString.begin(), ::tolower);

    std::string versionString = std::string(reinterpret_cast<const char*>(glGetString(GL_VERSION)));
    std::transform(versionString.begin(), versionString.end(), versionString.begin(), ::tolower);

    // Ensure that the renderer string contains D3D11, if we requested a D3D11 renderer.
    if (fixtureType.GetRequestedRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE || fixtureType.GetRequestedRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_WARP_ANGLE)
    {
        ASSERT_NE(rendererString.find(std::string("direct3d11")), std::string::npos);
    }

    // Ensure that the renderer string contains D3D9, if we requested a D3D9 renderer.
    if (fixtureType.GetRequestedRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE)
    {
        ASSERT_NE(rendererString.find(std::string("direct3d9")), std::string::npos);
    }

    // Ensure that the renderer uses WARP, if we requested it.
    if (fixtureType.GetRequestedRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_WARP_ANGLE)
    {
        auto basicRenderPos = rendererString.find(std::string("microsoft basic render"));
        auto softwareAdapterPos = rendererString.find(std::string("software adapter"));
        ASSERT_TRUE(basicRenderPos != std::string::npos || softwareAdapterPos != std::string::npos);
    }

    // Ensure that the renderer string contains GL ES 3.0, if we requested a GL ES 3.0
    if (fixtureType.GetGlesMajorVersion() == 3)
    {
        ASSERT_NE(versionString.find(std::string("es 3.0")), std::string::npos);
    }

    // Ensure that the version string contains GL ES 2.0, if we requested GL ES 2.0
    if (fixtureType.GetGlesMajorVersion() == 2)
    {
        ASSERT_NE(versionString.find(std::string("es 2.0")), std::string::npos);
    }
}

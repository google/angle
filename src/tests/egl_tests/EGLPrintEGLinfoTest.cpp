//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLPrintEGLinfoTest.cpp:
//   This test prints out the extension strings, configs and their attributes
//

#include <gtest/gtest.h>

#include "common/string_utils.h"
#include "test_utils/ANGLETest.h"

using namespace angle;

class EGLPrintEGLinfoTest : public ANGLETest
{
  protected:
    EGLPrintEGLinfoTest() {}

    void testSetUp() override
    {
        mDisplay = getEGLWindow()->getDisplay();
        ASSERT_TRUE(mDisplay != EGL_NO_DISPLAY);
    }

    EGLDisplay mDisplay = EGL_NO_DISPLAY;
};

// Parse space separated extension string into a vector of strings
std::vector<std::string> ParseExtensions(const char *extensions)
{
    std::string extensionsStr(extensions);
    std::vector<std::string> extensionsVec;
    SplitStringAlongWhitespace(extensionsStr, &extensionsVec);
    return extensionsVec;
}

// Query a EGL attribute
EGLint GetAttrib(EGLDisplay display, EGLConfig config, EGLint attrib)
{
    EGLint value = 0;
    EXPECT_EGL_TRUE(eglGetConfigAttrib(display, config, attrib, &value));
    return value;
}

// Query a egl string
const char *GetEGLString(EGLDisplay display, EGLint name)
{
    const char *value = "";
    value             = eglQueryString(display, name);
    EXPECT_TRUE(value != nullptr);
    return value;
}

// Query a GL string
const char *GetGLString(EGLint name)
{
    const char *value = "";
    value             = reinterpret_cast<const char *>(glGetString(name));
    EXPECT_TRUE(value != nullptr);
    return value;
}

// Print the EGL strings and extensions
TEST_P(EGLPrintEGLinfoTest, PrintEGLInfo)
{
    std::cout << "    EGL Information:" << std::endl;
    std::cout << "\tVendor: " << GetEGLString(mDisplay, EGL_VENDOR) << std::endl;
    std::cout << "\tVersion: " << GetEGLString(mDisplay, EGL_VENDOR) << std::endl;
    std::cout << "\tClient APIs: " << GetEGLString(mDisplay, EGL_CLIENT_APIS) << std::endl;

    std::cout << "\tEGL Client Extensions:" << std::endl;
    for (auto extension : ParseExtensions(GetEGLString(EGL_NO_DISPLAY, EGL_EXTENSIONS)))
    {
        std::cout << "\t\t" << extension << std::endl;
    }

    std::cout << "\tEGL Display Extensions:" << std::endl;
    for (auto extension : ParseExtensions(GetEGLString(mDisplay, EGL_EXTENSIONS)))
    {
        std::cout << "\t\t" << extension << std::endl;
    }

    std::cout << std::endl;
}

// Print the GL strings and extensions
TEST_P(EGLPrintEGLinfoTest, PrintGLInfo)
{
    std::cout << "    GLES Information:" << std::endl;
    std::cout << "\tVendor: " << GetGLString(GL_VENDOR) << std::endl;
    std::cout << "\tVersion: " << GetGLString(GL_VERSION) << std::endl;
    std::cout << "\tRenderer: " << GetGLString(GL_RENDERER) << std::endl;
    std::cout << "\tShader: " << GetGLString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    std::cout << "\tExtensions:" << std::endl;
    for (auto extension : ParseExtensions(GetGLString(GL_EXTENSIONS)))
    {
        std::cout << "\t\t" << extension << std::endl;
    }

    std::cout << std::endl;
}

// Print the EGL configs with attributes
TEST_P(EGLPrintEGLinfoTest, PrintConfigInfo)
{
    // Get all the configs
    EGLint count;
    EXPECT_EGL_TRUE(eglGetConfigs(mDisplay, nullptr, 0, &count));
    EXPECT_TRUE(count > 0);
    std::vector<EGLConfig> configs(count);
    EXPECT_EGL_TRUE(eglGetConfigs(mDisplay, configs.data(), count, &count));
    configs.resize(count);
    // sort configs by increaing ID
    std::sort(configs.begin(), configs.end(), [this](EGLConfig a, EGLConfig b) -> bool {
        return GetAttrib(mDisplay, a, EGL_CONFIG_ID) < GetAttrib(mDisplay, b, EGL_CONFIG_ID);
    });

    std::cout << "Configs - Count: " << count << std::endl;

    // For each config, print its attributes
    for (auto config : configs)
    {
        // Config ID
        std::cout << "    Config: " << GetAttrib(mDisplay, config, EGL_CONFIG_ID) << std::endl;

        // Color
        const char *componentType = (GetAttrib(mDisplay, config, EGL_COLOR_COMPONENT_TYPE_EXT) ==
                                     EGL_COLOR_COMPONENT_TYPE_FLOAT_EXT)
                                        ? "Float "
                                        : "Fixed ";
        const char *colorBuffType =
            (GetAttrib(mDisplay, config, EGL_COLOR_BUFFER_TYPE) == EGL_LUMINANCE_BUFFER)
                ? "LUMINANCE"
                : "RGB";
        std::cout << "\tColor:" << GetAttrib(mDisplay, config, EGL_BUFFER_SIZE) << "bit "
                  << componentType << colorBuffType
                  << " Red:" << GetAttrib(mDisplay, config, EGL_RED_SIZE)
                  << " Green:" << GetAttrib(mDisplay, config, EGL_GREEN_SIZE)
                  << " Blue:" << GetAttrib(mDisplay, config, EGL_BLUE_SIZE)
                  << " Alpha:" << GetAttrib(mDisplay, config, EGL_ALPHA_SIZE)
                  << " Lum:" << GetAttrib(mDisplay, config, EGL_LUMINANCE_SIZE)
                  << " AlphaMask:" << GetAttrib(mDisplay, config, EGL_ALPHA_MASK_SIZE) << std::endl;

        // Texture Binding
        std::cout << "\tBinding RGB:" << (bool)GetAttrib(mDisplay, config, EGL_BIND_TO_TEXTURE_RGB)
                  << " RGBA:" << (bool)GetAttrib(mDisplay, config, EGL_BIND_TO_TEXTURE_RGBA)
                  << " MaxWidth:" << GetAttrib(mDisplay, config, EGL_MAX_PBUFFER_WIDTH)
                  << " MaxHeight:" << GetAttrib(mDisplay, config, EGL_MAX_PBUFFER_HEIGHT)
                  << " MaxPixels:" << GetAttrib(mDisplay, config, EGL_MAX_PBUFFER_PIXELS)
                  << std::endl;

        // Conformant
        EGLint caveatAttrib = GetAttrib(mDisplay, config, EGL_CONFIG_CAVEAT);
        const char *caveat  = nullptr;
        switch (caveatAttrib)
        {
            case EGL_NONE:
                caveat = "None.";
                break;
            case EGL_SLOW_CONFIG:
                caveat = "Slow.";
                break;
            case EGL_NON_CONFORMANT_CONFIG:
                caveat = "Non-Conformant.";
                break;
            default:
                caveat = ".";
        }
        std::cout << "\tCaveate: " << caveat;

        EGLint conformant = GetAttrib(mDisplay, config, EGL_CONFORMANT);
        std::cout << " Conformant: ";
        if (conformant & EGL_OPENGL_BIT)
            std::cout << "OpenGL ";
        if (conformant & EGL_OPENGL_ES_BIT)
            std::cout << "ES1 ";
        if (conformant & EGL_OPENGL_ES2_BIT)
            std::cout << "ES2 ";
        if (conformant & EGL_OPENGL_ES3_BIT)
            std::cout << "ES3";
        std::cout << std::endl;

        // Ancilary buffers
        std::cout << "\tAncilary "
                  << "Depth:" << GetAttrib(mDisplay, config, EGL_DEPTH_SIZE)
                  << " Stencil:" << GetAttrib(mDisplay, config, EGL_STENCIL_SIZE)
                  << " SampleBuffs:" << GetAttrib(mDisplay, config, EGL_SAMPLE_BUFFERS)
                  << " Samples:" << GetAttrib(mDisplay, config, EGL_SAMPLES) << std::endl;

        // Swap interval
        std::cout << "\tSwap Interval"
                  << " Min:" << GetAttrib(mDisplay, config, EGL_MIN_SWAP_INTERVAL)
                  << " Max:" << GetAttrib(mDisplay, config, EGL_MAX_SWAP_INTERVAL) << std::endl;

        // Native
        std::cout << "\tNative Renderable: " << GetAttrib(mDisplay, config, EGL_NATIVE_RENDERABLE)
                  << ", VisualID: " << GetAttrib(mDisplay, config, EGL_NATIVE_VISUAL_ID)
                  << ", VisualType: " << GetAttrib(mDisplay, config, EGL_NATIVE_VISUAL_TYPE)
                  << std::endl;

        // Surface type
        EGLint surfaceType = GetAttrib(mDisplay, config, EGL_SURFACE_TYPE);
        std::cout << "\tSurface Type: ";
        if (surfaceType & EGL_WINDOW_BIT)
            std::cout << "WINDOW ";
        if (surfaceType & EGL_PIXMAP_BIT)
            std::cout << "PIXMAP ";
        if (surfaceType & EGL_PBUFFER_BIT)
            std::cout << "PBUFFER ";
        if (surfaceType & EGL_MULTISAMPLE_RESOLVE_BOX_BIT)
            std::cout << "MULTISAMPLE_RESOLVE_BOX ";
        if (surfaceType & EGL_SWAP_BEHAVIOR_PRESERVED_BIT)
            std::cout << "SWAP_PRESERVE ";
        std::cout << std::endl;

        // Renderable
        EGLint rendType = GetAttrib(mDisplay, config, EGL_RENDERABLE_TYPE);
        std::cout << "\tRender: ";
        if (rendType & EGL_OPENGL_BIT)
            std::cout << "OpenGL ";
        if (rendType & EGL_OPENGL_ES_BIT)
            std::cout << "ES1 ";
        if (rendType & EGL_OPENGL_ES2_BIT)
            std::cout << "ES2 ";
        if (rendType & EGL_OPENGL_ES3_BIT)
            std::cout << "ES3 ";
        std::cout << std::endl;

        // Extensions
        std::cout << "\tAndroid Recordable: " << GetAttrib(mDisplay, config, EGL_RECORDABLE_ANDROID)
                  << std::endl;

        // Separator between configs
        std::cout << std::endl;
    }
}

ANGLE_INSTANTIATE_TEST(EGLPrintEGLinfoTest, ES2_VULKAN(), ES3_VULKAN());

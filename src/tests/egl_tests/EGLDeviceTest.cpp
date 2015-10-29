//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef ANGLE_ENABLE_D3D9
#define ANGLE_ENABLE_D3D9
#endif

#ifndef ANGLE_ENABLE_D3D11
#define ANGLE_ENABLE_D3D11
#endif

#include "test_utils/ANGLETest.h"
#include "com_utils.h"

using namespace angle;

class EGLDeviceCreationTest : public testing::Test
{
  protected:
    EGLDeviceCreationTest()
        : mD3D11Available(false),
          mD3D11Module(nullptr),
          mD3D11CreateDevice(nullptr),
          mDevice(nullptr),
          mDeviceContext(nullptr),
          mDeviceCreationD3D11ExtAvailable(false)
    {
    }

    void SetUp() override
    {
        mD3D11Module = LoadLibrary(TEXT("d3d11.dll"));
        if (mD3D11Module == nullptr)
        {
            std::cout << "Unable to LoadLibrary D3D11" << std::endl;
            return;
        }

        mD3D11CreateDevice = reinterpret_cast<PFN_D3D11_CREATE_DEVICE>(
            GetProcAddress(mD3D11Module, "D3D11CreateDevice"));
        if (mD3D11CreateDevice == nullptr)
        {
            std::cout << "Could not retrieve D3D11CreateDevice from d3d11.dll" << std::endl;
            return;
        }

        mD3D11Available = true;

        const char *extensionString =
            static_cast<const char *>(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));
        if (strstr(extensionString, "EGL_ANGLE_device_creation"))
        {
            if (strstr(extensionString, "EGL_ANGLE_device_creation_d3d11"))
            {
                mDeviceCreationD3D11ExtAvailable = true;
            }
        }
    }

    void TearDown() override
    {
        SafeRelease(mDevice);
        SafeRelease(mDeviceContext);
    }

    bool CreateD3D11Device()
    {
        ASSERT(mD3D11Available);

        HRESULT hr =
            mD3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, 0, nullptr, 0,
                               D3D11_SDK_VERSION, &mDevice, &mFeatureLevel, &mDeviceContext);

        if (FAILED(hr) || mFeatureLevel < D3D_FEATURE_LEVEL_9_3)
        {
            std::cout << "Could not create D3D11 device, skipping test" << std::endl;
            return false;
        }

        return true;
    }

    bool mD3D11Available;
    HMODULE mD3D11Module;
    PFN_D3D11_CREATE_DEVICE mD3D11CreateDevice;

    ID3D11Device *mDevice;
    ID3D11DeviceContext *mDeviceContext;
    D3D_FEATURE_LEVEL mFeatureLevel;

    bool mDeviceCreationD3D11ExtAvailable;
};

// Test that creating a EGLDeviceEXT from D3D11 device works, and it can be queried to retrieve
// D3D11 device
TEST_F(EGLDeviceCreationTest, BasicD3D11Device)
{
    if (!mDeviceCreationD3D11ExtAvailable || !mD3D11Available || !CreateD3D11Device())
    {
        std::cout << "EGLDevice creation and/or D3D11 not available, skipping test" << std::endl;
        return;
    }

    EGLDeviceEXT eglDevice =
        eglCreateDeviceANGLE(EGL_D3D11_DEVICE_ANGLE, reinterpret_cast<void *>(mDevice), nullptr);
    ASSERT_NE(EGL_NO_DEVICE_EXT, eglDevice);
    ASSERT_EGL_SUCCESS();

    EGLAttrib deviceAttrib;
    eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE, &deviceAttrib);
    ASSERT_EGL_SUCCESS();

    ID3D11Device *queriedDevice = reinterpret_cast<ID3D11Device *>(deviceAttrib);
    ASSERT_EQ(mFeatureLevel, queriedDevice->GetFeatureLevel());

    eglReleaseDeviceANGLE(eglDevice);
}

// Test that creating a EGLDeviceEXT from D3D11 device works, and it can be queried to retrieve
// D3D11 device
TEST_F(EGLDeviceCreationTest, BasicD3D11DeviceViaFuncPointer)
{
    if (!mDeviceCreationD3D11ExtAvailable || !mD3D11Available || !CreateD3D11Device())
    {
        std::cout << "EGLDevice creation and/or D3D11 not available, skipping test" << std::endl;
        return;
    }

    PFNEGLCREATEDEVICEANGLEPROC createDeviceANGLE =
        (PFNEGLCREATEDEVICEANGLEPROC)eglGetProcAddress("eglCreateDeviceANGLE");
    PFNEGLRELEASEDEVICEANGLEPROC releaseDeviceANGLE =
        (PFNEGLRELEASEDEVICEANGLEPROC)eglGetProcAddress("eglReleaseDeviceANGLE");

    EGLDeviceEXT eglDevice =
        createDeviceANGLE(EGL_D3D11_DEVICE_ANGLE, reinterpret_cast<void *>(mDevice), nullptr);
    ASSERT_NE(EGL_NO_DEVICE_EXT, eglDevice);
    ASSERT_EGL_SUCCESS();

    EGLAttrib deviceAttrib;
    eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE, &deviceAttrib);
    ASSERT_EGL_SUCCESS();

    ID3D11Device *queriedDevice = reinterpret_cast<ID3D11Device *>(deviceAttrib);
    ASSERT_EQ(mFeatureLevel, queriedDevice->GetFeatureLevel());

    releaseDeviceANGLE(eglDevice);
}

// Test that creating a EGLDeviceEXT from an invalid D3D11 device fails
TEST_F(EGLDeviceCreationTest, InvalidD3D11Device)
{
    if (!mDeviceCreationD3D11ExtAvailable || !mD3D11Available || !CreateD3D11Device())
    {
        std::cout << "EGLDevice creation and/or D3D11 not available, skipping test" << std::endl;
        return;
    }

    // Use mDeviceContext instead of mDevice
    EGLDeviceEXT eglDevice = eglCreateDeviceANGLE(
        EGL_D3D11_DEVICE_ANGLE, reinterpret_cast<void *>(mDeviceContext), nullptr);
    EXPECT_EQ(EGL_NO_DEVICE_EXT, eglDevice);
    EXPECT_EGL_ERROR(EGL_BAD_ATTRIBUTE);
}

// Test that EGLDeviceEXT holds a ref to the D3D11 device
TEST_F(EGLDeviceCreationTest, D3D11DeviceReferenceCounting)
{
    if (!mDeviceCreationD3D11ExtAvailable || !mD3D11Available || !CreateD3D11Device())
    {
        std::cout << "EGLDevice creation and/or D3D11 not available, skipping test" << std::endl;
        return;
    }

    EGLDeviceEXT eglDevice =
        eglCreateDeviceANGLE(EGL_D3D11_DEVICE_ANGLE, reinterpret_cast<void *>(mDevice), nullptr);
    ASSERT_NE(EGL_NO_DEVICE_EXT, eglDevice);
    ASSERT_EGL_SUCCESS();

    // Now release our D3D11 device/context
    SafeRelease(mDevice);
    SafeRelease(mDeviceContext);

    EGLAttrib deviceAttrib;
    eglQueryDeviceAttribEXT(eglDevice, EGL_D3D11_DEVICE_ANGLE, &deviceAttrib);
    ASSERT_EGL_SUCCESS();

    ID3D11Device *queriedDevice = reinterpret_cast<ID3D11Device *>(deviceAttrib);
    ASSERT_EQ(mFeatureLevel, queriedDevice->GetFeatureLevel());

    eglReleaseDeviceANGLE(eglDevice);
}

// Test that creating a EGLDeviceEXT from a D3D9 device fails
TEST_F(EGLDeviceCreationTest, AnyD3D9Device)
{
    if (!mDeviceCreationD3D11ExtAvailable)
    {
        std::cout << "EGLDevice creation not available, skipping test" << std::endl;
        return;
    }

    std::string fakeD3DDevice = "This is a string, not a D3D device";

    EGLDeviceEXT eglDevice = eglCreateDeviceANGLE(
        EGL_D3D9_DEVICE_ANGLE, reinterpret_cast<void *>(&fakeD3DDevice), nullptr);
    EXPECT_EQ(EGL_NO_DEVICE_EXT, eglDevice);
    EXPECT_EGL_ERROR(EGL_BAD_ATTRIBUTE);
}

class EGLDeviceQueryTest : public ANGLETest
{
  protected:
    EGLDeviceQueryTest()
    {
        mQueryDisplayAttribEXT = nullptr;
        mQueryDeviceAttribEXT  = nullptr;
        mQueryDeviceStringEXT  = nullptr;
    }

    void SetUp() override
    {
        ANGLETest::SetUp();

        const char *extensionString =
            static_cast<const char *>(eglQueryString(getEGLWindow()->getDisplay(), EGL_EXTENSIONS));
        if (strstr(extensionString, "EGL_EXT_device_query"))
        {
            mQueryDisplayAttribEXT =
                (PFNEGLQUERYDISPLAYATTRIBEXTPROC)eglGetProcAddress("eglQueryDisplayAttribEXT");
            mQueryDeviceAttribEXT =
                (PFNEGLQUERYDEVICEATTRIBEXTPROC)eglGetProcAddress("eglQueryDeviceAttribEXT");
            mQueryDeviceStringEXT =
                (PFNEGLQUERYDEVICESTRINGEXTPROC)eglGetProcAddress("eglQueryDeviceStringEXT");
        }

        if (!mQueryDeviceStringEXT)
        {
            FAIL() << "ANGLE extension EGL_EXT_device_query export eglQueryDeviceStringEXT was not "
                      "found";
        }

        if (!mQueryDisplayAttribEXT)
        {
            FAIL() << "ANGLE extension EGL_EXT_device_query export eglQueryDisplayAttribEXT was "
                      "not found";
        }

        if (!mQueryDeviceAttribEXT)
        {
            FAIL() << "ANGLE extension EGL_EXT_device_query export eglQueryDeviceAttribEXT was not "
                      "found";
        }

        EGLAttrib angleDevice = 0;
        EXPECT_EQ(EGL_TRUE, mQueryDisplayAttribEXT(getEGLWindow()->getDisplay(), EGL_DEVICE_EXT,
                                                   &angleDevice));
        extensionString = static_cast<const char *>(
            mQueryDeviceStringEXT(reinterpret_cast<EGLDeviceEXT>(angleDevice), EGL_EXTENSIONS));
        if (strstr(extensionString, "EGL_ANGLE_device_d3d") == NULL)
        {
            FAIL() << "ANGLE extension EGL_ANGLE_device_d3d was not found";
        }
    }

    void TearDown() override { ANGLETest::TearDown(); }

    PFNEGLQUERYDISPLAYATTRIBEXTPROC mQueryDisplayAttribEXT;
    PFNEGLQUERYDEVICEATTRIBEXTPROC mQueryDeviceAttribEXT;
    PFNEGLQUERYDEVICESTRINGEXTPROC mQueryDeviceStringEXT;
};

// This test attempts to obtain a D3D11 device and a D3D9 device using the eglQueryDeviceAttribEXT
// function.
// If the test is configured to use D3D11 then it should succeed to obtain a D3D11 device.
// If the test is confitured to use D3D9, then it should succeed to obtain a D3D9 device.
TEST_P(EGLDeviceQueryTest, QueryDevice)
{
    EGLAttrib device      = 0;
    EGLAttrib angleDevice = 0;
    if (getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        EXPECT_EQ(EGL_TRUE, mQueryDisplayAttribEXT(getEGLWindow()->getDisplay(), EGL_DEVICE_EXT,
                                                   &angleDevice));
        EXPECT_EQ(EGL_TRUE, mQueryDeviceAttribEXT(reinterpret_cast<EGLDeviceEXT>(angleDevice),
                                                  EGL_D3D11_DEVICE_ANGLE, &device));
        ID3D11Device *d3d11Device = reinterpret_cast<ID3D11Device *>(device);
        IDXGIDevice *dxgiDevice = DynamicCastComObject<IDXGIDevice>(d3d11Device);
        EXPECT_TRUE(dxgiDevice != nullptr);
        SafeRelease(dxgiDevice);
    }

    if (getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE)
    {
        EXPECT_EQ(EGL_TRUE, mQueryDisplayAttribEXT(getEGLWindow()->getDisplay(), EGL_DEVICE_EXT,
                                                   &angleDevice));
        EXPECT_EQ(EGL_TRUE, mQueryDeviceAttribEXT(reinterpret_cast<EGLDeviceEXT>(angleDevice),
                                                  EGL_D3D9_DEVICE_ANGLE, &device));
        IDirect3DDevice9 *d3d9Device = reinterpret_cast<IDirect3DDevice9 *>(device);
        IDirect3D9 *d3d9 = nullptr;
        EXPECT_EQ(S_OK, d3d9Device->GetDirect3D(&d3d9));
        EXPECT_TRUE(d3d9 != nullptr);
        SafeRelease(d3d9);
    }
}

// This test attempts to obtain a D3D11 device from a D3D9 configured system and a D3D9 device from
// a D3D11 configured system using the eglQueryDeviceAttribEXT function.
// If the test is configured to use D3D11 then it should fail to obtain a D3D11 device.
// If the test is confitured to use D3D9, then it should fail to obtain a D3D9 device.
TEST_P(EGLDeviceQueryTest, QueryDeviceBadAttribute)
{
    EGLAttrib device      = 0;
    EGLAttrib angleDevice = 0;
    if (getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE)
    {
        EXPECT_EQ(EGL_TRUE, mQueryDisplayAttribEXT(getEGLWindow()->getDisplay(), EGL_DEVICE_EXT,
                                                   &angleDevice));
        EXPECT_EQ(EGL_FALSE, mQueryDeviceAttribEXT(reinterpret_cast<EGLDeviceEXT>(angleDevice),
                                                   EGL_D3D9_DEVICE_ANGLE, &device));
    }

    if (getPlatformRenderer() == EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE)
    {
        EXPECT_EQ(EGL_TRUE, mQueryDisplayAttribEXT(getEGLWindow()->getDisplay(), EGL_DEVICE_EXT,
                                                   &angleDevice));
        EXPECT_EQ(EGL_FALSE, mQueryDeviceAttribEXT(reinterpret_cast<EGLDeviceEXT>(angleDevice),
                                                   EGL_D3D11_DEVICE_ANGLE, &device));
    }
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST(EGLDeviceQueryTest, ES2_D3D9(), ES2_D3D11());

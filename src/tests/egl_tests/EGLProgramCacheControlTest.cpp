//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// EGLProgramCacheControlTest:
//   Unit tests for the EGL_ANGLE_program_cache_control extension.

#include <atomic>
#include <sstream>
#include <thread>

#include "common/angleutils.h"
#include "common/span.h"
#include "common/unsafe_buffers.h"
#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"
#include "util/EGLWindow.h"

using namespace angle;

constexpr EGLint kEnabledCacheSize = 0x10000;
constexpr char kEGLExtName[]       = "EGL_ANGLE_program_cache_control";

void TestCacheProgram(PlatformMethods *platform,
                      const ProgramKeyType &key,
                      size_t programSize,
                      const uint8_t *programBytes);

class EGLProgramCacheControlTest : public ANGLETest<>
{
  public:
    void onCache(const ProgramKeyType &key, Span<const uint8_t> programBinary)
    {
        mCachedKey = key;
        mCachedBinary.resize(programBinary.size());
        Span(mCachedBinary).copy_from(programBinary);
    }

  protected:
    EGLProgramCacheControlTest()
    {
        // Test flakiness was noticed when reusing displays.
        forceNewDisplay();
        setDeferContextInit(true);
        setContextProgramCacheEnabled(true);
        gDefaultPlatformMethods.cacheProgram = TestCacheProgram;
    }

    void testSetUp() override
    {
        if (extensionAvailable())
        {
            EGLDisplay display = getEGLWindow()->getDisplay();
            eglProgramCacheResizeANGLE(display, kEnabledCacheSize, EGL_PROGRAM_CACHE_RESIZE_ANGLE);
            ASSERT_EGL_SUCCESS();
        }

        ASSERT_TRUE(getEGLWindow()->initializeContext());
    }

    void testTearDown() override { gDefaultPlatformMethods.cacheProgram = DefaultCacheProgram; }

    bool extensionAvailable()
    {
        EGLDisplay display = getEGLWindow()->getDisplay();
        return IsEGLDisplayExtensionEnabled(display, kEGLExtName);
    }

    bool programBinaryAvailable()
    {
        return (getClientMajorVersion() >= 3 || IsGLExtensionEnabled("GL_OES_get_program_binary"));
    }

    ProgramKeyType mCachedKey;
    std::vector<uint8_t> mCachedBinary;
};

void TestCacheProgram(PlatformMethods *platform,
                      const ProgramKeyType &key,
                      size_t programSize,
                      const uint8_t *programBytes)
{
    // SAFETY: The signature of this callback is imposed by ANGLE's PlatformMethods
    // (CacheProgramFunc) and can't be changed. Its contract guarantees `programBytes` points to
    // `programSize` bytes.
    Span<const uint8_t> programBinary = ANGLE_UNSAFE_BUFFERS(Span(programBytes, programSize));

    auto *testPlatformContext = static_cast<TestPlatformContext *>(platform->context);
    auto *testCase =
        reinterpret_cast<EGLProgramCacheControlTest *>(testPlatformContext->currentTest);
    testCase->onCache(key, programBinary);
}

// Tests error conditions of the APIs.
TEST_P(EGLProgramCacheControlTest, NegativeAPI)
{
    ANGLE_SKIP_TEST_IF(!extensionAvailable());

    constexpr char kDefaultKey[]        = "defaultMakeItLongEnough";
    constexpr char kDefaultBinary[]     = "defaultMakeItLongEnough";
    constexpr EGLint kDefaultKeySize    = static_cast<EGLint>(ArraySize(kDefaultKey));
    constexpr EGLint kDefaultBinarySize = static_cast<EGLint>(ArraySize(kDefaultBinary));

    // Test that passing an invalid display to the entry point methods fails.
    eglProgramCacheGetAttribANGLE(EGL_NO_DISPLAY, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);

    eglProgramCachePopulateANGLE(EGL_NO_DISPLAY, kDefaultKey, kDefaultKeySize, kDefaultBinary,
                                 kDefaultBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);

    EGLint tempKeySize    = 0;
    EGLint tempBinarySize = 0;
    eglProgramCacheQueryANGLE(EGL_NO_DISPLAY, 0, nullptr, &tempKeySize, nullptr, &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);

    eglProgramCacheResizeANGLE(EGL_NO_DISPLAY, 0, EGL_PROGRAM_CACHE_TRIM_ANGLE);
    EXPECT_EGL_ERROR(EGL_BAD_DISPLAY);

    // Test querying properties with bad parameters.
    EGLDisplay display = getEGLWindow()->getDisplay();
    eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_RESIZE_ANGLE);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    // Test populating with invalid parameters.
    EGLint keySize = eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
    EXPECT_GT(kDefaultKeySize, keySize);
    eglProgramCachePopulateANGLE(display, kDefaultKey, keySize + 1, kDefaultBinary,
                                 kDefaultBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCachePopulateANGLE(display, kDefaultKey, keySize, kDefaultBinary, -1);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCachePopulateANGLE(display, nullptr, keySize, kDefaultBinary, kDefaultBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCachePopulateANGLE(display, kDefaultKey, keySize, nullptr, kDefaultBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    // Test querying cache entries with invalid parameters.
    eglProgramCachePopulateANGLE(display, kDefaultKey, keySize, kDefaultBinary, kDefaultBinarySize);
    ASSERT_EGL_SUCCESS();

    EGLint cacheSize = eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_SIZE_ANGLE);
    ASSERT_EQ(1, cacheSize);

    eglProgramCacheQueryANGLE(display, -1, nullptr, &tempKeySize, nullptr, &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheQueryANGLE(display, 1, nullptr, &tempKeySize, nullptr, &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheQueryANGLE(display, 0, nullptr, nullptr, nullptr, &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheQueryANGLE(display, 0, nullptr, &tempKeySize, nullptr, nullptr);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheQueryANGLE(display, 0, nullptr, &tempKeySize, nullptr, &tempBinarySize);
    ASSERT_EGL_SUCCESS();
    ASSERT_EQ(keySize, tempKeySize);
    ASSERT_EQ(kDefaultBinarySize, tempBinarySize);

    std::vector<uint8_t> tempKey(tempKeySize + 5);
    std::vector<uint8_t> tempBinary(tempBinarySize + 5);

    tempKeySize--;

    eglProgramCacheQueryANGLE(display, 0, tempKey.data(), &tempKeySize, tempBinary.data(),
                              &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    tempKeySize++;
    tempBinarySize--;

    eglProgramCacheQueryANGLE(display, 0, tempKey.data(), &tempKeySize, tempBinary.data(),
                              &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_ACCESS);

    EGLint negativeBinarySize = -1;
    eglProgramCacheQueryANGLE(display, 0, tempKey.data(), &tempKeySize, tempBinary.data(),
                              &negativeBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_ACCESS);

    // Mismatched key/binary null-ness is a parameter error regardless of the sizes.
    eglProgramCacheQueryANGLE(display, 0, nullptr, &tempKeySize, tempBinary.data(),
                              &negativeBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheQueryANGLE(display, 0, tempKey.data(), &tempKeySize, nullptr, &tempBinarySize);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    // Test resizing with invalid parameters.
    eglProgramCacheResizeANGLE(display, -1, EGL_PROGRAM_CACHE_TRIM_ANGLE);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);

    eglProgramCacheResizeANGLE(display, 0, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
    EXPECT_EGL_ERROR(EGL_BAD_PARAMETER);
}

// Tests a basic use case.
TEST_P(EGLProgramCacheControlTest, SaveAndReload)
{
    ANGLE_SKIP_TEST_IF(getEGLWindow()->isFeatureEnabled(Feature::DisableProgramCaching));

    ANGLE_SKIP_TEST_IF(!extensionAvailable() || !programBinaryAvailable());

    constexpr char kVS[] = "attribute vec4 position; void main() { gl_Position = position; }";
    constexpr char kFS[] = "void main() { gl_FragColor = vec4(1, 0, 0, 1); }";

    mCachedBinary.clear();
    // Link a program, which will miss the cache.
    {
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

        ANGLE_GL_PROGRAM(program, kVS, kFS);
        drawQuad(program, "position", 0.5f);
        EXPECT_GL_NO_ERROR();
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    }
    // Assert that the cache insertion added a program to the cache.
    EXPECT_TRUE(!mCachedBinary.empty());

    EGLDisplay display = getEGLWindow()->getDisplay();

    EGLint keySize    = 0;
    EGLint binarySize = 0;
    eglProgramCacheQueryANGLE(display, 0, nullptr, &keySize, nullptr, &binarySize);
    EXPECT_EQ(static_cast<EGLint>(mCachedKey.size()), keySize);
    ASSERT_EGL_SUCCESS();

    ProgramKeyType keyBuffer;
    std::vector<uint8_t> binaryBuffer(binarySize);
    eglProgramCacheQueryANGLE(display, 0, keyBuffer.data(), &keySize, binaryBuffer.data(),
                              &binarySize);
    ASSERT_EGL_SUCCESS();

    EXPECT_EQ(mCachedKey, keyBuffer);
    EXPECT_EQ(mCachedBinary, binaryBuffer);

    // Restart EGL and GL.
    recreateTestFixture();

    // Warm up the cache.
    EGLint newCacheSize = eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_SIZE_ANGLE);
    EXPECT_EQ(0, newCacheSize);
    eglProgramCachePopulateANGLE(display, keyBuffer.data(), keySize, binaryBuffer.data(),
                                 binarySize);

    mCachedBinary.clear();

    // Link a program, which will hit the cache.
    {
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

        ANGLE_GL_PROGRAM(program, kVS, kFS);
        drawQuad(program, "position", 0.5f);
        EXPECT_GL_NO_ERROR();
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    }

    // Verify no new shader was compiled.
    EXPECT_TRUE(mCachedBinary.empty());
}

// Tests that trying to link a program without correct shaders doesn't buggily call the cache.
TEST_P(EGLProgramCacheControlTest, LinkProgramWithBadShaders)
{
    ANGLE_SKIP_TEST_IF(!extensionAvailable());

    GLuint shader = glCreateShader(GL_FRAGMENT_SHADER);

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);

    GLint linkStatus = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
    EXPECT_GL_FALSE(linkStatus);
    EXPECT_GL_NO_ERROR();

    glDeleteShader(shader);
    glDeleteProgram(program);
}

// Tests the program cache can be disabled.
TEST_P(EGLProgramCacheControlTest, DisableProgramCache)
{
    ANGLE_SKIP_TEST_IF(!extensionAvailable() || !programBinaryAvailable());

    // Disable context program cache, and recreate context.
    setContextProgramCacheEnabled(false);
    recreateTestFixture();

    constexpr char kVS[] = "attribute vec4 position; void main() { gl_Position = position; }";
    constexpr char kFS[] = "void main() { gl_FragColor = vec4(1, 0, 0, 1); }";

    mCachedBinary.clear();
    // Link a program, which will miss the cache.
    {
        glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::green);

        ANGLE_GL_PROGRAM(program, kVS, kFS);
        drawQuad(program, "position", 0.5f);
        EXPECT_GL_NO_ERROR();
        EXPECT_PIXEL_COLOR_EQ(0, 0, GLColor::red);
    }

    // Expect that no program binary was inserted into the cache.
    EXPECT_TRUE(mCachedBinary.empty());
}

// Tests concurrent access to all EGL_ANGLE_program_cache_control entry points alongside program
// linking on another thread.
TEST_P(EGLProgramCacheControlTest, ConcurrentProgramCacheControl)
{
    ANGLE_SKIP_TEST_IF(!extensionAvailable());

    EGLDisplay display = getEGLWindow()->getDisplay();
    const EGLint keyLength =
        eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
    ASSERT_EGL_SUCCESS();
    ASSERT_GT(keyLength, 0);

    constexpr int kNumWorkerThreads = 4;
    constexpr int kWorkerIterations = 200;
    constexpr int kLinkIterations   = 16;

    std::atomic<bool> start(false);
    std::atomic<bool> stopWorkers(false);
    std::vector<std::thread> threads;
    threads.reserve(kNumWorkerThreads);

    for (int t = 0; t < kNumWorkerThreads; ++t)
    {
        threads.emplace_back([&, t]() {
            std::vector<uint8_t> key(keyLength, 0);
            std::vector<uint8_t> binary(256, 0);
            std::vector<uint8_t> queryKey(keyLength, 0);
            std::vector<uint8_t> queryBinary(kEnabledCacheSize, 0);

            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            // Run at least kWorkerIterations, then keep going until the main thread has finished
            // linking, with a hard cap in case linking is slow.
            constexpr int kMaxWorkerIterations = kWorkerIterations * 10;
            for (int i = 0; i < kMaxWorkerIterations; ++i)
            {
                if (i >= kWorkerIterations && stopWorkers.load(std::memory_order_acquire))
                {
                    break;
                }

                const uint8_t tag = static_cast<uint8_t>((t * 37 + i) & 0xFF);
                key[0]            = static_cast<uint8_t>(t);
                key[1]            = tag;
                binary[0]         = tag;

                switch (i % 5)
                {
                    case 0:
                    {
                        EGLint queriedKeyLen = eglProgramCacheGetAttribANGLE(
                            display, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
                        EXPECT_EGL_SUCCESS();
                        EXPECT_EQ(keyLength, queriedKeyLen);

                        EGLint cacheEntries =
                            eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_SIZE_ANGLE);
                        EXPECT_EGL_SUCCESS();
                        EXPECT_GE(cacheEntries, 0);
                        break;
                    }
                    case 1:
                    {
                        eglProgramCachePopulateANGLE(display, key.data(), keyLength, binary.data(),
                                                     static_cast<EGLint>(binary.size()));
                        EXPECT_EGL_SUCCESS();
                        break;
                    }
                    case 2:
                    {
                        EGLint count =
                            eglProgramCacheGetAttribANGLE(display, EGL_PROGRAM_CACHE_SIZE_ANGLE);
                        EXPECT_EGL_SUCCESS();
                        if (count > 0)
                        {
                            EGLint idx      = i % count;
                            EGLint qKeySize = 0;
                            EGLint qBinSize = 0;
                            eglProgramCacheQueryANGLE(display, idx, nullptr, &qKeySize, nullptr,
                                                      &qBinSize);
                            EGLint err = eglGetError();
                            EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_BAD_PARAMETER ||
                                        err == EGL_BAD_ACCESS);
                            if (err == EGL_SUCCESS &&
                                qBinSize <= static_cast<EGLint>(queryBinary.size()))
                            {
                                eglProgramCacheQueryANGLE(display, idx, queryKey.data(), &qKeySize,
                                                          queryBinary.data(), &qBinSize);
                                err = eglGetError();
                                EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_BAD_PARAMETER ||
                                            err == EGL_BAD_ACCESS);
                            }
                        }
                        break;
                    }
                    case 3:
                    {
                        EGLint trimmed = eglProgramCacheResizeANGLE(display, kEnabledCacheSize / 2,
                                                                    EGL_PROGRAM_CACHE_TRIM_ANGLE);
                        EXPECT_EGL_SUCCESS();
                        EXPECT_GE(trimmed, 0);
                        break;
                    }
                    case 4:
                    {
                        if (t == 0 && (i % 25 == 4))
                        {
                            EGLint prevSize = eglProgramCacheResizeANGLE(
                                display, kEnabledCacheSize, EGL_PROGRAM_CACHE_RESIZE_ANGLE);
                            EXPECT_EGL_SUCCESS();
                            EXPECT_GE(prevSize, 0);
                        }
                        break;
                    }
                }
            }

            EXPECT_EGL_TRUE(eglReleaseThread());
        });
    }

    start.store(true, std::memory_order_release);

    if (programBinaryAvailable())
    {
        constexpr char kVS[] = "attribute vec4 position; void main() { gl_Position = position; }";
        for (int i = 0; i < kLinkIterations; ++i)
        {
            std::ostringstream fs;
            fs << "void main() { gl_FragColor = vec4(" << ((i % 4) * 0.25f)
               << ", 0.5, 1.0, 1.0); }";
            ANGLE_GL_PROGRAM(program, kVS, fs.str().c_str());
            EXPECT_GL_NO_ERROR();
        }
    }

    stopWorkers.store(true, std::memory_order_release);
    for (std::thread &thread : threads)
    {
        thread.join();
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLProgramCacheControlTest);
ANGLE_INSTANTIATE_TEST(EGLProgramCacheControlTest,
                       ES2_D3D11(),
                       ES2_OPENGL(),
                       ES2_VULKAN());

class EGLProgramCacheControlTestNoFixture : public ANGLETest<>
{
  protected:
    void testTearDown() override
    {
        if (mDisplay != EGL_NO_DISPLAY)
        {
            EXPECT_EGL_TRUE(eglTerminate(mDisplay));
            mDisplay = EGL_NO_DISPLAY;
        }
        EXPECT_EGL_TRUE(eglReleaseThread());
    }

    EGLDisplay mDisplay = EGL_NO_DISPLAY;
};

// Tests racing EGL_ANGLE_program_cache_control entry points against concurrent eglInitialize() and
// eglTerminate() calls on another thread.
TEST_P(EGLProgramCacheControlTestNoFixture, ProgramCacheAndTerminateRace)
{
    std::vector<EGLAttrib> displayAttribs = {
        EGL_PLATFORM_ANGLE_TYPE_ANGLE,
        GetParam().getRenderer(),
        EGL_PLATFORM_ANGLE_NATIVE_PLATFORM_TYPE_ANGLE,
        static_cast<EGLAttrib>(GetPbufferOnlyDefaultPlatformType()),
    };
    if (GetParam().getDeviceType() != EGL_DONT_CARE)
    {
        displayAttribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
        displayAttribs.push_back(GetParam().getDeviceType());
    }
    displayAttribs.push_back(EGL_NONE);

    mDisplay = eglGetPlatformDisplay(
        GetEglPlatform(), reinterpret_cast<void *>(EGL_DEFAULT_DISPLAY), displayAttribs.data());
    ASSERT_NE(mDisplay, EGL_NO_DISPLAY);

    constexpr int kIterations    = 5;
    constexpr int kWorkerLoops   = 64;
    constexpr int kTriggerAtLoop = 8;

    for (int iter = 0; iter < kIterations; ++iter)
    {
        ASSERT_EGL_TRUE(eglInitialize(mDisplay, nullptr, nullptr));

        ANGLE_SKIP_TEST_IF(iter == 0 && !IsEGLDisplayExtensionEnabled(mDisplay, kEGLExtName));

        const EGLint keyLength =
            eglProgramCacheGetAttribANGLE(mDisplay, EGL_PROGRAM_CACHE_KEY_LENGTH_ANGLE);
        ASSERT_EGL_SUCCESS();
        ASSERT_GT(keyLength, 0);

        eglProgramCacheResizeANGLE(mDisplay, kEnabledCacheSize, EGL_PROGRAM_CACHE_RESIZE_ANGLE);
        ASSERT_EGL_SUCCESS();

        std::vector<uint8_t> initKey(keyLength, 1);
        std::vector<uint8_t> initBinary(128, 2);
        eglProgramCachePopulateANGLE(mDisplay, initKey.data(), keyLength, initBinary.data(),
                                     static_cast<EGLint>(initBinary.size()));
        ASSERT_EGL_SUCCESS();

        std::atomic<bool> start(false);
        std::atomic<bool> triggerTerminate(false);

        std::thread workerThread([&]() {
            std::vector<uint8_t> key(keyLength, 0);
            std::vector<uint8_t> binary(128, 0);
            std::vector<uint8_t> queryKey(keyLength, 0);
            std::vector<uint8_t> queryBinary(kEnabledCacheSize, 0);

            while (!start.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            for (int i = 0; i < kWorkerLoops; ++i)
            {
                if (i == kTriggerAtLoop)
                {
                    triggerTerminate.store(true, std::memory_order_release);
                }

                key[0]    = static_cast<uint8_t>(i & 0xFF);
                binary[0] = static_cast<uint8_t>(i & 0xFF);

                (void)eglProgramCacheGetAttribANGLE(mDisplay, EGL_PROGRAM_CACHE_SIZE_ANGLE);
                EGLint err = eglGetError();
                EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_NOT_INITIALIZED);

                eglProgramCachePopulateANGLE(mDisplay, key.data(), keyLength, binary.data(),
                                             static_cast<EGLint>(binary.size()));
                err = eglGetError();
                EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_NOT_INITIALIZED);

                EGLint qKeySize = keyLength;
                EGLint qBinSize = static_cast<EGLint>(queryBinary.size());
                eglProgramCacheQueryANGLE(mDisplay, 0, queryKey.data(), &qKeySize,
                                          queryBinary.data(), &qBinSize);
                err = eglGetError();
                EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_NOT_INITIALIZED ||
                            err == EGL_BAD_PARAMETER || err == EGL_BAD_ACCESS);

                (void)eglProgramCacheResizeANGLE(mDisplay, kEnabledCacheSize / 2,
                                                 EGL_PROGRAM_CACHE_TRIM_ANGLE);
                err = eglGetError();
                EXPECT_TRUE(err == EGL_SUCCESS || err == EGL_NOT_INITIALIZED);
            }

            EXPECT_EGL_TRUE(eglReleaseThread());
        });

        start.store(true, std::memory_order_release);

        // Run the redundant eglInitialize() and eglTerminate() on the same thread that initialized
        // the display so backends like DisplayEGL release their host current context cleanly.
        EXPECT_EGL_TRUE(eglInitialize(mDisplay, nullptr, nullptr));

        while (!triggerTerminate.load(std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        EXPECT_EGL_TRUE(eglTerminate(mDisplay));
        workerThread.join();
    }
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(EGLProgramCacheControlTestNoFixture);
ANGLE_INSTANTIATE_TEST(EGLProgramCacheControlTestNoFixture,
                       WithNoFixture(ES2_D3D11()),
                       WithNoFixture(ES2_OPENGL()),
                       WithNoFixture(ES2_VULKAN()));

//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "util/EGLWindow.h"

#include <cassert>
#include <iostream>
#include <vector>

#include <string.h>

#include "platform/Platform.h"
#include "util/OSWindow.h"
#include "util/system_utils.h"

EGLPlatformParameters::EGLPlatformParameters()
    : renderer(EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE),
      majorVersion(EGL_DONT_CARE),
      minorVersion(EGL_DONT_CARE),
      deviceType(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE),
      presentPath(EGL_DONT_CARE)
{}

EGLPlatformParameters::EGLPlatformParameters(EGLint renderer)
    : renderer(renderer),
      majorVersion(EGL_DONT_CARE),
      minorVersion(EGL_DONT_CARE),
      deviceType(EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE),
      presentPath(EGL_DONT_CARE)
{}

EGLPlatformParameters::EGLPlatformParameters(EGLint renderer,
                                             EGLint majorVersion,
                                             EGLint minorVersion,
                                             EGLint deviceType)
    : renderer(renderer),
      majorVersion(majorVersion),
      minorVersion(minorVersion),
      deviceType(deviceType),
      presentPath(EGL_DONT_CARE)
{}

EGLPlatformParameters::EGLPlatformParameters(EGLint renderer,
                                             EGLint majorVersion,
                                             EGLint minorVersion,
                                             EGLint deviceType,
                                             EGLint presentPath)
    : renderer(renderer),
      majorVersion(majorVersion),
      minorVersion(minorVersion),
      deviceType(deviceType),
      presentPath(presentPath)
{}

bool operator<(const EGLPlatformParameters &a, const EGLPlatformParameters &b)
{
    if (a.renderer != b.renderer)
    {
        return a.renderer < b.renderer;
    }

    if (a.majorVersion != b.majorVersion)
    {
        return a.majorVersion < b.majorVersion;
    }

    if (a.minorVersion != b.minorVersion)
    {
        return a.minorVersion < b.minorVersion;
    }

    if (a.deviceType != b.deviceType)
    {
        return a.deviceType < b.deviceType;
    }

    return a.presentPath < b.presentPath;
}

bool operator==(const EGLPlatformParameters &a, const EGLPlatformParameters &b)
{
    return (a.renderer == b.renderer) && (a.majorVersion == b.majorVersion) &&
           (a.minorVersion == b.minorVersion) && (a.deviceType == b.deviceType) &&
           (a.presentPath == b.presentPath);
}

// ConfigParameters implementation.
ConfigParameters::ConfigParameters()
    : platformMethods(nullptr),
      redBits(-1),
      greenBits(-1),
      blueBits(-1),
      alphaBits(-1),
      depthBits(-1),
      stencilBits(-1),
      swapInterval(-1),
      componentType(EGL_COLOR_COMPONENT_TYPE_FIXED_EXT),
      multisample(false),
      debug(false),
      noError(false),
      bindGeneratesResource(true),
      clientArraysEnabled(true),
      robustAccess(false),
      samples(-1)
{}

ConfigParameters::~ConfigParameters() = default;

void ConfigParameters::reset()
{
    *this = ConfigParameters();
}

// GLWindowBase implementation.
GLWindowBase::GLWindowBase(EGLint glesMajorVersion, EGLint glesMinorVersion)
    : mClientMajorVersion(glesMajorVersion), mClientMinorVersion(glesMinorVersion)
{}

GLWindowBase::~GLWindowBase() = default;

// EGLWindow implementation.
EGLWindow::EGLWindow(EGLint glesMajorVersion,
                     EGLint glesMinorVersion,
                     const EGLPlatformParameters &platform)
    : GLWindowBase(glesMajorVersion, glesMinorVersion),
      mConfig(0),
      mDisplay(EGL_NO_DISPLAY),
      mSurface(EGL_NO_SURFACE),
      mContext(EGL_NO_CONTEXT),
      mEGLMajorVersion(0),
      mEGLMinorVersion(0),
      mPlatform(platform)
{}

EGLWindow::~EGLWindow()
{
    destroyGL();
}

void EGLWindow::swap()
{
    eglSwapBuffers(mDisplay, mSurface);
}

EGLConfig EGLWindow::getConfig() const
{
    return mConfig;
}

EGLDisplay EGLWindow::getDisplay() const
{
    return mDisplay;
}

EGLSurface EGLWindow::getSurface() const
{
    return mSurface;
}

EGLContext EGLWindow::getContext() const
{
    return mContext;
}

bool EGLWindow::initializeGL(OSWindow *osWindow,
                             angle::Library *glWindowingLibrary,
                             const ConfigParameters &params)
{
    if (!initializeDisplay(osWindow, glWindowingLibrary, params))
        return false;
    if (!initializeSurface(osWindow, glWindowingLibrary, params))
        return false;
    return initializeContext();
}

bool EGLWindow::initializeDisplay(OSWindow *osWindow,
                                  angle::Library *glWindowingLibrary,
                                  const ConfigParameters &params)
{
    mConfigParams = params;

#if defined(ANGLE_USE_UTIL_LOADER)
    PFNEGLGETPROCADDRESSPROC getProcAddress;
    glWindowingLibrary->getAs("eglGetProcAddress", &getProcAddress);
    if (!getProcAddress)
    {
        return false;
    }

    // Likely we will need to use a fallback to Library::getAs on non-ANGLE platforms.
    angle::LoadEGL(getProcAddress);
#endif  // defined(ANGLE_USE_UTIL_LOADER)

    std::vector<EGLAttrib> displayAttributes;
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
    displayAttributes.push_back(mPlatform.renderer);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE);
    displayAttributes.push_back(mPlatform.majorVersion);
    displayAttributes.push_back(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE);
    displayAttributes.push_back(mPlatform.minorVersion);

    if (mPlatform.deviceType != EGL_DONT_CARE)
    {
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
        displayAttributes.push_back(mPlatform.deviceType);
    }

    if (mPlatform.presentPath != EGL_DONT_CARE)
    {
        const char *extensionString =
            static_cast<const char *>(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS));
        if (strstr(extensionString, "EGL_ANGLE_experimental_present_path") == nullptr)
        {
            destroyGL();
            return false;
        }

        displayAttributes.push_back(EGL_EXPERIMENTAL_PRESENT_PATH_ANGLE);
        displayAttributes.push_back(mPlatform.presentPath);
    }

    // Set debug layer settings if requested.
    if (mConfigParams.debugLayersEnabled.valid())
    {
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_DEBUG_LAYERS_ENABLED_ANGLE);
        displayAttributes.push_back(mConfigParams.debugLayersEnabled.value() ? EGL_TRUE
                                                                             : EGL_FALSE);
    }

    if (mConfigParams.contextVirtualization.valid())
    {
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_CONTEXT_VIRTUALIZATION_ANGLE);
        displayAttributes.push_back(mConfigParams.contextVirtualization.value() ? EGL_TRUE
                                                                                : EGL_FALSE);
    }

    if (mConfigParams.platformMethods)
    {
        static_assert(sizeof(EGLAttrib) == sizeof(mConfigParams.platformMethods),
                      "Unexpected pointer size");
        displayAttributes.push_back(EGL_PLATFORM_ANGLE_PLATFORM_METHODS_ANGLEX);
        displayAttributes.push_back(reinterpret_cast<EGLAttrib>(mConfigParams.platformMethods));
    }

    displayAttributes.push_back(EGL_NONE);

    mDisplay = eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                                     reinterpret_cast<void *>(osWindow->getNativeDisplay()),
                                     &displayAttributes[0]);
    if (mDisplay == EGL_NO_DISPLAY)
    {
        destroyGL();
        return false;
    }

    if (eglInitialize(mDisplay, &mEGLMajorVersion, &mEGLMinorVersion) == EGL_FALSE)
    {
        destroyGL();
        return false;
    }

    return true;
}

bool EGLWindow::initializeSurface(OSWindow *osWindow,
                                  angle::Library *glWindowingLibrary,
                                  const ConfigParameters &params)
{
    mConfigParams                 = params;
    const char *displayExtensions = eglQueryString(mDisplay, EGL_EXTENSIONS);

    std::vector<EGLint> configAttributes = {
        EGL_RED_SIZE,
        (mConfigParams.redBits >= 0) ? mConfigParams.redBits : EGL_DONT_CARE,
        EGL_GREEN_SIZE,
        (mConfigParams.greenBits >= 0) ? mConfigParams.greenBits : EGL_DONT_CARE,
        EGL_BLUE_SIZE,
        (mConfigParams.blueBits >= 0) ? mConfigParams.blueBits : EGL_DONT_CARE,
        EGL_ALPHA_SIZE,
        (mConfigParams.alphaBits >= 0) ? mConfigParams.alphaBits : EGL_DONT_CARE,
        EGL_DEPTH_SIZE,
        (mConfigParams.depthBits >= 0) ? mConfigParams.depthBits : EGL_DONT_CARE,
        EGL_STENCIL_SIZE,
        (mConfigParams.stencilBits >= 0) ? mConfigParams.stencilBits : EGL_DONT_CARE,
        EGL_SAMPLE_BUFFERS,
        mConfigParams.multisample ? 1 : 0,
        EGL_SAMPLES,
        (mConfigParams.samples >= 0) ? mConfigParams.samples : EGL_DONT_CARE,
    };

    // Add dynamic attributes
    bool hasPixelFormatFloat = strstr(displayExtensions, "EGL_EXT_pixel_format_float") != nullptr;
    if (!hasPixelFormatFloat && mConfigParams.componentType != EGL_COLOR_COMPONENT_TYPE_FIXED_EXT)
    {
        destroyGL();
        return false;
    }
    if (hasPixelFormatFloat)
    {
        configAttributes.push_back(EGL_COLOR_COMPONENT_TYPE_EXT);
        configAttributes.push_back(mConfigParams.componentType);
    }

    // Finish the attribute list
    configAttributes.push_back(EGL_NONE);

    if (!FindEGLConfig(mDisplay, configAttributes.data(), &mConfig))
    {
        std::cout << "Could not find a suitable EGL config!" << std::endl;
        destroyGL();
        return false;
    }

    eglGetConfigAttrib(mDisplay, mConfig, EGL_RED_SIZE, &mConfigParams.redBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_GREEN_SIZE, &mConfigParams.greenBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_BLUE_SIZE, &mConfigParams.blueBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_ALPHA_SIZE, &mConfigParams.alphaBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_DEPTH_SIZE, &mConfigParams.depthBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_STENCIL_SIZE, &mConfigParams.stencilBits);
    eglGetConfigAttrib(mDisplay, mConfig, EGL_SAMPLES, &mConfigParams.samples);

    std::vector<EGLint> surfaceAttributes;
    if (strstr(displayExtensions, "EGL_NV_post_sub_buffer") != nullptr)
    {
        surfaceAttributes.push_back(EGL_POST_SUB_BUFFER_SUPPORTED_NV);
        surfaceAttributes.push_back(EGL_TRUE);
    }

    bool hasRobustResourceInit =
        strstr(displayExtensions, "EGL_ANGLE_robust_resource_initialization") != nullptr;
    if (hasRobustResourceInit && mConfigParams.robustResourceInit.valid())
    {
        surfaceAttributes.push_back(EGL_ROBUST_RESOURCE_INITIALIZATION_ANGLE);
        surfaceAttributes.push_back(mConfigParams.robustResourceInit.value() ? EGL_TRUE
                                                                             : EGL_FALSE);
    }

    surfaceAttributes.push_back(EGL_NONE);

    osWindow->resetNativeWindow();

    mSurface = eglCreateWindowSurface(mDisplay, mConfig, osWindow->getNativeWindow(),
                                      &surfaceAttributes[0]);
    if (eglGetError() != EGL_SUCCESS || (mSurface == EGL_NO_SURFACE))
    {
        destroyGL();
        return false;
    }

#if defined(ANGLE_USE_UTIL_LOADER)
    angle::LoadGLES(eglGetProcAddress);
#endif  // defined(ANGLE_USE_UTIL_LOADER)

    return true;
}

EGLContext EGLWindow::createContext(EGLContext share) const
{
    const char *displayExtensions = eglQueryString(mDisplay, EGL_EXTENSIONS);

    // EGL_KHR_create_context is required to request a ES3+ context.
    bool hasKHRCreateContext = strstr(displayExtensions, "EGL_KHR_create_context") != nullptr;
    if (mClientMajorVersion > 2 && !(mEGLMajorVersion > 1 || mEGLMinorVersion >= 5) &&
        !hasKHRCreateContext)
    {
        std::cerr << "EGL_KHR_create_context incompatibility.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasWebGLCompatibility =
        strstr(displayExtensions, "EGL_ANGLE_create_context_webgl_compatibility") != nullptr;
    if (mConfigParams.webGLCompatibility.valid() && !hasWebGLCompatibility)
    {
        std::cerr << "EGL_ANGLE_create_context_webgl_compatibility missing.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasCreateContextExtensionsEnabled =
        strstr(displayExtensions, "EGL_ANGLE_create_context_extensions_enabled") != nullptr;
    if (mConfigParams.extensionsEnabled.valid() && !hasCreateContextExtensionsEnabled)
    {
        std::cerr << "EGL_ANGLE_create_context_extensions_enabled missing.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasRobustness = strstr(displayExtensions, "EGL_EXT_create_context_robustness") != nullptr;
    if (mConfigParams.robustAccess && !hasRobustness)
    {
        std::cerr << "EGL_EXT_create_context_robustness missing.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasBindGeneratesResource =
        strstr(displayExtensions, "EGL_CHROMIUM_create_context_bind_generates_resource") != nullptr;
    if (!mConfigParams.bindGeneratesResource && !hasBindGeneratesResource)
    {
        std::cerr << "EGL_CHROMIUM_create_context_bind_generates_resource missing.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasClientArraysExtension =
        strstr(displayExtensions, "EGL_ANGLE_create_context_client_arrays") != nullptr;
    if (!mConfigParams.clientArraysEnabled && !hasClientArraysExtension)
    {
        // Non-default state requested without the extension present
        std::cerr << "EGL_ANGLE_create_context_client_arrays missing.\n";
        return EGL_NO_CONTEXT;
    }

    bool hasProgramCacheControlExtension =
        strstr(displayExtensions, "EGL_ANGLE_program_cache_control ") != nullptr;
    if (mConfigParams.contextProgramCacheEnabled.valid() && !hasProgramCacheControlExtension)
    {
        std::cerr << "EGL_ANGLE_program_cache_control missing.\n";
        return EGL_NO_CONTEXT;
    }

    eglBindAPI(EGL_OPENGL_ES_API);
    if (eglGetError() != EGL_SUCCESS)
    {
        std::cerr << "Error on eglBindAPI.\n";
        return EGL_NO_CONTEXT;
    }

    std::vector<EGLint> contextAttributes;
    if (hasKHRCreateContext)
    {
        contextAttributes.push_back(EGL_CONTEXT_MAJOR_VERSION_KHR);
        contextAttributes.push_back(mClientMajorVersion);

        contextAttributes.push_back(EGL_CONTEXT_MINOR_VERSION_KHR);
        contextAttributes.push_back(mClientMinorVersion);

        contextAttributes.push_back(EGL_CONTEXT_OPENGL_DEBUG);
        contextAttributes.push_back(mConfigParams.debug ? EGL_TRUE : EGL_FALSE);

        // TODO(jmadill): Check for the extension string.
        // bool hasKHRCreateContextNoError = strstr(displayExtensions,
        // "EGL_KHR_create_context_no_error") != nullptr;

        contextAttributes.push_back(EGL_CONTEXT_OPENGL_NO_ERROR_KHR);
        contextAttributes.push_back(mConfigParams.noError ? EGL_TRUE : EGL_FALSE);

        if (mConfigParams.webGLCompatibility.valid())
        {
            contextAttributes.push_back(EGL_CONTEXT_WEBGL_COMPATIBILITY_ANGLE);
            contextAttributes.push_back(mConfigParams.webGLCompatibility.value() ? EGL_TRUE
                                                                                 : EGL_FALSE);
        }

        if (mConfigParams.extensionsEnabled.valid())
        {
            contextAttributes.push_back(EGL_EXTENSIONS_ENABLED_ANGLE);
            contextAttributes.push_back(mConfigParams.extensionsEnabled.value() ? EGL_TRUE
                                                                                : EGL_FALSE);
        }

        if (hasRobustness)
        {
            contextAttributes.push_back(EGL_CONTEXT_OPENGL_ROBUST_ACCESS_EXT);
            contextAttributes.push_back(mConfigParams.robustAccess ? EGL_TRUE : EGL_FALSE);
        }

        if (hasBindGeneratesResource)
        {
            contextAttributes.push_back(EGL_CONTEXT_BIND_GENERATES_RESOURCE_CHROMIUM);
            contextAttributes.push_back(mConfigParams.bindGeneratesResource ? EGL_TRUE : EGL_FALSE);
        }

        if (hasClientArraysExtension)
        {
            contextAttributes.push_back(EGL_CONTEXT_CLIENT_ARRAYS_ENABLED_ANGLE);
            contextAttributes.push_back(mConfigParams.clientArraysEnabled ? EGL_TRUE : EGL_FALSE);
        }

        if (mConfigParams.contextProgramCacheEnabled.valid())
        {
            contextAttributes.push_back(EGL_CONTEXT_PROGRAM_BINARY_CACHE_ENABLED_ANGLE);
            contextAttributes.push_back(
                mConfigParams.contextProgramCacheEnabled.value() ? EGL_TRUE : EGL_FALSE);
        }

        bool hasRobustResourceInit =
            strstr(displayExtensions, "EGL_ANGLE_robust_resource_initialization") != nullptr;
        if (hasRobustResourceInit && mConfigParams.robustResourceInit.valid())
        {
            contextAttributes.push_back(EGL_ROBUST_RESOURCE_INITIALIZATION_ANGLE);
            contextAttributes.push_back(mConfigParams.robustResourceInit.value() ? EGL_TRUE
                                                                                 : EGL_FALSE);
        }
    }
    contextAttributes.push_back(EGL_NONE);

    EGLContext context = eglCreateContext(mDisplay, mConfig, nullptr, &contextAttributes[0]);
    if (eglGetError() != EGL_SUCCESS)
    {
        std::cerr << "Error on eglCreateContext.\n";
        return EGL_NO_CONTEXT;
    }

    return context;
}

bool EGLWindow::initializeContext()
{
    mContext = createContext(EGL_NO_CONTEXT);
    if (mContext == EGL_NO_CONTEXT)
    {
        destroyGL();
        return false;
    }

    eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
    if (eglGetError() != EGL_SUCCESS)
    {
        std::cerr << "Error during eglMakeCurrent.\n";
        destroyGL();
        return false;
    }

    if (mConfigParams.swapInterval != -1)
    {
        eglSwapInterval(mDisplay, mConfigParams.swapInterval);
    }

    return true;
}

void EGLWindow::destroyGL()
{
    destroyContext();
    destroySurface();

    if (mDisplay != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglTerminate(mDisplay);
        mDisplay = EGL_NO_DISPLAY;
    }
}

void EGLWindow::destroySurface()
{
    if (mSurface != EGL_NO_SURFACE)
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        assert(mDisplay != EGL_NO_DISPLAY);
        eglDestroySurface(mDisplay, mSurface);
        mSurface = EGL_NO_SURFACE;
    }
}

void EGLWindow::destroyContext()
{
    if (mContext != EGL_NO_CONTEXT)
    {
        eglMakeCurrent(mDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        assert(mDisplay != EGL_NO_DISPLAY);
        eglDestroyContext(mDisplay, mContext);
        mContext = EGL_NO_CONTEXT;
    }
}

bool EGLWindow::isGLInitialized() const
{
    return mSurface != EGL_NO_SURFACE && mContext != EGL_NO_CONTEXT && mDisplay != EGL_NO_DISPLAY;
}

// Find an EGLConfig that is an exact match for the specified attributes. EGL_FALSE is returned if
// the EGLConfig is found.  This indicates that the EGLConfig is not supported.
EGLBoolean EGLWindow::FindEGLConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *config)
{
    EGLint numConfigs = 0;
    eglGetConfigs(dpy, nullptr, 0, &numConfigs);
    std::vector<EGLConfig> allConfigs(numConfigs);
    eglGetConfigs(dpy, allConfigs.data(), static_cast<EGLint>(allConfigs.size()), &numConfigs);

    for (size_t i = 0; i < allConfigs.size(); i++)
    {
        bool matchFound = true;
        for (const EGLint *curAttrib = attrib_list; curAttrib[0] != EGL_NONE; curAttrib += 2)
        {
            if (curAttrib[1] == EGL_DONT_CARE)
            {
                continue;
            }

            EGLint actualValue = EGL_DONT_CARE;
            eglGetConfigAttrib(dpy, allConfigs[i], curAttrib[0], &actualValue);
            if (curAttrib[1] != actualValue)
            {
                matchFound = false;
                break;
            }
        }

        if (matchFound)
        {
            *config = allConfigs[i];
            return EGL_TRUE;
        }
    }

    return EGL_FALSE;
}

void EGLWindow::makeCurrent()
{
    eglMakeCurrent(mDisplay, mSurface, mSurface, mContext);
}

bool EGLWindow::hasError() const
{
    return eglGetError() != EGL_SUCCESS;
}

// static
bool EGLWindow::ClientExtensionEnabled(const std::string &extName)
{
    return CheckExtensionExists(eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS), extName);
}

bool CheckExtensionExists(const char *allExtensions, const std::string &extName)
{
    const std::string paddedExtensions = std::string(" ") + allExtensions + std::string(" ");
    return paddedExtensions.find(std::string(" ") + extName + std::string(" ")) !=
           std::string::npos;
}

// static
void GLWindowBase::Delete(GLWindowBase **window)
{
    delete *window;
    *window = nullptr;
}

// static
EGLWindow *EGLWindow::New(EGLint glesMajorVersion,
                          EGLint glesMinorVersion,
                          const EGLPlatformParameters &platform)
{
    return new EGLWindow(glesMajorVersion, glesMinorVersion, platform);
}

// static
void EGLWindow::Delete(EGLWindow **window)
{
    delete *window;
    *window = nullptr;
}

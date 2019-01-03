//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef UTIL_EGLWINDOW_H_
#define UTIL_EGLWINDOW_H_

#include <stdint.h>
#include <list>
#include <memory>
#include <string>

#include "common/Optional.h"
#include "common/angleutils.h"
#include "util/EGLPlatformParameters.h"
#include "util/util_export.h"
#include "util/util_gl.h"

class OSWindow;

namespace angle
{
class Library;
struct PlatformMethods;
}  // namespace angle

class ANGLE_UTIL_EXPORT GLWindowBase : angle::NonCopyable
{
  public:
    static void Delete(GLWindowBase **window);

    // It should also be possible to set multisample and floating point framebuffers.
    void setConfigRedBits(int bits) { mRedBits = bits; }
    void setConfigGreenBits(int bits) { mGreenBits = bits; }
    void setConfigBlueBits(int bits) { mBlueBits = bits; }
    void setConfigAlphaBits(int bits) { mAlphaBits = bits; }
    void setConfigDepthBits(int bits) { mDepthBits = bits; }
    void setConfigStencilBits(int bits) { mStencilBits = bits; }
    void setSwapInterval(int swapInterval) { mSwapInterval = swapInterval; }

    int getConfigRedBits() const { return mRedBits; }
    int getConfigGreenBits() const { return mGreenBits; }
    int getConfigBlueBits() const { return mBlueBits; }
    int getConfigAlphaBits() const { return mAlphaBits; }
    int getConfigDepthBits() const { return mDepthBits; }
    int getConfigStencilBits() const { return mStencilBits; }
    int getSwapInterval() const { return mSwapInterval; }
    void setPlatformMethods(angle::PlatformMethods *platformMethods)
    {
        mPlatformMethods = platformMethods;
    }
    void setWebGLCompatibilityEnabled(bool webglCompatibility)
    {
        mWebGLCompatibility = webglCompatibility;
    }
    void setRobustResourceInit(bool enabled) { mRobustResourceInit = enabled; }

    EGLint getClientMajorVersion() const { return mClientMajorVersion; }
    EGLint getClientMinorVersion() const { return mClientMinorVersion; }

    virtual bool initializeGL(OSWindow *osWindow, angle::Library *glLibrary) = 0;
    virtual bool isGLInitialized() const                                     = 0;
    virtual void swap()                                                      = 0;
    virtual void destroyGL()                                                 = 0;
    virtual void makeCurrent()                                               = 0;

  protected:
    GLWindowBase(EGLint glesMajorVersion, EGLint glesMinorVersion);
    virtual ~GLWindowBase();

    EGLint mClientMajorVersion;
    EGLint mClientMinorVersion;
    int mRedBits;
    int mGreenBits;
    int mBlueBits;
    int mAlphaBits;
    int mDepthBits;
    int mStencilBits;
    int mSwapInterval;

    angle::PlatformMethods *mPlatformMethods;
    Optional<bool> mWebGLCompatibility;
    Optional<bool> mRobustResourceInit;
};

class ANGLE_UTIL_EXPORT EGLWindow : public GLWindowBase
{
  public:
    static EGLWindow *New(EGLint glesMajorVersion,
                          EGLint glesMinorVersion,
                          const EGLPlatformParameters &platform);
    static void Delete(EGLWindow **window);

    void setConfigComponentType(EGLenum componentType) { mComponentType = componentType; }
    void setMultisample(bool multisample) { mMultisample = multisample; }
    void setSamples(EGLint samples) { mSamples = samples; }
    void setDebugEnabled(bool debug) { mDebug = debug; }
    void setNoErrorEnabled(bool noError) { mNoError = noError; }
    void setExtensionsEnabled(bool extensionsEnabled) { mExtensionsEnabled = extensionsEnabled; }
    void setBindGeneratesResource(bool bindGeneratesResource)
    {
        mBindGeneratesResource = bindGeneratesResource;
    }
    void setDebugLayersEnabled(bool enabled) { mDebugLayersEnabled = enabled; }
    void setClientArraysEnabled(bool enabled) { mClientArraysEnabled = enabled; }
    void setRobustAccess(bool enabled) { mRobustAccess = enabled; }
    void setContextProgramCacheEnabled(bool enabled) { mContextProgramCacheEnabled = enabled; }
    void setContextVirtualization(bool enabled) { mContextVirtualization = enabled; }

    static EGLBoolean FindEGLConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *config);

    void swap() override;

    const EGLPlatformParameters &getPlatform() const { return mPlatform; }
    EGLConfig getConfig() const;
    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;
    bool isMultisample() const { return mMultisample; }
    bool isDebugEnabled() const { return mDebug; }
    const angle::PlatformMethods *getPlatformMethods() const { return mPlatformMethods; }

    // Internally initializes the Display, Surface and Context.
    bool initializeGL(OSWindow *osWindow, angle::Library *glWindowingLibrary) override;

    // Only initializes the Display and Surface.
    bool initializeDisplayAndSurface(OSWindow *osWindow, angle::Library *glWindowingLibrary);

    // Create an EGL context with this window's configuration
    EGLContext createContext(EGLContext share) const;

    // Only initializes the Context.
    bool initializeContext();

    void destroyGL() override;
    bool isGLInitialized() const override;
    void makeCurrent() override;

    static bool ClientExtensionEnabled(const std::string &extName);

  private:
    EGLWindow(EGLint glesMajorVersion,
              EGLint glesMinorVersion,
              const EGLPlatformParameters &platform);

    ~EGLWindow() override;

    EGLConfig mConfig;
    EGLDisplay mDisplay;
    EGLSurface mSurface;
    EGLContext mContext;

    EGLint mEGLMajorVersion;
    EGLint mEGLMinorVersion;
    EGLPlatformParameters mPlatform;
    EGLenum mComponentType;
    bool mMultisample;
    bool mDebug;
    bool mNoError;
    Optional<bool> mExtensionsEnabled;
    bool mBindGeneratesResource;
    bool mClientArraysEnabled;
    bool mRobustAccess;
    EGLint mSamples;
    Optional<bool> mDebugLayersEnabled;
    Optional<bool> mContextProgramCacheEnabled;
    Optional<bool> mContextVirtualization;
};

ANGLE_UTIL_EXPORT bool CheckExtensionExists(const char *allExtensions, const std::string &extName);

#endif  // UTIL_EGLWINDOW_H_

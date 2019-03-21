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

struct ANGLE_UTIL_EXPORT ConfigParameters
{
    ConfigParameters();
    ~ConfigParameters();

    void reset();

    static bool CanShareDisplay(const ConfigParameters &a, const ConfigParameters &b);

    // Display parameters.
    Optional<bool> debugLayersEnabled;
    Optional<bool> contextVirtualization;
    angle::PlatformMethods *platformMethods;

    // Surface and Context parameters.
    int redBits;
    int greenBits;
    int blueBits;
    int alphaBits;
    int depthBits;
    int stencilBits;
    int swapInterval;

    Optional<bool> webGLCompatibility;
    Optional<bool> robustResourceInit;

    // EGLWindow-specific.
    EGLenum componentType;
    bool multisample;
    bool debug;
    bool noError;
    Optional<bool> extensionsEnabled;
    bool bindGeneratesResource;
    bool clientArraysEnabled;
    bool robustAccess;
    EGLint samples;
    Optional<bool> contextProgramCacheEnabled;
};

class ANGLE_UTIL_EXPORT GLWindowBase : angle::NonCopyable
{
  public:
    static void Delete(GLWindowBase **window);

    // It should also be possible to set multisample and floating point framebuffers.
    EGLint getClientMajorVersion() const { return mClientMajorVersion; }
    EGLint getClientMinorVersion() const { return mClientMinorVersion; }

    virtual bool initializeGL(OSWindow *osWindow,
                              angle::Library *glWindowingLibrary,
                              const ConfigParameters &config) = 0;
    virtual bool isGLInitialized() const                      = 0;
    virtual void swap()                                       = 0;
    virtual void destroyGL()                                  = 0;
    virtual void makeCurrent()                                = 0;
    virtual bool hasError() const                             = 0;

    int getConfigRedBits() const { return mConfigParams.redBits; }
    int getConfigGreenBits() const { return mConfigParams.greenBits; }
    int getConfigBlueBits() const { return mConfigParams.blueBits; }
    int getConfigAlphaBits() const { return mConfigParams.alphaBits; }
    int getConfigDepthBits() const { return mConfigParams.depthBits; }
    int getConfigStencilBits() const { return mConfigParams.stencilBits; }
    int getSwapInterval() const { return mConfigParams.swapInterval; }

    bool isMultisample() const { return mConfigParams.multisample; }
    bool isDebugEnabled() const { return mConfigParams.debug; }

    const angle::PlatformMethods *getPlatformMethods() const
    {
        return mConfigParams.platformMethods;
    }

    const ConfigParameters &getConfigParams() const { return mConfigParams; }

  protected:
    GLWindowBase(EGLint glesMajorVersion, EGLint glesMinorVersion);
    virtual ~GLWindowBase();

    EGLint mClientMajorVersion;
    EGLint mClientMinorVersion;
    ConfigParameters mConfigParams;
};

class ANGLE_UTIL_EXPORT EGLWindow : public GLWindowBase
{
  public:
    static EGLWindow *New(EGLint glesMajorVersion,
                          EGLint glesMinorVersion,
                          const EGLPlatformParameters &platform);
    static void Delete(EGLWindow **window);

    static EGLBoolean FindEGLConfig(EGLDisplay dpy, const EGLint *attrib_list, EGLConfig *config);

    void swap() override;

    const EGLPlatformParameters &getPlatform() const { return mPlatform; }
    EGLConfig getConfig() const;
    EGLDisplay getDisplay() const;
    EGLSurface getSurface() const;
    EGLContext getContext() const;

    // Internally initializes the Display, Surface and Context.
    bool initializeGL(OSWindow *osWindow,
                      angle::Library *glWindowingLibrary,
                      const ConfigParameters &params) override;

    // Only initializes the Display.
    bool initializeDisplay(OSWindow *osWindow,
                           angle::Library *glWindowingLibrary,
                           const ConfigParameters &params);

    // Only initializes the Surface.
    bool initializeSurface(OSWindow *osWindow,
                           angle::Library *glWindowingLibrary,
                           const ConfigParameters &params);

    // Create an EGL context with this window's configuration
    EGLContext createContext(EGLContext share) const;

    // Only initializes the Context.
    bool initializeContext();

    void destroyGL() override;
    void destroySurface();
    void destroyContext();
    bool isGLInitialized() const override;
    void makeCurrent() override;
    bool hasError() const override;

    bool isDisplayInitialized() const { return mDisplay != EGL_NO_DISPLAY; }

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
};

ANGLE_UTIL_EXPORT bool CheckExtensionExists(const char *allExtensions, const std::string &extName);

#endif  // UTIL_EGLWINDOW_H_

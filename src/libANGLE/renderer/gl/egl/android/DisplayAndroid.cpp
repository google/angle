//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayAndroid.cpp: Android implementation of egl::Display

#include "libANGLE/renderer/gl/egl/android/DisplayAndroid.h"

#include <android/log.h>
#include <android/native_window.h>

#include "common/debug.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/gl/ContextGL.h"
#include "libANGLE/renderer/gl/RendererGL.h"
#include "libANGLE/renderer/gl/egl/ContextEGL.h"
#include "libANGLE/renderer/gl/egl/FunctionsEGLDL.h"
#include "libANGLE/renderer/gl/egl/RendererEGL.h"
#include "libANGLE/renderer/gl/egl/SurfaceEGL.h"
#include "libANGLE/renderer/gl/egl/android/NativeBufferImageSiblingAndroid.h"
#include "libANGLE/renderer/gl/renderergl_utils.h"

namespace
{
const char *GetEGLPath()
{
#if defined(__LP64__)
    return "/system/lib64/libEGL.so";
#else
    return "/system/lib/libEGL.so";
#endif
}
}  // namespace

namespace rx
{

static constexpr bool kDefaultEGLVirtualizedContexts = true;

DisplayAndroid::DisplayAndroid(const egl::DisplayState &state)
    : DisplayEGL(state),
      mVirtualizedContexts(kDefaultEGLVirtualizedContexts),
      mSupportsSurfaceless(false),
      mMockPbuffer(EGL_NO_SURFACE)
{}

DisplayAndroid::~DisplayAndroid() {}

egl::Error DisplayAndroid::initialize(egl::Display *display)
{
    mDisplayAttributes = display->getAttributeMap();
    mVirtualizedContexts =
        ShouldUseVirtualizedContexts(mDisplayAttributes, kDefaultEGLVirtualizedContexts);

    FunctionsEGLDL *egl = new FunctionsEGLDL();
    mEGL                = egl;
    void *eglHandle =
        reinterpret_cast<void *>(mDisplayAttributes.get(EGL_PLATFORM_ANGLE_EGL_HANDLE_ANGLE, 0));
    ANGLE_TRY(egl->initialize(display->getNativeDisplayId(), GetEGLPath(), eglHandle));

    gl::Version eglVersion(mEGL->majorVersion, mEGL->minorVersion);
    ASSERT(eglVersion >= gl::Version(1, 4));

    std::vector<EGLint> renderableTypes;
    static_assert(EGL_OPENGL_ES3_BIT == EGL_OPENGL_ES3_BIT_KHR, "Extension define must match core");
    if (eglVersion >= gl::Version(1, 5) || mEGL->hasExtension("EGL_KHR_create_context"))
    {
        renderableTypes.push_back(EGL_OPENGL_ES3_BIT);
    }
    renderableTypes.push_back(EGL_OPENGL_ES2_BIT);

    egl::AttributeMap baseConfigAttribs;
    baseConfigAttribs.insert(EGL_COLOR_BUFFER_TYPE, EGL_RGB_BUFFER);
    // Android doesn't support pixmaps
    baseConfigAttribs.insert(EGL_SURFACE_TYPE, EGL_WINDOW_BIT | EGL_PBUFFER_BIT);

    egl::AttributeMap configAttribsWithFormat(baseConfigAttribs);
    // Choose RGBA8888
    configAttribsWithFormat.insert(EGL_RED_SIZE, 8);
    configAttribsWithFormat.insert(EGL_GREEN_SIZE, 8);
    configAttribsWithFormat.insert(EGL_BLUE_SIZE, 8);
    configAttribsWithFormat.insert(EGL_ALPHA_SIZE, 8);

    // Choose D24S8
    // EGL1.5 spec Section 2.2 says that depth, multisample and stencil buffer depths
    // must match for contexts to be compatible.
    configAttribsWithFormat.insert(EGL_DEPTH_SIZE, 24);
    configAttribsWithFormat.insert(EGL_STENCIL_SIZE, 8);

    EGLConfig configWithFormat = EGL_NO_CONFIG_KHR;
    for (EGLint renderableType : renderableTypes)
    {
        baseConfigAttribs.insert(EGL_RENDERABLE_TYPE, renderableType);
        configAttribsWithFormat.insert(EGL_RENDERABLE_TYPE, renderableType);

        std::vector<EGLint> attribVector = configAttribsWithFormat.toIntVector();

        EGLint numConfig = 0;
        if (mEGL->chooseConfig(attribVector.data(), &configWithFormat, 1, &numConfig) == EGL_TRUE)
        {
            break;
        }
    }

    if (configWithFormat == EGL_NO_CONFIG_KHR)
    {
        return egl::EglNotInitialized()
               << "eglChooseConfig failed with " << egl::Error(mEGL->getError());
    }

    // A mock pbuffer is only needed if surfaceless contexts are not supported.
    mSupportsSurfaceless = mEGL->hasExtension("EGL_KHR_surfaceless_context");
    if (!mSupportsSurfaceless)
    {
        int mockPbufferAttribs[] = {
            EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE,
        };
        mMockPbuffer = mEGL->createPbufferSurface(configWithFormat, mockPbufferAttribs);
        if (mMockPbuffer == EGL_NO_SURFACE)
        {
            return egl::EglNotInitialized()
                   << "eglCreatePbufferSurface failed with " << egl::Error(mEGL->getError());
        }
    }

    // Create mMockPbuffer with a normal config, but create a no_config mContext, if possible
    if (mEGL->hasExtension("EGL_KHR_no_config_context"))
    {
        mConfigAttribList = baseConfigAttribs.toIntVector();
        mConfig           = EGL_NO_CONFIG_KHR;
    }
    else
    {
        mConfigAttribList = configAttribsWithFormat.toIntVector();
        mConfig           = configWithFormat;
    }

    ANGLE_TRY(createRenderer(EGL_NO_CONTEXT, true, false, &mRenderer));

    const gl::Version &maxVersion = mRenderer->getMaxSupportedESVersion();
    if (maxVersion < gl::Version(2, 0))
    {
        return egl::EglNotInitialized() << "OpenGL ES 2.0 is not supportable.";
    }

    ANGLE_TRY(DisplayGL::initialize(display));

    std::string rendererDescription = getRendererDescription();
    __android_log_print(ANDROID_LOG_INFO, "ANGLE", "%s", rendererDescription.c_str());
    return egl::NoError();
}

void DisplayAndroid::terminate()
{
    DisplayGL::terminate();

    EGLBoolean success = mEGL->makeCurrent(EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (success == EGL_FALSE)
    {
        ERR() << "eglMakeCurrent error " << egl::Error(mEGL->getError());
    }

    if (mMockPbuffer != EGL_NO_SURFACE)
    {
        success      = mEGL->destroySurface(mMockPbuffer);
        mMockPbuffer = EGL_NO_SURFACE;
        if (success == EGL_FALSE)
        {
            ERR() << "eglDestroySurface error " << egl::Error(mEGL->getError());
        }
    }

    mRenderer.reset();

    egl::Error result = mEGL->terminate();
    if (result.isError())
    {
        ERR() << "eglTerminate error " << result;
    }

    SafeDelete(mEGL);
}

ContextImpl *DisplayAndroid::createContext(const gl::State &state,
                                           gl::ErrorSet *errorSet,
                                           const egl::Config *configuration,
                                           const gl::Context *shareContext,
                                           const egl::AttributeMap &attribs)
{
    std::shared_ptr<RendererEGL> renderer;
    bool usingExternalContext = attribs.get(EGL_EXTERNAL_CONTEXT_ANGLE, EGL_FALSE) == EGL_TRUE;
    if (mVirtualizedContexts && !usingExternalContext)
    {
        renderer = mRenderer;
    }
    else
    {
        EGLContext nativeShareContext = EGL_NO_CONTEXT;
        if (usingExternalContext)
        {
            ASSERT(!shareContext);
        }
        else if (shareContext)
        {
            ContextEGL *shareContextEGL = GetImplAs<ContextEGL>(shareContext);
            nativeShareContext          = shareContextEGL->getContext();
        }

        // Create a new renderer for this context.  It only needs to share with the user's requested
        // share context because there are no internal resources in DisplayAndroid that are shared
        // at the GL level.
        egl::Error error =
            createRenderer(nativeShareContext, false, usingExternalContext, &renderer);
        if (error.isError())
        {
            ERR() << "Failed to create a shared renderer: " << error.getMessage();
            return nullptr;
        }
    }

    return new ContextEGL(state, errorSet, renderer,
                          RobustnessVideoMemoryPurgeStatus::NOT_REQUESTED);
}

class ExternalSurfaceEGL : public SurfaceEGL
{
  public:
    ExternalSurfaceEGL(const egl::SurfaceState &state,
                       const FunctionsEGL *egl,
                       EGLConfig config,
                       EGLint width,
                       EGLint height)
        : SurfaceEGL(state, egl, config), mWidth(width), mHeight(height)
    {}
    ~ExternalSurfaceEGL() override = default;

    egl::Error initialize(const egl::Display *display) override { return egl::NoError(); }
    EGLint getSwapBehavior() const override { return EGL_BUFFER_DESTROYED; }
    EGLint getWidth() const override { return mWidth; }
    EGLint getHeight() const override { return mHeight; }
    bool isExternal() const override { return true; }

  private:
    const EGLint mWidth;
    const EGLint mHeight;
};

SurfaceImpl *DisplayAndroid::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                           EGLenum buftype,
                                                           EGLClientBuffer clientBuffer,
                                                           const egl::AttributeMap &attribs)
{
    if (buftype == EGL_EXTERNAL_SURFACE_ANGLE)
    {
        ASSERT(clientBuffer == nullptr);

        EGLint width  = static_cast<EGLint>(attribs.get(EGL_WIDTH, 0));
        EGLint height = static_cast<EGLint>(attribs.get(EGL_HEIGHT, 0));

        // Use the ExternalSurfaceEGL, so ANGLE can know the framebuffer size.
        return new ExternalSurfaceEGL(state, mEGL, EGL_NO_CONFIG_KHR, width, height);
    }

    return DisplayEGL::createPbufferFromClientBuffer(state, buftype, clientBuffer, attribs);
}

bool DisplayAndroid::isValidNativeWindow(EGLNativeWindowType window) const
{
    return ANativeWindow_getFormat(window) >= 0;
}

egl::Error DisplayAndroid::validateClientBuffer(const egl::Config *configuration,
                                                EGLenum buftype,
                                                EGLClientBuffer clientBuffer,
                                                const egl::AttributeMap &attribs) const
{

    if (buftype == EGL_EXTERNAL_SURFACE_ANGLE)
    {
        ASSERT(clientBuffer == nullptr);
        return egl::NoError();
    }
    return DisplayEGL::validateClientBuffer(configuration, buftype, clientBuffer, attribs);
}

egl::Error DisplayAndroid::validateImageClientBuffer(const gl::Context *context,
                                                     EGLenum target,
                                                     EGLClientBuffer clientBuffer,
                                                     const egl::AttributeMap &attribs) const
{
    switch (target)
    {
        case EGL_NATIVE_BUFFER_ANDROID:
            return egl::NoError();

        default:
            return DisplayEGL::validateImageClientBuffer(context, target, clientBuffer, attribs);
    }
}

ExternalImageSiblingImpl *DisplayAndroid::createExternalImageSibling(
    const gl::Context *context,
    EGLenum target,
    EGLClientBuffer buffer,
    const egl::AttributeMap &attribs)
{
    switch (target)
    {
        case EGL_NATIVE_BUFFER_ANDROID:
            return new NativeBufferImageSiblingAndroid(buffer);

        default:
            return DisplayEGL::createExternalImageSibling(context, target, buffer, attribs);
    }
}

egl::Error DisplayAndroid::makeCurrent(egl::Display *display,
                                       egl::Surface *drawSurface,
                                       egl::Surface *readSurface,
                                       gl::Context *context)
{
    CurrentNativeContext &currentContext = mCurrentNativeContexts[std::this_thread::get_id()];

    EGLSurface newSurface = EGL_NO_SURFACE;
    if (drawSurface)
    {
        SurfaceEGL *drawSurfaceEGL = GetImplAs<SurfaceEGL>(drawSurface);
        newSurface                 = drawSurfaceEGL->getSurface();
    }

    EGLContext newContext = EGL_NO_CONTEXT;
    if (context)
    {
        ContextEGL *contextEGL = GetImplAs<ContextEGL>(context);
        newContext             = contextEGL->getContext();
    }

    if (currentContext.isExternalContext || (context && context->isExternal()))
    {
        ASSERT(currentContext.surface == EGL_NO_SURFACE);
        if (!currentContext.isExternalContext)
        {
            // Switch to an ANGLE external context.
            ASSERT(context);
            ASSERT(currentContext.context == EGL_NO_CONTEXT);
            currentContext.context           = newContext;
            currentContext.isExternalContext = true;

            // We only support using external surface with external context.
            ASSERT(GetImplAs<SurfaceEGL>(drawSurface)->isExternal());
            ASSERT(GetImplAs<SurfaceEGL>(drawSurface)->getSurface() == EGL_NO_SURFACE);
        }
        else if (context)
        {
            // Switch surface but not context.
            ASSERT(currentContext.context == newContext);
            ASSERT(newSurface == EGL_NO_SURFACE);
            ASSERT(newContext != EGL_NO_CONTEXT);
            // We only support using external surface with external context.
            ASSERT(GetImplAs<SurfaceEGL>(drawSurface)->isExternal());
            ASSERT(GetImplAs<SurfaceEGL>(drawSurface)->getSurface() == EGL_NO_SURFACE);
        }
        else
        {
            // Release the ANGLE external context.
            ASSERT(newSurface == EGL_NO_SURFACE);
            ASSERT(newContext == EGL_NO_CONTEXT);
            ASSERT(currentContext.context != EGL_NO_CONTEXT);
            currentContext.context           = EGL_NO_CONTEXT;
            currentContext.isExternalContext = false;
        }

        // Do not need to call eglMakeCurrent(), since we don't support swtiching EGLSurface for
        // external context.
        return DisplayGL::makeCurrent(display, drawSurface, readSurface, context);
    }

    // The context should never change when context virtualization is being used unless binding a
    // null context.
    if (mVirtualizedContexts && newContext != EGL_NO_CONTEXT)
    {
        ASSERT(currentContext.context == EGL_NO_CONTEXT || newContext == currentContext.context);

        newContext = mRenderer->getContext();

        // If we know that we're only running on one thread (mVirtualizedContexts == true) and
        // EGL_NO_SURFACE is going to be bound, we can optimize this case by not changing the
        // surface binding and emulate the surfaceless extension in the frontend.
        if (newSurface == EGL_NO_SURFACE)
        {
            newSurface = currentContext.surface;
        }

        // It's possible that no surface has been created yet and the driver doesn't support
        // surfaceless, bind the mock pbuffer.
        if (newSurface == EGL_NO_SURFACE && !mSupportsSurfaceless)
        {
            newSurface = mMockPbuffer;
            ASSERT(newSurface != EGL_NO_SURFACE);
        }
    }

    if (newSurface != currentContext.surface || newContext != currentContext.context)
    {
        if (mEGL->makeCurrent(newSurface, newContext) == EGL_FALSE)
        {
            return egl::Error(mEGL->getError(), "eglMakeCurrent failed");
        }
        currentContext.surface = newSurface;
        currentContext.context = newContext;
    }

    return DisplayGL::makeCurrent(display, drawSurface, readSurface, context);
}

void DisplayAndroid::destroyNativeContext(EGLContext context)
{
    DisplayEGL::destroyNativeContext(context);
}

void DisplayAndroid::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    DisplayEGL::generateExtensions(outExtensions);

    // Surfaceless can be support if the native driver supports it or we know that we are running on
    // a single thread (mVirtualizedContexts == true)
    outExtensions->surfacelessContext = mSupportsSurfaceless || mVirtualizedContexts;

    outExtensions->externalContextAndSurface = true;
}

egl::Error DisplayAndroid::createRenderer(EGLContext shareContext,
                                          bool makeNewContextCurrent,
                                          bool isExternalContext,
                                          std::shared_ptr<RendererEGL> *outRenderer)
{
    EGLContext context = EGL_NO_CONTEXT;
    native_egl::AttributeVector attribs;

    // If isExternalContext is true, the external context is current, so we don't need to make the
    // mMockPbuffer current.
    if (isExternalContext)
    {
        ASSERT(shareContext == EGL_NO_CONTEXT);
        ASSERT(!makeNewContextCurrent);
        // Should we consider creating a share context to avoid querying and restoring GL context
        // state?
        context = mEGL->getCurrentContext();
        ASSERT(context != EGL_NO_CONTEXT);
        // TODO: get the version from the current context.
        attribs = {EGL_CONTEXT_MAJOR_VERSION, 2, EGL_CONTEXT_MINOR_VERSION, 0, EGL_NONE};
    }
    else
    {
        ANGLE_TRY(initializeContext(shareContext, mDisplayAttributes, &context, &attribs));
        if (mEGL->makeCurrent(mMockPbuffer, context) == EGL_FALSE)
        {
            return egl::EglNotInitialized()
                   << "eglMakeCurrent failed with " << egl::Error(mEGL->getError());
        }
    }

    std::unique_ptr<FunctionsGL> functionsGL(mEGL->makeFunctionsGL());
    functionsGL->initialize(mDisplayAttributes);

    outRenderer->reset(new RendererEGL(std::move(functionsGL), mDisplayAttributes, this, context,
                                       attribs, isExternalContext));

    CurrentNativeContext &currentContext = mCurrentNativeContexts[std::this_thread::get_id()];
    if (makeNewContextCurrent)
    {
        currentContext.surface = mMockPbuffer;
        currentContext.context = context;
    }
    else if (!isExternalContext)
    {
        // Reset the current context back to the previous state
        if (mEGL->makeCurrent(currentContext.surface, currentContext.context) == EGL_FALSE)
        {
            return egl::EglNotInitialized()
                   << "eglMakeCurrent failed with " << egl::Error(mEGL->getError());
        }
    }

    return egl::NoError();
}

class WorkerContextAndroid final : public WorkerContext
{
  public:
    WorkerContextAndroid(EGLContext context, FunctionsEGL *functions, EGLSurface pbuffer);
    ~WorkerContextAndroid() override;

    bool makeCurrent() override;
    void unmakeCurrent() override;

  private:
    EGLContext mContext;
    FunctionsEGL *mFunctions;
    EGLSurface mPbuffer;
};

WorkerContextAndroid::WorkerContextAndroid(EGLContext context,
                                           FunctionsEGL *functions,
                                           EGLSurface pbuffer)
    : mContext(context), mFunctions(functions), mPbuffer(pbuffer)
{}

WorkerContextAndroid::~WorkerContextAndroid()
{
    mFunctions->destroyContext(mContext);
}

bool WorkerContextAndroid::makeCurrent()
{
    if (mFunctions->makeCurrent(mPbuffer, mContext) == EGL_FALSE)
    {
        ERR() << "Unable to make the EGL context current.";
        return false;
    }
    return true;
}

void WorkerContextAndroid::unmakeCurrent()
{
    mFunctions->makeCurrent(EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

WorkerContext *DisplayAndroid::createWorkerContext(std::string *infoLog,
                                                   EGLContext sharedContext,
                                                   const native_egl::AttributeVector workerAttribs)
{
    EGLContext context = mEGL->createContext(mConfig, sharedContext, workerAttribs.data());
    if (context == EGL_NO_CONTEXT)
    {
        *infoLog += "Unable to create the EGL context.";
        return nullptr;
    }
    return new WorkerContextAndroid(context, mEGL, mMockPbuffer);
}

}  // namespace rx

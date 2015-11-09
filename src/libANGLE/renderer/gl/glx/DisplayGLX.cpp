//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayGLX.cpp: GLX implementation of egl::Display

#include "libANGLE/renderer/gl/glx/DisplayGLX.h"

#include <EGL/eglext.h>
#include <algorithm>

#include "common/debug.h"
#include "libANGLE/Config.h"
#include "libANGLE/Display.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/gl/glx/PbufferSurfaceGLX.h"
#include "libANGLE/renderer/gl/glx/WindowSurfaceGLX.h"

namespace rx
{

static int IgnoreX11Errors(Display *, XErrorEvent *)
{
    return 0;
}

class FunctionsGLGLX : public FunctionsGL
{
  public:
    FunctionsGLGLX(PFNGETPROCPROC getProc)
      : mGetProc(getProc)
    {
    }

    virtual ~FunctionsGLGLX()
    {
    }

  private:
    void *loadProcAddress(const std::string &function) override
    {
        return reinterpret_cast<void*>(mGetProc(function.c_str()));
    }

    PFNGETPROCPROC mGetProc;
};

DisplayGLX::DisplayGLX()
    : DisplayGL(),
      mFunctionsGL(nullptr),
      mContextConfig(nullptr),
      mContext(nullptr),
      mDummyPbuffer(0),
      mUsesNewXDisplay(false),
      mIsMesa(false),
      mEGLDisplay(nullptr)
{
}

DisplayGLX::~DisplayGLX()
{
}

egl::Error DisplayGLX::initialize(egl::Display *display)
{
    mEGLDisplay = display;
    Display *xDisplay = display->getNativeDisplayId();

    // ANGLE_platform_angle allows the creation of a default display
    // using EGL_DEFAULT_DISPLAY (= nullptr). In this case just open
    // the display specified by the DISPLAY environment variable.
    if (xDisplay == EGL_DEFAULT_DISPLAY)
    {
        mUsesNewXDisplay = true;
        xDisplay = XOpenDisplay(NULL);
        if (!xDisplay)
        {
            return egl::Error(EGL_NOT_INITIALIZED, "Could not open the default X display.");
        }
    }

    std::string glxInitError;
    if (!mGLX.initialize(xDisplay, DefaultScreen(xDisplay), &glxInitError))
    {
        return egl::Error(EGL_NOT_INITIALIZED, glxInitError.c_str());
    }

    // Check we have the needed extensions
    {
        if (mGLX.minorVersion == 3 && !mGLX.hasExtension("GLX_ARB_multisample"))
        {
            return egl::Error(EGL_NOT_INITIALIZED, "GLX doesn't support ARB_multisample.");
        }
        // Require ARB_create_context which has been supported since Mesa 9 unconditionnaly
        // and is present in Mesa 8 in an almost always on compile flag. Also assume proprietary
        // drivers have it.
        if (!mGLX.hasExtension("GLX_ARB_create_context"))
        {
            return egl::Error(EGL_NOT_INITIALIZED, "GLX doesn't support ARB_create_context.");
        }
    }

    // When glXMakeCurrent is called, the context and the surface must be
    // compatible which in glX-speak means that their config have the same
    // color buffer type, are both RGBA or ColorIndex, and their buffers have
    // the same depth, if they exist.
    // Since our whole EGL implementation is backed by only one GL context, this
    // context must be compatible with all the GLXFBConfig corresponding to the
    // EGLconfigs that we will be exposing.
    {
        int nConfigs;
        int attribList[] =
        {
            // We want RGBA8 and DEPTH24_STENCIL8
            GLX_RED_SIZE, 8,
            GLX_GREEN_SIZE, 8,
            GLX_BLUE_SIZE, 8,
            GLX_ALPHA_SIZE, 8,
            GLX_DEPTH_SIZE, 24,
            GLX_STENCIL_SIZE, 8,
            // We want RGBA rendering (vs COLOR_INDEX) and doublebuffer
            GLX_RENDER_TYPE, GLX_RGBA_BIT,
            // Double buffer is not strictly required as a non-doublebuffer
            // context can work with a doublebuffered surface, but it still
            // flickers and all applications want doublebuffer anyway.
            GLX_DOUBLEBUFFER, True,
            // All of these must be supported for full EGL support
            GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT | GLX_PBUFFER_BIT | GLX_PIXMAP_BIT,
            // This makes sure the config have an associated visual Id
            GLX_X_RENDERABLE, True,
            GLX_CONFIG_CAVEAT, GLX_NONE,
            None
        };
        glx::FBConfig* candidates = mGLX.chooseFBConfig(attribList, &nConfigs);
        if (nConfigs == 0)
        {
            XFree(candidates);
            return egl::Error(EGL_NOT_INITIALIZED, "Could not find a decent GLX FBConfig to create the context.");
        }
        mContextConfig = candidates[0];
        XFree(candidates);
    }

    mContext = initializeContext(mContextConfig, display->getAttributeMap());
    if (!mContext)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not create GL context.");
    }

    // FunctionsGL and DisplayGL need to make a few GL calls, for example to
    // query the version of the context so we need to make the context current.
    // glXMakeCurrent requires a GLXDrawable so we create a temporary Pbuffer
    // (of size 1, 1) for the duration of these calls.
    // Ideally we would want to unset the current context and destroy the pbuffer
    // before going back to the application but this is TODO
    // We could use a pbuffer of size (0, 0) but it fails on the Intel Mesa driver
    // as commented on https://bugs.freedesktop.org/show_bug.cgi?id=38869 so we
    // use (1, 1) instead.

    int dummyPbufferAttribs[] =
    {
        GLX_PBUFFER_WIDTH, 1,
        GLX_PBUFFER_HEIGHT, 1,
        None,
    };
    mDummyPbuffer = mGLX.createPbuffer(mContextConfig, dummyPbufferAttribs);
    if (!mDummyPbuffer)
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not create the dummy pbuffer.");
    }

    if (!mGLX.makeCurrent(mDummyPbuffer, mContext))
    {
        return egl::Error(EGL_NOT_INITIALIZED, "Could not make the dummy pbuffer current.");
    }

    mFunctionsGL = new FunctionsGLGLX(mGLX.getProc);
    mFunctionsGL->initialize();

    syncXCommands();

    std::string rendererString =
        reinterpret_cast<const char*>(mFunctionsGL->getString(GL_RENDERER));
    mIsMesa = rendererString.find("Mesa") != std::string::npos;

    return DisplayGL::initialize(display);
}

void DisplayGLX::terminate()
{
    DisplayGL::terminate();

    if (mDummyPbuffer)
    {
        mGLX.destroyPbuffer(mDummyPbuffer);
        mDummyPbuffer = 0;
    }

    if (mContext)
    {
        mGLX.destroyContext(mContext);
        mContext = nullptr;
    }

    mGLX.terminate();

    SafeDelete(mFunctionsGL);
}

SurfaceImpl *DisplayGLX::createWindowSurface(const egl::Config *configuration,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    ASSERT(configIdToGLXConfig.count(configuration->configID) > 0);
    glx::FBConfig fbConfig = configIdToGLXConfig[configuration->configID];

    return new WindowSurfaceGLX(mGLX, this, this->getRenderer(), window, mGLX.getDisplay(),
                                mContext, fbConfig);
}

SurfaceImpl *DisplayGLX::createPbufferSurface(const egl::Config *configuration,
                                              const egl::AttributeMap &attribs)
{
    ASSERT(configIdToGLXConfig.count(configuration->configID) > 0);
    glx::FBConfig fbConfig = configIdToGLXConfig[configuration->configID];

    EGLint width = attribs.get(EGL_WIDTH, 0);
    EGLint height = attribs.get(EGL_HEIGHT, 0);
    bool largest = (attribs.get(EGL_LARGEST_PBUFFER, EGL_FALSE) == EGL_TRUE);

    return new PbufferSurfaceGLX(this->getRenderer(), width, height, largest, mGLX, mContext,
                                 fbConfig);
}

SurfaceImpl* DisplayGLX::createPbufferFromClientBuffer(const egl::Config *configuration,
                                                       EGLClientBuffer shareHandle,
                                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayGLX::createPixmapSurface(const egl::Config *configuration,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayGLX::getDevice(DeviceImpl **device)
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

glx::Context DisplayGLX::initializeContext(glx::FBConfig config,
                                           const egl::AttributeMap &eglAttributes)
{
    // Create a context of the requested version, if any.
    EGLint requestedMajorVersion =
        eglAttributes.get(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, EGL_DONT_CARE);
    EGLint requestedMinorVersion =
        eglAttributes.get(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, EGL_DONT_CARE);
    if (requestedMajorVersion != EGL_DONT_CARE && requestedMinorVersion != EGL_DONT_CARE)
    {
        std::vector<int> contextAttributes;
        contextAttributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
        contextAttributes.push_back(requestedMajorVersion);

        contextAttributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
        contextAttributes.push_back(requestedMinorVersion);

        contextAttributes.push_back(None);
        return createContextAttribs(config, contextAttributes);
    }

    // It is commonly assumed that glXCreateContextAttrib will create a context
    // of the highest version possible but it is not specified in the spec and
    // is not true on the Mesa drivers. Instead we try to create a context per
    // desktop GL version until we succeed, starting from newer version.
    // clang-format off
    const gl::Version desktopVersions[] = {
        gl::Version(4, 5),
        gl::Version(4, 4),
        gl::Version(4, 3),
        gl::Version(4, 2),
        gl::Version(4, 1),
        gl::Version(4, 0),
        gl::Version(3, 3),
        gl::Version(3, 2),
        gl::Version(3, 1),
        gl::Version(3, 0),
        gl::Version(2, 0),
        gl::Version(1, 5),
        gl::Version(1, 4),
        gl::Version(1, 3),
        gl::Version(1, 2),
        gl::Version(1, 1),
        gl::Version(1, 0),
    };
    // clang-format on

    bool useProfile = mGLX.hasExtension("GLX_ARB_create_context_profile");
    for (size_t i = 0; i < ArraySize(desktopVersions); ++i)
    {
        const auto &version = desktopVersions[i];

        std::vector<int> contextAttributes;
        contextAttributes.push_back(GLX_CONTEXT_MAJOR_VERSION_ARB);
        contextAttributes.push_back(version.major);

        contextAttributes.push_back(GLX_CONTEXT_MINOR_VERSION_ARB);
        contextAttributes.push_back(version.minor);

        if (useProfile && version >= gl::Version(3, 2))
        {
            contextAttributes.push_back(GLX_CONTEXT_PROFILE_MASK_ARB);
            contextAttributes.push_back(GLX_CONTEXT_CORE_PROFILE_BIT_ARB);
        }

        contextAttributes.push_back(None);
        auto context = createContextAttribs(config, contextAttributes);

        if (context)
        {
            return context;
        }
    }

    return nullptr;
}

egl::ConfigSet DisplayGLX::generateConfigs() const
{
    egl::ConfigSet configs;
    configIdToGLXConfig.clear();

    bool hasSwapControl = mGLX.hasExtension("GLX_EXT_swap_control");

    const gl::Version &maxVersion = getMaxSupportedESVersion();
    ASSERT(maxVersion >= gl::Version(2, 0));
    bool supportsES3 = maxVersion >= gl::Version(3, 0);

    int contextRedSize   = getGLXFBConfigAttrib(mContextConfig, GLX_RED_SIZE);
    int contextGreenSize = getGLXFBConfigAttrib(mContextConfig, GLX_GREEN_SIZE);
    int contextBlueSize  = getGLXFBConfigAttrib(mContextConfig, GLX_BLUE_SIZE);
    int contextAlphaSize = getGLXFBConfigAttrib(mContextConfig, GLX_ALPHA_SIZE);

    int contextDepthSize   = getGLXFBConfigAttrib(mContextConfig, GLX_DEPTH_SIZE);
    int contextStencilSize = getGLXFBConfigAttrib(mContextConfig, GLX_STENCIL_SIZE);

    int contextSamples = getGLXFBConfigAttrib(mContextConfig, GLX_SAMPLES);
    int contextSampleBuffers = getGLXFBConfigAttrib(mContextConfig, GLX_SAMPLE_BUFFERS);

    int contextAccumRedSize = getGLXFBConfigAttrib(mContextConfig, GLX_ACCUM_RED_SIZE);
    int contextAccumGreenSize = getGLXFBConfigAttrib(mContextConfig, GLX_ACCUM_GREEN_SIZE);
    int contextAccumBlueSize = getGLXFBConfigAttrib(mContextConfig, GLX_ACCUM_BLUE_SIZE);
    int contextAccumAlphaSize = getGLXFBConfigAttrib(mContextConfig, GLX_ACCUM_ALPHA_SIZE);

    int attribList[] =
    {
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_X_RENDERABLE, True,
        GLX_DOUBLEBUFFER, True,
        None,
    };

    int glxConfigCount;
    glx::FBConfig *glxConfigs = mGLX.chooseFBConfig(attribList, &glxConfigCount);

    for (int i = 0; i < glxConfigCount; i++)
    {
        glx::FBConfig glxConfig = glxConfigs[i];
        egl::Config config;

        // Native stuff
        config.nativeVisualID = getGLXFBConfigAttrib(glxConfig, GLX_VISUAL_ID);
        config.nativeVisualType = getGLXFBConfigAttrib(glxConfig, GLX_X_VISUAL_TYPE);
        config.nativeRenderable = EGL_TRUE;

        // Buffer sizes
        config.redSize = getGLXFBConfigAttrib(glxConfig, GLX_RED_SIZE);
        config.greenSize = getGLXFBConfigAttrib(glxConfig, GLX_GREEN_SIZE);
        config.blueSize = getGLXFBConfigAttrib(glxConfig, GLX_BLUE_SIZE);
        config.alphaSize = getGLXFBConfigAttrib(glxConfig, GLX_ALPHA_SIZE);
        config.depthSize = getGLXFBConfigAttrib(glxConfig, GLX_DEPTH_SIZE);
        config.stencilSize = getGLXFBConfigAttrib(glxConfig, GLX_STENCIL_SIZE);

        // We require RGBA8 and the D24S8 (or no DS buffer)
        if (config.redSize != contextRedSize || config.greenSize != contextGreenSize ||
            config.blueSize != contextBlueSize || config.alphaSize != contextAlphaSize)
        {
            continue;
        }
        // The GLX spec says that it is ok for a whole buffer to not be present
        // however the Mesa Intel driver (and probably on other Mesa drivers)
        // fails to make current when the Depth stencil doesn't exactly match the
        // configuration.
        bool hasSameDepthStencil =
            config.depthSize == contextDepthSize && config.stencilSize == contextStencilSize;
        bool hasNoDepthStencil = config.depthSize == 0 && config.stencilSize == 0;
        if (!hasSameDepthStencil && (mIsMesa || !hasNoDepthStencil))
        {
            continue;
        }

        config.colorBufferType = EGL_RGB_BUFFER;
        config.luminanceSize = 0;
        config.alphaMaskSize = 0;

        config.bufferSize = config.redSize + config.greenSize + config.blueSize + config.alphaSize;

        // Multisample and accumulation buffers
        int samples = getGLXFBConfigAttrib(glxConfig, GLX_SAMPLES);
        int sampleBuffers = getGLXFBConfigAttrib(glxConfig, GLX_SAMPLE_BUFFERS);

        int accumRedSize = getGLXFBConfigAttrib(glxConfig, GLX_ACCUM_RED_SIZE);
        int accumGreenSize = getGLXFBConfigAttrib(glxConfig, GLX_ACCUM_GREEN_SIZE);
        int accumBlueSize = getGLXFBConfigAttrib(glxConfig, GLX_ACCUM_BLUE_SIZE);
        int accumAlphaSize = getGLXFBConfigAttrib(glxConfig, GLX_ACCUM_ALPHA_SIZE);

        if (samples != contextSamples ||
            sampleBuffers != contextSampleBuffers ||
            accumRedSize != contextAccumRedSize ||
            accumGreenSize != contextAccumGreenSize ||
            accumBlueSize != contextAccumBlueSize ||
            accumAlphaSize != contextAccumAlphaSize)
        {
            continue;
        }

        config.samples = samples;
        config.sampleBuffers = sampleBuffers;

        // Transparency
        if (getGLXFBConfigAttrib(glxConfig, GLX_TRANSPARENT_TYPE) == GLX_TRANSPARENT_RGB)
        {
            config.transparentType = EGL_TRANSPARENT_RGB;
            config.transparentRedValue = getGLXFBConfigAttrib(glxConfig, GLX_TRANSPARENT_RED_VALUE);
            config.transparentGreenValue = getGLXFBConfigAttrib(glxConfig, GLX_TRANSPARENT_GREEN_VALUE);
            config.transparentBlueValue = getGLXFBConfigAttrib(glxConfig, GLX_TRANSPARENT_BLUE_VALUE);
        }
        else
        {
            config.transparentType = EGL_NONE;
        }

        // Pbuffer
        config.maxPBufferWidth = getGLXFBConfigAttrib(glxConfig, GLX_MAX_PBUFFER_WIDTH);
        config.maxPBufferHeight = getGLXFBConfigAttrib(glxConfig, GLX_MAX_PBUFFER_HEIGHT);
        config.maxPBufferPixels = getGLXFBConfigAttrib(glxConfig, GLX_MAX_PBUFFER_PIXELS);

        // Caveat
        config.configCaveat = EGL_NONE;

        int caveat = getGLXFBConfigAttrib(glxConfig, GLX_CONFIG_CAVEAT);
        if (caveat == GLX_SLOW_CONFIG)
        {
            config.configCaveat = EGL_SLOW_CONFIG;
        }
        else if (caveat == GLX_NON_CONFORMANT_CONFIG)
        {
            continue;
        }

        // Misc
        config.level = getGLXFBConfigAttrib(glxConfig, GLX_LEVEL);

        config.bindToTextureRGB = EGL_FALSE;
        config.bindToTextureRGBA = EGL_FALSE;

        int glxDrawable = getGLXFBConfigAttrib(glxConfig, GLX_DRAWABLE_TYPE);
        config.surfaceType = 0 |
            (glxDrawable & GLX_WINDOW_BIT ? EGL_WINDOW_BIT : 0) |
            (glxDrawable & GLX_PBUFFER_BIT ? EGL_PBUFFER_BIT : 0) |
            (glxDrawable & GLX_PIXMAP_BIT ? EGL_PIXMAP_BIT : 0);

        if (hasSwapControl)
        {
            // In GLX_EXT_swap_control querying these is done on a GLXWindow so we just set a default value.
            config.minSwapInterval = 0;
            config.maxSwapInterval = 4;
        }
        else
        {
            config.minSwapInterval = 1;
            config.maxSwapInterval = 1;
        }
        // TODO(cwallez) wildly guessing these formats, another TODO says they should be removed anyway
        config.renderTargetFormat = GL_RGBA8;
        config.depthStencilFormat = GL_DEPTH24_STENCIL8;

        config.conformant = EGL_OPENGL_ES2_BIT | (supportsES3 ? EGL_OPENGL_ES3_BIT_KHR : 0);
        config.renderableType = config.conformant;

        // TODO(cwallez) I have no idea what this is
        config.matchNativePixmap = EGL_NONE;

        int id = configs.add(config);
        configIdToGLXConfig[id] = glxConfig;
    }

    XFree(glxConfigs);

    return configs;
}

bool DisplayGLX::isDeviceLost() const
{
    // UNIMPLEMENTED();
    return false;
}

bool DisplayGLX::testDeviceLost()
{
    // UNIMPLEMENTED();
    return false;
}

egl::Error DisplayGLX::restoreLostDevice()
{
    UNIMPLEMENTED();
    return egl::Error(EGL_BAD_DISPLAY);
}

bool DisplayGLX::isValidNativeWindow(EGLNativeWindowType window) const
{
    // There is no function in Xlib to check the validity of a Window directly.
    // However a small number of functions used to obtain window information
    // return a status code (0 meaning failure) and guarantee that they will
    // fail if the window doesn't exist (the rational is that these function
    // are used by window managers). Out of these function we use XQueryTree
    // as it seems to be the simplest; a drawback is that it will allocate
    // memory for the list of children, becasue we use a child window for
    // WindowSurface.
    Window root;
    Window parent;
    Window *children = nullptr;
    unsigned nChildren;
    int status = XQueryTree(mGLX.getDisplay(), window, &root, &parent, &children, &nChildren);
    if (children)
    {
        XFree(children);
    }
    return status != 0;
}

std::string DisplayGLX::getVendorString() const
{
    // UNIMPLEMENTED();
    return "";
}

void DisplayGLX::syncXCommands() const
{
    if (mUsesNewXDisplay)
    {
        XSync(mGLX.getDisplay(), False);
    }
}

const FunctionsGL *DisplayGLX::getFunctionsGL() const
{
    return mFunctionsGL;
}

void DisplayGLX::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    outExtensions->createContext = true;
}

void DisplayGLX::generateCaps(egl::Caps *outCaps) const
{
    // UNIMPLEMENTED();
    outCaps->textureNPOT = true;
}

int DisplayGLX::getGLXFBConfigAttrib(glx::FBConfig config, int attrib) const
{
    int result;
    mGLX.getFBConfigAttrib(config, attrib, &result);
    return result;
}

glx::Context DisplayGLX::createContextAttribs(glx::FBConfig, const std::vector<int> &attribs) const
{
    // When creating a context with glXCreateContextAttribsARB, a variety of X11 errors can
    // be generated. To prevent these errors from crashing our process, we simply ignore
    // them and only look if GLXContext was created.
    auto oldErrorHandler = XSetErrorHandler(IgnoreX11Errors);
    auto context = mGLX.createContextAttribsARB(mContextConfig, nullptr, True, attribs.data());
    XSetErrorHandler(oldErrorHandler);

    return context;
}
}

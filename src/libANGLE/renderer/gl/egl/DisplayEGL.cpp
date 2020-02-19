//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// DisplayEGL.cpp: Common across EGL parts of platform specific egl::Display implementations

#include "libANGLE/renderer/gl/egl/DisplayEGL.h"

#include "libANGLE/Surface.h"
#include "libANGLE/renderer/gl/egl/ImageEGL.h"
#include "libANGLE/renderer/gl/egl/PbufferSurfaceEGL.h"
#include "libANGLE/renderer/gl/egl/RendererEGL.h"
#include "libANGLE/renderer/gl/egl/SyncEGL.h"
#include "libANGLE/renderer/gl/egl/WindowSurfaceEGL.h"

namespace rx
{

DisplayEGL::DisplayEGL(const egl::DisplayState &state)
    : DisplayGL(state), mRenderer(nullptr), mEGL(nullptr), mConfig(EGL_NO_CONFIG_KHR)
{}

DisplayEGL::~DisplayEGL() {}

ImageImpl *DisplayEGL::createImage(const egl::ImageState &state,
                                   const gl::Context *context,
                                   EGLenum target,
                                   const egl::AttributeMap &attribs)
{
    return new ImageEGL(state, context, target, attribs, mEGL);
}

EGLSyncImpl *DisplayEGL::createSync(const egl::AttributeMap &attribs)
{
    return new SyncEGL(attribs, mEGL);
}

std::string DisplayEGL::getVendorString() const
{
    const char *vendor = mEGL->queryString(EGL_VENDOR);
    ASSERT(vendor);
    return vendor;
}

egl::Error DisplayEGL::initializeContext(EGLContext shareContext,
                                         const egl::AttributeMap &eglAttributes,
                                         EGLContext *outContext,
                                         native_egl::AttributeVector *outAttribs) const
{
    gl::Version eglVersion(mEGL->majorVersion, mEGL->minorVersion);

    EGLint requestedMajor =
        eglAttributes.getAsInt(EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE, EGL_DONT_CARE);
    EGLint requestedMinor =
        eglAttributes.getAsInt(EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE, EGL_DONT_CARE);
    bool initializeRequested = requestedMajor != EGL_DONT_CARE && requestedMinor != EGL_DONT_CARE;

    static_assert(EGL_CONTEXT_MAJOR_VERSION == EGL_CONTEXT_MAJOR_VERSION_KHR,
                  "Major Version define should match");
    static_assert(EGL_CONTEXT_MINOR_VERSION == EGL_CONTEXT_MINOR_VERSION_KHR,
                  "Minor Version define should match");

    std::vector<native_egl::AttributeVector> contextAttribLists;
    if (eglVersion >= gl::Version(1, 5) || mEGL->hasExtension("EGL_KHR_create_context"))
    {
        if (initializeRequested)
        {
            contextAttribLists.push_back({EGL_CONTEXT_MAJOR_VERSION, requestedMajor,
                                          EGL_CONTEXT_MINOR_VERSION, requestedMinor, EGL_NONE});
        }
        else
        {
            // clang-format off
            const gl::Version esVersionsFrom2_0[] = {
                gl::Version(3, 2),
                gl::Version(3, 1),
                gl::Version(3, 0),
                gl::Version(2, 0),
            };
            // clang-format on

            for (const auto &version : esVersionsFrom2_0)
            {
                contextAttribLists.push_back(
                    {EGL_CONTEXT_MAJOR_VERSION, static_cast<EGLint>(version.major),
                     EGL_CONTEXT_MINOR_VERSION, static_cast<EGLint>(version.minor), EGL_NONE});
            }
        }
    }
    else
    {
        if (initializeRequested && (requestedMajor != 2 || requestedMinor != 0))
        {
            return egl::EglBadAttribute() << "Unsupported requested context version";
        }
        contextAttribLists.push_back({EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE});
    }

    EGLContext context = EGL_NO_CONTEXT;
    for (const auto &attribList : contextAttribLists)
    {
        context = mEGL->createContext(mConfig, shareContext, attribList.data());
        if (context != EGL_NO_CONTEXT)
        {
            *outContext = context;
            *outAttribs = attribList;
            return egl::NoError();
        }
    }

    return egl::Error(mEGL->getError(), "eglCreateContext failed");
}

SurfaceImpl *DisplayEGL::createWindowSurface(const egl::SurfaceState &state,
                                             EGLNativeWindowType window,
                                             const egl::AttributeMap &attribs)
{
    EGLConfig config;
    EGLint numConfig;
    EGLBoolean success;

    const EGLint configAttribList[] = {EGL_CONFIG_ID, mConfigIds[state.config->configID], EGL_NONE};
    success                         = mEGL->chooseConfig(configAttribList, &config, 1, &numConfig);
    ASSERT(success && numConfig == 1);

    return new WindowSurfaceEGL(state, mEGL, config, window);
}

SurfaceImpl *DisplayEGL::createPbufferSurface(const egl::SurfaceState &state,
                                              const egl::AttributeMap &attribs)
{
    EGLConfig config;
    EGLint numConfig;
    EGLBoolean success;

    const EGLint configAttribList[] = {EGL_CONFIG_ID, mConfigIds[state.config->configID], EGL_NONE};
    success                         = mEGL->chooseConfig(configAttribList, &config, 1, &numConfig);
    ASSERT(success && numConfig == 1);

    return new PbufferSurfaceEGL(state, mEGL, config);
}

SurfaceImpl *DisplayEGL::createPbufferFromClientBuffer(const egl::SurfaceState &state,
                                                       EGLenum buftype,
                                                       EGLClientBuffer clientBuffer,
                                                       const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

SurfaceImpl *DisplayEGL::createPixmapSurface(const egl::SurfaceState &state,
                                             NativePixmapType nativePixmap,
                                             const egl::AttributeMap &attribs)
{
    UNIMPLEMENTED();
    return nullptr;
}

template <typename T>
void DisplayEGL::getConfigAttrib(EGLConfig config, EGLint attribute, T *value) const
{
    EGLint tmp;
    EGLBoolean success = mEGL->getConfigAttrib(config, attribute, &tmp);
    ASSERT(success == EGL_TRUE);
    *value = tmp;
}

template <typename T, typename U>
void DisplayEGL::getConfigAttribIfExtension(EGLConfig config,
                                            EGLint attribute,
                                            T *value,
                                            const char *extension,
                                            const U &defaultValue) const
{
    if (mEGL->hasExtension(extension))
    {
        getConfigAttrib(config, attribute, value);
    }
    else
    {
        *value = static_cast<T>(defaultValue);
    }
}

egl::ConfigSet DisplayEGL::generateConfigs()
{
    egl::ConfigSet configSet;
    mConfigIds.clear();

    EGLint numConfigs;
    EGLBoolean success = mEGL->chooseConfig(mConfigAttribList.data(), nullptr, 0, &numConfigs);
    ASSERT(success == EGL_TRUE && numConfigs > 0);

    std::vector<EGLConfig> configs(numConfigs);
    EGLint numConfigs2;
    success =
        mEGL->chooseConfig(mConfigAttribList.data(), configs.data(), numConfigs, &numConfigs2);
    ASSERT(success == EGL_TRUE && numConfigs2 == numConfigs);

    for (int i = 0; i < numConfigs; i++)
    {
        egl::Config config;

        getConfigAttrib(configs[i], EGL_BUFFER_SIZE, &config.bufferSize);
        getConfigAttrib(configs[i], EGL_RED_SIZE, &config.redSize);
        getConfigAttrib(configs[i], EGL_GREEN_SIZE, &config.greenSize);
        getConfigAttrib(configs[i], EGL_BLUE_SIZE, &config.blueSize);
        getConfigAttrib(configs[i], EGL_LUMINANCE_SIZE, &config.luminanceSize);
        getConfigAttrib(configs[i], EGL_ALPHA_SIZE, &config.alphaSize);
        getConfigAttrib(configs[i], EGL_ALPHA_MASK_SIZE, &config.alphaMaskSize);
        getConfigAttrib(configs[i], EGL_BIND_TO_TEXTURE_RGB, &config.bindToTextureRGB);
        getConfigAttrib(configs[i], EGL_BIND_TO_TEXTURE_RGBA, &config.bindToTextureRGBA);
        getConfigAttrib(configs[i], EGL_COLOR_BUFFER_TYPE, &config.colorBufferType);
        getConfigAttrib(configs[i], EGL_CONFIG_CAVEAT, &config.configCaveat);
        getConfigAttrib(configs[i], EGL_CONFIG_ID, &config.configID);
        getConfigAttrib(configs[i], EGL_CONFORMANT, &config.conformant);
        getConfigAttrib(configs[i], EGL_DEPTH_SIZE, &config.depthSize);
        getConfigAttrib(configs[i], EGL_LEVEL, &config.level);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_WIDTH, &config.maxPBufferWidth);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_HEIGHT, &config.maxPBufferHeight);
        getConfigAttrib(configs[i], EGL_MAX_PBUFFER_PIXELS, &config.maxPBufferPixels);
        getConfigAttrib(configs[i], EGL_MAX_SWAP_INTERVAL, &config.maxSwapInterval);
        getConfigAttrib(configs[i], EGL_MIN_SWAP_INTERVAL, &config.minSwapInterval);
        getConfigAttrib(configs[i], EGL_NATIVE_RENDERABLE, &config.nativeRenderable);
        getConfigAttrib(configs[i], EGL_NATIVE_VISUAL_ID, &config.nativeVisualID);
        getConfigAttrib(configs[i], EGL_NATIVE_VISUAL_TYPE, &config.nativeVisualType);
        getConfigAttrib(configs[i], EGL_RENDERABLE_TYPE, &config.renderableType);
        getConfigAttrib(configs[i], EGL_SAMPLE_BUFFERS, &config.sampleBuffers);
        getConfigAttrib(configs[i], EGL_SAMPLES, &config.samples);
        getConfigAttrib(configs[i], EGL_STENCIL_SIZE, &config.stencilSize);
        getConfigAttrib(configs[i], EGL_SURFACE_TYPE, &config.surfaceType);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_TYPE, &config.transparentType);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_RED_VALUE, &config.transparentRedValue);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_GREEN_VALUE, &config.transparentGreenValue);
        getConfigAttrib(configs[i], EGL_TRANSPARENT_BLUE_VALUE, &config.transparentBlueValue);
        getConfigAttribIfExtension(configs[i], EGL_COLOR_COMPONENT_TYPE_EXT,
                                   &config.colorComponentType, "EGL_EXT_pixel_format_float",
                                   EGL_COLOR_COMPONENT_TYPE_FIXED_EXT);

        if (config.colorBufferType == EGL_RGB_BUFFER)
        {
            ASSERT(config.colorComponentType == EGL_COLOR_COMPONENT_TYPE_FIXED_EXT);
            if (config.redSize == 8 && config.greenSize == 8 && config.blueSize == 8 &&
                config.alphaSize == 8)
            {
                config.renderTargetFormat = GL_RGBA8;
            }
            else if (config.redSize == 8 && config.greenSize == 8 && config.blueSize == 8 &&
                     config.alphaSize == 0)
            {
                config.renderTargetFormat = GL_RGB8;
            }
            else if (config.redSize == 5 && config.greenSize == 6 && config.blueSize == 5 &&
                     config.alphaSize == 0)
            {
                config.renderTargetFormat = GL_RGB565;
            }
            else if (config.redSize == 5 && config.greenSize == 5 && config.blueSize == 5 &&
                     config.alphaSize == 1)
            {
                config.renderTargetFormat = GL_RGB5_A1;
            }
            else if (config.redSize == 4 && config.greenSize == 4 && config.blueSize == 4 &&
                     config.alphaSize == 4)
            {
                config.renderTargetFormat = GL_RGBA4;
            }
            else
            {
                ERR() << "RGBA(" << config.redSize << "," << config.greenSize << ","
                      << config.blueSize << "," << config.alphaSize << ") not handled";
                UNREACHABLE();
            }
        }
        else
        {
            UNREACHABLE();
        }

        if (config.depthSize == 0 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_ZERO;
        }
        else if (config.depthSize == 16 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_DEPTH_COMPONENT16;
        }
        else if (config.depthSize == 24 && config.stencilSize == 0)
        {
            config.depthStencilFormat = GL_DEPTH_COMPONENT24;
        }
        else if (config.depthSize == 24 && config.stencilSize == 8)
        {
            config.depthStencilFormat = GL_DEPTH24_STENCIL8;
        }
        else if (config.depthSize == 0 && config.stencilSize == 8)
        {
            config.depthStencilFormat = GL_STENCIL_INDEX8;
        }
        else
        {
            UNREACHABLE();
        }

        config.matchNativePixmap  = EGL_NONE;
        config.optimalOrientation = 0;

        int internalId         = configSet.add(config);
        mConfigIds[internalId] = config.configID;
    }

    return configSet;
}

bool DisplayEGL::testDeviceLost()
{
    return false;
}

egl::Error DisplayEGL::restoreLostDevice(const egl::Display *display)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

bool DisplayEGL::isValidNativeWindow(EGLNativeWindowType window) const
{
    return true;
}

DeviceImpl *DisplayEGL::createDevice()
{
    UNIMPLEMENTED();
    return nullptr;
}

egl::Error DisplayEGL::waitClient(const gl::Context *context)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

egl::Error DisplayEGL::waitNative(const gl::Context *context, EGLint engine)
{
    UNIMPLEMENTED();
    return egl::NoError();
}

gl::Version DisplayEGL::getMaxSupportedESVersion() const
{
    return mRenderer->getMaxSupportedESVersion();
}

void DisplayEGL::destroyNativeContext(EGLContext context)
{
    mEGL->destroyContext(context);
}

void DisplayEGL::generateExtensions(egl::DisplayExtensions *outExtensions) const
{
    gl::Version eglVersion(mEGL->majorVersion, mEGL->minorVersion);

    outExtensions->createContextRobustness =
        mEGL->hasExtension("EGL_EXT_create_context_robustness");

    outExtensions->postSubBuffer    = false;  // Since SurfaceEGL::postSubBuffer is not implemented
    outExtensions->presentationTime = mEGL->hasExtension("EGL_ANDROID_presentation_time");

    // Contexts are virtualized so textures can be shared globally
    outExtensions->displayTextureShareGroup = true;

    // We will fallback to regular swap if swapBuffersWithDamage isn't
    // supported, so indicate support here to keep validation happy.
    outExtensions->swapBuffersWithDamage = true;

    outExtensions->image     = mEGL->hasExtension("EGL_KHR_image");
    outExtensions->imageBase = mEGL->hasExtension("EGL_KHR_image_base");
    // Pixmaps are not supported in ANGLE's EGL implementation.
    // outExtensions->imagePixmap = mEGL->hasExtension("EGL_KHR_image_pixmap");
    outExtensions->glTexture2DImage      = mEGL->hasExtension("EGL_KHR_gl_texture_2D_image");
    outExtensions->glTextureCubemapImage = mEGL->hasExtension("EGL_KHR_gl_texture_cubemap_image");
    outExtensions->glTexture3DImage      = mEGL->hasExtension("EGL_KHR_gl_texture_3D_image");
    outExtensions->glRenderbufferImage   = mEGL->hasExtension("EGL_KHR_gl_renderbuffer_image");
    outExtensions->pixelFormatFloat      = mEGL->hasExtension("EGL_EXT_pixel_format_float");

    outExtensions->glColorspace = mEGL->hasExtension("EGL_KHR_gl_colorspace");
    if (outExtensions->glColorspace)
    {
        outExtensions->glColorspaceDisplayP3Linear =
            mEGL->hasExtension("EGL_EXT_gl_colorspace_display_p3_linear");
        outExtensions->glColorspaceDisplayP3 =
            mEGL->hasExtension("EGL_EXT_gl_colorspace_display_p3");
        outExtensions->glColorspaceScrgb = mEGL->hasExtension("EGL_EXT_gl_colorspace_scrgb");
        outExtensions->glColorspaceScrgbLinear =
            mEGL->hasExtension("EGL_EXT_gl_colorspace_scrgb_linear");
        outExtensions->glColorspaceDisplayP3Passthrough =
            mEGL->hasExtension("EGL_EXT_gl_colorspace_display_p3_passthrough");
    }

    outExtensions->imageNativeBuffer = mEGL->hasExtension("EGL_ANDROID_image_native_buffer");

    outExtensions->getFrameTimestamps = mEGL->hasExtension("EGL_ANDROID_get_frame_timestamps");

    outExtensions->fenceSync =
        eglVersion >= gl::Version(1, 5) || mEGL->hasExtension("EGL_KHR_fence_sync");
    outExtensions->waitSync =
        eglVersion >= gl::Version(1, 5) || mEGL->hasExtension("EGL_KHR_wait_sync");

    outExtensions->getNativeClientBufferANDROID =
        mEGL->hasExtension("EGL_ANDROID_get_native_client_buffer");

    outExtensions->nativeFenceSyncANDROID = mEGL->hasExtension("EGL_ANDROID_native_fence_sync");

    outExtensions->noConfigContext = mEGL->hasExtension("EGL_KHR_no_config_context");

    outExtensions->framebufferTargetANDROID = mEGL->hasExtension("EGL_ANDROID_framebuffer_target");

    DisplayGL::generateExtensions(outExtensions);
}

void DisplayEGL::generateCaps(egl::Caps *outCaps) const
{
    outCaps->textureNPOT = true;  // Since we request GLES >= 2
}

void DisplayEGL::setBlobCacheFuncs(EGLSetBlobFuncANDROID set, EGLGetBlobFuncANDROID get)
{
    if (mEGL->hasExtension("EGL_ANDROID_blob_cache"))
    {
        mEGL->setBlobCacheFuncsANDROID(set, get);
    }
}

egl::Error DisplayEGL::makeCurrentSurfaceless(gl::Context *context)
{
    // Nothing to do because EGL always uses the same context and the previous surface can be left
    // current.
    return egl::NoError();
}

void DisplayEGL::initializeFrontendFeatures(angle::FrontendFeatures *features) const
{
    mRenderer->initializeFrontendFeatures(features);
}

void DisplayEGL::populateFeatureList(angle::FeatureList *features)
{
    mRenderer->getFeatures().populateFeatureList(features);
}

}  // namespace rx

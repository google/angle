//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Display.h: Defines the egl::Display class, representing the abstract
// display on which graphics are drawn. Implements EGLDisplay.
// [EGL 1.4] section 2.1.2 page 3.

#ifndef LIBANGLE_DISPLAY_H_
#define LIBANGLE_DISPLAY_H_

#include <set>
#include <vector>

#include "libANGLE/AttributeMap.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Config.h"
#include "libANGLE/Error.h"
#include "libANGLE/LoggingAnnotator.h"
#include "libANGLE/Version.h"

namespace gl
{
class Context;
class TextureManager;
}

namespace rx
{
class DisplayImpl;
}

namespace egl
{
class Device;
class Image;
class Surface;
class Stream;
class Thread;

using SurfaceSet = std::set<Surface *>;

struct DisplayState final : private angle::NonCopyable
{
    SurfaceSet surfaceSet;
};

class Display final : angle::NonCopyable
{
  public:
    ~Display();

    Error initialize();
    void terminate();

    static Display *GetDisplayFromDevice(Device *device, const AttributeMap &attribMap);
    static Display *GetDisplayFromNativeDisplay(EGLNativeDisplayType nativeDisplay,
                                                const AttributeMap &attribMap);

    static const ClientExtensions &GetClientExtensions();
    static const std::string &GetClientExtensionString();

    std::vector<const Config *> getConfigs(const AttributeMap &attribs) const;

    Error createWindowSurface(const Config *configuration,
                              EGLNativeWindowType window,
                              const AttributeMap &attribs,
                              Surface **outSurface);
    Error createPbufferSurface(const Config *configuration,
                               const AttributeMap &attribs,
                               Surface **outSurface);
    Error createPbufferFromClientBuffer(const Config *configuration,
                                        EGLenum buftype,
                                        EGLClientBuffer clientBuffer,
                                        const AttributeMap &attribs,
                                        Surface **outSurface);
    Error createPixmapSurface(const Config *configuration,
                              NativePixmapType nativePixmap,
                              const AttributeMap &attribs,
                              Surface **outSurface);

    Error createImage(const gl::Context *context,
                      EGLenum target,
                      EGLClientBuffer buffer,
                      const AttributeMap &attribs,
                      Image **outImage);

    Error createStream(const AttributeMap &attribs, Stream **outStream);

    Error createContext(const Config *configuration,
                        gl::Context *shareContext,
                        const AttributeMap &attribs,
                        gl::Context **outContext);

    Error makeCurrent(Surface *drawSurface, Surface *readSurface, gl::Context *context);

    void destroySurface(Surface *surface);
    void destroyImage(Image *image);
    void destroyStream(Stream *stream);
    void destroyContext(gl::Context *context);

    bool isInitialized() const;
    bool isValidConfig(const Config *config) const;
    bool isValidContext(const gl::Context *context) const;
    bool isValidSurface(const Surface *surface) const;
    bool isValidImage(const Image *image) const;
    bool isValidStream(const Stream *stream) const;
    bool isValidNativeWindow(EGLNativeWindowType window) const;

    Error validateClientBuffer(const Config *configuration,
                               EGLenum buftype,
                               EGLClientBuffer clientBuffer,
                               const AttributeMap &attribs);

    static bool isValidDisplay(const Display *display);
    static bool isValidNativeDisplay(EGLNativeDisplayType display);
    static bool hasExistingWindowSurface(EGLNativeWindowType window);

    bool isDeviceLost() const;
    bool testDeviceLost();
    void notifyDeviceLost();

    Error waitClient(const gl::Context *context) const;
    Error waitNative(const gl::Context *context, EGLint engine) const;

    const Caps &getCaps() const;

    const DisplayExtensions &getExtensions() const;
    const std::string &getExtensionString() const;
    const std::string &getVendorString() const;

    const AttributeMap &getAttributeMap() const { return mAttributeMap; }
    EGLNativeDisplayType getNativeDisplayId() const { return mDisplayId; }

    rx::DisplayImpl *getImplementation() const { return mImplementation; }
    Device *getDevice() const;
    EGLenum getPlatform() const { return mPlatform; }

    gl::Version getMaxSupportedESVersion() const;

    const DisplayState &getState() const { return mState; }

    bool isRobustResourceInitEnabled() const;

  private:
    Display(EGLenum platform, EGLNativeDisplayType displayId, Device *eglDevice);

    void setAttributes(rx::DisplayImpl *impl, const AttributeMap &attribMap);

    Error restoreLostDevice();

    void initDisplayExtensions();
    void initVendorString();

    DisplayState mState;
    rx::DisplayImpl *mImplementation;

    EGLNativeDisplayType mDisplayId;
    AttributeMap mAttributeMap;

    ConfigSet mConfigSet;

    typedef std::set<gl::Context*> ContextSet;
    ContextSet mContextSet;

    typedef std::set<Image *> ImageSet;
    ImageSet mImageSet;

    typedef std::set<Stream *> StreamSet;
    StreamSet mStreamSet;

    bool mInitialized;
    bool mDeviceLost;

    Caps mCaps;

    DisplayExtensions mDisplayExtensions;
    std::string mDisplayExtensionString;

    std::string mVendorString;

    Device *mDevice;
    EGLenum mPlatform;
    angle::LoggingAnnotator mAnnotator;

    gl::TextureManager *mTextureManager;
    size_t mGlobalTextureShareGroupUsers;
};

}  // namespace egl

#endif   // LIBANGLE_DISPLAY_H_

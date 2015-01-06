//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Config.h: Defines the egl::Config class, describing the format, type
// and size for an egl::Surface. Implements EGLConfig and related functionality.
// [EGL 1.5] section 3.4 page 19.

#ifndef INCLUDE_CONFIG_H_
#define INCLUDE_CONFIG_H_

#include "libANGLE/renderer/Renderer.h"

#include "common/angleutils.h"

#include <EGL/egl.h>

#include <map>
#include <vector>

namespace egl
{
class Display;

class Config
{
  public:
    Config();
    Config(rx::ConfigDesc desc, EGLint minSwapInterval, EGLint maxSwapInterval, EGLint texWidth, EGLint texHeight);

    GLenum mRenderTargetFormat;
    GLenum mDepthStencilFormat;
    GLint mMultiSample;

    EGLint mBufferSize;              // Depth of the color buffer
    EGLint mRedSize;                 // Bits of Red in the color buffer
    EGLint mGreenSize;               // Bits of Green in the color buffer
    EGLint mBlueSize;                // Bits of Blue in the color buffer
    EGLint mLuminanceSize;           // Bits of Luminance in the color buffer
    EGLint mAlphaSize;               // Bits of Alpha in the color buffer
    EGLint mAlphaMaskSize;           // Bits of Alpha Mask in the mask buffer
    EGLBoolean mBindToTextureRGB;    // True if bindable to RGB textures.
    EGLBoolean mBindToTextureRGBA;   // True if bindable to RGBA textures.
    EGLenum mColorBufferType;        // Color buffer type
    EGLenum mConfigCaveat;           // Any caveats for the configuration
    EGLint mConfigID;                // Unique EGLConfig identifier
    EGLint mConformant;              // Whether contexts created with this config are conformant
    EGLint mDepthSize;               // Bits of Z in the depth buffer
    EGLint mLevel;                   // Frame buffer level
    EGLBoolean mMatchNativePixmap;   // Match the native pixmap format
    EGLint mMaxPBufferWidth;         // Maximum width of pbuffer
    EGLint mMaxPBufferHeight;        // Maximum height of pbuffer
    EGLint mMaxPBufferPixels;        // Maximum size of pbuffer
    EGLint mMaxSwapInterval;         // Maximum swap interval
    EGLint mMinSwapInterval;         // Minimum swap interval
    EGLBoolean mNativeRenderable;    // EGL_TRUE if native rendering APIs can render to surface
    EGLint mNativeVisualID;          // Handle of corresponding native visual
    EGLint mNativeVisualType;        // Native visual type of the associated visual
    EGLint mRenderableType;          // Which client rendering APIs are supported.
    EGLint mSampleBuffers;           // Number of multisample buffers
    EGLint mSamples;                 // Number of samples per pixel
    EGLint mStencilSize;             // Bits of Stencil in the stencil buffer
    EGLint mSurfaceType;             // Which types of EGL surfaces are supported.
    EGLenum mTransparentType;        // Type of transparency supported
    EGLint mTransparentRedValue;     // Transparent red value
    EGLint mTransparentGreenValue;   // Transparent green value
    EGLint mTransparentBlueValue;    // Transparent blue value
};

class ConfigSet
{
  public:
    EGLint add(const Config &config);
    const Config &get(EGLint id) const;

    void clear();

    size_t size() const;

    bool contains(const Config *config) const;

    // Filter configurations based on the table in [EGL 1.5] section 3.4.1.2 page 29
    std::vector<const Config*> filter(const AttributeMap &attributeMap) const;

  private:
    typedef std::map<EGLint, const Config> ConfigMap;
    ConfigMap mConfigs;
};

}

#endif   // INCLUDE_CONFIG_H_

//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Config.cpp: Implements the egl::Config class, describing the format, type
// and size for an egl::Surface. Implements EGLConfig and related functionality.
// [EGL 1.5] section 3.4 page 19.

#include "libANGLE/Config.h"
#include "libANGLE/AttributeMap.h"

#include <algorithm>
#include <vector>

#include "angle_gl.h"
#include <EGL/eglext.h>

#include "common/debug.h"

namespace egl
{

Config::Config()
{
}

Config::Config(rx::ConfigDesc desc, EGLint minInterval, EGLint maxInterval, EGLint texWidth, EGLint texHeight)
    : mRenderTargetFormat(desc.renderTargetFormat), mDepthStencilFormat(desc.depthStencilFormat), mMultiSample(desc.multiSample)
{
    mBindToTextureRGB = EGL_FALSE;
    mBindToTextureRGBA = EGL_FALSE;
    switch (desc.renderTargetFormat)
    {
      case GL_RGB5_A1:
        mBufferSize = 16;
        mRedSize = 5;
        mGreenSize = 5;
        mBlueSize = 5;
        mAlphaSize = 1;
        break;
      case GL_BGR5_A1_ANGLEX:
        mBufferSize = 16;
        mRedSize = 5;
        mGreenSize = 5;
        mBlueSize = 5;
        mAlphaSize = 1;
        break;
      case GL_RGBA8_OES:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 8;
        mBindToTextureRGBA = true;
        break;
      case GL_RGB565:
        mBufferSize = 16;
        mRedSize = 5;
        mGreenSize = 6;
        mBlueSize = 5;
        mAlphaSize = 0;
        break;
      case GL_RGB8_OES:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 0;
        mBindToTextureRGB = true;
        break;
      case GL_BGRA8_EXT:
        mBufferSize = 32;
        mRedSize = 8;
        mGreenSize = 8;
        mBlueSize = 8;
        mAlphaSize = 8;
        mBindToTextureRGBA = true;
        break;
      default:
        UNREACHABLE();   // Other formats should not be valid
    }

    mLuminanceSize = 0;
    mAlphaMaskSize = 0;
    mColorBufferType = EGL_RGB_BUFFER;
    mConfigCaveat = (desc.fastConfig) ? EGL_NONE : EGL_SLOW_CONFIG;
    mConfigID = 0;
    mConformant = 0;

    switch (desc.depthStencilFormat)
    {
      case GL_NONE:
        mDepthSize = 0;
        mStencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT32_OES:
        mDepthSize = 32;
        mStencilSize = 0;
        break;
      case GL_DEPTH24_STENCIL8_OES:
        mDepthSize = 24;
        mStencilSize = 8;
        break;
      case GL_DEPTH_COMPONENT24_OES:
        mDepthSize = 24;
        mStencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT16:
        mDepthSize = 16;
        mStencilSize = 0;
        break;
      default:
        UNREACHABLE();
    }

    mLevel = 0;
    mMatchNativePixmap = EGL_NONE;
    mMaxPBufferWidth = texWidth;
    mMaxPBufferHeight = texHeight;
    mMaxPBufferPixels = texWidth*texHeight;
    mMaxSwapInterval = maxInterval;
    mMinSwapInterval = minInterval;
    mNativeRenderable = EGL_FALSE;
    mNativeVisualID = 0;
    mNativeVisualType = 0;
    mRenderableType = EGL_OPENGL_ES2_BIT;
    mSampleBuffers = desc.multiSample ? 1 : 0;
    mSamples = desc.multiSample;
    mSurfaceType = EGL_PBUFFER_BIT | EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
    mTransparentType = EGL_NONE;
    mTransparentRedValue = 0;
    mTransparentGreenValue = 0;
    mTransparentBlueValue = 0;

    if (desc.es2Conformant)
    {
        mConformant = EGL_OPENGL_ES2_BIT;
    }

    if (desc.es3Capable)
    {
        mRenderableType |= EGL_OPENGL_ES3_BIT_KHR;
        mConformant |= EGL_OPENGL_ES3_BIT_KHR;
    }
}

EGLint ConfigSet::add(const Config &config)
{
    // Set the config's ID to a small number that starts at 1 ([EGL 1.5] section 3.4)
    EGLint id = mConfigs.size() + 1;

    Config copyConfig(config);
    copyConfig.mConfigID = id;
    mConfigs.insert(std::make_pair(id, copyConfig));

    return id;
}

const Config &ConfigSet::get(EGLint id) const
{
    return mConfigs.at(id);
}

void ConfigSet::clear()
{
    mConfigs.clear();
}

size_t ConfigSet::size() const
{
    return mConfigs.size();
}

bool ConfigSet::contains(const Config *config) const
{
    for (auto i = mConfigs.begin(); i != mConfigs.end(); i++)
    {
        const Config &item = i->second;
        if (config == &item)
        {
            return true;
        }
    }

    return false;
}

// Function object used by STL sorting routines for ordering Configs according to [EGL 1.5] section 3.4.1.2 page 28.
class ConfigSorter
{
  public:
    explicit ConfigSorter(const AttributeMap &attributeMap)
    {
        scanForWantedComponents(attributeMap);
    }

    bool operator()(const Config *x, const Config *y) const
    {
        return (*this)(*x, *y);
    }

    bool operator()(const Config &x, const Config &y) const
    {
        #define SORT(attribute)                        \
            if (x.attribute != y.attribute)            \
            {                                          \
                return x.attribute < y.attribute;      \
            }

        META_ASSERT(EGL_NONE < EGL_SLOW_CONFIG && EGL_SLOW_CONFIG < EGL_NON_CONFORMANT_CONFIG);
        SORT(mConfigCaveat);

        META_ASSERT(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER);
        SORT(mColorBufferType);

        // By larger total number of color bits, only considering those that are requested to be > 0.
        EGLint xComponentsSize = wantedComponentsSize(x);
        EGLint yComponentsSize = wantedComponentsSize(y);
        if (xComponentsSize != yComponentsSize)
        {
            return xComponentsSize > yComponentsSize;
        }

        SORT(mBufferSize);
        SORT(mSampleBuffers);
        SORT(mSamples);
        SORT(mDepthSize);
        SORT(mStencilSize);
        SORT(mAlphaMaskSize);
        SORT(mNativeVisualType);
        SORT(mConfigID);

        #undef SORT

        return false;
    }

  private:
    void scanForWantedComponents(const AttributeMap &attributeMap)
    {
        // [EGL 1.5] section 3.4.1.2 page 30
        // Sorting rule #3: by larger total number of color bits, not considering
        // components that are 0 or don't-care.
        for (auto attribIter = attributeMap.begin(); attribIter != attributeMap.end(); attribIter++)
        {
            EGLint attributeKey = attribIter->first;
            EGLint attributeValue = attribIter->second;
            if (attributeKey != 0 && attributeValue != EGL_DONT_CARE)
            {
                switch (attributeKey)
                {
                case EGL_RED_SIZE:       mWantRed = true; break;
                case EGL_GREEN_SIZE:     mWantGreen = true; break;
                case EGL_BLUE_SIZE:      mWantBlue = true; break;
                case EGL_ALPHA_SIZE:     mWantAlpha = true; break;
                case EGL_LUMINANCE_SIZE: mWantLuminance = true; break;
                }
            }
        }
    }

    EGLint wantedComponentsSize(const Config &config) const
    {
        EGLint total = 0;

        if (mWantRed)       total += config.mRedSize;
        if (mWantGreen)     total += config.mGreenSize;
        if (mWantBlue)      total += config.mBlueSize;
        if (mWantAlpha)     total += config.mAlphaSize;
        if (mWantLuminance) total += config.mLuminanceSize;

        return total;
    }

    bool mWantRed;
    bool mWantGreen;
    bool mWantBlue;
    bool mWantAlpha;
    bool mWantLuminance;
};

std::vector<const Config*> ConfigSet::filter(const AttributeMap &attributeMap) const
{
    std::vector<const Config*> result;
    result.reserve(mConfigs.size());

    for (auto configIter = mConfigs.begin(); configIter != mConfigs.end(); configIter++)
    {
        const Config &config = configIter->second;
        bool match = true;

        for (auto attribIter = attributeMap.begin(); attribIter != attributeMap.end(); attribIter++)
        {
            EGLint attributeKey = attribIter->first;
            EGLint attributeValue = attribIter->second;

            switch (attributeKey)
            {
              case EGL_BUFFER_SIZE:               match = config.mBufferSize >= attributeValue;                        break;
              case EGL_ALPHA_SIZE:                match = config.mAlphaSize >= attributeValue;                         break;
              case EGL_BLUE_SIZE:                 match = config.mBlueSize >= attributeValue;                          break;
              case EGL_GREEN_SIZE:                match = config.mGreenSize >= attributeValue;                         break;
              case EGL_RED_SIZE:                  match = config.mRedSize >= attributeValue;                           break;
              case EGL_DEPTH_SIZE:                match = config.mDepthSize >= attributeValue;                         break;
              case EGL_STENCIL_SIZE:              match = config.mStencilSize >= attributeValue;                       break;
              case EGL_CONFIG_CAVEAT:             match = config.mConfigCaveat == (EGLenum)attributeValue;             break;
              case EGL_CONFIG_ID:                 match = config.mConfigID == attributeValue;                          break;
              case EGL_LEVEL:                     match = config.mLevel >= attributeValue;                             break;
              case EGL_NATIVE_RENDERABLE:         match = config.mNativeRenderable == (EGLBoolean)attributeValue;      break;
              case EGL_NATIVE_VISUAL_TYPE:        match = config.mNativeVisualType == attributeValue;                  break;
              case EGL_SAMPLES:                   match = config.mSamples >= attributeValue;                           break;
              case EGL_SAMPLE_BUFFERS:            match = config.mSampleBuffers >= attributeValue;                     break;
              case EGL_SURFACE_TYPE:              match = (config.mSurfaceType & attributeValue) == attributeValue;    break;
              case EGL_TRANSPARENT_TYPE:          match = config.mTransparentType == (EGLenum)attributeValue;          break;
              case EGL_TRANSPARENT_BLUE_VALUE:    match = config.mTransparentBlueValue == attributeValue;              break;
              case EGL_TRANSPARENT_GREEN_VALUE:   match = config.mTransparentGreenValue == attributeValue;             break;
              case EGL_TRANSPARENT_RED_VALUE:     match = config.mTransparentRedValue == attributeValue;               break;
              case EGL_BIND_TO_TEXTURE_RGB:       match = config.mBindToTextureRGB == (EGLBoolean)attributeValue;      break;
              case EGL_BIND_TO_TEXTURE_RGBA:      match = config.mBindToTextureRGBA == (EGLBoolean)attributeValue;     break;
              case EGL_MIN_SWAP_INTERVAL:         match = config.mMinSwapInterval == attributeValue;                   break;
              case EGL_MAX_SWAP_INTERVAL:         match = config.mMaxSwapInterval == attributeValue;                   break;
              case EGL_LUMINANCE_SIZE:            match = config.mLuminanceSize >= attributeValue;                     break;
              case EGL_ALPHA_MASK_SIZE:           match = config.mAlphaMaskSize >= attributeValue;                     break;
              case EGL_COLOR_BUFFER_TYPE:         match = config.mColorBufferType == (EGLenum)attributeValue;          break;
              case EGL_RENDERABLE_TYPE:           match = (config.mRenderableType & attributeValue) == attributeValue; break;
              case EGL_MATCH_NATIVE_PIXMAP:       match = false; UNIMPLEMENTED();                                      break;
              case EGL_CONFORMANT:                match = (config.mConformant & attributeValue) == attributeValue;     break;
              case EGL_MAX_PBUFFER_WIDTH:         match = config.mMaxPBufferWidth >= attributeValue;                   break;
              case EGL_MAX_PBUFFER_HEIGHT:        match = config.mMaxPBufferHeight >= attributeValue;                  break;
              case EGL_MAX_PBUFFER_PIXELS:        match = config.mMaxPBufferPixels >= attributeValue;                  break;
              default: UNREACHABLE();
            }

            if (!match)
            {
                break;
            }
        }

        if (match)
        {
            result.push_back(&config);
        }
    }

    // Sort the result
    std::sort(result.begin(), result.end(), ConfigSorter(attributeMap));

    return result;
}

}

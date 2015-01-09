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
    : renderTargetFormat(desc.renderTargetFormat), depthStencilFormat(desc.depthStencilFormat), multiSample(desc.multiSample)
{
    bindToTextureRGB = EGL_FALSE;
    bindToTextureRGBA = EGL_FALSE;
    switch (desc.renderTargetFormat)
    {
      case GL_RGB5_A1:
        bufferSize = 16;
        redSize = 5;
        greenSize = 5;
        blueSize = 5;
        alphaSize = 1;
        break;
      case GL_BGR5_A1_ANGLEX:
        bufferSize = 16;
        redSize = 5;
        greenSize = 5;
        blueSize = 5;
        alphaSize = 1;
        break;
      case GL_RGBA8_OES:
        bufferSize = 32;
        redSize = 8;
        greenSize = 8;
        blueSize = 8;
        alphaSize = 8;
        bindToTextureRGBA = true;
        break;
      case GL_RGB565:
        bufferSize = 16;
        redSize = 5;
        greenSize = 6;
        blueSize = 5;
        alphaSize = 0;
        break;
      case GL_RGB8_OES:
        bufferSize = 32;
        redSize = 8;
        greenSize = 8;
        blueSize = 8;
        alphaSize = 0;
        bindToTextureRGB = true;
        break;
      case GL_BGRA8_EXT:
        bufferSize = 32;
        redSize = 8;
        greenSize = 8;
        blueSize = 8;
        alphaSize = 8;
        bindToTextureRGBA = true;
        break;
      default:
        UNREACHABLE();   // Other formats should not be valid
    }

    luminanceSize = 0;
    alphaMaskSize = 0;
    colorBufferType = EGL_RGB_BUFFER;
    configCaveat = (desc.fastConfig) ? EGL_NONE : EGL_SLOW_CONFIG;
    configID = 0;
    conformant = 0;

    switch (desc.depthStencilFormat)
    {
      case GL_NONE:
        depthSize = 0;
        stencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT32_OES:
        depthSize = 32;
        stencilSize = 0;
        break;
      case GL_DEPTH24_STENCIL8_OES:
        depthSize = 24;
        stencilSize = 8;
        break;
      case GL_DEPTH_COMPONENT24_OES:
        depthSize = 24;
        stencilSize = 0;
        break;
      case GL_DEPTH_COMPONENT16:
        depthSize = 16;
        stencilSize = 0;
        break;
      default:
        UNREACHABLE();
    }

    level = 0;
    matchNativePixmap = EGL_NONE;
    maxPBufferWidth = texWidth;
    maxPBufferHeight = texHeight;
    maxPBufferPixels = texWidth*texHeight;
    maxSwapInterval = maxInterval;
    minSwapInterval = minInterval;
    nativeRenderable = EGL_FALSE;
    nativeVisualID = 0;
    nativeVisualType = 0;
    renderableType = EGL_OPENGL_ES2_BIT;
    sampleBuffers = desc.multiSample ? 1 : 0;
    samples = desc.multiSample;
    surfaceType = EGL_PBUFFER_BIT | EGL_WINDOW_BIT | EGL_SWAP_BEHAVIOR_PRESERVED_BIT;
    transparentType = EGL_NONE;
    transparentRedValue = 0;
    transparentGreenValue = 0;
    transparentBlueValue = 0;

    if (desc.es2Conformant)
    {
        conformant = EGL_OPENGL_ES2_BIT;
    }

    if (desc.es3Capable)
    {
        renderableType |= EGL_OPENGL_ES3_BIT_KHR;
        conformant |= EGL_OPENGL_ES3_BIT_KHR;
    }
}

EGLint ConfigSet::add(const Config &config)
{
    // Set the config's ID to a small number that starts at 1 ([EGL 1.5] section 3.4)
    EGLint id = mConfigs.size() + 1;

    Config copyConfig(config);
    copyConfig.configID = id;
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
        SORT(configCaveat);

        META_ASSERT(EGL_RGB_BUFFER < EGL_LUMINANCE_BUFFER);
        SORT(colorBufferType);

        // By larger total number of color bits, only considering those that are requested to be > 0.
        EGLint xComponentsSize = wantedComponentsSize(x);
        EGLint yComponentsSize = wantedComponentsSize(y);
        if (xComponentsSize != yComponentsSize)
        {
            return xComponentsSize > yComponentsSize;
        }

        SORT(bufferSize);
        SORT(sampleBuffers);
        SORT(samples);
        SORT(depthSize);
        SORT(stencilSize);
        SORT(alphaMaskSize);
        SORT(nativeVisualType);
        SORT(configID);

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

        if (mWantRed)       total += config.redSize;
        if (mWantGreen)     total += config.greenSize;
        if (mWantBlue)      total += config.blueSize;
        if (mWantAlpha)     total += config.alphaSize;
        if (mWantLuminance) total += config.luminanceSize;

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
              case EGL_BUFFER_SIZE:               match = config.bufferSize >= attributeValue;                        break;
              case EGL_ALPHA_SIZE:                match = config.alphaSize >= attributeValue;                         break;
              case EGL_BLUE_SIZE:                 match = config.blueSize >= attributeValue;                          break;
              case EGL_GREEN_SIZE:                match = config.greenSize >= attributeValue;                         break;
              case EGL_RED_SIZE:                  match = config.redSize >= attributeValue;                           break;
              case EGL_DEPTH_SIZE:                match = config.depthSize >= attributeValue;                         break;
              case EGL_STENCIL_SIZE:              match = config.stencilSize >= attributeValue;                       break;
              case EGL_CONFIG_CAVEAT:             match = config.configCaveat == (EGLenum)attributeValue;             break;
              case EGL_CONFIG_ID:                 match = config.configID == attributeValue;                          break;
              case EGL_LEVEL:                     match = config.level >= attributeValue;                             break;
              case EGL_NATIVE_RENDERABLE:         match = config.nativeRenderable == (EGLBoolean)attributeValue;      break;
              case EGL_NATIVE_VISUAL_TYPE:        match = config.nativeVisualType == attributeValue;                  break;
              case EGL_SAMPLES:                   match = config.samples >= attributeValue;                           break;
              case EGL_SAMPLE_BUFFERS:            match = config.sampleBuffers >= attributeValue;                     break;
              case EGL_SURFACE_TYPE:              match = (config.surfaceType & attributeValue) == attributeValue;    break;
              case EGL_TRANSPARENT_TYPE:          match = config.transparentType == (EGLenum)attributeValue;          break;
              case EGL_TRANSPARENT_BLUE_VALUE:    match = config.transparentBlueValue == attributeValue;              break;
              case EGL_TRANSPARENT_GREEN_VALUE:   match = config.transparentGreenValue == attributeValue;             break;
              case EGL_TRANSPARENT_RED_VALUE:     match = config.transparentRedValue == attributeValue;               break;
              case EGL_BIND_TO_TEXTURE_RGB:       match = config.bindToTextureRGB == (EGLBoolean)attributeValue;      break;
              case EGL_BIND_TO_TEXTURE_RGBA:      match = config.bindToTextureRGBA == (EGLBoolean)attributeValue;     break;
              case EGL_MIN_SWAP_INTERVAL:         match = config.minSwapInterval == attributeValue;                   break;
              case EGL_MAX_SWAP_INTERVAL:         match = config.maxSwapInterval == attributeValue;                   break;
              case EGL_LUMINANCE_SIZE:            match = config.luminanceSize >= attributeValue;                     break;
              case EGL_ALPHA_MASK_SIZE:           match = config.alphaMaskSize >= attributeValue;                     break;
              case EGL_COLOR_BUFFER_TYPE:         match = config.colorBufferType == (EGLenum)attributeValue;          break;
              case EGL_RENDERABLE_TYPE:           match = (config.renderableType & attributeValue) == attributeValue; break;
              case EGL_MATCH_NATIVE_PIXMAP:       match = false; UNIMPLEMENTED();                                     break;
              case EGL_CONFORMANT:                match = (config.conformant & attributeValue) == attributeValue;     break;
              case EGL_MAX_PBUFFER_WIDTH:         match = config.maxPBufferWidth >= attributeValue;                   break;
              case EGL_MAX_PBUFFER_HEIGHT:        match = config.maxPBufferHeight >= attributeValue;                  break;
              case EGL_MAX_PBUFFER_PIXELS:        match = config.maxPBufferPixels >= attributeValue;                  break;
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

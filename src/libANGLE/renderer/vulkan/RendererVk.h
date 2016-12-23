//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// RendererVk.h:
//    Defines the class interface for RendererVk.
//

#ifndef LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_
#define LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

#include <vulkan/vulkan.h>

#include "common/angleutils.h"
#include "libANGLE/Caps.h"
#include "libANGLE/renderer/vulkan/renderervk_utils.h"

namespace egl
{
class AttributeMap;
}

namespace rx
{

class RendererVk : angle::NonCopyable
{
  public:
    RendererVk();
    ~RendererVk();

    vk::Error initialize(const egl::AttributeMap &attribs);

    std::string getRendererDescription() const;

    VkInstance getInstance() const { return mInstance; }

    const gl::Caps &getNativeCaps() const;
    const gl::TextureCapsMap &getNativeTextureCaps() const;
    const gl::Extensions &getNativeExtensions() const;
    const gl::Limitations &getNativeLimitations() const;

  private:
    void ensureCapsInitialized() const;
    void generateCaps(gl::Caps *outCaps,
                      gl::TextureCapsMap *outTextureCaps,
                      gl::Extensions *outExtensions,
                      gl::Limitations *outLimitations) const;

    mutable bool mCapsInitialized;
    mutable gl::Caps mNativeCaps;
    mutable gl::TextureCapsMap mNativeTextureCaps;
    mutable gl::Extensions mNativeExtensions;
    mutable gl::Limitations mNativeLimitations;

    VkInstance mInstance;
    bool mEnableValidationLayers;
    VkDebugReportCallbackEXT mDebugReportCallback;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_H_

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// renderervk_utils:
//    Helper functions for the Vulkan Renderer.
//

#ifndef LIBANGLE_RENDERER_VULKAN_RENDERERVK_UTILS_H_
#define LIBANGLE_RENDERER_VULKAN_RENDERERVK_UTILS_H_

#include <vulkan/vulkan.h>

#include "libANGLE/Error.h"

namespace rx
{

const char *VulkanResultString(VkResult result);
bool HasStandardValidationLayer(const std::vector<VkLayerProperties> &layerProps);

extern const char *g_VkStdValidationLayerName;

namespace vk
{

class Error final
{
  public:
    Error(VkResult result, const char *file, unsigned int line);
    ~Error();

    Error(const Error &other);
    Error &operator=(const Error &other);

    gl::Error toGL(GLenum glErrorCode) const;
    egl::Error toEGL(EGLint eglErrorCode) const;

    operator gl::Error() const;
    operator egl::Error() const;

    bool isError() const;

  private:
    std::string getExtendedMessage() const;

    VkResult mResult;
    const char *mFile;
    unsigned int mLine;
};

// Avoid conflicting with X headers which define "Success".
inline Error NoError()
{
    return Error(VK_SUCCESS, nullptr, 0);
}

}  // namespace vk

}  // namespace rx

#define ANGLE_VK_TRY(command)                                          \
    {                                                                  \
        auto ANGLE_LOCAL_VAR = command;                                \
        if (ANGLE_LOCAL_VAR != VK_SUCCESS)                             \
        {                                                              \
            return rx::vk::Error(ANGLE_LOCAL_VAR, __FILE__, __LINE__); \
        }                                                              \
    }                                                                  \
    ANGLE_EMPTY_STATEMENT

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_UTILS_H_

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// renderervk_utils:
//    Helper functions for the Vulkan Renderer.
//

#include "renderervk_utils.h"

#include "common/debug.h"

namespace rx
{

namespace
{
GLenum DefaultGLErrorCode(VkResult result)
{
    switch (result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_TOO_MANY_OBJECTS:
            return GL_OUT_OF_MEMORY;
        default:
            return GL_INVALID_OPERATION;
    }
}

EGLint DefaultEGLErrorCode(VkResult result)
{
    switch (result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_TOO_MANY_OBJECTS:
            return EGL_BAD_ALLOC;
        case VK_ERROR_INITIALIZATION_FAILED:
            return EGL_NOT_INITIALIZED;
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_DEVICE_LOST:
            return EGL_CONTEXT_LOST;
        default:
            return EGL_BAD_ACCESS;
    }
}
}  // anonymous namespace

// Mirrors std_validation_str in loader.h
// TODO(jmadill): Possibly wrap the loader into a safe source file. Can't be included trivially.
const char *g_VkStdValidationLayerName = "VK_LAYER_LUNARG_standard_validation";

const char *VulkanResultString(VkResult result)
{
    switch (result)
    {
        case VK_SUCCESS:
            return "Command successfully completed.";
        case VK_NOT_READY:
            return "A fence or query has not yet completed.";
        case VK_TIMEOUT:
            return "A wait operation has not completed in the specified time.";
        case VK_EVENT_SET:
            return "An event is signaled.";
        case VK_EVENT_RESET:
            return "An event is unsignaled.";
        case VK_INCOMPLETE:
            return "A return array was too small for the result.";
        case VK_SUBOPTIMAL_KHR:
            return "A swapchain no longer matches the surface properties exactly, but can still be "
                   "used to present to the surface successfully.";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "A host memory allocation has failed.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "A device memory allocation has failed.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "Initialization of an object could not be completed for implementation-specific "
                   "reasons.";
        case VK_ERROR_DEVICE_LOST:
            return "The logical or physical device has been lost.";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "Mapping of a memory object has failed.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "A requested layer is not present or could not be loaded.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "A requested extension is not supported.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "A requested feature is not supported.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "The requested version of Vulkan is not supported by the driver or is otherwise "
                   "incompatible for implementation-specific reasons.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "Too many objects of the type have already been created.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "A requested format is not supported on this device.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "A surface is no longer available.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "The requested window is already connected to a VkSurfaceKHR, or to some other "
                   "non-Vulkan API.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "A surface has changed in such a way that it is no longer compatible with the "
                   "swapchain.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "The display used by a swapchain does not use the same presentable image "
                   "layout, or is incompatible in a way that prevents sharing an image.";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "The validation layers detected invalid API usage.";
        default:
            return "Unknown vulkan error code.";
    }
}

namespace vk
{

Error::Error(VkResult result, const char *file, unsigned int line)
    : mResult(result), mFile(file), mLine(line)
{
}

Error::~Error()
{
}

Error::Error(const Error &other) = default;
Error &Error::operator=(const Error &other) = default;

gl::Error Error::toGL(GLenum glErrorCode) const
{
    if (!isError())
    {
        return gl::NoError();
    }

    // TODO(jmadill): Set extended error code to 'vulkan internal error'.
    const std::string &message = getExtendedMessage();
    return gl::Error(glErrorCode, message.c_str());
}

egl::Error Error::toEGL(EGLint eglErrorCode) const
{
    if (!isError())
    {
        return egl::Error(EGL_SUCCESS);
    }

    // TODO(jmadill): Set extended error code to 'vulkan internal error'.
    const std::string &message = getExtendedMessage();
    return egl::Error(eglErrorCode, message.c_str());
}

std::string Error::getExtendedMessage() const
{
    std::stringstream errorStream;
    errorStream << "Internal Vulkan error: " << VulkanResultString(mResult) << ", in " << mFile
                << ", line " << mLine << ".";
    return errorStream.str();
}

Error::operator gl::Error() const
{
    return toGL(DefaultGLErrorCode(mResult));
}

Error::operator egl::Error() const
{
    return toEGL(DefaultEGLErrorCode(mResult));
}

bool Error::isError() const
{
    return (mResult != VK_SUCCESS);
}

}  // namespace vk

bool HasStandardValidationLayer(const std::vector<VkLayerProperties> &layerProps)
{
    for (const auto &layerProp : layerProps)
    {
        if (std::string(layerProp.layerName) == g_VkStdValidationLayerName)
        {
            return true;
        }
    }

    return false;
}

}  // namespace rx

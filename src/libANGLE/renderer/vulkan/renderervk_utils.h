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
    Error(VkResult result);
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

template <typename ResultT>
using ErrorOrResult = angle::ErrorOrResultBase<Error, ResultT, VkResult, VK_SUCCESS>;

// Avoid conflicting with X headers which define "Success".
inline Error NoError()
{
    return Error(VK_SUCCESS);
}

template <typename HandleT>
class WrappedObject : angle::NonCopyable
{
  public:
    WrappedObject() : mDevice(VK_NULL_HANDLE), mHandle(VK_NULL_HANDLE) {}
    explicit WrappedObject(VkDevice device) : mDevice(device), mHandle(VK_NULL_HANDLE) {}
    WrappedObject(WrappedObject &&other) : mDevice(other.mDevice), mHandle(other.mHandle)
    {
        other.mDevice = VK_NULL_HANDLE;
        other.mHandle = VK_NULL_HANDLE;
    }
    virtual ~WrappedObject() {}

    HandleT getHandle() const { return mHandle; }
    bool validDevice() const { return (mDevice != VK_NULL_HANDLE); }
    bool valid() const { return (mHandle != VK_NULL_HANDLE) && validDevice(); }

  protected:
    void assignOpBase(WrappedObject &&other)
    {
        std::swap(mDevice, other.mDevice);
        std::swap(mHandle, other.mHandle);
    }

    VkDevice mDevice;
    HandleT mHandle;
};

// Helper class that wraps a Vulkan command buffer.
class CommandBuffer final : public WrappedObject<VkCommandBuffer>
{
  public:
    CommandBuffer(VkDevice device, VkCommandPool commandPool);
    ~CommandBuffer() override;

    Error begin();
    Error end();
    Error reset();
    void singleImageBarrier(VkPipelineStageFlags srcStageMask,
                            VkPipelineStageFlags dstStageMask,
                            VkDependencyFlags dependencyFlags,
                            const VkImageMemoryBarrier &imageMemoryBarrier);

  private:
    VkCommandPool mCommandPool;
};

class Image final : public WrappedObject<VkImage>
{
  public:
    // Use this constructor if the lifetime of the image is not controlled by ANGLE. (SwapChain)
    Image();
    explicit Image(VkImage image);
    Image(Image &&other);

    Image &operator=(Image &&other);

    ~Image() override;

    void changeLayout(VkImageAspectFlags aspectMask,
                      VkImageLayout newLayout,
                      CommandBuffer *commandBuffer);

  private:
    VkImageLayout mCurrentLayout;
};

class ImageView final : public WrappedObject<VkImageView>
{
  public:
    ImageView();
    explicit ImageView(VkDevice device);
    ImageView(ImageView &&other);

    ImageView &operator=(ImageView &&other);

    ~ImageView() override;

    Error init(const VkImageViewCreateInfo &createInfo);
};

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

#define ANGLE_VK_CHECK(test, error) ANGLE_VK_TRY(test ? VK_SUCCESS : error)

#endif  // LIBANGLE_RENDERER_VULKAN_RENDERERVK_UTILS_H_

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

#include "common/Optional.h"
#include "libANGLE/Error.h"

namespace gl
{
struct Box;
struct Extents;
struct Rectangle;
}

namespace rx
{
const char *VulkanResultString(VkResult result);
bool HasStandardValidationLayer(const std::vector<VkLayerProperties> &layerProps);

extern const char *g_VkStdValidationLayerName;

enum class TextureDimension
{
    TEX_2D,
    TEX_CUBE,
    TEX_3D,
    TEX_2D_ARRAY,
};

namespace vk
{
class DeviceMemory;
class Framebuffer;
class Image;
class RenderPass;

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
    template <typename T>
    operator gl::ErrorOrResult<T>() const
    {
        return static_cast<gl::Error>(*this);
    }

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

    void clearSingleColorImage(const vk::Image &image, const VkClearColorValue &color);

    void copySingleImage(const vk::Image &srcImage,
                         const vk::Image &destImage,
                         const gl::Box &copyRegion,
                         VkImageAspectFlags aspectMask);

    void beginRenderPass(const RenderPass &renderPass,
                         const Framebuffer &framebuffer,
                         const gl::Rectangle &renderArea,
                         const std::vector<VkClearValue> &clearValues);
    void endRenderPass();

  private:
    VkCommandPool mCommandPool;
};

class Image final : public WrappedObject<VkImage>
{
  public:
    // Use this constructor if the lifetime of the image is not controlled by ANGLE. (SwapChain)
    Image();
    Image(VkDevice device);
    explicit Image(VkImage image);
    Image(Image &&other);

    Image &operator=(Image &&other);

    ~Image() override;

    Error init(const VkImageCreateInfo &createInfo);

    void changeLayoutTop(VkImageAspectFlags aspectMask,
                         VkImageLayout newLayout,
                         CommandBuffer *commandBuffer);

    void changeLayoutWithStages(VkImageAspectFlags aspectMask,
                                VkImageLayout newLayout,
                                VkPipelineStageFlags srcStageMask,
                                VkPipelineStageFlags dstStageMask,
                                CommandBuffer *commandBuffer);

    void getMemoryRequirements(VkMemoryRequirements *requirementsOut) const;
    Error bindMemory(const vk::DeviceMemory &deviceMemory);

    VkImageLayout getCurrentLayout() const { return mCurrentLayout; }
    void updateLayout(VkImageLayout layout) { mCurrentLayout = layout; }

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

class Semaphore final : public WrappedObject<VkSemaphore>
{
  public:
    Semaphore();
    Semaphore(VkDevice device);
    Semaphore(Semaphore &&other);
    ~Semaphore();
    Semaphore &operator=(Semaphore &&other);

    Error init();
};

class Framebuffer final : public WrappedObject<VkFramebuffer>
{
  public:
    Framebuffer();
    Framebuffer(VkDevice device);
    Framebuffer(Framebuffer &&other);
    ~Framebuffer();
    Framebuffer &operator=(Framebuffer &&other);

    Error init(const VkFramebufferCreateInfo &createInfo);
};

class DeviceMemory final : public WrappedObject<VkDeviceMemory>
{
  public:
    DeviceMemory();
    DeviceMemory(VkDevice device);
    DeviceMemory(DeviceMemory &&other);
    ~DeviceMemory();
    DeviceMemory &operator=(DeviceMemory &&other);

    Error allocate(const VkMemoryAllocateInfo &allocInfo);
    Error map(VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, uint8_t **mapPointer);
    void unmap();
};

class RenderPass final : public WrappedObject<VkRenderPass>
{
  public:
    RenderPass();
    RenderPass(VkDevice device);
    RenderPass(RenderPass &&other);
    ~RenderPass();
    RenderPass &operator=(RenderPass &&other);

    Error init(const VkRenderPassCreateInfo &createInfo);
};

class StagingImage final : angle::NonCopyable
{
  public:
    StagingImage();
    StagingImage(VkDevice device);
    StagingImage(StagingImage &&other);
    ~StagingImage();
    StagingImage &operator=(StagingImage &&other);

    vk::Error init(uint32_t queueFamilyIndex,
                   uint32_t hostVisibleMemoryIndex,
                   TextureDimension dimension,
                   VkFormat format,
                   const gl::Extents &extent);

    Image &getImage() { return mImage; }
    const Image &getImage() const { return mImage; }
    DeviceMemory &getDeviceMemory() { return mDeviceMemory; }
    const DeviceMemory &getDeviceMemory() const { return mDeviceMemory; }
    VkDeviceSize getSize() const { return mSize; }

  private:
    Image mImage;
    DeviceMemory mDeviceMemory;
    VkDeviceSize mSize;
};

class Buffer final : public WrappedObject<VkBuffer>
{
  public:
    Buffer();
    Buffer(VkDevice device);
    Buffer(Buffer &&other);
    ~Buffer();
    Buffer &operator=(Buffer &&other);

    Error init(const VkBufferCreateInfo &createInfo);
    Error bindMemory();

    DeviceMemory &getMemory() { return mMemory; }
    const DeviceMemory &getMemory() const { return mMemory; }

  private:
    DeviceMemory mMemory;
};

class ShaderModule final : public WrappedObject<VkShaderModule>
{
  public:
    ShaderModule();
    ShaderModule(VkDevice device);
    ShaderModule(ShaderModule &&other);
    ~ShaderModule() override;
    ShaderModule &operator=(ShaderModule &&other);

    Error init(const VkShaderModuleCreateInfo &createInfo);
};

}  // namespace vk

Optional<uint32_t> FindMemoryType(const VkPhysicalDeviceMemoryProperties &memoryProps,
                                  const VkMemoryRequirements &requirements,
                                  uint32_t propertyFlagMask);

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

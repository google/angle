//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// VulkanExternalImageTest.cpp : Tests of images allocated externally using Vulkan.

#include "test_utils/ANGLETest.h"

#include "common/debug.h"
#include "test_utils/VulkanExternalHelper.h"
#include "test_utils/gl_raii.h"

namespace angle
{

namespace
{

constexpr int kInvalidFd = -1;

VkFormat ChooseAnyImageFormat(const VulkanExternalHelper &helper)
{
    static constexpr VkFormat kFormats[] = {
        VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_UNORM,
    };

    for (VkFormat format : kFormats)
    {
        if (helper.canCreateImageOpaqueFd(format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL))
        {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

// List of VkFormat/internalformat combinations Chrome uses.
// This is compiled from the maps in
// components/viz/common/resources/resource_format_utils.cc.
const struct ImageFormatPair
{
    VkFormat vkFormat;
    GLenum internalFormat;
    const char *requiredExtension;
} kChromeFormats[] = {
    {VK_FORMAT_R8G8B8A8_UNORM, GL_RGBA8_OES},                    // RGBA_8888
    {VK_FORMAT_B8G8R8A8_UNORM, GL_BGRA8_EXT},                    // BGRA_8888
    {VK_FORMAT_R4G4B4A4_UNORM_PACK16, GL_RGBA4},                 // RGBA_4444
    {VK_FORMAT_R16G16B16A16_SFLOAT, GL_RGBA16F_EXT},             // RGBA_F16
    {VK_FORMAT_R8_UNORM, GL_R8_EXT},                             // RED_8
    {VK_FORMAT_R5G6B5_UNORM_PACK16, GL_RGB565},                  // RGB_565
    {VK_FORMAT_R16_UNORM, GL_R16_EXT, "GL_EXT_texture_norm16"},  // R16_EXT
    {VK_FORMAT_A2B10G10R10_UNORM_PACK32, GL_RGB10_A2_EXT},       // RGBA_1010102
    {VK_FORMAT_R8_UNORM, GL_ALPHA8_EXT},                         // ALPHA_8
    {VK_FORMAT_R8_UNORM, GL_LUMINANCE8_EXT},                     // LUMINANCE_8
    {VK_FORMAT_R8G8_UNORM, GL_RG8_EXT},                          // RG_88

    // TODO(spang): Chrome could use GL_RGBA8_OES here if we can solve a couple
    // of validation comformance issues (see crbug.com/1058521). Or, we can add
    // a new internalformat that's unambiguously R8G8B8X8 in ANGLE and use that.
    {VK_FORMAT_R8G8B8A8_UNORM, GL_RGB8_OES},  // RGBX_8888
};

}  // namespace

class VulkanExternalImageTest : public ANGLETest
{
  protected:
    VulkanExternalImageTest()
    {
        setWindowWidth(1);
        setWindowHeight(1);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
    }
};

// glImportMemoryFdEXT must be able to import a valid opaque fd.
TEST_P(VulkanExternalImageTest, ShouldImportMemoryOpaqueFd)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_memory_object_fd"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    VkFormat format = ChooseAnyImageFormat(helper);
    ANGLE_SKIP_TEST_IF(format == VK_FORMAT_UNDEFINED);

    VkImage image                 = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;

    VkExtent3D extent = {1, 1, 1};
    VkResult result =
        helper.createImage2DOpaqueFd(format, extent, &image, &deviceMemory, &deviceMemorySize);
    EXPECT_EQ(result, VK_SUCCESS);

    int fd = kInvalidFd;
    result = helper.exportMemoryOpaqueFd(deviceMemory, &fd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(fd, kInvalidFd);

    {
        GLMemoryObject memoryObject;
        GLint dedicatedMemory = GL_TRUE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        glImportMemoryFdEXT(memoryObject, deviceMemorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);

        // Test that after calling glImportMemoryFdEXT, the parameters of the memory object cannot
        // be changed
        dedicatedMemory = GL_FALSE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        EXPECT_GL_ERROR(GL_INVALID_OPERATION);
    }

    EXPECT_GL_NO_ERROR();

    vkDestroyImage(helper.getDevice(), image, nullptr);
    vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
}

// glImportSemaphoreFdEXT must be able to import a valid opaque fd.
TEST_P(VulkanExternalImageTest, ShouldImportSemaphoreOpaqueFd)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_semaphore_fd"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    ANGLE_SKIP_TEST_IF(!helper.canCreateSemaphoreOpaqueFd());

    VkSemaphore vkSemaphore = VK_NULL_HANDLE;
    VkResult result         = helper.createSemaphoreOpaqueFd(&vkSemaphore);
    EXPECT_EQ(result, VK_SUCCESS);

    int fd = kInvalidFd;
    result = helper.exportSemaphoreOpaqueFd(vkSemaphore, &fd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(fd, kInvalidFd);

    {
        GLSemaphore glSemaphore;
        glImportSemaphoreFdEXT(glSemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);
    }

    EXPECT_GL_NO_ERROR();

    vkDestroySemaphore(helper.getDevice(), vkSemaphore, nullptr);
}

// Test creating and clearing a simple RGBA8 texture in a opaque fd.
TEST_P(VulkanExternalImageTest, ShouldClearOpaqueFdRGBA8)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_memory_object_fd"));
    // http://anglebug.com/4630
    ANGLE_SKIP_TEST_IF(IsAndroid() && (IsPixel2() || IsPixel2XL()));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    ANGLE_SKIP_TEST_IF(
        !helper.canCreateImageOpaqueFd(format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL));

    VkImage image                 = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;

    VkExtent3D extent = {1, 1, 1};
    VkResult result =
        helper.createImage2DOpaqueFd(format, extent, &image, &deviceMemory, &deviceMemorySize);
    EXPECT_EQ(result, VK_SUCCESS);

    int fd = kInvalidFd;
    result = helper.exportMemoryOpaqueFd(deviceMemory, &fd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(fd, kInvalidFd);

    {
        GLMemoryObject memoryObject;
        GLint dedicatedMemory = GL_TRUE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        glImportMemoryFdEXT(memoryObject, deviceMemorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);

        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, memoryObject, 0);

        GLFramebuffer framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);

        EXPECT_PIXEL_NEAR(0, 0, 128, 128, 128, 128, 1.0);
    }

    EXPECT_GL_NO_ERROR();

    vkDestroyImage(helper.getDevice(), image, nullptr);
    vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
}

// Test creating and clearing a simple RGBA8 texture in a zircon vmo.
TEST_P(VulkanExternalImageTest, ShouldClearZirconVmoRGBA8)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_memory_object_fuchsia"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    ANGLE_SKIP_TEST_IF(
        !helper.canCreateImageZirconVmo(format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL));

    VkImage image                 = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;

    VkExtent3D extent = {1, 1, 1};
    VkResult result =
        helper.createImage2DZirconVmo(format, extent, &image, &deviceMemory, &deviceMemorySize);
    EXPECT_EQ(result, VK_SUCCESS);

    zx_handle_t vmo = ZX_HANDLE_INVALID;
    result          = helper.exportMemoryZirconVmo(deviceMemory, &vmo);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(vmo, ZX_HANDLE_INVALID);

    {
        GLMemoryObject memoryObject;
        GLint dedicatedMemory = GL_TRUE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        glImportMemoryZirconHandleANGLE(memoryObject, deviceMemorySize,
                                        GL_HANDLE_TYPE_ZIRCON_VMO_ANGLE, vmo);

        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, memoryObject, 0);

        GLFramebuffer framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);

        EXPECT_PIXEL_NEAR(0, 0, 128, 128, 128, 128, 1.0);
    }

    EXPECT_GL_NO_ERROR();

    vkDestroyImage(helper.getDevice(), image, nullptr);
    vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
}

// Test all format combinations used by Chrome import successfully (opaque fd).
TEST_P(VulkanExternalImageTest, TextureFormatCompatChromiumFd)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_memory_object_fd"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());
    for (const ImageFormatPair &format : kChromeFormats)
    {
        if (!helper.canCreateImageOpaqueFd(format.vkFormat, VK_IMAGE_TYPE_2D,
                                           VK_IMAGE_TILING_OPTIMAL))
        {
            continue;
        }

        if (format.requiredExtension && !IsGLExtensionEnabled(format.requiredExtension))
        {
            continue;
        }

        VkImage image                 = VK_NULL_HANDLE;
        VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
        VkDeviceSize deviceMemorySize = 0;

        VkExtent3D extent = {113, 211, 1};
        VkResult result   = helper.createImage2DOpaqueFd(format.vkFormat, extent, &image,
                                                       &deviceMemory, &deviceMemorySize);
        EXPECT_EQ(result, VK_SUCCESS);

        int fd = kInvalidFd;
        result = helper.exportMemoryOpaqueFd(deviceMemory, &fd);
        EXPECT_EQ(result, VK_SUCCESS);
        EXPECT_NE(fd, kInvalidFd);

        {
            GLMemoryObject memoryObject;
            GLint dedicatedMemory = GL_TRUE;
            glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                         &dedicatedMemory);
            glImportMemoryFdEXT(memoryObject, deviceMemorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, fd);

            GLTexture texture;
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, format.internalFormat, extent.width,
                                 extent.height, memoryObject, 0);
        }

        EXPECT_GL_NO_ERROR();

        vkDestroyImage(helper.getDevice(), image, nullptr);
        vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
    }
}

// Test all format combinations used by Chrome import successfully (fuchsia).
TEST_P(VulkanExternalImageTest, TextureFormatCompatChromiumZirconHandle)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_memory_object_fuchsia"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());
    for (const ImageFormatPair &format : kChromeFormats)
    {
        if (!helper.canCreateImageZirconVmo(format.vkFormat, VK_IMAGE_TYPE_2D,
                                            VK_IMAGE_TILING_OPTIMAL))
        {
            continue;
        }

        if (format.requiredExtension && !IsGLExtensionEnabled(format.requiredExtension))
        {
            continue;
        }

        VkImage image                 = VK_NULL_HANDLE;
        VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
        VkDeviceSize deviceMemorySize = 0;

        VkExtent3D extent = {113, 211, 1};
        VkResult result   = helper.createImage2DZirconVmo(format.vkFormat, extent, &image,
                                                        &deviceMemory, &deviceMemorySize);
        EXPECT_EQ(result, VK_SUCCESS);

        zx_handle_t vmo = ZX_HANDLE_INVALID;
        result          = helper.exportMemoryZirconVmo(deviceMemory, &vmo);
        EXPECT_EQ(result, VK_SUCCESS);
        EXPECT_NE(vmo, ZX_HANDLE_INVALID);

        {
            GLMemoryObject memoryObject;
            GLint dedicatedMemory = GL_TRUE;
            glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                         &dedicatedMemory);
            glImportMemoryZirconHandleANGLE(memoryObject, deviceMemorySize,
                                            GL_HANDLE_TYPE_ZIRCON_VMO_ANGLE, vmo);

            GLTexture texture;
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, format.internalFormat, extent.width,
                                 extent.height, memoryObject, 0);
        }

        EXPECT_GL_NO_ERROR();

        vkDestroyImage(helper.getDevice(), image, nullptr);
        vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
    }
}

// Test creating and clearing RGBA8 texture in opaque fd with acquire/release.
TEST_P(VulkanExternalImageTest, ShouldClearOpaqueFdWithSemaphores)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_memory_object_fd"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_EXT_semaphore_fd"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    ANGLE_SKIP_TEST_IF(
        !helper.canCreateImageOpaqueFd(format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL));
    ANGLE_SKIP_TEST_IF(!helper.canCreateSemaphoreOpaqueFd());

    VkSemaphore vkAcquireSemaphore = VK_NULL_HANDLE;
    VkResult result                = helper.createSemaphoreOpaqueFd(&vkAcquireSemaphore);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_TRUE(vkAcquireSemaphore != VK_NULL_HANDLE);

    VkSemaphore vkReleaseSemaphore = VK_NULL_HANDLE;
    result                         = helper.createSemaphoreOpaqueFd(&vkReleaseSemaphore);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_TRUE(vkReleaseSemaphore != VK_NULL_HANDLE);

    int acquireSemaphoreFd = kInvalidFd;
    result = helper.exportSemaphoreOpaqueFd(vkAcquireSemaphore, &acquireSemaphoreFd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(acquireSemaphoreFd, kInvalidFd);

    int releaseSemaphoreFd = kInvalidFd;
    result = helper.exportSemaphoreOpaqueFd(vkReleaseSemaphore, &releaseSemaphoreFd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(releaseSemaphoreFd, kInvalidFd);

    VkImage image                 = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;

    VkExtent3D extent = {1, 1, 1};
    result = helper.createImage2DOpaqueFd(format, extent, &image, &deviceMemory, &deviceMemorySize);
    EXPECT_EQ(result, VK_SUCCESS);

    int memoryFd = kInvalidFd;
    result       = helper.exportMemoryOpaqueFd(deviceMemory, &memoryFd);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(memoryFd, kInvalidFd);

    {
        GLMemoryObject memoryObject;
        GLint dedicatedMemory = GL_TRUE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        glImportMemoryFdEXT(memoryObject, deviceMemorySize, GL_HANDLE_TYPE_OPAQUE_FD_EXT, memoryFd);

        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, memoryObject, 0);

        GLSemaphore glAcquireSemaphore;
        glImportSemaphoreFdEXT(glAcquireSemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT,
                               acquireSemaphoreFd);

        helper.releaseImageAndSignalSemaphore(image, VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_GENERAL, vkAcquireSemaphore);

        const GLuint barrierTextures[] = {
            texture,
        };
        constexpr uint32_t textureBarriersCount = std::extent<decltype(barrierTextures)>();
        const GLenum textureSrcLayouts[]        = {
            GL_LAYOUT_GENERAL_EXT,
        };
        constexpr uint32_t textureSrcLayoutsCount = std::extent<decltype(textureSrcLayouts)>();
        static_assert(textureBarriersCount == textureSrcLayoutsCount,
                      "barrierTextures and textureSrcLayouts must be the same length");
        glWaitSemaphoreEXT(glAcquireSemaphore, 0, nullptr, textureBarriersCount, barrierTextures,
                           textureSrcLayouts);

        GLFramebuffer framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLSemaphore glReleaseSemaphore;
        glImportSemaphoreFdEXT(glReleaseSemaphore, GL_HANDLE_TYPE_OPAQUE_FD_EXT,
                               releaseSemaphoreFd);

        const GLenum textureDstLayouts[] = {
            GL_LAYOUT_TRANSFER_SRC_EXT,
        };
        constexpr uint32_t textureDstLayoutsCount = std::extent<decltype(textureSrcLayouts)>();
        static_assert(textureBarriersCount == textureDstLayoutsCount,
                      "barrierTextures and textureDstLayouts must be the same length");
        glSignalSemaphoreEXT(glReleaseSemaphore, 0, nullptr, textureBarriersCount, barrierTextures,
                             textureDstLayouts);

        helper.waitSemaphoreAndAcquireImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            vkReleaseSemaphore);
        uint8_t pixels[4];
        VkOffset3D offset = {};
        VkExtent3D extent = {1, 1, 1};
        helper.readPixels(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, format, offset, extent,
                          pixels, sizeof(pixels));

        EXPECT_NEAR(0x80, pixels[0], 1);
        EXPECT_NEAR(0x80, pixels[1], 1);
        EXPECT_NEAR(0x80, pixels[2], 1);
        EXPECT_NEAR(0x80, pixels[3], 1);
    }

    EXPECT_GL_NO_ERROR();

    vkDeviceWaitIdle(helper.getDevice());
    vkDestroyImage(helper.getDevice(), image, nullptr);
    vkDestroySemaphore(helper.getDevice(), vkAcquireSemaphore, nullptr);
    vkDestroySemaphore(helper.getDevice(), vkReleaseSemaphore, nullptr);
    vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
}

// Test creating and clearing RGBA8 texture in zircon vmo with acquire/release.
TEST_P(VulkanExternalImageTest, ShouldClearZirconVmoWithSemaphores)
{
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_memory_object_fuchsia"));
    ANGLE_SKIP_TEST_IF(!EnsureGLExtensionEnabled("GL_ANGLE_semaphore_fuchsia"));

    VulkanExternalHelper helper;
    helper.initialize(isSwiftshader(), enableDebugLayers());

    VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
    ANGLE_SKIP_TEST_IF(
        !helper.canCreateImageZirconVmo(format, VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL));
    ANGLE_SKIP_TEST_IF(!helper.canCreateSemaphoreZirconEvent());

    VkSemaphore vkAcquireSemaphore = VK_NULL_HANDLE;
    VkResult result                = helper.createSemaphoreZirconEvent(&vkAcquireSemaphore);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_TRUE(vkAcquireSemaphore != VK_NULL_HANDLE);

    VkSemaphore vkReleaseSemaphore = VK_NULL_HANDLE;
    result                         = helper.createSemaphoreZirconEvent(&vkReleaseSemaphore);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_TRUE(vkReleaseSemaphore != VK_NULL_HANDLE);

    zx_handle_t acquireSemaphoreHandle = ZX_HANDLE_INVALID;
    result = helper.exportSemaphoreZirconEvent(vkAcquireSemaphore, &acquireSemaphoreHandle);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(acquireSemaphoreHandle, ZX_HANDLE_INVALID);

    zx_handle_t releaseSemaphoreHandle = ZX_HANDLE_INVALID;
    result = helper.exportSemaphoreZirconEvent(vkReleaseSemaphore, &releaseSemaphoreHandle);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(releaseSemaphoreHandle, ZX_HANDLE_INVALID);

    VkImage image                 = VK_NULL_HANDLE;
    VkDeviceMemory deviceMemory   = VK_NULL_HANDLE;
    VkDeviceSize deviceMemorySize = 0;

    VkExtent3D extent = {1, 1, 1};
    result =
        helper.createImage2DZirconVmo(format, extent, &image, &deviceMemory, &deviceMemorySize);
    EXPECT_EQ(result, VK_SUCCESS);

    zx_handle_t memoryHandle = ZX_HANDLE_INVALID;
    result                   = helper.exportMemoryZirconVmo(deviceMemory, &memoryHandle);
    EXPECT_EQ(result, VK_SUCCESS);
    EXPECT_NE(memoryHandle, ZX_HANDLE_INVALID);

    {
        GLMemoryObject memoryObject;
        GLint dedicatedMemory = GL_TRUE;
        glMemoryObjectParameterivEXT(memoryObject, GL_DEDICATED_MEMORY_OBJECT_EXT,
                                     &dedicatedMemory);
        glImportMemoryZirconHandleANGLE(memoryObject, deviceMemorySize,
                                        GL_HANDLE_TYPE_ZIRCON_VMO_ANGLE, memoryHandle);

        GLTexture texture;
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexStorageMem2DEXT(GL_TEXTURE_2D, 1, GL_RGBA8, 1, 1, memoryObject, 0);

        GLSemaphore glAcquireSemaphore;
        glImportSemaphoreZirconHandleANGLE(glAcquireSemaphore, GL_HANDLE_TYPE_ZIRCON_EVENT_ANGLE,
                                           acquireSemaphoreHandle);

        helper.releaseImageAndSignalSemaphore(image, VK_IMAGE_LAYOUT_UNDEFINED,
                                              VK_IMAGE_LAYOUT_GENERAL, vkAcquireSemaphore);

        const GLuint barrierTextures[] = {
            texture,
        };
        constexpr uint32_t textureBarriersCount = std::extent<decltype(barrierTextures)>();
        const GLenum textureSrcLayouts[]        = {
            GL_LAYOUT_GENERAL_EXT,
        };
        constexpr uint32_t textureSrcLayoutsCount = std::extent<decltype(textureSrcLayouts)>();
        static_assert(textureBarriersCount == textureSrcLayoutsCount,
                      "barrierTextures and textureSrcLayouts must be the same length");
        glWaitSemaphoreEXT(glAcquireSemaphore, 0, nullptr, textureBarriersCount, barrierTextures,
                           textureSrcLayouts);

        GLFramebuffer framebuffer;
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glClearColor(0.5f, 0.5f, 0.5f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);

        GLSemaphore glReleaseSemaphore;
        glImportSemaphoreZirconHandleANGLE(glReleaseSemaphore, GL_HANDLE_TYPE_ZIRCON_EVENT_ANGLE,
                                           releaseSemaphoreHandle);

        const GLenum textureDstLayouts[] = {
            GL_LAYOUT_TRANSFER_SRC_EXT,
        };
        constexpr uint32_t textureDstLayoutsCount = std::extent<decltype(textureSrcLayouts)>();
        static_assert(textureBarriersCount == textureDstLayoutsCount,
                      "barrierTextures and textureDstLayouts must be the same length");
        glSignalSemaphoreEXT(glReleaseSemaphore, 0, nullptr, textureBarriersCount, barrierTextures,
                             textureDstLayouts);

        helper.waitSemaphoreAndAcquireImage(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                            vkReleaseSemaphore);
        uint8_t pixels[4];
        VkOffset3D offset = {};
        VkExtent3D extent = {1, 1, 1};
        helper.readPixels(image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, format, offset, extent,
                          pixels, sizeof(pixels));

        EXPECT_NEAR(0x80, pixels[0], 1);
        EXPECT_NEAR(0x80, pixels[1], 1);
        EXPECT_NEAR(0x80, pixels[2], 1);
        EXPECT_NEAR(0x80, pixels[3], 1);
    }

    EXPECT_GL_NO_ERROR();

    vkDeviceWaitIdle(helper.getDevice());
    vkDestroyImage(helper.getDevice(), image, nullptr);
    vkDestroySemaphore(helper.getDevice(), vkAcquireSemaphore, nullptr);
    vkDestroySemaphore(helper.getDevice(), vkReleaseSemaphore, nullptr);
    vkFreeMemory(helper.getDevice(), deviceMemory, nullptr);
}

// Use this to select which configurations (e.g. which renderer, which GLES major version) these
// tests should be run against.
ANGLE_INSTANTIATE_TEST_ES2_AND_ES3(VulkanExternalImageTest);

}  // namespace angle

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// android_util.cpp: Utilities for the using the Android platform

#include "libANGLE/renderer/gl/egl/android/android_util.h"

namespace rx
{

namespace
{

// Taken from android/hardware_buffer.h
// https://android.googlesource.com/platform/frameworks/native/+/master/libs/nativewindow/include/android/hardware_buffer.h

// AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM AHARDWAREBUFFER_FORMAT_B4G4R4A4_UNORM,
// AHARDWAREBUFFER_FORMAT_B5G5R5A1_UNORM formats were deprecated and re-added explicitly.

// clang-format off
/**
 * Buffer pixel formats.
 */
enum {
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8A8_UNORM
     *   OpenGL ES: GL_RGBA8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM           = 1,
    /**
     * 32 bits per pixel, 8 bits per channel format where alpha values are
     * ignored (always opaque).
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8A8_UNORM
     *   OpenGL ES: GL_RGB8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM           = 2,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R8G8B8_UNORM
     *   OpenGL ES: GL_RGB8
     */
    AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM             = 3,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R5G6B5_UNORM_PACK16
     *   OpenGL ES: GL_RGB565
     */
    AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM             = 4,
    AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM           = 5,
    AHARDWAREBUFFER_FORMAT_B4G4R4A4_UNORM           = 6,
    AHARDWAREBUFFER_FORMAT_B5G5R5A1_UNORM           = 7,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_R16G16B16A16_SFLOAT
     *   OpenGL ES: GL_RGBA16F
     */
    AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT       = 0x16,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_A2B10G10R10_UNORM_PACK32
     *   OpenGL ES: GL_RGB10_A2
     */
    AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM        = 0x2b,
    /**
     * An opaque binary blob format that must have height 1, with width equal to
     * the buffer size in bytes.
     */
    AHARDWAREBUFFER_FORMAT_BLOB                     = 0x21,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D16_UNORM
     *   OpenGL ES: GL_DEPTH_COMPONENT16
     */
    AHARDWAREBUFFER_FORMAT_D16_UNORM                = 0x30,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_X8_D24_UNORM_PACK32
     *   OpenGL ES: GL_DEPTH_COMPONENT24
     */
    AHARDWAREBUFFER_FORMAT_D24_UNORM                = 0x31,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D24_UNORM_S8_UINT
     *   OpenGL ES: GL_DEPTH24_STENCIL8
     */
    AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT        = 0x32,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D32_SFLOAT
     *   OpenGL ES: GL_DEPTH_COMPONENT32F
     */
    AHARDWAREBUFFER_FORMAT_D32_FLOAT                = 0x33,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_D32_SFLOAT_S8_UINT
     *   OpenGL ES: GL_DEPTH32F_STENCIL8
     */
    AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT        = 0x34,
    /**
     * Corresponding formats:
     *   Vulkan: VK_FORMAT_S8_UINT
     *   OpenGL ES: GL_STENCIL_INDEX8
     */
    AHARDWAREBUFFER_FORMAT_S8_UINT                  = 0x35,
};
// clang-format on

}  // anonymous namespace

namespace android
{
GLenum NativePixelFormatToGLInternalFormat(int pixelFormat)
{
    switch (pixelFormat)
    {
        case AHARDWAREBUFFER_FORMAT_R8G8B8A8_UNORM:
            return GL_RGBA8;
        case AHARDWAREBUFFER_FORMAT_R8G8B8X8_UNORM:
            return GL_RGB8;
        case AHARDWAREBUFFER_FORMAT_R8G8B8_UNORM:
            return GL_RGB8;
        case AHARDWAREBUFFER_FORMAT_R5G6B5_UNORM:
            return GL_RGB565;
        case AHARDWAREBUFFER_FORMAT_B8G8R8A8_UNORM:
            return GL_BGRA8_EXT;
        case AHARDWAREBUFFER_FORMAT_B4G4R4A4_UNORM:
            return GL_RGBA4;
        case AHARDWAREBUFFER_FORMAT_B5G5R5A1_UNORM:
            return GL_RGB5_A1;
        case AHARDWAREBUFFER_FORMAT_R16G16B16A16_FLOAT:
            return GL_RGBA16F;
        case AHARDWAREBUFFER_FORMAT_R10G10B10A2_UNORM:
            return GL_RGB10_A2;
        case AHARDWAREBUFFER_FORMAT_BLOB:
            return GL_NONE;
        case AHARDWAREBUFFER_FORMAT_D16_UNORM:
            return GL_DEPTH_COMPONENT16;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM:
            return GL_DEPTH_COMPONENT24;
        case AHARDWAREBUFFER_FORMAT_D24_UNORM_S8_UINT:
            return GL_DEPTH24_STENCIL8;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT:
            return GL_DEPTH_COMPONENT32F;
        case AHARDWAREBUFFER_FORMAT_D32_FLOAT_S8_UINT:
            return GL_DEPTH32F_STENCIL8;
        case AHARDWAREBUFFER_FORMAT_S8_UINT:
            return GL_STENCIL_INDEX8;
        default:
            return GL_NONE;
    }
}
}
}

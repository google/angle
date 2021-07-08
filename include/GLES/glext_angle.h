//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// glext_angle.h: ANGLE modifications to the glext.h header file.
//   Currently we don't include this file directly, we patch glext.h
//   to include it implicitly so it is visible throughout our code.

#ifndef INCLUDE_GLES_GLEXT_ANGLE_H_
#define INCLUDE_GLES_GLEXT_ANGLE_H_

// clang-format off

// clang-format on

#ifndef GL_ANGLE_yuv_internal_format
#define GL_ANGLE_yuv_internal_format

// YUV formats introduced by GL_ANGLE_yuv_internal_format
// 8-bit YUV formats
#define GL_G8_B8R8_2PLANE_420_UNORM_ANGLE 0x96B1
#define GL_G8_B8_R8_3PLANE_420_UNORM_ANGLE 0x96B2

// 10-bit YUV formats
#define GL_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16_ANGLE 0x96B3
#define GL_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16_ANGLE 0x96B4

// 12-bit YUV formats
#define GL_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16_ANGLE 0x96B5
#define GL_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16_ANGLE 0x96B6

// 16-bit YUV formats
#define GL_G16_B16R16_2PLANE_420_UNORM_ANGLE 0x96B7
#define GL_G16_B16_R16_3PLANE_420_UNORM_ANGLE 0x96B8

#endif /* GL_ANGLE_yuv_internal_format */

#endif  // INCLUDE_GLES_GLEXT_ANGLE_H_

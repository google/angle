//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Format:
//   A universal description of typed GPU storage. Across multiple
//   renderer back-ends, there are common formats and some distinct
//   permutations, this enum encapsulates them all. Formats apply to
//   textures, but could also apply to any typed data.

#ifndef LIBANGLE_RENDERER_FORMAT_H_
#define LIBANGLE_RENDERER_FORMAT_H_

namespace angle
{

// TODO(jmadill): It would be nice if we could auto-generate this.

enum class Format
{
    NONE,
    A8_UNORM,
    B4G4R4A4_UNORM,
    B5G5R5A1_UNORM,
    B5G6R5_UNORM,
    B8G8R8A8_UNORM,
    BC1_UNORM,
    BC2_UNORM,
    BC3_UNORM,
    D16_UNORM,
    D24_UNORM_S8_UINT,
    D32_FLOAT,
    D32_FLOAT_S8X24_UINT,
    R10G10B10A2_UINT,
    R10G10B10A2_UNORM,
    R11G11B10_FLOAT,
    R16G16B16A16_FLOAT,
    R16G16B16A16_SINT,
    R16G16B16A16_SNORM,
    R16G16B16A16_UINT,
    R16G16B16A16_UNORM,
    R16G16_FLOAT,
    R16G16_SINT,
    R16G16_SNORM,
    R16G16_UINT,
    R16G16_UNORM,
    R16_FLOAT,
    R16_SINT,
    R16_SNORM,
    R16_UINT,
    R16_UNORM,
    R32G32B32A32_FLOAT,
    R32G32B32A32_SINT,
    R32G32B32A32_UINT,
    R32G32_FLOAT,
    R32G32_SINT,
    R32G32_UINT,
    R32_FLOAT,
    R32_SINT,
    R32_UINT,
    R8G8B8A8_SINT,
    R8G8B8A8_SNORM,
    R8G8B8A8_UINT,
    R8G8B8A8_UNORM,
    R8G8B8A8_UNORM_SRGB,
    R8G8_SINT,
    R8G8_SNORM,
    R8G8_UINT,
    R8G8_UNORM,
    R8_SINT,
    R8_SNORM,
    R8_UINT,
    R8_UNORM,
    R9G9B9E5_SHAREDEXP,
};

}  // namespace angle

#endif  // LIBANGLE_RENDERER_FORMAT_H_

//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// angletypes.h : Defines a variety of structures and enum types that are used throughout libGLESv2

#ifndef LIBGLESV2_ANGLETYPES_H_
#define LIBGLESV2_ANGLETYPES_H_

namespace gl
{

enum TextureType
{
    TEXTURE_2D,
    TEXTURE_CUBE,

    TEXTURE_TYPE_COUNT,
    TEXTURE_UNKNOWN
};

enum SamplerType
{
    SAMPLER_PIXEL,
    SAMPLER_VERTEX
};

}

#endif // LIBGLESV2_ANGLETYPES_H_

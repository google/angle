//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureImpl.cpp: Defines the abstract rx::TextureImpl classes.

#include "libANGLE/renderer/TextureImpl.h"

namespace rx
{

TextureImpl::TextureImpl(const gl::TextureState &state) : mState(state)
{
}

TextureImpl::~TextureImpl()
{
}

gl::Error TextureImpl::onDestroy(const gl::Context *context)
{
    return gl::NoError();
}

gl::Error TextureImpl::copyTexture(const gl::Context *context,
                                   const gl::ImageIndex &index,
                                   GLenum internalFormat,
                                   GLenum type,
                                   size_t sourceLevel,
                                   bool unpackFlipY,
                                   bool unpackPremultiplyAlpha,
                                   bool unpackUnmultiplyAlpha,
                                   const gl::Texture *source)
{
    UNREACHABLE();
    return gl::InternalError() << "CHROMIUM_copy_texture exposed but not implemented.";
}

gl::Error TextureImpl::copySubTexture(const gl::Context *context,
                                      const gl::ImageIndex &index,
                                      const gl::Offset &destOffset,
                                      size_t sourceLevel,
                                      const gl::Box &sourceBox,
                                      bool unpackFlipY,
                                      bool unpackPremultiplyAlpha,
                                      bool unpackUnmultiplyAlpha,
                                      const gl::Texture *source)
{
    UNREACHABLE();
    return gl::InternalError() << "CHROMIUM_copy_texture exposed but not implemented.";
}

gl::Error TextureImpl::copyCompressedTexture(const gl::Context *context, const gl::Texture *source)
{
    UNREACHABLE();
    return gl::InternalError() << "CHROMIUM_copy_compressed_texture exposed but not implemented.";
}

gl::Error TextureImpl::copy3DTexture(const gl::Context *context,
                                     gl::TextureTarget target,
                                     GLenum internalFormat,
                                     GLenum type,
                                     size_t sourceLevel,
                                     size_t destLevel,
                                     bool unpackFlipY,
                                     bool unpackPremultiplyAlpha,
                                     bool unpackUnmultiplyAlpha,
                                     const gl::Texture *source)
{
    UNREACHABLE();
    return gl::InternalError() << "ANGLE_copy_texture_3d exposed but not implemented.";
}

gl::Error TextureImpl::copy3DSubTexture(const gl::Context *context,
                                        const gl::TextureTarget target,
                                        const gl::Offset &destOffset,
                                        size_t sourceLevel,
                                        size_t destLevel,
                                        const gl::Box &srcBox,
                                        bool unpackFlipY,
                                        bool unpackPremultiplyAlpha,
                                        bool unpackUnmultiplyAlpha,
                                        const gl::Texture *source)
{
    UNREACHABLE();
    return gl::InternalError() << "ANGLE_copy_texture_3d exposed but not implemented.";
}

}  // namespace rx

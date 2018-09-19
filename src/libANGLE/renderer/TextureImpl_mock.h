//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// TextureImpl_mock.h: Defines a mock of the TextureImpl class.

#ifndef LIBANGLE_RENDERER_TEXTUREIMPLMOCK_H_
#define LIBANGLE_RENDERER_TEXTUREIMPLMOCK_H_

#include "gmock/gmock.h"

#include "libANGLE/renderer/TextureImpl.h"

namespace rx
{

class MockTextureImpl : public TextureImpl
{
  public:
    MockTextureImpl() : TextureImpl(mMockState), mMockState(gl::TextureType::_2D) {}
    virtual ~MockTextureImpl() { destructor(); }
    MOCK_METHOD8(setImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           GLenum,
                           const gl::Extents &,
                           GLenum,
                           GLenum,
                           const gl::PixelUnpackState &,
                           const uint8_t *));
    MOCK_METHOD8(setSubImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           const gl::Box &,
                           GLenum,
                           GLenum,
                           const gl::PixelUnpackState &,
                           gl::Buffer *,
                           const uint8_t *));
    MOCK_METHOD7(setCompressedImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           GLenum,
                           const gl::Extents &,
                           const gl::PixelUnpackState &,
                           size_t,
                           const uint8_t *));
    MOCK_METHOD7(setCompressedSubImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           const gl::Box &,
                           GLenum,
                           const gl::PixelUnpackState &,
                           size_t,
                           const uint8_t *));
    MOCK_METHOD5(copyImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           const gl::Rectangle &,
                           GLenum,
                           gl::Framebuffer *));
    MOCK_METHOD5(copySubImage,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           const gl::Offset &,
                           const gl::Rectangle &,
                           gl::Framebuffer *));
    MOCK_METHOD9(copyTexture,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           GLenum,
                           GLenum,
                           size_t,
                           bool,
                           bool,
                           bool,
                           const gl::Texture *));
    MOCK_METHOD9(copySubTexture,
                 gl::Error(const gl::Context *,
                           const gl::ImageIndex &,
                           const gl::Offset &,
                           size_t,
                           const gl::Box &,
                           bool,
                           bool,
                           bool,
                           const gl::Texture *));
    MOCK_METHOD2(copyCompressedTexture, gl::Error(const gl::Context *, const gl::Texture *source));
    MOCK_METHOD5(
        setStorage,
        gl::Error(const gl::Context *, gl::TextureType, size_t, GLenum, const gl::Extents &));
    MOCK_METHOD4(setImageExternal,
                 gl::Error(const gl::Context *,
                           gl::TextureType,
                           egl::Stream *,
                           const egl::Stream::GLTextureDescription &));
    MOCK_METHOD3(setEGLImageTarget, gl::Error(const gl::Context *, gl::TextureType, egl::Image *));
    MOCK_METHOD1(generateMipmap, gl::Error(const gl::Context *));
    MOCK_METHOD2(bindTexImage, gl::Error(const gl::Context *, egl::Surface *));
    MOCK_METHOD1(releaseTexImage, gl::Error(const gl::Context *));

    MOCK_METHOD4(getAttachmentRenderTarget,
                 gl::Error(const gl::Context *,
                           GLenum,
                           const gl::ImageIndex &,
                           FramebufferAttachmentRenderTarget **));

    MOCK_METHOD6(
        setStorageMultisample,
        gl::Error(const gl::Context *, gl::TextureType, GLsizei, GLint, const gl::Extents &, bool));

    MOCK_METHOD2(setBaseLevel, gl::Error(const gl::Context *, GLuint));

    MOCK_METHOD2(syncState, gl::Error(const gl::Context *, const gl::Texture::DirtyBits &));

    MOCK_METHOD0(destructor, void());

  protected:
    gl::TextureState mMockState;
};

}

#endif // LIBANGLE_RENDERER_TEXTUREIMPLMOCK_H_

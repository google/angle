//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.cpp: Implements the egl::Image class representing the EGLimage object.

#include "libANGLE/Image.h"

#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/renderer/EGLImplFactory.h"
#include "libANGLE/renderer/ImageImpl.h"

namespace egl
{

namespace
{
gl::ImageIndex GetImageIndex(EGLenum eglTarget, const egl::AttributeMap &attribs)
{
    if (eglTarget == EGL_GL_RENDERBUFFER)
    {
        return gl::ImageIndex::MakeInvalid();
    }

    GLenum target = egl_gl::EGLImageTargetToGLTextureTarget(eglTarget);
    GLint mip     = static_cast<GLint>(attribs.get(EGL_GL_TEXTURE_LEVEL_KHR, 0));
    GLint layer   = static_cast<GLint>(attribs.get(EGL_GL_TEXTURE_ZOFFSET_KHR, 0));

    if (target == GL_TEXTURE_3D)
    {
        return gl::ImageIndex::Make3D(mip, layer);
    }
    else
    {
        ASSERT(layer == 0);
        return gl::ImageIndex::MakeGeneric(target, mip);
    }
}
}  // anonymous namespace

ImageSibling::ImageSibling(GLuint id) : RefCountObject(id), mSourcesOf(), mTargetOf()
{
}

ImageSibling::~ImageSibling()
{
    // EGL images should hold a ref to their targets and siblings, a Texture should not be deletable
    // while it is attached to an EGL image.
    ASSERT(mSourcesOf.empty());
    orphanImages();
}

void ImageSibling::setTargetImage(egl::Image *imageTarget)
{
    ASSERT(imageTarget != nullptr);
    mTargetOf.set(imageTarget);
    imageTarget->addTargetSibling(this);
}

gl::Error ImageSibling::orphanImages()
{
    if (mTargetOf.get() != nullptr)
    {
        // Can't be a target and have sources.
        ASSERT(mSourcesOf.empty());

        ANGLE_TRY(mTargetOf->orphanSibling(this));
        mTargetOf.set(nullptr);
    }
    else
    {
        for (auto &sourceImage : mSourcesOf)
        {
            ANGLE_TRY(sourceImage->orphanSibling(this));
        }
        mSourcesOf.clear();
    }

    return gl::NoError();
}

void ImageSibling::addImageSource(egl::Image *imageSource)
{
    ASSERT(imageSource != nullptr);
    mSourcesOf.insert(imageSource);
}

void ImageSibling::removeImageSource(egl::Image *imageSource)
{
    ASSERT(mSourcesOf.find(imageSource) != mSourcesOf.end());
    mSourcesOf.erase(imageSource);
}

ImageState::ImageState(EGLenum target, ImageSibling *buffer, const AttributeMap &attribs)
    : imageIndex(GetImageIndex(target, attribs)), source(), targets()
{
    source.set(buffer);
}

Image::Image(rx::EGLImplFactory *factory,
             EGLenum target,
             ImageSibling *buffer,
             const AttributeMap &attribs)
    : RefCountObject(0),
      mState(target, buffer, attribs),
      mImplementation(factory->createImage(mState, target, attribs)),
      mFormat(gl::Format::Invalid()),
      mWidth(0),
      mHeight(0),
      mSamples(0)
{
    ASSERT(mImplementation != nullptr);
    ASSERT(buffer != nullptr);

    mState.source->addImageSource(this);

    if (IsTextureTarget(target))
    {
        gl::Texture *texture = rx::GetAs<gl::Texture>(mState.source.get());
        GLenum textureTarget = egl_gl::EGLImageTargetToGLTextureTarget(target);
        size_t level         = attribs.get(EGL_GL_TEXTURE_LEVEL_KHR, 0);
        mFormat              = texture->getFormat(textureTarget, level);
        mWidth               = texture->getWidth(textureTarget, level);
        mHeight              = texture->getHeight(textureTarget, level);
        mSamples             = 0;
    }
    else if (IsRenderbufferTarget(target))
    {
        gl::Renderbuffer *renderbuffer = rx::GetAs<gl::Renderbuffer>(mState.source.get());
        mFormat                        = renderbuffer->getFormat();
        mWidth                         = renderbuffer->getWidth();
        mHeight                        = renderbuffer->getHeight();
        mSamples                       = renderbuffer->getSamples();
    }
    else
    {
        UNREACHABLE();
    }
}

Image::~Image()
{
    SafeDelete(mImplementation);

    // All targets should hold a ref to the egl image and it should not be deleted until there are
    // no siblings left.
    ASSERT(mState.targets.empty());

    // Tell the source that it is no longer used by this image
    if (mState.source.get() != nullptr)
    {
        mState.source->removeImageSource(this);
        mState.source.set(nullptr);
    }
}

void Image::addTargetSibling(ImageSibling *sibling)
{
    mState.targets.insert(sibling);
}

gl::Error Image::orphanSibling(ImageSibling *sibling)
{
    // notify impl
    ANGLE_TRY(mImplementation->orphan(sibling));

    if (mState.source.get() == sibling)
    {
        // If the sibling is the source, it cannot be a target.
        ASSERT(mState.targets.find(sibling) == mState.targets.end());
        mState.source.set(nullptr);
    }
    else
    {
        mState.targets.erase(sibling);
    }

    return gl::NoError();
}

const gl::Format &Image::getFormat() const
{
    return mFormat;
}

size_t Image::getWidth() const
{
    return mWidth;
}

size_t Image::getHeight() const
{
    return mHeight;
}

size_t Image::getSamples() const
{
    return mSamples;
}

rx::ImageImpl *Image::getImplementation() const
{
    return mImplementation;
}

Error Image::initialize()
{
    return mImplementation->initialize();
}

}  // namespace egl

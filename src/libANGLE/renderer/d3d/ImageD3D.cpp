//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Image.h: Implements the rx::Image class, an abstract base class for the
// renderer-specific classes which will define the interface to the underlying
// surfaces or resources.

#include "libANGLE/renderer/d3d/ImageD3D.h"

#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/renderer/d3d/FramebufferD3D.h"

namespace rx
{

ImageD3D::ImageD3D()
{
}

ImageD3D *ImageD3D::makeImageD3D(Image *img)
{
    ASSERT(HAS_DYNAMIC_TYPE(ImageD3D*, img));
    return static_cast<ImageD3D*>(img);
}

gl::Error ImageD3D::copy(GLint xoffset, GLint yoffset, GLint zoffset, const gl::Rectangle &area, gl::Framebuffer *source)
{
    gl::FramebufferAttachment *colorbuffer = source->getReadColorbuffer();
    ASSERT(colorbuffer);

    RenderTarget *renderTarget = NULL;
    gl::Error error = GetAttachmentRenderTarget(colorbuffer, &renderTarget);
    if (error.isError())
    {
        return error;
    }

    ASSERT(renderTarget);
    return copy(xoffset, yoffset, zoffset, area, renderTarget);
}

}

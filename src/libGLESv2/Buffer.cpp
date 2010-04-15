//
// Copyright (c) 2002-2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Buffer.cpp: Implements the gl::Buffer class, representing storage of vertex and/or
// index data. Implements GL buffer objects and related functionality.
// [OpenGL ES 2.0.24] section 2.9 page 21.

#include "Buffer.h"

#include <cstdlib>
#include <limits>
#include <utility>

#include "common/debug.h"
#include "geometry/backend.h"

namespace gl
{

Buffer::Buffer(BufferBackEnd *backEnd)
    : mBackEnd(backEnd), mIdentityTranslation(NULL)
{
}

Buffer::~Buffer()
{
    delete mIdentityTranslation;
}

GLenum Buffer::bufferData(const void* data, GLsizeiptr size, GLenum usage)
{
    if (size < 0) return GL_INVALID_VALUE;

    const GLubyte* newdata = static_cast<const GLubyte*>(data);

    if (size != mContents.size())
    {
        // vector::resize only provides the basic exception guarantee, so use temporaries & swap to get the strong exception guarantee.
        // We don't want to risk having mContents and mIdentityTranslation that have different contents or even different sizes.
        std::vector<GLubyte> newContents;

        if (newdata != NULL)
        {
            newContents.assign(newdata, newdata + size);
        }
        else
        {
            newContents.assign(size, 0);
        }

        TranslatedVertexBuffer *newIdentityTranslation = mBackEnd->createVertexBuffer(size);

        // No exceptions allowed after this point.

        mContents.swap(newContents);

        delete mIdentityTranslation;
        mIdentityTranslation = newIdentityTranslation;
    }
    else if (newdata != NULL)
    {
        mContents.assign(newdata, newdata + size);
    }

    mUsage = usage;

    return copyToIdentityBuffer(0, size);
}

GLenum Buffer::bufferSubData(const void* data, GLsizeiptr size, GLintptr offset)
{
    if (size < 0 || offset < 0) return GL_INVALID_VALUE;
    if (std::numeric_limits<GLsizeiptr>::max() - offset < size) return GL_INVALID_VALUE;
    if (size + offset > static_cast<GLsizeiptr>(mContents.size())) return GL_INVALID_VALUE;

    const GLubyte *newdata = static_cast<const GLubyte*>(data);
    copy(newdata, newdata + size, mContents.begin() + offset);

    return copyToIdentityBuffer(offset, size);
}

GLenum Buffer::copyToIdentityBuffer(GLintptr offset, GLsizeiptr length)
{
    ASSERT(offset >= 0 && length >= 0);

    // This is a stalling map. Not great for performance.
    GLubyte *p = static_cast<GLubyte*>(mIdentityTranslation->map());

    memcpy(p + offset, &mContents[0] + offset, length);
    mIdentityTranslation->unmap();

    return GL_NO_ERROR;
}

}

//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BufferMtl.h:
//    Defines the class interface for BufferMtl, implementing BufferImpl.
//

#ifndef LIBANGLE_RENDERER_METAL_BUFFERMTL_H_
#define LIBANGLE_RENDERER_METAL_BUFFERMTL_H_

#import <Metal/Metal.h>

#include <utility>

#include "libANGLE/Buffer.h"
#include "libANGLE/Observer.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/BufferImpl.h"

namespace rx
{

class BufferMtl : public BufferImpl
{
  public:
    BufferMtl(const gl::BufferState &state);
    ~BufferMtl() override;
    void destroy(const gl::Context *context) override;

    angle::Result setData(const gl::Context *context,
                          gl::BufferBinding target,
                          const void *data,
                          size_t size,
                          gl::BufferUsage usage) override;
    angle::Result setSubData(const gl::Context *context,
                             gl::BufferBinding target,
                             const void *data,
                             size_t size,
                             size_t offset) override;
    angle::Result copySubData(const gl::Context *context,
                              BufferImpl *source,
                              GLintptr sourceOffset,
                              GLintptr destOffset,
                              GLsizeiptr size) override;
    angle::Result map(const gl::Context *context, GLenum access, void **mapPtr) override;
    angle::Result mapRange(const gl::Context *context,
                           size_t offset,
                           size_t length,
                           GLbitfield access,
                           void **mapPtr) override;
    angle::Result unmap(const gl::Context *context, GLboolean *result) override;

    angle::Result getIndexRange(const gl::Context *context,
                                gl::DrawElementsType type,
                                size_t offset,
                                size_t count,
                                bool primitiveRestartEnabled,
                                gl::IndexRange *outRange) override;
};

}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_BUFFERMTL_H_ */

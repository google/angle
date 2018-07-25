//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// QueryVk.h:
//    Defines the class interface for QueryVk, implementing QueryImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_QUERYVK_H_
#define LIBANGLE_RENDERER_VULKAN_QUERYVK_H_

#include "libANGLE/renderer/QueryImpl.h"

namespace rx
{

class QueryVk : public QueryImpl
{
  public:
    QueryVk(gl::QueryType type);
    ~QueryVk() override;

    gl::Error begin(const gl::Context *context) override;
    gl::Error end(const gl::Context *context) override;
    gl::Error queryCounter(const gl::Context *context) override;
    gl::Error getResult(const gl::Context *context, GLint *params) override;
    gl::Error getResult(const gl::Context *context, GLuint *params) override;
    gl::Error getResult(const gl::Context *context, GLint64 *params) override;
    gl::Error getResult(const gl::Context *context, GLuint64 *params) override;
    gl::Error isResultAvailable(const gl::Context *context, bool *available) override;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_QUERYVK_H_

//
// Copyright 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// angle_unittests_utils.h:
//   Helpers for mocking and unit testing.

#ifndef TESTS_ANGLE_UNITTESTS_UTILS_H_
#define TESTS_ANGLE_UNITTESTS_UTILS_H_

#include "libANGLE/renderer/ImplFactory.h"

namespace rx
{

// Useful when mocking a part of the ImplFactory class
class NullFactory : public ImplFactory
{
  public:
    NullFactory() {}

    // Shader creation
    CompilerImpl *createCompiler(const gl::Data &data) override { return nullptr; }
    ShaderImpl *createShader(GLenum type) override { return nullptr; }
    ProgramImpl *createProgram() override { return nullptr; }

    // Framebuffer creation
    DefaultAttachmentImpl *createDefaultAttachment(GLenum type, egl::Surface *surface) override { return nullptr; }
    FramebufferImpl *createDefaultFramebuffer(const gl::Framebuffer::Data &data) override { return nullptr; }
    FramebufferImpl *createFramebuffer(const gl::Framebuffer::Data &data) override { return nullptr; }

    // Texture creation
    TextureImpl *createTexture(GLenum target) override { return nullptr; }

    // Renderbuffer creation
    RenderbufferImpl *createRenderbuffer() override { return nullptr; }

    // Buffer creation
    BufferImpl *createBuffer() override { return nullptr; }

    // Vertex Array creation
    VertexArrayImpl *createVertexArray() override { return nullptr; }

    // Query and Fence creation
    QueryImpl *createQuery(GLenum type) override { return nullptr; }
    FenceNVImpl *createFenceNV() override { return nullptr; }
    FenceSyncImpl *createFenceSync() override { return nullptr; }

    // Transform Feedback creation
    TransformFeedbackImpl *createTransformFeedback() override { return nullptr; }

    DISALLOW_COPY_AND_ASSIGN(NullFactory);
};

}

#endif // TESTS_ANGLE_UNITTESTS_UTILS_H_

//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextVk.cpp:
//    Implements the class methods for ContextVk.
//

#include "libANGLE/renderer/vulkan/ContextVk.h"

#include "common/debug.h"

namespace rx
{

ContextVk::ContextVk(const gl::ContextState &state) : ContextImpl(state)
{
}

ContextVk::~ContextVk()
{
}

gl::Error ContextVk::initialize()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::flush()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::finish()
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::drawArrays(GLenum mode, GLint first, GLsizei count)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::drawArraysInstanced(GLenum mode,
                                         GLint first,
                                         GLsizei count,
                                         GLsizei instanceCount)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::drawElements(GLenum mode,
                                  GLsizei count,
                                  GLenum type,
                                  const GLvoid *indices,
                                  const gl::IndexRange &indexRange)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::drawElementsInstanced(GLenum mode,
                                           GLsizei count,
                                           GLenum type,
                                           const GLvoid *indices,
                                           GLsizei instances,
                                           const gl::IndexRange &indexRange)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

gl::Error ContextVk::drawRangeElements(GLenum mode,
                                       GLuint start,
                                       GLuint end,
                                       GLsizei count,
                                       GLenum type,
                                       const GLvoid *indices,
                                       const gl::IndexRange &indexRange)
{
    UNIMPLEMENTED();
    return gl::Error(GL_INVALID_OPERATION);
}

void ContextVk::notifyDeviceLost()
{
    UNIMPLEMENTED();
}

bool ContextVk::isDeviceLost() const
{
    UNIMPLEMENTED();
    return bool();
}

bool ContextVk::testDeviceLost()
{
    UNIMPLEMENTED();
    return bool();
}

bool ContextVk::testDeviceResettable()
{
    UNIMPLEMENTED();
    return bool();
}

std::string ContextVk::getVendorString() const
{
    UNIMPLEMENTED();
    return std::string();
}

std::string ContextVk::getRendererDescription() const
{
    UNIMPLEMENTED();
    return std::string();
}

void ContextVk::insertEventMarker(GLsizei length, const char *marker)
{
    UNIMPLEMENTED();
}

void ContextVk::pushGroupMarker(GLsizei length, const char *marker)
{
    UNIMPLEMENTED();
}

void ContextVk::popGroupMarker()
{
    UNIMPLEMENTED();
}

void ContextVk::syncState(const gl::State &state, const gl::State::DirtyBits &dirtyBits)
{
    UNIMPLEMENTED();
}

GLint ContextVk::getGPUDisjoint()
{
    UNIMPLEMENTED();
    return GLint();
}

GLint64 ContextVk::getTimestamp()
{
    UNIMPLEMENTED();
    return GLint64();
}

void ContextVk::onMakeCurrent(const gl::ContextState &data)
{
    UNIMPLEMENTED();
}

const gl::Caps &ContextVk::getNativeCaps() const
{
    UNIMPLEMENTED();
    static gl::Caps local;
    return local;
}

const gl::TextureCapsMap &ContextVk::getNativeTextureCaps() const
{
    UNIMPLEMENTED();
    static gl::TextureCapsMap local;
    return local;
}

const gl::Extensions &ContextVk::getNativeExtensions() const
{
    UNIMPLEMENTED();
    static gl::Extensions local;
    return local;
}

const gl::Limitations &ContextVk::getNativeLimitations() const
{
    UNIMPLEMENTED();
    static gl::Limitations local;
    return local;
}

CompilerImpl *ContextVk::createCompiler()
{
    UNIMPLEMENTED();
    return static_cast<CompilerImpl *>(0);
}

ShaderImpl *ContextVk::createShader(const gl::ShaderState &data)
{
    UNIMPLEMENTED();
    return static_cast<ShaderImpl *>(0);
}

ProgramImpl *ContextVk::createProgram(const gl::ProgramState &data)
{
    UNIMPLEMENTED();
    return static_cast<ProgramImpl *>(0);
}

FramebufferImpl *ContextVk::createFramebuffer(const gl::FramebufferState &data)
{
    UNIMPLEMENTED();
    return static_cast<FramebufferImpl *>(0);
}

TextureImpl *ContextVk::createTexture(const gl::TextureState &state)
{
    UNIMPLEMENTED();
    return static_cast<TextureImpl *>(0);
}

RenderbufferImpl *ContextVk::createRenderbuffer()
{
    UNIMPLEMENTED();
    return static_cast<RenderbufferImpl *>(0);
}

BufferImpl *ContextVk::createBuffer()
{
    UNIMPLEMENTED();
    return static_cast<BufferImpl *>(0);
}

VertexArrayImpl *ContextVk::createVertexArray(const gl::VertexArrayState &data)
{
    UNIMPLEMENTED();
    return static_cast<VertexArrayImpl *>(0);
}

QueryImpl *ContextVk::createQuery(GLenum type)
{
    UNIMPLEMENTED();
    return static_cast<QueryImpl *>(0);
}

FenceNVImpl *ContextVk::createFenceNV()
{
    UNIMPLEMENTED();
    return static_cast<FenceNVImpl *>(0);
}

FenceSyncImpl *ContextVk::createFenceSync()
{
    UNIMPLEMENTED();
    return static_cast<FenceSyncImpl *>(0);
}

TransformFeedbackImpl *ContextVk::createTransformFeedback()
{
    UNIMPLEMENTED();
    return static_cast<TransformFeedbackImpl *>(0);
}

SamplerImpl *ContextVk::createSampler()
{
    UNIMPLEMENTED();
    return static_cast<SamplerImpl *>(0);
}

}  // namespace rx

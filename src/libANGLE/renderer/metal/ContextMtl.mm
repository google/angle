//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextMtl.mm:
//    Implements the class methods for ContextMtl.
//

#include "libANGLE/renderer/metal/ContextMtl.h"

#include <TargetConditionals.h>

#include "common/debug.h"
#include "libANGLE/renderer/metal/BufferMtl.h"
#include "libANGLE/renderer/metal/CompilerMtl.h"
#include "libANGLE/renderer/metal/FrameBufferMtl.h"
#include "libANGLE/renderer/metal/ProgramMtl.h"
#include "libANGLE/renderer/metal/RenderBufferMtl.h"
#include "libANGLE/renderer/metal/RendererMtl.h"
#include "libANGLE/renderer/metal/ShaderMtl.h"
#include "libANGLE/renderer/metal/TextureMtl.h"
#include "libANGLE/renderer/metal/VertexArrayMtl.h"
#include "libANGLE/renderer/metal/mtl_common.h"

namespace rx
{

ContextMtl::ContextMtl(const gl::State &state, gl::ErrorSet *errorSet, RendererMtl *renderer)
    : ContextImpl(state, errorSet), mtl::Context(renderer)
{}

ContextMtl::~ContextMtl() {}

angle::Result ContextMtl::initialize()
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

void ContextMtl::onDestroy(const gl::Context *context) {}

// Flush and finish.
angle::Result ContextMtl::flush(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
angle::Result ContextMtl::finish(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

// Drawing methods.
angle::Result ContextMtl::drawArrays(const gl::Context *context,
                                     gl::PrimitiveMode mode,
                                     GLint first,
                                     GLsizei count)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
angle::Result ContextMtl::drawArraysInstanced(const gl::Context *context,
                                              gl::PrimitiveMode mode,
                                              GLint first,
                                              GLsizei count,
                                              GLsizei instanceCount)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result ContextMtl::drawArraysInstancedBaseInstance(const gl::Context *context,
                                                          gl::PrimitiveMode mode,
                                                          GLint first,
                                                          GLsizei count,
                                                          GLsizei instanceCount,
                                                          GLuint baseInstance)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result ContextMtl::drawElements(const gl::Context *context,
                                       gl::PrimitiveMode mode,
                                       GLsizei count,
                                       gl::DrawElementsType type,
                                       const void *indices)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
angle::Result ContextMtl::drawElementsInstanced(const gl::Context *context,
                                                gl::PrimitiveMode mode,
                                                GLsizei count,
                                                gl::DrawElementsType type,
                                                const void *indices,
                                                GLsizei instanceCount)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::drawElementsInstancedBaseVertexBaseInstance(const gl::Context *context,
                                                                      gl::PrimitiveMode mode,
                                                                      GLsizei count,
                                                                      gl::DrawElementsType type,
                                                                      const void *indices,
                                                                      GLsizei instances,
                                                                      GLint baseVertex,
                                                                      GLuint baseInstance)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::drawRangeElements(const gl::Context *context,
                                            gl::PrimitiveMode mode,
                                            GLuint start,
                                            GLuint end,
                                            GLsizei count,
                                            gl::DrawElementsType type,
                                            const void *indices)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::drawArraysIndirect(const gl::Context *context,
                                             gl::PrimitiveMode mode,
                                             const void *indirect)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::drawElementsIndirect(const gl::Context *context,
                                               gl::PrimitiveMode mode,
                                               gl::DrawElementsType type,
                                               const void *indirect)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

// Device loss
gl::GraphicsResetStatus ContextMtl::getResetStatus()
{
    return gl::GraphicsResetStatus::NoError;
}

// Vendor and description strings.
std::string ContextMtl::getVendorString() const
{
    return getRenderer()->getVendorString();
}
std::string ContextMtl::getRendererDescription() const
{
    return getRenderer()->getRendererDescription();
}

// EXT_debug_marker
void ContextMtl::insertEventMarker(GLsizei length, const char *marker) {}
void ContextMtl::pushGroupMarker(GLsizei length, const char *marker) {}
void ContextMtl::popGroupMarker() {}

// KHR_debug
void ContextMtl::pushDebugGroup(GLenum source, GLuint id, const std::string &message) {}
void ContextMtl::popDebugGroup() {}

// State sync with dirty bits.
angle::Result ContextMtl::syncState(const gl::Context *context,
                                    const gl::State::DirtyBits &dirtyBits,
                                    const gl::State::DirtyBits &bitMask)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}

// Disjoint timer queries
GLint ContextMtl::getGPUDisjoint()
{
    UNIMPLEMENTED();
    return 0;
}
GLint64 ContextMtl::getTimestamp()
{
    UNIMPLEMENTED();
    return 0;
}

// Context switching
angle::Result ContextMtl::onMakeCurrent(const gl::Context *context)
{
    UNIMPLEMENTED();
    return angle::Result::Continue;
}
angle::Result ContextMtl::onUnMakeCurrent(const gl::Context *context)
{
    return angle::Result::Continue;
}

// Native capabilities, unmodified by gl::Context.
gl::Caps ContextMtl::getNativeCaps() const
{
    UNIMPLEMENTED();
    return mNativeCaps;
}
const gl::TextureCapsMap &ContextMtl::getNativeTextureCaps() const
{
    UNIMPLEMENTED();
    return mNativeTextureCaps;
}
const gl::Extensions &ContextMtl::getNativeExtensions() const
{
    UNIMPLEMENTED();
    return mNativeExtensions;
}
const gl::Limitations &ContextMtl::getNativeLimitations() const
{
    return getRenderer()->getNativeLimitations();
}

// Shader creation
CompilerImpl *ContextMtl::createCompiler()
{
    return new CompilerMtl();
}
ShaderImpl *ContextMtl::createShader(const gl::ShaderState &state)
{
    return new ShaderMtl(state);
}
ProgramImpl *ContextMtl::createProgram(const gl::ProgramState &state)
{
    return new ProgramMtl(state);
}

// Framebuffer creation
FramebufferImpl *ContextMtl::createFramebuffer(const gl::FramebufferState &state)
{
    return new FramebufferMtl(state);
}

// Texture creation
TextureImpl *ContextMtl::createTexture(const gl::TextureState &state)
{
    return new TextureMtl(state);
}

// Renderbuffer creation
RenderbufferImpl *ContextMtl::createRenderbuffer(const gl::RenderbufferState &state)
{
    return new RenderbufferMtl(state);
}

// Buffer creation
BufferImpl *ContextMtl::createBuffer(const gl::BufferState &state)
{
    return new BufferMtl(state);
}

// Vertex Array creation
VertexArrayImpl *ContextMtl::createVertexArray(const gl::VertexArrayState &state)
{
    return new VertexArrayMtl(state);
}

// Query and Fence creation
QueryImpl *ContextMtl::createQuery(gl::QueryType type)
{
    UNIMPLEMENTED();
    return nullptr;
}
FenceNVImpl *ContextMtl::createFenceNV()
{
    UNIMPLEMENTED();
    return nullptr;
}
SyncImpl *ContextMtl::createSync()
{
    UNIMPLEMENTED();
    return nullptr;
}

// Transform Feedback creation
TransformFeedbackImpl *ContextMtl::createTransformFeedback(const gl::TransformFeedbackState &state)
{
    UNIMPLEMENTED();
    return nullptr;
}

// Sampler object creation
SamplerImpl *ContextMtl::createSampler(const gl::SamplerState &state)
{
    UNIMPLEMENTED();
    return nullptr;
}

// Program Pipeline object creation
ProgramPipelineImpl *ContextMtl::createProgramPipeline(const gl::ProgramPipelineState &data)
{
    UNIMPLEMENTED();
    return nullptr;
}

// Path object creation
std::vector<PathImpl *> ContextMtl::createPaths(GLsizei)
{
    UNIMPLEMENTED();
    return std::vector<PathImpl *>();
}

// Memory object creation.
MemoryObjectImpl *ContextMtl::createMemoryObject()
{
    UNIMPLEMENTED();
    return nullptr;
}

// Semaphore creation.
SemaphoreImpl *ContextMtl::createSemaphore()
{
    UNIMPLEMENTED();
    return nullptr;
}

OverlayImpl *ContextMtl::createOverlay(const gl::OverlayState &state)
{
    UNIMPLEMENTED();
    return nullptr;
}

angle::Result ContextMtl::dispatchCompute(const gl::Context *context,
                                          GLuint numGroupsX,
                                          GLuint numGroupsY,
                                          GLuint numGroupsZ)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::dispatchComputeIndirect(const gl::Context *context, GLintptr indirect)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result ContextMtl::memoryBarrier(const gl::Context *context, GLbitfield barriers)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result ContextMtl::memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers)
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

// override mtl::ErrorHandler
void ContextMtl::handleError(GLenum glErrorCode,
                             const char *file,
                             const char *function,
                             unsigned int line)
{
    std::stringstream errorStream;
    errorStream << "Metal backend encountered an error. Code=" << glErrorCode << ".";

    mErrors->handleError(glErrorCode, errorStream.str().c_str(), file, function, line);
}

}

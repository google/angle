//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextGL:
//   OpenGL-specific functionality associated with a GL Context.
//

#include "libANGLE/renderer/gl/ContextGL.h"

#include "libANGLE/Context.h"
#include "libANGLE/renderer/gl/BufferGL.h"
#include "libANGLE/renderer/gl/CompilerGL.h"
#include "libANGLE/renderer/gl/FenceNVGL.h"
#include "libANGLE/renderer/gl/FramebufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/PathGL.h"
#include "libANGLE/renderer/gl/ProgramGL.h"
#include "libANGLE/renderer/gl/ProgramPipelineGL.h"
#include "libANGLE/renderer/gl/QueryGL.h"
#include "libANGLE/renderer/gl/RenderbufferGL.h"
#include "libANGLE/renderer/gl/RendererGL.h"
#include "libANGLE/renderer/gl/SamplerGL.h"
#include "libANGLE/renderer/gl/ShaderGL.h"
#include "libANGLE/renderer/gl/StateManagerGL.h"
#include "libANGLE/renderer/gl/SyncGL.h"
#include "libANGLE/renderer/gl/TextureGL.h"
#include "libANGLE/renderer/gl/TransformFeedbackGL.h"
#include "libANGLE/renderer/gl/VertexArrayGL.h"

namespace rx
{

ContextGL::ContextGL(const gl::ContextState &state, const std::shared_ptr<RendererGL> &renderer)
    : ContextImpl(state), mRenderer(renderer)
{
}

ContextGL::~ContextGL()
{
}

angle::Result ContextGL::initialize()
{
    return angle::Result::Continue();
}

CompilerImpl *ContextGL::createCompiler()
{
    return new CompilerGL(getFunctions());
}

ShaderImpl *ContextGL::createShader(const gl::ShaderState &data)
{
    const FunctionsGL *functions = getFunctions();
    GLuint shader                = functions->createShader(ToGLenum(data.getShaderType()));

    return new ShaderGL(data, shader, mRenderer->getMultiviewImplementationType(), functions);
}

ProgramImpl *ContextGL::createProgram(const gl::ProgramState &data)
{
    return new ProgramGL(data, getFunctions(), getWorkaroundsGL(), getStateManager(),
                         getExtensions().pathRendering);
}

FramebufferImpl *ContextGL::createFramebuffer(const gl::FramebufferState &data)
{
    const FunctionsGL *funcs = getFunctions();

    GLuint fbo = 0;
    funcs->genFramebuffers(1, &fbo);

    return new FramebufferGL(data, fbo, false);
}

TextureImpl *ContextGL::createTexture(const gl::TextureState &state)
{
    const FunctionsGL *functions = getFunctions();
    StateManagerGL *stateManager = getStateManager();

    GLuint texture = 0;
    functions->genTextures(1, &texture);
    stateManager->bindTexture(state.getType(), texture);

    return new TextureGL(state, texture);
}

RenderbufferImpl *ContextGL::createRenderbuffer(const gl::RenderbufferState &state)
{
    return new RenderbufferGL(state, getFunctions(), getWorkaroundsGL(), getStateManager(),
                              mRenderer->getBlitter(), getNativeTextureCaps());
}

BufferImpl *ContextGL::createBuffer(const gl::BufferState &state)
{
    return new BufferGL(state, getFunctions(), getStateManager());
}

VertexArrayImpl *ContextGL::createVertexArray(const gl::VertexArrayState &data)
{
    return new VertexArrayGL(data, getFunctions(), getStateManager());
}

QueryImpl *ContextGL::createQuery(gl::QueryType type)
{
    switch (type)
    {
        case gl::QueryType::CommandsCompleted:
            return new SyncQueryGL(type, getFunctions());

        default:
            return new StandardQueryGL(type, getFunctions(), getStateManager());
    }
}

FenceNVImpl *ContextGL::createFenceNV()
{
    return new FenceNVGL(getFunctions());
}

SyncImpl *ContextGL::createSync()
{
    return new SyncGL(getFunctions());
}

TransformFeedbackImpl *ContextGL::createTransformFeedback(const gl::TransformFeedbackState &state)
{
    return new TransformFeedbackGL(state, getFunctions(), getStateManager());
}

SamplerImpl *ContextGL::createSampler(const gl::SamplerState &state)
{
    return new SamplerGL(state, getFunctions(), getStateManager());
}

ProgramPipelineImpl *ContextGL::createProgramPipeline(const gl::ProgramPipelineState &data)
{
    return new ProgramPipelineGL(data, getFunctions());
}

std::vector<PathImpl *> ContextGL::createPaths(GLsizei range)
{
    const FunctionsGL *funcs = getFunctions();

    std::vector<PathImpl *> ret;
    ret.reserve(range);

    const GLuint first = funcs->genPathsNV(range);
    if (first == 0)
        return ret;

    for (GLsizei i = 0; i < range; ++i)
    {
        const auto id = first + i;
        ret.push_back(new PathGL(funcs, id));
    }

    return ret;
}

angle::Result ContextGL::flush(const gl::Context *context)
{
    return mRenderer->flush();
}

angle::Result ContextGL::finish(const gl::Context *context)
{
    return mRenderer->finish();
}

angle::Result ContextGL::drawArrays(const gl::Context *context,
                                    gl::PrimitiveMode mode,
                                    GLint first,
                                    GLsizei count)
{
    return mRenderer->drawArrays(context, mode, first, count);
}

angle::Result ContextGL::drawArraysInstanced(const gl::Context *context,
                                             gl::PrimitiveMode mode,
                                             GLint first,
                                             GLsizei count,
                                             GLsizei instanceCount)
{
    return mRenderer->drawArraysInstanced(context, mode, first, count, instanceCount);
}

angle::Result ContextGL::drawElements(const gl::Context *context,
                                      gl::PrimitiveMode mode,
                                      GLsizei count,
                                      GLenum type,
                                      const void *indices)
{
    return mRenderer->drawElements(context, mode, count, type, indices);
}

angle::Result ContextGL::drawElementsInstanced(const gl::Context *context,
                                               gl::PrimitiveMode mode,
                                               GLsizei count,
                                               GLenum type,
                                               const void *indices,
                                               GLsizei instances)
{
    return mRenderer->drawElementsInstanced(context, mode, count, type, indices, instances);
}

angle::Result ContextGL::drawRangeElements(const gl::Context *context,
                                           gl::PrimitiveMode mode,
                                           GLuint start,
                                           GLuint end,
                                           GLsizei count,
                                           GLenum type,
                                           const void *indices)
{
    return mRenderer->drawRangeElements(context, mode, start, end, count, type, indices);
}

angle::Result ContextGL::drawArraysIndirect(const gl::Context *context,
                                            gl::PrimitiveMode mode,
                                            const void *indirect)
{
    return mRenderer->drawArraysIndirect(context, mode, indirect);
}

angle::Result ContextGL::drawElementsIndirect(const gl::Context *context,
                                              gl::PrimitiveMode mode,
                                              GLenum type,
                                              const void *indirect)
{
    return mRenderer->drawElementsIndirect(context, mode, type, indirect);
}

void ContextGL::stencilFillPath(const gl::Path *path, GLenum fillMode, GLuint mask)
{
    mRenderer->stencilFillPath(mState, path, fillMode, mask);
}

void ContextGL::stencilStrokePath(const gl::Path *path, GLint reference, GLuint mask)
{
    mRenderer->stencilStrokePath(mState, path, reference, mask);
}

void ContextGL::coverFillPath(const gl::Path *path, GLenum coverMode)
{
    mRenderer->coverFillPath(mState, path, coverMode);
}

void ContextGL::coverStrokePath(const gl::Path *path, GLenum coverMode)
{
    mRenderer->coverStrokePath(mState, path, coverMode);
}

void ContextGL::stencilThenCoverFillPath(const gl::Path *path,
                                         GLenum fillMode,
                                         GLuint mask,
                                         GLenum coverMode)
{
    mRenderer->stencilThenCoverFillPath(mState, path, fillMode, mask, coverMode);
}

void ContextGL::stencilThenCoverStrokePath(const gl::Path *path,
                                           GLint reference,
                                           GLuint mask,
                                           GLenum coverMode)
{
    mRenderer->stencilThenCoverStrokePath(mState, path, reference, mask, coverMode);
}

void ContextGL::coverFillPathInstanced(const std::vector<gl::Path *> &paths,
                                       GLenum coverMode,
                                       GLenum transformType,
                                       const GLfloat *transformValues)
{
    mRenderer->coverFillPathInstanced(mState, paths, coverMode, transformType, transformValues);
}

void ContextGL::coverStrokePathInstanced(const std::vector<gl::Path *> &paths,
                                         GLenum coverMode,
                                         GLenum transformType,
                                         const GLfloat *transformValues)
{
    mRenderer->coverStrokePathInstanced(mState, paths, coverMode, transformType, transformValues);
}

void ContextGL::stencilFillPathInstanced(const std::vector<gl::Path *> &paths,
                                         GLenum fillMode,
                                         GLuint mask,
                                         GLenum transformType,
                                         const GLfloat *transformValues)
{
    mRenderer->stencilFillPathInstanced(mState, paths, fillMode, mask, transformType,
                                        transformValues);
}

void ContextGL::stencilStrokePathInstanced(const std::vector<gl::Path *> &paths,
                                           GLint reference,
                                           GLuint mask,
                                           GLenum transformType,
                                           const GLfloat *transformValues)
{
    mRenderer->stencilStrokePathInstanced(mState, paths, reference, mask, transformType,
                                          transformValues);
}

void ContextGL::stencilThenCoverFillPathInstanced(const std::vector<gl::Path *> &paths,
                                                  GLenum coverMode,
                                                  GLenum fillMode,
                                                  GLuint mask,
                                                  GLenum transformType,
                                                  const GLfloat *transformValues)
{
    mRenderer->stencilThenCoverFillPathInstanced(mState, paths, coverMode, fillMode, mask,
                                                 transformType, transformValues);
}

void ContextGL::stencilThenCoverStrokePathInstanced(const std::vector<gl::Path *> &paths,
                                                    GLenum coverMode,
                                                    GLint reference,
                                                    GLuint mask,
                                                    GLenum transformType,
                                                    const GLfloat *transformValues)
{
    mRenderer->stencilThenCoverStrokePathInstanced(mState, paths, coverMode, reference, mask,
                                                   transformType, transformValues);
}

GLenum ContextGL::getResetStatus()
{
    return mRenderer->getResetStatus();
}

std::string ContextGL::getVendorString() const
{
    return mRenderer->getVendorString();
}

std::string ContextGL::getRendererDescription() const
{
    return mRenderer->getRendererDescription();
}

void ContextGL::insertEventMarker(GLsizei length, const char *marker)
{
    mRenderer->insertEventMarker(length, marker);
}

void ContextGL::pushGroupMarker(GLsizei length, const char *marker)
{
    mRenderer->pushGroupMarker(length, marker);
}

void ContextGL::popGroupMarker()
{
    mRenderer->popGroupMarker();
}

void ContextGL::pushDebugGroup(GLenum source, GLuint id, GLsizei length, const char *message)
{
    mRenderer->pushDebugGroup(source, id, length, message);
}

void ContextGL::popDebugGroup()
{
    mRenderer->popDebugGroup();
}

angle::Result ContextGL::syncState(const gl::Context *context,
                                   const gl::State::DirtyBits &dirtyBits,
                                   const gl::State::DirtyBits &bitMask)
{
    mRenderer->getStateManager()->syncState(context, dirtyBits, bitMask);
    return angle::Result::Continue();
}

GLint ContextGL::getGPUDisjoint()
{
    return mRenderer->getGPUDisjoint();
}

GLint64 ContextGL::getTimestamp()
{
    return mRenderer->getTimestamp();
}

angle::Result ContextGL::onMakeCurrent(const gl::Context *context)
{
    // Queries need to be paused/resumed on context switches
    return mRenderer->getStateManager()->onMakeCurrent(context);
}

gl::Caps ContextGL::getNativeCaps() const
{
    return mRenderer->getNativeCaps();
}

const gl::TextureCapsMap &ContextGL::getNativeTextureCaps() const
{
    return mRenderer->getNativeTextureCaps();
}

const gl::Extensions &ContextGL::getNativeExtensions() const
{
    return mRenderer->getNativeExtensions();
}

const gl::Limitations &ContextGL::getNativeLimitations() const
{
    return mRenderer->getNativeLimitations();
}

void ContextGL::applyNativeWorkarounds(gl::Workarounds *workarounds) const
{
    return mRenderer->applyNativeWorkarounds(workarounds);
}

const FunctionsGL *ContextGL::getFunctions() const
{
    return mRenderer->getFunctions();
}

StateManagerGL *ContextGL::getStateManager()
{
    return mRenderer->getStateManager();
}

const WorkaroundsGL &ContextGL::getWorkaroundsGL() const
{
    return mRenderer->getWorkarounds();
}

BlitGL *ContextGL::getBlitter() const
{
    return mRenderer->getBlitter();
}

ClearMultiviewGL *ContextGL::getMultiviewClearer() const
{
    return mRenderer->getMultiviewClearer();
}

angle::Result ContextGL::dispatchCompute(const gl::Context *context,
                                         GLuint numGroupsX,
                                         GLuint numGroupsY,
                                         GLuint numGroupsZ)
{
    return mRenderer->dispatchCompute(context, numGroupsX, numGroupsY, numGroupsZ);
}

angle::Result ContextGL::dispatchComputeIndirect(const gl::Context *context, GLintptr indirect)
{
    return mRenderer->dispatchComputeIndirect(context, indirect);
}

angle::Result ContextGL::memoryBarrier(const gl::Context *context, GLbitfield barriers)
{
    return mRenderer->memoryBarrier(barriers);
}
angle::Result ContextGL::memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers)
{
    return mRenderer->memoryBarrierByRegion(barriers);
}

void ContextGL::handleError(GLenum errorCode,
                            const char *message,
                            const char *file,
                            const char *function,
                            unsigned int line)
{
    std::stringstream errorStream;
    errorStream << "Internal OpenGL error: " << gl::FmtHex(errorCode) << ", in " << file << ", "
                << function << ":" << line << ". " << message;

    mErrors->handleError(gl::Error(errorCode, errorCode, errorStream.str()));
}
}  // namespace rx

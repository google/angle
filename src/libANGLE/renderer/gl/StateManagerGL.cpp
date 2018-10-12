//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManagerGL.h: Defines a class for caching applied OpenGL state

#include "libANGLE/renderer/gl/StateManagerGL.h"

#include <string.h>
#include <algorithm>
#include <limits>

#include "anglebase/numerics/safe_conversions.h"
#include "common/bitset_utils.h"
#include "common/mathutil.h"
#include "common/matrix_utils.h"
#include "libANGLE/Context.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/Query.h"
#include "libANGLE/TransformFeedback.h"
#include "libANGLE/VertexArray.h"
#include "libANGLE/renderer/gl/BufferGL.h"
#include "libANGLE/renderer/gl/FramebufferGL.h"
#include "libANGLE/renderer/gl/FunctionsGL.h"
#include "libANGLE/renderer/gl/ProgramGL.h"
#include "libANGLE/renderer/gl/QueryGL.h"
#include "libANGLE/renderer/gl/SamplerGL.h"
#include "libANGLE/renderer/gl/TextureGL.h"
#include "libANGLE/renderer/gl/TransformFeedbackGL.h"
#include "libANGLE/renderer/gl/VertexArrayGL.h"

namespace rx
{

StateManagerGL::IndexedBufferBinding::IndexedBufferBinding() : offset(0), size(0), buffer(0)
{
}

namespace
{
bool AllRectanglesMatch(const gl::Rectangle &matchingRectangle,
                        const std::vector<gl::Rectangle> &rectangles)
{
    for (const auto &rect : rectangles)
    {
        if (matchingRectangle != rect)
        {
            return false;
        }
    }
    return true;
}

std::vector<gl::Rectangle> ApplyOffsets(const gl::Rectangle &modifiableRectangle,
                                        const std::vector<gl::Offset> &offsets)
{
    std::vector<gl::Rectangle> result;
    result.reserve(offsets.size());
    for (size_t i = 0u; i < offsets.size(); ++i)
    {
        result.emplace_back(gl::Rectangle(modifiableRectangle.x + offsets[i].x,
                                          modifiableRectangle.y + offsets[i].y,
                                          modifiableRectangle.width, modifiableRectangle.height));
    }
    return result;
}
}  // namespace

StateManagerGL::StateManagerGL(const FunctionsGL *functions,
                               const gl::Caps &rendererCaps,
                               const gl::Extensions &extensions)
    : mFunctions(functions),
      mProgram(0),
      mVAO(0),
      mVertexAttribCurrentValues(rendererCaps.maxVertexAttributes),
      mBuffers(),
      mIndexedBuffers(),
      mTextureUnitIndex(0),
      mTextures{},
      mSamplers{},
      mImages(rendererCaps.maxImageUnits, ImageUnitBinding()),
      mTransformFeedback(0),
      mCurrentTransformFeedback(nullptr),
      mQueries(),
      mPrevDrawContext(0),
      mUnpackAlignment(4),
      mUnpackRowLength(0),
      mUnpackSkipRows(0),
      mUnpackSkipPixels(0),
      mUnpackImageHeight(0),
      mUnpackSkipImages(0),
      mPackAlignment(4),
      mPackRowLength(0),
      mPackSkipRows(0),
      mPackSkipPixels(0),
      mFramebuffers(angle::FramebufferBindingSingletonMax, 0),
      mRenderbuffer(0),
      mScissorTestEnabled(false),
      mScissors(extensions.maxViews),
      mViewports(extensions.maxViews),
      mViewportOffsets(extensions.maxViews),
      mNear(0.0f),
      mFar(1.0f),
      mBlendEnabled(false),
      mBlendColor(0, 0, 0, 0),
      mSourceBlendRGB(GL_ONE),
      mDestBlendRGB(GL_ZERO),
      mSourceBlendAlpha(GL_ONE),
      mDestBlendAlpha(GL_ZERO),
      mBlendEquationRGB(GL_FUNC_ADD),
      mBlendEquationAlpha(GL_FUNC_ADD),
      mColorMaskRed(true),
      mColorMaskGreen(true),
      mColorMaskBlue(true),
      mColorMaskAlpha(true),
      mSampleAlphaToCoverageEnabled(false),
      mSampleCoverageEnabled(false),
      mSampleCoverageValue(1.0f),
      mSampleCoverageInvert(false),
      mSampleMaskEnabled(false),
      mDepthTestEnabled(false),
      mDepthFunc(GL_LESS),
      mDepthMask(true),
      mStencilTestEnabled(false),
      mStencilFrontFunc(GL_ALWAYS),
      mStencilFrontRef(0),
      mStencilFrontValueMask(static_cast<GLuint>(-1)),
      mStencilFrontStencilFailOp(GL_KEEP),
      mStencilFrontStencilPassDepthFailOp(GL_KEEP),
      mStencilFrontStencilPassDepthPassOp(GL_KEEP),
      mStencilFrontWritemask(static_cast<GLuint>(-1)),
      mStencilBackFunc(GL_ALWAYS),
      mStencilBackRef(0),
      mStencilBackValueMask(static_cast<GLuint>(-1)),
      mStencilBackStencilFailOp(GL_KEEP),
      mStencilBackStencilPassDepthFailOp(GL_KEEP),
      mStencilBackStencilPassDepthPassOp(GL_KEEP),
      mStencilBackWritemask(static_cast<GLuint>(-1)),
      mCullFaceEnabled(false),
      mCullFace(gl::CullFaceMode::Back),
      mFrontFace(GL_CCW),
      mPolygonOffsetFillEnabled(false),
      mPolygonOffsetFactor(0.0f),
      mPolygonOffsetUnits(0.0f),
      mRasterizerDiscardEnabled(false),
      mLineWidth(1.0f),
      mPrimitiveRestartEnabled(false),
      mClearColor(0.0f, 0.0f, 0.0f, 0.0f),
      mClearDepth(1.0f),
      mClearStencil(0),
      mFramebufferSRGBEnabled(false),
      mDitherEnabled(true),
      mTextureCubemapSeamlessEnabled(false),
      mMultisamplingEnabled(true),
      mSampleAlphaToOneEnabled(false),
      mCoverageModulation(GL_NONE),
      mPathStencilFunc(GL_ALWAYS),
      mPathStencilRef(0),
      mPathStencilMask(std::numeric_limits<GLuint>::max()),
      mIsSideBySideDrawFramebuffer(false),
      mIsMultiviewEnabled(extensions.multiview),
      mLocalDirtyBits(),
      mMultiviewDirtyBits()
{
    ASSERT(mFunctions);
    ASSERT(extensions.maxViews >= 1u);

    mIndexedBuffers[gl::BufferBinding::Uniform].resize(rendererCaps.maxUniformBufferBindings);
    mIndexedBuffers[gl::BufferBinding::AtomicCounter].resize(
        rendererCaps.maxAtomicCounterBufferBindings);
    mIndexedBuffers[gl::BufferBinding::ShaderStorage].resize(
        rendererCaps.maxShaderStorageBufferBindings);

    mSampleMaskValues.fill(~GLbitfield(0));

    mQueries.fill(nullptr);
    mTemporaryPausedQueries.fill(nullptr);

    // Initialize point sprite state for desktop GL
    if (mFunctions->standard == STANDARD_GL_DESKTOP)
    {
        mFunctions->enable(GL_PROGRAM_POINT_SIZE);

        // GL_POINT_SPRITE was deprecated in the core profile. Point rasterization is always
        // performed
        // as though POINT_SPRITE were enabled.
        if ((mFunctions->profile & GL_CONTEXT_CORE_PROFILE_BIT) == 0)
        {
            mFunctions->enable(GL_POINT_SPRITE);
        }
    }

    angle::Matrix<GLfloat>::setToIdentity(mPathMatrixProj);
    angle::Matrix<GLfloat>::setToIdentity(mPathMatrixMV);
}

StateManagerGL::~StateManagerGL()
{
}

void StateManagerGL::deleteProgram(GLuint program)
{
    if (program != 0)
    {
        if (mProgram == program)
        {
            useProgram(0);
        }

        mFunctions->deleteProgram(program);
    }
}

void StateManagerGL::deleteVertexArray(GLuint vao)
{
    if (vao != 0)
    {
        if (mVAO == vao)
        {
            bindVertexArray(0, 0);
        }

        mFunctions->deleteVertexArrays(1, &vao);
    }
}

void StateManagerGL::deleteTexture(GLuint texture)
{
    if (texture != 0)
    {
        for (gl::TextureType type : angle::AllEnums<gl::TextureType>())
        {
            const auto &textureVector = mTextures[type];
            for (size_t textureUnitIndex = 0; textureUnitIndex < textureVector.size();
                 textureUnitIndex++)
            {
                if (textureVector[textureUnitIndex] == texture)
                {
                    activeTexture(textureUnitIndex);
                    bindTexture(type, 0);
                }
            }
        }

        for (size_t imageUnitIndex = 0; imageUnitIndex < mImages.size(); imageUnitIndex++)
        {
            if (mImages[imageUnitIndex].texture == texture)
            {
                bindImageTexture(imageUnitIndex, 0, 0, false, 0, GL_READ_ONLY, GL_R32UI);
            }
        }

        mFunctions->deleteTextures(1, &texture);
    }
}

void StateManagerGL::deleteSampler(GLuint sampler)
{
    if (sampler != 0)
    {
        for (size_t unit = 0; unit < mSamplers.size(); unit++)
        {
            if (mSamplers[unit] == sampler)
            {
                bindSampler(unit, 0);
            }
        }

        mFunctions->deleteSamplers(1, &sampler);
    }
}

void StateManagerGL::deleteBuffer(GLuint buffer)
{
    if (buffer == 0)
    {
        return;
    }

    for (auto target : angle::AllEnums<gl::BufferBinding>())
    {
        if (mBuffers[target] == buffer)
        {
            bindBuffer(target, 0);
        }

        auto &indexedTarget = mIndexedBuffers[target];
        for (size_t bindIndex = 0; bindIndex < indexedTarget.size(); ++bindIndex)
        {
            if (indexedTarget[bindIndex].buffer == buffer)
            {
                bindBufferBase(target, bindIndex, 0);
            }
        }
    }

    mFunctions->deleteBuffers(1, &buffer);
}

void StateManagerGL::deleteFramebuffer(GLuint fbo)
{
    if (fbo != 0)
    {
        for (size_t binding = 0; binding < mFramebuffers.size(); ++binding)
        {
            if (mFramebuffers[binding] == fbo)
            {
                GLenum enumValue = angle::FramebufferBindingToEnum(
                    static_cast<angle::FramebufferBinding>(binding));
                bindFramebuffer(enumValue, 0);
            }
        }
        mFunctions->deleteFramebuffers(1, &fbo);
    }
}

void StateManagerGL::deleteRenderbuffer(GLuint rbo)
{
    if (rbo != 0)
    {
        if (mRenderbuffer == rbo)
        {
            bindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        mFunctions->deleteRenderbuffers(1, &rbo);
    }
}

void StateManagerGL::deleteTransformFeedback(GLuint transformFeedback)
{
    if (transformFeedback != 0)
    {
        if (mTransformFeedback == transformFeedback)
        {
            bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        }

        if (mCurrentTransformFeedback != nullptr &&
            mCurrentTransformFeedback->getTransformFeedbackID() == transformFeedback)
        {
            mCurrentTransformFeedback = nullptr;
        }

        mFunctions->deleteTransformFeedbacks(1, &transformFeedback);
    }
}

void StateManagerGL::useProgram(GLuint program)
{
    if (mProgram != program)
    {
        forceUseProgram(program);
    }
}

void StateManagerGL::forceUseProgram(GLuint program)
{
    mProgram = program;
    mFunctions->useProgram(mProgram);
    mLocalDirtyBits.set(gl::State::DIRTY_BIT_PROGRAM_BINDING);
}

void StateManagerGL::bindVertexArray(GLuint vao, GLuint elementArrayBuffer)
{
    if (mVAO != vao)
    {
        mVAO                                      = vao;
        mBuffers[gl::BufferBinding::ElementArray] = elementArrayBuffer;
        mFunctions->bindVertexArray(vao);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_VERTEX_ARRAY_BINDING);
    }
}

void StateManagerGL::bindBuffer(gl::BufferBinding target, GLuint buffer)
{
    // GL drivers differ in whether the transform feedback bind point is modified when
    // glBindTransformFeedback is called. To avoid these behavior differences we shouldn't try to
    // use it.
    ASSERT(target != gl::BufferBinding::TransformFeedback);
    if (mBuffers[target] != buffer)
    {
        mBuffers[target] = buffer;
        mFunctions->bindBuffer(gl::ToGLenum(target), buffer);
    }
}

void StateManagerGL::bindBufferBase(gl::BufferBinding target, size_t index, GLuint buffer)
{
    ASSERT(index < mIndexedBuffers[target].size());
    auto &binding = mIndexedBuffers[target][index];
    if (binding.buffer != buffer || binding.offset != static_cast<size_t>(-1) ||
        binding.size != static_cast<size_t>(-1))
    {
        binding.buffer = buffer;
        binding.offset = static_cast<size_t>(-1);
        binding.size   = static_cast<size_t>(-1);
        mFunctions->bindBufferBase(gl::ToGLenum(target), static_cast<GLuint>(index), buffer);
    }
}

void StateManagerGL::bindBufferRange(gl::BufferBinding target,
                                     size_t index,
                                     GLuint buffer,
                                     size_t offset,
                                     size_t size)
{
    auto &binding = mIndexedBuffers[target][index];
    if (binding.buffer != buffer || binding.offset != offset || binding.size != size)
    {
        binding.buffer = buffer;
        binding.offset = offset;
        binding.size   = size;
        mFunctions->bindBufferRange(gl::ToGLenum(target), static_cast<GLuint>(index), buffer,
                                    offset, size);
    }
}

void StateManagerGL::activeTexture(size_t unit)
{
    if (mTextureUnitIndex != unit)
    {
        mTextureUnitIndex = unit;
        mFunctions->activeTexture(GL_TEXTURE0 + static_cast<GLenum>(mTextureUnitIndex));
    }
}

void StateManagerGL::bindTexture(gl::TextureType type, GLuint texture)
{
    if (mTextures[type][mTextureUnitIndex] != texture)
    {
        mTextures[type][mTextureUnitIndex] = texture;
        mFunctions->bindTexture(ToGLenum(type), texture);
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_TEXTURE_BINDINGS);
    }
}

void StateManagerGL::bindSampler(size_t unit, GLuint sampler)
{
    if (mSamplers[unit] != sampler)
    {
        mSamplers[unit] = sampler;
        mFunctions->bindSampler(static_cast<GLuint>(unit), sampler);
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLER_BINDINGS);
    }
}

void StateManagerGL::bindImageTexture(size_t unit,
                                      GLuint texture,
                                      GLint level,
                                      GLboolean layered,
                                      GLint layer,
                                      GLenum access,
                                      GLenum format)
{
    auto &binding = mImages[unit];
    if (binding.texture != texture || binding.level != level || binding.layered != layered ||
        binding.layer != layer || binding.access != access || binding.format != format)
    {
        binding.texture = texture;
        binding.level   = level;
        binding.layered = layered;
        binding.layer   = layer;
        binding.access  = access;
        binding.format  = format;
        mFunctions->bindImageTexture(angle::base::checked_cast<GLuint>(unit), texture, level,
                                     layered, layer, access, format);
    }
}

void StateManagerGL::setPixelUnpackState(const gl::PixelUnpackState &unpack)
{
    if (mUnpackAlignment != unpack.alignment)
    {
        mUnpackAlignment = unpack.alignment;
        mFunctions->pixelStorei(GL_UNPACK_ALIGNMENT, mUnpackAlignment);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }

    if (mUnpackRowLength != unpack.rowLength)
    {
        mUnpackRowLength = unpack.rowLength;
        mFunctions->pixelStorei(GL_UNPACK_ROW_LENGTH, mUnpackRowLength);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }

    if (mUnpackSkipRows != unpack.skipRows)
    {
        mUnpackSkipRows = unpack.skipRows;
        mFunctions->pixelStorei(GL_UNPACK_SKIP_ROWS, mUnpackSkipRows);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }

    if (mUnpackSkipPixels != unpack.skipPixels)
    {
        mUnpackSkipPixels = unpack.skipPixels;
        mFunctions->pixelStorei(GL_UNPACK_SKIP_PIXELS, mUnpackSkipPixels);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }

    if (mUnpackImageHeight != unpack.imageHeight)
    {
        mUnpackImageHeight = unpack.imageHeight;
        mFunctions->pixelStorei(GL_UNPACK_IMAGE_HEIGHT, mUnpackImageHeight);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }

    if (mUnpackSkipImages != unpack.skipImages)
    {
        mUnpackSkipImages = unpack.skipImages;
        mFunctions->pixelStorei(GL_UNPACK_SKIP_IMAGES, mUnpackSkipImages);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_UNPACK_STATE);
    }
}

void StateManagerGL::setPixelUnpackBuffer(const gl::Buffer *pixelBuffer)
{
    GLuint bufferID = 0;
    if (pixelBuffer != nullptr)
    {
        bufferID = GetImplAs<BufferGL>(pixelBuffer)->getBufferID();
    }
    bindBuffer(gl::BufferBinding::PixelUnpack, bufferID);
}

void StateManagerGL::setPixelPackState(const gl::PixelPackState &pack)
{
    if (mPackAlignment != pack.alignment)
    {
        mPackAlignment = pack.alignment;
        mFunctions->pixelStorei(GL_PACK_ALIGNMENT, mPackAlignment);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PACK_STATE);
    }

    if (mPackRowLength != pack.rowLength)
    {
        mPackRowLength = pack.rowLength;
        mFunctions->pixelStorei(GL_PACK_ROW_LENGTH, mPackRowLength);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PACK_STATE);
    }

    if (mPackSkipRows != pack.skipRows)
    {
        mPackSkipRows = pack.skipRows;
        mFunctions->pixelStorei(GL_PACK_SKIP_ROWS, mPackSkipRows);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PACK_STATE);
    }

    if (mPackSkipPixels != pack.skipPixels)
    {
        mPackSkipPixels = pack.skipPixels;
        mFunctions->pixelStorei(GL_PACK_SKIP_PIXELS, mPackSkipPixels);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PACK_STATE);
    }
}

void StateManagerGL::setPixelPackBuffer(const gl::Buffer *pixelBuffer)
{
    GLuint bufferID = 0;
    if (pixelBuffer != nullptr)
    {
        bufferID = GetImplAs<BufferGL>(pixelBuffer)->getBufferID();
    }
    bindBuffer(gl::BufferBinding::PixelPack, bufferID);
}

void StateManagerGL::bindFramebuffer(GLenum type, GLuint framebuffer)
{
    switch (type)
    {
        case GL_FRAMEBUFFER:
            if (mFramebuffers[angle::FramebufferBindingRead] != framebuffer ||
                mFramebuffers[angle::FramebufferBindingDraw] != framebuffer)
            {
                mFramebuffers[angle::FramebufferBindingRead] = framebuffer;
                mFramebuffers[angle::FramebufferBindingDraw] = framebuffer;
                mFunctions->bindFramebuffer(GL_FRAMEBUFFER, framebuffer);

                mLocalDirtyBits.set(gl::State::DIRTY_BIT_READ_FRAMEBUFFER_BINDING);
                mLocalDirtyBits.set(gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING);
            }
            break;

        case GL_READ_FRAMEBUFFER:
            if (mFramebuffers[angle::FramebufferBindingRead] != framebuffer)
            {
                mFramebuffers[angle::FramebufferBindingRead] = framebuffer;
                mFunctions->bindFramebuffer(GL_READ_FRAMEBUFFER, framebuffer);

                mLocalDirtyBits.set(gl::State::DIRTY_BIT_READ_FRAMEBUFFER_BINDING);
            }
            break;

        case GL_DRAW_FRAMEBUFFER:
            if (mFramebuffers[angle::FramebufferBindingDraw] != framebuffer)
            {
                mFramebuffers[angle::FramebufferBindingDraw] = framebuffer;
                mFunctions->bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebuffer);

                mLocalDirtyBits.set(gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING);
            }
            break;

        default:
            UNREACHABLE();
            break;
    }
}

void StateManagerGL::bindRenderbuffer(GLenum type, GLuint renderbuffer)
{
    ASSERT(type == GL_RENDERBUFFER);
    if (mRenderbuffer != renderbuffer)
    {
        mRenderbuffer = renderbuffer;
        mFunctions->bindRenderbuffer(type, mRenderbuffer);
    }
}

void StateManagerGL::bindTransformFeedback(GLenum type, GLuint transformFeedback)
{
    ASSERT(type == GL_TRANSFORM_FEEDBACK);
    if (mTransformFeedback != transformFeedback)
    {
        // Pause the current transform feedback if one is active.
        // To handle virtualized contexts, StateManagerGL needs to be able to bind a new transform
        // feedback at any time, even if there is one active.
        if (mCurrentTransformFeedback != nullptr &&
            mCurrentTransformFeedback->getTransformFeedbackID() != transformFeedback)
        {
            mCurrentTransformFeedback->syncPausedState(true);
            mCurrentTransformFeedback = nullptr;
        }

        mTransformFeedback = transformFeedback;
        mFunctions->bindTransformFeedback(type, mTransformFeedback);
        onTransformFeedbackStateChange();
    }
}

void StateManagerGL::onTransformFeedbackStateChange()
{
    mLocalDirtyBits.set(gl::State::DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING);
}

void StateManagerGL::beginQuery(gl::QueryType type, QueryGL *queryObject, GLuint queryId)
{
    // Make sure this is a valid query type and there is no current active query of this type
    ASSERT(mQueries[type] == nullptr);
    ASSERT(queryId != 0);

    mQueries[type] = queryObject;
    mFunctions->beginQuery(ToGLenum(type), queryId);
}

void StateManagerGL::endQuery(gl::QueryType type, QueryGL *queryObject, GLuint queryId)
{
    ASSERT(queryObject != nullptr);
    ASSERT(mQueries[type] == queryObject);
    mQueries[type] = nullptr;
    mFunctions->endQuery(ToGLenum(type));
}

angle::Result StateManagerGL::setDrawArraysState(const gl::Context *context,
                                                 GLint first,
                                                 GLsizei count,
                                                 GLsizei instanceCount)
{
    if (context->getStateCache().hasAnyActiveClientAttrib())
    {
        const gl::State &glState   = context->getGLState();
        const gl::Program *program = glState.getProgram();
        const gl::VertexArray *vao = glState.getVertexArray();
        const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(vao);

        ANGLE_TRY(vaoGL->syncClientSideData(context, program->getActiveAttribLocationsMask(), first,
                                            count, instanceCount));
    }

    return setGenericDrawState(context);
}

angle::Result StateManagerGL::setDrawElementsState(const gl::Context *context,
                                                   GLsizei count,
                                                   GLenum type,
                                                   const void *indices,
                                                   GLsizei instanceCount,
                                                   const void **outIndices)
{
    const gl::State &glState = context->getGLState();

    const gl::Program *program = glState.getProgram();

    const gl::VertexArray *vao = glState.getVertexArray();
    const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(vao);

    ANGLE_TRY(vaoGL->syncDrawElementsState(context, program->getActiveAttribLocationsMask(), count,
                                           type, indices, instanceCount,
                                           glState.isPrimitiveRestartEnabled(), outIndices));

    return setGenericDrawState(context);
}

angle::Result StateManagerGL::setDrawIndirectState(const gl::Context *context)
{
    return setGenericDrawState(context);
}

void StateManagerGL::updateDrawIndirectBufferBinding(const gl::Context *context)
{
    gl::Buffer *drawIndirectBuffer =
        context->getGLState().getTargetBuffer(gl::BufferBinding::DrawIndirect);
    if (drawIndirectBuffer != nullptr)
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(drawIndirectBuffer);
        bindBuffer(gl::BufferBinding::DrawIndirect, bufferGL->getBufferID());
    }
}

void StateManagerGL::updateDispatchIndirectBufferBinding(const gl::Context *context)
{
    gl::Buffer *dispatchIndirectBuffer =
        context->getGLState().getTargetBuffer(gl::BufferBinding::DispatchIndirect);
    if (dispatchIndirectBuffer != nullptr)
    {
        const BufferGL *bufferGL = GetImplAs<BufferGL>(dispatchIndirectBuffer);
        bindBuffer(gl::BufferBinding::DispatchIndirect, bufferGL->getBufferID());
    }
}

void StateManagerGL::pauseTransformFeedback()
{
    if (mCurrentTransformFeedback != nullptr)
    {
        mCurrentTransformFeedback->syncPausedState(true);
        onTransformFeedbackStateChange();
    }
}

angle::Result StateManagerGL::pauseAllQueries(const gl::Context *context)
{
    for (gl::QueryType type : angle::AllEnums<gl::QueryType>())
    {
        QueryGL *previousQuery = mQueries[type];

        if (previousQuery != nullptr)
        {
            ANGLE_TRY(previousQuery->pause(context));
            mTemporaryPausedQueries[type] = previousQuery;
            mQueries[type]                = nullptr;
        }
    }

    return angle::Result::Continue();
}

angle::Result StateManagerGL::pauseQuery(const gl::Context *context, gl::QueryType type)
{
    QueryGL *previousQuery = mQueries[type];

    if (previousQuery)
    {
        ANGLE_TRY(previousQuery->pause(context));
        mTemporaryPausedQueries[type] = previousQuery;
        mQueries[type]                = nullptr;
    }

    return angle::Result::Continue();
}

angle::Result StateManagerGL::resumeAllQueries(const gl::Context *context)
{
    for (gl::QueryType type : angle::AllEnums<gl::QueryType>())
    {
        QueryGL *pausedQuery = mTemporaryPausedQueries[type];

        if (pausedQuery != nullptr)
        {
            ASSERT(mQueries[type] == nullptr);
            ANGLE_TRY(pausedQuery->resume(context));
            mTemporaryPausedQueries[type] = nullptr;
        }
    }

    return angle::Result::Continue();
}

angle::Result StateManagerGL::resumeQuery(const gl::Context *context, gl::QueryType type)
{
    QueryGL *pausedQuery = mTemporaryPausedQueries[type];

    if (pausedQuery != nullptr)
    {
        ANGLE_TRY(pausedQuery->resume(context));
        mTemporaryPausedQueries[type] = nullptr;
    }

    return angle::Result::Continue();
}

angle::Result StateManagerGL::onMakeCurrent(const gl::Context *context)
{
    const gl::State &glState = context->getGLState();

#if defined(ANGLE_ENABLE_ASSERTS)
    // Temporarily pausing queries during context switch is not supported
    for (auto &pausedQuery : mTemporaryPausedQueries)
    {
        ASSERT(pausedQuery == nullptr);
    }
#endif

    // If the context has changed, pause the previous context's queries
    auto contextID = context->getContextState().getContextID();
    if (contextID != mPrevDrawContext)
    {
        for (gl::QueryType type : angle::AllEnums<gl::QueryType>())
        {
            QueryGL *currentQuery = mQueries[type];
            // Pause any old query object
            if (currentQuery != nullptr)
            {
                ANGLE_TRY(currentQuery->pause(context));
                mQueries[type] = nullptr;
            }

            // Check if this new context needs to resume a query
            gl::Query *newQuery = glState.getActiveQuery(type);
            if (newQuery != nullptr)
            {
                QueryGL *queryGL = GetImplAs<QueryGL>(newQuery);
                ANGLE_TRY(queryGL->resume(context));
            }
        }
    }
    onTransformFeedbackStateChange();
    mPrevDrawContext = contextID;

    // Seamless cubemaps are required for ES3 and higher contexts. It should be the cheapest to set
    // this state here since MakeCurrent is expected to be called less frequently than draw calls.
    setTextureCubemapSeamlessEnabled(context->getClientMajorVersion() >= 3);

    return angle::Result::Continue();
}

void StateManagerGL::updateProgramTextureBindings(const gl::Context *context)
{
    const gl::State &glState   = context->getGLState();
    const gl::Program *program = glState.getProgram();

    // It is possible there is no active program during a path operation.
    if (!program)
        return;

    const gl::ActiveTexturePointerArray &textures  = glState.getActiveTexturesCache();
    const gl::ActiveTextureMask &activeTextures    = program->getActiveSamplersMask();
    const gl::ActiveTextureTypeArray &textureTypes = program->getActiveSamplerTypes();

    for (size_t textureUnitIndex : activeTextures)
    {
        gl::TextureType textureType = textureTypes[textureUnitIndex];
        gl::Texture *texture        = textures[textureUnitIndex];

        // A nullptr texture indicates incomplete.
        if (texture != nullptr)
        {
            const TextureGL *textureGL = GetImplAs<TextureGL>(texture);
            ASSERT(!texture->hasAnyDirtyBit());
            ASSERT(!textureGL->hasAnyDirtyBit());

            activeTexture(textureUnitIndex);
            bindTexture(textureType, textureGL->getTextureID());
        }
        else
        {
            activeTexture(textureUnitIndex);
            bindTexture(textureType, 0);
        }
    }
}

void StateManagerGL::updateProgramStorageBufferBindings(const gl::Context *context)
{
    const gl::State &glState   = context->getGLState();
    const gl::Program *program = glState.getProgram();

    // It is possible there is no active program during a path operation.
    if (!program)
        return;

    for (size_t blockIndex = 0; blockIndex < program->getActiveShaderStorageBlockCount();
         blockIndex++)
    {
        GLuint binding = program->getShaderStorageBlockBinding(static_cast<GLuint>(blockIndex));
        const auto &shaderStorageBuffer = glState.getIndexedShaderStorageBuffer(binding);

        if (shaderStorageBuffer.get() != nullptr)
        {
            BufferGL *bufferGL = GetImplAs<BufferGL>(shaderStorageBuffer.get());

            if (shaderStorageBuffer.getSize() == 0)
            {
                bindBufferBase(gl::BufferBinding::ShaderStorage, binding, bufferGL->getBufferID());
            }
            else
            {
                bindBufferRange(gl::BufferBinding::ShaderStorage, binding, bufferGL->getBufferID(),
                                shaderStorageBuffer.getOffset(), shaderStorageBuffer.getSize());
            }
        }
    }
}

void StateManagerGL::updateProgramUniformBufferBindings(const gl::Context *context)
{
    // Sync the current program state
    const gl::State &glState   = context->getGLState();
    const gl::Program *program = glState.getProgram();

    // It is possible there is no active program during a path operation.
    if (!program)
        return;

    for (size_t uniformBlockIndex = 0; uniformBlockIndex < program->getActiveUniformBlockCount();
         uniformBlockIndex++)
    {
        GLuint binding = program->getUniformBlockBinding(static_cast<GLuint>(uniformBlockIndex));
        const auto &uniformBuffer = glState.getIndexedUniformBuffer(binding);

        if (uniformBuffer.get() != nullptr)
        {
            BufferGL *bufferGL = GetImplAs<BufferGL>(uniformBuffer.get());

            if (uniformBuffer.getSize() == 0)
            {
                bindBufferBase(gl::BufferBinding::Uniform, binding, bufferGL->getBufferID());
            }
            else
            {
                bindBufferRange(gl::BufferBinding::Uniform, binding, bufferGL->getBufferID(),
                                uniformBuffer.getOffset(), uniformBuffer.getSize());
            }
        }
    }
}

void StateManagerGL::updateProgramAtomicCounterBufferBindings(const gl::Context *context)
{
    const gl::State &glState   = context->getGLState();
    const gl::Program *program = glState.getProgram();

    // It is possible there is no active program during a path operation.
    if (!program)
        return;

    for (const auto &atomicCounterBuffer : program->getState().getAtomicCounterBuffers())
    {
        GLuint binding     = atomicCounterBuffer.binding;
        const auto &buffer = glState.getIndexedAtomicCounterBuffer(binding);

        if (buffer.get() != nullptr)
        {
            BufferGL *bufferGL = GetImplAs<BufferGL>(buffer.get());

            if (buffer.getSize() == 0)
            {
                bindBufferBase(gl::BufferBinding::AtomicCounter, binding, bufferGL->getBufferID());
            }
            else
            {
                bindBufferRange(gl::BufferBinding::AtomicCounter, binding, bufferGL->getBufferID(),
                                buffer.getOffset(), buffer.getSize());
            }
        }
    }
}

void StateManagerGL::updateProgramImageBindings(const gl::Context *context)
{
    const gl::State &glState   = context->getGLState();
    const gl::Program *program = glState.getProgram();

    // It is possible there is no active program during a path operation.
    if (!program)
        return;

    ASSERT(context->getClientVersion() >= gl::ES_3_1 || program->getImageBindings().size() == 0);
    for (size_t imageUnitIndex : program->getActiveImagesMask())
    {
        const gl::ImageUnit &imageUnit = glState.getImageUnit(imageUnitIndex);
        const TextureGL *textureGL     = SafeGetImplAs<TextureGL>(imageUnit.texture.get());
        if (textureGL)
        {
            bindImageTexture(imageUnitIndex, textureGL->getTextureID(), imageUnit.level,
                             imageUnit.layered, imageUnit.layer, imageUnit.access,
                             imageUnit.format);
        }
        else
        {
            bindImageTexture(imageUnitIndex, 0, imageUnit.level, imageUnit.layered, imageUnit.layer,
                             imageUnit.access, imageUnit.format);
        }
    }
}

angle::Result StateManagerGL::setGenericDrawState(const gl::Context *context)
{
    if (context->getExtensions().webglCompatibility)
    {
        const gl::State &glState     = context->getGLState();
        FramebufferGL *framebufferGL = GetImplAs<FramebufferGL>(glState.getDrawFramebuffer());
        auto activeOutputs = glState.getProgram()->getState().getActiveOutputVariables();
        framebufferGL->maskOutInactiveOutputDrawBuffers(context, GL_DRAW_FRAMEBUFFER,
                                                        activeOutputs);
    }

    ASSERT(
        mFramebuffers[angle::FramebufferBindingDraw] ==
        GetImplAs<FramebufferGL>(context->getGLState().getDrawFramebuffer())->getFramebufferID());
    ASSERT(mVAO ==
           GetImplAs<VertexArrayGL>(context->getGLState().getVertexArray())->getVertexArrayID());

    return angle::Result::Continue();
}

void StateManagerGL::setAttributeCurrentData(size_t index,
                                             const gl::VertexAttribCurrentValueData &data)
{
    if (mVertexAttribCurrentValues[index] != data)
    {
        mVertexAttribCurrentValues[index] = data;
        switch (mVertexAttribCurrentValues[index].Type)
        {
            case GL_FLOAT:
                mFunctions->vertexAttrib4fv(static_cast<GLuint>(index),
                                            mVertexAttribCurrentValues[index].FloatValues);
                break;
            case GL_INT:
                mFunctions->vertexAttribI4iv(static_cast<GLuint>(index),
                                             mVertexAttribCurrentValues[index].IntValues);
                break;
            case GL_UNSIGNED_INT:
                mFunctions->vertexAttribI4uiv(static_cast<GLuint>(index),
                                              mVertexAttribCurrentValues[index].UnsignedIntValues);
                break;
            default:
                UNREACHABLE();
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CURRENT_VALUES);
        mLocalDirtyCurrentValues.set(index);
    }
}

void StateManagerGL::setScissorTestEnabled(bool enabled)
{
    if (mScissorTestEnabled != enabled)
    {
        mScissorTestEnabled = enabled;
        if (mScissorTestEnabled)
        {
            mFunctions->enable(GL_SCISSOR_TEST);
        }
        else
        {
            mFunctions->disable(GL_SCISSOR_TEST);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR_TEST_ENABLED);
    }
}

void StateManagerGL::setScissor(const gl::Rectangle &scissor)
{
    if (!AllRectanglesMatch(scissor, mScissors))
    {
        mScissors.assign(mScissors.size(), scissor);
        mFunctions->scissor(scissor.x, scissor.y, scissor.width, scissor.height);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR);
    }
}

void StateManagerGL::setScissorArrayv(GLuint first, const std::vector<gl::Rectangle> &scissors)
{
    ASSERT(mFunctions->scissorArrayv != nullptr);
    size_t offset = static_cast<size_t>(first);
    ASSERT(offset + scissors.size() <= mScissors.size());
    if (!std::equal(scissors.cbegin(), scissors.cend(), mScissors.cbegin() + offset))
    {
        std::copy(scissors.begin(), scissors.end(), mScissors.begin() + offset);
        mFunctions->scissorArrayv(first, static_cast<GLsizei>(scissors.size()), &scissors[0].x);
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR);
    }
}

void StateManagerGL::setScissorIndexed(GLuint index, const gl::Rectangle &scissor)
{
    ASSERT(mFunctions->scissorIndexed != nullptr);
    ASSERT(static_cast<size_t>(index) < mScissors.size());
    if (mScissors[index] != scissor)
    {
        mScissors[index] = scissor;
        mFunctions->scissorIndexed(index, scissor.x, scissor.y, scissor.width, scissor.height);
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR);
    }
}

void StateManagerGL::setViewport(const gl::Rectangle &viewport)
{
    if (!AllRectanglesMatch(viewport, mViewports))
    {
        mViewports.assign(mViewports.size(), viewport);
        mFunctions->viewport(viewport.x, viewport.y, viewport.width, viewport.height);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_VIEWPORT);
    }
}

void StateManagerGL::setViewportArrayv(GLuint first, const std::vector<gl::Rectangle> &viewports)
{
    ASSERT(mFunctions->viewportArrayv != nullptr);
    size_t offset = static_cast<size_t>(first);
    ASSERT(offset + viewports.size() <= mViewports.size());
    if (!std::equal(viewports.cbegin(), viewports.cend(), mViewports.cbegin() + offset))
    {
        std::copy(viewports.begin(), viewports.end(), mViewports.begin() + offset);
        std::vector<float> viewportsAsFloats(4u * viewports.size());
        for (size_t i = 0u; i < viewports.size(); ++i)
        {
            viewportsAsFloats[i * 4u]      = static_cast<float>(viewports[i].x);
            viewportsAsFloats[i * 4u + 1u] = static_cast<float>(viewports[i].y);
            viewportsAsFloats[i * 4u + 2u] = static_cast<float>(viewports[i].width);
            viewportsAsFloats[i * 4u + 3u] = static_cast<float>(viewports[i].height);
        }
        mFunctions->viewportArrayv(first, static_cast<GLsizei>(viewports.size()),
                                   viewportsAsFloats.data());
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_VIEWPORT);
    }
}

void StateManagerGL::setViewportOffsets(const std::vector<gl::Offset> &viewportOffsets)
{
    if (!std::equal(viewportOffsets.cbegin(), viewportOffsets.cend(), mViewportOffsets.cbegin()))
    {
        std::copy(viewportOffsets.begin(), viewportOffsets.end(), mViewportOffsets.begin());

        const std::vector<gl::Rectangle> &viewportArray =
            ApplyOffsets(mViewports[0], viewportOffsets);
        setViewportArrayv(0u, viewportArray);

        const std::vector<gl::Rectangle> &scissorArray =
            ApplyOffsets(mScissors[0], viewportOffsets);
        setScissorArrayv(0u, scissorArray);

        mMultiviewDirtyBits.set(MULTIVIEW_DIRTY_BIT_VIEWPORT_OFFSETS);
    }
}

void StateManagerGL::setSideBySide(bool isSideBySide)
{
    if (mIsSideBySideDrawFramebuffer != isSideBySide)
    {
        mIsSideBySideDrawFramebuffer = isSideBySide;
        mMultiviewDirtyBits.set(MULTIVIEW_DIRTY_BIT_SIDE_BY_SIDE_LAYOUT);
    }
}

void StateManagerGL::setDepthRange(float near, float far)
{
    if (mNear != near || mFar != far)
    {
        mNear = near;
        mFar  = far;

        // The glDepthRangef function isn't available until OpenGL 4.1.  Prefer it when it is
        // available because OpenGL ES only works in floats.
        if (mFunctions->depthRangef)
        {
            mFunctions->depthRangef(mNear, mFar);
        }
        else
        {
            ASSERT(mFunctions->depthRange);
            mFunctions->depthRange(mNear, mFar);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_DEPTH_RANGE);
    }
}

void StateManagerGL::setBlendEnabled(bool enabled)
{
    if (mBlendEnabled != enabled)
    {
        mBlendEnabled = enabled;
        if (mBlendEnabled)
        {
            mFunctions->enable(GL_BLEND);
        }
        else
        {
            mFunctions->disable(GL_BLEND);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_BLEND_ENABLED);
    }
}

void StateManagerGL::setBlendColor(const gl::ColorF &blendColor)
{
    if (mBlendColor != blendColor)
    {
        mBlendColor = blendColor;
        mFunctions->blendColor(mBlendColor.red, mBlendColor.green, mBlendColor.blue,
                               mBlendColor.alpha);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_BLEND_COLOR);
    }
}

void StateManagerGL::setBlendFuncs(GLenum sourceBlendRGB,
                                   GLenum destBlendRGB,
                                   GLenum sourceBlendAlpha,
                                   GLenum destBlendAlpha)
{
    if (mSourceBlendRGB != sourceBlendRGB || mDestBlendRGB != destBlendRGB ||
        mSourceBlendAlpha != sourceBlendAlpha || mDestBlendAlpha != destBlendAlpha)
    {
        mSourceBlendRGB   = sourceBlendRGB;
        mDestBlendRGB     = destBlendRGB;
        mSourceBlendAlpha = sourceBlendAlpha;
        mDestBlendAlpha   = destBlendAlpha;

        mFunctions->blendFuncSeparate(mSourceBlendRGB, mDestBlendRGB, mSourceBlendAlpha,
                                      mDestBlendAlpha);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_BLEND_FUNCS);
    }
}

void StateManagerGL::setBlendEquations(GLenum blendEquationRGB, GLenum blendEquationAlpha)
{
    if (mBlendEquationRGB != blendEquationRGB || mBlendEquationAlpha != blendEquationAlpha)
    {
        mBlendEquationRGB   = blendEquationRGB;
        mBlendEquationAlpha = blendEquationAlpha;

        mFunctions->blendEquationSeparate(mBlendEquationRGB, mBlendEquationAlpha);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_BLEND_EQUATIONS);
    }
}

void StateManagerGL::setColorMask(bool red, bool green, bool blue, bool alpha)
{
    if (mColorMaskRed != red || mColorMaskGreen != green || mColorMaskBlue != blue ||
        mColorMaskAlpha != alpha)
    {
        mColorMaskRed   = red;
        mColorMaskGreen = green;
        mColorMaskBlue  = blue;
        mColorMaskAlpha = alpha;
        mFunctions->colorMask(mColorMaskRed, mColorMaskGreen, mColorMaskBlue, mColorMaskAlpha);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_COLOR_MASK);
    }
}

void StateManagerGL::setSampleAlphaToCoverageEnabled(bool enabled)
{
    if (mSampleAlphaToCoverageEnabled != enabled)
    {
        mSampleAlphaToCoverageEnabled = enabled;
        if (mSampleAlphaToCoverageEnabled)
        {
            mFunctions->enable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }
        else
        {
            mFunctions->disable(GL_SAMPLE_ALPHA_TO_COVERAGE);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED);
    }
}

void StateManagerGL::setSampleCoverageEnabled(bool enabled)
{
    if (mSampleCoverageEnabled != enabled)
    {
        mSampleCoverageEnabled = enabled;
        if (mSampleCoverageEnabled)
        {
            mFunctions->enable(GL_SAMPLE_COVERAGE);
        }
        else
        {
            mFunctions->disable(GL_SAMPLE_COVERAGE);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_COVERAGE_ENABLED);
    }
}

void StateManagerGL::setSampleCoverage(float value, bool invert)
{
    if (mSampleCoverageValue != value || mSampleCoverageInvert != invert)
    {
        mSampleCoverageValue  = value;
        mSampleCoverageInvert = invert;
        mFunctions->sampleCoverage(mSampleCoverageValue, mSampleCoverageInvert);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_COVERAGE);
    }
}

void StateManagerGL::setSampleMaskEnabled(bool enabled)
{
    if (mSampleMaskEnabled != enabled)
    {
        mSampleMaskEnabled = enabled;
        if (mSampleMaskEnabled)
        {
            mFunctions->enable(GL_SAMPLE_MASK);
        }
        else
        {
            mFunctions->disable(GL_SAMPLE_MASK);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_MASK_ENABLED);
    }
}

void StateManagerGL::setSampleMaski(GLuint maskNumber, GLbitfield mask)
{
    ASSERT(maskNumber < mSampleMaskValues.size());
    if (mSampleMaskValues[maskNumber] != mask)
    {
        mSampleMaskValues[maskNumber] = mask;
        mFunctions->sampleMaski(maskNumber, mask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_MASK);
    }
}

void StateManagerGL::setDepthTestEnabled(bool enabled)
{
    if (mDepthTestEnabled != enabled)
    {
        mDepthTestEnabled = enabled;
        if (mDepthTestEnabled)
        {
            mFunctions->enable(GL_DEPTH_TEST);
        }
        else
        {
            mFunctions->disable(GL_DEPTH_TEST);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_DEPTH_TEST_ENABLED);
    }
}

void StateManagerGL::setDepthFunc(GLenum depthFunc)
{
    if (mDepthFunc != depthFunc)
    {
        mDepthFunc = depthFunc;
        mFunctions->depthFunc(mDepthFunc);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_DEPTH_FUNC);
    }
}

void StateManagerGL::setDepthMask(bool mask)
{
    if (mDepthMask != mask)
    {
        mDepthMask = mask;
        mFunctions->depthMask(mDepthMask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_DEPTH_MASK);
    }
}

void StateManagerGL::setStencilTestEnabled(bool enabled)
{
    if (mStencilTestEnabled != enabled)
    {
        mStencilTestEnabled = enabled;
        if (mStencilTestEnabled)
        {
            mFunctions->enable(GL_STENCIL_TEST);
        }
        else
        {
            mFunctions->disable(GL_STENCIL_TEST);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED);
    }
}

void StateManagerGL::setStencilFrontWritemask(GLuint mask)
{
    if (mStencilFrontWritemask != mask)
    {
        mStencilFrontWritemask = mask;
        mFunctions->stencilMaskSeparate(GL_FRONT, mStencilFrontWritemask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_WRITEMASK_FRONT);
    }
}

void StateManagerGL::setStencilBackWritemask(GLuint mask)
{
    if (mStencilBackWritemask != mask)
    {
        mStencilBackWritemask = mask;
        mFunctions->stencilMaskSeparate(GL_BACK, mStencilBackWritemask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK);
    }
}

void StateManagerGL::setStencilFrontFuncs(GLenum func, GLint ref, GLuint mask)
{
    if (mStencilFrontFunc != func || mStencilFrontRef != ref || mStencilFrontValueMask != mask)
    {
        mStencilFrontFunc      = func;
        mStencilFrontRef       = ref;
        mStencilFrontValueMask = mask;
        mFunctions->stencilFuncSeparate(GL_FRONT, mStencilFrontFunc, mStencilFrontRef,
                                        mStencilFrontValueMask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_FUNCS_FRONT);
    }
}

void StateManagerGL::setStencilBackFuncs(GLenum func, GLint ref, GLuint mask)
{
    if (mStencilBackFunc != func || mStencilBackRef != ref || mStencilBackValueMask != mask)
    {
        mStencilBackFunc      = func;
        mStencilBackRef       = ref;
        mStencilBackValueMask = mask;
        mFunctions->stencilFuncSeparate(GL_BACK, mStencilBackFunc, mStencilBackRef,
                                        mStencilBackValueMask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_FUNCS_BACK);
    }
}

void StateManagerGL::setStencilFrontOps(GLenum sfail, GLenum dpfail, GLenum dppass)
{
    if (mStencilFrontStencilFailOp != sfail || mStencilFrontStencilPassDepthFailOp != dpfail ||
        mStencilFrontStencilPassDepthPassOp != dppass)
    {
        mStencilFrontStencilFailOp          = sfail;
        mStencilFrontStencilPassDepthFailOp = dpfail;
        mStencilFrontStencilPassDepthPassOp = dppass;
        mFunctions->stencilOpSeparate(GL_FRONT, mStencilFrontStencilFailOp,
                                      mStencilFrontStencilPassDepthFailOp,
                                      mStencilFrontStencilPassDepthPassOp);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_OPS_FRONT);
    }
}

void StateManagerGL::setStencilBackOps(GLenum sfail, GLenum dpfail, GLenum dppass)
{
    if (mStencilBackStencilFailOp != sfail || mStencilBackStencilPassDepthFailOp != dpfail ||
        mStencilBackStencilPassDepthPassOp != dppass)
    {
        mStencilBackStencilFailOp          = sfail;
        mStencilBackStencilPassDepthFailOp = dpfail;
        mStencilBackStencilPassDepthPassOp = dppass;
        mFunctions->stencilOpSeparate(GL_BACK, mStencilBackStencilFailOp,
                                      mStencilBackStencilPassDepthFailOp,
                                      mStencilBackStencilPassDepthPassOp);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_STENCIL_OPS_BACK);
    }
}

void StateManagerGL::setCullFaceEnabled(bool enabled)
{
    if (mCullFaceEnabled != enabled)
    {
        mCullFaceEnabled = enabled;
        if (mCullFaceEnabled)
        {
            mFunctions->enable(GL_CULL_FACE);
        }
        else
        {
            mFunctions->disable(GL_CULL_FACE);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CULL_FACE_ENABLED);
    }
}

void StateManagerGL::setCullFace(gl::CullFaceMode cullFace)
{
    if (mCullFace != cullFace)
    {
        mCullFace = cullFace;
        mFunctions->cullFace(ToGLenum(mCullFace));

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CULL_FACE);
    }
}

void StateManagerGL::setFrontFace(GLenum frontFace)
{
    if (mFrontFace != frontFace)
    {
        mFrontFace = frontFace;
        mFunctions->frontFace(mFrontFace);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_FRONT_FACE);
    }
}

void StateManagerGL::setPolygonOffsetFillEnabled(bool enabled)
{
    if (mPolygonOffsetFillEnabled != enabled)
    {
        mPolygonOffsetFillEnabled = enabled;
        if (mPolygonOffsetFillEnabled)
        {
            mFunctions->enable(GL_POLYGON_OFFSET_FILL);
        }
        else
        {
            mFunctions->disable(GL_POLYGON_OFFSET_FILL);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED);
    }
}

void StateManagerGL::setPolygonOffset(float factor, float units)
{
    if (mPolygonOffsetFactor != factor || mPolygonOffsetUnits != units)
    {
        mPolygonOffsetFactor = factor;
        mPolygonOffsetUnits  = units;
        mFunctions->polygonOffset(mPolygonOffsetFactor, mPolygonOffsetUnits);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_POLYGON_OFFSET);
    }
}

void StateManagerGL::setRasterizerDiscardEnabled(bool enabled)
{
    if (mRasterizerDiscardEnabled != enabled)
    {
        mRasterizerDiscardEnabled = enabled;
        if (mRasterizerDiscardEnabled)
        {
            mFunctions->enable(GL_RASTERIZER_DISCARD);
        }
        else
        {
            mFunctions->disable(GL_RASTERIZER_DISCARD);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_RASTERIZER_DISCARD_ENABLED);
    }
}

void StateManagerGL::setLineWidth(float width)
{
    if (mLineWidth != width)
    {
        mLineWidth = width;
        mFunctions->lineWidth(mLineWidth);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_LINE_WIDTH);
    }
}

void StateManagerGL::setPrimitiveRestartEnabled(bool enabled)
{
    if (mPrimitiveRestartEnabled != enabled)
    {
        mPrimitiveRestartEnabled = enabled;

        if (mPrimitiveRestartEnabled)
        {
            mFunctions->enable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        }
        else
        {
            mFunctions->disable(GL_PRIMITIVE_RESTART_FIXED_INDEX);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PRIMITIVE_RESTART_ENABLED);
    }
}

void StateManagerGL::setClearDepth(float clearDepth)
{
    if (mClearDepth != clearDepth)
    {
        mClearDepth = clearDepth;

        // The glClearDepthf function isn't available until OpenGL 4.1.  Prefer it when it is
        // available because OpenGL ES only works in floats.
        if (mFunctions->clearDepthf)
        {
            mFunctions->clearDepthf(mClearDepth);
        }
        else
        {
            ASSERT(mFunctions->clearDepth);
            mFunctions->clearDepth(mClearDepth);
        }

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CLEAR_DEPTH);
    }
}

void StateManagerGL::setClearColor(const gl::ColorF &clearColor)
{
    if (mClearColor != clearColor)
    {
        mClearColor = clearColor;
        mFunctions->clearColor(mClearColor.red, mClearColor.green, mClearColor.blue,
                               mClearColor.alpha);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CLEAR_COLOR);
    }
}

void StateManagerGL::setClearStencil(GLint clearStencil)
{
    if (mClearStencil != clearStencil)
    {
        mClearStencil = clearStencil;
        mFunctions->clearStencil(mClearStencil);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_CLEAR_STENCIL);
    }
}

void StateManagerGL::syncState(const gl::Context *context,
                               const gl::State::DirtyBits &glDirtyBits,
                               const gl::State::DirtyBits &bitMask)
{
    const gl::State &state = context->getGLState();

    // Changing the draw framebuffer binding sometimes requires resetting srgb blending.
    if (glDirtyBits[gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING])
    {
        if (mFunctions->standard == STANDARD_GL_DESKTOP)
        {
            mLocalDirtyBits.set(gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB);
        }

        if (mIsMultiviewEnabled)
        {
            // When a new draw framebuffer is bound, we have to mark the layout, viewport offsets,
            // scissor test, scissor and viewport rectangle bits as dirty because it could be a
            // transition from or to a side-by-side draw framebuffer.
            mMultiviewDirtyBits.set(MULTIVIEW_DIRTY_BIT_SIDE_BY_SIDE_LAYOUT);
            mMultiviewDirtyBits.set(MULTIVIEW_DIRTY_BIT_VIEWPORT_OFFSETS);
            mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR_TEST_ENABLED);
            mLocalDirtyBits.set(gl::State::DIRTY_BIT_SCISSOR);
            mLocalDirtyBits.set(gl::State::DIRTY_BIT_VIEWPORT);
        }
    }

    // Iterate over and resolve multi-view dirty bits.
    if (mMultiviewDirtyBits.any())
    {
        for (auto dirtyBit : mMultiviewDirtyBits)
        {
            switch (dirtyBit)
            {
                case MULTIVIEW_DIRTY_BIT_SIDE_BY_SIDE_LAYOUT:
                {
                    const gl::Framebuffer *drawFramebuffer = state.getDrawFramebuffer();
                    ASSERT(drawFramebuffer != nullptr);
                    setSideBySide(drawFramebuffer->getMultiviewLayout() ==
                                  GL_FRAMEBUFFER_MULTIVIEW_SIDE_BY_SIDE_ANGLE);
                }
                break;
                case MULTIVIEW_DIRTY_BIT_VIEWPORT_OFFSETS:
                {
                    const gl::Framebuffer *drawFramebuffer = state.getDrawFramebuffer();
                    ASSERT(drawFramebuffer != nullptr);
                    const std::vector<gl::Offset> *attachmentViewportOffsets =
                        drawFramebuffer->getViewportOffsets();
                    const std::vector<gl::Offset> &viewportOffsets =
                        attachmentViewportOffsets != nullptr
                            ? *attachmentViewportOffsets
                            : gl::FramebufferAttachment::GetDefaultViewportOffsetVector();
                    setViewportOffsets(viewportOffsets);
                }
                break;
                default:
                    UNREACHABLE();
            }
        }
        mMultiviewDirtyBits.reset();
    }

    const gl::State::DirtyBits &glAndLocalDirtyBits = (glDirtyBits | mLocalDirtyBits) & bitMask;

    if (!glAndLocalDirtyBits.any())
    {
        return;
    }

    // TODO(jmadill): Investigate only syncing vertex state for active attributes
    for (auto iter = glAndLocalDirtyBits.begin(), endIter = glAndLocalDirtyBits.end();
         iter != endIter; ++iter)
    {
        switch (*iter)
        {
            case gl::State::DIRTY_BIT_SCISSOR_TEST_ENABLED:
                setScissorTestEnabled(state.isScissorTestEnabled());
                break;
            case gl::State::DIRTY_BIT_SCISSOR:
            {
                const gl::Rectangle &scissor = state.getScissor();
                if (!mIsSideBySideDrawFramebuffer)
                {
                    setScissor(scissor);
                }
                else
                {
                    const gl::Framebuffer *drawFramebuffer = state.getDrawFramebuffer();
                    ASSERT(drawFramebuffer != nullptr);
                    applyViewportOffsetsAndSetScissors(scissor, *drawFramebuffer);
                }
            }
            break;
            case gl::State::DIRTY_BIT_VIEWPORT:
            {
                const gl::Rectangle &viewport = state.getViewport();
                if (!mIsSideBySideDrawFramebuffer)
                {
                    setViewport(viewport);
                }
                else
                {
                    const gl::Framebuffer *drawFramebuffer = state.getDrawFramebuffer();
                    ASSERT(drawFramebuffer != nullptr);
                    applyViewportOffsetsAndSetViewports(viewport, *drawFramebuffer);
                }
            }
            break;
            case gl::State::DIRTY_BIT_DEPTH_RANGE:
                setDepthRange(state.getNearPlane(), state.getFarPlane());
                break;
            case gl::State::DIRTY_BIT_BLEND_ENABLED:
                setBlendEnabled(state.isBlendEnabled());
                break;
            case gl::State::DIRTY_BIT_BLEND_COLOR:
                setBlendColor(state.getBlendColor());
                break;
            case gl::State::DIRTY_BIT_BLEND_FUNCS:
            {
                const auto &blendState = state.getBlendState();
                setBlendFuncs(blendState.sourceBlendRGB, blendState.destBlendRGB,
                              blendState.sourceBlendAlpha, blendState.destBlendAlpha);
                break;
            }
            case gl::State::DIRTY_BIT_BLEND_EQUATIONS:
            {
                const auto &blendState = state.getBlendState();
                setBlendEquations(blendState.blendEquationRGB, blendState.blendEquationAlpha);
                break;
            }
            case gl::State::DIRTY_BIT_COLOR_MASK:
            {
                const auto &blendState = state.getBlendState();
                setColorMask(blendState.colorMaskRed, blendState.colorMaskGreen,
                             blendState.colorMaskBlue, blendState.colorMaskAlpha);
                break;
            }
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED:
                setSampleAlphaToCoverageEnabled(state.isSampleAlphaToCoverageEnabled());
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE_ENABLED:
                setSampleCoverageEnabled(state.isSampleCoverageEnabled());
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE:
                setSampleCoverage(state.getSampleCoverageValue(), state.getSampleCoverageInvert());
                break;
            case gl::State::DIRTY_BIT_DEPTH_TEST_ENABLED:
                setDepthTestEnabled(state.isDepthTestEnabled());
                break;
            case gl::State::DIRTY_BIT_DEPTH_FUNC:
                setDepthFunc(state.getDepthStencilState().depthFunc);
                break;
            case gl::State::DIRTY_BIT_DEPTH_MASK:
                setDepthMask(state.getDepthStencilState().depthMask);
                break;
            case gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED:
                setStencilTestEnabled(state.isStencilTestEnabled());
                break;
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_FRONT:
            {
                const auto &depthStencilState = state.getDepthStencilState();
                setStencilFrontFuncs(depthStencilState.stencilFunc, state.getStencilRef(),
                                     depthStencilState.stencilMask);
                break;
            }
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_BACK:
            {
                const auto &depthStencilState = state.getDepthStencilState();
                setStencilBackFuncs(depthStencilState.stencilBackFunc, state.getStencilBackRef(),
                                    depthStencilState.stencilBackMask);
                break;
            }
            case gl::State::DIRTY_BIT_STENCIL_OPS_FRONT:
            {
                const auto &depthStencilState = state.getDepthStencilState();
                setStencilFrontOps(depthStencilState.stencilFail,
                                   depthStencilState.stencilPassDepthFail,
                                   depthStencilState.stencilPassDepthPass);
                break;
            }
            case gl::State::DIRTY_BIT_STENCIL_OPS_BACK:
            {
                const auto &depthStencilState = state.getDepthStencilState();
                setStencilBackOps(depthStencilState.stencilBackFail,
                                  depthStencilState.stencilBackPassDepthFail,
                                  depthStencilState.stencilBackPassDepthPass);
                break;
            }
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_FRONT:
                setStencilFrontWritemask(state.getDepthStencilState().stencilWritemask);
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK:
                setStencilBackWritemask(state.getDepthStencilState().stencilBackWritemask);
                break;
            case gl::State::DIRTY_BIT_CULL_FACE_ENABLED:
                setCullFaceEnabled(state.isCullFaceEnabled());
                break;
            case gl::State::DIRTY_BIT_CULL_FACE:
                setCullFace(state.getRasterizerState().cullMode);
                break;
            case gl::State::DIRTY_BIT_FRONT_FACE:
                setFrontFace(state.getRasterizerState().frontFace);
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED:
                setPolygonOffsetFillEnabled(state.isPolygonOffsetFillEnabled());
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET:
            {
                const auto &rasterizerState = state.getRasterizerState();
                setPolygonOffset(rasterizerState.polygonOffsetFactor,
                                 rasterizerState.polygonOffsetUnits);
                break;
            }
            case gl::State::DIRTY_BIT_RASTERIZER_DISCARD_ENABLED:
                setRasterizerDiscardEnabled(state.isRasterizerDiscardEnabled());
                break;
            case gl::State::DIRTY_BIT_LINE_WIDTH:
                setLineWidth(state.getLineWidth());
                break;
            case gl::State::DIRTY_BIT_PRIMITIVE_RESTART_ENABLED:
                setPrimitiveRestartEnabled(state.isPrimitiveRestartEnabled());
                break;
            case gl::State::DIRTY_BIT_CLEAR_COLOR:
                setClearColor(state.getColorClearValue());
                break;
            case gl::State::DIRTY_BIT_CLEAR_DEPTH:
                setClearDepth(state.getDepthClearValue());
                break;
            case gl::State::DIRTY_BIT_CLEAR_STENCIL:
                setClearStencil(state.getStencilClearValue());
                break;
            case gl::State::DIRTY_BIT_UNPACK_STATE:
                setPixelUnpackState(state.getUnpackState());
                break;
            case gl::State::DIRTY_BIT_UNPACK_BUFFER_BINDING:
                setPixelUnpackBuffer(state.getTargetBuffer(gl::BufferBinding::PixelUnpack));
                break;
            case gl::State::DIRTY_BIT_PACK_STATE:
                setPixelPackState(state.getPackState());
                break;
            case gl::State::DIRTY_BIT_PACK_BUFFER_BINDING:
                setPixelPackBuffer(state.getTargetBuffer(gl::BufferBinding::PixelPack));
                break;
            case gl::State::DIRTY_BIT_DITHER_ENABLED:
                setDitherEnabled(state.isDitherEnabled());
                break;
            case gl::State::DIRTY_BIT_GENERATE_MIPMAP_HINT:
                // TODO(jmadill): implement this
                break;
            case gl::State::DIRTY_BIT_SHADER_DERIVATIVE_HINT:
                // TODO(jmadill): implement this
                break;
            case gl::State::DIRTY_BIT_READ_FRAMEBUFFER_BINDING:
            {
                gl::Framebuffer *framebuffer = state.getReadFramebuffer();

                // Necessary for an Intel TexImage workaround.
                if (!framebuffer)
                    continue;

                FramebufferGL *framebufferGL = GetImplAs<FramebufferGL>(framebuffer);
                bindFramebuffer(GL_READ_FRAMEBUFFER, framebufferGL->getFramebufferID());
                break;
            }
            case gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING:
            {
                gl::Framebuffer *framebuffer = state.getDrawFramebuffer();

                // Necessary for an Intel TexImage workaround.
                if (!framebuffer)
                    continue;

                FramebufferGL *framebufferGL = GetImplAs<FramebufferGL>(framebuffer);
                bindFramebuffer(GL_DRAW_FRAMEBUFFER, framebufferGL->getFramebufferID());

                const gl::Program *program = state.getProgram();
                if (program)
                {
                    updateMultiviewBaseViewLayerIndexUniform(program, framebufferGL->getState());
                }
                break;
            }
            case gl::State::DIRTY_BIT_RENDERBUFFER_BINDING:
                // TODO(jmadill): implement this
                break;
            case gl::State::DIRTY_BIT_VERTEX_ARRAY_BINDING:
            {
                const VertexArrayGL *vaoGL = GetImplAs<VertexArrayGL>(state.getVertexArray());
                bindVertexArray(vaoGL->getVertexArrayID(), vaoGL->getAppliedElementArrayBufferID());

                propagateProgramToVAO(state.getProgram(),
                                      GetImplAs<VertexArrayGL>(state.getVertexArray()));
                break;
            }
            case gl::State::DIRTY_BIT_DRAW_INDIRECT_BUFFER_BINDING:
                updateDrawIndirectBufferBinding(context);
                break;
            case gl::State::DIRTY_BIT_DISPATCH_INDIRECT_BUFFER_BINDING:
                updateDispatchIndirectBufferBinding(context);
                break;
            case gl::State::DIRTY_BIT_PROGRAM_BINDING:
            {
                gl::Program *program = state.getProgram();
                if (program != nullptr)
                {
                    useProgram(GetImplAs<ProgramGL>(program)->getProgramID());
                }
                break;
            }
            case gl::State::DIRTY_BIT_PROGRAM_EXECUTABLE:
            {
                const gl::Program *program = state.getProgram();
                if (program)
                {
                    iter.setLaterBit(gl::State::DIRTY_BIT_TEXTURE_BINDINGS);

                    if (program->getActiveImagesMask().any())
                    {
                        iter.setLaterBit(gl::State::DIRTY_BIT_IMAGE_BINDINGS);
                    }

                    if (program->getActiveShaderStorageBlockCount() > 0)
                    {
                        iter.setLaterBit(gl::State::DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING);
                    }

                    if (program->getActiveUniformBlockCount() > 0)
                    {
                        iter.setLaterBit(gl::State::DIRTY_BIT_UNIFORM_BUFFER_BINDINGS);
                    }

                    if (program->getActiveAtomicCounterBufferCount() > 0)
                    {
                        iter.setLaterBit(gl::State::DIRTY_BIT_ATOMIC_COUNTER_BUFFER_BINDING);
                    }

                    if (mIsMultiviewEnabled && program->usesMultiview())
                    {
                        updateMultiviewBaseViewLayerIndexUniform(
                            program, state.getDrawFramebuffer()->getImplementation()->getState());
                    }
                }

                if (!program || !program->hasLinkedShaderStage(gl::ShaderType::Compute))
                {
                    propagateProgramToVAO(program,
                                          GetImplAs<VertexArrayGL>(state.getVertexArray()));
                }
                break;
            }
            case gl::State::DIRTY_BIT_TEXTURE_BINDINGS:
                updateProgramTextureBindings(context);
                break;
            case gl::State::DIRTY_BIT_SAMPLER_BINDINGS:
                syncSamplersState(context);
                break;
            case gl::State::DIRTY_BIT_IMAGE_BINDINGS:
                updateProgramImageBindings(context);
                break;
            case gl::State::DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING:
                syncTransformFeedbackState(context);
                break;
            case gl::State::DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING:
                updateProgramStorageBufferBindings(context);
                break;
            case gl::State::DIRTY_BIT_UNIFORM_BUFFER_BINDINGS:
                updateProgramUniformBufferBindings(context);
                break;
            case gl::State::DIRTY_BIT_ATOMIC_COUNTER_BUFFER_BINDING:
                updateProgramAtomicCounterBufferBindings(context);
                break;
            case gl::State::DIRTY_BIT_MULTISAMPLING:
                setMultisamplingStateEnabled(state.isMultisamplingEnabled());
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_ONE:
                setSampleAlphaToOneStateEnabled(state.isSampleAlphaToOneEnabled());
                break;
            case gl::State::DIRTY_BIT_COVERAGE_MODULATION:
                setCoverageModulation(state.getCoverageModulation());
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING:
                setPathRenderingModelViewMatrix(
                    state.getPathRenderingMatrix(GL_PATH_MODELVIEW_MATRIX_CHROMIUM));
                setPathRenderingProjectionMatrix(
                    state.getPathRenderingMatrix(GL_PATH_PROJECTION_MATRIX_CHROMIUM));
                setPathRenderingStencilState(state.getPathStencilFunc(), state.getPathStencilRef(),
                                             state.getPathStencilMask());
                break;
            case gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB:
                setFramebufferSRGBEnabledForFramebuffer(
                    context, state.getFramebufferSRGB(),
                    GetImplAs<FramebufferGL>(state.getDrawFramebuffer()));
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK_ENABLED:
                setSampleMaskEnabled(state.isSampleMaskEnabled());
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK:
            {
                for (GLuint maskNumber = 0; maskNumber < state.getMaxSampleMaskWords();
                     ++maskNumber)
                {
                    setSampleMaski(maskNumber, state.getSampleMaskWord(maskNumber));
                }
                break;
            }
            case gl::State::DIRTY_BIT_CURRENT_VALUES:
            {
                gl::AttributesMask combinedMask =
                    (state.getAndResetDirtyCurrentValues() | mLocalDirtyCurrentValues);
                mLocalDirtyCurrentValues.reset();

                for (auto attribIndex : combinedMask)
                {
                    setAttributeCurrentData(attribIndex,
                                            state.getVertexAttribCurrentValue(attribIndex));
                }
                break;
            }
            default:
                UNREACHABLE();
                break;
        }
    }

    mLocalDirtyBits &= ~(bitMask);
}

void StateManagerGL::setFramebufferSRGBEnabled(const gl::Context *context, bool enabled)
{
    if (!context->getExtensions().sRGBWriteControl)
    {
        return;
    }

    if (mFramebufferSRGBEnabled != enabled)
    {
        mFramebufferSRGBEnabled = enabled;
        if (mFramebufferSRGBEnabled)
        {
            mFunctions->enable(GL_FRAMEBUFFER_SRGB);
        }
        else
        {
            mFunctions->disable(GL_FRAMEBUFFER_SRGB);
        }
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB);
    }
}

void StateManagerGL::setFramebufferSRGBEnabledForFramebuffer(const gl::Context *context,
                                                             bool enabled,
                                                             const FramebufferGL *framebuffer)
{
    if (mFunctions->standard == STANDARD_GL_DESKTOP && framebuffer->isDefault())
    {
        // Obey the framebuffer sRGB state for blending on all framebuffers except the default
        // framebuffer on Desktop OpenGL.
        // When SRGB blending is enabled, only SRGB capable formats will use it but the default
        // framebuffer will always use it if it is enabled.
        // TODO(geofflang): Update this when the framebuffer binding dirty changes, when it exists.
        setFramebufferSRGBEnabled(context, false);
    }
    else
    {
        setFramebufferSRGBEnabled(context, enabled);
    }
}

void StateManagerGL::setDitherEnabled(bool enabled)
{
    if (mDitherEnabled != enabled)
    {
        mDitherEnabled = enabled;
        if (mDitherEnabled)
        {
            mFunctions->enable(GL_DITHER);
        }
        else
        {
            mFunctions->disable(GL_DITHER);
        }
    }
}

void StateManagerGL::setMultisamplingStateEnabled(bool enabled)
{
    if (mMultisamplingEnabled != enabled)
    {
        mMultisamplingEnabled = enabled;
        if (mMultisamplingEnabled)
        {
            mFunctions->enable(GL_MULTISAMPLE_EXT);
        }
        else
        {
            mFunctions->disable(GL_MULTISAMPLE_EXT);
        }
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_MULTISAMPLING);
    }
}

void StateManagerGL::setSampleAlphaToOneStateEnabled(bool enabled)
{
    if (mSampleAlphaToOneEnabled != enabled)
    {
        mSampleAlphaToOneEnabled = enabled;
        if (mSampleAlphaToOneEnabled)
        {
            mFunctions->enable(GL_SAMPLE_ALPHA_TO_ONE);
        }
        else
        {
            mFunctions->disable(GL_SAMPLE_ALPHA_TO_ONE);
        }
        mLocalDirtyBits.set(gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_ONE);
    }
}

void StateManagerGL::setCoverageModulation(GLenum components)
{
    if (mCoverageModulation != components)
    {
        mCoverageModulation = components;
        mFunctions->coverageModulationNV(components);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_COVERAGE_MODULATION);
    }
}

void StateManagerGL::setPathRenderingModelViewMatrix(const GLfloat *m)
{
    if (memcmp(mPathMatrixMV, m, sizeof(mPathMatrixMV)) != 0)
    {
        memcpy(mPathMatrixMV, m, sizeof(mPathMatrixMV));
        mFunctions->matrixLoadfEXT(GL_PATH_MODELVIEW_CHROMIUM, m);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PATH_RENDERING);
    }
}

void StateManagerGL::setPathRenderingProjectionMatrix(const GLfloat *m)
{
    if (memcmp(mPathMatrixProj, m, sizeof(mPathMatrixProj)) != 0)
    {
        memcpy(mPathMatrixProj, m, sizeof(mPathMatrixProj));
        mFunctions->matrixLoadfEXT(GL_PATH_PROJECTION_CHROMIUM, m);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PATH_RENDERING);
    }
}

void StateManagerGL::setPathRenderingStencilState(GLenum func, GLint ref, GLuint mask)
{
    if (func != mPathStencilFunc || ref != mPathStencilRef || mask != mPathStencilMask)
    {
        mPathStencilFunc = func;
        mPathStencilRef  = ref;
        mPathStencilMask = mask;
        mFunctions->pathStencilFuncNV(func, ref, mask);

        mLocalDirtyBits.set(gl::State::DIRTY_BIT_PATH_RENDERING);
    }
}

void StateManagerGL::setTextureCubemapSeamlessEnabled(bool enabled)
{
    // TODO(jmadill): Also check for seamless extension.
    if (!mFunctions->isAtLeastGL(gl::Version(3, 2)))
    {
        return;
    }

    if (mTextureCubemapSeamlessEnabled != enabled)
    {
        mTextureCubemapSeamlessEnabled = enabled;
        if (mTextureCubemapSeamlessEnabled)
        {
            mFunctions->enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
        else
        {
            mFunctions->disable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        }
    }
}

// According to the ANGLE_multiview spec, the behavior of glScissor and glViewport is different
// if the active draw framebuffer has a side-by-side layout. In such situations glScissor and
// glViewport specify the static offset and dimensions for each view to which the viewport
// offsets have to be applied to compute the view's final scissor and viewport rectangles.
void StateManagerGL::applyViewportOffsetsAndSetScissors(const gl::Rectangle &scissor,
                                                        const gl::Framebuffer &drawFramebuffer)
{

    const std::vector<gl::Offset> *attachmentViewportOffsets = drawFramebuffer.getViewportOffsets();
    const std::vector<gl::Offset> &viewportOffsets =
        attachmentViewportOffsets != nullptr
            ? *attachmentViewportOffsets
            : gl::FramebufferAttachment::GetDefaultViewportOffsetVector();
    const std::vector<gl::Rectangle> &scissorArray = ApplyOffsets(scissor, viewportOffsets);
    setScissorArrayv(0u, scissorArray);
}

void StateManagerGL::applyViewportOffsetsAndSetViewports(const gl::Rectangle &viewport,
                                                         const gl::Framebuffer &drawFramebuffer)
{
    const std::vector<gl::Offset> *attachmentViewportOffsets = drawFramebuffer.getViewportOffsets();
    const std::vector<gl::Offset> &viewportOffsets =
        attachmentViewportOffsets != nullptr
            ? *attachmentViewportOffsets
            : gl::FramebufferAttachment::GetDefaultViewportOffsetVector();
    const std::vector<gl::Rectangle> &viewportArray = ApplyOffsets(viewport, viewportOffsets);
    setViewportArrayv(0u, viewportArray);
}

void StateManagerGL::propagateProgramToVAO(const gl::Program *program, VertexArrayGL *vao)
{
    if (vao == nullptr)
    {
        return;
    }

    // Number of views:
    if (mIsMultiviewEnabled)
    {
        int programNumViews = 1;
        if (program && program->usesMultiview())
        {
            programNumViews = program->getNumViews();
        }
        vao->applyNumViewsToDivisor(programNumViews);
    }

    // Attribute enabled mask:
    if (program)
    {
        vao->applyActiveAttribLocationsMask(program->getActiveAttribLocationsMask());
    }
}

void StateManagerGL::updateMultiviewBaseViewLayerIndexUniformImpl(
    const gl::Program *program,
    const gl::FramebufferState &drawFramebufferState) const
{
    ASSERT(mIsMultiviewEnabled && program && program->usesMultiview());
    const ProgramGL *programGL = GetImplAs<ProgramGL>(program);
    switch (drawFramebufferState.getMultiviewLayout())
    {
        case GL_FRAMEBUFFER_MULTIVIEW_SIDE_BY_SIDE_ANGLE:
            programGL->enableSideBySideRenderingPath();
            break;
        case GL_FRAMEBUFFER_MULTIVIEW_LAYERED_ANGLE:
            programGL->enableLayeredRenderingPath(drawFramebufferState.getBaseViewIndex());
            break;
        default:
            break;
    }
}

void StateManagerGL::syncSamplersState(const gl::Context *context)
{
    const gl::State::SamplerBindingVector &samplers = context->getGLState().getSamplers();

    // This could be optimized by using a separate binding dirty bit per sampler.
    for (size_t samplerIndex = 0; samplerIndex < samplers.size(); ++samplerIndex)
    {
        const gl::Sampler *sampler = samplers[samplerIndex].get();
        if (sampler != nullptr)
        {
            SamplerGL *samplerGL = GetImplAs<SamplerGL>(sampler);
            bindSampler(samplerIndex, samplerGL->getSamplerID());
        }
        else
        {
            bindSampler(samplerIndex, 0);
        }
    }
}

void StateManagerGL::syncTransformFeedbackState(const gl::Context *context)
{
    // Set the current transform feedback state
    gl::TransformFeedback *transformFeedback = context->getGLState().getCurrentTransformFeedback();
    if (transformFeedback)
    {
        TransformFeedbackGL *transformFeedbackGL =
            GetImplAs<TransformFeedbackGL>(transformFeedback);
        bindTransformFeedback(GL_TRANSFORM_FEEDBACK, transformFeedbackGL->getTransformFeedbackID());
        transformFeedbackGL->syncActiveState(transformFeedback->isActive(),
                                             transformFeedback->getPrimitiveMode());
        transformFeedbackGL->syncPausedState(transformFeedback->isPaused());
        mCurrentTransformFeedback = transformFeedbackGL;
    }
    else
    {
        bindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
        mCurrentTransformFeedback = nullptr;
    }
}
}

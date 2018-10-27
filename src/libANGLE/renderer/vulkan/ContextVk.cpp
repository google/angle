//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextVk.cpp:
//    Implements the class methods for ContextVk.
//

#include "libANGLE/renderer/vulkan/ContextVk.h"

#include "common/bitset_utils.h"
#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/Program.h"
#include "libANGLE/Surface.h"
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/CompilerVk.h"
#include "libANGLE/renderer/vulkan/FenceNVVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/ProgramPipelineVk.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/QueryVk.h"
#include "libANGLE/renderer/vulkan/RenderbufferVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/SamplerVk.h"
#include "libANGLE/renderer/vulkan/ShaderVk.h"
#include "libANGLE/renderer/vulkan/SyncVk.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"
#include "libANGLE/renderer/vulkan/TransformFeedbackVk.h"
#include "libANGLE/renderer/vulkan/VertexArrayVk.h"

namespace rx
{

namespace
{
GLenum DefaultGLErrorCode(VkResult result)
{
    switch (result)
    {
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_TOO_MANY_OBJECTS:
            return GL_OUT_OF_MEMORY;
        default:
            return GL_INVALID_OPERATION;
    }
}

void BindNonNullVertexBufferRanges(vk::CommandBuffer *commandBuffer,
                                   const gl::AttributesMask &nonNullAttribMask,
                                   uint32_t maxAttrib,
                                   const gl::AttribArray<VkBuffer> &arrayBufferHandles,
                                   const gl::AttribArray<VkDeviceSize> &arrayBufferOffsets)
{
    // Vulkan does not allow binding a null vertex buffer but the default state of null buffers is
    // valid.

    // We can detect if there are no gaps in active attributes by using the mask of the program
    // attribs and the max enabled attrib.
    ASSERT(maxAttrib > 0);
    if (nonNullAttribMask.to_ulong() == (maxAttrib - 1))
    {
        commandBuffer->bindVertexBuffers(0, maxAttrib, arrayBufferHandles.data(),
                                         arrayBufferOffsets.data());
        return;
    }

    // Find ranges of non-null buffers and bind them all together.
    for (uint32_t attribIdx = 0; attribIdx < maxAttrib; attribIdx++)
    {
        if (arrayBufferHandles[attribIdx] != VK_NULL_HANDLE)
        {
            // Find the end of this range of non-null handles
            uint32_t rangeCount = 1;
            while (attribIdx + rangeCount < maxAttrib &&
                   arrayBufferHandles[attribIdx + rangeCount] != VK_NULL_HANDLE)
            {
                rangeCount++;
            }

            commandBuffer->bindVertexBuffers(attribIdx, rangeCount, &arrayBufferHandles[attribIdx],
                                             &arrayBufferOffsets[attribIdx]);
            attribIdx += rangeCount;
        }
    }
}

constexpr gl::Rectangle kMaxSizedScissor(0,
                                         0,
                                         std::numeric_limits<int>::max(),
                                         std::numeric_limits<int>::max());

constexpr VkColorComponentFlags kAllColorChannelsMask =
    (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
     VK_COLOR_COMPONENT_A_BIT);

constexpr VkBufferUsageFlags kVertexBufferUsage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
constexpr size_t kDefaultValueSize              = sizeof(float) * 4;
constexpr size_t kDefaultBufferSize             = kDefaultValueSize * 16;
}  // anonymous namespace

// std::array only uses aggregate init. Thus we make a helper macro to reduce on code duplication.
#define INIT                                   \
    {                                          \
        kVertexBufferUsage, kDefaultBufferSize \
    }

ContextVk::ContextVk(const gl::ContextState &state, RendererVk *renderer)
    : ContextImpl(state),
      vk::Context(renderer),
      mCurrentDrawMode(gl::PrimitiveMode::InvalidEnum),
      mVertexArray(nullptr),
      mDrawFramebuffer(nullptr),
      mProgram(nullptr),
      mLastIndexBufferOffset(0),
      mCurrentDrawElementsType(GL_NONE),
      mClearColorMask(kAllColorChannelsMask),
      mFlipYForCurrentSurface(false),
      mDriverUniformsBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(DriverUniforms) * 16),
      mDriverUniformsDescriptorSet(VK_NULL_HANDLE),
      mDefaultAttribBuffers{{INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT, INIT,
                             INIT, INIT, INIT, INIT}}
{
    memset(&mClearColorValue, 0, sizeof(mClearColorValue));
    memset(&mClearDepthStencilValue, 0, sizeof(mClearDepthStencilValue));

    mNonIndexedDirtyBitsMask.set();
    mNonIndexedDirtyBitsMask.reset(DIRTY_BIT_INDEX_BUFFER);

    mIndexedDirtyBitsMask.set();

    mNewCommandBufferDirtyBits.set(DIRTY_BIT_PIPELINE);
    mNewCommandBufferDirtyBits.set(DIRTY_BIT_TEXTURES);
    mNewCommandBufferDirtyBits.set(DIRTY_BIT_VERTEX_BUFFERS);
    mNewCommandBufferDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
    mNewCommandBufferDirtyBits.set(DIRTY_BIT_DESCRIPTOR_SETS);

    mDirtyBitHandlers[DIRTY_BIT_DEFAULT_ATTRIBS] = &ContextVk::handleDirtyDefaultAttribs;
    mDirtyBitHandlers[DIRTY_BIT_PIPELINE]        = &ContextVk::handleDirtyPipeline;
    mDirtyBitHandlers[DIRTY_BIT_TEXTURES]        = &ContextVk::handleDirtyTextures;
    mDirtyBitHandlers[DIRTY_BIT_VERTEX_BUFFERS]  = &ContextVk::handleDirtyVertexBuffers;
    mDirtyBitHandlers[DIRTY_BIT_INDEX_BUFFER]    = &ContextVk::handleDirtyIndexBuffer;
    mDirtyBitHandlers[DIRTY_BIT_DRIVER_UNIFORMS] = &ContextVk::handleDirtyDriverUniforms;
    mDirtyBitHandlers[DIRTY_BIT_DESCRIPTOR_SETS] = &ContextVk::handleDirtyDescriptorSets;

    mDirtyBits = mNewCommandBufferDirtyBits;
}

#undef INIT

ContextVk::~ContextVk() = default;

void ContextVk::onDestroy(const gl::Context *context)
{
    mDriverUniformsSetLayout.reset();
    mIncompleteTextures.onDestroy(context);
    mDriverUniformsBuffer.destroy(getDevice());
    mDriverUniformsDescriptorPoolBinding.reset();

    for (vk::DynamicDescriptorPool &descriptorPool : mDynamicDescriptorPools)
    {
        descriptorPool.destroy(getDevice());
    }

    for (vk::DynamicBuffer &defaultBuffer : mDefaultAttribBuffers)
    {
        defaultBuffer.destroy(getDevice());
    }

    for (vk::DynamicQueryPool &queryPool : mQueryPools)
    {
        queryPool.destroy(getDevice());
    }
}

gl::Error ContextVk::getIncompleteTexture(const gl::Context *context,
                                          gl::TextureType type,
                                          gl::Texture **textureOut)
{
    // At some point, we'll need to support multisample and we'll pass "this" instead of nullptr
    // and implement the necessary interface.
    return mIncompleteTextures.getIncompleteTexture(context, type, nullptr, textureOut);
}

angle::Result ContextVk::initialize()
{
    // Note that this may reserve more sets than strictly necessary for a particular layout.
    ANGLE_TRY(mDynamicDescriptorPools[kUniformsDescriptorSetIndex].init(
        this, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, GetUniformBufferDescriptorCount()));
    ANGLE_TRY(mDynamicDescriptorPools[kTextureDescriptorSetIndex].init(
        this, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, mRenderer->getMaxActiveTextures()));
    ANGLE_TRY(mDynamicDescriptorPools[kDriverUniformsDescriptorSetIndex].init(
        this, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1));

    ANGLE_TRY(mQueryPools[gl::QueryType::AnySamples].init(this, VK_QUERY_TYPE_OCCLUSION,
                                                          vk::kDefaultOcclusionQueryPoolSize));
    ANGLE_TRY(mQueryPools[gl::QueryType::AnySamplesConservative].init(
        this, VK_QUERY_TYPE_OCCLUSION, vk::kDefaultOcclusionQueryPoolSize));
    ANGLE_TRY(mQueryPools[gl::QueryType::Timestamp].init(this, VK_QUERY_TYPE_TIMESTAMP,
                                                         vk::kDefaultTimestampQueryPoolSize));
    ANGLE_TRY(mQueryPools[gl::QueryType::TimeElapsed].init(this, VK_QUERY_TYPE_TIMESTAMP,
                                                           vk::kDefaultTimestampQueryPoolSize));
    // TODO(syoussefi): Initialize other query pools as they get implemented.

    size_t minAlignment = static_cast<size_t>(
        mRenderer->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
    mDriverUniformsBuffer.init(minAlignment, mRenderer);

    mPipelineDesc.reset(new vk::PipelineDesc());
    mPipelineDesc->initDefaults();

    // Initialize current value/default attribute buffers.
    for (vk::DynamicBuffer &buffer : mDefaultAttribBuffers)
    {
        buffer.init(1, mRenderer);
    }

    return angle::Result::Continue();
}

angle::Result ContextVk::flush(const gl::Context *context)
{
    return mRenderer->flush(this);
}

angle::Result ContextVk::finish(const gl::Context *context)
{
    return mRenderer->finish(this);
}

angle::Result ContextVk::initPipeline(const gl::DrawCallParams &drawCallParams)
{
    ASSERT(!mCurrentPipeline);

    const gl::AttributesMask activeAttribLocationsMask =
        mProgram->getState().getActiveAttribLocationsMask();

    // Ensure the topology of the pipeline description is updated.
    mPipelineDesc->updateTopology(mCurrentDrawMode);

    // Copy over the latest attrib and binding descriptions.
    mVertexArray->getPackedInputDescriptions(mPipelineDesc.get());

    // Ensure that the RenderPass description is updated.
    mPipelineDesc->updateRenderPassDesc(mDrawFramebuffer->getRenderPassDesc());

    // Trigger draw call shader patching and fill out the pipeline desc.
    const vk::ShaderAndSerial *vertexShaderAndSerial   = nullptr;
    const vk::ShaderAndSerial *fragmentShaderAndSerial = nullptr;
    const vk::PipelineLayout *pipelineLayout           = nullptr;
    ANGLE_TRY(mProgram->initShaders(this, drawCallParams, &vertexShaderAndSerial,
                                    &fragmentShaderAndSerial, &pipelineLayout));

    mPipelineDesc->updateShaders(vertexShaderAndSerial->getSerial(),
                                 fragmentShaderAndSerial->getSerial());

    ANGLE_TRY(mRenderer->getPipeline(this, *vertexShaderAndSerial, *fragmentShaderAndSerial,
                                     *pipelineLayout, *mPipelineDesc, activeAttribLocationsMask,
                                     &mCurrentPipeline));

    return angle::Result::Continue();
}

angle::Result ContextVk::setupDraw(const gl::Context *context,
                                   const gl::DrawCallParams &drawCallParams,
                                   DirtyBits dirtyBitMask,
                                   vk::CommandBuffer **commandBufferOut)
{
    // Set any dirty bits that depend on draw call parameters or other objects.
    if (drawCallParams.mode() != mCurrentDrawMode)
    {
        mCurrentPipeline = nullptr;
        mDirtyBits.set(DIRTY_BIT_PIPELINE);
        mCurrentDrawMode = drawCallParams.mode();
    }

    if (!mDrawFramebuffer->appendToStartedRenderPass(mRenderer, commandBufferOut))
    {
        ANGLE_TRY(mDrawFramebuffer->startNewRenderPass(this, commandBufferOut));
        mDirtyBits |= mNewCommandBufferDirtyBits;
    }

    if (context->getStateCache().hasAnyActiveClientAttrib())
    {
        ANGLE_TRY(mVertexArray->updateClientAttribs(context, drawCallParams));
        mDirtyBits.set(DIRTY_BIT_VERTEX_BUFFERS);
    }

    if (mProgram->dirtyUniforms())
    {
        ANGLE_TRY(mProgram->updateUniforms(this));
        mDirtyBits.set(DIRTY_BIT_DESCRIPTOR_SETS);
    }

    DirtyBits dirtyBits = mDirtyBits & dirtyBitMask;

    if (dirtyBits.none())
        return angle::Result::Continue();

    // Flush any relevant dirty bits.
    for (size_t dirtyBit : dirtyBits)
    {
        mDirtyBits.reset(dirtyBit);
        ANGLE_TRY((this->*mDirtyBitHandlers[dirtyBit])(context, drawCallParams, *commandBufferOut));
    }

    return angle::Result::Continue();
}

angle::Result ContextVk::setupIndexedDraw(const gl::Context *context,
                                          const gl::DrawCallParams &drawCallParams,
                                          vk::CommandBuffer **commandBufferOut)
{
    if (drawCallParams.type() != mCurrentDrawElementsType)
    {
        mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
        mCurrentDrawElementsType = drawCallParams.type();
    }

    const gl::Buffer *elementArrayBuffer = mVertexArray->getState().getElementArrayBuffer();
    if (!elementArrayBuffer)
    {
        mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
        ANGLE_TRY(mVertexArray->updateIndexTranslation(this, drawCallParams));
    }
    else
    {
        if (drawCallParams.indices() != mLastIndexBufferOffset)
        {
            mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
            mLastIndexBufferOffset = drawCallParams.indices();
            mVertexArray->updateCurrentElementArrayBufferOffset(mLastIndexBufferOffset);
        }

        if (drawCallParams.type() == GL_UNSIGNED_BYTE && mDirtyBits[DIRTY_BIT_INDEX_BUFFER])
        {
            ANGLE_TRY(mVertexArray->updateIndexTranslation(this, drawCallParams));
        }
    }

    return setupDraw(context, drawCallParams, mIndexedDirtyBitsMask, commandBufferOut);
}

angle::Result ContextVk::setupLineLoopDraw(const gl::Context *context,
                                           const gl::DrawCallParams &drawCallParams,
                                           vk::CommandBuffer **commandBufferOut)
{
    ANGLE_TRY(mVertexArray->handleLineLoop(this, drawCallParams));
    mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
    mCurrentDrawElementsType =
        drawCallParams.isDrawElements() ? drawCallParams.type() : GL_UNSIGNED_INT;
    return setupDraw(context, drawCallParams, mIndexedDirtyBitsMask, commandBufferOut);
}

angle::Result ContextVk::handleDirtyDefaultAttribs(const gl::Context *context,
                                                   const gl::DrawCallParams &drawCallParams,
                                                   vk::CommandBuffer *commandBuffer)
{
    ASSERT(mDirtyDefaultAttribsMask.any());

    for (size_t attribIndex : mDirtyDefaultAttribsMask)
    {
        ANGLE_TRY(updateDefaultAttribute(attribIndex))
    }

    mDirtyDefaultAttribsMask.reset();
    return angle::Result::Continue();
}

angle::Result ContextVk::handleDirtyPipeline(const gl::Context *context,
                                             const gl::DrawCallParams &drawCallParams,
                                             vk::CommandBuffer *commandBuffer)
{
    if (!mCurrentPipeline)
    {
        ANGLE_TRY(initPipeline(drawCallParams));
    }

    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, mCurrentPipeline->get());

    // Update the queue serial for the pipeline object.
    ASSERT(mCurrentPipeline && mCurrentPipeline->valid());
    mCurrentPipeline->updateSerial(mRenderer->getCurrentQueueSerial());
    return angle::Result::Continue();
}

angle::Result ContextVk::handleDirtyTextures(const gl::Context *context,
                                             const gl::DrawCallParams &drawCallParams,
                                             vk::CommandBuffer *commandBuffer)
{
    ANGLE_TRY(updateActiveTextures(context));

    // TODO(jmadill): Should probably merge this for loop with programVk's descriptor update.
    for (size_t textureIndex : mProgram->getState().getActiveSamplersMask())
    {
        // Ensure any writes to the textures are flushed before we read from them.
        TextureVk *textureVk = mActiveTextures[textureIndex];
        ANGLE_TRY(textureVk->ensureImageInitialized(this));
        textureVk->getImage().addReadDependency(mDrawFramebuffer->getFramebuffer());
    }

    if (mProgram->hasTextures())
    {
        ANGLE_TRY(mProgram->updateTexturesDescriptorSet(this));
    }
    return angle::Result::Continue();
}

angle::Result ContextVk::handleDirtyVertexBuffers(const gl::Context *context,
                                                  const gl::DrawCallParams &drawCallParams,
                                                  vk::CommandBuffer *commandBuffer)
{
    BindNonNullVertexBufferRanges(
        commandBuffer, mProgram->getState().getActiveAttribLocationsMask(),
        mProgram->getState().getMaxActiveAttribLocation(),
        mVertexArray->getCurrentArrayBufferHandles(), mVertexArray->getCurrentArrayBufferOffsets());

    const auto &arrayBufferResources = mVertexArray->getCurrentArrayBufferResources();

    for (size_t attribIndex : context->getStateCache().getActiveBufferedAttribsMask())
    {
        if (arrayBufferResources[attribIndex])
            arrayBufferResources[attribIndex]->addReadDependency(
                mDrawFramebuffer->getFramebuffer());
    }
    return angle::Result::Continue();
}

angle::Result ContextVk::handleDirtyIndexBuffer(const gl::Context *context,
                                                const gl::DrawCallParams &drawCallParams,
                                                vk::CommandBuffer *commandBuffer)
{
    commandBuffer->bindIndexBuffer(mVertexArray->getCurrentElementArrayBufferHandle(),
                                   mVertexArray->getCurrentElementArrayBufferOffset(),
                                   gl_vk::GetIndexType(mCurrentDrawElementsType));

    vk::CommandGraphResource *elementArrayBufferResource =
        mVertexArray->getCurrentElementArrayBufferResource();
    if (elementArrayBufferResource)
    {
        elementArrayBufferResource->addReadDependency(mDrawFramebuffer->getFramebuffer());
    }
    return angle::Result::Continue();
}

angle::Result ContextVk::handleDirtyDescriptorSets(const gl::Context *context,
                                                   const gl::DrawCallParams &drawCallParams,
                                                   vk::CommandBuffer *commandBuffer)
{
    ANGLE_TRY(mProgram->updateDescriptorSets(this, drawCallParams, commandBuffer));

    // Bind the graphics descriptor sets.
    commandBuffer->bindDescriptorSets(
        VK_PIPELINE_BIND_POINT_GRAPHICS, mProgram->getPipelineLayout(),
        kDriverUniformsDescriptorSetIndex, 1, &mDriverUniformsDescriptorSet, 0, nullptr);
    return angle::Result::Continue();
}

angle::Result ContextVk::drawArrays(const gl::Context *context,
                                    gl::PrimitiveMode mode,
                                    GLint first,
                                    GLsizei count)
{
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandBuffer *commandBuffer = nullptr;
    uint32_t clampedVertexCount      = drawCallParams.getClampedVertexCount<uint32_t>();

    if (mode == gl::PrimitiveMode::LineLoop)
    {
        ANGLE_TRY(setupLineLoopDraw(context, drawCallParams, &commandBuffer));
        vk::LineLoopHelper::Draw(clampedVertexCount, commandBuffer);
    }
    else
    {
        ANGLE_TRY(setupDraw(context, drawCallParams, mNonIndexedDirtyBitsMask, &commandBuffer));
        commandBuffer->draw(clampedVertexCount, 1, first, 0);
    }

    return angle::Result::Continue();
}

angle::Result ContextVk::drawArraysInstanced(const gl::Context *context,
                                             gl::PrimitiveMode mode,
                                             GLint first,
                                             GLsizei count,
                                             GLsizei instanceCount)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::drawElements(const gl::Context *context,
                                      gl::PrimitiveMode mode,
                                      GLsizei count,
                                      GLenum type,
                                      const void *indices)
{
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandBuffer *commandBuffer = nullptr;
    if (mode == gl::PrimitiveMode::LineLoop)
    {
        ANGLE_TRY(setupLineLoopDraw(context, drawCallParams, &commandBuffer));
        vk::LineLoopHelper::Draw(count, commandBuffer);
    }
    else
    {
        ANGLE_TRY(setupIndexedDraw(context, drawCallParams, &commandBuffer));
        commandBuffer->drawIndexed(count, 1, 0, 0, 0);
    }

    return angle::Result::Continue();
}

angle::Result ContextVk::drawElementsInstanced(const gl::Context *context,
                                               gl::PrimitiveMode mode,
                                               GLsizei count,
                                               GLenum type,
                                               const void *indices,
                                               GLsizei instances)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::drawRangeElements(const gl::Context *context,
                                           gl::PrimitiveMode mode,
                                           GLuint start,
                                           GLuint end,
                                           GLsizei count,
                                           GLenum type,
                                           const void *indices)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

VkDevice ContextVk::getDevice() const
{
    return mRenderer->getDevice();
}

angle::Result ContextVk::drawArraysIndirect(const gl::Context *context,
                                            gl::PrimitiveMode mode,
                                            const void *indirect)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::drawElementsIndirect(const gl::Context *context,
                                              gl::PrimitiveMode mode,
                                              GLenum type,
                                              const void *indirect)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

GLenum ContextVk::getResetStatus()
{
    if (mRenderer->isDeviceLost())
    {
        // TODO(geofflang): It may be possible to track which context caused the device lost and
        // return either GL_GUILTY_CONTEXT_RESET or GL_INNOCENT_CONTEXT_RESET.
        // http://anglebug.com/2787
        return GL_UNKNOWN_CONTEXT_RESET;
    }

    return GL_NO_ERROR;
}

std::string ContextVk::getVendorString() const
{
    UNIMPLEMENTED();
    return std::string();
}

std::string ContextVk::getRendererDescription() const
{
    return mRenderer->getRendererDescription();
}

void ContextVk::insertEventMarker(GLsizei length, const char *marker)
{
    // TODO: Forward this to a Vulkan debug marker.  http://anglebug.com/2853
}

void ContextVk::pushGroupMarker(GLsizei length, const char *marker)
{
    // TODO: Forward this to a Vulkan debug marker.  http://anglebug.com/2853
}

void ContextVk::popGroupMarker()
{
    // TODO: Forward this to a Vulkan debug marker.  http://anglebug.com/2853
}

void ContextVk::pushDebugGroup(GLenum source, GLuint id, GLsizei length, const char *message)
{
    // TODO: Forward this to a Vulkan debug marker.  http://anglebug.com/2853
}

void ContextVk::popDebugGroup()
{
    // TODO: Forward this to a Vulkan debug marker.  http://anglebug.com/2853
}

bool ContextVk::isViewportFlipEnabledForDrawFBO() const
{
    return mFlipViewportForDrawFramebuffer && mFlipYForCurrentSurface;
}

bool ContextVk::isViewportFlipEnabledForReadFBO() const
{
    return mFlipViewportForReadFramebuffer;
}

void ContextVk::updateColorMask(const gl::BlendState &blendState)
{
    mClearColorMask =
        gl_vk::GetColorComponentFlags(blendState.colorMaskRed, blendState.colorMaskGreen,
                                      blendState.colorMaskBlue, blendState.colorMaskAlpha);

    FramebufferVk *framebufferVk = vk::GetImpl(mState.getState().getDrawFramebuffer());
    mPipelineDesc->updateColorWriteMask(mClearColorMask,
                                        framebufferVk->getEmulatedAlphaAttachmentMask());
}

void ContextVk::updateScissor(const gl::State &glState) const
{
    FramebufferVk *framebufferVk = vk::GetImpl(glState.getDrawFramebuffer());
    gl::Box dimensions           = framebufferVk->getState().getDimensions();
    gl::Rectangle renderArea(0, 0, dimensions.width, dimensions.height);

    if (glState.isScissorTestEnabled())
    {
        mPipelineDesc->updateScissor(glState.getScissor(), isViewportFlipEnabledForDrawFBO(),
                                     renderArea);
    }
    else
    {
        // If the scissor test isn't enabled, we can simply use a really big scissor that's
        // certainly larger than the current surface using the maximum size of a 2D texture
        // for the width and height.
        mPipelineDesc->updateScissor(kMaxSizedScissor, isViewportFlipEnabledForDrawFBO(),
                                     renderArea);
    }
}

angle::Result ContextVk::syncState(const gl::Context *context,
                                   const gl::State::DirtyBits &dirtyBits,
                                   const gl::State::DirtyBits &bitMask)
{
    if (dirtyBits.any())
    {
        invalidateCurrentPipeline();
    }

    const gl::State &glState = context->getGLState();

    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::State::DIRTY_BIT_SCISSOR_TEST_ENABLED:
            case gl::State::DIRTY_BIT_SCISSOR:
                updateScissor(glState);
                break;
            case gl::State::DIRTY_BIT_VIEWPORT:
            {
                FramebufferVk *framebufferVk = vk::GetImpl(glState.getDrawFramebuffer());
                mPipelineDesc->updateViewport(framebufferVk, glState.getViewport(),
                                              glState.getNearPlane(), glState.getFarPlane(),
                                              isViewportFlipEnabledForDrawFBO());
                invalidateDriverUniforms();
                break;
            }
            case gl::State::DIRTY_BIT_DEPTH_RANGE:
                mPipelineDesc->updateDepthRange(glState.getNearPlane(), glState.getFarPlane());
                invalidateDriverUniforms();
                break;
            case gl::State::DIRTY_BIT_BLEND_ENABLED:
                mPipelineDesc->updateBlendEnabled(glState.isBlendEnabled());
                break;
            case gl::State::DIRTY_BIT_BLEND_COLOR:
                mPipelineDesc->updateBlendColor(glState.getBlendColor());
                break;
            case gl::State::DIRTY_BIT_BLEND_FUNCS:
                mPipelineDesc->updateBlendFuncs(glState.getBlendState());
                break;
            case gl::State::DIRTY_BIT_BLEND_EQUATIONS:
                mPipelineDesc->updateBlendEquations(glState.getBlendState());
                break;
            case gl::State::DIRTY_BIT_COLOR_MASK:
                updateColorMask(glState.getBlendState());
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE_ENABLED:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK_ENABLED:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK:
                break;
            case gl::State::DIRTY_BIT_DEPTH_TEST_ENABLED:
                mPipelineDesc->updateDepthTestEnabled(glState.getDepthStencilState(),
                                                      glState.getDrawFramebuffer());
                break;
            case gl::State::DIRTY_BIT_DEPTH_FUNC:
                mPipelineDesc->updateDepthFunc(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_DEPTH_MASK:
                mPipelineDesc->updateDepthWriteEnabled(glState.getDepthStencilState(),
                                                       glState.getDrawFramebuffer());
                break;
            case gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED:
                mPipelineDesc->updateStencilTestEnabled(glState.getDepthStencilState(),
                                                        glState.getDrawFramebuffer());
                break;
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_FRONT:
                mPipelineDesc->updateStencilFrontFuncs(glState.getStencilRef(),
                                                       glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_BACK:
                mPipelineDesc->updateStencilBackFuncs(glState.getStencilBackRef(),
                                                      glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_OPS_FRONT:
                mPipelineDesc->updateStencilFrontOps(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_OPS_BACK:
                mPipelineDesc->updateStencilBackOps(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_FRONT:
                mPipelineDesc->updateStencilFrontWriteMask(glState.getDepthStencilState(),
                                                           glState.getDrawFramebuffer());
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK:
                mPipelineDesc->updateStencilBackWriteMask(glState.getDepthStencilState(),
                                                          glState.getDrawFramebuffer());
                break;
            case gl::State::DIRTY_BIT_CULL_FACE_ENABLED:
            case gl::State::DIRTY_BIT_CULL_FACE:
                mPipelineDesc->updateCullMode(glState.getRasterizerState());
                break;
            case gl::State::DIRTY_BIT_FRONT_FACE:
                mPipelineDesc->updateFrontFace(glState.getRasterizerState(),
                                               isViewportFlipEnabledForDrawFBO());
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED:
                mPipelineDesc->updatePolygonOffsetFillEnabled(glState.isPolygonOffsetFillEnabled());
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET:
                mPipelineDesc->updatePolygonOffset(glState.getRasterizerState());
                break;
            case gl::State::DIRTY_BIT_RASTERIZER_DISCARD_ENABLED:
                break;
            case gl::State::DIRTY_BIT_LINE_WIDTH:
                mPipelineDesc->updateLineWidth(glState.getLineWidth());
                break;
            case gl::State::DIRTY_BIT_PRIMITIVE_RESTART_ENABLED:
                break;
            case gl::State::DIRTY_BIT_CLEAR_COLOR:
                mClearColorValue.color.float32[0] = glState.getColorClearValue().red;
                mClearColorValue.color.float32[1] = glState.getColorClearValue().green;
                mClearColorValue.color.float32[2] = glState.getColorClearValue().blue;
                mClearColorValue.color.float32[3] = glState.getColorClearValue().alpha;
                break;
            case gl::State::DIRTY_BIT_CLEAR_DEPTH:
                mClearDepthStencilValue.depthStencil.depth = glState.getDepthClearValue();
                break;
            case gl::State::DIRTY_BIT_CLEAR_STENCIL:
                mClearDepthStencilValue.depthStencil.stencil =
                    static_cast<uint32_t>(glState.getStencilClearValue());
                break;
            case gl::State::DIRTY_BIT_UNPACK_STATE:
                // This is a no-op, its only important to use the right unpack state when we do
                // setImage or setSubImage in TextureVk, which is plumbed through the frontend call
                break;
            case gl::State::DIRTY_BIT_UNPACK_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_PACK_STATE:
                // This is a no-op, its only important to use the right pack state when we do
                // call readPixels later on.
                break;
            case gl::State::DIRTY_BIT_PACK_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_DITHER_ENABLED:
                break;
            case gl::State::DIRTY_BIT_GENERATE_MIPMAP_HINT:
                break;
            case gl::State::DIRTY_BIT_SHADER_DERIVATIVE_HINT:
                break;
            case gl::State::DIRTY_BIT_READ_FRAMEBUFFER_BINDING:
                updateFlipViewportReadFramebuffer(context->getGLState());
                break;
            case gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING:
            {
                mDrawFramebuffer = vk::GetImpl(glState.getDrawFramebuffer());
                updateFlipViewportDrawFramebuffer(glState);
                mPipelineDesc->updateViewport(mDrawFramebuffer, glState.getViewport(),
                                              glState.getNearPlane(), glState.getFarPlane(),
                                              isViewportFlipEnabledForDrawFBO());
                updateColorMask(glState.getBlendState());
                mPipelineDesc->updateCullMode(glState.getRasterizerState());
                updateScissor(glState);
                mPipelineDesc->updateDepthTestEnabled(glState.getDepthStencilState(),
                                                      glState.getDrawFramebuffer());
                mPipelineDesc->updateDepthWriteEnabled(glState.getDepthStencilState(),
                                                       glState.getDrawFramebuffer());
                mPipelineDesc->updateStencilTestEnabled(glState.getDepthStencilState(),
                                                        glState.getDrawFramebuffer());
                mPipelineDesc->updateStencilFrontWriteMask(glState.getDepthStencilState(),
                                                           glState.getDrawFramebuffer());
                mPipelineDesc->updateStencilBackWriteMask(glState.getDepthStencilState(),
                                                          glState.getDrawFramebuffer());
                invalidateDriverUniforms();
                break;
            }
            case gl::State::DIRTY_BIT_RENDERBUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_VERTEX_ARRAY_BINDING:
            {
                mVertexArray = vk::GetImpl(glState.getVertexArray());
                invalidateDefaultAttributes(context->getStateCache().getActiveDefaultAttribsMask());
                break;
            }
            case gl::State::DIRTY_BIT_DRAW_INDIRECT_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_DISPATCH_INDIRECT_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_PROGRAM_BINDING:
                mProgram = vk::GetImpl(glState.getProgram());
                break;
            case gl::State::DIRTY_BIT_PROGRAM_EXECUTABLE:
            {
                invalidateCurrentTextures();
                // No additional work is needed here. We will update the pipeline desc later.
                invalidateDefaultAttributes(context->getStateCache().getActiveDefaultAttribsMask());
                bool useVertexBuffer = (mProgram->getState().getMaxActiveAttribLocation());
                mNonIndexedDirtyBitsMask.set(DIRTY_BIT_VERTEX_BUFFERS, useVertexBuffer);
                mIndexedDirtyBitsMask.set(DIRTY_BIT_VERTEX_BUFFERS, useVertexBuffer);
                break;
            }
            case gl::State::DIRTY_BIT_TEXTURE_BINDINGS:
                invalidateCurrentTextures();
                break;
            case gl::State::DIRTY_BIT_SAMPLER_BINDINGS:
                invalidateCurrentTextures();
                break;
            case gl::State::DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING:
                break;
            case gl::State::DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_UNIFORM_BUFFER_BINDINGS:
                break;
            case gl::State::DIRTY_BIT_ATOMIC_COUNTER_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_IMAGE_BINDINGS:
                break;
            case gl::State::DIRTY_BIT_MULTISAMPLING:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_ONE:
                break;
            case gl::State::DIRTY_BIT_COVERAGE_MODULATION:
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING:
                break;
            case gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB:
                break;
            case gl::State::DIRTY_BIT_CURRENT_VALUES:
            {
                invalidateDefaultAttributes(glState.getAndResetDirtyCurrentValues());
                break;
            }
            break;
            default:
                UNREACHABLE();
                break;
        }
    }

    return angle::Result::Continue();
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

angle::Result ContextVk::onMakeCurrent(const gl::Context *context)
{
    // Flip viewports if FeaturesVk::flipViewportY is enabled and the user did not request that the
    // surface is flipped.
    egl::Surface *drawSurface = context->getCurrentDrawSurface();
    mFlipYForCurrentSurface =
        drawSurface != nullptr && mRenderer->getFeatures().flipViewportY &&
        !IsMaskFlagSet(drawSurface->getOrientation(), EGL_SURFACE_ORIENTATION_INVERT_Y_ANGLE);

    const gl::State &glState = context->getGLState();
    updateFlipViewportDrawFramebuffer(glState);
    updateFlipViewportReadFramebuffer(glState);
    invalidateDriverUniforms();
    return angle::Result::Continue();
}

void ContextVk::updateFlipViewportDrawFramebuffer(const gl::State &glState)
{
    gl::Framebuffer *drawFramebuffer = glState.getDrawFramebuffer();
    mFlipViewportForDrawFramebuffer =
        drawFramebuffer->isDefault() && mRenderer->getFeatures().flipViewportY;
}

void ContextVk::updateFlipViewportReadFramebuffer(const gl::State &glState)
{
    gl::Framebuffer *readFramebuffer = glState.getReadFramebuffer();
    mFlipViewportForReadFramebuffer =
        readFramebuffer->isDefault() && mRenderer->getFeatures().flipViewportY;
}

gl::Caps ContextVk::getNativeCaps() const
{
    return mRenderer->getNativeCaps();
}

const gl::TextureCapsMap &ContextVk::getNativeTextureCaps() const
{
    return mRenderer->getNativeTextureCaps();
}

const gl::Extensions &ContextVk::getNativeExtensions() const
{
    return mRenderer->getNativeExtensions();
}

const gl::Limitations &ContextVk::getNativeLimitations() const
{
    return mRenderer->getNativeLimitations();
}

CompilerImpl *ContextVk::createCompiler()
{
    return new CompilerVk();
}

ShaderImpl *ContextVk::createShader(const gl::ShaderState &state)
{
    return new ShaderVk(state);
}

ProgramImpl *ContextVk::createProgram(const gl::ProgramState &state)
{
    return new ProgramVk(state);
}

FramebufferImpl *ContextVk::createFramebuffer(const gl::FramebufferState &state)
{
    return FramebufferVk::CreateUserFBO(mRenderer, state);
}

TextureImpl *ContextVk::createTexture(const gl::TextureState &state)
{
    return new TextureVk(state, mRenderer);
}

RenderbufferImpl *ContextVk::createRenderbuffer(const gl::RenderbufferState &state)
{
    return new RenderbufferVk(state);
}

BufferImpl *ContextVk::createBuffer(const gl::BufferState &state)
{
    return new BufferVk(state);
}

VertexArrayImpl *ContextVk::createVertexArray(const gl::VertexArrayState &state)
{
    return new VertexArrayVk(state, mRenderer);
}

QueryImpl *ContextVk::createQuery(gl::QueryType type)
{
    return new QueryVk(type);
}

FenceNVImpl *ContextVk::createFenceNV()
{
    return new FenceNVVk();
}

SyncImpl *ContextVk::createSync()
{
    return new SyncVk();
}

TransformFeedbackImpl *ContextVk::createTransformFeedback(const gl::TransformFeedbackState &state)
{
    return new TransformFeedbackVk(state);
}

SamplerImpl *ContextVk::createSampler(const gl::SamplerState &state)
{
    return new SamplerVk(state);
}

ProgramPipelineImpl *ContextVk::createProgramPipeline(const gl::ProgramPipelineState &state)
{
    return new ProgramPipelineVk(state);
}

std::vector<PathImpl *> ContextVk::createPaths(GLsizei)
{
    return std::vector<PathImpl *>();
}

void ContextVk::invalidateCurrentPipeline()
{
    mDirtyBits.set(DIRTY_BIT_PIPELINE);
    mDirtyBits.set(DIRTY_BIT_VERTEX_BUFFERS);
    mDirtyBits.set(DIRTY_BIT_INDEX_BUFFER);
    mCurrentPipeline = nullptr;
}

void ContextVk::invalidateCurrentTextures()
{
    ASSERT(mProgram);
    if (mProgram->hasTextures())
    {
        mDirtyBits.set(DIRTY_BIT_TEXTURES);
        mDirtyBits.set(DIRTY_BIT_DESCRIPTOR_SETS);
    }
}

void ContextVk::invalidateDriverUniforms()
{
    mDirtyBits.set(DIRTY_BIT_DRIVER_UNIFORMS);
    mDirtyBits.set(DIRTY_BIT_DESCRIPTOR_SETS);
}

angle::Result ContextVk::dispatchCompute(const gl::Context *context,
                                         GLuint numGroupsX,
                                         GLuint numGroupsY,
                                         GLuint numGroupsZ)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::dispatchComputeIndirect(const gl::Context *context, GLintptr indirect)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::memoryBarrier(const gl::Context *context, GLbitfield barriers)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

angle::Result ContextVk::memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers)
{
    ANGLE_VK_UNREACHABLE(this);
    return angle::Result::Stop();
}

vk::DynamicDescriptorPool *ContextVk::getDynamicDescriptorPool(uint32_t descriptorSetIndex)
{
    return &mDynamicDescriptorPools[descriptorSetIndex];
}

vk::DynamicQueryPool *ContextVk::getQueryPool(gl::QueryType queryType)
{
    ASSERT(queryType == gl::QueryType::AnySamples ||
           queryType == gl::QueryType::AnySamplesConservative ||
           queryType == gl::QueryType::Timestamp || queryType == gl::QueryType::TimeElapsed);
    ASSERT(mQueryPools[queryType].isValid());
    return &mQueryPools[queryType];
}

const VkClearValue &ContextVk::getClearColorValue() const
{
    return mClearColorValue;
}

const VkClearValue &ContextVk::getClearDepthStencilValue() const
{
    return mClearDepthStencilValue;
}

VkColorComponentFlags ContextVk::getClearColorMask() const
{
    return mClearColorMask;
}

const FeaturesVk &ContextVk::getFeatures() const
{
    return mRenderer->getFeatures();
}

angle::Result ContextVk::handleDirtyDriverUniforms(const gl::Context *context,
                                                   const gl::DrawCallParams &drawCallParams,
                                                   vk::CommandBuffer *commandBuffer)
{
    // Release any previously retained buffers.
    mDriverUniformsBuffer.releaseRetainedBuffers(mRenderer);

    const gl::Rectangle &glViewport = mState.getState().getViewport();
    float halfRenderAreaHeight =
        static_cast<float>(mDrawFramebuffer->getState().getDimensions().height) * 0.5f;

    // Allocate a new region in the dynamic buffer.
    uint8_t *ptr            = nullptr;
    VkBuffer buffer         = VK_NULL_HANDLE;
    VkDeviceSize offset     = 0;
    bool newBufferAllocated = false;
    ANGLE_TRY(mDriverUniformsBuffer.allocate(this, sizeof(DriverUniforms), &ptr, &buffer, &offset,
                                             &newBufferAllocated));
    float scaleY = isViewportFlipEnabledForDrawFBO() ? -1.0f : 1.0f;

    float depthRangeNear = mState.getState().getNearPlane();
    float depthRangeFar  = mState.getState().getFarPlane();
    float depthRangeDiff = depthRangeFar - depthRangeNear;

    // Copy and flush to the device.
    DriverUniforms *driverUniforms = reinterpret_cast<DriverUniforms *>(ptr);
    *driverUniforms                = {
        {static_cast<float>(glViewport.x), static_cast<float>(glViewport.y),
         static_cast<float>(glViewport.width), static_cast<float>(glViewport.height)},
        halfRenderAreaHeight,
        scaleY,
        -scaleY,
        0.0f,
        {depthRangeNear, depthRangeFar, depthRangeDiff, 0.0f}};

    ANGLE_TRY(mDriverUniformsBuffer.flush(this));

    // Get the descriptor set layout.
    if (!mDriverUniformsSetLayout.valid())
    {
        vk::DescriptorSetLayoutDesc desc;
        desc.update(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1);

        ANGLE_TRY(mRenderer->getDescriptorSetLayout(this, desc, &mDriverUniformsSetLayout));
    }

    // Allocate a new descriptor set.
    ANGLE_TRY(mDynamicDescriptorPools[kDriverUniformsDescriptorSetIndex].allocateSets(
        this, mDriverUniformsSetLayout.get().ptr(), 1, &mDriverUniformsDescriptorPoolBinding,
        &mDriverUniformsDescriptorSet));

    // Update the driver uniform descriptor set.
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range  = sizeof(DriverUniforms);

    VkWriteDescriptorSet writeInfo = {};
    writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.dstSet           = mDriverUniformsDescriptorSet;
    writeInfo.dstBinding       = 0;
    writeInfo.dstArrayElement  = 0;
    writeInfo.descriptorCount  = 1;
    writeInfo.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writeInfo.pImageInfo       = nullptr;
    writeInfo.pTexelBufferView = nullptr;
    writeInfo.pBufferInfo      = &bufferInfo;

    vkUpdateDescriptorSets(getDevice(), 1, &writeInfo, 0, nullptr);

    return angle::Result::Continue();
}

void ContextVk::handleError(VkResult errorCode, const char *file, unsigned int line)
{
    GLenum glErrorCode = DefaultGLErrorCode(errorCode);

    std::stringstream errorStream;
    errorStream << "Internal Vulkan error: " << VulkanResultString(errorCode) << ", in " << file
                << ", line " << line << ".";

    if (errorCode == VK_ERROR_DEVICE_LOST)
    {
        mRenderer->markDeviceLost();
    }

    mErrors->handleError(gl::Error(glErrorCode, glErrorCode, errorStream.str()));
}

angle::Result ContextVk::updateActiveTextures(const gl::Context *context)
{
    const gl::State &glState   = mState.getState();
    const gl::Program *program = glState.getProgram();

    mActiveTextures.fill(nullptr);

    const gl::ActiveTexturePointerArray &textures  = glState.getActiveTexturesCache();
    const gl::ActiveTextureMask &activeTextures    = program->getActiveSamplersMask();
    const gl::ActiveTextureTypeArray &textureTypes = program->getActiveSamplerTypes();

    for (size_t textureUnit : activeTextures)
    {
        gl::Texture *texture        = textures[textureUnit];
        gl::TextureType textureType = textureTypes[textureUnit];

        // Null textures represent incomplete textures.
        if (texture == nullptr)
        {
            ANGLE_TRY_HANDLE(context, getIncompleteTexture(context, textureType, &texture));
        }

        mActiveTextures[textureUnit] = vk::GetImpl(texture);
    }

    return angle::Result::Continue();
}

const gl::ActiveTextureArray<TextureVk *> &ContextVk::getActiveTextures() const
{
    return mActiveTextures;
}

void ContextVk::invalidateDefaultAttribute(size_t attribIndex)
{
    mDirtyDefaultAttribsMask.set(attribIndex);
    mDirtyBits.set(DIRTY_BIT_DEFAULT_ATTRIBS);
}

void ContextVk::invalidateDefaultAttributes(const gl::AttributesMask &dirtyMask)
{
    if (dirtyMask.any())
    {
        mDirtyDefaultAttribsMask = dirtyMask;
        mDirtyBits.set(DIRTY_BIT_DEFAULT_ATTRIBS);
    }
}

angle::Result ContextVk::updateDefaultAttribute(size_t attribIndex)
{
    vk::DynamicBuffer &defaultBuffer = mDefaultAttribBuffers[attribIndex];

    defaultBuffer.releaseRetainedBuffers(mRenderer);

    uint8_t *ptr;
    VkBuffer bufferHandle = VK_NULL_HANDLE;
    VkDeviceSize offset   = 0;
    ANGLE_TRY(
        defaultBuffer.allocate(this, kDefaultValueSize, &ptr, &bufferHandle, &offset, nullptr));

    const gl::State &glState = mState.getState();
    const gl::VertexAttribCurrentValueData &defaultValue =
        glState.getVertexAttribCurrentValues()[attribIndex];

    ASSERT(defaultValue.Type == GL_FLOAT);

    memcpy(ptr, defaultValue.FloatValues, kDefaultValueSize);

    ANGLE_TRY(defaultBuffer.flush(this));

    mVertexArray->updateDefaultAttrib(mRenderer, attribIndex, bufferHandle,
                                      static_cast<uint32_t>(offset));
    return angle::Result::Continue();
}
}  // namespace rx

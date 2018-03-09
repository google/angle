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
#include "libANGLE/renderer/vulkan/BufferVk.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/CompilerVk.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DeviceVk.h"
#include "libANGLE/renderer/vulkan/FenceNVVk.h"
#include "libANGLE/renderer/vulkan/FramebufferVk.h"
#include "libANGLE/renderer/vulkan/ImageVk.h"
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
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{

namespace
{

VkIndexType GetVkIndexType(GLenum glIndexType)
{
    switch (glIndexType)
    {
        case GL_UNSIGNED_BYTE:
        case GL_UNSIGNED_SHORT:
            return VK_INDEX_TYPE_UINT16;
        case GL_UNSIGNED_INT:
            return VK_INDEX_TYPE_UINT32;
        default:
            UNREACHABLE();
            return VK_INDEX_TYPE_MAX_ENUM;
    }
}

enum DescriptorPoolIndex : uint8_t
{
    UniformBufferPool = 0,
    TexturePool       = 1,
};

constexpr size_t kStreamingVertexDataSize = 1024 * 1024;
constexpr size_t kStreamingIndexDataSize  = 1024 * 8;

}  // anonymous namespace

ContextVk::ContextVk(const gl::ContextState &state, RendererVk *renderer)
    : ContextImpl(state),
      mRenderer(renderer),
      mCurrentDrawMode(GL_NONE),
      mVertexArrayDirty(false),
      mTexturesDirty(false),
      mStreamingVertexData(VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, kStreamingVertexDataSize),
      mStreamingIndexData(VK_BUFFER_USAGE_INDEX_BUFFER_BIT, kStreamingIndexDataSize)
{
    memset(&mClearColorValue, 0, sizeof(mClearColorValue));
    memset(&mClearDepthStencilValue, 0, sizeof(mClearDepthStencilValue));
}

ContextVk::~ContextVk()
{
}

void ContextVk::onDestroy(const gl::Context *context)
{
    VkDevice device = mRenderer->getDevice();

    mDescriptorPool.destroy(device);
    mStreamingVertexData.destroy(device);
    mStreamingIndexData.destroy(device);
    mLineLoopHandler.destroy(device);
}

gl::Error ContextVk::initialize()
{
    VkDevice device = mRenderer->getDevice();

    VkDescriptorPoolSize poolSizes[2];
    poolSizes[UniformBufferPool].type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[UniformBufferPool].descriptorCount = 1024;
    poolSizes[TexturePool].type                  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[TexturePool].descriptorCount       = 1024;

    VkDescriptorPoolCreateInfo descriptorPoolInfo;
    descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptorPoolInfo.pNext = nullptr;
    descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    // TODO(jmadill): Pick non-arbitrary max.
    descriptorPoolInfo.maxSets = 2048;

    // Reserve pools for uniform blocks and textures.
    descriptorPoolInfo.poolSizeCount = 2;
    descriptorPoolInfo.pPoolSizes    = poolSizes;

    ANGLE_TRY(mDescriptorPool.init(device, descriptorPoolInfo));

    mPipelineDesc.reset(new vk::PipelineDesc());
    mPipelineDesc->initDefaults();

    return gl::NoError();
}

gl::Error ContextVk::flush(const gl::Context *context)
{
    // TODO(jmadill): Flush will need to insert a semaphore for the next flush to wait on.
    UNIMPLEMENTED();

    // dEQP tests rely on having no errors thrown at the end of the test and they always call
    // flush at the end of the their tests. Just returning NoError until we implement flush
    // allow us to work on enabling many tests in the meantime.
    return gl::NoError();
}

gl::Error ContextVk::finish(const gl::Context *context)
{
    return mRenderer->finish(context);
}

gl::Error ContextVk::initPipeline(const gl::Context *context)
{
    ASSERT(!mCurrentPipeline);

    const gl::State &state       = mState.getState();
    VertexArrayVk *vertexArrayVk = vk::GetImpl(state.getVertexArray());
    FramebufferVk *framebufferVk = vk::GetImpl(state.getDrawFramebuffer());
    ProgramVk *programVk         = vk::GetImpl(state.getProgram());
    const gl::AttributesMask activeAttribLocationsMask =
        state.getProgram()->getActiveAttribLocationsMask();

    // Ensure the topology of the pipeline description is updated.
    mPipelineDesc->updateTopology(mCurrentDrawMode);

    // Copy over the latest attrib and binding descriptions.
    vertexArrayVk->getPackedInputDescriptions(mPipelineDesc.get());

    // Ensure that the RenderPass description is updated.
    mPipelineDesc->updateRenderPassDesc(framebufferVk->getRenderPassDesc(context));

    // TODO(jmadill): Validate with ASSERT against physical device limits/caps?
    ANGLE_TRY(mRenderer->getPipeline(programVk, *mPipelineDesc, activeAttribLocationsMask,
                                     &mCurrentPipeline));

    return gl::NoError();
}

gl::Error ContextVk::setupDraw(const gl::Context *context,
                               GLenum mode,
                               DrawType drawType,
                               size_t firstVertex,
                               size_t lastVertex,
                               ResourceVk *elementArrayBufferOverride,
                               vk::CommandBuffer **commandBuffer)
{
    if (mode != mCurrentDrawMode)
    {
        invalidateCurrentPipeline();
        mCurrentDrawMode = mode;
    }

    if (!mCurrentPipeline)
    {
        ANGLE_TRY(initPipeline(context));
    }

    const auto &state            = mState.getState();
    const gl::Program *programGL = state.getProgram();
    ProgramVk *programVk         = vk::GetImpl(programGL);
    const gl::VertexArray *vao   = state.getVertexArray();
    VertexArrayVk *vkVAO         = vk::GetImpl(vao);
    const auto *drawFBO          = state.getDrawFramebuffer();
    FramebufferVk *vkFBO         = vk::GetImpl(drawFBO);
    Serial queueSerial           = mRenderer->getCurrentQueueSerial();
    uint32_t maxAttrib           = programGL->getState().getMaxActiveAttribLocation();

    vk::CommandGraphNode *graphNode = nullptr;
    ANGLE_TRY(vkFBO->getCommandGraphNodeForDraw(context, &graphNode));

    if (!graphNode->getInsideRenderPassCommands()->valid())
    {
        mVertexArrayDirty = true;
        mTexturesDirty    = true;
        ANGLE_TRY(graphNode->beginInsideRenderPassRecording(mRenderer, commandBuffer));
    }
    else
    {
        *commandBuffer = graphNode->getInsideRenderPassCommands();
    }

    // Ensure any writes to the VAO buffers are flushed before we read from them.
    if (mVertexArrayDirty || elementArrayBufferOverride != nullptr)
    {

        mVertexArrayDirty = false;
        vkVAO->updateDrawDependencies(graphNode, programGL->getActiveAttribLocationsMask(),
                                      elementArrayBufferOverride, queueSerial, drawType);
    }

    // Ensure any writes to the textures are flushed before we read from them.
    if (mTexturesDirty)
    {
        mTexturesDirty = false;
        // TODO(jmadill): Should probably merge this for loop with programVk's descriptor update.
        const auto &completeTextures = state.getCompleteTextureCache();
        for (const gl::SamplerBinding &samplerBinding : programGL->getSamplerBindings())
        {
            ASSERT(!samplerBinding.unreferenced);

            // TODO(jmadill): Sampler arrays
            ASSERT(samplerBinding.boundTextureUnits.size() == 1);

            GLuint textureUnit         = samplerBinding.boundTextureUnits[0];
            const gl::Texture *texture = completeTextures[textureUnit];

            // TODO(jmadill): Incomplete textures handling.
            ASSERT(texture);

            TextureVk *textureVk = vk::GetImpl(texture);
            textureVk->onReadResource(graphNode, mRenderer->getCurrentQueueSerial());
        }
    }

    (*commandBuffer)->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, mCurrentPipeline->get());
    ContextVk *contextVk = vk::GetImpl(context);
    ANGLE_TRY(vkVAO->streamVertexData(contextVk, &mStreamingVertexData, firstVertex, lastVertex));
    (*commandBuffer)
        ->bindVertexBuffers(0, maxAttrib, vkVAO->getCurrentArrayBufferHandles().data(),
                            vkVAO->getCurrentArrayBufferOffsets().data());

    // Update the queue serial for the pipeline object.
    ASSERT(mCurrentPipeline && mCurrentPipeline->valid());
    mCurrentPipeline->updateSerial(queueSerial);

    // TODO(jmadill): Can probably use more dirty bits here.
    ANGLE_TRY(programVk->updateUniforms(this));
    programVk->updateTexturesDescriptorSet(this);

    // Bind the graphics descriptor sets.
    // TODO(jmadill): Handle multiple command buffers.
    const auto &descriptorSets = programVk->getDescriptorSets();
    const gl::RangeUI &usedRange = programVk->getUsedDescriptorSetRange();
    if (!usedRange.empty())
    {
        ASSERT(!descriptorSets.empty());
        const vk::PipelineLayout &pipelineLayout = mRenderer->getGraphicsPipelineLayout();
        (*commandBuffer)
            ->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, usedRange.low(),
                                 usedRange.length(), &descriptorSets[usedRange.low()], 0, nullptr);
    }

    return gl::NoError();
}

gl::Error ContextVk::drawArrays(const gl::Context *context, GLenum mode, GLint first, GLsizei count)
{
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(setupDraw(context, mode, DrawType::Arrays, first, first + count - 1, nullptr,
                        &commandBuffer));

    if (mode == GL_LINE_LOOP)
    {
        ANGLE_TRY(mLineLoopHandler.createIndexBuffer(this, first, count));
        mLineLoopHandler.bindIndexBuffer(VK_INDEX_TYPE_UINT32, &commandBuffer);
        ANGLE_TRY(mLineLoopHandler.draw(count, commandBuffer));
    }
    else
    {
        commandBuffer->draw(count, 1, first, 0);
    }

    return gl::NoError();
}

gl::Error ContextVk::drawArraysInstanced(const gl::Context *context,
                                         GLenum mode,
                                         GLint first,
                                         GLsizei count,
                                         GLsizei instanceCount)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::drawElements(const gl::Context *context,
                                  GLenum mode,
                                  GLsizei count,
                                  GLenum type,
                                  const void *indices)
{
    gl::VertexArray *vao                 = mState.getState().getVertexArray();
    const gl::Buffer *elementArrayBuffer = vao->getElementArrayBuffer().get();
    vk::CommandBuffer *commandBuffer = nullptr;

    if (mode == GL_LINE_LOOP)
    {
        if (!elementArrayBuffer)
        {
            UNIMPLEMENTED();
            return gl::InternalError() << "Line loop indices in client memory not supported";
        }

        BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);

        ANGLE_TRY(mLineLoopHandler.createIndexBufferFromElementArrayBuffer(
            this, elementArrayBufferVk, GetVkIndexType(type), count));

        // TODO(fjhenigman): calculate the index range and pass to setupDraw()
        ANGLE_TRY(setupDraw(context, mode, DrawType::Elements, 0, 0,
                            mLineLoopHandler.getLineLoopBufferResource(), &commandBuffer));

        mLineLoopHandler.bindIndexBuffer(GetVkIndexType(type), &commandBuffer);
        commandBuffer->drawIndexed(count + 1, 1, 0, 0, 0);
    }
    else
    {
        ContextVk *contextVk         = vk::GetImpl(context);
        const bool computeIndexRange = vk::GetImpl(vao)->attribsToStream(contextVk).any();
        gl::IndexRange range;
        VkBuffer buffer     = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;

        if (elementArrayBuffer)
        {
            if (type == GL_UNSIGNED_BYTE)
            {
                // TODO(fjhenigman): Index format translation.
                UNIMPLEMENTED();
                return gl::InternalError() << "Unsigned byte translation is not implemented for "
                                           << "indices in a buffer object";
            }

            BufferVk *elementArrayBufferVk = vk::GetImpl(elementArrayBuffer);
            buffer                         = elementArrayBufferVk->getVkBuffer().getHandle();
            offset                         = 0;

            if (computeIndexRange)
            {
                ANGLE_TRY(elementArrayBufferVk->getIndexRange(
                    context, type, 0, count, false /*primitiveRestartEnabled*/, &range));
            }
        }
        else
        {
            const GLsizei amount = sizeof(GLushort) * count;
            GLubyte *dst         = nullptr;

            ANGLE_TRY(mStreamingIndexData.allocate(contextVk, amount, &dst, &buffer, &offset));
            if (type == GL_UNSIGNED_BYTE)
            {
                // Unsigned bytes don't have direct support in Vulkan so we have to expand the
                // memory to a GLushort.
                const GLubyte *in     = static_cast<const GLubyte *>(indices);
                GLushort *expandedDst = reinterpret_cast<GLushort *>(dst);
                for (GLsizei index = 0; index < count; index++)
                {
                    expandedDst[index] = static_cast<GLushort>(in[index]);
                }
            }
            else
            {
                memcpy(dst, indices, amount);
            }
            ANGLE_TRY(mStreamingIndexData.flush(contextVk));

            if (computeIndexRange)
            {
                range =
                    gl::ComputeIndexRange(type, indices, count, false /*primitiveRestartEnabled*/);
            }
        }

        ANGLE_TRY(setupDraw(context, mode, DrawType::Elements, range.start, range.end, nullptr,
                            &commandBuffer));
        commandBuffer->bindIndexBuffer(buffer, offset, GetVkIndexType(type));
        commandBuffer->drawIndexed(count, 1, 0, 0, 0);
    }

    return gl::NoError();
}

gl::Error ContextVk::drawElementsInstanced(const gl::Context *context,
                                           GLenum mode,
                                           GLsizei count,
                                           GLenum type,
                                           const void *indices,
                                           GLsizei instances)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::drawRangeElements(const gl::Context *context,
                                       GLenum mode,
                                       GLuint start,
                                       GLuint end,
                                       GLsizei count,
                                       GLenum type,
                                       const void *indices)
{
    return gl::NoError();
}

VkDevice ContextVk::getDevice() const
{
    return mRenderer->getDevice();
}

gl::Error ContextVk::drawArraysIndirect(const gl::Context *context,
                                        GLenum mode,
                                        const void *indirect)
{
    UNIMPLEMENTED();
    return gl::InternalError() << "DrawArraysIndirect hasn't been implemented for vulkan backend.";
}

gl::Error ContextVk::drawElementsIndirect(const gl::Context *context,
                                          GLenum mode,
                                          GLenum type,
                                          const void *indirect)
{
    UNIMPLEMENTED();
    return gl::InternalError()
           << "DrawElementsIndirect hasn't been implemented for vulkan backend.";
}

GLenum ContextVk::getResetStatus()
{
    UNIMPLEMENTED();
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

void ContextVk::pushDebugGroup(GLenum source, GLuint id, GLsizei length, const char *message)
{
    UNIMPLEMENTED();
}

void ContextVk::popDebugGroup()
{
    UNIMPLEMENTED();
}

void ContextVk::syncState(const gl::Context *context, const gl::State::DirtyBits &dirtyBits)
{
    if (dirtyBits.any())
    {
        invalidateCurrentPipeline();
    }

    const auto &glState = context->getGLState();

    // TODO(jmadill): Full dirty bits implementation.
    bool dirtyTextures = false;

    for (auto dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::State::DIRTY_BIT_SCISSOR_TEST_ENABLED:
                if (glState.isScissorTestEnabled())
                {
                    mPipelineDesc->updateScissor(glState.getScissor());
                }
                else
                {
                    mPipelineDesc->updateScissor(glState.getViewport());
                }
                break;
            case gl::State::DIRTY_BIT_SCISSOR:
                // Only modify the scissor region if the test is enabled, otherwise we want to keep
                // the viewport size as the scissor region.
                if (glState.isScissorTestEnabled())
                {
                    mPipelineDesc->updateScissor(glState.getScissor());
                }
                break;
            case gl::State::DIRTY_BIT_VIEWPORT:
                mPipelineDesc->updateViewport(glState.getViewport(), glState.getNearPlane(),
                                              glState.getFarPlane());

                // If the scissor test isn't enabled, we have to also update the scissor to
                // be equal to the viewport to make sure we keep rendering everything in the
                // viewport.
                if (!glState.isScissorTestEnabled())
                {
                    mPipelineDesc->updateScissor(glState.getViewport());
                }
                break;
            case gl::State::DIRTY_BIT_DEPTH_RANGE:
                WARN() << "DIRTY_BIT_DEPTH_RANGE unimplemented";
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
                WARN() << "DIRTY_BIT_COLOR_MASK unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED:
                WARN() << "DIRTY_BIT_SAMPLE_ALPHA_TO_COVERAGE_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE_ENABLED:
                WARN() << "DIRTY_BIT_SAMPLE_COVERAGE_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_COVERAGE:
                WARN() << "DIRTY_BIT_SAMPLE_COVERAGE unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK_ENABLED:
                WARN() << "DIRTY_BIT_SAMPLE_MASK_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_MASK:
                WARN() << "DIRTY_BIT_SAMPLE_MASK unimplemented";
                break;
            case gl::State::DIRTY_BIT_DEPTH_TEST_ENABLED:
                WARN() << "DIRTY_BIT_DEPTH_TEST_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_DEPTH_FUNC:
                WARN() << "DIRTY_BIT_DEPTH_FUNC unimplemented";
                break;
            case gl::State::DIRTY_BIT_DEPTH_MASK:
                WARN() << "DIRTY_BIT_DEPTH_MASK unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED:
                WARN() << "DIRTY_BIT_STENCIL_TEST_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_FRONT:
                WARN() << "DIRTY_BIT_STENCIL_FUNCS_FRONT unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_FUNCS_BACK:
                WARN() << "DIRTY_BIT_STENCIL_FUNCS_BACK unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_OPS_FRONT:
                WARN() << "DIRTY_BIT_STENCIL_OPS_FRONT unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_OPS_BACK:
                WARN() << "DIRTY_BIT_STENCIL_OPS_BACK unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_FRONT:
                WARN() << "DIRTY_BIT_STENCIL_WRITEMASK_FRONT unimplemented";
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK:
                WARN() << "DIRTY_BIT_STENCIL_WRITEMASK_BACK unimplemented";
                break;
            case gl::State::DIRTY_BIT_CULL_FACE_ENABLED:
            case gl::State::DIRTY_BIT_CULL_FACE:
                mPipelineDesc->updateCullMode(glState.getRasterizerState());
                break;
            case gl::State::DIRTY_BIT_FRONT_FACE:
                mPipelineDesc->updateFrontFace(glState.getRasterizerState());
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED:
                WARN() << "DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET:
                WARN() << "DIRTY_BIT_POLYGON_OFFSET unimplemented";
                break;
            case gl::State::DIRTY_BIT_RASTERIZER_DISCARD_ENABLED:
                WARN() << "DIRTY_BIT_RASTERIZER_DISCARD_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_LINE_WIDTH:
                mPipelineDesc->updateLineWidth(glState.getLineWidth());
                break;
            case gl::State::DIRTY_BIT_PRIMITIVE_RESTART_ENABLED:
                WARN() << "DIRTY_BIT_PRIMITIVE_RESTART_ENABLED unimplemented";
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
                WARN() << "DIRTY_BIT_UNPACK_STATE unimplemented";
                break;
            case gl::State::DIRTY_BIT_UNPACK_BUFFER_BINDING:
                WARN() << "DIRTY_BIT_UNPACK_BUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_PACK_STATE:
                WARN() << "DIRTY_BIT_PACK_STATE unimplemented";
                break;
            case gl::State::DIRTY_BIT_PACK_BUFFER_BINDING:
                WARN() << "DIRTY_BIT_PACK_BUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_DITHER_ENABLED:
                WARN() << "DIRTY_BIT_DITHER_ENABLED unimplemented";
                break;
            case gl::State::DIRTY_BIT_GENERATE_MIPMAP_HINT:
                WARN() << "DIRTY_BIT_GENERATE_MIPMAP_HINT unimplemented";
                break;
            case gl::State::DIRTY_BIT_SHADER_DERIVATIVE_HINT:
                WARN() << "DIRTY_BIT_SHADER_DERIVATIVE_HINT unimplemented";
                break;
            case gl::State::DIRTY_BIT_READ_FRAMEBUFFER_BINDING:
                WARN() << "DIRTY_BIT_READ_FRAMEBUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING:
                WARN() << "DIRTY_BIT_DRAW_FRAMEBUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_RENDERBUFFER_BINDING:
                WARN() << "DIRTY_BIT_RENDERBUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_VERTEX_ARRAY_BINDING:
                mVertexArrayDirty = true;
                break;
            case gl::State::DIRTY_BIT_DRAW_INDIRECT_BUFFER_BINDING:
                WARN() << "DIRTY_BIT_DRAW_INDIRECT_BUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_DISPATCH_INDIRECT_BUFFER_BINDING:
                WARN() << "DIRTY_BIT_DISPATCH_INDIRECT_BUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_PROGRAM_BINDING:
                WARN() << "DIRTY_BIT_PROGRAM_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_PROGRAM_EXECUTABLE:
            {
                ProgramVk *programVk = vk::GetImpl(glState.getProgram());
                mPipelineDesc->updateShaders(programVk);
                dirtyTextures = true;
                break;
            }
            case gl::State::DIRTY_BIT_TEXTURE_BINDINGS:
                dirtyTextures = true;
                break;
            case gl::State::DIRTY_BIT_SAMPLER_BINDINGS:
                dirtyTextures = true;
                break;
            case gl::State::DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING:
                WARN() << "DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING:
                WARN() << "DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING unimplemented";
                break;
            case gl::State::DIRTY_BIT_UNIFORM_BUFFER_BINDINGS:
                WARN() << "DIRTY_BIT_UNIFORM_BUFFER_BINDINGS unimplemented";
                break;
            case gl::State::DIRTY_BIT_MULTISAMPLING:
                WARN() << "DIRTY_BIT_MULTISAMPLING unimplemented";
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_ONE:
                WARN() << "DIRTY_BIT_SAMPLE_ALPHA_TO_ONE unimplemented";
                break;
            case gl::State::DIRTY_BIT_COVERAGE_MODULATION:
                WARN() << "DIRTY_BIT_COVERAGE_MODULATION unimplemented";
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_MATRIX_MV:
                WARN() << "DIRTY_BIT_PATH_RENDERING_MATRIX_MV unimplemented";
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_MATRIX_PROJ:
                WARN() << "DIRTY_BIT_PATH_RENDERING_MATRIX_PROJ unimplemented";
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_STENCIL_STATE:
                WARN() << "DIRTY_BIT_PATH_RENDERING_STENCIL_STATE unimplemented";
                break;
            case gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB:
                WARN() << "DIRTY_BIT_FRAMEBUFFER_SRGB unimplemented";
                break;
            case gl::State::DIRTY_BIT_CURRENT_VALUES:
                WARN() << "DIRTY_BIT_CURRENT_VALUES unimplemented";
                break;
            default:
                UNREACHABLE();
                break;
        }
    }

    if (dirtyTextures)
    {
        ProgramVk *programVk = vk::GetImpl(glState.getProgram());
        programVk->invalidateTextures();
        mTexturesDirty = true;
    }
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

void ContextVk::onMakeCurrent(const gl::Context * /*context*/)
{
}

const gl::Caps &ContextVk::getNativeCaps() const
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
    return FramebufferVk::CreateUserFBO(state);
}

TextureImpl *ContextVk::createTexture(const gl::TextureState &state)
{
    return new TextureVk(state);
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
    return new VertexArrayVk(state);
}

QueryImpl *ContextVk::createQuery(GLenum type)
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
    mCurrentPipeline = nullptr;
}

void ContextVk::onVertexArrayChange()
{
    // TODO(jmadill): Does not handle dependent state changes.
    mVertexArrayDirty = true;
    invalidateCurrentPipeline();
}

gl::Error ContextVk::dispatchCompute(const gl::Context *context,
                                     GLuint numGroupsX,
                                     GLuint numGroupsY,
                                     GLuint numGroupsZ)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::dispatchComputeIndirect(const gl::Context *context, GLintptr indirect)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::memoryBarrier(const gl::Context *context, GLbitfield barriers)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

vk::DescriptorPool *ContextVk::getDescriptorPool()
{
    return &mDescriptorPool;
}

const VkClearValue &ContextVk::getClearColorValue() const
{
    return mClearColorValue;
}

const VkClearValue &ContextVk::getClearDepthStencilValue() const
{
    return mClearDepthStencilValue;
}

}  // namespace rx

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
ContextVk::ContextVk(const gl::ContextState &state, RendererVk *renderer)
    : ContextImpl(state),
      mRenderer(renderer),
      mCurrentDrawMode(GL_NONE),
      mDynamicDescriptorPool(),
      mTexturesDirty(false),
      mVertexArrayBindingHasChanged(false)
{
    memset(&mClearColorValue, 0, sizeof(mClearColorValue));
    memset(&mClearDepthStencilValue, 0, sizeof(mClearDepthStencilValue));
}

ContextVk::~ContextVk()
{
}

void ContextVk::onDestroy(const gl::Context *context)
{
    mDynamicDescriptorPool.destroy(mRenderer);
}

gl::Error ContextVk::initialize()
{
    ANGLE_TRY(mDynamicDescriptorPool.init(this->getDevice(),
                                          mRenderer->getUniformBufferDescriptorCount(),
                                          mRenderer->getMaxActiveTextures()));

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
                               const gl::DrawCallParams &drawCallParams,
                               vk::CommandGraphNode **drawNodeOut,
                               bool *newCommandBufferOut)
{
    if (drawCallParams.mode() != mCurrentDrawMode)
    {
        invalidateCurrentPipeline();
        mCurrentDrawMode = drawCallParams.mode();
    }

    if (!mCurrentPipeline)
    {
        ANGLE_TRY(initPipeline(context));
    }

    const auto &state            = mState.getState();
    const gl::Program *programGL = state.getProgram();
    ProgramVk *programVk         = vk::GetImpl(programGL);
    const auto *drawFBO          = state.getDrawFramebuffer();
    FramebufferVk *vkFBO         = vk::GetImpl(drawFBO);
    Serial queueSerial           = mRenderer->getCurrentQueueSerial();

    vk::CommandGraphNode *graphNode = nullptr;
    ANGLE_TRY(vkFBO->getCommandGraphNodeForDraw(context, &graphNode));

    vk::CommandBuffer *commandBuffer = nullptr;

    if (!graphNode->getInsideRenderPassCommands()->valid())
    {
        mTexturesDirty    = true;
        *newCommandBufferOut = true;
        ANGLE_TRY(graphNode->beginInsideRenderPassRecording(mRenderer, &commandBuffer));
    }
    else
    {
        *newCommandBufferOut          = mVertexArrayBindingHasChanged;
        mVertexArrayBindingHasChanged = false;
        commandBuffer                 = graphNode->getInsideRenderPassCommands();
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

    commandBuffer->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, mCurrentPipeline->get());

    // Update the queue serial for the pipeline object.
    ASSERT(mCurrentPipeline && mCurrentPipeline->valid());
    mCurrentPipeline->updateSerial(queueSerial);

    // TODO(jmadill): Can probably use more dirty bits here.
    ANGLE_TRY(programVk->updateUniforms(this));
    ANGLE_TRY(programVk->updateTexturesDescriptorSet(this));

    // Bind the graphics descriptor sets.
    // TODO(jmadill): Handle multiple command buffers.
    const auto &descriptorSets = programVk->getDescriptorSets();
    const gl::RangeUI &usedRange = programVk->getUsedDescriptorSetRange();
    if (!usedRange.empty())
    {
        ASSERT(!descriptorSets.empty());
        const vk::PipelineLayout &pipelineLayout = mRenderer->getGraphicsPipelineLayout();

        commandBuffer->bindDescriptorSets(
            VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, usedRange.low(), usedRange.length(),
            &descriptorSets[usedRange.low()], programVk->getDynamicOffsetsCount(),
            programVk->getDynamicOffsets());
    }

    *drawNodeOut = graphNode;
    return gl::NoError();
}

gl::Error ContextVk::drawArrays(const gl::Context *context, GLenum mode, GLint first, GLsizei count)
{
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandGraphNode *drawNode = nullptr;
    bool newCommands               = false;
    ANGLE_TRY(setupDraw(context, drawCallParams, &drawNode, &newCommands));

    const gl::VertexArray *vertexArray = context->getGLState().getVertexArray();
    VertexArrayVk *vertexArrayVk       = vk::GetImpl(vertexArray);
    ANGLE_TRY(vertexArrayVk->drawArrays(context, mRenderer, drawCallParams, drawNode, newCommands));

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
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandGraphNode *drawNode = nullptr;
    bool newCommands               = false;
    ANGLE_TRY(setupDraw(context, drawCallParams, &drawNode, &newCommands));

    gl::VertexArray *vao         = mState.getState().getVertexArray();
    VertexArrayVk *vertexArrayVk = vk::GetImpl(vao);
    ANGLE_TRY(
        vertexArrayVk->drawElements(context, mRenderer, drawCallParams, drawNode, newCommands));

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
                mPipelineDesc->updateColorWriteMask(glState.getBlendState());
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
                mPipelineDesc->updateDepthTestEnabled(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_DEPTH_FUNC:
                mPipelineDesc->updateDepthFunc(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_DEPTH_MASK:
                mPipelineDesc->updateDepthWriteEnabled(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_TEST_ENABLED:
                mPipelineDesc->updateStencilTestEnabled(glState.getDepthStencilState());
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
                mPipelineDesc->updateStencilFrontWriteMask(glState.getDepthStencilState());
                break;
            case gl::State::DIRTY_BIT_STENCIL_WRITEMASK_BACK:
                mPipelineDesc->updateStencilBackWriteMask(glState.getDepthStencilState());
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
                invalidateCurrentPipeline();
                mVertexArrayBindingHasChanged = true;
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

vk::DynamicDescriptorPool *ContextVk::getDynamicDescriptorPool()
{
    return &mDynamicDescriptorPool;
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

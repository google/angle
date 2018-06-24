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

constexpr gl::Rectangle kMaxSizedScissor(0,
                                         0,
                                         std::numeric_limits<int>::max(),
                                         std::numeric_limits<int>::max());

constexpr VkColorComponentFlags kAllColorChannelsMask =
    (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT |
     VK_COLOR_COMPONENT_A_BIT);
}  // anonymous namespace

ContextVk::ContextVk(const gl::ContextState &state, RendererVk *renderer)
    : ContextImpl(state),
      vk::Context(renderer),
      mCurrentDrawMode(gl::PrimitiveMode::InvalidEnum),
      mTexturesDirty(false),
      mVertexArrayBindingHasChanged(false),
      mClearColorMask(kAllColorChannelsMask),
      mFlipYForCurrentSurface(false),
      mDriverUniformsBuffer(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(DriverUniforms) * 16),
      mDriverUniformsDescriptorSet(VK_NULL_HANDLE)
{
    memset(&mClearColorValue, 0, sizeof(mClearColorValue));
    memset(&mClearDepthStencilValue, 0, sizeof(mClearDepthStencilValue));
}

ContextVk::~ContextVk() = default;

void ContextVk::onDestroy(const gl::Context *context)
{
    mDriverUniformsSetLayout.reset();
    mIncompleteTextures.onDestroy(context);
    mDriverUniformsBuffer.destroy(getDevice());

    for (vk::DynamicDescriptorPool &descriptorPool : mDynamicDescriptorPools)
    {
        descriptorPool.destroy(getDevice());
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

gl::Error ContextVk::initialize()
{
    // Note that this may reserve more sets than strictly necessary for a particular layout.
    VkDescriptorPoolSize uniformPoolSize = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        GetUniformBufferDescriptorCount() * vk::kDefaultDescriptorPoolMaxSets};

    ANGLE_TRY(mDynamicDescriptorPools[kUniformsDescriptorSetIndex].init(this, uniformPoolSize));

    VkDescriptorPoolSize imageSamplerPoolSize = {
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        mRenderer->getMaxActiveTextures() * vk::kDefaultDescriptorPoolMaxSets};
    ANGLE_TRY(mDynamicDescriptorPools[kTextureDescriptorSetIndex].init(this, imageSamplerPoolSize));

    VkDescriptorPoolSize driverUniformsPoolSize = {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                                   vk::kDefaultDescriptorPoolMaxSets};
    ANGLE_TRY(mDynamicDescriptorPools[kDriverUniformsDescriptorSetIndex].init(
        this, driverUniformsPoolSize));

    mPipelineDesc.reset(new vk::PipelineDesc());
    mPipelineDesc->initDefaults();

    return gl::NoError();
}

gl::Error ContextVk::flush(const gl::Context *context)
{
    // TODO(jmadill): Multiple flushes will need to insert semaphores. http://anglebug.com/2504

    // dEQP tests rely on having no errors thrown at the end of the test and they always call
    // flush at the end of the their tests. Just returning NoError until we implement flush
    // allow us to work on enabling many tests in the meantime.
    WARN() << "Flush is unimplemented. http://anglebug.com/2504";
    return gl::NoError();
}

gl::Error ContextVk::finish(const gl::Context *context)
{
    return mRenderer->finish(this);
}

gl::Error ContextVk::initPipeline(const gl::DrawCallParams &drawCallParams)
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
    mPipelineDesc->updateRenderPassDesc(framebufferVk->getRenderPassDesc());

    // Trigger draw call shader patching and fill out the pipeline desc.
    const vk::ShaderAndSerial *vertexShaderAndSerial   = nullptr;
    const vk::ShaderAndSerial *fragmentShaderAndSerial = nullptr;
    ANGLE_TRY(programVk->initShaders(this, drawCallParams, &vertexShaderAndSerial,
                                     &fragmentShaderAndSerial));

    mPipelineDesc->updateShaders(vertexShaderAndSerial->getSerial(),
                                 fragmentShaderAndSerial->getSerial());

    const vk::PipelineLayout &pipelineLayout = programVk->getPipelineLayout();

    ANGLE_TRY(mRenderer->getPipeline(this, *vertexShaderAndSerial, *fragmentShaderAndSerial,
                                     pipelineLayout, *mPipelineDesc, activeAttribLocationsMask,
                                     &mCurrentPipeline));

    return gl::NoError();
}

gl::Error ContextVk::setupDraw(const gl::Context *context,
                               const gl::DrawCallParams &drawCallParams,
                               vk::CommandBuffer **commandBufferOut,
                               bool *shouldApplyVertexArrayOut)
{
    if (drawCallParams.mode() != mCurrentDrawMode)
    {
        invalidateCurrentPipeline();
        mCurrentDrawMode = drawCallParams.mode();
    }

    if (!mCurrentPipeline)
    {
        ANGLE_TRY(initPipeline(drawCallParams));
    }

    const auto &state                  = mState.getState();
    const gl::Program *programGL       = state.getProgram();
    ProgramVk *programVk               = vk::GetImpl(programGL);
    const gl::Framebuffer *framebuffer = state.getDrawFramebuffer();
    FramebufferVk *framebufferVk       = vk::GetImpl(framebuffer);
    Serial queueSerial                 = mRenderer->getCurrentQueueSerial();

    vk::RecordingMode mode = vk::RecordingMode::Start;
    ANGLE_TRY(framebufferVk->getCommandBufferForDraw(this, commandBufferOut, &mode));

    if (mode == vk::RecordingMode::Start)
    {
        mTexturesDirty             = true;
        *shouldApplyVertexArrayOut = true;
    }
    else
    {
        *shouldApplyVertexArrayOut    = mVertexArrayBindingHasChanged;
        mVertexArrayBindingHasChanged = false;
    }

    // Ensure any writes to the textures are flushed before we read from them.
    if (mTexturesDirty)
    {
        mTexturesDirty = false;

        // TODO(jmadill): Should probably merge this for loop with programVk's descriptor update.
        for (size_t textureIndex : state.getActiveTexturesMask())
        {
            TextureVk *textureVk = mActiveTextures[textureIndex];
            ANGLE_TRY(textureVk->ensureImageInitialized(this));
            textureVk->addReadDependency(framebufferVk);
        }
    }

    (*commandBufferOut)->bindPipeline(VK_PIPELINE_BIND_POINT_GRAPHICS, mCurrentPipeline->get());

    // Update the queue serial for the pipeline object.
    ASSERT(mCurrentPipeline && mCurrentPipeline->valid());
    mCurrentPipeline->updateSerial(queueSerial);

    // TODO(jmadill): Can probably use more dirty bits here.
    ANGLE_TRY(programVk->updateUniforms(this));
    ANGLE_TRY(programVk->updateTexturesDescriptorSet(this));

    // Bind the graphics descriptor sets.
    // TODO(jmadill): Handle multiple command buffers.
    const auto &descriptorSets               = programVk->getDescriptorSets();
    const gl::RangeUI &usedRange             = programVk->getUsedDescriptorSetRange();
    const vk::PipelineLayout &pipelineLayout = programVk->getPipelineLayout();
    if (!usedRange.empty())
    {
        ASSERT(!descriptorSets.empty());
        (*commandBufferOut)
            ->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, usedRange.low(),
                                 usedRange.length(), &descriptorSets[usedRange.low()],
                                 programVk->getDynamicOffsetsCount(),
                                 programVk->getDynamicOffsets());
    }

    (*commandBufferOut)
        ->bindDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
                             kDriverUniformsDescriptorSetIndex, 1, &mDriverUniformsDescriptorSet, 0,
                             nullptr);

    return gl::NoError();
}

gl::Error ContextVk::drawArrays(const gl::Context *context,
                                gl::PrimitiveMode mode,
                                GLint first,
                                GLsizei count)
{
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandBuffer *commandBuffer = nullptr;
    bool shouldApplyVertexArray      = false;
    ANGLE_TRY(setupDraw(context, drawCallParams, &commandBuffer, &shouldApplyVertexArray));

    const gl::VertexArray *vertexArray = context->getGLState().getVertexArray();
    VertexArrayVk *vertexArrayVk       = vk::GetImpl(vertexArray);
    ANGLE_TRY(
        vertexArrayVk->drawArrays(context, drawCallParams, commandBuffer, shouldApplyVertexArray));

    return gl::NoError();
}

gl::Error ContextVk::drawArraysInstanced(const gl::Context *context,
                                         gl::PrimitiveMode mode,
                                         GLint first,
                                         GLsizei count,
                                         GLsizei instanceCount)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::drawElements(const gl::Context *context,
                                  gl::PrimitiveMode mode,
                                  GLsizei count,
                                  GLenum type,
                                  const void *indices)
{
    const gl::DrawCallParams &drawCallParams = context->getParams<gl::DrawCallParams>();

    vk::CommandBuffer *commandBuffer = nullptr;
    bool shouldApplyVertexArray      = false;
    ANGLE_TRY(setupDraw(context, drawCallParams, &commandBuffer, &shouldApplyVertexArray));

    gl::VertexArray *vao         = mState.getState().getVertexArray();
    VertexArrayVk *vertexArrayVk = vk::GetImpl(vao);
    ANGLE_TRY(vertexArrayVk->drawElements(context, drawCallParams, commandBuffer,
                                          shouldApplyVertexArray));

    return gl::NoError();
}

gl::Error ContextVk::drawElementsInstanced(const gl::Context *context,
                                           gl::PrimitiveMode mode,
                                           GLsizei count,
                                           GLenum type,
                                           const void *indices,
                                           GLsizei instances)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error ContextVk::drawRangeElements(const gl::Context *context,
                                       gl::PrimitiveMode mode,
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
                                        gl::PrimitiveMode mode,
                                        const void *indirect)
{
    UNIMPLEMENTED();
    return gl::InternalError() << "DrawArraysIndirect hasn't been implemented for vulkan backend.";
}

gl::Error ContextVk::drawElementsIndirect(const gl::Context *context,
                                          gl::PrimitiveMode mode,
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

gl::Error ContextVk::syncState(const gl::Context *context, const gl::State::DirtyBits &dirtyBits)
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
            case gl::State::DIRTY_BIT_SCISSOR:
                updateScissor(glState);
                break;
            case gl::State::DIRTY_BIT_VIEWPORT:
            {
                FramebufferVk *framebufferVk = vk::GetImpl(glState.getDrawFramebuffer());
                mPipelineDesc->updateViewport(framebufferVk, glState.getViewport(),
                                              glState.getNearPlane(), glState.getFarPlane(),
                                              isViewportFlipEnabledForDrawFBO());
                ANGLE_TRY(updateDriverUniforms(glState));
                break;
            }
            case gl::State::DIRTY_BIT_DEPTH_RANGE:
                mPipelineDesc->updateDepthRange(glState.getNearPlane(), glState.getFarPlane());
                ANGLE_TRY(updateDriverUniforms(glState));
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
                mPipelineDesc->updateFrontFace(glState.getRasterizerState(),
                                               isViewportFlipEnabledForDrawFBO());
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET_FILL_ENABLED:
                break;
            case gl::State::DIRTY_BIT_POLYGON_OFFSET:
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
                ANGLE_TRY(updateDriverUniforms(glState));
                updateFlipViewportDrawFramebuffer(glState);
                FramebufferVk *framebufferVk = vk::GetImpl(glState.getDrawFramebuffer());
                mPipelineDesc->updateViewport(framebufferVk, glState.getViewport(),
                                              glState.getNearPlane(), glState.getFarPlane(),
                                              isViewportFlipEnabledForDrawFBO());
                updateColorMask(glState.getBlendState());
                mPipelineDesc->updateCullMode(glState.getRasterizerState());
                updateScissor(glState);
                break;
            }
            case gl::State::DIRTY_BIT_RENDERBUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_VERTEX_ARRAY_BINDING:
                mVertexArrayBindingHasChanged = true;
                break;
            case gl::State::DIRTY_BIT_DRAW_INDIRECT_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_DISPATCH_INDIRECT_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_PROGRAM_BINDING:
                break;
            case gl::State::DIRTY_BIT_PROGRAM_EXECUTABLE:
            {
                dirtyTextures = true;
                // No additional work is needed here. We will update the pipeline desc later.
                break;
            }
            case gl::State::DIRTY_BIT_TEXTURE_BINDINGS:
                dirtyTextures = true;
                break;
            case gl::State::DIRTY_BIT_SAMPLER_BINDINGS:
                dirtyTextures = true;
                break;
            case gl::State::DIRTY_BIT_TRANSFORM_FEEDBACK_BINDING:
                break;
            case gl::State::DIRTY_BIT_SHADER_STORAGE_BUFFER_BINDING:
                break;
            case gl::State::DIRTY_BIT_UNIFORM_BUFFER_BINDINGS:
                break;
            case gl::State::DIRTY_BIT_MULTISAMPLING:
                break;
            case gl::State::DIRTY_BIT_SAMPLE_ALPHA_TO_ONE:
                break;
            case gl::State::DIRTY_BIT_COVERAGE_MODULATION:
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_MATRIX_MV:
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_MATRIX_PROJ:
                break;
            case gl::State::DIRTY_BIT_PATH_RENDERING_STENCIL_STATE:
                break;
            case gl::State::DIRTY_BIT_FRAMEBUFFER_SRGB:
                break;
            case gl::State::DIRTY_BIT_CURRENT_VALUES:
                break;
            default:
                UNREACHABLE();
                break;
        }
    }

    if (dirtyTextures)
    {
        ANGLE_TRY(updateActiveTextures(context));

        ProgramVk *programVk = vk::GetImpl(glState.getProgram());
        programVk->invalidateTextures();
        mTexturesDirty = true;
    }

    return gl::NoError();
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

gl::Error ContextVk::onMakeCurrent(const gl::Context *context)
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
    ANGLE_TRY(updateDriverUniforms(glState));
    return gl::NoError();
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
    return FramebufferVk::CreateUserFBO(state);
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

vk::DynamicDescriptorPool *ContextVk::getDynamicDescriptorPool(uint32_t descriptorSetIndex)
{
    return &mDynamicDescriptorPools[descriptorSetIndex];
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

angle::Result ContextVk::updateDriverUniforms(const gl::State &glState)
{
    if (!mDriverUniformsBuffer.valid())
    {
        size_t minAlignment = static_cast<size_t>(
            mRenderer->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);
        mDriverUniformsBuffer.init(minAlignment, mRenderer);
    }

    // Release any previously retained buffers.
    mDriverUniformsBuffer.releaseRetainedBuffers(mRenderer);

    const gl::Rectangle &glViewport = glState.getViewport();

    // Allocate a new region in the dynamic buffer.
    uint8_t *ptr            = nullptr;
    VkBuffer buffer         = VK_NULL_HANDLE;
    uint32_t offset         = 0;
    bool newBufferAllocated = false;
    ANGLE_TRY(mDriverUniformsBuffer.allocate(this, sizeof(DriverUniforms), &ptr, &buffer, &offset,
                                             &newBufferAllocated));
    float scaleY = isViewportFlipEnabledForDrawFBO() ? 1.0f : -1.0f;

    float depthRangeNear = glState.getNearPlane();
    float depthRangeFar  = glState.getFarPlane();
    float depthRangeDiff = depthRangeFar - depthRangeNear;

    // Copy and flush to the device.
    DriverUniforms *driverUniforms = reinterpret_cast<DriverUniforms *>(ptr);
    *driverUniforms                = {
        {static_cast<float>(glViewport.x), static_cast<float>(glViewport.y),
         static_cast<float>(glViewport.width), static_cast<float>(glViewport.height)},
        {1.0f, scaleY, 1.0f, 1.0f},
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
        this, mDriverUniformsSetLayout.get().ptr(), 1, &mDriverUniformsDescriptorSet));

    // Update the driver uniform descriptor set.
    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range  = sizeof(DriverUniforms);

    VkWriteDescriptorSet writeInfo;
    writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeInfo.pNext            = nullptr;
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

    mErrors->handleError(gl::Error(glErrorCode, glErrorCode, errorStream.str()));
}

gl::Error ContextVk::updateActiveTextures(const gl::Context *context)
{
    const auto &completeTextures = mState.getState().getCompleteTextureCache();
    const gl::Program *program = mState.getState().getProgram();

    mActiveTextures.fill(nullptr);

    for (const gl::SamplerBinding &samplerBinding : program->getSamplerBindings())
    {
        ASSERT(!samplerBinding.unreferenced);

        for (GLuint textureUnit : samplerBinding.boundTextureUnits)
        {
            gl::Texture *texture = completeTextures[textureUnit];

            // Null textures represent incomplete textures.
            if (texture == nullptr)
            {
                ANGLE_TRY(getIncompleteTexture(context, samplerBinding.textureType, &texture));
            }

            mActiveTextures[textureUnit] = vk::GetImpl(texture);
        }
    }

    return gl::NoError();
}

const gl::ActiveTextureArray<TextureVk *> &ContextVk::getActiveTextures() const
{
    return mActiveTextures;
}
}  // namespace rx

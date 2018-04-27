//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferVk.cpp:
//    Implements the class methods for FramebufferVk.
//

#include "libANGLE/renderer/vulkan/FramebufferVk.h"

#include <vulkan/vulkan.h>
#include <array>

#include "common/debug.h"
#include "image_util/imageformats.h"
#include "libANGLE/Context.h"
#include "libANGLE/Display.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/renderer/vulkan/CommandGraph.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/DisplayVk.h"
#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/SurfaceVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"

namespace rx
{

namespace
{
const gl::InternalFormat &GetReadAttachmentInfo(const gl::Context *context,
                                                RenderTargetVk *renderTarget)
{
    GLenum implFormat =
        renderTarget->image->getFormat().textureFormat().fboImplementationInternalFormat;
    return gl::GetSizedInternalFormatInfo(implFormat);
}
}  // anonymous namespace

// static
FramebufferVk *FramebufferVk::CreateUserFBO(const gl::FramebufferState &state)
{
    return new FramebufferVk(state);
}

// static
FramebufferVk *FramebufferVk::CreateDefaultFBO(const gl::FramebufferState &state,
                                               WindowSurfaceVk *backbuffer)
{
    return new FramebufferVk(state, backbuffer);
}

FramebufferVk::FramebufferVk(const gl::FramebufferState &state)
    : FramebufferImpl(state), mBackbuffer(nullptr), mRenderPassDesc(), mFramebuffer()
{
}

FramebufferVk::FramebufferVk(const gl::FramebufferState &state, WindowSurfaceVk *backbuffer)
    : FramebufferImpl(state), mBackbuffer(backbuffer), mRenderPassDesc(), mFramebuffer()
{
}

FramebufferVk::~FramebufferVk()
{
}

void FramebufferVk::destroy(const gl::Context *context)
{
    RendererVk *renderer = vk::GetImpl(context)->getRenderer();

    renderer->releaseResource(*this, &mFramebuffer);
}

void FramebufferVk::destroyDefault(const egl::Display *display)
{
    VkDevice device = vk::GetImpl(display)->getRenderer()->getDevice();

    mFramebuffer.destroy(device);
}

gl::Error FramebufferVk::discard(const gl::Context *context,
                                 size_t count,
                                 const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::invalidate(const gl::Context *context,
                                    size_t count,
                                    const GLenum *attachments)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::invalidateSub(const gl::Context *context,
                                       size_t count,
                                       const GLenum *attachments,
                                       const gl::Rectangle &area)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clear(const gl::Context *context, GLbitfield mask)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    Serial currentSerial = renderer->getCurrentQueueSerial();

    // This command buffer is only started once.
    vk::CommandBuffer *commandBuffer  = nullptr;
    vk::CommandGraphNode *writingNode = nullptr;

    const gl::FramebufferAttachment *depthAttachment = mState.getDepthAttachment();
    bool clearDepth = (depthAttachment && (mask & GL_DEPTH_BUFFER_BIT) != 0);
    ASSERT(!clearDepth || depthAttachment->isAttached());

    const gl::FramebufferAttachment *stencilAttachment = mState.getStencilAttachment();
    bool clearStencil = (stencilAttachment && (mask & GL_STENCIL_BUFFER_BIT) != 0);
    ASSERT(!clearStencil || stencilAttachment->isAttached());

    bool clearColor = IsMaskFlagSet(static_cast<int>(mask), GL_COLOR_BUFFER_BIT);

    const gl::FramebufferAttachment *depthStencilAttachment = mState.getDepthStencilAttachment();
    const gl::State &glState                                = context->getGLState();

    // If we clear the depth OR the stencil but not both, and we have a packed depth stencil
    // attachment, we need to use clearAttachment instead of clearDepthStencil since Vulkan won't
    // allow us to clear one or the other separately.
    bool isSingleClearOnPackedDepthStencilAttachment =
        depthStencilAttachment && (clearDepth != clearStencil);
    if (glState.isScissorTestEnabled() || isSingleClearOnPackedDepthStencilAttachment)
    {
        // With scissor test enabled, we clear very differently and we don't need to access
        // the image inside each attachment we can just use clearCmdAttachments with our
        // scissor region instead.
        ANGLE_TRY(clearWithClearAttachments(contextVk, clearColor, clearDepth, clearStencil));
        return gl::NoError();
    }

    // Standard Depth/stencil clear without scissor.
    if (clearDepth || clearStencil)
    {
        ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));
        writingNode = getCurrentWritingNode();

        const VkClearDepthStencilValue &clearDepthStencilValue =
            contextVk->getClearDepthStencilValue().depthStencil;

        // We only support packed depth/stencil, not separate.
        ASSERT(!(clearDepth && clearStencil) || depthStencilAttachment);

        const VkImageAspectFlags aspectFlags =
            (depthAttachment ? VK_IMAGE_ASPECT_DEPTH_BIT : 0) |
            (stencilAttachment ? VK_IMAGE_ASPECT_STENCIL_BIT : 0);

        RenderTargetVk *renderTarget = mRenderTargetCache.getDepthStencil();
        renderTarget->resource->onWriteResource(writingNode, currentSerial);
        renderTarget->image->clearDepthStencil(aspectFlags, clearDepthStencilValue, commandBuffer);

        if (!clearColor)
        {
            return gl::NoError();
        }
    }

    ASSERT(clearColor);
    const auto *attachment = mState.getFirstNonNullAttachment();
    ASSERT(attachment && attachment->isAttached());

    if (!commandBuffer)
    {
        ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));
        writingNode = getCurrentWritingNode();
    }

    // TODO(jmadill): Check for masked color clear. http://anglebug.com/2455

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        colorRenderTarget->resource->onWriteResource(writingNode, currentSerial);
        colorRenderTarget->image->clearColor(contextVk->getClearColorValue().color, commandBuffer);
    }

    return gl::NoError();
}

gl::Error FramebufferVk::clearBufferfv(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLfloat *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferuiv(const gl::Context *context,
                                        GLenum buffer,
                                        GLint drawbuffer,
                                        const GLuint *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferiv(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       const GLint *values)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

gl::Error FramebufferVk::clearBufferfi(const gl::Context *context,
                                       GLenum buffer,
                                       GLint drawbuffer,
                                       GLfloat depth,
                                       GLint stencil)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

GLenum FramebufferVk::getImplementationColorReadFormat(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).format;
}

GLenum FramebufferVk::getImplementationColorReadType(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, mRenderTargetCache.getColorRead(mState)).type;
}

gl::Error FramebufferVk::readPixels(const gl::Context *context,
                                    const gl::Rectangle &area,
                                    GLenum format,
                                    GLenum type,
                                    void *pixels)
{
    const gl::State &glState = context->getGLState();
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();
    VkDevice device      = renderer->getDevice();

    RenderTargetVk *renderTarget = mRenderTargetCache.getColorRead(mState);
    ASSERT(renderTarget);

    vk::ImageHelper stagingImage;
    ANGLE_TRY(stagingImage.init2DStaging(
        device, renderer->getMemoryProperties(), renderTarget->image->getFormat(),
        gl::Extents(area.width, area.height, 1), vk::StagingUsage::Read));

    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(beginWriteResource(renderer, &commandBuffer));

    stagingImage.changeLayoutWithStages(VK_IMAGE_ASPECT_COLOR_BIT, VK_IMAGE_LAYOUT_GENERAL,
                                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                                        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, commandBuffer);

    vk::ImageHelper::Copy(renderTarget->image, &stagingImage, gl::Offset(area.x, area.y, 0),
                          gl::Offset(), gl::Extents(area.width, area.height, 1),
                          VK_IMAGE_ASPECT_COLOR_BIT, commandBuffer);

    // Triggers a full finish.
    // TODO(jmadill): Don't block on asynchronous readback.
    ANGLE_TRY(renderer->finish(context));

    // TODO(jmadill): parameters
    uint8_t *mapPointer = nullptr;
    ANGLE_TRY(stagingImage.getDeviceMemory().map(device, 0, stagingImage.getAllocatedMemorySize(),
                                                 0, &mapPointer));

    const angle::Format &angleFormat = renderTarget->image->getFormat().textureFormat();
    GLuint outputPitch               = angleFormat.pixelBytes * area.width;

    // Get the staging image pitch and use it to pack the pixels later.
    VkSubresourceLayout subresourceLayout;
    stagingImage.getImage().getSubresourceLayout(device, VK_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                                                 &subresourceLayout);

    PackPixelsParams params;
    params.area        = area;
    params.format      = format;
    params.type        = type;
    params.outputPitch = outputPitch;
    params.packBuffer  = glState.getTargetBuffer(gl::BufferBinding::PixelPack);
    params.pack        = glState.getPackState();

    PackPixels(params, angleFormat, static_cast<int>(subresourceLayout.rowPitch), mapPointer,
               reinterpret_cast<uint8_t *>(pixels));

    stagingImage.getDeviceMemory().unmap(device);
    renderer->releaseObject(renderer->getCurrentQueueSerial(), &stagingImage);

    return vk::NoError();
}

gl::Error FramebufferVk::blit(const gl::Context *context,
                              const gl::Rectangle &sourceArea,
                              const gl::Rectangle &destArea,
                              GLbitfield mask,
                              GLenum filter)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

bool FramebufferVk::checkStatus(const gl::Context *context) const
{
    return true;
}

gl::Error FramebufferVk::syncState(const gl::Context *context,
                                   const gl::Framebuffer::DirtyBits &dirtyBits)
{
    ContextVk *contextVk = vk::GetImpl(context);
    RendererVk *renderer = contextVk->getRenderer();

    ASSERT(dirtyBits.any());
    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::Framebuffer::DIRTY_BIT_DEPTH_ATTACHMENT:
            case gl::Framebuffer::DIRTY_BIT_STENCIL_ATTACHMENT:
                ANGLE_TRY(mRenderTargetCache.updateDepthStencilRenderTarget(context, mState));
                break;
            case gl::Framebuffer::DIRTY_BIT_DRAW_BUFFERS:
            case gl::Framebuffer::DIRTY_BIT_READ_BUFFER:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_WIDTH:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_HEIGHT:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_SAMPLES:
            case gl::Framebuffer::DIRTY_BIT_DEFAULT_FIXED_SAMPLE_LOCATIONS:
                break;
            default:
            {
                ASSERT(gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0 == 0 &&
                       dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_MAX);
                size_t colorIndex =
                    static_cast<size_t>(dirtyBit - gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0);
                ANGLE_TRY(mRenderTargetCache.updateColorRenderTarget(context, mState, colorIndex));
                break;
            }
        }
    }

    mRenderPassDesc.reset();
    renderer->releaseResource(*this, &mFramebuffer);

    // Trigger a new set of secondary commands next time we render to this FBO.
    getNewWritingNode(renderer);

    contextVk->invalidateCurrentPipeline();

    return gl::NoError();
}

const vk::RenderPassDesc &FramebufferVk::getRenderPassDesc()
{
    if (mRenderPassDesc.valid())
    {
        return mRenderPassDesc.value();
    }

    vk::RenderPassDesc desc;

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        desc.packColorAttachment(*colorRenderTarget->image);
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        desc.packDepthStencilAttachment(*depthStencilRenderTarget->image);
    }

    mRenderPassDesc = desc;
    return mRenderPassDesc.value();
}

gl::ErrorOrResult<vk::Framebuffer *> FramebufferVk::getFramebuffer(RendererVk *rendererVk)
{
    // If we've already created our cached Framebuffer, return it.
    if (mFramebuffer.valid())
    {
        return &mFramebuffer;
    }

    const vk::RenderPassDesc &desc = getRenderPassDesc();

    vk::RenderPass *renderPass = nullptr;
    ANGLE_TRY(rendererVk->getCompatibleRenderPass(desc, &renderPass));

    // If we've a Framebuffer provided by a Surface (default FBO/backbuffer), query it.
    VkDevice device = rendererVk->getDevice();
    if (mBackbuffer)
    {
        return mBackbuffer->getCurrentFramebuffer(device, *renderPass);
    }

    // Gather VkImageViews over all FBO attachments, also size of attached region.
    std::vector<VkImageView> attachments;
    gl::Extents attachmentsSize;

    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);
        attachments.push_back(colorRenderTarget->imageView->getHandle());

        ASSERT(attachmentsSize.empty() ||
               attachmentsSize == colorRenderTarget->image->getExtents());
        attachmentsSize = colorRenderTarget->image->getExtents();
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        attachments.push_back(depthStencilRenderTarget->imageView->getHandle());

        ASSERT(attachmentsSize.empty() ||
               attachmentsSize == depthStencilRenderTarget->image->getExtents());
        attachmentsSize = depthStencilRenderTarget->image->getExtents();
    }

    ASSERT(!attachments.empty());

    VkFramebufferCreateInfo framebufferInfo;

    framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.pNext           = nullptr;
    framebufferInfo.flags           = 0;
    framebufferInfo.renderPass      = renderPass->getHandle();
    framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments    = attachments.data();
    framebufferInfo.width           = static_cast<uint32_t>(attachmentsSize.width);
    framebufferInfo.height          = static_cast<uint32_t>(attachmentsSize.height);
    framebufferInfo.layers          = 1;

    ANGLE_TRY(mFramebuffer.init(device, framebufferInfo));

    return &mFramebuffer;
}

gl::Error FramebufferVk::clearWithClearAttachments(ContextVk *contextVk,
                                                   bool clearColor,
                                                   bool clearDepth,
                                                   bool clearStencil)
{
    RendererVk *renderer = contextVk->getRenderer();

    // This command can only happen inside a render pass, so obtain one if its already happening
    // or create a new one if not.
    vk::CommandGraphNode *node       = nullptr;
    vk::CommandBuffer *commandBuffer = nullptr;
    ANGLE_TRY(getCommandGraphNodeForDraw(contextVk, &node));
    if (node->getInsideRenderPassCommands()->valid())
    {
        commandBuffer = node->getInsideRenderPassCommands();
    }
    else
    {
        ANGLE_TRY(node->beginInsideRenderPassRecording(renderer, &commandBuffer));
    }

    // TODO(jmadill): Cube map attachments. http://anglebug.com/2470
    // We assume for now that we always need to clear only 1 layer starting at the
    // baseArrayLayer 0, this might need to change depending how we'll implement
    // cube maps, 3d textures and array textures.
    VkClearRect clearRect;
    clearRect.baseArrayLayer = 0;
    clearRect.layerCount     = 1;

    // When clearing, the scissor region must be clipped to the renderArea per the validation rules
    // in Vulkan.
    gl::Rectangle intersection;
    if (!ClipRectangle(contextVk->getGLState().getScissor(), node->getRenderPassRenderArea(),
                       &intersection))
    {
        // There is nothing to clear since the scissor is outside of the render area.
        return gl::NoError();
    }

    clearRect.rect = gl_vk::GetRect(intersection);

    gl::AttachmentArray<VkClearAttachment> clearAttachments;
    int clearAttachmentIndex = 0;

    if (clearColor)
    {
        // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
        for (size_t colorIndex : mState.getEnabledDrawBuffers())
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_COLOR_BIT;
            clearAttachment.colorAttachment    = static_cast<uint32_t>(colorIndex);
            clearAttachment.clearValue         = contextVk->getClearColorValue();
            ++clearAttachmentIndex;
        }
    }

    if (clearDepth && clearStencil && mState.getDepthStencilAttachment() != nullptr)
    {
        // When we have a packed depth/stencil attachment we can do 1 clear for both when it
        // applies.
        VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
        clearAttachment.aspectMask      = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        clearAttachment.colorAttachment = VK_ATTACHMENT_UNUSED;
        clearAttachment.clearValue      = contextVk->getClearDepthStencilValue();
        ++clearAttachmentIndex;
    }
    else
    {
        if (clearDepth)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_DEPTH_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = contextVk->getClearDepthStencilValue();
            ++clearAttachmentIndex;
        }

        if (clearStencil)
        {
            VkClearAttachment &clearAttachment = clearAttachments[clearAttachmentIndex];
            clearAttachment.aspectMask         = VK_IMAGE_ASPECT_STENCIL_BIT;
            clearAttachment.colorAttachment    = VK_ATTACHMENT_UNUSED;
            clearAttachment.clearValue         = contextVk->getClearDepthStencilValue();
            ++clearAttachmentIndex;
        }
    }

    commandBuffer->clearAttachments(static_cast<uint32_t>(clearAttachmentIndex),
                                    clearAttachments.data(), 1, &clearRect);
    return gl::NoError();
}

gl::Error FramebufferVk::getSamplePosition(size_t index, GLfloat *xy) const
{
    UNIMPLEMENTED();
    return gl::InternalError() << "getSamplePosition is unimplemented.";
}

gl::Error FramebufferVk::getCommandGraphNodeForDraw(ContextVk *contextVk,
                                                    vk::CommandGraphNode **nodeOut)
{
    RendererVk *renderer = contextVk->getRenderer();
    Serial currentSerial = renderer->getCurrentQueueSerial();

    // This will reset the current writing node if it has been completed.
    updateQueueSerial(currentSerial);

    if (hasChildlessWritingNode())
    {
        *nodeOut = getCurrentWritingNode();
    }
    else
    {
        *nodeOut = getNewWritingNode(renderer);
    }

    if ((*nodeOut)->getInsideRenderPassCommands()->valid())
    {
        return gl::NoError();
    }

    vk::Framebuffer *framebuffer = nullptr;
    ANGLE_TRY_RESULT(getFramebuffer(renderer), framebuffer);

    std::vector<VkClearValue> attachmentClearValues;

    vk::CommandBuffer *commandBuffer = nullptr;
    if (!(*nodeOut)->getOutsideRenderPassCommands()->valid())
    {
        ANGLE_TRY((*nodeOut)->beginOutsideRenderPassRecording(
            renderer->getDevice(), renderer->getCommandPool(), &commandBuffer));
    }
    else
    {
        commandBuffer = (*nodeOut)->getOutsideRenderPassCommands();
    }

    // Initialize RenderPass info.
    // TODO(jmadill): Support gaps in RenderTargets. http://anglebug.com/2394
    const auto &colorRenderTargets = mRenderTargetCache.getColors();
    for (size_t colorIndex : mState.getEnabledDrawBuffers())
    {
        RenderTargetVk *colorRenderTarget = colorRenderTargets[colorIndex];
        ASSERT(colorRenderTarget);

        (*nodeOut)->appendColorRenderTarget(currentSerial, colorRenderTarget);
        attachmentClearValues.emplace_back(contextVk->getClearColorValue());
    }

    RenderTargetVk *depthStencilRenderTarget = mRenderTargetCache.getDepthStencil();
    if (depthStencilRenderTarget)
    {
        (*nodeOut)->appendDepthStencilRenderTarget(currentSerial, depthStencilRenderTarget);
        attachmentClearValues.emplace_back(contextVk->getClearDepthStencilValue());
    }

    gl::Rectangle renderArea =
        gl::Rectangle(0, 0, mState.getDimensions().width, mState.getDimensions().height);
    // Hard-code RenderPass to clear the first render target to the current clear value.
    // TODO(jmadill): Proper clear value implementation. http://anglebug.com/2361
    (*nodeOut)->storeRenderPassInfo(*framebuffer, renderArea, attachmentClearValues);

    return gl::NoError();
}

}  // namespace rx

//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// FramebufferMtl.mm:
//    Implements the class methods for FramebufferMtl.
//

#include "libANGLE/renderer/metal/ContextMtl.h"

#include <TargetConditionals.h>

#include "common/MemoryBuffer.h"
#include "common/angleutils.h"
#include "common/debug.h"
#include "libANGLE/renderer/metal/DisplayMtl.h"
#include "libANGLE/renderer/metal/FrameBufferMtl.h"
#include "libANGLE/renderer/metal/SurfaceMtl.h"
#include "libANGLE/renderer/metal/mtl_utils.h"
#include "libANGLE/renderer/renderer_utils.h"

namespace rx
{

namespace
{

const gl::InternalFormat &GetReadAttachmentInfo(const gl::Context *context,
                                                RenderTargetMtl *renderTarget)
{
    GLenum implFormat;

    if (renderTarget && renderTarget->getFormat())
    {
        implFormat = renderTarget->getFormat()->actualAngleFormat().fboImplementationInternalFormat;
    }
    else
    {
        implFormat = GL_NONE;
    }

    return gl::GetSizedInternalFormatInfo(implFormat);
}
}

// FramebufferMtl implementation
FramebufferMtl::FramebufferMtl(const gl::FramebufferState &state, bool flipY, bool alwaysDiscard)
    : FramebufferImpl(state), mAlwaysDiscardDepthStencil(alwaysDiscard), mFlipY(flipY)
{
    reset();
}

FramebufferMtl::~FramebufferMtl() {}

void FramebufferMtl::reset()
{
    for (auto &rt : mColorRenderTargets)
    {
        rt = nullptr;
    }
    for (auto &discardColor : mDiscardColors)
    {
        discardColor = false;
    }
    mDepthRenderTarget = mStencilRenderTarget = nullptr;
    mDiscardDepth = mDiscardStencil = false;
}

void FramebufferMtl::destroy(const gl::Context *context)
{
    reset();
}

angle::Result FramebufferMtl::discard(const gl::Context *context,
                                      size_t count,
                                      const GLenum *attachments)
{
    return invalidate(context, count, attachments);
}

angle::Result FramebufferMtl::invalidate(const gl::Context *context,
                                         size_t count,
                                         const GLenum *attachments)
{
    return invalidateImpl(mtl::GetImpl(context), count, attachments);
}

angle::Result FramebufferMtl::invalidateSub(const gl::Context *context,
                                            size_t count,
                                            const GLenum *attachments,
                                            const gl::Rectangle &area)
{
    // NOTE(hqle): ES 3.0 feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

angle::Result FramebufferMtl::clear(const gl::Context *context, GLbitfield mask)
{
    ContextMtl *contextMtl = mtl::GetImpl(context);

    mtl::ClearRectParams clearOpts;

    bool clearColor   = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_COLOR_BUFFER_BIT));
    bool clearDepth   = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_DEPTH_BUFFER_BIT));
    bool clearStencil = IsMaskFlagSet(mask, static_cast<GLbitfield>(GL_STENCIL_BUFFER_BIT));

    gl::DrawBufferMask clearColorBuffers;
    if (clearColor)
    {
        clearColorBuffers    = mState.getEnabledDrawBuffers();
        clearOpts.clearColor = contextMtl->getClearColorValue();
    }
    if (clearDepth)
    {
        clearOpts.clearDepth = contextMtl->getClearDepthValue();
    }
    if (clearStencil)
    {
        clearOpts.clearStencil = contextMtl->getClearStencilValue();
    }

    return clearImpl(context, clearColorBuffers, &clearOpts);
}

angle::Result FramebufferMtl::clearBufferfv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLfloat *values)
{
    // NOTE(hqle): ES 3.0 feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferuiv(const gl::Context *context,
                                             GLenum buffer,
                                             GLint drawbuffer,
                                             const GLuint *values)
{
    // NOTE(hqle): ES 3.0 feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferiv(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            const GLint *values)
{
    // NOTE(hqle): ES 3.0 feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}
angle::Result FramebufferMtl::clearBufferfi(const gl::Context *context,
                                            GLenum buffer,
                                            GLint drawbuffer,
                                            GLfloat depth,
                                            GLint stencil)
{
    // NOTE(hqle): ES 3.0 feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

GLenum FramebufferMtl::getImplementationColorReadFormat(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, getColorReadRenderTarget()).format;
}

GLenum FramebufferMtl::getImplementationColorReadType(const gl::Context *context) const
{
    return GetReadAttachmentInfo(context, getColorReadRenderTarget()).type;
}

angle::Result FramebufferMtl::readPixels(const gl::Context *context,
                                         const gl::Rectangle &area,
                                         GLenum format,
                                         GLenum type,
                                         void *pixels)
{
    // Clip read area to framebuffer.
    const gl::Extents &fbSize = getState().getReadAttachment()->getSize();
    const gl::Rectangle fbRect(0, 0, fbSize.width, fbSize.height);

    gl::Rectangle clippedArea;
    if (!ClipRectangle(area, fbRect, &clippedArea))
    {
        // nothing to read
        return angle::Result::Continue;
    }
    gl::Rectangle flippedArea = getReadPixelArea(clippedArea);

    ContextMtl *contextMtl              = mtl::GetImpl(context);
    const gl::State &glState            = context->getState();
    const gl::PixelPackState &packState = glState.getPackState();

    const gl::InternalFormat &sizedFormatInfo = gl::GetInternalFormatInfo(format, type);

    GLuint outputPitch = 0;
    ANGLE_CHECK_GL_MATH(contextMtl,
                        sizedFormatInfo.computeRowPitch(type, area.width, packState.alignment,
                                                        packState.rowLength, &outputPitch));
    GLuint outputSkipBytes = 0;
    ANGLE_CHECK_GL_MATH(contextMtl, sizedFormatInfo.computeSkipBytes(
                                        type, outputPitch, 0, packState, false, &outputSkipBytes));

    outputSkipBytes += (clippedArea.x - area.x) * sizedFormatInfo.pixelBytes +
                       (clippedArea.y - area.y) * outputPitch;

    const angle::Format &angleFormat = GetFormatFromFormatType(format, type);

    PackPixelsParams params(flippedArea, angleFormat, outputPitch, packState.reverseRowOrder,
                            glState.getTargetBuffer(gl::BufferBinding::PixelPack), 0);
    if (mFlipY)
    {
        params.reverseRowOrder = !params.reverseRowOrder;
    }

    ANGLE_TRY(readPixelsImpl(context, flippedArea, params, getColorReadRenderTarget(),
                             static_cast<uint8_t *>(pixels) + outputSkipBytes));

    return angle::Result::Continue;
}

angle::Result FramebufferMtl::blit(const gl::Context *context,
                                   const gl::Rectangle &sourceArea,
                                   const gl::Rectangle &destArea,
                                   GLbitfield mask,
                                   GLenum filter)
{
    // NOTE(hqle): MSAA feature.
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

bool FramebufferMtl::checkStatus(const gl::Context *context) const
{
    if (!mState.attachmentsHaveSameDimensions())
    {
        return false;
    }

    ContextMtl *contextMtl = mtl::GetImpl(context);
    if (!contextMtl->getDisplay()->getFeatures().allowSeparatedDepthStencilBuffers.enabled &&
        mState.hasSeparateDepthAndStencilAttachments())
    {
        return false;
    }

    return true;
}

angle::Result FramebufferMtl::syncState(const gl::Context *context,
                                        const gl::Framebuffer::DirtyBits &dirtyBits)
{
    ContextMtl *contextMtl = mtl::GetImpl(context);
    ASSERT(dirtyBits.any());
    for (size_t dirtyBit : dirtyBits)
    {
        switch (dirtyBit)
        {
            case gl::Framebuffer::DIRTY_BIT_DEPTH_ATTACHMENT:
                ANGLE_TRY(updateDepthRenderTarget(context));
                break;
            case gl::Framebuffer::DIRTY_BIT_STENCIL_ATTACHMENT:
                ANGLE_TRY(updateStencilRenderTarget(context));
                break;
            case gl::Framebuffer::DIRTY_BIT_DEPTH_BUFFER_CONTENTS:
            case gl::Framebuffer::DIRTY_BIT_STENCIL_BUFFER_CONTENTS:
                // NOTE(hqle): What are we supposed to do?
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
                static_assert(gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0 == 0, "FB dirty bits");
                if (dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_MAX)
                {
                    size_t colorIndexGL = static_cast<size_t>(
                        dirtyBit - gl::Framebuffer::DIRTY_BIT_COLOR_ATTACHMENT_0);
                    ANGLE_TRY(updateColorRenderTarget(context, colorIndexGL));
                }
                else
                {
                    ASSERT(dirtyBit >= gl::Framebuffer::DIRTY_BIT_COLOR_BUFFER_CONTENTS_0 &&
                           dirtyBit < gl::Framebuffer::DIRTY_BIT_COLOR_BUFFER_CONTENTS_MAX);
                    // NOTE(hqle): What are we supposed to do?
                }
                break;
            }
        }
    }

    auto oldRenderPassDesc = mRenderPassDesc;

    ANGLE_TRY(prepareRenderPass(context, mState.getEnabledDrawBuffers(), &mRenderPassDesc));

    if (!oldRenderPassDesc.equalIgnoreLoadStoreOptions(mRenderPassDesc))
    {
        FramebufferMtl *currentDrawFramebuffer =
            mtl::GetImpl(context->getState().getDrawFramebuffer());
        if (currentDrawFramebuffer == this)
        {
            contextMtl->onDrawFrameBufferChange(context, this);
        }
    }

    return angle::Result::Continue;
}

angle::Result FramebufferMtl::getSamplePosition(const gl::Context *context,
                                                size_t index,
                                                GLfloat *xy) const
{
    UNIMPLEMENTED();
    return angle::Result::Stop;
}

RenderTargetMtl *FramebufferMtl::getColorReadRenderTarget() const
{
    if (mState.getReadIndex() >= mColorRenderTargets.size())
    {
        return nullptr;
    }
    return mColorRenderTargets[mState.getReadIndex()];
}

gl::Rectangle FramebufferMtl::getCompleteRenderArea() const
{
    return gl::Rectangle(0, 0, mState.getDimensions().width, mState.getDimensions().height);
}

const mtl::RenderPassDesc &FramebufferMtl::getRenderPassDesc(ContextMtl *context)
{
    return mRenderPassDesc;
}

void FramebufferMtl::onFinishedDrawingToFrameBuffer(const gl::Context *context,
                                                    mtl::RenderCommandEncoder *encoder)
{
    if (!encoder->valid())
    {
        return;
    }

    ContextMtl *contextMtl = mtl::GetImpl(context);

    ASSERT(encoder->renderPassDesc().equalIgnoreLoadStoreOptions(mRenderPassDesc));

    for (uint32_t i = 0; i < mRenderPassDesc.numColorAttachments; ++i)
    {
        if (mDiscardColors[i])
        {
            encoder->setColorStoreAction(MTLStoreActionDontCare, i);
        }
    }
    encoder->setDepthStencilStoreAction(
        (mDiscardDepth || mAlwaysDiscardDepthStencil) ? MTLStoreActionDontCare
                                                      : MTLStoreActionStore,
        (mDiscardStencil || mAlwaysDiscardDepthStencil) ? MTLStoreActionDontCare
                                                        : MTLStoreActionStore);

    contextMtl->endEncoding(encoder);
}

angle::Result FramebufferMtl::updateColorRenderTarget(const gl::Context *context,
                                                      size_t colorIndexGL)
{
    return updateCachedRenderTarget(context, mState.getColorAttachment(colorIndexGL),
                                    &mColorRenderTargets[colorIndexGL]);
}

angle::Result FramebufferMtl::updateDepthRenderTarget(const gl::Context *context)
{
    return updateCachedRenderTarget(context, mState.getDepthAttachment(), &mDepthRenderTarget);
}

angle::Result FramebufferMtl::updateStencilRenderTarget(const gl::Context *context)
{
    return updateCachedRenderTarget(context, mState.getStencilAttachment(), &mStencilRenderTarget);
}

angle::Result FramebufferMtl::updateCachedRenderTarget(const gl::Context *context,
                                                       const gl::FramebufferAttachment *attachment,
                                                       RenderTargetMtl **cachedRenderTarget)
{
    RenderTargetMtl *newRenderTarget = nullptr;
    if (attachment)
    {
        ASSERT(attachment->isAttached());
        ANGLE_TRY(attachment->getRenderTarget(context, attachment->getRenderToTextureSamples(),
                                              &newRenderTarget));
    }
    *cachedRenderTarget = newRenderTarget;
    return angle::Result::Continue;
}

angle::Result FramebufferMtl::prepareRenderPass(const gl::Context *context,
                                                gl::DrawBufferMask drawColorBuffers,
                                                mtl::RenderPassDesc *pDescOut)
{
    auto &desc = *pDescOut;

    desc.numColorAttachments = static_cast<uint32_t>(drawColorBuffers.count());
    size_t attachmentIdx     = 0;

    for (size_t colorIndexGL : drawColorBuffers)
    {
        if (colorIndexGL >= mtl::kMaxRenderTargets)
        {
            continue;
        }
        const RenderTargetMtl *colorRenderTarget = mColorRenderTargets[colorIndexGL];
        ASSERT(colorRenderTarget);

        mtl::RenderPassColorAttachmentDesc &colorAttachment =
            desc.colorAttachments[attachmentIdx++];
        colorAttachment.reset();
        colorRenderTarget->toRenderPassAttachmentDesc(&colorAttachment);
    }

    if (mDepthRenderTarget)
    {
        desc.depthAttachment.reset();
        mDepthRenderTarget->toRenderPassAttachmentDesc(&desc.depthAttachment);
    }

    if (mStencilRenderTarget)
    {
        desc.stencilAttachment.reset();
        mStencilRenderTarget->toRenderPassAttachmentDesc(&desc.stencilAttachment);
    }

    return angle::Result::Continue;
}

// Override clear color based on texture's write mask
void FramebufferMtl::overrideClearColor(const mtl::TextureRef &texture,
                                        MTLClearColor clearColor,
                                        MTLClearColor *colorOut)
{
    *colorOut = mtl::EmulatedAlphaClearColor(clearColor, texture->getColorWritableMask());
}

angle::Result FramebufferMtl::clearWithLoadOp(const gl::Context *context,
                                              gl::DrawBufferMask clearColorBuffers,
                                              const mtl::ClearRectParams &clearOpts)
{
    ContextMtl *contextMtl             = mtl::GetImpl(context);
    bool startedRenderPass             = contextMtl->hasStartedRenderPass(mRenderPassDesc);
    mtl::RenderCommandEncoder *encoder = nullptr;
    mtl::RenderPassDesc tempDesc       = mRenderPassDesc;

    if (startedRenderPass)
    {
        encoder = contextMtl->getRenderCommandEncoder();
    }

    size_t attachmentCount = 0;
    for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
    {
        ASSERT(colorIndexGL < mtl::kMaxRenderTargets);

        uint32_t attachmentIdx = static_cast<uint32_t>(attachmentCount++);
        mtl::RenderPassColorAttachmentDesc &colorAttachment =
            tempDesc.colorAttachments[attachmentIdx];
        const mtl::TextureRef &texture = colorAttachment.texture;

        if (clearColorBuffers.test(colorIndexGL))
        {
            if (startedRenderPass)
            {
                encoder->setColorStoreAction(MTLStoreActionDontCare, attachmentIdx);
            }
            colorAttachment.loadAction = MTLLoadActionClear;
            overrideClearColor(texture, clearOpts.clearColor.value(), &colorAttachment.clearColor);
        }
        else
        {
            if (startedRenderPass)
            {
                encoder->setColorStoreAction(MTLStoreActionStore, attachmentIdx);
            }
            colorAttachment.loadAction = MTLLoadActionLoad;
        }
    }

    MTLStoreAction preClearDethpStoreAction, preClearStencilStoreAction;
    if (clearOpts.clearDepth.valid())
    {
        preClearDethpStoreAction            = MTLStoreActionDontCare;
        tempDesc.depthAttachment.loadAction = MTLLoadActionClear;
        tempDesc.depthAttachment.clearDepth = clearOpts.clearDepth.value();
    }
    else
    {
        preClearDethpStoreAction            = MTLStoreActionStore;
        tempDesc.depthAttachment.loadAction = MTLLoadActionLoad;
    }

    if (clearOpts.clearStencil.valid())
    {
        preClearStencilStoreAction              = MTLStoreActionDontCare;
        tempDesc.stencilAttachment.loadAction   = MTLLoadActionClear;
        tempDesc.stencilAttachment.clearStencil = clearOpts.clearStencil.value();
    }
    else
    {
        preClearStencilStoreAction            = MTLStoreActionStore;
        tempDesc.stencilAttachment.loadAction = MTLLoadActionLoad;
    }

    // End current render pass.
    if (startedRenderPass)
    {
        encoder->setDepthStencilStoreAction(preClearDethpStoreAction, preClearStencilStoreAction);
        contextMtl->endEncoding(encoder);
    }

    // Start new render encoder with loadOp=Clear
    contextMtl->getRenderCommandEncoder(tempDesc);

    return angle::Result::Continue;
}

angle::Result FramebufferMtl::clearWithDraw(const gl::Context *context,
                                            gl::DrawBufferMask clearColorBuffers,
                                            const mtl::ClearRectParams &clearOpts)
{
    ContextMtl *contextMtl = mtl::GetImpl(context);
    DisplayMtl *display    = contextMtl->getDisplay();
    mtl::RenderPassDesc rpDesc;
    ANGLE_TRY(prepareRenderPass(context, mState.getEnabledDrawBuffers(), &rpDesc));

    for (uint32_t i = 0, attachmentIdx = 0; i < rpDesc.numColorAttachments; ++i, ++attachmentIdx)
    {
        mtl::RenderPassColorAttachmentDesc &colorAttachment =
            rpDesc.colorAttachments[attachmentIdx];

        // Need to preserve previous content, since we only clear a portion of the framebuffer
        colorAttachment.loadAction = MTLLoadActionLoad;
    }

    if (rpDesc.depthAttachment.texture)
    {
        rpDesc.depthAttachment.loadAction = MTLLoadActionLoad;
    }

    if (rpDesc.stencilAttachment.texture)
    {
        rpDesc.stencilAttachment.loadAction = MTLLoadActionLoad;
    }

    // Start new render encoder with loadOp=Load
    mtl::RenderCommandEncoder *encoder = contextMtl->getRenderCommandEncoder(rpDesc);

    display->getUtils().clearWithDraw(context, encoder, clearOpts);

    return angle::Result::Continue;
}

angle::Result FramebufferMtl::clearImpl(const gl::Context *context,
                                        gl::DrawBufferMask clearColorBuffers,
                                        mtl::ClearRectParams *pClearOpts)
{
    auto &clearOpts = *pClearOpts;

    if (!clearOpts.clearColor.valid() && !clearOpts.clearDepth.valid() &&
        !clearOpts.clearStencil.valid())
    {
        // No Op.
        return angle::Result::Continue;
    }

    ContextMtl *contextMtl = mtl::GetImpl(context);
    const gl::Rectangle renderArea(0, 0, mState.getDimensions().width,
                                   mState.getDimensions().height);

    clearOpts.clearArea = ClipRectToScissor(contextMtl->getState(), renderArea, false);
    clearOpts.flipY     = mFlipY;

    // Discard clear altogether if scissor has 0 width or height.
    if (clearOpts.clearArea.width == 0 || clearOpts.clearArea.height == 0)
    {
        return angle::Result::Continue;
    }

    MTLColorWriteMask colorMask = contextMtl->getColorMask();
    uint32_t stencilMask        = contextMtl->getStencilMask();
    if (!contextMtl->isDepthWriteEnabled())
    {
        // Disable depth clearing, since depth write is disable
        clearOpts.clearDepth.reset();
    }

    if (clearOpts.clearArea == renderArea &&
        (!clearOpts.clearColor.valid() || colorMask == MTLColorWriteMaskAll) &&
        (!clearOpts.clearStencil.valid() ||
         (stencilMask & mtl::kStencilMaskAll) == mtl::kStencilMaskAll))
    {
        return clearWithLoadOp(context, clearColorBuffers, clearOpts);
    }

    return clearWithDraw(context, clearColorBuffers, clearOpts);
}

angle::Result FramebufferMtl::invalidateImpl(ContextMtl *contextMtl,
                                             size_t count,
                                             const GLenum *attachments)
{
    gl::DrawBufferMask invalidateColorBuffers;
    bool invalidateDepthBuffer   = false;
    bool invalidateStencilBuffer = false;

    for (size_t i = 0; i < count; ++i)
    {
        const GLenum attachment = attachments[i];

        switch (attachment)
        {
            case GL_DEPTH:
            case GL_DEPTH_ATTACHMENT:
                invalidateDepthBuffer = true;
                break;
            case GL_STENCIL:
            case GL_STENCIL_ATTACHMENT:
                invalidateStencilBuffer = true;
                break;
            case GL_DEPTH_STENCIL_ATTACHMENT:
                invalidateDepthBuffer   = true;
                invalidateStencilBuffer = true;
                break;
            default:
                ASSERT(
                    (attachment >= GL_COLOR_ATTACHMENT0 && attachment <= GL_COLOR_ATTACHMENT15) ||
                    (attachment == GL_COLOR));

                invalidateColorBuffers.set(
                    attachment == GL_COLOR ? 0u : (attachment - GL_COLOR_ATTACHMENT0));
        }
    }

    // Set the appropriate storeOp for attachments.
    size_t attachmentIdx = 0;
    for (size_t colorIndexGL : mState.getEnabledDrawBuffers())
    {
        if (mColorRenderTargets[colorIndexGL] && invalidateColorBuffers.test(colorIndexGL))
        {
            mDiscardColors[attachmentIdx] = true;
        }
        ++attachmentIdx;
    }

    if (invalidateDepthBuffer && mDepthRenderTarget)
    {
        mDiscardDepth = true;
    }

    if (invalidateStencilBuffer && mStencilRenderTarget)
    {
        mDiscardStencil = true;
    }

    return angle::Result::Continue;
}

gl::Rectangle FramebufferMtl::getReadPixelArea(const gl::Rectangle &glArea)
{
    RenderTargetMtl *colorReadRT = getColorReadRenderTarget();
    ASSERT(colorReadRT);
    gl::Rectangle flippedArea = glArea;
    if (mFlipY)
    {
        flippedArea.y =
            colorReadRT->getTexture()->height(static_cast<uint32_t>(colorReadRT->getLevelIndex())) -
            flippedArea.y - flippedArea.height;
    }

    return flippedArea;
}

angle::Result FramebufferMtl::readPixelsImpl(const gl::Context *context,
                                             const gl::Rectangle &area,
                                             const PackPixelsParams &packPixelsParams,
                                             RenderTargetMtl *renderTarget,
                                             uint8_t *pixels)
{
    ContextMtl *contextMtl = mtl::GetImpl(context);
    if (packPixelsParams.packBuffer)
    {
        // NOTE(hqle): PBO is not supported atm
        ANGLE_MTL_CHECK(contextMtl, false, GL_INVALID_OPERATION);
    }
    if (!renderTarget)
    {
        return angle::Result::Continue;
    }
    const mtl::TextureRef &texture = renderTarget->getTexture();

    if (!texture)
    {
        return angle::Result::Continue;
    }

    const mtl::Format &readFormat        = *renderTarget->getFormat();
    const angle::Format &readAngleFormat = readFormat.actualAngleFormat();

    // NOTE(hqle): resolve MSAA texture before readback
    int srcRowPitch = area.width * readAngleFormat.pixelBytes;
    angle::MemoryBuffer readPixelRowBuffer;
    ANGLE_CHECK_GL_ALLOC(contextMtl, readPixelRowBuffer.resize(srcRowPitch));

    auto packPixelsRowParams  = packPixelsParams;
    MTLRegion mtlSrcRowRegion = MTLRegionMake2D(area.x, area.y, area.width, 1);

    int rowOffset = packPixelsParams.reverseRowOrder ? -1 : 1;
    int startRow  = packPixelsParams.reverseRowOrder ? (area.y1() - 1) : area.y;

    // Copy pixels row by row
    packPixelsRowParams.area.height     = 1;
    packPixelsRowParams.reverseRowOrder = false;
    for (int r = startRow, i = 0; i < area.height;
         ++i, r += rowOffset, pixels += packPixelsRowParams.outputPitch)
    {
        mtlSrcRowRegion.origin.y   = r;
        packPixelsRowParams.area.y = packPixelsParams.area.y + i;

        // Read the pixels data to the row buffer
        texture->getBytes(contextMtl, srcRowPitch, mtlSrcRowRegion,
                          static_cast<uint32_t>(renderTarget->getLevelIndex()),
                          readPixelRowBuffer.data());

        // Convert to destination format
        PackPixels(packPixelsRowParams, readAngleFormat, srcRowPitch, readPixelRowBuffer.data(),
                   pixels);
    }

    return angle::Result::Continue;
}
}

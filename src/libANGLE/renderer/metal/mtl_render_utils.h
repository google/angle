//
// Copyright 2019 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// mtl_render_utils.h:
//    Defines the class interface for RenderUtils, which contains many utility functions and shaders
//    for converting, blitting, copying as well as generating data, and many more.
//

#ifndef LIBANGLE_RENDERER_METAL_MTL_RENDER_UTILS_H_
#define LIBANGLE_RENDERER_METAL_MTL_RENDER_UTILS_H_

#import <Metal/Metal.h>

#include "libANGLE/angletypes.h"
#include "libANGLE/renderer/metal/RenderTargetMtl.h"
#include "libANGLE/renderer/metal/mtl_command_buffer.h"
#include "libANGLE/renderer/metal/mtl_state_cache.h"
#include "libANGLE/renderer/metal/shaders/constants.h"

namespace rx
{

class BufferMtl;
class ContextMtl;
class DisplayMtl;

namespace mtl
{
struct ClearRectParams : public ClearOptions
{
    gl::Extents dstTextureSize;

    // Only clear enabled buffers
    gl::DrawBufferMask enabledBuffers;
    gl::Rectangle clearArea;

    bool flipY = false;
};

struct BlitParams
{
    gl::Extents dstTextureSize;
    gl::Rectangle dstRect;
    gl::Rectangle dstScissorRect;
    // Destination texture needs to have viewport Y flipped?
    // The difference between this param and unpackFlipY is that unpackFlipY is from
    // glCopyImageCHROMIUM()/glBlitFramebuffer(), and dstFlipY controls whether the final viewport
    // needs to be flipped when drawing to destination texture. It is possible to combine the two
    // flags before passing to RenderUtils. However, to avoid duplicated works, just pass the two
    // flags to RenderUtils, they will be combined internally by RenderUtils logic.
    bool dstFlipY = false;
    bool dstFlipX = false;

    TextureRef src;
    uint32_t srcLevel = 0;
    uint32_t srcLayer = 0;

    // Source rectangle:
    // NOTE: if srcYFlipped=true, this rectangle will be converted internally to flipped rect before
    // blitting.
    gl::Rectangle srcRect;

    bool srcYFlipped = false;  // source texture has data flipped in Y direction
    bool unpackFlipX = false;  // flip texture data copying process in X direction
    bool unpackFlipY = false;  // flip texture data copying process in Y direction
};

struct ColorBlitParams : public BlitParams
{
    MTLColorWriteMask blitColorMask = MTLColorWriteMaskAll;
    gl::DrawBufferMask enabledBuffers;
    GLenum filter               = GL_NEAREST;
    bool unpackPremultiplyAlpha = false;
    bool unpackUnmultiplyAlpha  = false;
    bool dstLuminance           = false;
};

struct DepthStencilBlitParams : public BlitParams
{
    TextureRef srcStencil;
};

// Stencil blit via an intermediate buffer. NOTE: source depth texture parameter is ignored.
// See DepthStencilBlitUtils::blitStencilViaCopyBuffer()
struct StencilBlitViaBufferParams : public DepthStencilBlitParams
{
    StencilBlitViaBufferParams();
    StencilBlitViaBufferParams(const DepthStencilBlitParams &src);

    TextureRef dstStencil;
    uint32_t dstStencilLevel         = 0;
    uint32_t dstStencilLayer         = 0;
    bool dstPackedDepthStencilFormat = false;
};

struct TriFanFromArrayParams
{
    uint32_t firstVertex;
    uint32_t vertexCount;
    BufferRef dstBuffer;
    // Must be multiples of kIndexBufferOffsetAlignment
    uint32_t dstOffset;
};

struct IndexConversionParams
{

    gl::DrawElementsType srcType;
    uint32_t indexCount;
    const BufferRef &srcBuffer;
    uint32_t srcOffset;
    const BufferRef &dstBuffer;
    // Must be multiples of kIndexBufferOffsetAlignment
    uint32_t dstOffset;
};

struct IndexGenerationParams
{
    gl::DrawElementsType srcType;
    GLsizei indexCount;
    const void *indices;
    BufferRef dstBuffer;
    uint32_t dstOffset;
};

// Utils class for clear & blitting
class ClearUtils
{
  public:
    ClearUtils();

    void onDestroy();

    // Clear current framebuffer
    angle::Result clearWithDraw(const gl::Context *context,
                                RenderCommandEncoder *cmdEncoder,
                                const ClearRectParams &params);

  private:
    void ensureRenderPipelineStateCacheInitialized(ContextMtl *ctx, uint32_t numColorAttachments);

    void setupClearWithDraw(const gl::Context *context,
                            RenderCommandEncoder *cmdEncoder,
                            const ClearRectParams &params);
    id<MTLDepthStencilState> getClearDepthStencilState(const gl::Context *context,
                                                       const ClearRectParams &params);
    id<MTLRenderPipelineState> getClearRenderPipelineState(const gl::Context *context,
                                                           RenderCommandEncoder *cmdEncoder,
                                                           const ClearRectParams &params);

    // Render pipeline cache for clear with draw:
    std::array<RenderPipelineCache, kMaxRenderTargets + 1> mClearRenderPipelineCache;
};

class ColorBlitUtils
{
  public:
    ColorBlitUtils();

    void onDestroy();

    // Blit texture data to current framebuffer
    angle::Result blitColorWithDraw(const gl::Context *context,
                                    RenderCommandEncoder *cmdEncoder,
                                    const ColorBlitParams &params);

  private:
    void ensureRenderPipelineStateCacheInitialized(ContextMtl *ctx,
                                                   uint32_t numColorAttachments,
                                                   int alphaPremultiplyType,
                                                   int sourceTextureType,
                                                   RenderPipelineCache *cacheOut);

    void setupColorBlitWithDraw(const gl::Context *context,
                                RenderCommandEncoder *cmdEncoder,
                                const ColorBlitParams &params);

    id<MTLRenderPipelineState> getColorBlitRenderPipelineState(const gl::Context *context,
                                                               RenderCommandEncoder *cmdEncoder,
                                                               const ColorBlitParams &params);

    // Blit with draw pipeline caches:
    // First array dimension: number of outputs.
    // Second array dimension: source texture type (2d, ms, array, 3d, etc)
    using ColorBlitRenderPipelineCacheArray =
        std::array<std::array<RenderPipelineCache, mtl_shader::kTextureTypeCount>,
                   kMaxRenderTargets>;
    ColorBlitRenderPipelineCacheArray mBlitRenderPipelineCache;
    ColorBlitRenderPipelineCacheArray mBlitPremultiplyAlphaRenderPipelineCache;
    ColorBlitRenderPipelineCacheArray mBlitUnmultiplyAlphaRenderPipelineCache;
};

class DepthStencilBlitUtils
{
  public:
    void onDestroy();

    angle::Result blitDepthStencilWithDraw(const gl::Context *context,
                                           RenderCommandEncoder *cmdEncoder,
                                           const DepthStencilBlitParams &params);
    // Blit stencil data using intermediate buffer. This function is used on devices with no
    // support for direct stencil write in shader. Thus an intermediate buffer storing copied
    // stencil data is needed.
    // NOTE: this function shares the params struct with depth & stencil blit, but depth texture
    // parameter is not used. This function will break existing render pass.
    angle::Result blitStencilViaCopyBuffer(const gl::Context *context,
                                           const StencilBlitViaBufferParams &params);

  private:
    void ensureRenderPipelineStateCacheInitialized(ContextMtl *ctx,
                                                   int sourceDepthTextureType,
                                                   int sourceStencilTextureType,
                                                   RenderPipelineCache *cacheOut);

    void setupDepthStencilBlitWithDraw(const gl::Context *context,
                                       RenderCommandEncoder *cmdEncoder,
                                       const DepthStencilBlitParams &params);

    id<MTLRenderPipelineState> getDepthStencilBlitRenderPipelineState(
        const gl::Context *context,
        RenderCommandEncoder *cmdEncoder,
        const DepthStencilBlitParams &params);

    id<MTLComputePipelineState> getStencilToBufferComputePipelineState(
        ContextMtl *ctx,
        const StencilBlitViaBufferParams &params);

    std::array<RenderPipelineCache, mtl_shader::kTextureTypeCount> mDepthBlitRenderPipelineCache;
    std::array<RenderPipelineCache, mtl_shader::kTextureTypeCount> mStencilBlitRenderPipelineCache;
    std::array<std::array<RenderPipelineCache, mtl_shader::kTextureTypeCount>,
               mtl_shader::kTextureTypeCount>
        mDepthStencilBlitRenderPipelineCache;

    std::array<AutoObjCPtr<id<MTLComputePipelineState>>, mtl_shader::kTextureTypeCount>
        mStencilBlitToBufferComPipelineCache;

    // Intermediate buffer for storing copied stencil data. Used when device doesn't support
    // writing stencil in shader.
    BufferRef mStencilCopyBuffer;
};

// util class for generating index buffer
class IndexGeneratorUtils
{
  public:
    void onDestroy();

    // Convert index buffer.
    angle::Result convertIndexBufferGPU(ContextMtl *contextMtl,
                                        const IndexConversionParams &params);
    // Generate triangle fan index buffer for glDrawArrays().
    angle::Result generateTriFanBufferFromArrays(ContextMtl *contextMtl,
                                                 const TriFanFromArrayParams &params);
    // Generate triangle fan index buffer for glDrawElements().
    angle::Result generateTriFanBufferFromElementsArray(ContextMtl *contextMtl,
                                                        const IndexGenerationParams &params);

    // Generate line loop's last segment index buffer for glDrawArrays().
    angle::Result generateLineLoopLastSegment(ContextMtl *contextMtl,
                                              uint32_t firstVertex,
                                              uint32_t lastVertex,
                                              const BufferRef &dstBuffer,
                                              uint32_t dstOffset);
    // Generate line loop's last segment index buffer for glDrawElements().
    angle::Result generateLineLoopLastSegmentFromElementsArray(ContextMtl *contextMtl,
                                                               const IndexGenerationParams &params);

  private:
    // Index generator compute pipelines:
    //  - First dimension: index type.
    //  - second dimension: source buffer's offset is aligned or not.
    using IndexConversionPipelineArray =
        std::array<std::array<AutoObjCPtr<id<MTLComputePipelineState>>, 2>,
                   angle::EnumSize<gl::DrawElementsType>()>;

    // Get compute pipeline to convert index between buffers.
    AutoObjCPtr<id<MTLComputePipelineState>> getIndexConversionPipeline(
        ContextMtl *contextMtl,
        gl::DrawElementsType srcType,
        uint32_t srcOffset);
    // Get compute pipeline to generate tri fan index for glDrawElements().
    AutoObjCPtr<id<MTLComputePipelineState>> getTriFanFromElemArrayGeneratorPipeline(
        ContextMtl *contextMtl,
        gl::DrawElementsType srcType,
        uint32_t srcOffset);
    // Defer loading of compute pipeline to generate tri fan index for glDrawArrays().
    void ensureTriFanFromArrayGeneratorInitialized(ContextMtl *contextMtl);

    angle::Result generateTriFanBufferFromElementsArrayGPU(
        ContextMtl *contextMtl,
        gl::DrawElementsType srcType,
        uint32_t indexCount,
        const BufferRef &srcBuffer,
        uint32_t srcOffset,
        const BufferRef &dstBuffer,
        // Must be multiples of kIndexBufferOffsetAlignment
        uint32_t dstOffset);
    angle::Result generateTriFanBufferFromElementsArrayCPU(ContextMtl *contextMtl,
                                                           const IndexGenerationParams &params);

    angle::Result generateLineLoopLastSegmentFromElementsArrayCPU(
        ContextMtl *contextMtl,
        const IndexGenerationParams &params);

    IndexConversionPipelineArray mIndexConversionPipelineCaches;

    IndexConversionPipelineArray mTriFanFromElemArrayGeneratorPipelineCaches;
    AutoObjCPtr<id<MTLComputePipelineState>> mTriFanFromArraysGeneratorPipeline;
};

// RenderUtils: container class of various util classes above
class RenderUtils : public Context, angle::NonCopyable
{
  public:
    RenderUtils(DisplayMtl *display);
    ~RenderUtils() override;

    angle::Result initialize();
    void onDestroy();

    // Clear current framebuffer
    angle::Result clearWithDraw(const gl::Context *context,
                                RenderCommandEncoder *cmdEncoder,
                                const ClearRectParams &params);
    // Blit texture data to current framebuffer
    angle::Result blitColorWithDraw(const gl::Context *context,
                                    RenderCommandEncoder *cmdEncoder,
                                    const ColorBlitParams &params);
    // Same as above but blit the whole texture to the whole of current framebuffer.
    // This function assumes the framebuffer and the source texture have same size.
    angle::Result blitColorWithDraw(const gl::Context *context,
                                    RenderCommandEncoder *cmdEncoder,
                                    const TextureRef &srcTexture);

    angle::Result blitDepthStencilWithDraw(const gl::Context *context,
                                           RenderCommandEncoder *cmdEncoder,
                                           const DepthStencilBlitParams &params);
    // See DepthStencilBlitUtils::blitStencilViaCopyBuffer()
    angle::Result blitStencilViaCopyBuffer(const gl::Context *context,
                                           const StencilBlitViaBufferParams &params);

    // See IndexGeneratorUtils
    angle::Result convertIndexBufferGPU(ContextMtl *contextMtl,
                                        const IndexConversionParams &params);
    angle::Result generateTriFanBufferFromArrays(ContextMtl *contextMtl,
                                                 const TriFanFromArrayParams &params);
    angle::Result generateTriFanBufferFromElementsArray(ContextMtl *contextMtl,
                                                        const IndexGenerationParams &params);
    angle::Result generateLineLoopLastSegment(ContextMtl *contextMtl,
                                              uint32_t firstVertex,
                                              uint32_t lastVertex,
                                              const BufferRef &dstBuffer,
                                              uint32_t dstOffset);
    angle::Result generateLineLoopLastSegmentFromElementsArray(ContextMtl *contextMtl,
                                                               const IndexGenerationParams &params);

  private:
    // override ErrorHandler
    void handleError(GLenum error,
                     const char *file,
                     const char *function,
                     unsigned int line) override;
    void handleError(NSError *_Nullable error,
                     const char *file,
                     const char *function,
                     unsigned int line) override;

    ClearUtils mClearUtils;
    ColorBlitUtils mColorBlitUtils;
    DepthStencilBlitUtils mDepthStencilBlitUtils;
    IndexGeneratorUtils mIndexUtils;
};

}  // namespace mtl
}  // namespace rx

#endif /* LIBANGLE_RENDERER_METAL_MTL_RENDER_UTILS_H_ */

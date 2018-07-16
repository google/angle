//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ContextVk.h:
//    Defines the class interface for ContextVk, implementing ContextImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_
#define LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_

#include <vulkan/vulkan.h>

#include "libANGLE/renderer/ContextImpl.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
struct FeaturesVk;
class RendererVk;

class ContextVk : public ContextImpl, public vk::Context
{
  public:
    ContextVk(const gl::ContextState &state, RendererVk *renderer);
    ~ContextVk() override;

    gl::Error initialize() override;

    void onDestroy(const gl::Context *context) override;

    // Flush and finish.
    gl::Error flush(const gl::Context *context) override;
    gl::Error finish(const gl::Context *context) override;

    // Drawing methods.
    gl::Error drawArrays(const gl::Context *context,
                         gl::PrimitiveMode mode,
                         GLint first,
                         GLsizei count) override;
    gl::Error drawArraysInstanced(const gl::Context *context,
                                  gl::PrimitiveMode mode,
                                  GLint first,
                                  GLsizei count,
                                  GLsizei instanceCount) override;

    gl::Error drawElements(const gl::Context *context,
                           gl::PrimitiveMode mode,
                           GLsizei count,
                           GLenum type,
                           const void *indices) override;
    gl::Error drawElementsInstanced(const gl::Context *context,
                                    gl::PrimitiveMode mode,
                                    GLsizei count,
                                    GLenum type,
                                    const void *indices,
                                    GLsizei instances) override;
    gl::Error drawRangeElements(const gl::Context *context,
                                gl::PrimitiveMode mode,
                                GLuint start,
                                GLuint end,
                                GLsizei count,
                                GLenum type,
                                const void *indices) override;
    gl::Error drawArraysIndirect(const gl::Context *context,
                                 gl::PrimitiveMode mode,
                                 const void *indirect) override;
    gl::Error drawElementsIndirect(const gl::Context *context,
                                   gl::PrimitiveMode mode,
                                   GLenum type,
                                   const void *indirect) override;

    // Device loss
    GLenum getResetStatus() override;

    // Vendor and description strings.
    std::string getVendorString() const override;
    std::string getRendererDescription() const override;

    // EXT_debug_marker
    void insertEventMarker(GLsizei length, const char *marker) override;
    void pushGroupMarker(GLsizei length, const char *marker) override;
    void popGroupMarker() override;

    // KHR_debug
    void pushDebugGroup(GLenum source, GLuint id, GLsizei length, const char *message) override;
    void popDebugGroup() override;

    bool isViewportFlipEnabledForDrawFBO() const;
    bool isViewportFlipEnabledForReadFBO() const;

    // State sync with dirty bits.
    gl::Error syncState(const gl::Context *context, const gl::State::DirtyBits &dirtyBits) override;

    // Disjoint timer queries
    GLint getGPUDisjoint() override;
    GLint64 getTimestamp() override;

    // Context switching
    gl::Error onMakeCurrent(const gl::Context *context) override;

    // Native capabilities, unmodified by gl::Context.
    gl::Caps getNativeCaps() const override;
    const gl::TextureCapsMap &getNativeTextureCaps() const override;
    const gl::Extensions &getNativeExtensions() const override;
    const gl::Limitations &getNativeLimitations() const override;

    // Shader creation
    CompilerImpl *createCompiler() override;
    ShaderImpl *createShader(const gl::ShaderState &state) override;
    ProgramImpl *createProgram(const gl::ProgramState &state) override;

    // Framebuffer creation
    FramebufferImpl *createFramebuffer(const gl::FramebufferState &state) override;

    // Texture creation
    TextureImpl *createTexture(const gl::TextureState &state) override;

    // Renderbuffer creation
    RenderbufferImpl *createRenderbuffer(const gl::RenderbufferState &state) override;

    // Buffer creation
    BufferImpl *createBuffer(const gl::BufferState &state) override;

    // Vertex Array creation
    VertexArrayImpl *createVertexArray(const gl::VertexArrayState &state) override;

    // Query and Fence creation
    QueryImpl *createQuery(gl::QueryType type) override;
    FenceNVImpl *createFenceNV() override;
    SyncImpl *createSync() override;

    // Transform Feedback creation
    TransformFeedbackImpl *createTransformFeedback(
        const gl::TransformFeedbackState &state) override;

    // Sampler object creation
    SamplerImpl *createSampler(const gl::SamplerState &state) override;

    // Program Pipeline object creation
    ProgramPipelineImpl *createProgramPipeline(const gl::ProgramPipelineState &data) override;

    // Path object creation
    std::vector<PathImpl *> createPaths(GLsizei) override;

    gl::Error dispatchCompute(const gl::Context *context,
                              GLuint numGroupsX,
                              GLuint numGroupsY,
                              GLuint numGroupsZ) override;
    gl::Error dispatchComputeIndirect(const gl::Context *context, GLintptr indirect) override;

    gl::Error memoryBarrier(const gl::Context *context, GLbitfield barriers) override;
    gl::Error memoryBarrierByRegion(const gl::Context *context, GLbitfield barriers) override;

    VkDevice getDevice() const;
    const FeaturesVk &getFeatures() const;

    void invalidateCurrentPipeline();
    void invalidateDefaultAttribute(size_t attribIndex);

    vk::DynamicDescriptorPool *getDynamicDescriptorPool(uint32_t descriptorSetIndex);

    const VkClearValue &getClearColorValue() const;
    const VkClearValue &getClearDepthStencilValue() const;
    VkColorComponentFlags getClearColorMask() const;
    const VkRect2D &getScissor() const { return mPipelineDesc->getScissor(); }
    gl::Error getIncompleteTexture(const gl::Context *context,
                                   gl::TextureType type,
                                   gl::Texture **textureOut);
    void updateColorMask(const gl::BlendState &blendState);

    void handleError(VkResult errorCode, const char *file, unsigned int line) override;
    const gl::ActiveTextureArray<TextureVk *> &getActiveTextures() const;

  private:
    gl::Error initPipeline(const gl::DrawCallParams &drawCallParams);
    gl::Error setupDraw(const gl::Context *context,
                        const gl::DrawCallParams &drawCallParams,
                        vk::CommandBuffer **commandBufferOut,
                        bool *shouldApplyVertexArrayOut);

    void updateScissor(const gl::State &glState) const;
    void updateFlipViewportDrawFramebuffer(const gl::State &glState);
    void updateFlipViewportReadFramebuffer(const gl::State &glState);

    angle::Result updateDriverUniforms(const gl::State &glState);
    gl::Error updateActiveTextures(const gl::Context *context);
    angle::Result updateDefaultAttributes();
    angle::Result updateDefaultAttribute(size_t attribIndex);

    vk::PipelineAndSerial *mCurrentPipeline;
    gl::PrimitiveMode mCurrentDrawMode;

    // Keep a cached pipeline description structure that can be used to query the pipeline cache.
    // Kept in a pointer so allocations can be aligned, and structs can be portably packed.
    std::unique_ptr<vk::PipelineDesc> mPipelineDesc;

    // The descriptor pools are externally sychronized, so cannot be accessed from different
    // threads simultaneously. Hence, we keep them in the ContextVk instead of the RendererVk.
    vk::DescriptorSetLayoutArray<vk::DynamicDescriptorPool> mDynamicDescriptorPools;

    // Triggers adding dependencies to the command graph.
    bool mTexturesDirty;
    bool mVertexArrayBindingHasChanged;

    // Cached clear value/mask for color and depth/stencil.
    VkClearValue mClearColorValue;
    VkClearValue mClearDepthStencilValue;
    VkColorComponentFlags mClearColorMask;

    IncompleteTextureSet mIncompleteTextures;

    // If the current surface bound to this context wants to have all rendering flipped vertically.
    // Updated on calls to onMakeCurrent.
    bool mFlipYForCurrentSurface;
    bool mFlipViewportForDrawFramebuffer;
    bool mFlipViewportForReadFramebuffer;

    // For shader uniforms such as gl_DepthRange and the viewport size.
    struct DriverUniforms
    {
        std::array<float, 4> viewport;
        std::array<float, 4> viewportScaleFactor;

        // We'll use x, y, z for near / far / diff respectively.
        std::array<float, 4> depthRange;
    };
    vk::DynamicBuffer mDriverUniformsBuffer;
    VkDescriptorSet mDriverUniformsDescriptorSet;
    vk::BindingPointer<vk::DescriptorSetLayout> mDriverUniformsSetLayout;

    // This cache should also probably include the texture index (shader location) and array
    // index (also in the shader). This info is used in the descriptor update step.
    gl::ActiveTextureArray<TextureVk *> mActiveTextures;

    // "Current Value" aka default vertex attribute state.
    gl::AttributesMask mDirtyDefaultAttribs;
    gl::AttribArray<vk::DynamicBuffer> mDefaultAttribBuffers;
};
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_CONTEXTVK_H_

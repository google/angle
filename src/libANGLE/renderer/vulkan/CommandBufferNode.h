//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandBufferNode:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#ifndef LIBANGLE_RENDERER_VULKAN_COMMAND_BUFFER_NODE_H_
#define LIBANGLE_RENDERER_VULKAN_COMMAND_BUFFER_NODE_H_

#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{

namespace vk
{

enum class VisitedState
{
    Unvisited,
    Ready,
    Visited,
};

class CommandBufferNode final : angle::NonCopyable
{
  public:
    CommandBufferNode();
    ~CommandBufferNode();

    // Immutable queries for when we're walking the commands tree.
    CommandBuffer *getOutsideRenderPassCommands();
    CommandBuffer *getInsideRenderPassCommands();

    // For outside the render pass (copies, transitions, etc).
    Error startRecording(VkDevice device,
                         const CommandPool &commandPool,
                         CommandBuffer **commandsOut);

    // For rendering commands (draws).
    Error startRenderPassRecording(RendererVk *renderer, CommandBuffer **commandsOut);

    bool isFinishedRecording() const;
    void finishRecording();

    // Commands for storing info relevant to the RenderPass.
    // RenderTargets must be added in order, with the depth/stencil being added last.
    void storeRenderPassInfo(const Framebuffer &framebuffer,
                             const gl::Rectangle renderArea,
                             const std::vector<VkClearValue> &clearValues);
    void appendColorRenderTarget(Serial serial, RenderTargetVk *colorRenderTarget);
    void appendDepthStencilRenderTarget(Serial serial, RenderTargetVk *depthStencilRenderTarget);

    // Commands for linking nodes in the dependency graph.
    static void SetHappensBeforeDependency(CommandBufferNode *beforeNode,
                                           CommandBufferNode *afterNode);
    static void SetHappensBeforeDependencies(const std::vector<CommandBufferNode *> &beforeNodes,
                                             CommandBufferNode *afterNode);
    bool hasHappensBeforeDependencies() const;
    bool hasHappensAfterDependencies() const;

    // Used for testing only.
    bool happensAfter(CommandBufferNode *beforeNode);

    // Commands for traversing the node on a flush operation.
    VisitedState visitedState() const;
    void visitDependencies(std::vector<CommandBufferNode *> *stack);
    Error visitAndExecute(RendererVk *renderer, CommandBuffer *primaryCommandBuffer);

  private:
    void initAttachmentDesc(VkAttachmentDescription *desc);
    void setHasHappensAfterDependencies();

    // Only used if we need a RenderPass for these commands.
    RenderPassDesc mRenderPassDesc;
    Framebuffer mRenderPassFramebuffer;
    gl::Rectangle mRenderPassRenderArea;
    gl::AttachmentArray<VkClearValue> mRenderPassClearValues;

    // Keep a separate buffers for commands inside and outside a RenderPass.
    // TODO(jmadill): We might not need inside and outside RenderPass commands separate.
    CommandBuffer mOutsideRenderPassCommands;
    CommandBuffer mInsideRenderPassCommands;

    // These commands must be submitted before 'this' command can be submitted correctly.
    std::vector<CommandBufferNode *> mHappensBeforeDependencies;

    // If this is true, other commands exist that must be submitted after 'this' command.
    bool mHasHappensAfterDependencies;

    // Used when traversing the dependency graph.
    VisitedState mVisitedState;

    // Is recording currently enabled?
    bool mIsFinishedRecording;
};
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_BUFFER_NODE_H_

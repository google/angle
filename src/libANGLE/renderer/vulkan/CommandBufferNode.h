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

#include "libANGLE/renderer/vulkan/renderervk_utils.h"

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

    // Commands for storing info relevant to the RenderPass.
    // RenderTargets must be added in order, with the depth/stencil being added last.
    void storeRenderPassInfo(const Framebuffer &framebuffer,
                             const gl::Rectangle renderArea,
                             const std::vector<VkClearValue> &clearValues);
    void appendColorRenderTarget(Serial serial, RenderTargetVk *colorRenderTarget);
    void appendDepthStencilRenderTarget(Serial serial, RenderTargetVk *depthStencilRenderTarget);

    // Commands for linking nodes in the dependency graph.
    void addDependency(CommandBufferNode *node);
    void addDependencies(const std::vector<CommandBufferNode *> &nodes);
    bool hasDependencies() const;
    bool isDependency() const;

    // Used for testing only.
    bool hasDependency(CommandBufferNode *searchNode);

    // Commands for traversing the node on a flush operation.
    VisitedState visitedState() const;
    void visitDependencies(std::vector<CommandBufferNode *> *stack);
    Error visitAndExecute(RendererVk *renderer, CommandBuffer *primaryCommandBuffer);

  private:
    void initAttachmentDesc(VkAttachmentDescription *desc);
    void markAsDependency();

    // Only used if we need a RenderPass for these commands.
    RenderPassDesc mRenderPassDesc;
    Framebuffer mRenderPassFramebuffer;
    gl::Rectangle mRenderPassRenderArea;
    gl::AttachmentArray<VkClearValue> mRenderPassClearValues;

    // Keep a separate buffers for commands inside and outside a RenderPass.
    // TODO(jmadill): We might not need inside and outside RenderPass commands separate.
    CommandBuffer mOutsideRenderPassCommands;
    CommandBuffer mInsideRenderPassCommands;

    // Dependency commands must finish before these command can execute.
    std::vector<CommandBufferNode *> mDependencies;
    bool mIsDependency;

    // Used when traversing the dependency graph.
    VisitedState mVisitedState;
};
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_BUFFER_NODE_H_

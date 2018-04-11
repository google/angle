//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandGraph:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#ifndef LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_
#define LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_

#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

namespace rx
{

namespace vk
{

// This is a helper class for back-end objects used in Vk command buffers. It records a serial
// at command recording times indicating an order in the queue. We use Fences to detect when
// commands finish, and then release any unreferenced and deleted resources based on the stored
// queue serial in a special 'garbage' queue. Resources also track current read and write
// dependencies. Only one command buffer node can be writing to the Resource at a time, but many
// can be reading from it. Together the dependencies will form a command graph at submission time.
class CommandGraphResource
{
  public:
    CommandGraphResource();
    virtual ~CommandGraphResource();

    void updateQueueSerial(Serial queueSerial);
    Serial getQueueSerial() const;

    // Returns true if this node has a current writing node with no children.
    bool hasChildlessWritingNode() const;

    // Returns the active write node.
    CommandGraphNode *getCurrentWritingNode();

    // Allocates a new write node and calls onWriteResource internally.
    CommandGraphNode *getNewWritingNode(RendererVk *renderer);

    // Allocates a write node via getNewWriteNode and returns a started command buffer.
    // The started command buffer will render outside of a RenderPass.
    Error beginWriteResource(RendererVk *renderer, CommandBuffer **commandBufferOut);

    // Sets up dependency relations. 'writingNode' will modify 'this' ResourceVk.
    void onWriteResource(CommandGraphNode *writingNode, Serial serial);

    // Sets up dependency relations. 'readingNode' will read from 'this' ResourceVk.
    void onReadResource(CommandGraphNode *readingNode, Serial serial);

    // Returns false if the resource is not in use, and clears any current read/write nodes.
    bool checkResourceInUseAndRefreshDeps(RendererVk *renderer);

  private:
    Serial mStoredQueueSerial;
    std::vector<CommandGraphNode *> mCurrentReadingNodes;
    CommandGraphNode *mCurrentWritingNode;
};

enum class VisitedState
{
    Unvisited,
    Ready,
    Visited,
};

// Translating OpenGL commands into Vulkan and submitting them immediately loses out on some
// of the powerful flexiblity Vulkan offers in RenderPasses. Load/Store ops can automatically
// clear RenderPass attachments, or preserve the contents. RenderPass automatic layout transitions
// can improve certain performance cases. Also, we can remove redundant RenderPass Begin and Ends
// when processing interleaved draw operations on independent Framebuffers.
//
// ANGLE's CommandGraph (and CommandGraphNode) attempt to solve these problems using deferred
// command submission. We also sometimes call this command re-ordering. A brief summary:
//
// During GL command processing, we record Vulkan commands into secondary command buffers, which
// are stored in CommandGraphNodes, and these nodes are chained together via dependencies to
// for a directed acyclic CommandGraph. When we need to submit the CommandGraph, say during a
// SwapBuffers or ReadPixels call, we begin a primary Vulkan CommandBuffer, and walk the
// CommandGraph, starting at the most senior nodes, recording secondary CommandBuffers inside
// and outside RenderPasses as necessary, filled with the right load/store operations. Once
// the primary CommandBuffer has recorded all of the secondary CommandBuffers from all the open
// CommandGraphNodes, we submit the primary CommandBuffer to the VkQueue on the device.

class CommandGraphNode final : angle::NonCopyable
{
  public:
    CommandGraphNode();
    ~CommandGraphNode();

    // Immutable queries for when we're walking the commands tree.
    CommandBuffer *getOutsideRenderPassCommands();
    CommandBuffer *getInsideRenderPassCommands();

    // For outside the render pass (copies, transitions, etc).
    Error beginOutsideRenderPassRecording(VkDevice device,
                                          const CommandPool &commandPool,
                                          CommandBuffer **commandsOut);

    // For rendering commands (draws).
    Error beginInsideRenderPassRecording(RendererVk *renderer, CommandBuffer **commandsOut);

    // storeRenderPassInfo and append*RenderTarget store info relevant to the RenderPass.
    void storeRenderPassInfo(const Framebuffer &framebuffer,
                             const gl::Rectangle renderArea,
                             const std::vector<VkClearValue> &clearValues);

    // storeRenderPassInfo and append*RenderTarget store info relevant to the RenderPass.
    // Note: RenderTargets must be added in order, with the depth/stencil being added last.
    void appendColorRenderTarget(Serial serial, RenderTargetVk *colorRenderTarget);
    void appendDepthStencilRenderTarget(Serial serial, RenderTargetVk *depthStencilRenderTarget);

    // Dependency commands order node execution in the command graph.
    // Once a node has commands that must happen after it, recording is stopped and the node is
    // frozen forever.
    static void SetHappensBeforeDependency(CommandGraphNode *beforeNode,
                                           CommandGraphNode *afterNode);
    static void SetHappensBeforeDependencies(const std::vector<CommandGraphNode *> &beforeNodes,
                                             CommandGraphNode *afterNode);
    bool hasParents() const;
    bool hasChildren() const;

    // Commands for traversing the node on a flush operation.
    VisitedState visitedState() const;
    void visitParents(std::vector<CommandGraphNode *> *stack);
    Error visitAndExecute(VkDevice device,
                          Serial serial,
                          RenderPassCache *renderPassCache,
                          CommandBuffer *primaryCommandBuffer);

    const gl::Rectangle &getRenderPassRenderArea() const;

  private:
    void setHasChildren();

    // Used for testing only.
    bool isChildOf(CommandGraphNode *parent);

    // Only used if we need a RenderPass for these commands.
    RenderPassDesc mRenderPassDesc;
    Framebuffer mRenderPassFramebuffer;
    gl::Rectangle mRenderPassRenderArea;
    gl::AttachmentArray<VkClearValue> mRenderPassClearValues;

    // Keep a separate buffers for commands inside and outside a RenderPass.
    // TODO(jmadill): We might not need inside and outside RenderPass commands separate.
    CommandBuffer mOutsideRenderPassCommands;
    CommandBuffer mInsideRenderPassCommands;

    // Parents are commands that must be submitted before 'this' CommandNode can be submitted.
    std::vector<CommandGraphNode *> mParents;

    // If this is true, other commands exist that must be submitted after 'this' command.
    bool mHasChildren;

    // Used when traversing the dependency graph.
    VisitedState mVisitedState;
};

// The Command Graph consists of an array of open Command Graph Nodes. It supports allocating new
// nodes for the graph, which are linked via dependency relation calls in CommandGraphNode, and
// also submitting the whole command graph via submitCommands.
class CommandGraph final : angle::NonCopyable
{
  public:
    CommandGraph();
    ~CommandGraph();

    // Allocates a new CommandGraphNode and adds it to the list of current open nodes. No ordering
    // relations exist in the node by default. Call CommandGraphNode::SetHappensBeforeDependency
    // to set up dependency relations.
    CommandGraphNode *allocateNode();

    Error submitCommands(VkDevice device,
                         Serial serial,
                         RenderPassCache *renderPassCache,
                         CommandPool *commandPool,
                         CommandBuffer *primaryCommandBufferOut);
    bool empty() const;

  private:
    std::vector<CommandGraphNode *> mNodes;
};

}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_

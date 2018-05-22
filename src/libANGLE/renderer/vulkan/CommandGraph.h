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
class CommandGraphNode;

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

    // Returns true if the resource is in use by the renderer.
    bool isResourceInUse(RendererVk *renderer) const;

    // Sets up dependency relations. 'this' resource is the resource being written to.
    void addWriteDependency(CommandGraphResource *writingResource);

    // Sets up dependency relations. 'this' resource is the resource being read.
    void addReadDependency(CommandGraphResource *readingResource);

  protected:
    // Allocates a write node via getNewWriteNode and returns a started command buffer.
    // The started command buffer will render outside of a RenderPass.
    Error beginWriteResource(RendererVk *renderer, CommandBuffer **commandBufferOut);

    // Check if we have started writing outside a RenderPass.
    bool hasStartedWriteResource() const;

    // Starts rendering to an existing command buffer for the resource.
    // The started command buffer will render outside of a RenderPass.
    // Calls beginWriteResource if we have not yet started writing.
    Error appendWriteResource(RendererVk *renderer, CommandBuffer **commandBufferOut);

    // Begins a command buffer on the current graph node for in-RenderPass rendering.
    // Currently only called from FramebufferVk::getCommandBufferForDraw.
    Error beginRenderPass(RendererVk *renderer,
                          const Framebuffer &framebuffer,
                          const gl::Rectangle &renderArea,
                          const RenderPassDesc &renderPassDesc,
                          const std::vector<VkClearValue> &clearValues,
                          CommandBuffer **commandBufferOut) const;

    // Checks if we're in a RenderPass, returning true if so. Updates serial internally.
    // Returns the started command buffer in commandBufferOut.
    bool appendToStartedRenderPass(RendererVk *renderer, CommandBuffer **commandBufferOut);

    // Accessor for RenderPass RenderArea.
    const gl::Rectangle &getRenderPassRenderArea() const;

    // Called when 'this' object changes, but we'd like to start a new command buffer later.
    void onResourceChanged(RendererVk *renderer);

    // Get the current queue serial for this resource. Only used to release resources.
    Serial getStoredQueueSerial() const;

  private:
    void onWriteImpl(CommandGraphNode *writingNode, Serial currentSerial);

    // Returns true if this node has a current writing node with no children.
    bool hasChildlessWritingNode() const;

    // Checks if we're in a RenderPass without children.
    bool hasStartedRenderPass() const;

    // Updates the in-use serial tracked for this resource. Will clear dependencies if the resource
    // was not used in this set of command nodes.
    void updateQueueSerial(Serial queueSerial);

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
//
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

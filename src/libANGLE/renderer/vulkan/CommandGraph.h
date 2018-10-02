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
enum class VisitedState
{
    Unvisited,
    Ready,
    Visited,
};

enum class CommandGraphResourceType
{
    Buffer,
    Framebuffer,
    Image,
};

// Only used internally in the command graph. Kept in the header for better inlining performance.
class CommandGraphNode final : angle::NonCopyable
{
  public:
    CommandGraphNode();
    ~CommandGraphNode();

    // Immutable queries for when we're walking the commands tree.
    CommandBuffer *getOutsideRenderPassCommands();

    CommandBuffer *getInsideRenderPassCommands()
    {
        ASSERT(!mHasChildren);
        return &mInsideRenderPassCommands;
    }

    // For outside the render pass (copies, transitions, etc).
    angle::Result beginOutsideRenderPassRecording(Context *context,
                                                  const CommandPool &commandPool,
                                                  CommandBuffer **commandsOut);

    // For rendering commands (draws).
    angle::Result beginInsideRenderPassRecording(Context *context, CommandBuffer **commandsOut);

    // storeRenderPassInfo and append*RenderTarget store info relevant to the RenderPass.
    void storeRenderPassInfo(const Framebuffer &framebuffer,
                             const gl::Rectangle renderArea,
                             const vk::RenderPassDesc &renderPassDesc,
                             const std::vector<VkClearValue> &clearValues);

    // Dependency commands order node execution in the command graph.
    // Once a node has commands that must happen after it, recording is stopped and the node is
    // frozen forever.
    static void SetHappensBeforeDependency(CommandGraphNode *beforeNode,
                                           CommandGraphNode *afterNode);
    static void SetHappensBeforeDependencies(const std::vector<CommandGraphNode *> &beforeNodes,
                                             CommandGraphNode *afterNode);
    bool hasParents() const;
    bool hasChildren() const { return mHasChildren; }

    // Commands for traversing the node on a flush operation.
    VisitedState visitedState() const;
    void visitParents(std::vector<CommandGraphNode *> *stack);
    angle::Result visitAndExecute(Context *context,
                                  Serial serial,
                                  RenderPassCache *renderPassCache,
                                  CommandBuffer *primaryCommandBuffer);

    // Only used in the command graph diagnostics.
    const std::vector<CommandGraphNode *> &getParentsForDiagnostics() const;
    void setDiagnosticInfo(CommandGraphResourceType resourceType, uintptr_t resourceID);

    CommandGraphResourceType getResourceTypeForDiagnostics() const { return mResourceType; }
    uintptr_t getResourceIDForDiagnostics() const { return mResourceID; }

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

    // Additional diagnostic information.
    CommandGraphResourceType mResourceType;
    uintptr_t mResourceID;
};

// This is a helper class for back-end objects used in Vk command buffers. It records a serial
// at command recording times indicating an order in the queue. We use Fences to detect when
// commands finish, and then release any unreferenced and deleted resources based on the stored
// queue serial in a special 'garbage' queue. Resources also track current read and write
// dependencies. Only one command buffer node can be writing to the Resource at a time, but many
// can be reading from it. Together the dependencies will form a command graph at submission time.
class CommandGraphResource : angle::NonCopyable
{
  public:
    virtual ~CommandGraphResource();

    // Returns true if the resource is in use by the renderer.
    bool isResourceInUse(RendererVk *renderer) const;

    // Sets up dependency relations. 'this' resource is the resource being written to.
    void addWriteDependency(CommandGraphResource *writingResource);

    // Sets up dependency relations. 'this' resource is the resource being read.
    void addReadDependency(CommandGraphResource *readingResource);

    // Allocates a write node via getNewWriteNode and returns a started command buffer.
    // The started command buffer will render outside of a RenderPass.
    // Will append to an existing command buffer/graph node if possible.
    angle::Result recordCommands(Context *context, CommandBuffer **commandBufferOut);

    // Begins a command buffer on the current graph node for in-RenderPass rendering.
    // Currently only called from FramebufferVk::getCommandBufferForDraw.
    angle::Result beginRenderPass(Context *context,
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
    void finishCurrentCommands(RendererVk *renderer);

  protected:
    explicit CommandGraphResource(CommandGraphResourceType resourceType);

    // Get the current queue serial for this resource. Only used to release resources.
    Serial getStoredQueueSerial() const;

  private:
    void onWriteImpl(CommandGraphNode *writingNode, Serial currentSerial);

    // Returns true if this node has a current writing node with no children.
    bool hasChildlessWritingNode() const
    {
        return (mCurrentWritingNode != nullptr && !mCurrentWritingNode->hasChildren());
    }

    // Checks if we're in a RenderPass without children.
    bool hasStartedRenderPass() const
    {
        return hasChildlessWritingNode() &&
               mCurrentWritingNode->getInsideRenderPassCommands()->valid();
    }

    // Updates the in-use serial tracked for this resource. Will clear dependencies if the resource
    // was not used in this set of command nodes.
    void updateQueueSerial(Serial queueSerial);

    Serial mStoredQueueSerial;
    std::vector<CommandGraphNode *> mCurrentReadingNodes;
    CommandGraphNode *mCurrentWritingNode;

    // Additional diagnostic information.
    CommandGraphResourceType mResourceType;
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
    explicit CommandGraph(bool enableGraphDiagnostics);
    ~CommandGraph();

    // Allocates a new CommandGraphNode and adds it to the list of current open nodes. No ordering
    // relations exist in the node by default. Call CommandGraphNode::SetHappensBeforeDependency
    // to set up dependency relations.
    CommandGraphNode *allocateNode();

    angle::Result submitCommands(Context *context,
                                 Serial serial,
                                 RenderPassCache *renderPassCache,
                                 CommandPool *commandPool,
                                 CommandBuffer *primaryCommandBufferOut);
    bool empty() const;

  private:
    void dumpGraphDotFile(std::ostream &out) const;

    std::vector<CommandGraphNode *> mNodes;
    bool mEnableGraphDiagnostics;
};
}  // namespace vk
}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_COMMAND_GRAPH_H_

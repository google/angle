//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandGraph:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#include "libANGLE/renderer/vulkan/CommandGraph.h"

#include <iostream>

#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

#include "third_party/trace_event/trace_event.h"

namespace rx
{
namespace vk
{
namespace
{
angle::Result InitAndBeginCommandBuffer(vk::Context *context,
                                        const CommandPool &commandPool,
                                        const VkCommandBufferInheritanceInfo &inheritanceInfo,
                                        VkCommandBufferUsageFlags flags,
                                        CommandBuffer *commandBuffer)
{
    ASSERT(!commandBuffer->valid());

    VkCommandBufferAllocateInfo createInfo = {};
    createInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.commandPool                 = commandPool.getHandle();
    createInfo.level                       = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    createInfo.commandBufferCount          = 1;

    ANGLE_VK_TRY(context, commandBuffer->init(context->getDevice(), createInfo));

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = flags | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo         = &inheritanceInfo;

    ANGLE_VK_TRY(context, commandBuffer->begin(beginInfo));
    return angle::Result::Continue();
}

const char *GetResourceTypeName(CommandGraphResourceType resourceType,
                                CommandGraphNodeFunction function)
{
    switch (resourceType)
    {
        case CommandGraphResourceType::Buffer:
            return "Buffer";
        case CommandGraphResourceType::Framebuffer:
            return "Framebuffer";
        case CommandGraphResourceType::Image:
            return "Image";
        case CommandGraphResourceType::Query:
            switch (function)
            {
                case CommandGraphNodeFunction::BeginQuery:
                    return "BeginQuery";
                case CommandGraphNodeFunction::EndQuery:
                    return "EndQuery";
                case CommandGraphNodeFunction::WriteTimestamp:
                    return "WriteTimestamp";
                default:
                    UNREACHABLE();
                    return "Query";
            }
        default:
            UNREACHABLE();
            return "";
    }
}
}  // anonymous namespace

// CommandGraphResource implementation.
CommandGraphResource::CommandGraphResource(CommandGraphResourceType resourceType)
    : mResourceType(resourceType), mCurrentWritingNode(nullptr)
{
}

CommandGraphResource::~CommandGraphResource() = default;

bool CommandGraphResource::isResourceInUse(RendererVk *renderer) const
{
    return renderer->isSerialInUse(mStoredQueueSerial);
}

bool CommandGraphResource::hasPendingWork(RendererVk *renderer) const
{
    // If the renderer has a queue serial higher than the stored one, the command buffers recorded
    // by this resource have already been submitted, so there is no pending work.
    return mStoredQueueSerial == renderer->getCurrentQueueSerial();
}

Serial CommandGraphResource::getStoredQueueSerial() const
{
    return mStoredQueueSerial;
}

// RecordableGraphResource implementation.
RecordableGraphResource::RecordableGraphResource(CommandGraphResourceType resourceType)
    : CommandGraphResource(resourceType)
{
}

RecordableGraphResource::~RecordableGraphResource() = default;

void RecordableGraphResource::updateQueueSerial(Serial queueSerial)
{
    ASSERT(queueSerial >= mStoredQueueSerial);

    if (queueSerial > mStoredQueueSerial)
    {
        mCurrentWritingNode = nullptr;
        mCurrentReadingNodes.clear();
        mStoredQueueSerial = queueSerial;
    }
}

angle::Result RecordableGraphResource::recordCommands(Context *context,
                                                      CommandBuffer **commandBufferOut)
{
    updateQueueSerial(context->getRenderer()->getCurrentQueueSerial());

    if (!hasChildlessWritingNode() || hasStartedRenderPass())
    {
        startNewCommands(context->getRenderer());
        return mCurrentWritingNode->beginOutsideRenderPassRecording(
            context, context->getRenderer()->getCommandPool(), commandBufferOut);
    }

    CommandBuffer *outsideRenderPassCommands = mCurrentWritingNode->getOutsideRenderPassCommands();
    if (!outsideRenderPassCommands->valid())
    {
        ANGLE_TRY(mCurrentWritingNode->beginOutsideRenderPassRecording(
            context, context->getRenderer()->getCommandPool(), commandBufferOut));
    }
    else
    {
        *commandBufferOut = outsideRenderPassCommands;
    }

    return angle::Result::Continue();
}

bool RecordableGraphResource::appendToStartedRenderPass(RendererVk *renderer,
                                                        CommandBuffer **commandBufferOut)
{
    updateQueueSerial(renderer->getCurrentQueueSerial());
    if (hasStartedRenderPass())
    {
        *commandBufferOut = mCurrentWritingNode->getInsideRenderPassCommands();
        return true;
    }
    else
    {
        return false;
    }
}

const gl::Rectangle &RecordableGraphResource::getRenderPassRenderArea() const
{
    ASSERT(hasStartedRenderPass());
    return mCurrentWritingNode->getRenderPassRenderArea();
}

angle::Result RecordableGraphResource::beginRenderPass(Context *context,
                                                       const Framebuffer &framebuffer,
                                                       const gl::Rectangle &renderArea,
                                                       const RenderPassDesc &renderPassDesc,
                                                       const std::vector<VkClearValue> &clearValues,
                                                       CommandBuffer **commandBufferOut)
{
    // If a barrier has been inserted in the meantime, stop the command buffer.
    if (!hasChildlessWritingNode())
    {
        startNewCommands(context->getRenderer());
    }

    // Hard-code RenderPass to clear the first render target to the current clear value.
    // TODO(jmadill): Proper clear value implementation. http://anglebug.com/2361
    mCurrentWritingNode->storeRenderPassInfo(framebuffer, renderArea, renderPassDesc, clearValues);

    return mCurrentWritingNode->beginInsideRenderPassRecording(context, commandBufferOut);
}

void RecordableGraphResource::addWriteDependency(RecordableGraphResource *writingResource)
{
    CommandGraphNode *writingNode = writingResource->mCurrentWritingNode;
    ASSERT(writingNode);

    onWriteImpl(writingNode, writingResource->getStoredQueueSerial());
}

void RecordableGraphResource::addReadDependency(RecordableGraphResource *readingResource)
{
    updateQueueSerial(readingResource->getStoredQueueSerial());

    CommandGraphNode *readingNode = readingResource->mCurrentWritingNode;
    ASSERT(readingNode);

    if (hasChildlessWritingNode())
    {
        // Ensure 'readingNode' happens after the current writing node.
        CommandGraphNode::SetHappensBeforeDependency(mCurrentWritingNode, readingNode);
    }

    // Add the read node to the list of nodes currently reading this resource.
    mCurrentReadingNodes.push_back(readingNode);
}

void RecordableGraphResource::finishCurrentCommands(RendererVk *renderer)
{
    startNewCommands(renderer);
}

void RecordableGraphResource::startNewCommands(RendererVk *renderer)
{
    CommandGraphNode *newCommands =
        renderer->getCommandGraph()->allocateNode(CommandGraphNodeFunction::Generic);
    newCommands->setDiagnosticInfo(mResourceType, reinterpret_cast<uintptr_t>(this));
    onWriteImpl(newCommands, renderer->getCurrentQueueSerial());
}

void RecordableGraphResource::onWriteImpl(CommandGraphNode *writingNode, Serial currentSerial)
{
    updateQueueSerial(currentSerial);

    // Make sure any open reads and writes finish before we execute 'writingNode'.
    if (!mCurrentReadingNodes.empty())
    {
        CommandGraphNode::SetHappensBeforeDependencies(mCurrentReadingNodes.data(),
                                                       mCurrentReadingNodes.size(), writingNode);
        mCurrentReadingNodes.clear();
    }

    if (mCurrentWritingNode && mCurrentWritingNode != writingNode)
    {
        CommandGraphNode::SetHappensBeforeDependency(mCurrentWritingNode, writingNode);
    }

    mCurrentWritingNode = writingNode;
}

// QueryGraphResource implementation.
QueryGraphResource::QueryGraphResource() : CommandGraphResource(CommandGraphResourceType::Query)
{
}

QueryGraphResource::~QueryGraphResource() = default;

void QueryGraphResource::beginQuery(Context *context,
                                    const QueryPool *queryPool,
                                    uint32_t queryIndex)
{
    startNewCommands(context->getRenderer(), CommandGraphNodeFunction::BeginQuery);
    mCurrentWritingNode->setQueryPool(queryPool, queryIndex);
}

void QueryGraphResource::endQuery(Context *context, const QueryPool *queryPool, uint32_t queryIndex)
{
    startNewCommands(context->getRenderer(), CommandGraphNodeFunction::EndQuery);
    mCurrentWritingNode->setQueryPool(queryPool, queryIndex);
}

void QueryGraphResource::writeTimestamp(Context *context,
                                        const QueryPool *queryPool,
                                        uint32_t queryIndex)
{
    startNewCommands(context->getRenderer(), CommandGraphNodeFunction::WriteTimestamp);
    mCurrentWritingNode->setQueryPool(queryPool, queryIndex);
}

void QueryGraphResource::startNewCommands(RendererVk *renderer, CommandGraphNodeFunction function)
{
    CommandGraph *commandGraph = renderer->getCommandGraph();
    CommandGraphNode *newNode  = commandGraph->allocateNode(function);
    newNode->setDiagnosticInfo(mResourceType, reinterpret_cast<uintptr_t>(this));
    commandGraph->setNewBarrier(newNode);

    mStoredQueueSerial  = renderer->getCurrentQueueSerial();
    mCurrentWritingNode = newNode;
}

// CommandGraphNode implementation.
CommandGraphNode::CommandGraphNode(CommandGraphNodeFunction function)
    : mRenderPassClearValues{},
      mFunction(function),
      mQueryPool(VK_NULL_HANDLE),
      mQueryIndex(0),
      mHasChildren(false),
      mVisitedState(VisitedState::Unvisited),
      mGlobalMemoryBarrierSrcAccess(0),
      mGlobalMemoryBarrierDstAccess(0)
{
}

CommandGraphNode::~CommandGraphNode()
{
    mRenderPassFramebuffer.setHandle(VK_NULL_HANDLE);

    // Command buffers are managed by the command pool, so don't need to be freed.
    mOutsideRenderPassCommands.releaseHandle();
    mInsideRenderPassCommands.releaseHandle();
}

CommandBuffer *CommandGraphNode::getOutsideRenderPassCommands()
{
    ASSERT(!mHasChildren);
    return &mOutsideRenderPassCommands;
}

angle::Result CommandGraphNode::beginOutsideRenderPassRecording(Context *context,
                                                                const CommandPool &commandPool,
                                                                CommandBuffer **commandsOut)
{
    ASSERT(!mHasChildren);

    VkCommandBufferInheritanceInfo inheritanceInfo = {};
    inheritanceInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass  = VK_NULL_HANDLE;
    inheritanceInfo.subpass     = 0;
    inheritanceInfo.framebuffer = VK_NULL_HANDLE;
    inheritanceInfo.occlusionQueryEnable =
        context->getRenderer()->getPhysicalDeviceFeatures().inheritedQueries;
    inheritanceInfo.queryFlags         = 0;
    inheritanceInfo.pipelineStatistics = 0;

    ANGLE_TRY(InitAndBeginCommandBuffer(context, commandPool, inheritanceInfo, 0,
                                        &mOutsideRenderPassCommands));

    *commandsOut = &mOutsideRenderPassCommands;
    return angle::Result::Continue();
}

angle::Result CommandGraphNode::beginInsideRenderPassRecording(Context *context,
                                                               CommandBuffer **commandsOut)
{
    ASSERT(!mHasChildren);

    // Get a compatible RenderPass from the cache so we can initialize the inheritance info.
    // TODO(jmadill): Support query for compatible/conformant render pass. http://anglebug.com/2361
    RenderPass *compatibleRenderPass;
    ANGLE_TRY(context->getRenderer()->getCompatibleRenderPass(context, mRenderPassDesc,
                                                              &compatibleRenderPass));

    VkCommandBufferInheritanceInfo inheritanceInfo = {};
    inheritanceInfo.sType       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.renderPass  = compatibleRenderPass->getHandle();
    inheritanceInfo.subpass     = 0;
    inheritanceInfo.framebuffer = mRenderPassFramebuffer.getHandle();
    inheritanceInfo.occlusionQueryEnable =
        context->getRenderer()->getPhysicalDeviceFeatures().inheritedQueries;
    inheritanceInfo.queryFlags         = 0;
    inheritanceInfo.pipelineStatistics = 0;

    ANGLE_TRY(InitAndBeginCommandBuffer(
        context, context->getRenderer()->getCommandPool(), inheritanceInfo,
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &mInsideRenderPassCommands));

    *commandsOut = &mInsideRenderPassCommands;
    return angle::Result::Continue();
}

void CommandGraphNode::storeRenderPassInfo(const Framebuffer &framebuffer,
                                           const gl::Rectangle renderArea,
                                           const vk::RenderPassDesc &renderPassDesc,
                                           const std::vector<VkClearValue> &clearValues)
{
    mRenderPassDesc = renderPassDesc;
    mRenderPassFramebuffer.setHandle(framebuffer.getHandle());
    mRenderPassRenderArea = renderArea;
    std::copy(clearValues.begin(), clearValues.end(), mRenderPassClearValues.begin());
}

// static
void CommandGraphNode::SetHappensBeforeDependency(CommandGraphNode *beforeNode,
                                                  CommandGraphNode *afterNode)
{
    ASSERT(beforeNode != afterNode && !beforeNode->isChildOf(afterNode));
    afterNode->mParents.emplace_back(beforeNode);
    beforeNode->setHasChildren();
}

// static
void CommandGraphNode::SetHappensBeforeDependencies(CommandGraphNode **beforeNodes,
                                                    size_t beforeNodesCount,
                                                    CommandGraphNode *afterNode)
{
    afterNode->mParents.insert(afterNode->mParents.end(), beforeNodes,
                               beforeNodes + beforeNodesCount);

    // TODO(jmadill): is there a faster way to do this?
    for (size_t i = 0; i < beforeNodesCount; ++i)
    {
        beforeNodes[i]->setHasChildren();

        ASSERT(beforeNodes[i] != afterNode && !beforeNodes[i]->isChildOf(afterNode));
    }
}

void CommandGraphNode::SetHappensBeforeDependencies(CommandGraphNode *beforeNode,
                                                    CommandGraphNode **afterNodes,
                                                    size_t afterNodesCount)
{
    for (size_t i = 0; i < afterNodesCount; ++i)
    {
        SetHappensBeforeDependency(beforeNode, afterNodes[i]);
    }
}

bool CommandGraphNode::hasParents() const
{
    return !mParents.empty();
}

void CommandGraphNode::setQueryPool(const QueryPool *queryPool, uint32_t queryIndex)
{
    ASSERT(mFunction == CommandGraphNodeFunction::BeginQuery ||
           mFunction == CommandGraphNodeFunction::EndQuery ||
           mFunction == CommandGraphNodeFunction::WriteTimestamp);
    mQueryPool  = queryPool->getHandle();
    mQueryIndex = queryIndex;
}

void CommandGraphNode::addGlobalMemoryBarrier(VkFlags srcAccess, VkFlags dstAccess)
{
    mGlobalMemoryBarrierSrcAccess |= srcAccess;
    mGlobalMemoryBarrierDstAccess |= dstAccess;
}

void CommandGraphNode::setHasChildren()
{
    mHasChildren = true;
}

// Do not call this in anything but testing code, since it's slow.
bool CommandGraphNode::isChildOf(CommandGraphNode *parent)
{
    std::set<CommandGraphNode *> visitedList;
    std::vector<CommandGraphNode *> openList;
    openList.insert(openList.begin(), mParents.begin(), mParents.end());
    while (!openList.empty())
    {
        CommandGraphNode *current = openList.back();
        openList.pop_back();
        if (visitedList.count(current) == 0)
        {
            if (current == parent)
            {
                return true;
            }
            visitedList.insert(current);
            openList.insert(openList.end(), current->mParents.begin(), current->mParents.end());
        }
    }

    return false;
}

VisitedState CommandGraphNode::visitedState() const
{
    return mVisitedState;
}

void CommandGraphNode::visitParents(std::vector<CommandGraphNode *> *stack)
{
    ASSERT(mVisitedState == VisitedState::Unvisited);
    stack->insert(stack->end(), mParents.begin(), mParents.end());
    mVisitedState = VisitedState::Ready;
}

angle::Result CommandGraphNode::visitAndExecute(vk::Context *context,
                                                Serial serial,
                                                RenderPassCache *renderPassCache,
                                                CommandBuffer *primaryCommandBuffer)
{
    switch (mFunction)
    {
        case CommandGraphNodeFunction::Generic:
            ASSERT(mQueryPool == VK_NULL_HANDLE);

            // Record the deferred pipeline barrier if necessary.
            ASSERT((mGlobalMemoryBarrierDstAccess == 0) == (mGlobalMemoryBarrierSrcAccess == 0));
            if (mGlobalMemoryBarrierSrcAccess)
            {
                VkMemoryBarrier memoryBarrier = {};
                memoryBarrier.sType           = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
                memoryBarrier.srcAccessMask   = mGlobalMemoryBarrierSrcAccess;
                memoryBarrier.dstAccessMask   = mGlobalMemoryBarrierDstAccess;

                // Use the top of pipe stage to keep the state management simple.
                primaryCommandBuffer->pipelineBarrier(VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                                      VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0, 1,
                                                      &memoryBarrier, 0, nullptr, 0, nullptr);
            }

            if (mOutsideRenderPassCommands.valid())
            {
                ANGLE_VK_TRY(context, mOutsideRenderPassCommands.end());
                primaryCommandBuffer->executeCommands(1, &mOutsideRenderPassCommands);
            }

            if (mInsideRenderPassCommands.valid())
            {
                // Pull a compatible RenderPass from the cache.
                // TODO(jmadill): Insert real ops and layout transitions.
                RenderPass *renderPass = nullptr;
                ANGLE_TRY(renderPassCache->getCompatibleRenderPass(context, serial, mRenderPassDesc,
                                                                   &renderPass));

                ANGLE_VK_TRY(context, mInsideRenderPassCommands.end());

                VkRenderPassBeginInfo beginInfo = {};
                beginInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
                beginInfo.renderPass            = renderPass->getHandle();
                beginInfo.framebuffer           = mRenderPassFramebuffer.getHandle();
                beginInfo.renderArea.offset.x   = static_cast<uint32_t>(mRenderPassRenderArea.x);
                beginInfo.renderArea.offset.y   = static_cast<uint32_t>(mRenderPassRenderArea.y);
                beginInfo.renderArea.extent.width =
                    static_cast<uint32_t>(mRenderPassRenderArea.width);
                beginInfo.renderArea.extent.height =
                    static_cast<uint32_t>(mRenderPassRenderArea.height);
                beginInfo.clearValueCount = mRenderPassDesc.attachmentCount();
                beginInfo.pClearValues    = mRenderPassClearValues.data();

                primaryCommandBuffer->beginRenderPass(
                    beginInfo, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
                primaryCommandBuffer->executeCommands(1, &mInsideRenderPassCommands);
                primaryCommandBuffer->endRenderPass();
            }
            break;

        case CommandGraphNodeFunction::BeginQuery:
            ASSERT(!mOutsideRenderPassCommands.valid() && !mInsideRenderPassCommands.valid());
            ASSERT(mQueryPool != VK_NULL_HANDLE);

            primaryCommandBuffer->resetQueryPool(mQueryPool, mQueryIndex, 1);
            primaryCommandBuffer->beginQuery(mQueryPool, mQueryIndex, 0);

            break;

        case CommandGraphNodeFunction::EndQuery:
            ASSERT(!mOutsideRenderPassCommands.valid() && !mInsideRenderPassCommands.valid());
            ASSERT(mQueryPool != VK_NULL_HANDLE);

            primaryCommandBuffer->endQuery(mQueryPool, mQueryIndex);

            break;

        case CommandGraphNodeFunction::WriteTimestamp:
            ASSERT(!mOutsideRenderPassCommands.valid() && !mInsideRenderPassCommands.valid());
            ASSERT(mQueryPool != VK_NULL_HANDLE);

            primaryCommandBuffer->resetQueryPool(mQueryPool, mQueryIndex, 1);
            primaryCommandBuffer->writeTimestamp(VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, mQueryPool,
                                                 mQueryIndex);

            break;

        default:
            UNREACHABLE();
    }

    mVisitedState = VisitedState::Visited;
    return angle::Result::Continue();
}

const std::vector<CommandGraphNode *> &CommandGraphNode::getParentsForDiagnostics() const
{
    return mParents;
}

void CommandGraphNode::setDiagnosticInfo(CommandGraphResourceType resourceType,
                                         uintptr_t resourceID)
{
    mResourceType = resourceType;
    mResourceID   = resourceID;
}

const gl::Rectangle &CommandGraphNode::getRenderPassRenderArea() const
{
    return mRenderPassRenderArea;
}

// CommandGraph implementation.
CommandGraph::CommandGraph(bool enableGraphDiagnostics)
    : mEnableGraphDiagnostics(enableGraphDiagnostics), mLastBarrierIndex(kInvalidNodeIndex)
{
}

CommandGraph::~CommandGraph()
{
    ASSERT(empty());
}

CommandGraphNode *CommandGraph::allocateNode(CommandGraphNodeFunction function)
{
    // TODO(jmadill): Use a pool allocator for the CPU node allocations.
    CommandGraphNode *newCommands = new CommandGraphNode(function);
    mNodes.emplace_back(newCommands);
    return newCommands;
}

void CommandGraph::setNewBarrier(CommandGraphNode *newBarrier)
{
    size_t previousBarrierIndex       = 0;
    CommandGraphNode *previousBarrier = getLastBarrierNode(&previousBarrierIndex);

    // Add a dependency from previousBarrier to all nodes in (previousBarrier, newBarrier].
    if (previousBarrier && previousBarrierIndex + 1 < mNodes.size())
    {
        size_t afterNodesCount = mNodes.size() - (previousBarrierIndex + 1);
        CommandGraphNode::SetHappensBeforeDependencies(
            previousBarrier, &mNodes[previousBarrierIndex + 1], afterNodesCount);
    }

    // Add a dependency from all nodes in (previousBarrier, newBarrier) to newBarrier.
    addDependenciesToNextBarrier(previousBarrierIndex + 1, mNodes.size() - 1, newBarrier);

    mLastBarrierIndex = mNodes.size() - 1;
}

angle::Result CommandGraph::submitCommands(Context *context,
                                           Serial serial,
                                           RenderPassCache *renderPassCache,
                                           CommandPool *commandPool,
                                           CommandBuffer *primaryCommandBufferOut)
{
    // There is no point in submitting an empty command buffer, so make sure not to call this
    // function if there's nothing to do.
    ASSERT(!mNodes.empty());

    size_t previousBarrierIndex       = 0;
    CommandGraphNode *previousBarrier = getLastBarrierNode(&previousBarrierIndex);

    // Add a dependency from previousBarrier to all nodes in (previousBarrier, end].
    if (previousBarrier && previousBarrierIndex + 1 < mNodes.size())
    {
        size_t afterNodesCount = mNodes.size() - (previousBarrierIndex + 1);
        CommandGraphNode::SetHappensBeforeDependencies(
            previousBarrier, &mNodes[previousBarrierIndex + 1], afterNodesCount);
    }

    mLastBarrierIndex = kInvalidNodeIndex;

    VkCommandBufferAllocateInfo primaryInfo = {};
    primaryInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    primaryInfo.commandPool                 = commandPool->getHandle();
    primaryInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    primaryInfo.commandBufferCount          = 1;

    ANGLE_VK_TRY(context, primaryCommandBufferOut->init(context->getDevice(), primaryInfo));

    if (mEnableGraphDiagnostics)
    {
        dumpGraphDotFile(std::cout);
    }

    std::vector<CommandGraphNode *> nodeStack;

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo         = nullptr;

    ANGLE_VK_TRY(context, primaryCommandBufferOut->begin(beginInfo));

    ANGLE_TRY(context->getRenderer()->traceGpuEvent(
        context, primaryCommandBufferOut, TRACE_EVENT_PHASE_BEGIN, "Primary Command Buffer"));

    for (CommandGraphNode *topLevelNode : mNodes)
    {
        // Only process commands that don't have child commands. The others will be pulled in
        // automatically. Also skip commands that have already been visited.
        if (topLevelNode->hasChildren() || topLevelNode->visitedState() != VisitedState::Unvisited)
            continue;

        nodeStack.push_back(topLevelNode);

        while (!nodeStack.empty())
        {
            CommandGraphNode *node = nodeStack.back();

            switch (node->visitedState())
            {
                case VisitedState::Unvisited:
                    node->visitParents(&nodeStack);
                    break;
                case VisitedState::Ready:
                    ANGLE_TRY(node->visitAndExecute(context, serial, renderPassCache,
                                                    primaryCommandBufferOut));
                    nodeStack.pop_back();
                    break;
                case VisitedState::Visited:
                    nodeStack.pop_back();
                    break;
                default:
                    UNREACHABLE();
                    break;
            }
        }
    }

    ANGLE_TRY(context->getRenderer()->traceGpuEvent(
        context, primaryCommandBufferOut, TRACE_EVENT_PHASE_END, "Primary Command Buffer"));

    ANGLE_VK_TRY(context, primaryCommandBufferOut->end());

    // TODO(jmadill): Use pool allocation so we don't need to deallocate command graph.
    for (CommandGraphNode *node : mNodes)
    {
        delete node;
    }
    mNodes.clear();

    return angle::Result::Continue();
}

bool CommandGraph::empty() const
{
    return mNodes.empty();
}

// Dumps the command graph into a dot file that works with graphviz.
void CommandGraph::dumpGraphDotFile(std::ostream &out) const
{
    // This ID maps a node pointer to a monatonic ID. It allows us to look up parent node IDs.
    std::map<const CommandGraphNode *, int> nodeIDMap;
    std::map<uintptr_t, int> objectIDMap;

    // Map nodes to ids.
    for (size_t nodeIndex = 0; nodeIndex < mNodes.size(); ++nodeIndex)
    {
        const CommandGraphNode *node = mNodes[nodeIndex];
        nodeIDMap[node]              = static_cast<int>(nodeIndex) + 1;
    }

    int bufferIDCounter      = 1;
    int framebufferIDCounter = 1;
    int imageIDCounter       = 1;
    int queryIDCounter       = 1;

    out << "digraph {" << std::endl;

    for (const CommandGraphNode *node : mNodes)
    {
        int nodeID = nodeIDMap[node];

        std::stringstream strstr;
        strstr << GetResourceTypeName(node->getResourceTypeForDiagnostics(), node->getFunction());
        strstr << " ";

        auto it = objectIDMap.find(node->getResourceIDForDiagnostics());
        if (it != objectIDMap.end())
        {
            strstr << it->second;
        }
        else
        {
            int id = 0;

            switch (node->getResourceTypeForDiagnostics())
            {
                case CommandGraphResourceType::Buffer:
                    id = bufferIDCounter++;
                    break;
                case CommandGraphResourceType::Framebuffer:
                    id = framebufferIDCounter++;
                    break;
                case CommandGraphResourceType::Image:
                    id = imageIDCounter++;
                    break;
                case CommandGraphResourceType::Query:
                    id = queryIDCounter++;
                    break;
                default:
                    UNREACHABLE();
                    break;
            }

            objectIDMap[node->getResourceIDForDiagnostics()] = id;
            strstr << id;
        }

        const std::string &label = strstr.str();
        out << "  " << nodeID << "[label =<" << label << "<BR/> <FONT POINT-SIZE=\"10\">Node ID "
            << nodeID << "</FONT>>];" << std::endl;
    }

    for (const CommandGraphNode *node : mNodes)
    {
        int nodeID = nodeIDMap[node];

        for (const CommandGraphNode *parent : node->getParentsForDiagnostics())
        {
            int parentID = nodeIDMap[parent];
            out << "  " << parentID << " -> " << nodeID << ";" << std::endl;
        }
    }

    out << "}" << std::endl;
}

CommandGraphNode *CommandGraph::getLastBarrierNode(size_t *indexOut)
{
    *indexOut = mLastBarrierIndex == kInvalidNodeIndex ? 0 : mLastBarrierIndex;
    return mLastBarrierIndex == kInvalidNodeIndex ? nullptr : mNodes[mLastBarrierIndex];
}

void CommandGraph::addDependenciesToNextBarrier(size_t begin,
                                                size_t end,
                                                CommandGraphNode *nextBarrier)
{
    for (size_t i = begin; i < end; ++i)
    {
        // As a small optimization, only add edges to childless nodes.  The others have an
        // indirect dependency.
        if (!mNodes[i]->hasChildren())
        {
            CommandGraphNode::SetHappensBeforeDependency(mNodes[i], nextBarrier);
        }
    }
}

}  // namespace vk
}  // namespace rx

//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandGraph:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#include "libANGLE/renderer/vulkan/CommandGraph.h"

#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{

namespace vk
{

namespace
{

Error InitAndBeginCommandBuffer(VkDevice device,
                                const CommandPool &commandPool,
                                const VkCommandBufferInheritanceInfo &inheritanceInfo,
                                VkCommandBufferUsageFlags flags,
                                CommandBuffer *commandBuffer)
{
    ASSERT(!commandBuffer->valid());

    VkCommandBufferAllocateInfo createInfo;
    createInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    createInfo.pNext              = nullptr;
    createInfo.commandPool        = commandPool.getHandle();
    createInfo.level              = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    createInfo.commandBufferCount = 1;

    ANGLE_TRY(commandBuffer->init(device, createInfo));

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext            = nullptr;
    beginInfo.flags            = flags | VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    beginInfo.pInheritanceInfo = &inheritanceInfo;

    ANGLE_TRY(commandBuffer->begin(beginInfo));
    return NoError();
}

}  // anonymous namespace

// CommandGraphResource implementation.
CommandGraphResource::CommandGraphResource() : mCurrentWritingNode(nullptr)
{
}

CommandGraphResource::~CommandGraphResource()
{
}

void CommandGraphResource::updateQueueSerial(Serial queueSerial)
{
    ASSERT(queueSerial >= mStoredQueueSerial);

    if (queueSerial > mStoredQueueSerial)
    {
        mCurrentWritingNode = nullptr;
        mCurrentReadingNodes.clear();
        mStoredQueueSerial = queueSerial;
    }
}

Serial CommandGraphResource::getQueueSerial() const
{
    return mStoredQueueSerial;
}

bool CommandGraphResource::hasChildlessWritingNode() const
{
    return (mCurrentWritingNode != nullptr && !mCurrentWritingNode->hasChildren());
}

CommandGraphNode *CommandGraphResource::getCurrentWritingNode()
{
    return mCurrentWritingNode;
}

CommandGraphNode *CommandGraphResource::getNewWritingNode(RendererVk *renderer)
{
    CommandGraphNode *newCommands = renderer->allocateCommandNode();
    onWriteResource(newCommands, renderer->getCurrentQueueSerial());
    return newCommands;
}

Error CommandGraphResource::beginWriteResource(RendererVk *renderer,
                                               CommandBuffer **commandBufferOut)
{
    CommandGraphNode *commands = getNewWritingNode(renderer);

    VkDevice device = renderer->getDevice();
    ANGLE_TRY(commands->beginOutsideRenderPassRecording(device, renderer->getCommandPool(),
                                                        commandBufferOut));
    return NoError();
}

void CommandGraphResource::onWriteResource(CommandGraphNode *writingNode, Serial serial)
{
    updateQueueSerial(serial);

    // Make sure any open reads and writes finish before we execute 'writingNode'.
    if (!mCurrentReadingNodes.empty())
    {
        CommandGraphNode::SetHappensBeforeDependencies(mCurrentReadingNodes, writingNode);
        mCurrentReadingNodes.clear();
    }

    if (mCurrentWritingNode && mCurrentWritingNode != writingNode)
    {
        CommandGraphNode::SetHappensBeforeDependency(mCurrentWritingNode, writingNode);
    }

    mCurrentWritingNode = writingNode;
}

void CommandGraphResource::onReadResource(CommandGraphNode *readingNode, Serial serial)
{
    updateQueueSerial(serial);

    if (hasChildlessWritingNode())
    {
        ASSERT(mStoredQueueSerial == serial);

        // Ensure 'readingNode' happens after the current writing node.
        CommandGraphNode::SetHappensBeforeDependency(mCurrentWritingNode, readingNode);
    }

    // Add the read node to the list of nodes currently reading this resource.
    mCurrentReadingNodes.push_back(readingNode);
}

bool CommandGraphResource::checkResourceInUseAndRefreshDeps(RendererVk *renderer)
{
    if (!renderer->isResourceInUse(*this) ||
        (renderer->getCurrentQueueSerial() > mStoredQueueSerial))
    {
        mCurrentReadingNodes.clear();
        mCurrentWritingNode = nullptr;
        return false;
    }
    else
    {
        return true;
    }
}

// CommandGraphNode implementation.

CommandGraphNode::CommandGraphNode() : mHasChildren(false), mVisitedState(VisitedState::Unvisited)
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

CommandBuffer *CommandGraphNode::getInsideRenderPassCommands()
{
    ASSERT(!mHasChildren);
    return &mInsideRenderPassCommands;
}

Error CommandGraphNode::beginOutsideRenderPassRecording(VkDevice device,
                                                        const CommandPool &commandPool,
                                                        CommandBuffer **commandsOut)
{
    ASSERT(!mHasChildren);

    VkCommandBufferInheritanceInfo inheritanceInfo;
    inheritanceInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext                = nullptr;
    inheritanceInfo.renderPass           = VK_NULL_HANDLE;
    inheritanceInfo.subpass              = 0;
    inheritanceInfo.framebuffer          = VK_NULL_HANDLE;
    inheritanceInfo.occlusionQueryEnable = VK_FALSE;
    inheritanceInfo.queryFlags           = 0;
    inheritanceInfo.pipelineStatistics   = 0;

    ANGLE_TRY(InitAndBeginCommandBuffer(device, commandPool, inheritanceInfo, 0,
                                        &mOutsideRenderPassCommands));

    *commandsOut = &mOutsideRenderPassCommands;
    return NoError();
}

Error CommandGraphNode::beginInsideRenderPassRecording(RendererVk *renderer,
                                                       CommandBuffer **commandsOut)
{
    ASSERT(!mHasChildren);

    // Get a compatible RenderPass from the cache so we can initialize the inheritance info.
    // TODO(jmadill): Support query for compatible/conformant render pass. htto://anglebug.com/2361
    RenderPass *compatibleRenderPass;
    ANGLE_TRY(renderer->getCompatibleRenderPass(mRenderPassDesc, &compatibleRenderPass));

    VkCommandBufferInheritanceInfo inheritanceInfo;
    inheritanceInfo.sType                = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
    inheritanceInfo.pNext                = nullptr;
    inheritanceInfo.renderPass           = compatibleRenderPass->getHandle();
    inheritanceInfo.subpass              = 0;
    inheritanceInfo.framebuffer          = mRenderPassFramebuffer.getHandle();
    inheritanceInfo.occlusionQueryEnable = VK_FALSE;
    inheritanceInfo.queryFlags           = 0;
    inheritanceInfo.pipelineStatistics   = 0;

    ANGLE_TRY(InitAndBeginCommandBuffer(
        renderer->getDevice(), renderer->getCommandPool(), inheritanceInfo,
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT, &mInsideRenderPassCommands));

    *commandsOut = &mInsideRenderPassCommands;
    return NoError();
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
void CommandGraphNode::SetHappensBeforeDependencies(
    const std::vector<CommandGraphNode *> &beforeNodes,
    CommandGraphNode *afterNode)
{
    afterNode->mParents.insert(afterNode->mParents.end(), beforeNodes.begin(), beforeNodes.end());

    // TODO(jmadill): is there a faster way to do this?
    for (CommandGraphNode *beforeNode : beforeNodes)
    {
        beforeNode->setHasChildren();

        ASSERT(beforeNode != afterNode && !beforeNode->isChildOf(afterNode));
    }
}

bool CommandGraphNode::hasParents() const
{
    return !mParents.empty();
}

void CommandGraphNode::setHasChildren()
{
    mHasChildren = true;
}

bool CommandGraphNode::hasChildren() const
{
    return mHasChildren;
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

Error CommandGraphNode::visitAndExecute(VkDevice device,
                                        Serial serial,
                                        RenderPassCache *renderPassCache,
                                        CommandBuffer *primaryCommandBuffer)
{
    if (mOutsideRenderPassCommands.valid())
    {
        mOutsideRenderPassCommands.end();
        primaryCommandBuffer->executeCommands(1, &mOutsideRenderPassCommands);
    }

    if (mInsideRenderPassCommands.valid())
    {
        // Pull a compatible RenderPass from the cache.
        // TODO(jmadill): Insert real ops and layout transitions.
        RenderPass *renderPass = nullptr;
        ANGLE_TRY(
            renderPassCache->getCompatibleRenderPass(device, serial, mRenderPassDesc, &renderPass));

        mInsideRenderPassCommands.end();

        VkRenderPassBeginInfo beginInfo;
        beginInfo.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.pNext                    = nullptr;
        beginInfo.renderPass               = renderPass->getHandle();
        beginInfo.framebuffer              = mRenderPassFramebuffer.getHandle();
        beginInfo.renderArea.offset.x      = static_cast<uint32_t>(mRenderPassRenderArea.x);
        beginInfo.renderArea.offset.y      = static_cast<uint32_t>(mRenderPassRenderArea.y);
        beginInfo.renderArea.extent.width  = static_cast<uint32_t>(mRenderPassRenderArea.width);
        beginInfo.renderArea.extent.height = static_cast<uint32_t>(mRenderPassRenderArea.height);
        beginInfo.clearValueCount          = mRenderPassDesc.attachmentCount();
        beginInfo.pClearValues             = mRenderPassClearValues.data();

        primaryCommandBuffer->beginRenderPass(beginInfo,
                                              VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);
        primaryCommandBuffer->executeCommands(1, &mInsideRenderPassCommands);
        primaryCommandBuffer->endRenderPass();
    }

    mVisitedState = VisitedState::Visited;
    return NoError();
}

const gl::Rectangle &CommandGraphNode::getRenderPassRenderArea() const
{
    return mRenderPassRenderArea;
}

// CommandGraph implementation.
CommandGraph::CommandGraph()
{
}

CommandGraph::~CommandGraph()
{
    ASSERT(empty());
}

CommandGraphNode *CommandGraph::allocateNode()
{
    // TODO(jmadill): Use a pool allocator for the CPU node allocations.
    CommandGraphNode *newCommands = new CommandGraphNode();
    mNodes.emplace_back(newCommands);
    return newCommands;
}

Error CommandGraph::submitCommands(VkDevice device,
                                   Serial serial,
                                   RenderPassCache *renderPassCache,
                                   CommandPool *commandPool,
                                   CommandBuffer *primaryCommandBufferOut)
{
    VkCommandBufferAllocateInfo primaryInfo;
    primaryInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    primaryInfo.pNext              = nullptr;
    primaryInfo.commandPool        = commandPool->getHandle();
    primaryInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    primaryInfo.commandBufferCount = 1;

    ANGLE_TRY(primaryCommandBufferOut->init(device, primaryInfo));

    if (mNodes.empty())
    {
        return NoError();
    }

    std::vector<CommandGraphNode *> nodeStack;

    VkCommandBufferBeginInfo beginInfo;
    beginInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.pNext            = nullptr;
    beginInfo.flags            = 0;
    beginInfo.pInheritanceInfo = nullptr;

    ANGLE_TRY(primaryCommandBufferOut->begin(beginInfo));

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
                    ANGLE_TRY(node->visitAndExecute(device, serial, renderPassCache,
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

    ANGLE_TRY(primaryCommandBufferOut->end());

    // TODO(jmadill): Use pool allocation so we don't need to deallocate command graph.
    for (CommandGraphNode *node : mNodes)
    {
        delete node;
    }
    mNodes.clear();

    return NoError();
}

bool CommandGraph::empty() const
{
    return mNodes.empty();
}

}  // namespace vk
}  // namespace rx

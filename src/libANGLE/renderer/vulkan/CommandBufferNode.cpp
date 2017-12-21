//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// CommandBufferNode:
//    Deferred work constructed by GL calls, that will later be flushed to Vulkan.
//

#include "libANGLE/renderer/vulkan/CommandBufferNode.h"

#include "libANGLE/renderer/vulkan/RenderTargetVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/formatutilsvk.h"

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

// CommandBufferNode implementation.

CommandBufferNode::CommandBufferNode()
    : mIsDependency(false), mVisitedState(VisitedState::Unvisited)
{
}

CommandBufferNode::~CommandBufferNode()
{
    mRenderPassFramebuffer.setHandle(VK_NULL_HANDLE);

    // Command buffers are managed by the command pool, so don't need to be freed.
    mOutsideRenderPassCommands.releaseHandle();
    mInsideRenderPassCommands.releaseHandle();
}

CommandBuffer *CommandBufferNode::getOutsideRenderPassCommands()
{
    return &mOutsideRenderPassCommands;
}

CommandBuffer *CommandBufferNode::getInsideRenderPassCommands()
{
    return &mInsideRenderPassCommands;
}

Error CommandBufferNode::startRecording(VkDevice device,
                                        const CommandPool &commandPool,
                                        CommandBuffer **commandsOut)
{
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

Error CommandBufferNode::startRenderPassRecording(RendererVk *renderer, CommandBuffer **commandsOut)
{
    // Get a compatible RenderPass from the cache so we can initialize the inheritance info.
    // TODO(jmadill): Use different query method for compatible vs conformant render pass.
    vk::RenderPass *compatibleRenderPass;
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

void CommandBufferNode::storeRenderPassInfo(const Framebuffer &framebuffer,
                                            const gl::Rectangle renderArea,
                                            const std::vector<VkClearValue> &clearValues)
{
    mRenderPassFramebuffer.setHandle(framebuffer.getHandle());
    mRenderPassRenderArea = renderArea;
    std::copy(clearValues.begin(), clearValues.end(), mRenderPassClearValues.begin());
}

void CommandBufferNode::appendColorRenderTarget(Serial serial, RenderTargetVk *colorRenderTarget)
{
    // TODO(jmadill): Layout transition?
    mRenderPassDesc.packColorAttachment(*colorRenderTarget->format, colorRenderTarget->samples);
    colorRenderTarget->resource->setWriteNode(serial, this);
}

void CommandBufferNode::appendDepthStencilRenderTarget(Serial serial,
                                                       RenderTargetVk *depthStencilRenderTarget)
{
    // TODO(jmadill): Layout transition?
    mRenderPassDesc.packDepthStencilAttachment(*depthStencilRenderTarget->format,
                                               depthStencilRenderTarget->samples);
    depthStencilRenderTarget->resource->setWriteNode(serial, this);
}

void CommandBufferNode::initAttachmentDesc(VkAttachmentDescription *desc)
{
    desc->flags          = 0;
    desc->format         = VK_FORMAT_UNDEFINED;
    desc->samples        = static_cast<VkSampleCountFlagBits>(0);
    desc->loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc->storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    desc->stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc->stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc->initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
    desc->finalLayout    = VK_IMAGE_LAYOUT_UNDEFINED;
}

void CommandBufferNode::addDependency(CommandBufferNode *node)
{
    mDependencies.emplace_back(node);
    node->markAsDependency();
    ASSERT(node != this && !node->hasDependency(this));
}

void CommandBufferNode::addDependencies(const std::vector<CommandBufferNode *> &nodes)
{
    mDependencies.insert(mDependencies.end(), nodes.begin(), nodes.end());

    // TODO(jmadill): is there a faster way to do this?
    for (CommandBufferNode *node : nodes)
    {
        node->markAsDependency();
        ASSERT(node != this && !node->hasDependency(this));
    }
}

bool CommandBufferNode::hasDependencies() const
{
    return !mDependencies.empty();
}

void CommandBufferNode::markAsDependency()
{
    mIsDependency = true;
}

bool CommandBufferNode::isDependency() const
{
    return mIsDependency;
}

// Do not call this in anything but testing code, since it's slow.
bool CommandBufferNode::hasDependency(CommandBufferNode *searchNode)
{
    std::set<CommandBufferNode *> visitedList;
    std::vector<CommandBufferNode *> openList;
    openList.insert(openList.begin(), mDependencies.begin(), mDependencies.end());
    while (!openList.empty())
    {
        CommandBufferNode *node = openList.back();
        openList.pop_back();
        if (visitedList.count(node) == 0)
        {
            if (node == searchNode)
            {
                return true;
            }
            visitedList.insert(node);
            openList.insert(openList.end(), node->mDependencies.begin(), node->mDependencies.end());
        }
    }

    return false;
}

VisitedState CommandBufferNode::visitedState() const
{
    return mVisitedState;
}

void CommandBufferNode::visitDependencies(std::vector<CommandBufferNode *> *stack)
{
    ASSERT(mVisitedState == VisitedState::Unvisited);
    stack->insert(stack->end(), mDependencies.begin(), mDependencies.end());
    mVisitedState = VisitedState::Ready;
}

vk::Error CommandBufferNode::visitAndExecute(RendererVk *renderer,
                                             vk::CommandBuffer *primaryCommandBuffer)
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
        vk::RenderPass *renderPass = nullptr;
        ANGLE_TRY(renderer->getCompatibleRenderPass(mRenderPassDesc, &renderPass));

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
    return vk::NoError();
}

}  // namespace vk
}  // namespace rx

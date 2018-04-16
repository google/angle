//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// vk_cache_utils.cpp:
//    Contains the classes for the Pipeline State Object cache as well as the RenderPass cache.
//    Also contains the structures for the packed descriptions for the RenderPass and Pipeline.
//

#include "libANGLE/renderer/vulkan/vk_cache_utils.h"

#include "common/aligned_memory.h"
#include "libANGLE/SizedMRUCache.h"
#include "libANGLE/VertexAttribute.h"
#include "libANGLE/renderer/vulkan/ProgramVk.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_format_utils.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
namespace vk
{

namespace
{

uint8_t PackGLBlendOp(GLenum blendOp)
{
    switch (blendOp)
    {
        case GL_FUNC_ADD:
            return static_cast<uint8_t>(VK_BLEND_OP_ADD);
        case GL_FUNC_SUBTRACT:
            return static_cast<uint8_t>(VK_BLEND_OP_SUBTRACT);
        case GL_FUNC_REVERSE_SUBTRACT:
            return static_cast<uint8_t>(VK_BLEND_OP_REVERSE_SUBTRACT);
        default:
            UNREACHABLE();
            return 0;
    }
}

uint8_t PackGLBlendFactor(GLenum blendFactor)
{
    switch (blendFactor)
    {
        case GL_ZERO:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ZERO);
        case GL_ONE:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
        case GL_SRC_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_SRC_COLOR);
        case GL_DST_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_DST_COLOR);
        case GL_ONE_MINUS_SRC_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR);
        case GL_SRC_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_SRC_ALPHA);
        case GL_ONE_MINUS_SRC_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
        case GL_DST_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_DST_ALPHA);
        case GL_ONE_MINUS_DST_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA);
        case GL_ONE_MINUS_DST_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR);
        case GL_SRC_ALPHA_SATURATE:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_SRC_ALPHA_SATURATE);
        case GL_CONSTANT_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_CONSTANT_COLOR);
        case GL_CONSTANT_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_CONSTANT_ALPHA);
        case GL_ONE_MINUS_CONSTANT_COLOR:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR);
        case GL_ONE_MINUS_CONSTANT_ALPHA:
            return static_cast<uint8_t>(VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA);
        default:
            UNREACHABLE();
            return 0;
    }
}

uint8_t PackGLStencilOp(GLenum compareOp)
{
    switch (compareOp)
    {
        case GL_KEEP:
            return VK_STENCIL_OP_KEEP;
        case GL_ZERO:
            return VK_STENCIL_OP_ZERO;
        case GL_REPLACE:
            return VK_STENCIL_OP_REPLACE;
        case GL_INCR:
            return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
        case GL_DECR:
            return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
        case GL_INCR_WRAP:
            return VK_STENCIL_OP_INCREMENT_AND_WRAP;
        case GL_DECR_WRAP:
            return VK_STENCIL_OP_DECREMENT_AND_WRAP;
        case GL_INVERT:
            return VK_STENCIL_OP_INVERT;
        default:
            UNREACHABLE();
            return 0;
    }
}

uint8_t PackGLCompareFunc(GLenum compareFunc)
{
    switch (compareFunc)
    {
        case GL_NEVER:
            return VK_COMPARE_OP_NEVER;
        case GL_ALWAYS:
            return VK_COMPARE_OP_ALWAYS;
        case GL_LESS:
            return VK_COMPARE_OP_LESS;
        case GL_LEQUAL:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case GL_EQUAL:
            return VK_COMPARE_OP_EQUAL;
        case GL_GREATER:
            return VK_COMPARE_OP_GREATER;
        case GL_GEQUAL:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        case GL_NOTEQUAL:
            return VK_COMPARE_OP_NOT_EQUAL;
        default:
            UNREACHABLE();
            return 0;
    }
}

void UnpackAttachmentDesc(VkAttachmentDescription *desc,
                          const vk::PackedAttachmentDesc &packedDesc,
                          const vk::PackedAttachmentOpsDesc &ops)
{
    desc->flags          = static_cast<VkAttachmentDescriptionFlags>(packedDesc.flags);
    desc->format         = static_cast<VkFormat>(packedDesc.format);
    desc->samples        = gl_vk::GetSamples(packedDesc.samples);
    desc->loadOp         = static_cast<VkAttachmentLoadOp>(ops.loadOp);
    desc->storeOp        = static_cast<VkAttachmentStoreOp>(ops.storeOp);
    desc->stencilLoadOp  = static_cast<VkAttachmentLoadOp>(ops.stencilLoadOp);
    desc->stencilStoreOp = static_cast<VkAttachmentStoreOp>(ops.stencilStoreOp);
    desc->initialLayout  = static_cast<VkImageLayout>(ops.initialLayout);
    desc->finalLayout    = static_cast<VkImageLayout>(ops.finalLayout);
}

void UnpackStencilState(const vk::PackedStencilOpState &packedState, VkStencilOpState *stateOut)
{
    stateOut->failOp      = static_cast<VkStencilOp>(packedState.failOp);
    stateOut->passOp      = static_cast<VkStencilOp>(packedState.passOp);
    stateOut->depthFailOp = static_cast<VkStencilOp>(packedState.depthFailOp);
    stateOut->compareOp   = static_cast<VkCompareOp>(packedState.compareOp);
    stateOut->compareMask = packedState.compareMask;
    stateOut->writeMask   = packedState.writeMask;
    stateOut->reference   = packedState.reference;
}

void UnpackBlendAttachmentState(const vk::PackedColorBlendAttachmentState &packedState,
                                VkPipelineColorBlendAttachmentState *stateOut)
{
    stateOut->blendEnable         = static_cast<VkBool32>(packedState.blendEnable);
    stateOut->srcColorBlendFactor = static_cast<VkBlendFactor>(packedState.srcColorBlendFactor);
    stateOut->dstColorBlendFactor = static_cast<VkBlendFactor>(packedState.dstColorBlendFactor);
    stateOut->colorBlendOp        = static_cast<VkBlendOp>(packedState.colorBlendOp);
    stateOut->srcAlphaBlendFactor = static_cast<VkBlendFactor>(packedState.srcAlphaBlendFactor);
    stateOut->dstAlphaBlendFactor = static_cast<VkBlendFactor>(packedState.dstAlphaBlendFactor);
    stateOut->alphaBlendOp        = static_cast<VkBlendOp>(packedState.alphaBlendOp);
    stateOut->colorWriteMask      = static_cast<VkColorComponentFlags>(packedState.colorWriteMask);
}

Error InitializeRenderPassFromDesc(VkDevice device,
                                   const RenderPassDesc &desc,
                                   const AttachmentOpsArray &ops,
                                   RenderPass *renderPass)
{
    uint32_t attachmentCount = desc.attachmentCount();
    ASSERT(attachmentCount > 0);

    gl::DrawBuffersArray<VkAttachmentReference> colorAttachmentRefs;

    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        VkAttachmentReference &colorRef = colorAttachmentRefs[colorIndex];
        colorRef.attachment             = colorIndex;
        colorRef.layout                 = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }

    VkAttachmentReference depthStencilAttachmentRef;
    if (desc.depthStencilAttachmentCount() > 0)
    {
        ASSERT(desc.depthStencilAttachmentCount() == 1);
        depthStencilAttachmentRef.attachment = desc.colorAttachmentCount();
        depthStencilAttachmentRef.layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }

    VkSubpassDescription subpassDesc;

    subpassDesc.flags                = 0;
    subpassDesc.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDesc.inputAttachmentCount = 0;
    subpassDesc.pInputAttachments    = nullptr;
    subpassDesc.colorAttachmentCount = desc.colorAttachmentCount();
    subpassDesc.pColorAttachments    = colorAttachmentRefs.data();
    subpassDesc.pResolveAttachments  = nullptr;
    subpassDesc.pDepthStencilAttachment =
        (desc.depthStencilAttachmentCount() > 0 ? &depthStencilAttachmentRef : nullptr);
    subpassDesc.preserveAttachmentCount = 0;
    subpassDesc.pPreserveAttachments    = nullptr;

    // Unpack the packed and split representation into the format required by Vulkan.
    gl::AttachmentArray<VkAttachmentDescription> attachmentDescs;
    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        UnpackAttachmentDesc(&attachmentDescs[colorIndex], desc[colorIndex], ops[colorIndex]);
    }

    if (desc.depthStencilAttachmentCount() > 0)
    {
        uint32_t depthStencilIndex = desc.colorAttachmentCount();
        UnpackAttachmentDesc(&attachmentDescs[depthStencilIndex], desc[depthStencilIndex],
                             ops[depthStencilIndex]);
    }

    VkRenderPassCreateInfo createInfo;
    createInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.pNext           = nullptr;
    createInfo.flags           = 0;
    createInfo.attachmentCount = attachmentCount;
    createInfo.pAttachments    = attachmentDescs.data();
    createInfo.subpassCount    = 1;
    createInfo.pSubpasses      = &subpassDesc;
    createInfo.dependencyCount = 0;
    createInfo.pDependencies   = nullptr;

    ANGLE_TRY(renderPass->init(device, createInfo));
    return vk::NoError();
}

}  // anonymous namespace

// RenderPassDesc implementation.
RenderPassDesc::RenderPassDesc()
{
    UNUSED_VARIABLE(mPadding);
    memset(this, 0, sizeof(RenderPassDesc));
}

RenderPassDesc::~RenderPassDesc()
{
}

RenderPassDesc::RenderPassDesc(const RenderPassDesc &other)
{
    memcpy(this, &other, sizeof(RenderPassDesc));
}

void RenderPassDesc::packAttachment(uint32_t index, const ImageHelper &imageHelper)
{
    PackedAttachmentDesc &desc = mAttachmentDescs[index];

    // TODO(jmadill): We would only need this flag for duplicated attachments.
    desc.flags = VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT;
    ASSERT(imageHelper.getSamples() < std::numeric_limits<uint8_t>::max());
    desc.samples         = static_cast<uint8_t>(imageHelper.getSamples());
    const Format &format = imageHelper.getFormat();
    ASSERT(format.vkTextureFormat < std::numeric_limits<uint16_t>::max());
    desc.format = static_cast<uint16_t>(format.vkTextureFormat);
}

void RenderPassDesc::packColorAttachment(const ImageHelper &imageHelper)
{
    ASSERT(mDepthStencilAttachmentCount == 0);
    ASSERT(mColorAttachmentCount < gl::IMPLEMENTATION_MAX_DRAW_BUFFERS);
    packAttachment(mColorAttachmentCount++, imageHelper);
}

void RenderPassDesc::packDepthStencilAttachment(const ImageHelper &imageHelper)
{
    ASSERT(mDepthStencilAttachmentCount == 0);
    packAttachment(mColorAttachmentCount + mDepthStencilAttachmentCount++, imageHelper);
}

RenderPassDesc &RenderPassDesc::operator=(const RenderPassDesc &other)
{
    memcpy(this, &other, sizeof(RenderPassDesc));
    return *this;
}

size_t RenderPassDesc::hash() const
{
    return angle::ComputeGenericHash(*this);
}

uint32_t RenderPassDesc::attachmentCount() const
{
    return (mColorAttachmentCount + mDepthStencilAttachmentCount);
}

uint32_t RenderPassDesc::colorAttachmentCount() const
{
    return mColorAttachmentCount;
}

uint32_t RenderPassDesc::depthStencilAttachmentCount() const
{
    return mDepthStencilAttachmentCount;
}

const PackedAttachmentDesc &RenderPassDesc::operator[](size_t index) const
{
    ASSERT(index < mAttachmentDescs.size());
    return mAttachmentDescs[index];
}

bool operator==(const RenderPassDesc &lhs, const RenderPassDesc &rhs)
{
    return (memcmp(&lhs, &rhs, sizeof(RenderPassDesc)) == 0);
}

// PipelineDesc implementation.
// Use aligned allocation and free so we can use the alignas keyword.
void *PipelineDesc::operator new(std::size_t size)
{
    return angle::AlignedAlloc(size, 32);
}

void PipelineDesc::operator delete(void *ptr)
{
    return angle::AlignedFree(ptr);
}

PipelineDesc::PipelineDesc()
{
    memset(this, 0, sizeof(PipelineDesc));
}

PipelineDesc::~PipelineDesc()
{
}

PipelineDesc::PipelineDesc(const PipelineDesc &other)
{
    memcpy(this, &other, sizeof(PipelineDesc));
}

PipelineDesc &PipelineDesc::operator=(const PipelineDesc &other)
{
    memcpy(this, &other, sizeof(PipelineDesc));
    return *this;
}

size_t PipelineDesc::hash() const
{
    return angle::ComputeGenericHash(*this);
}

bool PipelineDesc::operator==(const PipelineDesc &other) const
{
    return (memcmp(this, &other, sizeof(PipelineDesc)) == 0);
}

void PipelineDesc::initDefaults()
{
    mInputAssemblyInfo.topology = static_cast<uint32_t>(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    mInputAssemblyInfo.primitiveRestartEnable = 0;

    mRasterizationStateInfo.depthClampEnable           = 0;
    mRasterizationStateInfo.rasterizationDiscardEnable = 0;
    mRasterizationStateInfo.polygonMode     = static_cast<uint16_t>(VK_POLYGON_MODE_FILL);
    mRasterizationStateInfo.cullMode        = static_cast<uint16_t>(VK_CULL_MODE_NONE);
    mRasterizationStateInfo.frontFace       = static_cast<uint16_t>(VK_FRONT_FACE_CLOCKWISE);
    mRasterizationStateInfo.depthBiasEnable = 0;
    mRasterizationStateInfo.depthBiasConstantFactor = 0.0f;
    mRasterizationStateInfo.depthBiasClamp          = 0.0f;
    mRasterizationStateInfo.depthBiasSlopeFactor    = 0.0f;
    mRasterizationStateInfo.lineWidth               = 1.0f;

    mMultisampleStateInfo.rasterizationSamples = 1;
    mMultisampleStateInfo.sampleShadingEnable  = 0;
    mMultisampleStateInfo.minSampleShading     = 0.0f;
    for (int maskIndex = 0; maskIndex < gl::MAX_SAMPLE_MASK_WORDS; ++maskIndex)
    {
        mMultisampleStateInfo.sampleMask[maskIndex] = 0;
    }
    mMultisampleStateInfo.alphaToCoverageEnable = 0;
    mMultisampleStateInfo.alphaToOneEnable      = 0;

    mDepthStencilStateInfo.depthTestEnable       = 0;
    mDepthStencilStateInfo.depthWriteEnable      = 1;
    mDepthStencilStateInfo.depthCompareOp        = static_cast<uint8_t>(VK_COMPARE_OP_LESS);
    mDepthStencilStateInfo.depthBoundsTestEnable = 0;
    mDepthStencilStateInfo.stencilTestEnable     = 0;
    mDepthStencilStateInfo.minDepthBounds        = 0.0f;
    mDepthStencilStateInfo.maxDepthBounds        = 0.0f;
    mDepthStencilStateInfo.front.failOp          = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.passOp          = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.depthFailOp     = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.front.compareOp       = static_cast<uint8_t>(VK_COMPARE_OP_ALWAYS);
    mDepthStencilStateInfo.front.compareMask     = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.front.writeMask       = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.front.reference       = 0;
    mDepthStencilStateInfo.back.failOp           = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.passOp           = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.depthFailOp      = static_cast<uint8_t>(VK_STENCIL_OP_KEEP);
    mDepthStencilStateInfo.back.compareOp        = static_cast<uint8_t>(VK_COMPARE_OP_ALWAYS);
    mDepthStencilStateInfo.back.compareMask      = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.back.writeMask        = static_cast<uint32_t>(-1);
    mDepthStencilStateInfo.back.reference        = 0;

    // TODO(jmadill): Blend state/MRT.
    PackedColorBlendAttachmentState blendAttachmentState;
    blendAttachmentState.blendEnable         = 0;
    blendAttachmentState.srcColorBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.dstColorBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.colorBlendOp        = static_cast<uint8_t>(VK_BLEND_OP_ADD);
    blendAttachmentState.srcAlphaBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.dstAlphaBlendFactor = static_cast<uint8_t>(VK_BLEND_FACTOR_ONE);
    blendAttachmentState.alphaBlendOp        = static_cast<uint8_t>(VK_BLEND_OP_ADD);
    blendAttachmentState.colorWriteMask =
        static_cast<uint8_t>(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                             VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT);

    mColorBlendStateInfo.logicOpEnable     = 0;
    mColorBlendStateInfo.logicOp           = static_cast<uint32_t>(VK_LOGIC_OP_CLEAR);
    mColorBlendStateInfo.attachmentCount   = 1;
    mColorBlendStateInfo.blendConstants[0] = 0.0f;
    mColorBlendStateInfo.blendConstants[1] = 0.0f;
    mColorBlendStateInfo.blendConstants[2] = 0.0f;
    mColorBlendStateInfo.blendConstants[3] = 0.0f;

    std::fill(&mColorBlendStateInfo.attachments[0],
              &mColorBlendStateInfo.attachments[gl::IMPLEMENTATION_MAX_DRAW_BUFFERS],
              blendAttachmentState);
}

Error PipelineDesc::initializePipeline(VkDevice device,
                                       const RenderPass &compatibleRenderPass,
                                       const PipelineLayout &pipelineLayout,
                                       const gl::AttributesMask &activeAttribLocationsMask,
                                       const ShaderModule &vertexModule,
                                       const ShaderModule &fragmentModule,
                                       Pipeline *pipelineOut) const
{
    VkPipelineShaderStageCreateInfo shaderStages[2];
    VkPipelineVertexInputStateCreateInfo vertexInputState;
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
    VkPipelineViewportStateCreateInfo viewportState;
    VkPipelineRasterizationStateCreateInfo rasterState;
    VkPipelineMultisampleStateCreateInfo multisampleState;
    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    std::array<VkPipelineColorBlendAttachmentState, gl::IMPLEMENTATION_MAX_DRAW_BUFFERS>
        blendAttachmentState;
    VkPipelineColorBlendStateCreateInfo blendState;
    VkGraphicsPipelineCreateInfo createInfo;

    shaderStages[0].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].pNext               = nullptr;
    shaderStages[0].flags               = 0;
    shaderStages[0].stage               = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module              = vertexModule.getHandle();
    shaderStages[0].pName               = "main";
    shaderStages[0].pSpecializationInfo = nullptr;

    shaderStages[1].sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].pNext               = nullptr;
    shaderStages[1].flags               = 0;
    shaderStages[1].stage               = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module              = fragmentModule.getHandle();
    shaderStages[1].pName               = "main";
    shaderStages[1].pSpecializationInfo = nullptr;

    // TODO(jmadill): Possibly use different path for ES 3.1 split bindings/attribs.
    gl::AttribArray<VkVertexInputBindingDescription> bindingDescs;
    gl::AttribArray<VkVertexInputAttributeDescription> attributeDescs;

    uint32_t vertexAttribCount = 0;

    for (size_t attribIndexSizeT : activeAttribLocationsMask)
    {
        const auto attribIndex = static_cast<uint32_t>(attribIndexSizeT);

        VkVertexInputBindingDescription &bindingDesc       = bindingDescs[attribIndex];
        VkVertexInputAttributeDescription &attribDesc      = attributeDescs[attribIndex];
        const PackedVertexInputBindingDesc &packedBinding  = mVertexInputBindings[attribIndex];
        const PackedVertexInputAttributeDesc &packedAttrib = mVertexInputAttribs[attribIndex];

        // TODO(jmadill): Support for gaps in vertex attribute specification.
        vertexAttribCount = attribIndex + 1;

        bindingDesc.binding   = attribIndex;
        bindingDesc.inputRate = static_cast<VkVertexInputRate>(packedBinding.inputRate);
        bindingDesc.stride    = static_cast<uint32_t>(packedBinding.stride);

        attribDesc.binding  = attribIndex;
        attribDesc.format   = static_cast<VkFormat>(packedAttrib.format);
        attribDesc.location = static_cast<uint32_t>(packedAttrib.location);
        attribDesc.offset   = packedAttrib.offset;
    }

    // The binding descriptions are filled in at draw time.
    vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputState.pNext = nullptr;
    vertexInputState.flags = 0;
    vertexInputState.vertexBindingDescriptionCount   = vertexAttribCount;
    vertexInputState.pVertexBindingDescriptions      = bindingDescs.data();
    vertexInputState.vertexAttributeDescriptionCount = vertexAttribCount;
    vertexInputState.pVertexAttributeDescriptions    = attributeDescs.data();

    // Primitive topology is filled in at draw time.
    inputAssemblyState.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyState.pNext    = nullptr;
    inputAssemblyState.flags    = 0;
    inputAssemblyState.topology = static_cast<VkPrimitiveTopology>(mInputAssemblyInfo.topology);
    inputAssemblyState.primitiveRestartEnable =
        static_cast<VkBool32>(mInputAssemblyInfo.primitiveRestartEnable);

    // Set initial viewport and scissor state.

    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext         = nullptr;
    viewportState.flags         = 0;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &mViewport;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &mScissor;

    // Rasterizer state.
    rasterState.sType            = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterState.pNext            = nullptr;
    rasterState.flags            = 0;
    rasterState.depthClampEnable = static_cast<VkBool32>(mRasterizationStateInfo.depthClampEnable);
    rasterState.rasterizerDiscardEnable =
        static_cast<VkBool32>(mRasterizationStateInfo.rasterizationDiscardEnable);
    rasterState.polygonMode     = static_cast<VkPolygonMode>(mRasterizationStateInfo.polygonMode);
    rasterState.cullMode        = static_cast<VkCullModeFlags>(mRasterizationStateInfo.cullMode);
    rasterState.frontFace       = static_cast<VkFrontFace>(mRasterizationStateInfo.frontFace);
    rasterState.depthBiasEnable = static_cast<VkBool32>(mRasterizationStateInfo.depthBiasEnable);
    rasterState.depthBiasConstantFactor = mRasterizationStateInfo.depthBiasConstantFactor;
    rasterState.depthBiasClamp          = mRasterizationStateInfo.depthBiasClamp;
    rasterState.depthBiasSlopeFactor    = mRasterizationStateInfo.depthBiasSlopeFactor;
    rasterState.lineWidth               = mRasterizationStateInfo.lineWidth;

    // Multisample state.
    multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampleState.pNext = nullptr;
    multisampleState.flags = 0;
    multisampleState.rasterizationSamples =
        gl_vk::GetSamples(mMultisampleStateInfo.rasterizationSamples);
    multisampleState.sampleShadingEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.sampleShadingEnable);
    multisampleState.minSampleShading = mMultisampleStateInfo.minSampleShading;
    // TODO(jmadill): sample masks
    multisampleState.pSampleMask = nullptr;
    multisampleState.alphaToCoverageEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.alphaToCoverageEnable);
    multisampleState.alphaToOneEnable =
        static_cast<VkBool32>(mMultisampleStateInfo.alphaToOneEnable);

    // Depth/stencil state.
    depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilState.pNext = nullptr;
    depthStencilState.flags = 0;
    depthStencilState.depthTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthTestEnable);
    depthStencilState.depthWriteEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthWriteEnable);
    depthStencilState.depthCompareOp =
        static_cast<VkCompareOp>(mDepthStencilStateInfo.depthCompareOp);
    depthStencilState.depthBoundsTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.depthBoundsTestEnable);
    depthStencilState.stencilTestEnable =
        static_cast<VkBool32>(mDepthStencilStateInfo.stencilTestEnable);
    UnpackStencilState(mDepthStencilStateInfo.front, &depthStencilState.front);
    UnpackStencilState(mDepthStencilStateInfo.back, &depthStencilState.back);
    depthStencilState.minDepthBounds = mDepthStencilStateInfo.minDepthBounds;
    depthStencilState.maxDepthBounds = mDepthStencilStateInfo.maxDepthBounds;

    blendState.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.pNext           = 0;
    blendState.flags           = 0;
    blendState.logicOpEnable   = static_cast<VkBool32>(mColorBlendStateInfo.logicOpEnable);
    blendState.logicOp         = static_cast<VkLogicOp>(mColorBlendStateInfo.logicOp);
    blendState.attachmentCount = mColorBlendStateInfo.attachmentCount;
    blendState.pAttachments    = blendAttachmentState.data();

    for (int i = 0; i < 4; i++)
    {
        blendState.blendConstants[i] = mColorBlendStateInfo.blendConstants[i];
    }

    for (uint32_t colorIndex = 0; colorIndex < blendState.attachmentCount; ++colorIndex)
    {
        UnpackBlendAttachmentState(mColorBlendStateInfo.attachments[colorIndex],
                                   &blendAttachmentState[colorIndex]);
    }

    // TODO(jmadill): Dynamic state.

    createInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    createInfo.pNext               = nullptr;
    createInfo.flags               = 0;
    createInfo.stageCount          = 2;
    createInfo.pStages             = shaderStages;
    createInfo.pVertexInputState   = &vertexInputState;
    createInfo.pInputAssemblyState = &inputAssemblyState;
    createInfo.pTessellationState  = nullptr;
    createInfo.pViewportState      = &viewportState;
    createInfo.pRasterizationState = &rasterState;
    createInfo.pMultisampleState   = &multisampleState;
    createInfo.pDepthStencilState  = &depthStencilState;
    createInfo.pColorBlendState    = &blendState;
    createInfo.pDynamicState       = nullptr;
    createInfo.layout              = pipelineLayout.getHandle();
    createInfo.renderPass          = compatibleRenderPass.getHandle();
    createInfo.subpass             = 0;
    createInfo.basePipelineHandle  = VK_NULL_HANDLE;
    createInfo.basePipelineIndex   = 0;

    ANGLE_TRY(pipelineOut->initGraphics(device, createInfo));

    return NoError();
}

const ShaderStageInfo &PipelineDesc::getShaderStageInfo() const
{
    return mShaderStageInfo;
}

void PipelineDesc::updateShaders(ProgramVk *programVk)
{
    ASSERT(programVk->getVertexModuleSerial() < std::numeric_limits<uint32_t>::max());
    mShaderStageInfo[0].moduleSerial =
        static_cast<uint32_t>(programVk->getVertexModuleSerial().getValue());
    ASSERT(programVk->getFragmentModuleSerial() < std::numeric_limits<uint32_t>::max());
    mShaderStageInfo[1].moduleSerial =
        static_cast<uint32_t>(programVk->getFragmentModuleSerial().getValue());
}

void PipelineDesc::updateViewport(const gl::Rectangle &viewport, float nearPlane, float farPlane)
{
    mViewport.x        = static_cast<float>(viewport.x);
    mViewport.y        = static_cast<float>(viewport.y);
    mViewport.width    = static_cast<float>(viewport.width);
    mViewport.height   = static_cast<float>(viewport.height);
    updateDepthRange(nearPlane, farPlane);
}

void PipelineDesc::updateDepthRange(float nearPlane, float farPlane)
{
    // GLES2.0 Section 2.12.1: Each of n and f are clamped to lie within [0, 1], as are all
    // arguments of type clampf.
    mViewport.minDepth = gl::clamp01(nearPlane);
    mViewport.maxDepth = gl::clamp01(farPlane);
}

void PipelineDesc::updateVertexInputInfo(const VertexInputBindings &bindings,
                                         const VertexInputAttributes &attribs)
{
    mVertexInputBindings = bindings;
    mVertexInputAttribs  = attribs;
}

void PipelineDesc::updateTopology(GLenum drawMode)
{
    mInputAssemblyInfo.topology = static_cast<uint32_t>(gl_vk::GetPrimitiveTopology(drawMode));
}

void PipelineDesc::updateCullMode(const gl::RasterizerState &rasterState)
{
    mRasterizationStateInfo.cullMode = static_cast<uint16_t>(gl_vk::GetCullMode(rasterState));
}

void PipelineDesc::updateFrontFace(const gl::RasterizerState &rasterState)
{
    mRasterizationStateInfo.frontFace =
        static_cast<uint16_t>(gl_vk::GetFrontFace(rasterState.frontFace));
}

void PipelineDesc::updateLineWidth(float lineWidth)
{
    mRasterizationStateInfo.lineWidth = lineWidth;
}

const RenderPassDesc &PipelineDesc::getRenderPassDesc() const
{
    return mRenderPassDesc;
}

void PipelineDesc::updateBlendColor(const gl::ColorF &color)
{
    mColorBlendStateInfo.blendConstants[0] = color.red;
    mColorBlendStateInfo.blendConstants[1] = color.green;
    mColorBlendStateInfo.blendConstants[2] = color.blue;
    mColorBlendStateInfo.blendConstants[3] = color.alpha;
}

void PipelineDesc::updateBlendEnabled(bool isBlendEnabled)
{
    for (auto &blendAttachmentState : mColorBlendStateInfo.attachments)
    {
        blendAttachmentState.blendEnable = isBlendEnabled;
    }
}

void PipelineDesc::updateBlendEquations(const gl::BlendState &blendState)
{
    for (auto &blendAttachmentState : mColorBlendStateInfo.attachments)
    {
        blendAttachmentState.colorBlendOp = PackGLBlendOp(blendState.blendEquationRGB);
        blendAttachmentState.alphaBlendOp = PackGLBlendOp(blendState.blendEquationAlpha);
    }
}

void PipelineDesc::updateBlendFuncs(const gl::BlendState &blendState)
{
    for (auto &blendAttachmentState : mColorBlendStateInfo.attachments)
    {
        blendAttachmentState.srcColorBlendFactor = PackGLBlendFactor(blendState.sourceBlendRGB);
        blendAttachmentState.dstColorBlendFactor = PackGLBlendFactor(blendState.destBlendRGB);
        blendAttachmentState.srcAlphaBlendFactor = PackGLBlendFactor(blendState.sourceBlendAlpha);
        blendAttachmentState.dstAlphaBlendFactor = PackGLBlendFactor(blendState.destBlendAlpha);
    }
}

void PipelineDesc::updateColorWriteMask(const gl::BlendState &blendState)
{
    for (auto &blendAttachmentState : mColorBlendStateInfo.attachments)
    {
        int colorMask = blendState.colorMaskRed ? VK_COLOR_COMPONENT_R_BIT : 0;
        colorMask |= blendState.colorMaskGreen ? VK_COLOR_COMPONENT_G_BIT : 0;
        colorMask |= blendState.colorMaskBlue ? VK_COLOR_COMPONENT_B_BIT : 0;
        colorMask |= blendState.colorMaskAlpha ? VK_COLOR_COMPONENT_A_BIT : 0;

        blendAttachmentState.colorWriteMask = static_cast<uint8_t>(colorMask);
    }
}

void PipelineDesc::updateDepthTestEnabled(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.depthTestEnable = static_cast<uint8_t>(depthStencilState.depthTest);
}

void PipelineDesc::updateDepthFunc(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.depthCompareOp = PackGLCompareFunc(depthStencilState.depthFunc);
}

void PipelineDesc::updateDepthWriteEnabled(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.depthWriteEnable = (depthStencilState.depthMask == GL_FALSE ? 0 : 1);
}

void PipelineDesc::updateStencilTestEnabled(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.stencilTestEnable = static_cast<uint8_t>(depthStencilState.stencilTest);
}

void PipelineDesc::updateStencilFrontFuncs(GLint ref,
                                           const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.front.reference   = ref;
    mDepthStencilStateInfo.front.compareOp   = PackGLCompareFunc(depthStencilState.stencilFunc);
    mDepthStencilStateInfo.front.compareMask = static_cast<uint32_t>(depthStencilState.stencilMask);
}

void PipelineDesc::updateStencilBackFuncs(GLint ref, const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.back.reference = ref;
    mDepthStencilStateInfo.back.compareOp = PackGLCompareFunc(depthStencilState.stencilBackFunc);
    mDepthStencilStateInfo.back.compareMask =
        static_cast<uint32_t>(depthStencilState.stencilBackMask);
}

void PipelineDesc::updateStencilFrontOps(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.front.passOp = PackGLStencilOp(depthStencilState.stencilPassDepthPass);
    mDepthStencilStateInfo.front.failOp = PackGLStencilOp(depthStencilState.stencilFail);
    mDepthStencilStateInfo.front.depthFailOp =
        PackGLStencilOp(depthStencilState.stencilPassDepthFail);
}

void PipelineDesc::updateStencilBackOps(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.back.passOp =
        PackGLStencilOp(depthStencilState.stencilBackPassDepthPass);
    mDepthStencilStateInfo.back.failOp = PackGLStencilOp(depthStencilState.stencilBackFail);
    mDepthStencilStateInfo.back.depthFailOp =
        PackGLStencilOp(depthStencilState.stencilBackPassDepthFail);
}

void PipelineDesc::updateStencilFrontWriteMask(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.front.writeMask =
        static_cast<uint32_t>(depthStencilState.stencilWritemask);
}

void PipelineDesc::updateStencilBackWriteMask(const gl::DepthStencilState &depthStencilState)
{
    mDepthStencilStateInfo.back.writeMask =
        static_cast<uint32_t>(depthStencilState.stencilBackWritemask);
}

void PipelineDesc::updateRenderPassDesc(const RenderPassDesc &renderPassDesc)
{
    mRenderPassDesc = renderPassDesc;
}

void PipelineDesc::updateScissor(const gl::Rectangle &rect)
{
    mScissor = gl_vk::GetRect(rect);
}

// AttachmentOpsArray implementation.
AttachmentOpsArray::AttachmentOpsArray()
{
    memset(&mOps, 0, sizeof(PackedAttachmentOpsDesc) * mOps.size());
}

AttachmentOpsArray::~AttachmentOpsArray()
{
}

AttachmentOpsArray::AttachmentOpsArray(const AttachmentOpsArray &other)
{
    memcpy(&mOps, &other.mOps, sizeof(PackedAttachmentOpsDesc) * mOps.size());
}

AttachmentOpsArray &AttachmentOpsArray::operator=(const AttachmentOpsArray &other)
{
    memcpy(&mOps, &other.mOps, sizeof(PackedAttachmentOpsDesc) * mOps.size());
    return *this;
}

const PackedAttachmentOpsDesc &AttachmentOpsArray::operator[](size_t index) const
{
    return mOps[index];
}

PackedAttachmentOpsDesc &AttachmentOpsArray::operator[](size_t index)
{
    return mOps[index];
}

void AttachmentOpsArray::initDummyOp(size_t index,
                                     VkImageLayout initialLayout,
                                     VkImageLayout finalLayout)
{
    PackedAttachmentOpsDesc &ops = mOps[index];

    ops.loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD;
    ops.storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
    ops.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    ops.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    ops.initialLayout  = static_cast<uint16_t>(initialLayout);
    ops.finalLayout    = static_cast<uint16_t>(finalLayout);
}

size_t AttachmentOpsArray::hash() const
{
    return angle::ComputeGenericHash(mOps);
}

bool operator==(const AttachmentOpsArray &lhs, const AttachmentOpsArray &rhs)
{
    return (memcmp(&lhs, &rhs, sizeof(AttachmentOpsArray)) == 0);
}
}  // namespace vk

// RenderPassCache implementation.
RenderPassCache::RenderPassCache()
{
}

RenderPassCache::~RenderPassCache()
{
    ASSERT(mPayload.empty());
}

void RenderPassCache::destroy(VkDevice device)
{
    for (auto &outerIt : mPayload)
    {
        for (auto &innerIt : outerIt.second)
        {
            innerIt.second.get().destroy(device);
        }
    }
    mPayload.clear();
}

vk::Error RenderPassCache::getCompatibleRenderPass(VkDevice device,
                                                   Serial serial,
                                                   const vk::RenderPassDesc &desc,
                                                   vk::RenderPass **renderPassOut)
{
    auto outerIt = mPayload.find(desc);
    if (outerIt != mPayload.end())
    {
        InnerCache &innerCache = outerIt->second;
        ASSERT(!innerCache.empty());

        // Find the first element and return it.
        innerCache.begin()->second.updateSerial(serial);
        *renderPassOut = &innerCache.begin()->second.get();
        return vk::NoError();
    }

    // Insert some dummy attachment ops.
    // TODO(jmadill): Pre-populate the cache in the Renderer so we rarely miss here.
    vk::AttachmentOpsArray ops;
    for (uint32_t colorIndex = 0; colorIndex < desc.colorAttachmentCount(); ++colorIndex)
    {
        ops.initDummyOp(colorIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
    }

    if (desc.depthStencilAttachmentCount() > 0)
    {
        ops.initDummyOp(desc.colorAttachmentCount(),
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    }

    return getRenderPassWithOps(device, serial, desc, ops, renderPassOut);
}

vk::Error RenderPassCache::getRenderPassWithOps(VkDevice device,
                                                Serial serial,
                                                const vk::RenderPassDesc &desc,
                                                const vk::AttachmentOpsArray &attachmentOps,
                                                vk::RenderPass **renderPassOut)
{
    auto outerIt = mPayload.find(desc);
    if (outerIt != mPayload.end())
    {
        InnerCache &innerCache = outerIt->second;

        auto innerIt = innerCache.find(attachmentOps);
        if (innerIt != innerCache.end())
        {
            // Update the serial before we return.
            // TODO(jmadill): Could possibly use an MRU cache here.
            innerIt->second.updateSerial(serial);
            *renderPassOut = &innerIt->second.get();
            return vk::NoError();
        }
    }
    else
    {
        auto emplaceResult = mPayload.emplace(desc, InnerCache());
        outerIt            = emplaceResult.first;
    }

    vk::RenderPass newRenderPass;
    ANGLE_TRY(vk::InitializeRenderPassFromDesc(device, desc, attachmentOps, &newRenderPass));

    vk::RenderPassAndSerial withSerial(std::move(newRenderPass), serial);

    InnerCache &innerCache = outerIt->second;
    auto insertPos         = innerCache.emplace(attachmentOps, std::move(withSerial));
    *renderPassOut         = &insertPos.first->second.get();

    // TODO(jmadill): Trim cache, and pre-populate with the most common RPs on startup.
    return vk::NoError();
}

// PipelineCache implementation.
PipelineCache::PipelineCache()
{
}

PipelineCache::~PipelineCache()
{
    ASSERT(mPayload.empty());
}

void PipelineCache::destroy(VkDevice device)
{
    for (auto &item : mPayload)
    {
        item.second.get().destroy(device);
    }

    mPayload.clear();
}

vk::Error PipelineCache::getPipeline(VkDevice device,
                                     const vk::RenderPass &compatibleRenderPass,
                                     const vk::PipelineLayout &pipelineLayout,
                                     const gl::AttributesMask &activeAttribLocationsMask,
                                     const vk::ShaderModule &vertexModule,
                                     const vk::ShaderModule &fragmentModule,
                                     const vk::PipelineDesc &desc,
                                     vk::PipelineAndSerial **pipelineOut)
{
    auto item = mPayload.find(desc);
    if (item != mPayload.end())
    {
        *pipelineOut = &item->second;
        return vk::NoError();
    }

    vk::Pipeline newPipeline;

    // This "if" is left here for the benefit of VulkanPipelineCachePerfTest.
    if (device != VK_NULL_HANDLE)
    {
        ANGLE_TRY(desc.initializePipeline(device, compatibleRenderPass, pipelineLayout,
                                          activeAttribLocationsMask, vertexModule, fragmentModule,
                                          &newPipeline));
    }

    // The Serial will be updated outside of this query.
    auto insertedItem =
        mPayload.emplace(desc, vk::PipelineAndSerial(std::move(newPipeline), Serial()));
    *pipelineOut = &insertedItem.first->second;

    return vk::NoError();
}

void PipelineCache::populate(const vk::PipelineDesc &desc, vk::Pipeline &&pipeline)
{
    auto item = mPayload.find(desc);
    if (item != mPayload.end())
    {
        return;
    }

    mPayload.emplace(desc, vk::PipelineAndSerial(std::move(pipeline), Serial()));
}

}  // namespace rx

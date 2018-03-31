//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramVk.cpp:
//    Implements the class methods for ProgramVk.
//

#include "libANGLE/renderer/vulkan/ProgramVk.h"

#include "common/debug.h"
#include "common/utilities.h"
#include "libANGLE/Context.h"
#include "libANGLE/renderer/vulkan/ContextVk.h"
#include "libANGLE/renderer/vulkan/GlslangWrapper.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/TextureVk.h"

namespace rx
{

namespace
{

constexpr size_t kUniformBlockDynamicBufferMinSize = 256 * 128;

gl::Error InitDefaultUniformBlock(const gl::Context *context,
                                  gl::Shader *shader,
                                  sh::BlockLayoutMap *blockLayoutMapOut,
                                  size_t *blockSizeOut)
{
    const auto &uniforms = shader->getUniforms(context);

    if (uniforms.empty())
    {
        *blockSizeOut = 0;
        return gl::NoError();
    }

    sh::Std140BlockEncoder blockEncoder;
    sh::GetUniformBlockInfo(uniforms, "", &blockEncoder, blockLayoutMapOut);

    size_t blockSize = blockEncoder.getBlockSize();

    // TODO(jmadill): I think we still need a valid block for the pipeline even if zero sized.
    if (blockSize == 0)
    {
        *blockSizeOut = 0;
        return gl::NoError();
    }

    *blockSizeOut = blockSize;
    return gl::NoError();
}

template <typename T>
void UpdateDefaultUniformBlock(GLsizei count,
                               uint32_t arrayIndex,
                               int componentCount,
                               const T *v,
                               const sh::BlockMemberInfo &layoutInfo,
                               angle::MemoryBuffer *uniformData)
{
    const int elementSize = sizeof(T) * componentCount;

    uint8_t *dst = uniformData->data() + layoutInfo.offset;
    if (layoutInfo.arrayStride == 0 || layoutInfo.arrayStride == elementSize)
    {
        uint32_t arrayOffset = arrayIndex * layoutInfo.arrayStride;
        uint8_t *writePtr    = dst + arrayOffset;
        memcpy(writePtr, v, elementSize * count);
    }
    else
    {
        // Have to respect the arrayStride between each element of the array.
        int maxIndex = arrayIndex + count;
        for (int writeIndex = arrayIndex, readIndex = 0; writeIndex < maxIndex;
             writeIndex++, readIndex++)
        {
            const int arrayOffset = writeIndex * layoutInfo.arrayStride;
            uint8_t *writePtr     = dst + arrayOffset;
            const T *readPtr      = v + readIndex;
            memcpy(writePtr, readPtr, elementSize);
        }
    }
}

template <typename T>
void ReadFromDefaultUniformBlock(int componentCount,
                                 uint32_t arrayIndex,
                                 T *dst,
                                 const sh::BlockMemberInfo &layoutInfo,
                                 const angle::MemoryBuffer *uniformData)
{
    ASSERT(layoutInfo.offset != -1);

    const int elementSize = sizeof(T) * componentCount;
    const uint8_t *source = uniformData->data() + layoutInfo.offset;

    if (layoutInfo.arrayStride == 0 || layoutInfo.arrayStride == elementSize)
    {
        const uint8_t *readPtr = source + arrayIndex * layoutInfo.arrayStride;
        memcpy(dst, readPtr, elementSize);
    }
    else
    {
        // Have to respect the arrayStride between each element of the array.
        const int arrayOffset  = arrayIndex * layoutInfo.arrayStride;
        const uint8_t *readPtr = source + arrayOffset;
        memcpy(dst, readPtr, elementSize);
    }
}

vk::Error SyncDefaultUniformBlock(RendererVk *renderer,
                                  vk::DynamicBuffer *dynamicBuffer,
                                  const angle::MemoryBuffer &bufferData,
                                  uint32_t *outOffset,
                                  bool *outBufferModified)
{
    ASSERT(!bufferData.empty());
    uint8_t *data       = nullptr;
    VkBuffer *outBuffer = nullptr;
    uint32_t offset;
    ANGLE_TRY(dynamicBuffer->allocate(renderer, bufferData.size(), &data, outBuffer, &offset,
                                      outBufferModified));
    *outOffset = offset;
    memcpy(data, bufferData.data(), bufferData.size());
    ANGLE_TRY(dynamicBuffer->flush(renderer->getDevice()));
    return vk::NoError();
}

// TODO(jiawei.shao@intel.com): Fully remove this enum by gl::ShaderType. (BUG=angleproject:2169)
enum ShaderIndex : uint32_t
{
    MinShaderIndex = 0,
    VertexShader   = MinShaderIndex,
    FragmentShader = 1,
    MaxShaderIndex = kShaderTypeCount,
};

gl::Shader *GetShader(const gl::ProgramState &programState, uint32_t shaderIndex)
{
    switch (shaderIndex)
    {
        case VertexShader:
            return programState.getAttachedShader(gl::ShaderType::Vertex);
        case FragmentShader:
            return programState.getAttachedShader(gl::ShaderType::Fragment);
        default:
            UNREACHABLE();
            return nullptr;
    }
}

}  // anonymous namespace

ProgramVk::DefaultUniformBlock::DefaultUniformBlock()
    : storage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
              kUniformBlockDynamicBufferMinSize),
      uniformData(),
      uniformsDirty(false),
      uniformLayout()
{
}

ProgramVk::DefaultUniformBlock::~DefaultUniformBlock()
{
}

ProgramVk::ProgramVk(const gl::ProgramState &state)
    : ProgramImpl(state),
      mDefaultUniformBlocks(),
      mUniformBlocksOffsets(),
      mUsedDescriptorSetRange(),
      mDirtyTextures(true)
{
    mUniformBlocksOffsets.fill(0);
    mUsedDescriptorSetRange.invalidate();
}

ProgramVk::~ProgramVk()
{
}

gl::Error ProgramVk::destroy(const gl::Context *contextImpl)
{
    ContextVk *contextVk = vk::GetImpl(contextImpl);
    return reset(contextVk);
}

vk::Error ProgramVk::reset(ContextVk *contextVk)
{
    // TODO(jmadill): Handle re-linking a program that is in-use. http://anglebug.com/2397

    VkDevice device = contextVk->getDevice();

    for (auto &uniformBlock : mDefaultUniformBlocks)
    {
        uniformBlock.storage.destroy(device);
    }

    mEmptyUniformBlockStorage.memory.destroy(device);
    mEmptyUniformBlockStorage.buffer.destroy(device);

    mLinkedFragmentModule.destroy(device);
    mLinkedVertexModule.destroy(device);
    mVertexModuleSerial   = Serial();
    mFragmentModuleSerial = Serial();

    mDescriptorSets.clear();
    mUsedDescriptorSetRange.invalidate();
    mDirtyTextures       = false;

    return vk::NoError();
}

gl::LinkResult ProgramVk::load(const gl::Context *contextImpl,
                               gl::InfoLog &infoLog,
                               gl::BinaryInputStream *stream)
{
    UNIMPLEMENTED();
    return gl::InternalError();
}

void ProgramVk::save(const gl::Context *context, gl::BinaryOutputStream *stream)
{
    UNIMPLEMENTED();
}

void ProgramVk::setBinaryRetrievableHint(bool retrievable)
{
    UNIMPLEMENTED();
}

void ProgramVk::setSeparable(bool separable)
{
    UNIMPLEMENTED();
}

gl::LinkResult ProgramVk::link(const gl::Context *glContext,
                               const gl::ProgramLinkedResources &resources,
                               gl::InfoLog &infoLog)
{
    ContextVk *contextVk           = vk::GetImpl(glContext);
    RendererVk *renderer           = contextVk->getRenderer();
    GlslangWrapper *glslangWrapper = renderer->getGlslangWrapper();
    VkDevice device                = renderer->getDevice();

    ANGLE_TRY(reset(contextVk));

    std::vector<uint32_t> vertexCode;
    std::vector<uint32_t> fragmentCode;
    bool linkSuccess = false;
    ANGLE_TRY_RESULT(
        glslangWrapper->linkProgram(glContext, mState, resources, &vertexCode, &fragmentCode),
        linkSuccess);
    if (!linkSuccess)
    {
        return false;
    }

    {
        VkShaderModuleCreateInfo vertexShaderInfo;
        vertexShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        vertexShaderInfo.pNext    = nullptr;
        vertexShaderInfo.flags    = 0;
        vertexShaderInfo.codeSize = vertexCode.size() * sizeof(uint32_t);
        vertexShaderInfo.pCode    = vertexCode.data();

        ANGLE_TRY(mLinkedVertexModule.init(device, vertexShaderInfo));
        mVertexModuleSerial = renderer->issueProgramSerial();
    }

    {
        VkShaderModuleCreateInfo fragmentShaderInfo;
        fragmentShaderInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        fragmentShaderInfo.pNext    = nullptr;
        fragmentShaderInfo.flags    = 0;
        fragmentShaderInfo.codeSize = fragmentCode.size() * sizeof(uint32_t);
        fragmentShaderInfo.pCode    = fragmentCode.data();

        ANGLE_TRY(mLinkedFragmentModule.init(device, fragmentShaderInfo));
        mFragmentModuleSerial = renderer->issueProgramSerial();
    }

    ANGLE_TRY(initDefaultUniformBlocks(glContext));

    if (!mState.getSamplerUniformRange().empty())
    {
        // Ensure the descriptor set range includes the textures at position 1.
        mUsedDescriptorSetRange.extend(1);
        mDirtyTextures = true;
    }

    return true;
}

gl::Error ProgramVk::initDefaultUniformBlocks(const gl::Context *glContext)
{
    ContextVk *contextVk = vk::GetImpl(glContext);
    RendererVk *renderer = contextVk->getRenderer();
    VkDevice device      = contextVk->getDevice();

    // Process vertex and fragment uniforms into std140 packing.
    std::array<sh::BlockLayoutMap, MaxShaderIndex> layoutMap;
    std::array<size_t, MaxShaderIndex> requiredBufferSize = {{0, 0}};

    for (uint32_t shaderIndex = MinShaderIndex; shaderIndex < MaxShaderIndex; ++shaderIndex)
    {
        ANGLE_TRY(InitDefaultUniformBlock(glContext, GetShader(mState, shaderIndex),
                                          &layoutMap[shaderIndex],
                                          &requiredBufferSize[shaderIndex]));
    }

    // Init the default block layout info.
    const auto &locations = mState.getUniformLocations();
    const auto &uniforms  = mState.getUniforms();
    for (size_t locationIndex = 0; locationIndex < locations.size(); ++locationIndex)
    {
        std::array<sh::BlockMemberInfo, MaxShaderIndex> layoutInfo;

        const auto &location = locations[locationIndex];
        if (location.used() && !location.ignored)
        {
            const auto &uniform = uniforms[location.index];

            if (uniform.isSampler())
                continue;

            std::string uniformName = uniform.name;
            if (uniform.isArray())
            {
                // Gets the uniform name without the [0] at the end.
                uniformName = gl::ParseResourceName(uniformName, nullptr);
            }

            bool found = false;

            for (uint32_t shaderIndex = MinShaderIndex; shaderIndex < MaxShaderIndex; ++shaderIndex)
            {
                auto it = layoutMap[shaderIndex].find(uniformName);
                if (it != layoutMap[shaderIndex].end())
                {
                    found                   = true;
                    layoutInfo[shaderIndex] = it->second;
                }
            }

            ASSERT(found);
        }

        for (uint32_t shaderIndex = MinShaderIndex; shaderIndex < MaxShaderIndex; ++shaderIndex)
        {
            mDefaultUniformBlocks[shaderIndex].uniformLayout.push_back(layoutInfo[shaderIndex]);
        }
    }

    bool anyDirty = false;
    bool allDirty = true;

    for (uint32_t shaderIndex = MinShaderIndex; shaderIndex < MaxShaderIndex; ++shaderIndex)
    {
        if (requiredBufferSize[shaderIndex] > 0)
        {
            if (!mDefaultUniformBlocks[shaderIndex].uniformData.resize(
                    requiredBufferSize[shaderIndex]))
            {
                return gl::OutOfMemory() << "Memory allocation failure.";
            }
            size_t minAlignment = static_cast<size_t>(
                renderer->getPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment);

            mDefaultUniformBlocks[shaderIndex].storage.init(minAlignment);

            // Initialize uniform buffer memory to zero by default.
            mDefaultUniformBlocks[shaderIndex].uniformData.fill(0);
            mDefaultUniformBlocks[shaderIndex].uniformsDirty = true;

            anyDirty = true;
        }
        else
        {
            allDirty = false;
        }
    }

    if (anyDirty)
    {
        // Initialize the "empty" uniform block if necessary.
        if (!allDirty)
        {
            VkBufferCreateInfo uniformBufferInfo;
            uniformBufferInfo.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            uniformBufferInfo.pNext                 = nullptr;
            uniformBufferInfo.flags                 = 0;
            uniformBufferInfo.size                  = 1;
            uniformBufferInfo.usage                 = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            uniformBufferInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
            uniformBufferInfo.queueFamilyIndexCount = 0;
            uniformBufferInfo.pQueueFamilyIndices   = nullptr;

            ANGLE_TRY(mEmptyUniformBlockStorage.buffer.init(device, uniformBufferInfo));

            // Assume host visible/coherent memory available.
            VkMemoryPropertyFlags flags =
                (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            size_t requiredSize = 0;
            ANGLE_TRY(AllocateBufferMemory(renderer, flags, &mEmptyUniformBlockStorage.buffer,
                                           &mEmptyUniformBlockStorage.memory, &requiredSize));
        }

        // Ensure the descriptor set range includes the uniform buffers at position 0.
        mUsedDescriptorSetRange.extend(0);
    }

    return gl::NoError();
}

GLboolean ProgramVk::validate(const gl::Caps &caps, gl::InfoLog *infoLog)
{
    UNIMPLEMENTED();
    return GLboolean();
}

template <typename T>
void ProgramVk::setUniformImpl(GLint location, GLsizei count, const T *v, GLenum entryPointType)
{
    const gl::VariableLocation &locationInfo = mState.getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = mState.getUniforms()[locationInfo.index];

    if (linkedUniform.isSampler())
    {
        UNIMPLEMENTED();
        return;
    }

    if (linkedUniform.typeInfo->type == entryPointType)
    {
        for (auto &uniformBlock : mDefaultUniformBlocks)
        {
            const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

            // Assume an offset of -1 means the block is unused.
            if (layoutInfo.offset == -1)
            {
                continue;
            }

            const GLint componentCount = linkedUniform.typeInfo->componentCount;
            UpdateDefaultUniformBlock(count, locationInfo.arrayIndex, componentCount, v, layoutInfo,
                                      &uniformBlock.uniformData);
            uniformBlock.uniformsDirty = true;
        }
    }
    else
    {
        for (auto &uniformBlock : mDefaultUniformBlocks)
        {
            const sh::BlockMemberInfo &layoutInfo = uniformBlock.uniformLayout[location];

            // Assume an offset of -1 means the block is unused.
            if (layoutInfo.offset == -1)
            {
                continue;
            }

            const GLint componentCount = linkedUniform.typeInfo->componentCount;

            ASSERT(linkedUniform.typeInfo->type == gl::VariableBoolVectorType(entryPointType));

            GLint initialArrayOffset = locationInfo.arrayIndex * layoutInfo.arrayStride;
            for (GLint i = 0; i < count; i++)
            {
                GLint elementOffset = i * layoutInfo.arrayStride + initialArrayOffset;
                GLint *dest =
                    reinterpret_cast<GLint *>(uniformBlock.uniformData.data() + elementOffset);
                const T *source = v + i * componentCount;

                for (int c = 0; c < componentCount; c++)
                {
                    dest[c] = (source[c] == static_cast<T>(0)) ? GL_FALSE : GL_TRUE;
                }
            }
            uniformBlock.uniformsDirty = true;
        }
    }
}

template <typename T>
void ProgramVk::getUniformImpl(GLint location, T *v, GLenum entryPointType) const
{
    const gl::VariableLocation &locationInfo = mState.getUniformLocations()[location];
    const gl::LinkedUniform &linkedUniform   = mState.getUniforms()[locationInfo.index];

    if (linkedUniform.isSampler())
    {
        UNIMPLEMENTED();
        return;
    }

    const gl::ShaderType shaderType = linkedUniform.getFirstShaderTypeWhereActive();
    ASSERT(shaderType != gl::ShaderType::InvalidEnum);

    const DefaultUniformBlock &uniformBlock =
        mDefaultUniformBlocks[static_cast<GLuint>(shaderType)];
    const sh::BlockMemberInfo &layoutInfo   = uniformBlock.uniformLayout[location];

    ASSERT(linkedUniform.typeInfo->componentType == entryPointType ||
           linkedUniform.typeInfo->componentType == gl::VariableBoolVectorType(entryPointType));
    ReadFromDefaultUniformBlock(linkedUniform.typeInfo->componentCount, locationInfo.arrayIndex, v,
                                layoutInfo, &uniformBlock.uniformData);
}

void ProgramVk::setUniform1fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniformImpl(location, count, v, GL_FLOAT);
}

void ProgramVk::setUniform2fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniformImpl(location, count, v, GL_FLOAT_VEC2);
}

void ProgramVk::setUniform3fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniformImpl(location, count, v, GL_FLOAT_VEC3);
}

void ProgramVk::setUniform4fv(GLint location, GLsizei count, const GLfloat *v)
{
    setUniformImpl(location, count, v, GL_FLOAT_VEC4);
}

void ProgramVk::setUniform1iv(GLint location, GLsizei count, const GLint *v)
{
    setUniformImpl(location, count, v, GL_INT);
}

void ProgramVk::setUniform2iv(GLint location, GLsizei count, const GLint *v)
{
    setUniformImpl(location, count, v, GL_INT_VEC2);
}

void ProgramVk::setUniform3iv(GLint location, GLsizei count, const GLint *v)
{
    setUniformImpl(location, count, v, GL_INT_VEC3);
}

void ProgramVk::setUniform4iv(GLint location, GLsizei count, const GLint *v)
{
    setUniformImpl(location, count, v, GL_INT_VEC4);
}

void ProgramVk::setUniform1uiv(GLint location, GLsizei count, const GLuint *v)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniform2uiv(GLint location, GLsizei count, const GLuint *v)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniform3uiv(GLint location, GLsizei count, const GLuint *v)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniform4uiv(GLint location, GLsizei count, const GLuint *v)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix2fv(GLint location,
                                    GLsizei count,
                                    GLboolean transpose,
                                    const GLfloat *value)
{
    if (transpose == GL_TRUE)
    {
        UNIMPLEMENTED();
        return;
    }

    setUniformImpl(location, count, value, GL_FLOAT_MAT2);
}

void ProgramVk::setUniformMatrix3fv(GLint location,
                                    GLsizei count,
                                    GLboolean transpose,
                                    const GLfloat *value)
{
    if (transpose == GL_TRUE)
    {
        UNIMPLEMENTED();
        return;
    }
    setUniformImpl(location, count, value, GL_FLOAT_MAT3);
}

void ProgramVk::setUniformMatrix4fv(GLint location,
                                    GLsizei count,
                                    GLboolean transpose,
                                    const GLfloat *value)
{
    if (transpose == GL_TRUE)
    {
        UNIMPLEMENTED();
        return;
    }

    setUniformImpl(location, count, value, GL_FLOAT_MAT4);
}

void ProgramVk::setUniformMatrix2x3fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix3x2fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix2x4fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix4x2fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix3x4fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformMatrix4x3fv(GLint location,
                                      GLsizei count,
                                      GLboolean transpose,
                                      const GLfloat *value)
{
    UNIMPLEMENTED();
}

void ProgramVk::setUniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBinding)
{
    UNIMPLEMENTED();
}

void ProgramVk::setPathFragmentInputGen(const std::string &inputName,
                                        GLenum genMode,
                                        GLint components,
                                        const GLfloat *coeffs)
{
    UNIMPLEMENTED();
}

const vk::ShaderModule &ProgramVk::getLinkedVertexModule() const
{
    ASSERT(mLinkedVertexModule.getHandle() != VK_NULL_HANDLE);
    return mLinkedVertexModule;
}

Serial ProgramVk::getVertexModuleSerial() const
{
    return mVertexModuleSerial;
}

const vk::ShaderModule &ProgramVk::getLinkedFragmentModule() const
{
    ASSERT(mLinkedFragmentModule.getHandle() != VK_NULL_HANDLE);
    return mLinkedFragmentModule;
}

Serial ProgramVk::getFragmentModuleSerial() const
{
    return mFragmentModuleSerial;
}

vk::Error ProgramVk::allocateDescriptorSet(ContextVk *contextVk, uint32_t descriptorSetIndex)
{
    RendererVk *renderer = contextVk->getRenderer();

    // Write out to a new a descriptor set.
    vk::DynamicDescriptorPool *dynamicDescriptorPool = contextVk->getDynamicDescriptorPool();
    const auto &descriptorSetLayouts = renderer->getGraphicsDescriptorSetLayouts();

    uint32_t potentialNewCount = descriptorSetIndex + 1;
    if (potentialNewCount > mDescriptorSets.size())
    {
        mDescriptorSets.resize(potentialNewCount, VK_NULL_HANDLE);
    }

    const VkDescriptorSetLayout *descriptorSetLayout =
        descriptorSetLayouts[descriptorSetIndex].ptr();

    ANGLE_TRY(dynamicDescriptorPool->allocateDescriptorSets(contextVk, descriptorSetLayout, 1,
                                                            &mDescriptorSets[descriptorSetIndex]));
    return vk::NoError();
}

void ProgramVk::getUniformfv(const gl::Context *context, GLint location, GLfloat *params) const
{
    getUniformImpl(location, params, GL_FLOAT);
}

void ProgramVk::getUniformiv(const gl::Context *context, GLint location, GLint *params) const
{
    getUniformImpl(location, params, GL_INT);
}

void ProgramVk::getUniformuiv(const gl::Context *context, GLint location, GLuint *params) const
{
    UNIMPLEMENTED();
}

vk::Error ProgramVk::updateUniforms(ContextVk *contextVk)
{
    if (!mDefaultUniformBlocks[VertexShader].uniformsDirty &&
        !mDefaultUniformBlocks[FragmentShader].uniformsDirty)
    {
        return vk::NoError();
    }

    ASSERT(mUsedDescriptorSetRange.contains(0));

    // Update buffer memory by immediate mapping. This immediate update only works once.
    // TODO(jmadill): Handle inserting updates into the command stream, or use dynamic buffers.
    bool anyNewBufferAllocated = false;
    for (size_t index = 0; index < mDefaultUniformBlocks.size(); index++)
    {
        DefaultUniformBlock &uniformBlock = mDefaultUniformBlocks[index];

        if (uniformBlock.uniformsDirty)
        {
            bool bufferModified = false;
            ANGLE_TRY(SyncDefaultUniformBlock(contextVk->getRenderer(), &uniformBlock.storage,
                                              uniformBlock.uniformData,
                                              &mUniformBlocksOffsets[index], &bufferModified));
            uniformBlock.uniformsDirty = false;

            if (bufferModified)
            {
                anyNewBufferAllocated = true;
            }
        }
    }

    if (anyNewBufferAllocated)
    {
        // We need to reinitialize the descriptor sets if we newly allocated buffers since we can't
        // modify the descriptor sets once initialized.
        ANGLE_TRY(allocateDescriptorSet(contextVk, vk::UniformBufferIndex));
        ANGLE_TRY(updateDefaultUniformsDescriptorSet(contextVk));
    }

    return vk::NoError();
}

vk::Error ProgramVk::updateDefaultUniformsDescriptorSet(ContextVk *contextVk)
{
    std::array<VkDescriptorBufferInfo, MaxShaderIndex> descriptorBufferInfo;
    std::array<VkWriteDescriptorSet, MaxShaderIndex> writeDescriptorInfo;
    uint32_t bufferCount = 0;

    for (auto &uniformBlock : mDefaultUniformBlocks)
    {
        auto &bufferInfo = descriptorBufferInfo[bufferCount];
        auto &writeInfo  = writeDescriptorInfo[bufferCount];

        if (!uniformBlock.uniformData.empty())
        {
            bufferInfo.buffer = uniformBlock.storage.getCurrentBufferHandle();
        }
        else
        {
            bufferInfo.buffer = mEmptyUniformBlockStorage.buffer.getHandle();
        }

        bufferInfo.offset = 0;
        bufferInfo.range  = VK_WHOLE_SIZE;

        writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.pNext            = nullptr;
        writeInfo.dstSet           = mDescriptorSets[0];
        writeInfo.dstBinding       = bufferCount;
        writeInfo.dstArrayElement  = 0;
        writeInfo.descriptorCount  = 1;
        writeInfo.descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        writeInfo.pImageInfo       = nullptr;
        writeInfo.pBufferInfo      = &bufferInfo;
        writeInfo.pTexelBufferView = nullptr;

        bufferCount++;
    }

    VkDevice device = contextVk->getDevice();

    vkUpdateDescriptorSets(device, bufferCount, writeDescriptorInfo.data(), 0, nullptr);

    return vk::NoError();
}

const std::vector<VkDescriptorSet> &ProgramVk::getDescriptorSets() const
{
    return mDescriptorSets;
}

const uint32_t *ProgramVk::getDynamicOffsets()
{
    // If we have no descriptor set being used, we do not need to specify any offsets when binding
    // the descriptor sets.
    if (!mUsedDescriptorSetRange.contains(0))
        return nullptr;

    return mUniformBlocksOffsets.data();
}

uint32_t ProgramVk::getDynamicOffsetsCount()
{
    if (!mUsedDescriptorSetRange.contains(0))
        return 0;

    return static_cast<uint32_t>(mUniformBlocksOffsets.size());
}

const gl::RangeUI &ProgramVk::getUsedDescriptorSetRange() const
{
    return mUsedDescriptorSetRange;
}

vk::Error ProgramVk::updateTexturesDescriptorSet(ContextVk *contextVk)
{
    if (mState.getSamplerBindings().empty() || !mDirtyTextures)
    {
        return vk::NoError();
    }

    ANGLE_TRY(allocateDescriptorSet(contextVk, vk::TextureIndex));

    ASSERT(mUsedDescriptorSetRange.contains(1));
    VkDescriptorSet descriptorSet = mDescriptorSets[1];

    // TODO(jmadill): Don't hard-code the texture limit.
    ShaderTextureArray<VkDescriptorImageInfo> descriptorImageInfo;
    ShaderTextureArray<VkWriteDescriptorSet> writeDescriptorInfo;
    uint32_t imageCount = 0;

    const gl::State &glState     = contextVk->getGLState();
    const auto &completeTextures = glState.getCompleteTextureCache();

    for (const gl::SamplerBinding &samplerBinding : mState.getSamplerBindings())
    {
        ASSERT(!samplerBinding.unreferenced);

        // TODO(jmadill): Sampler arrays
        ASSERT(samplerBinding.boundTextureUnits.size() == 1);

        GLuint textureUnit         = samplerBinding.boundTextureUnits[0];
        const gl::Texture *texture = completeTextures[textureUnit];

        // TODO(jmadill): Incomplete textures handling.
        ASSERT(texture);

        TextureVk *textureVk   = vk::GetImpl(texture);
        const vk::ImageHelper &image = textureVk->getImage();

        VkDescriptorImageInfo &imageInfo = descriptorImageInfo[imageCount];

        imageInfo.sampler     = textureVk->getSampler().getHandle();
        imageInfo.imageView   = textureVk->getImageView().getHandle();
        imageInfo.imageLayout = image.getCurrentLayout();

        auto &writeInfo = writeDescriptorInfo[imageCount];

        writeInfo.sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeInfo.pNext            = nullptr;
        writeInfo.dstSet           = descriptorSet;
        writeInfo.dstBinding       = imageCount;
        writeInfo.dstArrayElement  = 0;
        writeInfo.descriptorCount  = 1;
        writeInfo.descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeInfo.pImageInfo       = &imageInfo;
        writeInfo.pBufferInfo      = nullptr;
        writeInfo.pTexelBufferView = nullptr;

        imageCount++;
    }

    VkDevice device = contextVk->getDevice();

    ASSERT(imageCount > 0);
    vkUpdateDescriptorSets(device, imageCount, writeDescriptorInfo.data(), 0, nullptr);

    mDirtyTextures = false;
    return vk::NoError();
}

void ProgramVk::invalidateTextures()
{
    mDirtyTextures = true;
}

void ProgramVk::setDefaultUniformBlocksMinSizeForTesting(size_t minSize)
{
    for (DefaultUniformBlock &block : mDefaultUniformBlocks)
    {
        block.storage.setMinimumSizeForTesting(minSize);
    }
}
}  // namespace rx

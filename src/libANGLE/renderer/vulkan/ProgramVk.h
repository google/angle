//
// Copyright 2016 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ProgramVk.h:
//    Defines the class interface for ProgramVk, implementing ProgramImpl.
//

#ifndef LIBANGLE_RENDERER_VULKAN_PROGRAMVK_H_
#define LIBANGLE_RENDERER_VULKAN_PROGRAMVK_H_

#include <array>

#include "libANGLE/Constants.h"
#include "libANGLE/renderer/ProgramImpl.h"
#include "libANGLE/renderer/vulkan/RendererVk.h"
#include "libANGLE/renderer/vulkan/vk_helpers.h"

namespace rx
{
class ProgramVk : public ProgramImpl
{
  public:
    ProgramVk(const gl::ProgramState &state);
    ~ProgramVk() override;
    gl::Error destroy(const gl::Context *context) override;

    gl::LinkResult load(const gl::Context *context,
                        gl::InfoLog &infoLog,
                        gl::BinaryInputStream *stream) override;
    void save(const gl::Context *context, gl::BinaryOutputStream *stream) override;
    void setBinaryRetrievableHint(bool retrievable) override;
    void setSeparable(bool separable) override;

    gl::LinkResult link(const gl::Context *context,
                        const gl::ProgramLinkedResources &resources,
                        gl::InfoLog &infoLog) override;
    GLboolean validate(const gl::Caps &caps, gl::InfoLog *infoLog) override;

    void setUniform1fv(GLint location, GLsizei count, const GLfloat *v) override;
    void setUniform2fv(GLint location, GLsizei count, const GLfloat *v) override;
    void setUniform3fv(GLint location, GLsizei count, const GLfloat *v) override;
    void setUniform4fv(GLint location, GLsizei count, const GLfloat *v) override;
    void setUniform1iv(GLint location, GLsizei count, const GLint *v) override;
    void setUniform2iv(GLint location, GLsizei count, const GLint *v) override;
    void setUniform3iv(GLint location, GLsizei count, const GLint *v) override;
    void setUniform4iv(GLint location, GLsizei count, const GLint *v) override;
    void setUniform1uiv(GLint location, GLsizei count, const GLuint *v) override;
    void setUniform2uiv(GLint location, GLsizei count, const GLuint *v) override;
    void setUniform3uiv(GLint location, GLsizei count, const GLuint *v) override;
    void setUniform4uiv(GLint location, GLsizei count, const GLuint *v) override;
    void setUniformMatrix2fv(GLint location,
                             GLsizei count,
                             GLboolean transpose,
                             const GLfloat *value) override;
    void setUniformMatrix3fv(GLint location,
                             GLsizei count,
                             GLboolean transpose,
                             const GLfloat *value) override;
    void setUniformMatrix4fv(GLint location,
                             GLsizei count,
                             GLboolean transpose,
                             const GLfloat *value) override;
    void setUniformMatrix2x3fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;
    void setUniformMatrix3x2fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;
    void setUniformMatrix2x4fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;
    void setUniformMatrix4x2fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;
    void setUniformMatrix3x4fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;
    void setUniformMatrix4x3fv(GLint location,
                               GLsizei count,
                               GLboolean transpose,
                               const GLfloat *value) override;

    void getUniformfv(const gl::Context *context, GLint location, GLfloat *params) const override;
    void getUniformiv(const gl::Context *context, GLint location, GLint *params) const override;
    void getUniformuiv(const gl::Context *context, GLint location, GLuint *params) const override;

    // TODO: synchronize in syncState when dirty bits exist.
    void setUniformBlockBinding(GLuint uniformBlockIndex, GLuint uniformBlockBinding) override;

    void setPathFragmentInputGen(const std::string &inputName,
                                 GLenum genMode,
                                 GLint components,
                                 const GLfloat *coeffs) override;

    // Also initializes the pipeline layout, descriptor set layouts, and used descriptor ranges.
    gl::Error initShaders(const ContextVk *contextVk,
                          const gl::DrawCallParams &drawCallParams,
                          const vk::ShaderAndSerial **vertexShaderAndSerialOut,
                          const vk::ShaderAndSerial **fragmentShaderAndSerialOut);

    angle::Result updateUniforms(ContextVk *contextVk);

    const std::vector<VkDescriptorSet> &getDescriptorSets() const;
    const uint32_t *getDynamicOffsets();
    uint32_t getDynamicOffsetsCount();

    // In Vulkan, it is invalid to pass in a NULL descriptor set to vkCmdBindDescriptorSets.
    // However, it's valid to leave them in an undefined, unbound state, if they are never used.
    // This means when we want to ignore a descriptor set index, we need to pass in an offset
    // parameter to BindDescriptorSets, which is an offset into the getDescriptorSets array.
    // This allows us to skip binding blank descriptor sets when the Program doesn't use Uniforms
    // or Textures.
    const gl::RangeUI &getUsedDescriptorSetRange() const;

    gl::Error updateTexturesDescriptorSet(const gl::Context *context);
    void invalidateTextures();

    const vk::PipelineLayout &getPipelineLayout() const;

    // For testing only.
    void setDefaultUniformBlocksMinSizeForTesting(size_t minSize);

  private:
    template <int cols, int rows>
    void setUniformMatrixfv(GLint location,
                            GLsizei count,
                            GLboolean transpose,
                            const GLfloat *value);

    angle::Result reset(ContextVk *contextVk);
    angle::Result allocateDescriptorSet(ContextVk *contextVk, uint32_t descriptorSetIndex);
    gl::Error initDefaultUniformBlocks(const gl::Context *glContext);
    angle::Result updateDefaultUniformsDescriptorSet(ContextVk *contextVk);

    template <class T>
    void getUniformImpl(GLint location, T *v, GLenum entryPointType) const;

    template <typename T>
    void setUniformImpl(GLint location, GLsizei count, const T *v, GLenum entryPointType);

    vk::ShaderAndSerial mDefaultVertexShaderAndSerial;
    vk::ShaderAndSerial mDefaultFragmentShaderAndSerial;

    // State for the default uniform blocks.
    struct DefaultUniformBlock final : private angle::NonCopyable
    {
        DefaultUniformBlock();
        ~DefaultUniformBlock();

        vk::DynamicBuffer storage;

        // Shadow copies of the shader uniform data.
        angle::MemoryBuffer uniformData;
        bool uniformsDirty;

        // Since the default blocks are laid out in std140, this tells us where to write on a call
        // to a setUniform method. They are arranged in uniform location order.
        std::vector<sh::BlockMemberInfo> uniformLayout;
    };

    vk::ShaderMap<DefaultUniformBlock> mDefaultUniformBlocks;
    vk::ShaderMap<uint32_t> mUniformBlocksOffsets;

    // This is a special "empty" placeholder buffer for when a shader has no uniforms.
    // It is necessary because we want to keep a compatible pipeline layout in all cases,
    // and Vulkan does not tolerate having null handles in a descriptor set.
    vk::BufferAndMemory mEmptyUniformBlockStorage;

    // Descriptor sets for uniform blocks and textures for this program.
    std::vector<VkDescriptorSet> mDescriptorSets;
    gl::RangeUI mUsedDescriptorSetRange;
    bool mDirtyTextures;

    template <typename T>
    using ShaderTextureArray = std::array<T, gl::IMPLEMENTATION_MAX_SHADER_TEXTURES>;

    // We keep a reference to the pipeline and descriptor set layouts. This ensures they don't get
    // deleted while this program is in use.
    vk::BindingPointer<vk::PipelineLayout> mPipelineLayout;
    vk::DescriptorSetLayoutPointerArray mDescriptorSetLayouts;
};

}  // namespace rx

#endif  // LIBANGLE_RENDERER_VULKAN_PROGRAMVK_H_

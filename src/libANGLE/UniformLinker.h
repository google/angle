//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// UniformLinker.h: implements link-time checks for default block uniforms, and generates uniform
// locations. Populates data structures related to uniforms so that they can be stored in program
// state.

#ifndef LIBANGLE_UNIFORMLINKER_H_
#define LIBANGLE_UNIFORMLINKER_H_

#include "libANGLE/Program.h"
#include "libANGLE/Uniform.h"

#include <functional>

namespace gl
{

class UniformLinker
{
  public:
    UniformLinker(const ProgramState &state);

    bool link(const Context *context,
              InfoLog &infoLog,
              const Program::Bindings &uniformLocationBindings);

    void getResults(std::vector<LinkedUniform> *uniforms,
                    std::vector<VariableLocation> *uniformLocations);

  private:
    struct ShaderUniformCount
    {
        ShaderUniformCount() : vectorCount(0), samplerCount(0), imageCount(0), atomicCounterCount(0)
        {
        }
        ShaderUniformCount(const ShaderUniformCount &other) = default;
        ShaderUniformCount &operator=(const ShaderUniformCount &other) = default;

        ShaderUniformCount &operator+=(const ShaderUniformCount &other)
        {
            vectorCount += other.vectorCount;
            samplerCount += other.samplerCount;
            imageCount += other.imageCount;
            atomicCounterCount += other.atomicCounterCount;
            return *this;
        }

        unsigned int vectorCount;
        unsigned int samplerCount;
        unsigned int imageCount;
        unsigned int atomicCounterCount;
    };

    bool validateVertexAndFragmentUniforms(const Context *context, InfoLog &infoLog) const;

    static bool linkValidateUniforms(InfoLog &infoLog,
                                     const std::string &uniformName,
                                     const sh::Uniform &vertexUniform,
                                     const sh::Uniform &fragmentUniform);

    bool flattenUniformsAndCheckCapsForShader(const Context *context,
                                              Shader *shader,
                                              GLuint maxUniformComponents,
                                              GLuint maxTextureImageUnits,
                                              GLuint maxImageUnits,
                                              GLuint maxAtomicCounters,
                                              const std::string &componentsErrorMessage,
                                              const std::string &samplerErrorMessage,
                                              const std::string &imageErrorMessage,
                                              const std::string &atomicCounterErrorMessage,
                                              std::vector<LinkedUniform> &samplerUniforms,
                                              std::vector<LinkedUniform> &imageUniforms,
                                              std::vector<LinkedUniform> &atomicCounterUniforms,
                                              InfoLog &infoLog);

    bool flattenUniformsAndCheckCaps(const Context *context, InfoLog &infoLog);
    bool checkMaxCombinedAtomicCounters(const Caps &caps, InfoLog &infoLog);

    ShaderUniformCount flattenUniform(const sh::Uniform &uniform,
                                      std::vector<LinkedUniform> *samplerUniforms,
                                      std::vector<LinkedUniform> *imageUniforms,
                                      std::vector<LinkedUniform> *atomicCounterUniforms,
                                      GLenum shaderType);

    // markStaticUse is given as a separate parameter because it is tracked here at struct
    // granularity.
    ShaderUniformCount flattenUniformImpl(const sh::ShaderVariable &uniform,
                                          const std::string &fullName,
                                          const std::string &fullMappedName,
                                          std::vector<LinkedUniform> *samplerUniforms,
                                          std::vector<LinkedUniform> *imageUniforms,
                                          std::vector<LinkedUniform> *atomicCounterUniforms,
                                          GLenum shaderType,
                                          bool markStaticUse,
                                          int binding,
                                          int offset,
                                          int *location);

    bool indexUniforms(InfoLog &infoLog, const Program::Bindings &uniformLocationBindings);
    bool gatherUniformLocationsAndCheckConflicts(InfoLog &infoLog,
                                                 const Program::Bindings &uniformLocationBindings,
                                                 std::set<GLuint> *reservedLocations,
                                                 std::set<GLuint> *ignoredLocations,
                                                 int *maxUniformLocation);
    void pruneUnusedUniforms();

    const ProgramState &mState;
    std::vector<LinkedUniform> mUniforms;
    std::vector<VariableLocation> mUniformLocations;
};

// This class is intended to be used during the link step to store interface block information.
// It is called by the Impl class during ProgramImpl::link so that it has access to the
// real block size and layout.
class InterfaceBlockLinker : angle::NonCopyable
{
  public:
    virtual ~InterfaceBlockLinker();

    using GetBlockSize = std::function<
        bool(const std::string &blockName, const std::string &blockMappedName, size_t *sizeOut)>;
    using GetBlockMemberInfo = std::function<
        bool(const std::string &name, const std::string &mappedName, sh::BlockMemberInfo *infoOut)>;

    // This is called once per shader stage. It stores a pointer to the block vector, so it's
    // important that this class does not persist longer than the duration of Program::link.
    void addShaderBlocks(GLenum shader, const std::vector<sh::InterfaceBlock> *blocks);

    // This is called once during a link operation, after all shader blocks are added.
    void linkBlocks(const GetBlockSize &getBlockSize,
                    const GetBlockMemberInfo &getMemberInfo) const;

  protected:
    InterfaceBlockLinker(std::vector<InterfaceBlock> *blocksOut);
    void defineInterfaceBlock(const GetBlockSize &getBlockSize,
                              const GetBlockMemberInfo &getMemberInfo,
                              const sh::InterfaceBlock &interfaceBlock,
                              GLenum shaderType) const;

    template <typename VarT>
    void defineBlockMembers(const GetBlockMemberInfo &getMemberInfo,
                            const std::vector<VarT> &fields,
                            const std::string &prefix,
                            const std::string &mappedPrefix,
                            int blockIndex) const;

    virtual void defineBlockMember(const sh::ShaderVariable &field,
                                   const std::string &fullName,
                                   const std::string &fullMappedName,
                                   int blockIndex,
                                   const sh::BlockMemberInfo &memberInfo) const = 0;
    virtual size_t getCurrentBlockMemberIndex() const                           = 0;

    using ShaderBlocks = std::pair<GLenum, const std::vector<sh::InterfaceBlock> *>;
    std::vector<ShaderBlocks> mShaderBlocks;

    std::vector<InterfaceBlock> *mBlocksOut;
};

class UniformBlockLinker final : public InterfaceBlockLinker
{
  public:
    UniformBlockLinker(std::vector<InterfaceBlock> *blocksOut,
                       std::vector<LinkedUniform> *uniformsOut);
    virtual ~UniformBlockLinker();

  private:
    void defineBlockMember(const sh::ShaderVariable &field,
                           const std::string &fullName,
                           const std::string &fullMappedName,
                           int blockIndex,
                           const sh::BlockMemberInfo &memberInfo) const override;
    size_t getCurrentBlockMemberIndex() const override;
    std::vector<LinkedUniform> *mUniformsOut;
};

// TODO(jiajia.qin@intel.com): Add buffer variables support.
class ShaderStorageBlockLinker final : public InterfaceBlockLinker
{
  public:
    ShaderStorageBlockLinker(std::vector<InterfaceBlock> *blocksOut);
    virtual ~ShaderStorageBlockLinker();

  private:
    void defineBlockMember(const sh::ShaderVariable &field,
                           const std::string &fullName,
                           const std::string &fullMappedName,
                           int blockIndex,
                           const sh::BlockMemberInfo &memberInfo) const override;
    size_t getCurrentBlockMemberIndex() const override;
};

}  // namespace gl

#endif  // LIBANGLE_UNIFORMLINKER_H_

//
// Copyright (c) 2013-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// blocklayout.h:
//   Methods and classes related to uniform layout and packing in GLSL and HLSL.
//

#ifndef COMMON_BLOCKLAYOUT_H_
#define COMMON_BLOCKLAYOUT_H_

#include <cstddef>
#include <map>
#include <vector>

#include <GLSLANG/ShaderLang.h>
#include "angle_gl.h"

namespace sh
{
struct ShaderVariable;
struct InterfaceBlockField;
struct Uniform;
struct Varying;
struct InterfaceBlock;

struct BlockMemberInfo
{
    constexpr BlockMemberInfo() = default;

    constexpr BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
        : offset(offset),
          arrayStride(arrayStride),
          matrixStride(matrixStride),
          isRowMajorMatrix(isRowMajorMatrix)
    {}

    constexpr BlockMemberInfo(int offset,
                              int arrayStride,
                              int matrixStride,
                              bool isRowMajorMatrix,
                              int topLevelArrayStride)
        : offset(offset),
          arrayStride(arrayStride),
          matrixStride(matrixStride),
          isRowMajorMatrix(isRowMajorMatrix),
          topLevelArrayStride(topLevelArrayStride)
    {}

    // A single integer identifying the offset of an active variable.
    int offset = -1;

    // A single integer identifying the stride between array elements in an active variable.
    int arrayStride = -1;

    // A single integer identifying the stride between columns of a column-major matrix or rows of a
    // row-major matrix.
    int matrixStride = -1;

    // A single integer identifying whether an active variable is a row-major matrix.
    bool isRowMajorMatrix = false;

    // A single integer identifying the number of active array elements of the top-level shader
    // storage block member containing the active variable.
    int topLevelArrayStride = -1;
};

constexpr BlockMemberInfo kDefaultBlockMemberInfo;

class BlockLayoutEncoder
{
  public:
    BlockLayoutEncoder();
    virtual ~BlockLayoutEncoder() {}

    BlockMemberInfo encodeType(GLenum type,
                               const std::vector<unsigned int> &arraySizes,
                               bool isRowMajorMatrix);

    size_t getBlockSize() const { return mCurrentOffset * kBytesPerComponent; }
    size_t getStructureBaseAlignment() const { return mStructureBaseAlignment; }
    void increaseCurrentOffset(size_t offsetInBytes);
    void setStructureBaseAlignment(size_t baseAlignment);

    virtual void enterAggregateType() = 0;
    virtual void exitAggregateType()  = 0;

    static constexpr size_t kBytesPerComponent           = 4u;
    static constexpr unsigned int kComponentsPerRegister = 4u;

    static size_t GetBlockRegister(const BlockMemberInfo &info);
    static size_t GetBlockRegisterElement(const BlockMemberInfo &info);

  protected:
    size_t mCurrentOffset;
    size_t mStructureBaseAlignment;

    virtual void nextRegister();

    virtual void getBlockLayoutInfo(GLenum type,
                                    const std::vector<unsigned int> &arraySizes,
                                    bool isRowMajorMatrix,
                                    int *arrayStrideOut,
                                    int *matrixStrideOut) = 0;
    virtual void advanceOffset(GLenum type,
                               const std::vector<unsigned int> &arraySizes,
                               bool isRowMajorMatrix,
                               int arrayStride,
                               int matrixStride)          = 0;
};

// Will return default values for everything.
class DummyBlockEncoder : public BlockLayoutEncoder
{
  public:
    DummyBlockEncoder() = default;

    void enterAggregateType() override {}
    void exitAggregateType() override {}

  protected:
    void getBlockLayoutInfo(GLenum type,
                            const std::vector<unsigned int> &arraySizes,
                            bool isRowMajorMatrix,
                            int *arrayStrideOut,
                            int *matrixStrideOut) override
    {
        *arrayStrideOut  = 0;
        *matrixStrideOut = 0;
    }

    void advanceOffset(GLenum type,
                       const std::vector<unsigned int> &arraySizes,
                       bool isRowMajorMatrix,
                       int arrayStride,
                       int matrixStride) override
    {}
};

// Block layout according to the std140 block layout
// See "Standard Uniform Block Layout" in Section 2.11.6 of the OpenGL ES 3.0 specification

class Std140BlockEncoder : public BlockLayoutEncoder
{
  public:
    Std140BlockEncoder();

    void enterAggregateType() override;
    void exitAggregateType() override;

  protected:
    void getBlockLayoutInfo(GLenum type,
                            const std::vector<unsigned int> &arraySizes,
                            bool isRowMajorMatrix,
                            int *arrayStrideOut,
                            int *matrixStrideOut) override;
    void advanceOffset(GLenum type,
                       const std::vector<unsigned int> &arraySizes,
                       bool isRowMajorMatrix,
                       int arrayStride,
                       int matrixStride) override;
};

class Std430BlockEncoder : public Std140BlockEncoder
{
  public:
    Std430BlockEncoder();

  protected:
    void nextRegister() override;
    void getBlockLayoutInfo(GLenum type,
                            const std::vector<unsigned int> &arraySizes,
                            bool isRowMajorMatrix,
                            int *arrayStrideOut,
                            int *matrixStrideOut) override;
};

using BlockLayoutMap = std::map<std::string, BlockMemberInfo>;

void GetInterfaceBlockInfo(const std::vector<InterfaceBlockField> &fields,
                           const std::string &prefix,
                           BlockLayoutEncoder *encoder,
                           BlockLayoutMap *blockInfoOut);

// Used for laying out the default uniform block on the Vulkan backend.
void GetUniformBlockInfo(const std::vector<Uniform> &uniforms,
                         const std::string &prefix,
                         BlockLayoutEncoder *encoder,
                         BlockLayoutMap *blockInfoOut);

class ShaderVariableVisitor
{
  public:
    virtual ~ShaderVariableVisitor() {}

    virtual void enterStruct(const ShaderVariable &structVar) {}
    virtual void exitStruct(const ShaderVariable &structVar) {}

    virtual void enterStructAccess(const ShaderVariable &structVar) {}
    virtual void exitStructAccess(const ShaderVariable &structVar) {}

    virtual void enterArray(const ShaderVariable &arrayVar) {}
    virtual void exitArray(const ShaderVariable &arrayVar) {}

    virtual void enterArrayElement(const ShaderVariable &arrayVar, unsigned int arrayElement) {}
    virtual void exitArrayElement(const ShaderVariable &arrayVar, unsigned int arrayElement) {}

    virtual void visitSampler(const sh::ShaderVariable &sampler) {}

    virtual void visitVariable(const ShaderVariable &variable, bool isRowMajor) = 0;

  protected:
    ShaderVariableVisitor() {}
};

class VariableNameVisitor : public ShaderVariableVisitor
{
  public:
    VariableNameVisitor(const std::string &namePrefix, const std::string &mappedNamePrefix);
    ~VariableNameVisitor();

    void enterStruct(const ShaderVariable &structVar) override;
    void exitStruct(const ShaderVariable &structVar) override;
    void enterStructAccess(const ShaderVariable &structVar) override;
    void exitStructAccess(const ShaderVariable &structVar) override;
    void enterArray(const ShaderVariable &arrayVar) override;
    void exitArray(const ShaderVariable &arrayVar) override;
    void enterArrayElement(const ShaderVariable &arrayVar, unsigned int arrayElement) override;
    void exitArrayElement(const ShaderVariable &arrayVar, unsigned int arrayElement) override
    {
        mNameStack.pop_back();
        mMappedNameStack.pop_back();
    }

  protected:
    virtual void visitNamedSampler(const sh::ShaderVariable &sampler,
                                   const std::string &name,
                                   const std::string &mappedName)
    {}
    virtual void visitNamedVariable(const ShaderVariable &variable,
                                    bool isRowMajor,
                                    const std::string &name,
                                    const std::string &mappedName) = 0;

  private:
    void visitSampler(const sh::ShaderVariable &sampler) final;
    void visitVariable(const ShaderVariable &variable, bool isRowMajor) final;
    std::string collapseNameStack() const;
    std::string collapseMappedNameStack() const;

    std::vector<std::string> mNameStack;
    std::vector<std::string> mMappedNameStack;
};

class BlockEncoderVisitor : public VariableNameVisitor
{
  public:
    BlockEncoderVisitor(const std::string &namePrefix,
                        const std::string &mappedNamePrefix,
                        BlockLayoutEncoder *encoder);
    ~BlockEncoderVisitor();

    void enterStructAccess(const ShaderVariable &structVar) override;
    void exitStructAccess(const ShaderVariable &structVar) override;

    void visitNamedVariable(const ShaderVariable &variable,
                            bool isRowMajor,
                            const std::string &name,
                            const std::string &mappedName) override;

    virtual void encodeVariable(const ShaderVariable &variable,
                                const BlockMemberInfo &variableInfo,
                                const std::string &name,
                                const std::string &mappedName) = 0;

  private:
    BlockLayoutEncoder *mEncoder;
};

void TraverseShaderVariable(const ShaderVariable &variable,
                            bool isRowMajorLayout,
                            ShaderVariableVisitor *visitor);

template <typename T>
void TraverseShaderVariables(const std::vector<T> &vars,
                             bool isRowMajorLayout,
                             ShaderVariableVisitor *visitor)
{
    for (const T &var : vars)
    {
        TraverseShaderVariable(var, isRowMajorLayout, visitor);
    }
}
}  // namespace sh

#endif  // COMMON_BLOCKLAYOUT_H_

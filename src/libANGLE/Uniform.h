//
// Copyright 2010 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBANGLE_UNIFORM_H_
#define LIBANGLE_UNIFORM_H_

#include <string>
#include <vector>

#include "angle_gl.h"
#include "common/MemoryBuffer.h"
#include "common/debug.h"
#include "common/utilities.h"
#include "compiler/translator/blocklayout.h"
#include "libANGLE/angletypes.h"

namespace gl
{
class BinaryInputStream;
class BinaryOutputStream;
struct UniformTypeInfo;
struct UsedUniform;

// Note: keep this struct memcpy-able: i.e, a simple struct with basic types only and no virtual
// functions. LinkedUniform relies on this so that it can use memcpy to initialize uniform for
// performance.
struct ActiveVariable
{
    ActiveVariable();
    ActiveVariable(const ActiveVariable &rhs);
    ~ActiveVariable();

    ActiveVariable &operator=(const ActiveVariable &rhs);

    ShaderType getFirstActiveShaderType() const
    {
        return static_cast<ShaderType>(ScanForward(mActiveUseBits.bits()));
    }
    void setActive(ShaderType shaderType, bool used, uint32_t id);
    void unionReferencesWith(const ActiveVariable &other);
    bool isActive(ShaderType shaderType) const
    {
        ASSERT(shaderType != ShaderType::InvalidEnum);
        return mActiveUseBits[shaderType];
    }
    const ShaderMap<uint32_t> &getIds() const { return mIds; }
    uint32_t getId(ShaderType shaderType) const { return mIds[shaderType]; }
    ShaderBitSet activeShaders() const { return mActiveUseBits; }
    GLuint activeShaderCount() const { return static_cast<GLuint>(mActiveUseBits.count()); }

  private:
    ShaderBitSet mActiveUseBits;
    // The id of a linked variable in each shader stage.  This id originates from
    // sh::ShaderVariable::id or sh::InterfaceBlock::id
    ShaderMap<uint32_t> mIds;
};

// Helper struct representing a single shader uniform. Most of this structure's data member and
// access functions mirrors ShaderVariable; See ShaderVars.h for more info.
struct LinkedUniform
{
    LinkedUniform();
    LinkedUniform(GLenum typeIn,
                  GLenum precisionIn,
                  const std::string &nameIn,
                  const std::vector<unsigned int> &arraySizesIn,
                  const int bindingIn,
                  const int offsetIn,
                  const int locationIn,
                  const int bufferIndexIn,
                  const sh::BlockMemberInfo &blockInfoIn);
    LinkedUniform(const LinkedUniform &other);
    LinkedUniform(const UsedUniform &usedUniform);
    LinkedUniform &operator=(const LinkedUniform &other);
    ~LinkedUniform();

    void setBlockInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
    {
        mFixedSizeData.blockInfo.offset           = offset;
        mFixedSizeData.blockInfo.arrayStride      = arrayStride;
        mFixedSizeData.blockInfo.matrixStride     = matrixStride;
        mFixedSizeData.blockInfo.isRowMajorMatrix = isRowMajorMatrix;
    }
    void setBufferIndex(int bufferIndex) { mFixedSizeData.bufferIndex = bufferIndex; }

    bool isSampler() const { return typeInfo->isSampler; }
    bool isImage() const { return typeInfo->isImageType; }
    bool isAtomicCounter() const { return IsAtomicCounterType(mFixedSizeData.type); }
    bool isInDefaultBlock() const { return mFixedSizeData.bufferIndex == -1; }
    size_t getElementSize() const { return typeInfo->externalSize; }
    size_t getElementComponents() const { return typeInfo->componentCount; }

    bool isStruct() const { return mFixedSizeData.flagBits.isStruct; }
    bool isTexelFetchStaticUse() const { return mFixedSizeData.flagBits.texelFetchStaticUse; }
    bool isFragmentInOut() const { return mFixedSizeData.flagBits.isFragmentInOut; }

    bool isArrayOfArrays() const { return arraySizes.size() >= 2u; }
    bool isArray() const { return !arraySizes.empty(); }
    unsigned int getArraySizeProduct() const { return gl::ArraySizeProduct(arraySizes); }
    unsigned int getOutermostArraySize() const { return isArray() ? arraySizes.back() : 0; }
    unsigned int getBasicTypeElementCount() const
    {
        ASSERT(!isArrayOfArrays());
        ASSERT(!isStruct() || !isArray());

        if (isArray())
        {
            return getOutermostArraySize();
        }
        return 1u;
    }

    GLenum getType() const { return mFixedSizeData.type; }
    unsigned int getExternalSize() const;
    unsigned int getOuterArrayOffset() const { return mFixedSizeData.outerArrayOffset; }
    unsigned int getOuterArraySizeProduct() const { return mFixedSizeData.outerArraySizeProduct; }
    int getBinding() const { return mFixedSizeData.binding; }
    int getOffset() const { return mFixedSizeData.offset; }
    const sh::BlockMemberInfo &getBlockInfo() const { return mFixedSizeData.blockInfo; }
    int getBufferIndex() const { return mFixedSizeData.bufferIndex; }
    int getLocation() const { return mFixedSizeData.location; }
    GLenum getImageUnitFormat() const { return mFixedSizeData.imageUnitFormat; }

    bool findInfoByMappedName(const std::string &mappedFullName,
                              const sh::ShaderVariable **leafVar,
                              std::string *originalFullName) const;
    bool isBuiltIn() const { return gl::IsBuiltInName(name); }

    int parentArrayIndex() const
    {
        return hasParentArrayIndex() ? mFixedSizeData.flattenedOffsetInParentArrays : 0;
    }

    bool hasParentArrayIndex() const { return mFixedSizeData.flattenedOffsetInParentArrays != -1; }
    bool isSameInterfaceBlockFieldAtLinkTime(const sh::ShaderVariable &other) const;

    bool isSameVariableAtLinkTime(const sh::ShaderVariable &other,
                                  bool matchPrecision,
                                  bool matchName) const;

    ShaderType getFirstActiveShaderType() const
    {
        return mFixedSizeData.activeVariable.getFirstActiveShaderType();
    }
    void setActive(ShaderType shaderType, bool used, uint32_t _id)
    {
        mFixedSizeData.activeVariable.setActive(shaderType, used, _id);
    }
    bool isActive(ShaderType shaderType) const
    {
        return mFixedSizeData.activeVariable.isActive(shaderType);
    }
    const ShaderMap<uint32_t> &getIds() const { return mFixedSizeData.activeVariable.getIds(); }
    uint32_t getId(ShaderType shaderType) const
    {
        return mFixedSizeData.activeVariable.getId(shaderType);
    }
    ShaderBitSet activeShaders() const { return mFixedSizeData.activeVariable.activeShaders(); }
    GLuint activeShaderCount() const { return mFixedSizeData.activeVariable.activeShaderCount(); }
    const ActiveVariable &getActiveVariable() const { return mFixedSizeData.activeVariable; }

    void save(BinaryOutputStream *stream) const;
    void load(BinaryInputStream *stream);

    std::string name;
    // Only used by GL backend
    std::string mappedName;

    std::vector<unsigned int> arraySizes;

    const UniformTypeInfo *typeInfo;

  private:
    // Important: The fixed size data structure with fundamental data types only, so that we can
    // initialize with memcpy. Do not put any std::vector or objects with virtual functions in it.
    struct
    {
        GLenum type;
        GLenum precision;
        int location;
        int binding;
        GLenum imageUnitFormat;
        int offset;
        uint32_t id;
        int flattenedOffsetInParentArrays;
        int bufferIndex;
        sh::BlockMemberInfo blockInfo;
        unsigned int outerArraySizeProduct;
        unsigned int outerArrayOffset;
        ActiveVariable activeVariable;

        union
        {
            struct
            {
                uint32_t staticUse : 1;
                uint32_t active : 1;
                uint32_t isStruct : 1;
                uint32_t rasterOrdered : 1;
                uint32_t readonly : 1;
                uint32_t writeonly : 1;
                uint32_t isFragmentInOut : 1;
                uint32_t texelFetchStaticUse : 1;
                uint32_t padding : 24;
            } flagBits;

            uint32_t flagBitsAsUInt;
        };
    } mFixedSizeData;
};

struct BufferVariable : public sh::ShaderVariable
{
    BufferVariable();
    BufferVariable(GLenum type,
                   GLenum precision,
                   const std::string &name,
                   const std::vector<unsigned int> &arraySizes,
                   const int bufferIndex,
                   const sh::BlockMemberInfo &blockInfo);
    ~BufferVariable();

    void setActive(ShaderType shaderType, bool used, uint32_t _id)
    {
        activeVariable.setActive(shaderType, used, _id);
    }
    bool isActive(ShaderType shaderType) const { return activeVariable.isActive(shaderType); }
    uint32_t getId(ShaderType shaderType) const { return activeVariable.getId(shaderType); }
    ShaderBitSet activeShaders() const { return activeVariable.activeShaders(); }

    ActiveVariable activeVariable;
    int bufferIndex;
    sh::BlockMemberInfo blockInfo;

    int topLevelArraySize;
};

// Parent struct for atomic counter, uniform block, and shader storage block buffer, which all
// contain a group of shader variables, and have a GL buffer backed.
struct ShaderVariableBuffer
{
    ShaderVariableBuffer();
    ShaderVariableBuffer(const ShaderVariableBuffer &other);
    ~ShaderVariableBuffer();

    ShaderType getFirstActiveShaderType() const
    {
        return activeVariable.getFirstActiveShaderType();
    }
    void setActive(ShaderType shaderType, bool used, uint32_t _id)
    {
        activeVariable.setActive(shaderType, used, _id);
    }
    void unionReferencesWith(const ActiveVariable &other)
    {
        activeVariable.unionReferencesWith(other);
    }
    bool isActive(ShaderType shaderType) const { return activeVariable.isActive(shaderType); }
    const ShaderMap<uint32_t> &getIds() const { return activeVariable.getIds(); }
    uint32_t getId(ShaderType shaderType) const { return activeVariable.getId(shaderType); }
    ShaderBitSet activeShaders() const { return activeVariable.activeShaders(); }
    int numActiveVariables() const;

    ActiveVariable activeVariable;
    int binding;
    unsigned int dataSize;
    std::vector<unsigned int> memberIndexes;
};

using AtomicCounterBuffer = ShaderVariableBuffer;

// Helper struct representing a single shader interface block
struct InterfaceBlock : public ShaderVariableBuffer
{
    InterfaceBlock();
    InterfaceBlock(const std::string &nameIn,
                   const std::string &mappedNameIn,
                   bool isArrayIn,
                   bool isReadOnlyIn,
                   unsigned int arrayElementIn,
                   unsigned int firstFieldArraySizeIn,
                   int bindingIn);
    InterfaceBlock(const InterfaceBlock &other);

    std::string nameWithArrayIndex() const;
    std::string mappedNameWithArrayIndex() const;

    std::string name;
    std::string mappedName;
    bool isArray;
    // Only valid for SSBOs, specifies whether it has the readonly qualifier.
    bool isReadOnly;
    unsigned int arrayElement;
    unsigned int firstFieldArraySize;
};

}  // namespace gl

#endif  // LIBANGLE_UNIFORM_H_

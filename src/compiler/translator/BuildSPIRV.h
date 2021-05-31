//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BuildSPIRV: Helper for OutputSPIRV to build SPIR-V.
//

#ifndef COMPILER_TRANSLATOR_BUILDSPIRV_H_
#define COMPILER_TRANSLATOR_BUILDSPIRV_H_

#include "common/hash_utils.h"
#include "common/spirv/spirv_instruction_builder_autogen.h"
#include "compiler/translator/Compiler.h"

namespace spirv = angle::spirv;

namespace sh
{
// Helper classes to map types to ids
struct SpirvType
{
    // If struct or interface block, the type is identified by the pointer.  Note that both
    // TStructure and TInterfaceBlock inherit from TFieldListCollection, and their difference is
    // irrelevant as far as SPIR-V type is concerned.
    const TFieldListCollection *block = nullptr;

    // If a structure is used in two interface blocks with different layouts, it would have
    // to generate two SPIR-V types, as its fields' Offset decorations could be different.
    // For non-block types, when used in an interface block as an array, they could generate
    // different ArrayStride decorations.  As such, the block storage is part of the SPIR-V type
    // except for non-block non-array types.
    TLayoutBlockStorage blockStorage = EbsUnspecified;

    // Otherwise, it's a basic type + column, row and array dimensions, or it's an image
    // declaration.
    //
    // Notes:
    //
    // - `precision` turns into a RelaxedPrecision decoration on the variable and instructions.
    // - `precise` turns into a NoContraction decoration on the instructions.
    // - `readonly`, `writeonly`, `coherent`, `volatile` and `restrict` only apply to memory object
    //    declarations
    // - `invariant` only applies to variable or members of a block
    // - `matrixPacking` only applies to members of a struct
    TBasicType type = EbtFloat;

    uint8_t primarySize                = 1;
    uint8_t secondarySize              = 1;
    TLayoutMatrixPacking matrixPacking = EmpColumnMajor;

    TSpan<const unsigned int> arraySizes;

    // Only useful for image types.
    TLayoutImageInternalFormat imageInternalFormat = EiifUnspecified;

    // For sampled images (i.e. GLSL samplers), there are two type ids; one is the OpTypeImage that
    // declares the image itself, and one OpTypeSampledImage.  `imageOnly` distinguishes between
    // these two types.  Note that for the former, the basic type is still Ebt*Sampler* to
    // distinguish it from storage images (which have a basic type of Ebt*Image*).
    bool isSamplerBaseImage = false;
};

struct SpirvIdAndIdList
{
    spirv::IdRef id;
    spirv::IdRefList idList;

    bool operator==(const SpirvIdAndIdList &other) const
    {
        return id == other.id && idList == other.idList;
    }
};

struct SpirvIdAndStorageClass
{
    spirv::IdRef id;
    spv::StorageClass storageClass;

    bool operator==(const SpirvIdAndStorageClass &other) const
    {
        return id == other.id && storageClass == other.storageClass;
    }
};

struct SpirvTypeHash
{
    size_t operator()(const sh::SpirvType &type) const
    {
        // Block storage must only affect the type if it's a block type or array type (in a block).
        ASSERT(type.blockStorage == sh::EbsUnspecified || type.block != nullptr ||
               !type.arraySizes.empty());

        size_t result = 0;

        if (!type.arraySizes.empty())
        {
            result = angle::ComputeGenericHash(type.arraySizes.data(),
                                               type.arraySizes.size() * sizeof(type.arraySizes[0]));
        }

        if (type.block != nullptr)
        {
            return result ^ angle::ComputeGenericHash(&type.block, sizeof(type.block)) ^
                   type.blockStorage;
        }

        static_assert(sh::EbtLast < 256, "Basic type doesn't fit in uint8_t");
        static_assert(sh::EbsLast < 8, "Block storage doesn't fit in 3 bits");
        static_assert(sh::EiifLast < 32, "Image format doesn't fit in 5 bits");
        static_assert(sh::EmpLast < 4, "Matrix packing doesn't fit in 2 bits");
        ASSERT(type.primarySize > 0 && type.primarySize <= 4);
        ASSERT(type.secondarySize > 0 && type.secondarySize <= 4);

        const uint8_t properties[4] = {
            static_cast<uint8_t>(type.type),
            static_cast<uint8_t>((type.primarySize - 1) | (type.secondarySize - 1) << 2 |
                                 type.isSamplerBaseImage << 4 | type.matrixPacking << 5),
            static_cast<uint8_t>(type.blockStorage | type.imageInternalFormat << 3),
            // Padding because ComputeGenericHash expects a key size divisible by 4
        };

        return result ^ angle::ComputeGenericHash(properties, sizeof(properties));
    }
};

struct SpirvIdAndIdListHash
{
    size_t operator()(const SpirvIdAndIdList &key) const
    {
        return angle::ComputeGenericHash(key.idList.data(),
                                         key.idList.size() * sizeof(key.idList[0])) ^
               key.id;
    }
};

struct SpirvIdAndStorageClassHash
{
    size_t operator()(const SpirvIdAndStorageClass &key) const
    {
        ASSERT(key.storageClass < 16);
        return key.storageClass | key.id << 4;
    }
};

// Data tracked per SPIR-V type (keyed by SpirvType).
struct SpirvTypeData
{
    // The SPIR-V id corresponding to the type.
    spirv::IdRef id;
    // The base alignment and size of the type based on the storage block it's used in (if
    // applicable)
    uint32_t baseAlignment;
    uint32_t sizeInStorageBlock;
};

// A block of code.  SPIR-V produces forward references to blocks, such as OpBranchConditional
// specifying the id of the if and else blocks, each of those referencing the id of the block after
// the else.  Additionally, local variable declarations are accumulated at the top of the first
// block in a function.  For these reasons, each block of SPIR-V is generated separately and
// assembled at the end of the function, allowing prior blocks to be modified when necessary.
struct SpirvBlock
{
    // Id of the block
    spirv::IdRef labelId;

    // Local variable declarations.  Only the first block of a function is allowed to contain any
    // instructions here.
    spirv::Blob localVariables;

    // Everything *after* OpLabel (which itself is not generated until blocks are assembled) and
    // local variables.
    spirv::Blob body;

    // Whether the block is terminated.  Useful for functions without return, asserting that code is
    // not added after return/break/continue etc (i.e. dead code, which should really be removed
    // earlier by a transformation, but could also be hacked by returning a bogus block to contain
    // all the "garbage" to throw away), last switch case without a break, etc.
    bool isTerminated = false;
};

// Helper class to construct SPIR-V
class SPIRVBuilder : angle::NonCopyable
{
  public:
    SPIRVBuilder(gl::ShaderType shaderType, ShHashFunction64 hashFunction, NameMap &nameMap)
        : mShaderType(shaderType),
          mNextAvailableId(1),
          mHashFunction(hashFunction),
          mNameMap(nameMap),
          mNextUnusedBinding(0),
          mNextUnusedInputLocation(0),
          mNextUnusedOutputLocation(0)
    {}

    spirv::IdRef getNewId();
    SpirvType getSpirvType(const TType &type, TLayoutBlockStorage blockStorage) const;
    const SpirvTypeData &getTypeData(const TType &type, TLayoutBlockStorage blockStorage);
    const SpirvTypeData &getSpirvTypeData(const SpirvType &type, const char *blockName);
    spirv::IdRef getTypePointerId(spirv::IdRef typeId, spv::StorageClass storageClass);
    spirv::IdRef getFunctionTypeId(spirv::IdRef returnTypeId, const spirv::IdRefList &paramTypeIds);

    spirv::Blob *getSpirvExecutionModes() { return &mSpirvExecutionModes; }
    spirv::Blob *getSpirvDebug() { return &mSpirvDebug; }
    spirv::Blob *getSpirvDecorations() { return &mSpirvDecorations; }
    spirv::Blob *getSpirvTypeAndConstantDecls() { return &mSpirvTypeAndConstantDecls; }
    spirv::Blob *getSpirvTypePointerDecls() { return &mSpirvTypePointerDecls; }
    spirv::Blob *getSpirvVariableDecls() { return &mSpirvVariableDecls; }
    spirv::Blob *getSpirvFunctions() { return &mSpirvFunctions; }
    spirv::Blob *getSpirvCurrentFunctionBlock()
    {
        ASSERT(!mSpirvCurrentFunctionBlocks.empty() &&
               !mSpirvCurrentFunctionBlocks.back().isTerminated);
        return &mSpirvCurrentFunctionBlocks.back().body;
    }
    bool isCurrentFunctionBlockTerminated() const
    {
        ASSERT(!mSpirvCurrentFunctionBlocks.empty());
        return mSpirvCurrentFunctionBlocks.back().isTerminated;
    }
    void terminateCurrentFunctionBlock()
    {
        ASSERT(!mSpirvCurrentFunctionBlocks.empty());
        mSpirvCurrentFunctionBlocks.back().isTerminated = true;
    }

    void addCapability(spv::Capability capability);
    void addExecutionMode(spv::ExecutionMode executionMode);
    void setEntryPointId(spirv::IdRef id);
    void addEntryPointInterfaceVariableId(spirv::IdRef id);
    void writePerVertexBuiltIns(const TType &type, spirv::IdRef typeId);
    void writeInterfaceVariableDecorations(const TType &type, spirv::IdRef variableId);

    uint32_t calculateBaseAlignmentAndSize(const SpirvType &type, uint32_t *sizeInStorageBlockOut);
    uint32_t calculateSizeAndWriteOffsetDecorations(const SpirvType &type, spirv::IdRef typeId);

    spirv::IdRef getBoolConstant(bool value);
    spirv::IdRef getUintConstant(uint32_t value);
    spirv::IdRef getIntConstant(int32_t value);
    spirv::IdRef getFloatConstant(float value);
    spirv::IdRef getCompositeConstant(spirv::IdRef typeId, const spirv::IdRefList &values);

    // Helpers to start and end a function.
    void startNewFunction();
    void assembleSpirvFunctionBlocks();

    // Helper to declare a variable.  Function-local variables must be placed in the first block of
    // the current function.
    spirv::IdRef declareVariable(spirv::IdRef typeId,
                                 spv::StorageClass storageClass,
                                 spirv::IdRef *initializerId,
                                 const char *name);

    // TODO: remove name hashing once translation through glslang is removed.  That is necessary to
    // avoid name collision between ANGLE's internal symbols and user-defined ones when compiling
    // the generated GLSL, but is irrelevant when generating SPIR-V directly.  Currently, the SPIR-V
    // transformer relies on the "mapped" names, which should also be changed when this hashing is
    // removed.
    ImmutableString hashName(const TSymbol *symbol);
    ImmutableString hashTypeName(const TType &type);
    ImmutableString hashFieldName(const TField *field);
    ImmutableString hashFunctionName(const TFunction *func);

    spirv::Blob getSpirv();

  private:
    SpirvTypeData declareType(const SpirvType &type, const char *blockName);

    // Helpers for type declaration.
    void getImageTypeParameters(TBasicType type,
                                spirv::IdRef *sampledTypeOut,
                                spv::Dim *dimOut,
                                spirv::LiteralInteger *depthOut,
                                spirv::LiteralInteger *arrayedOut,
                                spirv::LiteralInteger *multisampledOut,
                                spirv::LiteralInteger *sampledOut);
    spv::ImageFormat getImageFormat(TLayoutImageInternalFormat imageInternalFormat);

    spirv::IdRef getBasicConstantHelper(uint32_t value,
                                        TBasicType type,
                                        angle::HashMap<uint32_t, spirv::IdRef> *constants);

    uint32_t nextUnusedBinding();
    uint32_t nextUnusedInputLocation(uint32_t consumedCount);
    uint32_t nextUnusedOutputLocation(uint32_t consumedCount);

    gl::ShaderType mShaderType;

    // Capabilities the shader is using.  Accumulated as the instructions are generated.  The Shader
    // capability is unconditionally generated, so it's not tracked.
    std::set<spv::Capability> mCapabilities;

    // Execution modes the shader is enabling.  Accumulated as the instructions are generated.
    // Execution mode instructions that require a parameter are written to mSpirvExecutionModes as
    // instructions; they are always generated once so don't benefit from being in a std::set.
    std::set<spv::ExecutionMode> mExecutionModes;

    // The list of interface variables and the id of main() populated as the instructions are
    // generated.  Used for the OpEntryPoint instruction.
    spirv::IdRefList mEntryPointInterfaceList;
    spirv::IdRef mEntryPointId;

    // Current ID bound, used to allocate new ids.
    spirv::IdRef mNextAvailableId;

    // A map from the AST type to the corresponding SPIR-V ID and associated data.  Note that TType
    // includes a lot of information that pertains to the variable that has the type, not the type
    // itself.  SpirvType instead contains only information that can identify the type itself.
    angle::HashMap<SpirvType, SpirvTypeData, SpirvTypeHash> mTypeMap;

    // Various sections of SPIR-V.  Each section grows as SPIR-V is generated, and the final result
    // is obtained by stitching the sections together.  This puts the instructions in the order
    // required by the spec.
    spirv::Blob mSpirvExecutionModes;
    spirv::Blob mSpirvDebug;
    spirv::Blob mSpirvDecorations;
    spirv::Blob mSpirvTypeAndConstantDecls;
    spirv::Blob mSpirvTypePointerDecls;
    spirv::Blob mSpirvVariableDecls;
    spirv::Blob mSpirvFunctions;
    // A list of blocks created for the current function.  These are assembled by
    // assembleSpirvFunctionBlocks() when the function is entirely visited.  Local variables need to
    // be inserted at the beginning of the first function block, so the entire SPIR-V of the
    // function cannot be obtained until it's fully visited.
    //
    // The last block in this list is the one currently being written to.
    std::vector<SpirvBlock> mSpirvCurrentFunctionBlocks;

    // List of constants that are already defined (for reuse).
    spirv::IdRef mBoolConstants[2];
    angle::HashMap<uint32_t, spirv::IdRef> mUintConstants;
    angle::HashMap<uint32_t, spirv::IdRef> mIntConstants;
    angle::HashMap<uint32_t, spirv::IdRef> mFloatConstants;
    angle::HashMap<SpirvIdAndIdList, spirv::IdRef, SpirvIdAndIdListHash> mCompositeConstants;
    // TODO: Use null constants as optimization for when complex types are initialized with all
    // zeros.  http://anglebug.com/4889

    // List of type pointers that are already defined.
    // TODO: if all users call getTypeData(), move to SpirvTypeData.  http://anglebug.com/4889
    angle::HashMap<SpirvIdAndStorageClass, spirv::IdRef, SpirvIdAndStorageClassHash>
        mTypePointerIdMap;

    // List of function types that are already defined.
    angle::HashMap<SpirvIdAndIdList, spirv::IdRef, SpirvIdAndIdListHash> mFunctionTypeIdMap;

    // name hashing.
    ShHashFunction64 mHashFunction;
    NameMap &mNameMap;

    // Every resource that requires set & binding layout qualifiers is assigned set 0 and an
    // arbitrary binding.  Every input/output that requires a location layout qualifier is assigned
    // an arbitrary location as well.
    //
    // The link-time SPIR-V transformer modifies set, binding and location decorations in SPIR-V
    // directly.
    uint32_t mNextUnusedBinding;
    uint32_t mNextUnusedInputLocation;
    uint32_t mNextUnusedOutputLocation;
};
}  // namespace sh

#endif  // COMPILER_TRANSLATOR_BUILDSPIRV_H_

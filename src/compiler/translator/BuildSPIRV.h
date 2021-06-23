//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// BuildSPIRV: Helper for OutputSPIRV to build SPIR-V.
//

#ifndef COMPILER_TRANSLATOR_BUILDSPIRV_H_
#define COMPILER_TRANSLATOR_BUILDSPIRV_H_

#include "common/FixedVector.h"
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

    // If a structure is used in two I/O blocks or output varyings with and without the invariant
    // qualifier, it would also have to generate two SPIR-V types, as its fields' Invariant
    // decorations would be different.
    bool isInvariant = false;

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

    uint8_t primarySize   = 1;
    uint8_t secondarySize = 1;

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

        // Invariant must only affect the type if it's a block type.
        ASSERT(!type.isInvariant || type.block != nullptr);

        size_t result = 0;

        if (!type.arraySizes.empty())
        {
            result = angle::ComputeGenericHash(type.arraySizes.data(),
                                               type.arraySizes.size() * sizeof(type.arraySizes[0]));
        }

        if (type.block != nullptr)
        {
            return result ^ angle::ComputeGenericHash(&type.block, sizeof(type.block)) ^
                   static_cast<size_t>(type.isInvariant) ^ (type.blockStorage << 1);
        }

        static_assert(sh::EbtLast < 256, "Basic type doesn't fit in uint8_t");
        static_assert(sh::EbsLast < 8, "Block storage doesn't fit in 3 bits");
        static_assert(sh::EiifLast < 32, "Image format doesn't fit in 5 bits");
        ASSERT(type.primarySize > 0 && type.primarySize <= 4);
        ASSERT(type.secondarySize > 0 && type.secondarySize <= 4);

        const uint8_t properties[4] = {
            static_cast<uint8_t>(type.type),
            static_cast<uint8_t>((type.primarySize - 1) | (type.secondarySize - 1) << 2 |
                                 type.isSamplerBaseImage << 4),
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
    // applicable).
    uint32_t baseAlignment;
    uint32_t sizeInStorageBlock;
};

// Decorations to be applied to variable or intermediate ids which are not part of the SPIR-V type
// and are not specific enough (like DescriptorSet) to be handled automatically.  Currently, these
// are:
//
//     RelaxedPrecision: used to implement |lowp| and |mediump|
//     NoContraction: used to implement |precise|.  TODO: support this.  It requires the precise
//                    property to be promoted through the nodes in the AST, which currently isn't.
//                    http://anglebug.com/4889
//     Invariant: used to implement |invariant|, which is applied to output variables.
//
// Note that Invariant applies to variables and NoContraction to arithmetic instructions, so they
// are mutually exclusive and a maximum of 2 decorations are possible.  FixedVector::push_back will
// ASSERT if the given size is ever not enough.
using SpirvDecorations = angle::FixedVector<spv::Decoration, 2>;

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

// Conditional code, constituting ifs, switches and loops.
struct SpirvConditional
{
    // The id of blocks that make up the conditional.
    //
    // - For if, there are three blocks: the then, else and merge blocks
    // - For loops, there are four blocks: the condition, body, continue and merge blocks
    // - For switch, there are a number of blocks based on the cases.
    //
    // In all cases, the merge block is the last block in this list.  When the conditional is done
    // with, that's the block that will be made "current" and future instructions written to.  The
    // merge block is also the branch target of "break" instructions.
    //
    // For loops, the continue target block is the one before last block in this list.
    std::vector<spirv::IdRef> blockIds;

    // Up to which block is already generated.  Used by nextConditionalBlock() to generate a block
    // and give it an id pre-determined in blockIds.
    size_t nextBlockToWrite = 0;

    // Used to determine if continue will affect this (i.e. it's a loop).
    bool isContinuable = false;
    // Used to determine if break will affect this (i.e. it's a loop or switch).
    bool isBreakable = false;
};

// Helper class to construct SPIR-V
class SPIRVBuilder : angle::NonCopyable
{
  public:
    SPIRVBuilder(TCompiler *compiler,
                 ShCompileOptions compileOptions,
                 bool forceHighp,
                 ShHashFunction64 hashFunction,
                 NameMap &nameMap)
        : mCompiler(compiler),
          mCompileOptions(compileOptions),
          mShaderType(gl::FromGLenum<gl::ShaderType>(compiler->getShaderType())),
          mDisableRelaxedPrecision(forceHighp),
          mNextAvailableId(1),
          mHashFunction(hashFunction),
          mNameMap(nameMap),
          mNextUnusedBinding(0),
          mNextUnusedInputLocation(0),
          mNextUnusedOutputLocation(0)
    {}

    spirv::IdRef getNewId(const SpirvDecorations &decorations);
    TLayoutBlockStorage getBlockStorage(const TType &type) const;
    SpirvType getSpirvType(const TType &type, TLayoutBlockStorage blockStorage) const;
    const SpirvTypeData &getTypeData(const TType &type, TLayoutBlockStorage blockStorage);
    const SpirvTypeData &getSpirvTypeData(const SpirvType &type, const TSymbol *block);
    spirv::IdRef getTypePointerId(spirv::IdRef typeId, spv::StorageClass storageClass);
    spirv::IdRef getFunctionTypeId(spirv::IdRef returnTypeId, const spirv::IdRefList &paramTypeIds);

    // Decorations that may apply to intermediate instructions (in addition to variables).
    SpirvDecorations getDecorations(const TType &type);

    // Extended instructions
    spirv::IdRef getExtInstImportIdStd();

    spirv::Blob *getSpirvDebug() { return &mSpirvDebug; }
    spirv::Blob *getSpirvDecorations() { return &mSpirvDecorations; }
    spirv::Blob *getSpirvTypeAndConstantDecls() { return &mSpirvTypeAndConstantDecls; }
    spirv::Blob *getSpirvTypePointerDecls() { return &mSpirvTypePointerDecls; }
    spirv::Blob *getSpirvFunctionTypeDecls() { return &mSpirvFunctionTypeDecls; }
    spirv::Blob *getSpirvVariableDecls() { return &mSpirvVariableDecls; }
    spirv::Blob *getSpirvFunctions() { return &mSpirvFunctions; }
    spirv::Blob *getSpirvCurrentFunctionBlock()
    {
        ASSERT(!mSpirvCurrentFunctionBlocks.empty() &&
               !mSpirvCurrentFunctionBlocks.back().isTerminated);
        return &mSpirvCurrentFunctionBlocks.back().body;
    }
    spirv::IdRef getSpirvCurrentFunctionBlockId()
    {
        ASSERT(!mSpirvCurrentFunctionBlocks.empty() &&
               !mSpirvCurrentFunctionBlocks.back().isTerminated);
        return mSpirvCurrentFunctionBlocks.back().labelId;
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
    const SpirvConditional *getCurrentConditional() { return &mConditionalStack.back(); }

    bool isInvariantOutput(const TType &type) const;

    void addCapability(spv::Capability capability);
    void setEntryPointId(spirv::IdRef id);
    void addEntryPointInterfaceVariableId(spirv::IdRef id);
    void writePerVertexBuiltIns(const TType &type, spirv::IdRef typeId);
    void writeInterfaceVariableDecorations(const TType &type, spirv::IdRef variableId);
    void writeBranchConditional(spirv::IdRef conditionValue,
                                spirv::IdRef trueBlock,
                                spirv::IdRef falseBlock,
                                spirv::IdRef mergeBlock);
    void writeBranchConditionalBlockEnd();
    void writeLoopHeader(spirv::IdRef branchToBlock,
                         spirv::IdRef continueBlock,
                         spirv::IdRef mergeBlock);
    void writeLoopConditionEnd(spirv::IdRef conditionValue,
                               spirv::IdRef branchToBlock,
                               spirv::IdRef mergeBlock);
    void writeLoopContinueEnd(spirv::IdRef headerBlock);
    void writeLoopBodyEnd(spirv::IdRef continueBlock);
    void writeSwitch(spirv::IdRef conditionValue,
                     spirv::IdRef defaultBlock,
                     const spirv::PairLiteralIntegerIdRefList &targetPairList,
                     spirv::IdRef mergeBlock);
    void writeSwitchCaseBlockEnd();

    spirv::IdRef getBoolConstant(bool value);
    spirv::IdRef getUintConstant(uint32_t value);
    spirv::IdRef getIntConstant(int32_t value);
    spirv::IdRef getFloatConstant(float value);
    spirv::IdRef getCompositeConstant(spirv::IdRef typeId, const spirv::IdRefList &values);

    // Helpers to start and end a function.
    void startNewFunction(spirv::IdRef functionId, const TFunction *func);
    void assembleSpirvFunctionBlocks();

    // Helper to declare a variable.  Function-local variables must be placed in the first block of
    // the current function.
    spirv::IdRef declareVariable(spirv::IdRef typeId,
                                 spv::StorageClass storageClass,
                                 const SpirvDecorations &decorations,
                                 spirv::IdRef *initializerId,
                                 const char *name);
    // Helper to declare specialization constants.
    spirv::IdRef declareSpecConst(TBasicType type, int id, const char *name);

    // Helpers for conditionals.
    void startConditional(size_t blockCount, bool isContinuable, bool isBreakable);
    void nextConditionalBlock();
    void endConditional();
    bool isInLoop() const;
    spirv::IdRef getBreakTargetId() const;
    spirv::IdRef getContinueTargetId() const;

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
    SpirvTypeData declareType(const SpirvType &type, const TSymbol *block);

    const SpirvTypeData &getFieldTypeDataForAlignmentAndSize(const TType &type,
                                                             TLayoutBlockStorage blockStorage);
    uint32_t calculateBaseAlignmentAndSize(const SpirvType &type, uint32_t *sizeInStorageBlockOut);
    uint32_t calculateSizeAndWriteOffsetDecorations(const SpirvType &type, spirv::IdRef typeId);
    void writeMemberDecorations(const SpirvType &type, spirv::IdRef typeId);

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

    void generateExecutionModes(spirv::Blob *blob);

    ANGLE_MAYBE_UNUSED TCompiler *mCompiler;
    ShCompileOptions mCompileOptions;
    gl::ShaderType mShaderType;
    const bool mDisableRelaxedPrecision;

    // Capabilities the shader is using.  Accumulated as the instructions are generated.  The Shader
    // capability is unconditionally generated, so it's not tracked.
    std::set<spv::Capability> mCapabilities;

    // The list of interface variables and the id of main() populated as the instructions are
    // generated.  Used for the OpEntryPoint instruction.
    spirv::IdRefList mEntryPointInterfaceList;
    spirv::IdRef mEntryPointId;

    // Id of imported instructions, if used.
    spirv::IdRef mExtInstImportIdStd;

    // Current ID bound, used to allocate new ids.
    spirv::IdRef mNextAvailableId;

    // A map from the AST type to the corresponding SPIR-V ID and associated data.  Note that TType
    // includes a lot of information that pertains to the variable that has the type, not the type
    // itself.  SpirvType instead contains only information that can identify the type itself.
    angle::HashMap<SpirvType, SpirvTypeData, SpirvTypeHash> mTypeMap;

    // Various sections of SPIR-V.  Each section grows as SPIR-V is generated, and the final result
    // is obtained by stitching the sections together.  This puts the instructions in the order
    // required by the spec.
    spirv::Blob mSpirvDebug;
    spirv::Blob mSpirvDecorations;
    spirv::Blob mSpirvTypeAndConstantDecls;
    spirv::Blob mSpirvTypePointerDecls;
    spirv::Blob mSpirvFunctionTypeDecls;
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

    // Stack of conditionals.  When an if, loop or switch is visited, a new conditional scope is
    // added.  When the conditional construct is entirely visited, it's popped.  As the blocks of
    // the conditional constructs are visited, ids are consumed from the top of the stack.  When
    // break or continue is visited, the stack is traversed backwards until a loop or switch is
    // found.
    std::vector<SpirvConditional> mConditionalStack;

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

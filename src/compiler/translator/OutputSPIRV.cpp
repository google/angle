//
// Copyright 2021 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// OutputSPIRV: Generate SPIR-V from the AST.
//

#include "compiler/translator/OutputSPIRV.h"

#include "angle_gl.h"
#include "common/debug.h"
#include "common/mathutil.h"
#include "common/spirv/spirv_instruction_builder_autogen.h"
#include "compiler/translator/BuildSPIRV.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/tree_util/IntermTraverse.h"

#include <cfloat>

// SPIR-V tools include for disassembly
#include <spirv-tools/libspirv.hpp>

// Enable this for debug logging of pre-transform SPIR-V:
#if !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)
#    define ANGLE_DEBUG_SPIRV_TRANSFORMER 0
#endif  // !defined(ANGLE_DEBUG_SPIRV_TRANSFORMER)

namespace sh
{
namespace
{
// A struct to hold either SPIR-V ids or literal constants.   If id is not valid, a literal is
// assumed.
struct SpirvIdOrLiteral
{
    SpirvIdOrLiteral() = default;
    SpirvIdOrLiteral(const spirv::IdRef idIn) : id(idIn) {}
    SpirvIdOrLiteral(const spirv::LiteralInteger literalIn) : literal(literalIn) {}

    spirv::IdRef id;
    spirv::LiteralInteger literal;
};

// A data structure to facilitate generating array indexing, block field selection, swizzle and
// such.  Used in conjunction with NodeData which includes the access chain's baseId and idList.
//
// - rvalue[literal].field[literal] generates OpCompositeExtract
// - rvalue.x generates OpCompositeExtract
// - rvalue.xyz generates OpVectorShuffle
// - rvalue.xyz[i] generates OpVectorExtractDynamic (xyz[i] itself generates an
//   OpVectorExtractDynamic as well)
// - rvalue[i].field[j] generates a temp variable OpStore'ing rvalue and then generating an
//   OpAccessChain and OpLoad
//
// - lvalue[i].field[j].x generates OpAccessChain and OpStore
// - lvalue.xyz generates an OpLoad followed by OpVectorShuffle and OpStore
// - lvalue.xyz[i] generates OpAccessChain and OpStore (xyz[i] itself generates an
//   OpVectorExtractDynamic as well)
//
// storageClass == Max implies an rvalue.
//
struct AccessChain
{
    // The storage class for lvalues.  If Max, it's an rvalue.
    spv::StorageClass storageClass = spv::StorageClassMax;
    // If the access chain ends in swizzle, the swizzle components are specified here.  Swizzles
    // select multiple components so need special treatment when used as lvalue.
    std::vector<uint32_t> swizzles;
    // If a vector component is selected dynamically (i.e. indexed with a non-literal index),
    // dynamicComponent will contain the id of the index.
    spirv::IdRef dynamicComponent;

    // Type of expression before swizzle is applied, after swizzle is applied and after dynamic
    // component is applied.
    spirv::IdRef preSwizzleTypeId;
    spirv::IdRef postSwizzleTypeId;
    spirv::IdRef postDynamicComponentTypeId;

    // If the OpAccessChain is already generated (done by accessChainCollapse()), this caches the
    // id.
    spirv::IdRef accessChainId;

    // Whether all indices are literal.  Avoids looping through indices to determine this
    // information.
    bool areAllIndicesLiteral = true;
    // The number of components in the vector, if vector and swizzle is used.  This is cached to
    // avoid a type look up when handling swizzles.
    uint8_t swizzledVectorComponentCount = 0;
    // The block storage of the base id.  Used to correctly select the SPIR-V type id when visiting
    // EOpIndex* binary nodes.
    TLayoutBlockStorage baseBlockStorage;
};

bool IsAccessChainRValue(const AccessChain &accessChain)
{
    return accessChain.storageClass == spv::StorageClassMax;
}

// As each node is traversed, it produces data.  When visiting back the parent, this data is used to
// complete the data of the parent.  For example, the children of a function call (i.e. the
// arguments) each produce a SPIR-V id corresponding to the result of their expression.  The
// function call node itself in PostVisit uses those ids to generate the function call instruction.
struct NodeData
{
    // An id whose meaning depends on the node.  It could be a temporary id holding the result of an
    // expression, a reference to a variable etc.
    spirv::IdRef baseId;

    // List of relevant SPIR-V ids accumulated while traversing the children.  Meaning depends on
    // the node, for example a list of parameters to be passed to a function, a set of ids used to
    // construct an access chain etc.
    std::vector<SpirvIdOrLiteral> idList;

    // For constructing access chains.
    AccessChain accessChain;
};

// A traverser that generates SPIR-V as it walks the AST.
class OutputSPIRVTraverser : public TIntermTraverser
{
  public:
    OutputSPIRVTraverser(TCompiler *compiler, ShCompileOptions compileOptions);

    spirv::Blob getSpirv();

  protected:
    void visitSymbol(TIntermSymbol *node) override;
    void visitConstantUnion(TIntermConstantUnion *node) override;
    bool visitSwizzle(Visit visit, TIntermSwizzle *node) override;
    bool visitBinary(Visit visit, TIntermBinary *node) override;
    bool visitUnary(Visit visit, TIntermUnary *node) override;
    bool visitTernary(Visit visit, TIntermTernary *node) override;
    bool visitIfElse(Visit visit, TIntermIfElse *node) override;
    bool visitSwitch(Visit visit, TIntermSwitch *node) override;
    bool visitCase(Visit visit, TIntermCase *node) override;
    void visitFunctionPrototype(TIntermFunctionPrototype *node) override;
    bool visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node) override;
    bool visitAggregate(Visit visit, TIntermAggregate *node) override;
    bool visitBlock(Visit visit, TIntermBlock *node) override;
    bool visitGlobalQualifierDeclaration(Visit visit,
                                         TIntermGlobalQualifierDeclaration *node) override;
    bool visitDeclaration(Visit visit, TIntermDeclaration *node) override;
    bool visitLoop(Visit visit, TIntermLoop *node) override;
    bool visitBranch(Visit visit, TIntermBranch *node) override;
    void visitPreprocessorDirective(TIntermPreprocessorDirective *node) override;

  private:
    // Access chain handling.
    void accessChainPush(NodeData *data, spirv::IdRef index, spirv::IdRef typeId) const;
    void accessChainPushLiteral(NodeData *data,
                                spirv::LiteralInteger index,
                                spirv::IdRef typeId) const;
    void accessChainPushSwizzle(NodeData *data,
                                const TVector<int> &swizzle,
                                spirv::IdRef typeId,
                                uint8_t componentCount) const;
    void accessChainPushDynamicComponent(NodeData *data, spirv::IdRef index, spirv::IdRef typeId);
    spirv::IdRef accessChainCollapse(NodeData *data);
    spirv::IdRef accessChainLoad(NodeData *data);
    void accessChainStore(NodeData *data, spirv::IdRef value);

    // Access chain helpers.
    void makeAccessChainIdList(NodeData *data, spirv::IdRefList *idsOut);
    void makeAccessChainLiteralList(NodeData *data, spirv::LiteralIntegerList *literalsOut);
    spirv::IdRef getAccessChainTypeId(NodeData *data);

    // Node data handling.
    void nodeDataInitLValue(NodeData *data,
                            spirv::IdRef baseId,
                            spirv::IdRef typeId,
                            spv::StorageClass storageClass,
                            TLayoutBlockStorage blockStorage) const;
    void nodeDataInitRValue(NodeData *data, spirv::IdRef baseId, spirv::IdRef typeId) const;

    ANGLE_MAYBE_UNUSED TCompiler *mCompiler;
    ANGLE_MAYBE_UNUSED ShCompileOptions mCompileOptions;

    SPIRVBuilder mBuilder;

    // Traversal state.  Nodes generally push() once to this stack on PreVisit.  On InVisit and
    // PostVisit, they pop() once (data corresponding to the result of the child) and accumulate it
    // in back() (data corresponding to the node itself).  On PostVisit, code is generated.
    std::vector<NodeData> mNodeData;

    // A map of TSymbol to its SPIR-V id.  This could be a:
    //
    // - TVariable, or
    // - TInterfaceBlock: because TIntermSymbols referencing a field of an unnamed interface block
    //   don't reference the TVariable that defines the struct, but the TInterfaceBlock itself.
    angle::HashMap<const TSymbol *, spirv::IdRef> mSymbolIdMap;
};

spv::StorageClass GetStorageClass(const TType &type)
{
    // Opaque uniforms (samplers and images) have the UniformConstant storage class
    if (type.isSampler() || type.isImage())
    {
        return spv::StorageClassUniformConstant;
    }

    // Input varying and IO blocks have the Input storage class
    if (IsShaderIn(type.getQualifier()))
    {
        return spv::StorageClassInput;
    }

    // Output varying and IO blocks have the Input storage class
    if (IsShaderOut(type.getQualifier()))
    {
        return spv::StorageClassOutput;
    }

    // Uniform and storage buffers have the Uniform storage class
    if (type.isInterfaceBlock())
    {
        // I/O blocks must have already been classified as input or output above.
        ASSERT(!IsShaderIoBlock(type.getQualifier()));
        return spv::StorageClassUniform;
    }

    // Compute shader shared memory has the Workgroup storage class
    if (type.getQualifier() == EvqShared)
    {
        return spv::StorageClassWorkgroup;
    }

    // All other variables are either Private or Function, based on whether they are global or
    // function-local.
    if (type.getQualifier() == EvqGlobal)
    {
        return spv::StorageClassPrivate;
    }

    ASSERT(type.getQualifier() == EvqTemporary);
    return spv::StorageClassFunction;
}

OutputSPIRVTraverser::OutputSPIRVTraverser(TCompiler *compiler, ShCompileOptions compileOptions)
    : TIntermTraverser(true, true, true, &compiler->getSymbolTable()),
      mCompiler(compiler),
      mCompileOptions(compileOptions),
      mBuilder(gl::FromGLenum<gl::ShaderType>(compiler->getShaderType()),
               compiler->getHashFunction(),
               compiler->getNameMap())
{}

void OutputSPIRVTraverser::nodeDataInitLValue(NodeData *data,
                                              spirv::IdRef baseId,
                                              spirv::IdRef typeId,
                                              spv::StorageClass storageClass,
                                              TLayoutBlockStorage blockStorage) const
{
    *data = {};

    // Initialize the access chain as an lvalue.  Useful when an access chain is resolved, but needs
    // to be replaced by a reference to a temporary variable holding the result.
    data->baseId                       = baseId;
    data->accessChain.preSwizzleTypeId = typeId;
    data->accessChain.storageClass     = storageClass;
    data->accessChain.baseBlockStorage = blockStorage;
}

void OutputSPIRVTraverser::nodeDataInitRValue(NodeData *data,
                                              spirv::IdRef baseId,
                                              spirv::IdRef typeId) const
{
    *data = {};

    // Initialize the access chain as an rvalue.  Useful when an access chain is resolved, and needs
    // to be replaced by a reference to it.
    data->baseId                       = baseId;
    data->accessChain.preSwizzleTypeId = typeId;
}

void OutputSPIRVTraverser::accessChainPush(NodeData *data,
                                           spirv::IdRef index,
                                           spirv::IdRef typeId) const
{
    // Simply add the index to the chain of indices.
    data->idList.emplace_back(index);
    data->accessChain.areAllIndicesLiteral = false;
    data->accessChain.preSwizzleTypeId     = typeId;
}

void OutputSPIRVTraverser::accessChainPushLiteral(NodeData *data,
                                                  spirv::LiteralInteger index,
                                                  spirv::IdRef typeId) const
{
    // Add the literal integer in the chain of indices.  Since this is an id list, fake it as an id.
    data->idList.emplace_back(index);
    data->accessChain.preSwizzleTypeId = typeId;
}

void OutputSPIRVTraverser::accessChainPushSwizzle(NodeData *data,
                                                  const TVector<int> &swizzle,
                                                  spirv::IdRef typeId,
                                                  uint8_t componentCount) const
{
    AccessChain &accessChain = data->accessChain;

    // Record the swizzle as multi-component swizzles require special handling.  When loading
    // through the access chain, the swizzle is applied after loading the vector first (see
    // |accessChainLoad()|).  When storing through the access chain, the whole vector is loaded,
    // swizzled components overwritten and the whoel vector written back (see |accessChainStore()|).
    ASSERT(accessChain.swizzles.empty());

    if (swizzle.size() == 1)
    {
        // If this swizzle is selecting a single component, fold it into the access chain.
        accessChainPushLiteral(data, spirv::LiteralInteger(swizzle[0]), typeId);
    }
    else
    {
        // Otherwise keep them separate.
        accessChain.swizzles.insert(accessChain.swizzles.end(), swizzle.begin(), swizzle.end());
        accessChain.postSwizzleTypeId            = typeId;
        accessChain.swizzledVectorComponentCount = componentCount;
    }
}

void OutputSPIRVTraverser::accessChainPushDynamicComponent(NodeData *data,
                                                           spirv::IdRef index,
                                                           spirv::IdRef typeId)
{
    AccessChain &accessChain = data->accessChain;

    // Record the index used to dynamically select a component of a vector.
    ASSERT(!accessChain.dynamicComponent.valid());

    if (IsAccessChainRValue(accessChain) && accessChain.areAllIndicesLiteral)
    {
        // If the access chain is an rvalue with all-literal indices, keep this index separate so
        // that OpCompositeExtract can be used for the access chain up to this index.
        accessChain.dynamicComponent           = index;
        accessChain.postDynamicComponentTypeId = typeId;
        return;
    }

    if (!accessChain.swizzles.empty())
    {
        // Otherwise if there's a swizzle, fold the swizzle and dynamic component selection into a
        // single dynamic component selection.
        ASSERT(accessChain.swizzles.size() > 1);

        // Create a vector constant from the swizzles.
        spirv::IdRefList swizzleIds;
        for (uint32_t component : accessChain.swizzles)
        {
            swizzleIds.push_back(mBuilder.getUintConstant(component));
        }

        SpirvType type;
        type.type                     = EbtUInt;
        const spirv::IdRef uintTypeId = mBuilder.getSpirvTypeData(type, "").id;

        type.primarySize              = static_cast<uint8_t>(swizzleIds.size());
        const spirv::IdRef uvecTypeId = mBuilder.getSpirvTypeData(type, "").id;

        const spirv::IdRef swizzlesId = mBuilder.getNewId();
        spirv::WriteConstantComposite(mBuilder.getSpirvTypeAndConstantDecls(), uvecTypeId,
                                      swizzlesId, swizzleIds);

        // Index that vector constant with the dynamic index.  For example, vec.ywxz[i] becomes the
        // constant {1, 3, 0, 2} indexed with i, and that index used on vec.
        const spirv::IdRef newIndex = mBuilder.getNewId();
        spirv::WriteVectorExtractDynamic(mBuilder.getSpirvFunctions(), uintTypeId, newIndex,
                                         swizzlesId, index);

        index = newIndex;
        accessChain.swizzles.clear();
    }

    // Fold it into the access chain.
    accessChainPush(data, index, typeId);
}

spirv::IdRef OutputSPIRVTraverser::accessChainCollapse(NodeData *data)
{
    AccessChain &accessChain = data->accessChain;

    ASSERT(accessChain.storageClass != spv::StorageClassMax);

    if (accessChain.accessChainId.valid())
    {
        return accessChain.accessChainId;
    }

    // If there are no indices, the baseId is where access is done to/from.
    if (data->idList.empty())
    {
        accessChain.accessChainId = data->baseId;
        return accessChain.accessChainId;
    }

    // Otherwise create an OpAccessChain instruction.  Swizzle handling is special as it selects
    // multiple components, and is done differently for load and store.
    spirv::IdRefList indexIds;
    makeAccessChainIdList(data, &indexIds);

    const spirv::IdRef typePointerId =
        mBuilder.getTypePointerId(accessChain.preSwizzleTypeId, accessChain.storageClass);

    accessChain.accessChainId = mBuilder.getNewId();
    spirv::WriteAccessChain(mBuilder.getSpirvFunctions(), typePointerId, accessChain.accessChainId,
                            data->baseId, indexIds);

    return accessChain.accessChainId;
}

spirv::IdRef OutputSPIRVTraverser::accessChainLoad(NodeData *data)
{
    // Loading through the access chain can generate different instructions based on whether it's an
    // rvalue, the indices are literal, there's a swizzle etc.
    //
    // - If rvalue:
    //  * With indices:
    //   + All literal: OpCompositeExtract which uses literal integers to access the rvalue.
    //   + Otherwise: Can't use OpAccessChain on an rvalue, so create a temporary variable, OpStore
    //     the rvalue into it, then use OpAccessChain and OpLoad to load from it.
    //  * Without indices: Take the base id.
    // - If lvalue:
    //  * With indices: Use OpAccessChain and OpLoad
    //  * Without indices: Use OpLoad
    // - With swizzle: Use OpVectorShuffle on the result of the previous step
    // - With dynamic component: Use OpVectorExtractDynamic on the result of the previous step

    AccessChain &accessChain = data->accessChain;

    spirv::IdRef loadResult = data->baseId;

    if (IsAccessChainRValue(accessChain))
    {
        if (data->idList.size() > 0)
        {
            if (accessChain.areAllIndicesLiteral)
            {
                // Use OpCompositeExtract on an rvalue with all literal indices.
                spirv::LiteralIntegerList indexList;
                makeAccessChainLiteralList(data, &indexList);

                const spirv::IdRef result = mBuilder.getNewId();
                spirv::WriteCompositeExtract(mBuilder.getSpirvFunctions(),
                                             accessChain.preSwizzleTypeId, result, loadResult,
                                             indexList);
                loadResult = result;
            }
            else
            {
                // Create a temp variable to hold the rvalue so an access chain can be made on it.
                // TODO: variables need to be placed at the top of the SPIR-V block.  This will be
                // fixed when blocks are properly supported.  http://anglebug.com/4889
                const spirv::IdRef tempVar = mBuilder.getNewId();
                spirv::WriteVariable(mBuilder.getSpirvFunctions(), accessChain.preSwizzleTypeId,
                                     tempVar, spv::StorageClassFunction, nullptr);

                // Write the rvalue into the temp variable
                spirv::WriteStore(mBuilder.getSpirvFunctions(), tempVar, loadResult, nullptr);

                // Make the temp variable the source of the access chain.
                data->baseId                   = tempVar;
                data->accessChain.storageClass = spv::StorageClassFunction;

                // Load from the temp variable.
                const spirv::IdRef accessChainId = accessChainCollapse(data);
                loadResult                       = mBuilder.getNewId();
                spirv::WriteLoad(mBuilder.getSpirvFunctions(), accessChain.preSwizzleTypeId,
                                 loadResult, accessChainId, nullptr);
            }
        }
    }
    else
    {
        // Load from the access chain.
        const spirv::IdRef accessChainId = accessChainCollapse(data);
        loadResult                       = mBuilder.getNewId();
        spirv::WriteLoad(mBuilder.getSpirvFunctions(), accessChain.preSwizzleTypeId, loadResult,
                         accessChainId, nullptr);
    }

    if (!accessChain.swizzles.empty())
    {
        // Single-component swizzles are already folded into the index list.
        ASSERT(accessChain.swizzles.size() > 1);

        // Take the loaded value and use OpVectorShuffle to create the swizzle.
        spirv::LiteralIntegerList swizzleList;
        for (uint32_t component : accessChain.swizzles)
        {
            swizzleList.push_back(spirv::LiteralInteger(component));
        }

        const spirv::IdRef result = mBuilder.getNewId();
        spirv::WriteVectorShuffle(mBuilder.getSpirvFunctions(), accessChain.postSwizzleTypeId,
                                  result, loadResult, loadResult, swizzleList);
        loadResult = result;
    }

    if (accessChain.dynamicComponent.valid())
    {
        // Dynamic component in combination with swizzle is already folded.
        ASSERT(accessChain.swizzles.empty());

        // Use OpVectorExtractDynamic to select the component.
        const spirv::IdRef result = mBuilder.getNewId();
        spirv::WriteVectorExtractDynamic(mBuilder.getSpirvFunctions(),
                                         accessChain.postDynamicComponentTypeId, result, loadResult,
                                         accessChain.dynamicComponent);
        loadResult = result;
    }

    return loadResult;
}

void OutputSPIRVTraverser::accessChainStore(NodeData *data, spirv::IdRef value)
{
    // Storing through the access chain can generate different instructions based on whether the
    // there's a swizzle.
    //
    // - Without swizzle: Use OpAccessChain and OpStore
    // - With swizzle: Use OpAccessChain and OpLoad to load the vector, then use OpVectorShuffle to
    //   replace the components being overwritten.  Finally, use OpStore to write the result back.

    AccessChain &accessChain = data->accessChain;

    // Single-component swizzles are already folded into the indices.
    ASSERT(accessChain.swizzles.size() != 1);
    // Since store can only happen through lvalues, it's impossible to have a dynamic component as
    // that always gets folded into the indices except for rvalues.
    ASSERT(!accessChain.dynamicComponent.valid());

    const spirv::IdRef accessChainId = accessChainCollapse(data);

    if (!accessChain.swizzles.empty())
    {
        // Load the vector before the swizzle.
        const spirv::IdRef loadResult = mBuilder.getNewId();
        spirv::WriteLoad(mBuilder.getSpirvFunctions(), accessChain.preSwizzleTypeId, loadResult,
                         accessChainId, nullptr);

        // Overwrite the components being written.  This is done by first creating an identity
        // swizzle, then replacing the components being written with a swizzle from the value.  For
        // example, take the following:
        //
        //     vec4 v;
        //     v.zx = u;
        //
        // The OpVectorShuffle instruction takes two vectors (v and u) and selects components from
        // each (in this example, swizzles [0, 3] select from v and [4, 7] select from u).  This
        // algorithm first creates the identity swizzles {0, 1, 2, 3}, then replaces z and x (the
        // 0th and 2nd element) with swizzles from u (4 + {0, 1}) to get the result
        // {4+1, 1, 4+0, 3}.

        spirv::LiteralIntegerList swizzleList;
        for (uint32_t component = 0; component < accessChain.swizzledVectorComponentCount;
             ++component)
        {
            swizzleList.push_back(spirv::LiteralInteger(component));
        }
        uint32_t srcComponent = 0;
        for (uint32_t dstComponent : accessChain.swizzles)
        {
            swizzleList[dstComponent] =
                spirv::LiteralInteger(accessChain.swizzledVectorComponentCount + srcComponent);
            ++srcComponent;
        }

        // Use the generated swizzle to select components from the loaded vector and the value to be
        // written.  Use the final result as the value to be written to the vector.
        const spirv::IdRef result = mBuilder.getNewId();
        spirv::WriteVectorShuffle(mBuilder.getSpirvFunctions(), accessChain.postSwizzleTypeId,
                                  result, loadResult, value, swizzleList);
        value = result;
    }

    // Store through the access chain.
    spirv::WriteStore(mBuilder.getSpirvFunctions(), accessChainId, value, nullptr);
}

void OutputSPIRVTraverser::makeAccessChainIdList(NodeData *data, spirv::IdRefList *idsOut)
{
    for (size_t index = 0; index < data->idList.size(); ++index)
    {
        spirv::IdRef indexId = data->idList[index].id;

        if (!indexId.valid())
        {
            // The index is a literal integer, so replace it with an OpConstant id.
            indexId = mBuilder.getUintConstant(data->idList[index].literal);
        }

        idsOut->push_back(indexId);
    }
}

void OutputSPIRVTraverser::makeAccessChainLiteralList(NodeData *data,
                                                      spirv::LiteralIntegerList *literalsOut)
{
    for (size_t index = 0; index < data->idList.size(); ++index)
    {
        ASSERT(!data->idList[index].id.valid());
        literalsOut->push_back(data->idList[index].literal);
    }
}

spirv::IdRef OutputSPIRVTraverser::getAccessChainTypeId(NodeData *data)
{
    // Load and store through the access chain may be done in multiple steps.  These steps produce
    // the following types:
    //
    // - preSwizzleTypeId
    // - postSwizzleTypeId
    // - postDynamicComponentTypeId
    //
    // The last of these types is the final type of the expression this access chain corresponds to.
    const AccessChain &accessChain = data->accessChain;

    if (accessChain.postDynamicComponentTypeId.valid())
    {
        return accessChain.postDynamicComponentTypeId;
    }
    if (accessChain.postSwizzleTypeId.valid())
    {
        return accessChain.postSwizzleTypeId;
    }
    ASSERT(accessChain.preSwizzleTypeId.valid());
    return accessChain.preSwizzleTypeId;
}

void OutputSPIRVTraverser::visitSymbol(TIntermSymbol *node)
{
    mNodeData.emplace_back();

    // The symbol is either:
    //
    // - A variable (local, varying etc)
    // - An interface block
    // - A field of an unnamed interface block

    const TType &type                     = node->getType();
    const TInterfaceBlock *interfaceBlock = type.getInterfaceBlock();
    const TSymbol *symbol                 = interfaceBlock;
    if (interfaceBlock == nullptr)
    {
        symbol = &node->variable();
    }

    // Track the block storage; it's needed to determine the derived type in an access chain, but is
    // not promoted in intermediate nodes' TType.  Defaults to std140.
    TLayoutBlockStorage blockStorage = EbsUnspecified;
    if (interfaceBlock)
    {
        blockStorage = type.getLayoutQualifier().blockStorage;
        if (!IsShaderIoBlock(type.getQualifier()) && blockStorage != EbsStd430)
        {
            blockStorage = EbsStd140;
        }
    }

    spirv::IdRef typeId = mBuilder.getTypeData(type, blockStorage).id;

    nodeDataInitLValue(&mNodeData.back(), mSymbolIdMap[symbol], typeId, GetStorageClass(type),
                       blockStorage);

    // If a field of a nameless interface block, create an access chain.
    if (interfaceBlock && !type.isInterfaceBlock())
    {
        uint32_t fieldIndex = static_cast<uint32_t>(type.getInterfaceBlockFieldIndex());
        accessChainPushLiteral(&mNodeData.back(), spirv::LiteralInteger(fieldIndex), typeId);
    }
}

void OutputSPIRVTraverser::visitConstantUnion(TIntermConstantUnion *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();
}

bool OutputSPIRVTraverser::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The child will create one, which we won't pop.
        return true;
    }

    ASSERT(visit == PostVisit);
    ASSERT(mNodeData.size() >= 1);

    const TType &vectorType            = node->getOperand()->getType();
    const uint8_t vectorComponentCount = static_cast<uint8_t>(vectorType.getNominalSize());
    const TVector<int> &swizzle        = node->getSwizzleOffsets();

    // As an optimization, do nothing if the swizzle is selecting all the components of the vector
    // in order.
    bool isIdentity = swizzle.size() == vectorComponentCount;
    for (size_t index = 0; index < swizzle.size(); ++index)
    {
        isIdentity = isIdentity && static_cast<size_t>(swizzle[index]) == index;
    }

    if (isIdentity)
    {
        return true;
    }

    const spirv::IdRef typeId =
        mBuilder.getTypeData(node->getType(), mNodeData.back().accessChain.baseBlockStorage).id;

    accessChainPushSwizzle(&mNodeData.back(), swizzle, typeId, vectorComponentCount);

    return true;
}

bool OutputSPIRVTraverser::visitBinary(Visit visit, TIntermBinary *node)
{
    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The left child will create one, which we won't pop.
        return true;
    }

    if (visit == InVisit)
    {
        // Left child visited.  Take the entry it created as the current node's.
        ASSERT(mNodeData.size() >= 1);

        // As an optimization, if the index is EOpIndexDirect*, take the constant index directly and
        // add it to the access chain as constant.
        switch (node->getOp())
        {
            case EOpIndexDirect:
            case EOpIndexDirectStruct:
            case EOpIndexDirectInterfaceBlock:
                accessChainPushLiteral(
                    &mNodeData.back(),
                    spirv::LiteralInteger(node->getRight()->getAsConstantUnion()->getIConst(0)),
                    mBuilder
                        .getTypeData(node->getType(), mNodeData.back().accessChain.baseBlockStorage)
                        .id);
                // Don't visit the right child, it's already processed.
                return false;
            default:
                break;
        }

        return true;
    }

    // There are at least two entries, one for the left node and one for the right one.
    ASSERT(mNodeData.size() >= 2);

    // Load the result of the right node right away.
    const spirv::IdRef rightTypeId = getAccessChainTypeId(&mNodeData.back());
    const spirv::IdRef rightValue  = accessChainLoad(&mNodeData.back());
    mNodeData.pop_back();

    // For EOpIndex* operations, push the right value as an index to the left value's access chain.
    // For the other operations, evaluate the expression.
    NodeData &left = mNodeData.back();
    spirv::IdRef typeId;

    switch (node->getOp())
    {
        case EOpIndexDirect:
        case EOpIndexDirectStruct:
        case EOpIndexDirectInterfaceBlock:
            UNREACHABLE();
            break;
        case EOpIndexIndirect:
            typeId = mBuilder.getTypeData(node->getType(), left.accessChain.baseBlockStorage).id;
            if (!node->getLeft()->getType().isArray() && node->getLeft()->getType().isVector())
            {
                accessChainPushDynamicComponent(&left, rightValue, typeId);
            }
            else
            {
                accessChainPush(&left, rightValue, typeId);
            }
            break;
        case EOpAssign:
            // Store into the access chain.  Since the result of the (a = b) expression is b, change
            // the access chain to an unindexed rvalue which is |rightValue|.
            accessChainStore(&left, rightValue);
            nodeDataInitRValue(&left, rightValue, rightTypeId);
            break;
        default:
            UNIMPLEMENTED();
            break;
    }

    return true;
}

bool OutputSPIRVTraverser::visitUnary(Visit visit, TIntermUnary *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitTernary(Visit visit, TIntermTernary *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitIfElse(Visit visit, TIntermIfElse *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitSwitch(Visit visit, TIntermSwitch *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitCase(Visit visit, TIntermCase *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return false;
}

bool OutputSPIRVTraverser::visitBlock(Visit visit, TIntermBlock *node)
{
    // If global block, nothing to do.
    if (getCurrentTraversalDepth() == 0)
    {
        return true;
    }

    // When starting the block, generate an OpLabel instruction.  This is referenced by instructions
    // that reference the block such as OpBranchConditional.
    if (visit == PreVisit)
    {
        mNodeData.emplace_back();

        const spirv::IdRef blockLabelId = mBuilder.getNewId();
        spirv::WriteLabel(mBuilder.getSpirvFunctions(), blockLabelId);

        mNodeData.back().baseId = blockLabelId;
    }
    else
    {
        // Any node that needed to generate code has already done so, just clean up its data.  If
        // the child node has no effect, it's automatically discarded (such as variable.field[n].x,
        // side effects of n already having generated code).
        mNodeData.pop_back();
    }

    return true;
}

bool OutputSPIRVTraverser::visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node)
{
    if (visit == PreVisit)
    {
        const TFunction *function = node->getFunction();

        // Declare the function type
        const spirv::IdRef returnTypeId =
            mBuilder.getTypeData(function->getReturnType(), EbsUnspecified).id;

        spirv::IdRefList paramTypeIds;
        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            paramTypeIds.push_back(
                mBuilder.getTypeData(function->getParam(paramIndex)->getType(), EbsUnspecified).id);
        }

        const spirv::IdRef functionTypeId = mBuilder.getFunctionTypeId(returnTypeId, paramTypeIds);

        // Declare the function itself
        const spirv::IdRef functionId = mBuilder.getNewId();
        spirv::WriteFunction(mBuilder.getSpirvFunctions(), returnTypeId, functionId,
                             spv::FunctionControlMaskNone, functionTypeId);

        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            const spirv::IdRef paramId = mBuilder.getNewId();
            spirv::WriteFunctionParameter(mBuilder.getSpirvFunctions(), paramTypeIds[paramIndex],
                                          paramId);

            // TODO: Add to TVariable to variableId map so references to this variable can discover
            // the ID.  http://anglebug.com/4889
        }

        // Remember the ID of main() for the sake of OpEntryPoint.
        if (function->isMain())
        {
            mBuilder.setEntryPointId(functionId);
        }

        return true;
    }

    if (visit == PostVisit)
    {
        // TODO: if the function returns void, the AST may not have an explicit OpReturn node, so
        // generate one at the end if not already.  For testing, unconditionally add it.
        // http://anglebug.com/4889
        if (node->getFunction()->getReturnType().getBasicType() == EbtVoid)
        {
            spirv::WriteReturn(mBuilder.getSpirvFunctions());
        }

        // End the function
        spirv::WriteFunctionEnd(mBuilder.getSpirvFunctions());
    }

    return true;
}

bool OutputSPIRVTraverser::visitGlobalQualifierDeclaration(Visit visit,
                                                           TIntermGlobalQualifierDeclaration *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

void OutputSPIRVTraverser::visitFunctionPrototype(TIntermFunctionPrototype *node)
{
    // Nothing to do.  The function type is declared together with its definition.
}

bool OutputSPIRVTraverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return false;
}

bool OutputSPIRVTraverser::visitDeclaration(Visit visit, TIntermDeclaration *node)
{
    if (visit != PreVisit)
    {
        return true;
    }

    const TIntermSequence &sequence = *node->getSequence();

    // Enforced by ValidateASTOptions::validateMultiDeclarations.
    ASSERT(sequence.size() == 1);

    TIntermSymbol *symbol = sequence.front()->getAsSymbolNode();
    ASSERT(symbol != nullptr);

    const TType &type         = symbol->getType();
    const TVariable *variable = &symbol->variable();

    // If this is just a struct declaration (and not a variable declaration), don't declare the
    // struct up-front and let it be lazily defined.  If the struct is only used inside an interface
    // block for example, this avoids it being doubly defined (once with the unspecified block
    // storage and once with interface block's).
    if (type.isStructSpecifier() && variable->symbolType() == SymbolType::Empty)
    {
        return false;
    }

    const spirv::IdRef typeId = mBuilder.getTypeData(type, EbsUnspecified).id;

    // TODO: handle constant declarations.  http://anglebug.com/4889

    spv::StorageClass storageClass   = GetStorageClass(type);
    const spirv::IdRef typePointerId = mBuilder.getTypePointerId(typeId, storageClass);

    spirv::Blob *spirvSection = storageClass == spv::StorageClassFunction
                                    ? mBuilder.getSpirvFunctions()
                                    : mBuilder.getSpirvVariableDecls();

    const spirv::IdRef variableId = mBuilder.getNewId();
    // TODO: handle initializers.  http://anglebug.com/4889
    spirv::WriteVariable(spirvSection, typePointerId, variableId, storageClass, nullptr);

    if (IsShaderIn(type.getQualifier()) || IsShaderOut(type.getQualifier()))
    {
        // Add in and out variables to the list of interface variables.
        mBuilder.addEntryPointInterfaceVariableId(variableId);

        if (IsShaderIoBlock(type.getQualifier()) && type.isInterfaceBlock())
        {
            // For gl_PerVertex in particular, write the necessary BuiltIn decorations
            if (type.getQualifier() == EvqPerVertexIn || type.getQualifier() == EvqPerVertexOut)
            {
                mBuilder.writePerVertexBuiltIns(type, typeId);
            }

            // I/O blocks are decorated with Block
            spirv::WriteDecorate(mBuilder.getSpirvDecorations(), typeId, spv::DecorationBlock, {});
        }
    }
    else if (type.getBasicType() == EbtInterfaceBlock)
    {
        // For uniform and buffer variables, add Block and BufferBlock decorations respectively.
        const spv::Decoration decoration =
            type.getQualifier() == EvqUniform ? spv::DecorationBlock : spv::DecorationBufferBlock;
        spirv::WriteDecorate(mBuilder.getSpirvDecorations(), typeId, decoration, {});
    }

    // Write DescriptorSet, Binding, Location etc decorations if necessary.
    mBuilder.writeInterfaceVariableDecorations(type, variableId);

    // Output debug information.
    spirv::WriteName(mBuilder.getSpirvDebug(), variableId, mBuilder.hashName(variable).data());

    // Remember the id of the variable for future look up.  For interface blocks, also remember the
    // id of the interface block.
    ASSERT(mSymbolIdMap.count(variable) == 0);
    mSymbolIdMap[variable] = variableId;

    if (type.isInterfaceBlock())
    {
        ASSERT(mSymbolIdMap.count(type.getInterfaceBlock()) == 0);
        mSymbolIdMap[type.getInterfaceBlock()] = variableId;
    }

    return false;
}

bool OutputSPIRVTraverser::visitLoop(Visit visit, TIntermLoop *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

bool OutputSPIRVTraverser::visitBranch(Visit visit, TIntermBranch *node)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();

    return true;
}

void OutputSPIRVTraverser::visitPreprocessorDirective(TIntermPreprocessorDirective *node)
{
    // No preprocessor directives expected at this point.
    UNREACHABLE();
}

spirv::Blob OutputSPIRVTraverser::getSpirv()
{
    spirv::Blob result = mBuilder.getSpirv();

    // Validate that correct SPIR-V was generated
    ASSERT(spirv::Validate(result));

#if ANGLE_DEBUG_SPIRV_TRANSFORMER
    // Disassemble and log the generated SPIR-V for debugging.
    spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);
    std::string readableSpirv;
    spirvTools.Disassemble(result, &readableSpirv, 0);
    fprintf(stderr, "%s\n", readableSpirv.c_str());
#endif  // ANGLE_DEBUG_SPIRV_TRANSFORMER

    return result;
}
}  // anonymous namespace

bool OutputSPIRV(TCompiler *compiler, TIntermBlock *root, ShCompileOptions compileOptions)
{
    // Traverse the tree and generate SPIR-V instructions
    OutputSPIRVTraverser traverser(compiler, compileOptions);
    root->traverse(&traverser);

    // Generate the final SPIR-V and store in the sink
    spirv::Blob spirvBlob = traverser.getSpirv();
    compiler->getInfoSink().obj.setBinary(std::move(spirvBlob));

    return true;
}
}  // namespace sh

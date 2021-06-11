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

// Extended instructions
namespace spv
{
#include <spirv/unified1/GLSL.std.450.h>
}

// SPIR-V tools include for disassembly
#include <spirv-tools/libspirv.hpp>

// Enable this for debug logging of pre-transform SPIR-V:
#if !defined(ANGLE_DEBUG_SPIRV_GENERATION)
#    define ANGLE_DEBUG_SPIRV_GENERATION 0
#endif  // !defined(ANGLE_DEBUG_SPIRV_GENERATION)

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

    // Type of base expression, before swizzle is applied, after swizzle is applied and after
    // dynamic component is applied.
    spirv::IdRef baseTypeId;
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

struct FunctionIds
{
    // Id of the function type, return type and parameter types.
    spirv::IdRef functionTypeId;
    spirv::IdRef returnTypeId;
    spirv::IdRefList parameterTypeIds;

    // Id of the function itself.
    spirv::IdRef functionId;
};

bool IsAccessChainRValue(const AccessChain &accessChain)
{
    return accessChain.storageClass == spv::StorageClassMax;
}

bool IsAccessChainUnindexedLValue(const NodeData &data)
{
    return !IsAccessChainRValue(data.accessChain) && data.idList.empty() &&
           data.accessChain.swizzles.empty() && !data.accessChain.dynamicComponent.valid();
}

// A traverser that generates SPIR-V as it walks the AST.
class OutputSPIRVTraverser : public TIntermTraverser
{
  public:
    OutputSPIRVTraverser(TCompiler *compiler, ShCompileOptions compileOptions, bool forceHighp);
    ~OutputSPIRVTraverser() override;

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
    spirv::IdRef getSymbolIdAndStorageClass(const TSymbol *symbol,
                                            const TType &type,
                                            spv::StorageClass *storageClass);

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
    spirv::IdRef accessChainLoad(NodeData *data, const SpirvDecorations &decorations);
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

    void declareSpecConst(TIntermDeclaration *decl);
    spirv::IdRef createConstant(const TType &type,
                                TBasicType expectedBasicType,
                                const TConstantUnion *constUnion);
    spirv::IdRef createComplexConstant(const TType &type,
                                       spirv::IdRef typeId,
                                       const spirv::IdRefList &parameters);
    spirv::IdRef createConstructor(TIntermAggregate *node, spirv::IdRef typeId);
    spirv::IdRef createArrayOrStructConstructor(TIntermAggregate *node,
                                                spirv::IdRef typeId,
                                                const spirv::IdRefList &parameters);
    spirv::IdRef createConstructorVectorFromScalar(const TType &type,
                                                   spirv::IdRef typeId,
                                                   const spirv::IdRefList &parameters);
    spirv::IdRef createConstructorVectorFromNonScalar(TIntermAggregate *node,
                                                      spirv::IdRef typeId,
                                                      const spirv::IdRefList &parameters);
    spirv::IdRef createConstructorMatrixFromScalar(TIntermAggregate *node,
                                                   spirv::IdRef typeId,
                                                   const spirv::IdRefList &parameters);
    spirv::IdRef createConstructorMatrixFromVectors(TIntermAggregate *node,
                                                    spirv::IdRef typeId,
                                                    const spirv::IdRefList &parameters);
    spirv::IdRef createConstructorMatrixFromMatrix(TIntermAggregate *node,
                                                   spirv::IdRef typeId,
                                                   const spirv::IdRefList &parameters);
    spirv::IdRefList loadAllParams(TIntermOperator *node);
    void extractComponents(TIntermAggregate *node,
                           size_t componentCount,
                           const spirv::IdRefList &parameters,
                           spirv::IdRefList *extractedComponentsOut);

    void startShortCircuit(TIntermBinary *node);
    spirv::IdRef endShortCircuit(TIntermBinary *node, spirv::IdRef *typeId);

    spirv::IdRef visitOperator(TIntermOperator *node, spirv::IdRef resultTypeId);
    spirv::IdRef createIncrementDecrement(TIntermOperator *node, spirv::IdRef resultTypeId);
    spirv::IdRef createAtomicBuiltIn(TIntermOperator *node, spirv::IdRef resultTypeId);
    spirv::IdRef createTextureBuiltIn(TIntermOperator *node, spirv::IdRef resultTypeId);
    spirv::IdRef createImageBuiltIn(TIntermOperator *node, spirv::IdRef resultTypeId);

    spirv::IdRef createFunctionCall(TIntermAggregate *node, spirv::IdRef resultTypeId);

    ANGLE_MAYBE_UNUSED TCompiler *mCompiler;
    ShCompileOptions mCompileOptions;

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

    // A map of TFunction to its various SPIR-V ids.
    angle::HashMap<const TFunction *, FunctionIds> mFunctionIdMap;

    // Whether the current symbol being visited is being declared.
    bool mIsSymbolBeingDeclared = false;
};

spv::StorageClass GetStorageClass(const TType &type)
{
    // Opaque uniforms (samplers and images) have the UniformConstant storage class
    if (type.isSampler() || type.isImage())
    {
        return spv::StorageClassUniformConstant;
    }

    const TQualifier qualifier = type.getQualifier();

    // Input varying and IO blocks have the Input storage class
    if (IsShaderIn(qualifier))
    {
        return spv::StorageClassInput;
    }

    // Output varying and IO blocks have the Input storage class
    if (IsShaderOut(qualifier))
    {
        return spv::StorageClassOutput;
    }

    // Uniform and storage buffers have the Uniform storage class.  Default uniforms are gathered in
    // a uniform block as well.
    if (type.isInterfaceBlock() || qualifier == EvqUniform)
    {
        // I/O blocks must have already been classified as input or output above.
        ASSERT(!IsShaderIoBlock(qualifier));
        return spv::StorageClassUniform;
    }

    switch (qualifier)
    {
        case EvqShared:
            // Compute shader shared memory has the Workgroup storage class
            return spv::StorageClassWorkgroup;

        case EvqGlobal:
            // Global variables have the Private class.
            return spv::StorageClassPrivate;

        case EvqTemporary:
        case EvqIn:
        case EvqOut:
        case EvqInOut:
            // Function-local variables have the Function class
            return spv::StorageClassFunction;

        case EvqVertexID:
        case EvqInstanceID:
        case EvqFragCoord:
        case EvqFrontFacing:
        case EvqPointCoord:
        case EvqHelperInvocation:
        case EvqNumWorkGroups:
        case EvqWorkGroupID:
        case EvqLocalInvocationID:
        case EvqGlobalInvocationID:
        case EvqLocalInvocationIndex:
            return spv::StorageClassInput;

        case EvqFragDepth:
            return spv::StorageClassOutput;

        default:
            // TODO: http://anglebug.com/4889
            UNIMPLEMENTED();
    }

    UNREACHABLE();
    return spv::StorageClassPrivate;
}

OutputSPIRVTraverser::OutputSPIRVTraverser(TCompiler *compiler,
                                           ShCompileOptions compileOptions,
                                           bool forceHighp)
    : TIntermTraverser(true, true, true, &compiler->getSymbolTable()),
      mCompiler(compiler),
      mCompileOptions(compileOptions),
      mBuilder(compiler,
               compileOptions,
               forceHighp,
               compiler->getHashFunction(),
               compiler->getNameMap())
{}

OutputSPIRVTraverser::~OutputSPIRVTraverser()
{
    ASSERT(mNodeData.empty());
}

spirv::IdRef OutputSPIRVTraverser::getSymbolIdAndStorageClass(const TSymbol *symbol,
                                                              const TType &type,
                                                              spv::StorageClass *storageClass)
{
    *storageClass = GetStorageClass(type);
    auto iter     = mSymbolIdMap.find(symbol);
    if (iter != mSymbolIdMap.end())
    {
        return iter->second;
    }

    // This must be an implicitly defined variable, define it now.
    const char *name               = nullptr;
    spv::BuiltIn builtInDecoration = spv::BuiltInMax;

    switch (type.getQualifier())
    {
        case EvqVertexID:
            name              = "gl_VertexIndex";
            builtInDecoration = spv::BuiltInVertexIndex;
            break;
        case EvqInstanceID:
            name              = "gl_InstanceIndex";
            builtInDecoration = spv::BuiltInInstanceIndex;
            break;

        // Fragment shader built-ins
        case EvqFragCoord:
            name              = "gl_FragCoord";
            builtInDecoration = spv::BuiltInFragCoord;
            break;
        case EvqFrontFacing:
            name              = "gl_FrontFacing";
            builtInDecoration = spv::BuiltInFrontFacing;
            break;
        case EvqPointCoord:
            name              = "gl_PointCoord";
            builtInDecoration = spv::BuiltInPointCoord;
            break;
        case EvqFragDepth:
            name              = "gl_FragDepth";
            builtInDecoration = spv::BuiltInFragDepth;
            break;
        case EvqHelperInvocation:
            name              = "gl_HelperInvocation";
            builtInDecoration = spv::BuiltInHelperInvocation;
            break;

        // Compute shader built-ins
        case EvqNumWorkGroups:
            name              = "gl_NumWorkGroups";
            builtInDecoration = spv::BuiltInNumWorkgroups;
            break;
        case EvqWorkGroupID:
            name              = "gl_WorkGroupID";
            builtInDecoration = spv::BuiltInWorkgroupId;
            break;
        case EvqLocalInvocationID:
            name              = "gl_LocalInvocationID";
            builtInDecoration = spv::BuiltInLocalInvocationId;
            break;
        case EvqGlobalInvocationID:
            name              = "gl_GlobalInvocationID";
            builtInDecoration = spv::BuiltInGlobalInvocationId;
            break;
        case EvqLocalInvocationIndex:
            name              = "gl_LocalInvocationIndex";
            builtInDecoration = spv::BuiltInLocalInvocationIndex;
            break;
        default:
            // TODO: more built-ins.  http://anglebug.com/4889
            UNIMPLEMENTED();
    }

    const spirv::IdRef typeId = mBuilder.getTypeData(type, EbsUnspecified).id;
    const spirv::IdRef varId  = mBuilder.declareVariable(
        typeId, *storageClass, mBuilder.getDecorations(type), nullptr, name);

    mBuilder.addEntryPointInterfaceVariableId(varId);
    spirv::WriteDecorate(mBuilder.getSpirvDecorations(), varId, spv::DecorationBuiltIn,
                         {spirv::LiteralInteger(builtInDecoration)});

    mSymbolIdMap.insert({symbol, varId});
    return varId;
}

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
    data->accessChain.baseTypeId       = typeId;
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
    data->accessChain.baseTypeId       = typeId;
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
        const spirv::IdRef uintTypeId = mBuilder.getSpirvTypeData(type, nullptr).id;

        type.primarySize              = static_cast<uint8_t>(swizzleIds.size());
        const spirv::IdRef uvecTypeId = mBuilder.getSpirvTypeData(type, nullptr).id;

        const spirv::IdRef swizzlesId = mBuilder.getNewId({});
        spirv::WriteConstantComposite(mBuilder.getSpirvTypeAndConstantDecls(), uvecTypeId,
                                      swizzlesId, swizzleIds);

        // Index that vector constant with the dynamic index.  For example, vec.ywxz[i] becomes the
        // constant {1, 3, 0, 2} indexed with i, and that index used on vec.
        const spirv::IdRef newIndex = mBuilder.getNewId({});
        spirv::WriteVectorExtractDynamic(mBuilder.getSpirvCurrentFunctionBlock(), uintTypeId,
                                         newIndex, swizzlesId, index);

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

    accessChain.accessChainId = mBuilder.getNewId({});
    spirv::WriteAccessChain(mBuilder.getSpirvCurrentFunctionBlock(), typePointerId,
                            accessChain.accessChainId, data->baseId, indexIds);

    return accessChain.accessChainId;
}

spirv::IdRef OutputSPIRVTraverser::accessChainLoad(NodeData *data,
                                                   const SpirvDecorations &decorations)
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

                const spirv::IdRef result = mBuilder.getNewId(decorations);
                spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(),
                                             accessChain.preSwizzleTypeId, result, loadResult,
                                             indexList);
                loadResult = result;
            }
            else
            {
                // Create a temp variable to hold the rvalue so an access chain can be made on it.
                const spirv::IdRef tempVar =
                    mBuilder.declareVariable(accessChain.baseTypeId, spv::StorageClassFunction,
                                             decorations, nullptr, "indexable");

                // Write the rvalue into the temp variable
                spirv::WriteStore(mBuilder.getSpirvCurrentFunctionBlock(), tempVar, loadResult,
                                  nullptr);

                // Make the temp variable the source of the access chain.
                data->baseId                   = tempVar;
                data->accessChain.storageClass = spv::StorageClassFunction;

                // Load from the temp variable.
                const spirv::IdRef accessChainId = accessChainCollapse(data);
                loadResult                       = mBuilder.getNewId(decorations);
                spirv::WriteLoad(mBuilder.getSpirvCurrentFunctionBlock(),
                                 accessChain.preSwizzleTypeId, loadResult, accessChainId, nullptr);
            }
        }
    }
    else
    {
        // Load from the access chain.
        const spirv::IdRef accessChainId = accessChainCollapse(data);
        loadResult                       = mBuilder.getNewId(decorations);
        spirv::WriteLoad(mBuilder.getSpirvCurrentFunctionBlock(), accessChain.preSwizzleTypeId,
                         loadResult, accessChainId, nullptr);
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

        const spirv::IdRef result = mBuilder.getNewId(decorations);
        spirv::WriteVectorShuffle(mBuilder.getSpirvCurrentFunctionBlock(),
                                  accessChain.postSwizzleTypeId, result, loadResult, loadResult,
                                  swizzleList);
        loadResult = result;
    }

    if (accessChain.dynamicComponent.valid())
    {
        // Dynamic component in combination with swizzle is already folded.
        ASSERT(accessChain.swizzles.empty());

        // Use OpVectorExtractDynamic to select the component.
        const spirv::IdRef result = mBuilder.getNewId(decorations);
        spirv::WriteVectorExtractDynamic(mBuilder.getSpirvCurrentFunctionBlock(),
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
        const spirv::IdRef loadResult = mBuilder.getNewId({});
        spirv::WriteLoad(mBuilder.getSpirvCurrentFunctionBlock(), accessChain.preSwizzleTypeId,
                         loadResult, accessChainId, nullptr);

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
        const spirv::IdRef result = mBuilder.getNewId({});
        spirv::WriteVectorShuffle(mBuilder.getSpirvCurrentFunctionBlock(),
                                  accessChain.preSwizzleTypeId, result, loadResult, value,
                                  swizzleList);
        value = result;
    }

    // Store through the access chain.
    spirv::WriteStore(mBuilder.getSpirvCurrentFunctionBlock(), accessChainId, value, nullptr);
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

void OutputSPIRVTraverser::declareSpecConst(TIntermDeclaration *decl)
{
    const TIntermSequence &sequence = *decl->getSequence();
    ASSERT(sequence.size() == 1);

    TIntermBinary *assign = sequence.front()->getAsBinaryNode();
    ASSERT(assign != nullptr && assign->getOp() == EOpInitialize);

    TIntermSymbol *symbol = assign->getLeft()->getAsSymbolNode();
    ASSERT(symbol != nullptr && symbol->getType().getQualifier() == EvqSpecConst);

    TIntermConstantUnion *initializer = assign->getRight()->getAsConstantUnion();
    ASSERT(initializer != nullptr);

    const TType &type         = symbol->getType();
    const TVariable *variable = &symbol->variable();

    // All spec consts in ANGLE are initialized to 0.
    ASSERT(initializer->isZero(0));

    const spirv::IdRef specConstId =
        mBuilder.declareSpecConst(type.getBasicType(), type.getLayoutQualifier().location,
                                  mBuilder.hashName(variable).data());

    // Remember the id of the variable for future look up.
    ASSERT(mSymbolIdMap.count(variable) == 0);
    mSymbolIdMap[variable] = specConstId;
}

spirv::IdRef OutputSPIRVTraverser::createConstant(const TType &type,
                                                  TBasicType expectedBasicType,
                                                  const TConstantUnion *constUnion)
{
    const spirv::IdRef typeId = mBuilder.getTypeData(type, EbsUnspecified).id;
    spirv::IdRefList componentIds;

    if (type.getBasicType() == EbtStruct)
    {
        // If it's a struct constant, get the constant id for each field.
        for (const TField *field : type.getStruct()->fields())
        {
            const TType *fieldType = field->type();
            componentIds.push_back(
                createConstant(*fieldType, fieldType->getBasicType(), constUnion));

            constUnion += fieldType->getObjectSize();
        }
    }
    else
    {
        // Otherwise get the constant id for each component.
        const size_t size = type.getObjectSize();
        ASSERT(expectedBasicType == EbtFloat || expectedBasicType == EbtInt ||
               expectedBasicType == EbtUInt || expectedBasicType == EbtBool);

        for (size_t component = 0; component < size; ++component, ++constUnion)
        {
            spirv::IdRef componentId;

            // If the constant has a different type than expected, cast it right away.
            TConstantUnion castConstant;
            bool valid = castConstant.cast(expectedBasicType, *constUnion);
            ASSERT(valid);

            switch (castConstant.getType())
            {
                case EbtFloat:
                    componentId = mBuilder.getFloatConstant(castConstant.getFConst());
                    break;
                case EbtInt:
                    componentId = mBuilder.getIntConstant(castConstant.getIConst());
                    break;
                case EbtUInt:
                    componentId = mBuilder.getUintConstant(castConstant.getUConst());
                    break;
                case EbtBool:
                    componentId = mBuilder.getBoolConstant(castConstant.getBConst());
                    break;
                default:
                    UNREACHABLE();
            }
            componentIds.push_back(componentId);
        }
    }

    // If this is a composite, create a composite constant from the components.
    if (type.getBasicType() == EbtStruct || componentIds.size() > 1)
    {
        return createComplexConstant(type, typeId, componentIds);
    }

    // Otherwise return the sole component.
    ASSERT(componentIds.size() == 1);
    return componentIds[0];
}

spirv::IdRef OutputSPIRVTraverser::createComplexConstant(const TType &type,
                                                         spirv::IdRef typeId,
                                                         const spirv::IdRefList &parameters)
{
    if (type.isMatrix() && !type.isArray())
    {
        // Matrices are constructed from its columns.
        spirv::IdRefList columnIds;

        SpirvType columnType            = mBuilder.getSpirvType(type, EbsUnspecified);
        columnType.primarySize          = columnType.secondarySize;
        columnType.secondarySize        = 1;
        const spirv::IdRef columnTypeId = mBuilder.getSpirvTypeData(columnType, nullptr).id;

        for (int columnIndex = 0; columnIndex < type.getCols(); ++columnIndex)
        {
            auto columnParametersStart = parameters.begin() + columnIndex * type.getRows();
            spirv::IdRefList columnParameters(columnParametersStart,
                                              columnParametersStart + type.getRows());

            columnIds.push_back(mBuilder.getCompositeConstant(columnTypeId, columnParameters));
        }

        return mBuilder.getCompositeConstant(typeId, columnIds);
    }

    return mBuilder.getCompositeConstant(typeId, parameters);
}

spirv::IdRef OutputSPIRVTraverser::createConstructor(TIntermAggregate *node, spirv::IdRef typeId)
{
    const TType &type                = node->getType();
    const TIntermSequence &arguments = *node->getSequence();
    const TType &arg0Type            = arguments[0]->getAsTyped()->getType();

    // Take each constructor argument that is visited and evaluate it as rvalue
    spirv::IdRefList parameters = loadAllParams(node);

    // Constructors in GLSL can take various shapes, resulting in different translations to SPIR-V
    // (in each case, if the parameter doesn't match the type being constructed, it must be cast):
    //
    // - float(f): This should translate to just f
    // - vecN(f): This should translate to OpCompositeConstruct %vecN %f %f .. %f
    // - vecN(v1.zy, v2.x): This can technically translate to OpCompositeConstruct with two ids; the
    //   results of v1.zy and v2.x.  However, for simplicity it's easier to generate that
    //   instruction with three ids; the results of v1.z, v1.y and v2.x (see below where a matrix is
    //   used as parameter).
    // - vecN(m): This takes N components from m in column-major order (for example, vec4
    //   constructed out of a 4x3 matrix would select components (0,0), (0,1), (0,2) and (1,0)).
    //   This translates to OpCompositeConstruct with the id of the individual components extracted
    //   from m.
    // - matNxM(f): This creates a diagonal matrix.  It generates N OpCompositeConstruct
    //   instructions for each column (which are vecM), followed by an OpCompositeConstruct that
    //   constructs the final result.
    // - matNxM(m):
    //   * With m larger than NxM, this extracts a submatrix out of m.  It generates
    //     OpCompositeExtracts for N columns of m, followed by an OpVectorShuffle (swizzle) if the
    //     rows of m are more than M.  OpCompositeConstruct is used to construct the final result.
    //   * If m is not larger than NxM, an identity matrix is created and superimposed with m.
    //     OpCompositeExtract is used to extract each component of m (that is necessary), and
    //     together with the zero or one constants necessary used to create the columns (with
    //     OpCompositeConstruct).  OpCompositeConstruct is used to construct the final result.
    // - matNxM(v1.zy, v2.x, ...): Similarly to constructing a vector, a list of single components
    //   are extracted from the parameters, which are divided up and used to construct each column,
    //   which is finally constructed into the final result.
    //
    // Additionally, array and structs are constructed by OpCompositeConstruct followed by ids of
    // each parameter which must enumerate every individual element / field.

    // In some cases, constructors with constant value are not folded.  That is handled here.
    if (node->hasConstantValue())
    {
        return createComplexConstant(node->getType(), typeId, parameters);
    }

    if (type.isArray() || type.getStruct() != nullptr)
    {
        return createArrayOrStructConstructor(node, typeId, parameters);
    }

    if (type.isScalar())
    {
        // TODO: handle casting.  http://anglebug.com/4889.
        return parameters[0];
    }

    if (type.isVector())
    {
        if (arguments.size() == 1 && arg0Type.isScalar())
        {
            return createConstructorVectorFromScalar(node->getType(), typeId, parameters);
        }

        return createConstructorVectorFromNonScalar(node, typeId, parameters);
    }

    ASSERT(type.isMatrix());

    if (arg0Type.isScalar())
    {
        return createConstructorMatrixFromScalar(node, typeId, parameters);
    }
    if (arg0Type.isMatrix())
    {
        return createConstructorMatrixFromMatrix(node, typeId, parameters);
    }
    return createConstructorMatrixFromVectors(node, typeId, parameters);
}

spirv::IdRef OutputSPIRVTraverser::createArrayOrStructConstructor(
    TIntermAggregate *node,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   parameters);
    return result;
}

spirv::IdRef OutputSPIRVTraverser::createConstructorVectorFromScalar(
    const TType &type,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    // vecN(f) translates to OpCompositeConstruct %vecN %f ... %f
    ASSERT(parameters.size() == 1);
    spirv::IdRefList replicatedParameter(type.getNominalSize(), parameters[0]);

    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(type));
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   replicatedParameter);
    return result;
}

spirv::IdRef OutputSPIRVTraverser::createConstructorVectorFromNonScalar(
    TIntermAggregate *node,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    // vecN(v1.zy, v2.x) translates to OpCompositeConstruct %vecN %v1.z %v1.y %v2.x
    // vecN(m) translates to OpCompositeConstruct %vecN %m[0][0] %m[0][1] ...
    spirv::IdRefList extractedComponents;
    extractComponents(node, node->getType().getNominalSize(), parameters, &extractedComponents);

    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   extractedComponents);
    return result;
}

spirv::IdRef OutputSPIRVTraverser::createConstructorMatrixFromScalar(
    TIntermAggregate *node,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    // matNxM(f) translates to
    //
    //     %c0 = OpCompositeConstruct %vecM %f %zero %zero ..
    //     %c1 = OpCompositeConstruct %vecM %zero %f %zero ..
    //     %c2 = OpCompositeConstruct %vecM %zero %zero %f ..
    //     ...
    //     %m  = OpCompositeConstruct %matNxM %c0 %c1 %c2 ...

    const TType &type = node->getType();
    // TODO: handle casting.  http://anglebug.com/4889.
    const spirv::IdRef scalarId = parameters[0];
    spirv::IdRef zeroId;

    SpirvDecorations decorations = mBuilder.getDecorations(type);

    switch (type.getBasicType())
    {
        case EbtFloat:
            zeroId = mBuilder.getFloatConstant(0);
            break;
        case EbtInt:
            zeroId = mBuilder.getIntConstant(0);
            break;
        case EbtUInt:
            zeroId = mBuilder.getUintConstant(0);
            break;
        case EbtBool:
            zeroId = mBuilder.getBoolConstant(0);
            break;
        default:
            UNREACHABLE();
    }

    spirv::IdRefList componentIds(type.getRows(), zeroId);
    spirv::IdRefList columnIds;

    SpirvType columnType            = mBuilder.getSpirvType(type, EbsUnspecified);
    columnType.primarySize          = columnType.secondarySize;
    columnType.secondarySize        = 1;
    const spirv::IdRef columnTypeId = mBuilder.getSpirvTypeData(columnType, nullptr).id;

    for (int columnIndex = 0; columnIndex < type.getCols(); ++columnIndex)
    {
        columnIds.push_back(mBuilder.getNewId(decorations));

        // Place the scalar at the correct index (diagonal of the matrix, i.e. row == col).
        componentIds[columnIndex] = scalarId;
        if (columnIndex > 0)
        {
            componentIds[columnIndex - 1] = zeroId;
        }

        // Create the column.
        spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                       columnIds.back(), componentIds);
    }

    // Create the matrix out of the columns.
    const spirv::IdRef result = mBuilder.getNewId(decorations);
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   columnIds);
    return result;
}

spirv::IdRef OutputSPIRVTraverser::createConstructorMatrixFromVectors(
    TIntermAggregate *node,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    // matNxM(v1.zy, v2.x, ...) translates to:
    //
    //     %c0 = OpCompositeConstruct %vecM %v1.z %v1.y %v2.x ..
    //     ...
    //     %m  = OpCompositeConstruct %matNxM %c0 %c1 %c2 ...

    const TType &type = node->getType();

    SpirvDecorations decorations = mBuilder.getDecorations(type);

    spirv::IdRefList extractedComponents;
    extractComponents(node, type.getCols() * type.getRows(), parameters, &extractedComponents);

    spirv::IdRefList columnIds;

    SpirvType columnType            = mBuilder.getSpirvType(type, EbsUnspecified);
    columnType.primarySize          = columnType.secondarySize;
    columnType.secondarySize        = 1;
    const spirv::IdRef columnTypeId = mBuilder.getSpirvTypeData(columnType, nullptr).id;

    // Chunk up the extracted components by column and construct intermediary vectors.
    for (int columnIndex = 0; columnIndex < type.getCols(); ++columnIndex)
    {
        columnIds.push_back(mBuilder.getNewId(decorations));

        auto componentsStart = extractedComponents.begin() + columnIndex * type.getRows();
        const spirv::IdRefList componentIds(componentsStart, componentsStart + type.getRows());

        // Create the column.
        spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                       columnIds.back(), componentIds);
    }

    const spirv::IdRef result = mBuilder.getNewId(decorations);
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   columnIds);
    return result;
}

spirv::IdRef OutputSPIRVTraverser::createConstructorMatrixFromMatrix(
    TIntermAggregate *node,
    spirv::IdRef typeId,
    const spirv::IdRefList &parameters)
{
    // matNxM(m) translates to:
    //
    // - If m is SxR where S>=N and R>=M:
    //
    //     %c0 = OpCompositeExtract %vecR %m 0
    //     %c1 = OpCompositeExtract %vecR %m 1
    //     ...
    //     // If R (column size of m) != M, OpVectorShuffle to extract M components out of %ci.
    //     ...
    //     %m  = OpCompositeConstruct %matNxM %c0 %c1 %c2 ...
    //
    // - Otherwise, an identity matrix is created and super imposed by m:
    //
    //     %c0 = OpCompositeConstruct %vecM %m[0][0] %m[0][1] %0 %0
    //     %c1 = OpCompositeConstruct %vecM %m[1][0] %m[1][1] %0 %0
    //     %c2 = OpCompositeConstruct %vecM %m[2][0] %m[2][1] %1 %0
    //     %c3 = OpCompositeConstruct %vecM       %0       %0 %0 %1
    //     %m  = OpCompositeConstruct %matNxM %c0 %c1 %c2 %c3

    const TType &type          = node->getType();
    const TType &parameterType = (*node->getSequence())[0]->getAsTyped()->getType();

    SpirvDecorations decorations = mBuilder.getDecorations(type);

    // TODO: handle casting.  http://anglebug.com/4889.

    ASSERT(parameters.size() == 1);

    spirv::IdRefList columnIds;

    SpirvType columnType            = mBuilder.getSpirvType(type, EbsUnspecified);
    columnType.primarySize          = columnType.secondarySize;
    columnType.secondarySize        = 1;
    const spirv::IdRef columnTypeId = mBuilder.getSpirvTypeData(columnType, nullptr).id;

    if (parameterType.getCols() >= type.getCols() && parameterType.getRows() >= type.getRows())
    {
        // If the parameter is a larger matrix than the constructor type, extract the columns
        // directly and potentially swizzle them.
        SpirvType paramColumnType     = mBuilder.getSpirvType(parameterType, EbsUnspecified);
        paramColumnType.secondarySize = 1;
        const spirv::IdRef paramColumnTypeId =
            mBuilder.getSpirvTypeData(paramColumnType, nullptr).id;

        const bool needsSwizzle           = parameterType.getRows() > type.getRows();
        spirv::LiteralIntegerList swizzle = {spirv::LiteralInteger(0), spirv::LiteralInteger(1),
                                             spirv::LiteralInteger(2), spirv::LiteralInteger(3)};
        swizzle.resize(type.getRows());

        for (int columnIndex = 0; columnIndex < type.getCols(); ++columnIndex)
        {
            // Extract the column.
            const spirv::IdRef parameterColumnId = mBuilder.getNewId(decorations);
            spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(), paramColumnTypeId,
                                         parameterColumnId, parameters[0],
                                         {spirv::LiteralInteger(columnIndex)});

            // If the column has too many components, select the appropriate number of components.
            spirv::IdRef constructorColumnId = parameterColumnId;
            if (needsSwizzle)
            {
                constructorColumnId = mBuilder.getNewId(decorations);
                spirv::WriteVectorShuffle(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                          constructorColumnId, parameterColumnId, parameterColumnId,
                                          swizzle);
            }

            columnIds.push_back(constructorColumnId);
        }
    }
    else
    {
        // Otherwise create an identity matrix and fill in the components that can be taken from the
        // given parameter.
        SpirvType paramComponentType     = mBuilder.getSpirvType(parameterType, EbsUnspecified);
        paramComponentType.primarySize   = 1;
        paramComponentType.secondarySize = 1;
        const spirv::IdRef paramComponentTypeId =
            mBuilder.getSpirvTypeData(paramComponentType, nullptr).id;

        for (int columnIndex = 0; columnIndex < type.getCols(); ++columnIndex)
        {
            spirv::IdRefList componentIds;

            for (int componentIndex = 0; componentIndex < type.getRows(); ++componentIndex)
            {
                // Take the component from the constructor parameter if possible.
                spirv::IdRef componentId;
                if (componentIndex < parameterType.getRows())
                {
                    componentId = mBuilder.getNewId(decorations);
                    spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(),
                                                 paramComponentTypeId, componentId, parameters[0],
                                                 {spirv::LiteralInteger(columnIndex),
                                                  spirv::LiteralInteger(componentIndex)});
                }
                else
                {
                    const bool isOnDiagonal = columnIndex == componentIndex;
                    switch (type.getBasicType())
                    {
                        case EbtFloat:
                            componentId = mBuilder.getFloatConstant(isOnDiagonal ? 0.0f : 1.0f);
                            break;
                        case EbtInt:
                            componentId = mBuilder.getIntConstant(isOnDiagonal ? 0 : 1);
                            break;
                        case EbtUInt:
                            componentId = mBuilder.getUintConstant(isOnDiagonal ? 0 : 1);
                            break;
                        case EbtBool:
                            componentId = mBuilder.getBoolConstant(isOnDiagonal);
                            break;
                        default:
                            UNREACHABLE();
                    }
                }

                componentIds.push_back(componentId);
            }

            // Create the column vector.
            columnIds.push_back(mBuilder.getNewId(decorations));
            spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                           columnIds.back(), componentIds);
        }
    }

    const spirv::IdRef result = mBuilder.getNewId(decorations);
    spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                                   columnIds);
    return result;
}

spirv::IdRefList OutputSPIRVTraverser::loadAllParams(TIntermOperator *node)
{
    const size_t parameterCount = node->getChildCount();
    spirv::IdRefList parameters;

    for (size_t paramIndex = 0; paramIndex < parameterCount; ++paramIndex)
    {
        // Take each parameter that is visited and evaluate it as rvalue
        NodeData &param = mNodeData[mNodeData.size() - parameterCount + paramIndex];

        const spirv::IdRef paramValue = accessChainLoad(
            &param,
            mBuilder.getDecorations(node->getChildNode(paramIndex)->getAsTyped()->getType()));

        // TODO: handle mismatching types.  http://anglebug.com/6000

        parameters.push_back(paramValue);
    }

    return parameters;
}

void OutputSPIRVTraverser::extractComponents(TIntermAggregate *node,
                                             size_t componentCount,
                                             const spirv::IdRefList &parameters,
                                             spirv::IdRefList *extractedComponentsOut)
{
    // A helper function that takes the list of parameters passed to a constructor (which may have
    // more components than necessary) and extracts the first componentCount components.
    const TIntermSequence &arguments = *node->getSequence();

    SpirvDecorations decorations = mBuilder.getDecorations(node->getType());

    // TODO: handle casting.  http://anglebug.com/4889.

    ASSERT(arguments.size() == parameters.size());

    for (size_t argumentIndex = 0;
         argumentIndex < arguments.size() && extractedComponentsOut->size() < componentCount;
         ++argumentIndex)
    {
        const TType &argumentType      = arguments[argumentIndex]->getAsTyped()->getType();
        const spirv::IdRef parameterId = parameters[argumentIndex];

        if (argumentType.isScalar())
        {
            // For scalar parameters, there's nothing to do.
            extractedComponentsOut->push_back(parameterId);
            continue;
        }
        if (argumentType.isVector())
        {
            SpirvType componentType   = mBuilder.getSpirvType(argumentType, EbsUnspecified);
            componentType.primarySize = 1;
            const spirv::IdRef componentTypeId =
                mBuilder.getSpirvTypeData(componentType, nullptr).id;

            // For vector parameters, take components out of the vector one by one.
            for (int componentIndex = 0; componentIndex < argumentType.getNominalSize() &&
                                         extractedComponentsOut->size() < componentCount;
                 ++componentIndex)
            {
                const spirv::IdRef componentId = mBuilder.getNewId(decorations);
                spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(),
                                             componentTypeId, componentId, parameterId,
                                             {spirv::LiteralInteger(componentIndex)});

                extractedComponentsOut->push_back(componentId);
            }
            continue;
        }

        ASSERT(argumentType.isMatrix());

        SpirvType componentType            = mBuilder.getSpirvType(argumentType, EbsUnspecified);
        componentType.primarySize          = 1;
        componentType.secondarySize        = 1;
        const spirv::IdRef componentTypeId = mBuilder.getSpirvTypeData(componentType, nullptr).id;

        // For matrix parameters, take components out of the matrix one by one in column-major
        // order.
        for (int columnIndex = 0; columnIndex < argumentType.getCols() &&
                                  extractedComponentsOut->size() < componentCount;
             ++columnIndex)
        {
            for (int componentIndex = 0; componentIndex < argumentType.getRows() &&
                                         extractedComponentsOut->size() < componentCount;
                 ++componentIndex)
            {
                const spirv::IdRef componentId = mBuilder.getNewId(decorations);
                spirv::WriteCompositeExtract(
                    mBuilder.getSpirvCurrentFunctionBlock(), componentTypeId, componentId,
                    parameterId,
                    {spirv::LiteralInteger(columnIndex), spirv::LiteralInteger(componentIndex)});

                extractedComponentsOut->push_back(componentId);
            }
        }
    }
}

void OutputSPIRVTraverser::startShortCircuit(TIntermBinary *node)
{
    // Emulate && and || as such:
    //
    //   || => if (!left) result = right
    //   && => if ( left) result = right
    //
    // When this function is called, |left| has already been visited, so it creates the appropriate
    // |if| construct in preparation for visiting |right|.

    // Load |left| and replace the access chain with an rvalue that's the result.
    const spirv::IdRef typeId = getAccessChainTypeId(&mNodeData.back());
    const spirv::IdRef left =
        accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(node->getLeft()->getType()));
    nodeDataInitRValue(&mNodeData.back(), left, typeId);

    // Keep the id of the block |left| was evaluated in.
    mNodeData.back().idList.push_back(mBuilder.getSpirvCurrentFunctionBlockId());

    // Two blocks necessary, one for the |if| block, and one for the merge block.
    mBuilder.startConditional(2, false, false);

    // Generate the branch instructions.
    const SpirvConditional *conditional = mBuilder.getCurrentConditional();

    const spirv::IdRef mergeBlock = conditional->blockIds.back();
    const spirv::IdRef ifBlock    = conditional->blockIds.front();
    const spirv::IdRef trueBlock  = node->getOp() == EOpLogicalAnd ? ifBlock : mergeBlock;
    const spirv::IdRef falseBlock = node->getOp() == EOpLogicalOr ? ifBlock : mergeBlock;

    // Note that no logical not is necessary.  For ||, the branch will target the merge block in the
    // true case.
    mBuilder.writeBranchConditional(left, trueBlock, falseBlock, mergeBlock);
}

spirv::IdRef OutputSPIRVTraverser::endShortCircuit(TIntermBinary *node, spirv::IdRef *typeId)
{
    // Load the right hand side.
    const spirv::IdRef right =
        accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(node->getRight()->getType()));
    mNodeData.pop_back();

    // Get the id of the block |right| is evaluated in.
    const spirv::IdRef rightBlockId = mBuilder.getSpirvCurrentFunctionBlockId();

    // And the cached id of the block |left| is evaluated in.
    ASSERT(mNodeData.back().idList.size() == 1);
    const spirv::IdRef leftBlockId = mNodeData.back().idList[0].id;
    mNodeData.back().idList.clear();

    // Move on to the merge block.
    mBuilder.writeBranchConditionalBlockEnd();

    // Pop from the conditional stack.
    mBuilder.endConditional();

    // Get the previously loaded result of the left hand side.
    *typeId                 = getAccessChainTypeId(&mNodeData.back());
    const spirv::IdRef left = mNodeData.back().baseId;

    // Create an OpPhi instruction that selects either the |left| or |right| based on which block
    // was traversed.
    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));

    spirv::WritePhi(
        mBuilder.getSpirvCurrentFunctionBlock(), *typeId, result,
        {spirv::PairIdRefIdRef{left, leftBlockId}, spirv::PairIdRefIdRef{right, rightBlockId}});

    return result;
}

spirv::IdRef OutputSPIRVTraverser::createFunctionCall(TIntermAggregate *node,
                                                      spirv::IdRef resultTypeId)
{
    const TFunction *function = node->getFunction();
    ASSERT(function);

    ASSERT(mFunctionIdMap.count(function) > 0);
    const spirv::IdRef functionId = mFunctionIdMap[function].functionId;

    // Get the list of parameters passed to the function.  The function parameters can only be
    // memory variables, or if the function argument is |const|, an rvalue.
    //
    // For in variables:
    //
    // - If the parameter is const, pass it directly as rvalue, otherwise
    // - If the parameter is an unindexed lvalue, pass it directly, otherwise
    // - Write it to a temp variable first and pass that.
    //
    // For out variables:
    //
    // - If the parameter is an unindexed lvalue, pass it directly, otherwise
    // - Pass a temporary variable.  After the function call, copy that variable to the parameter.
    //
    // For inout variables:
    //
    // - If the parameter is an unindexed lvalue, pass it directly, otherwise
    // - Write the parameter to a temp variable and pass that.  After the function call, copy that
    //   variable back to the parameter.
    //
    // - For opaque uniforms, pass it directly as lvalue,
    //
    const size_t parameterCount = node->getChildCount();
    spirv::IdRefList parameters;
    spirv::IdRefList tempVarIds(parameterCount);
    spirv::IdRefList tempVarTypeIds(parameterCount);

    for (size_t paramIndex = 0; paramIndex < parameterCount; ++paramIndex)
    {
        const TType &paramType           = function->getParam(paramIndex)->getType();
        const TQualifier &paramQualifier = paramType.getQualifier();
        NodeData &param = mNodeData[mNodeData.size() - parameterCount + paramIndex];

        spirv::IdRef paramValue;
        SpirvDecorations decorations = mBuilder.getDecorations(paramType);

        if (IsOpaqueType(paramType.getBasicType()) || paramQualifier == EvqConst)
        {
            // The following parameters are passed as rvalue:
            //
            // - Opaque uniforms,
            // - const parameters,
            paramValue = accessChainLoad(&param, decorations);
        }
        else if (IsAccessChainUnindexedLValue(param) &&
                 (mCompileOptions & SH_GENERATE_SPIRV_WORKAROUNDS) == 0)
        {
            // The following parameters are passed directly:
            //
            // - unindexed lvalues.
            //
            // This optimization is not applied on buggy drivers.  http://anglebug.com/6110.
            paramValue = param.baseId;
        }
        else
        {
            ASSERT(paramQualifier == EvqIn || paramQualifier == EvqOut ||
                   paramQualifier == EvqInOut);

            // Need to create a temp variable and pass that.
            tempVarTypeIds[paramIndex] = mBuilder.getTypeData(paramType, EbsUnspecified).id;
            tempVarIds[paramIndex] =
                mBuilder.declareVariable(tempVarTypeIds[paramIndex], spv::StorageClassFunction,
                                         decorations, nullptr, "param");

            // If it's an in or inout parameter, the temp variable needs to be initialized with the
            // value of the parameter first.
            //
            // TODO: handle mismatching types.  http://anglebug.com/6000
            if (paramQualifier == EvqIn || paramQualifier == EvqInOut)
            {
                paramValue = accessChainLoad(&param, decorations);
                spirv::WriteStore(mBuilder.getSpirvCurrentFunctionBlock(), tempVarIds[paramIndex],
                                  paramValue, nullptr);
            }

            paramValue = tempVarIds[paramIndex];
        }

        parameters.push_back(paramValue);
    }

    // Make the actual function call.
    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));
    spirv::WriteFunctionCall(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result,
                             functionId, parameters);

    // Copy from the out and inout temp variables back to the original parameters.
    for (size_t paramIndex = 0; paramIndex < parameterCount; ++paramIndex)
    {
        if (!tempVarIds[paramIndex].valid())
        {
            continue;
        }

        const TType &paramType           = function->getParam(paramIndex)->getType();
        const TQualifier &paramQualifier = paramType.getQualifier();
        NodeData &param = mNodeData[mNodeData.size() - parameterCount + paramIndex];

        if (paramQualifier == EvqIn)
        {
            continue;
        }

        // Copy from the temp variable to the parameter.
        //
        // TODO: handle mismatching types.  http://anglebug.com/6000
        NodeData tempVarData;
        nodeDataInitLValue(&tempVarData, tempVarIds[paramIndex], tempVarTypeIds[paramIndex],
                           spv::StorageClassFunction, EbsUnspecified);
        const spirv::IdRef tempVarValue =
            accessChainLoad(&tempVarData, mBuilder.getDecorations(paramType));
        accessChainStore(&param, tempVarValue);
    }

    return result;
}

bool IsShortCircuitNeeded(TIntermOperator *node)
{
    TOperator op = node->getOp();

    // Short circuit is only necessary for && and ||.
    if (op != EOpLogicalAnd && op != EOpLogicalOr)
    {
        return false;
    }

    ASSERT(node->getChildCount() == 2);

    // If the right hand side does not have side effects, short-circuiting is unnecessary.
    // TODO: experiment with the performance of OpLogicalAnd/Or vs short-circuit based on the
    // complexity of the right hand side expression.  We could potentially only allow
    // OpLogicalAnd/Or if the right hand side is a constant or an access chain and have more complex
    // expressions be placed inside an if block.  http://anglebug.com/4889
    return node->getChildNode(1)->getAsTyped()->hasSideEffects();
}

using WriteUnaryOp      = void (*)(spirv::Blob *blob,
                              spirv::IdResultType idResultType,
                              spirv::IdResult idResult,
                              spirv::IdRef operand);
using WriteBinaryOp     = void (*)(spirv::Blob *blob,
                               spirv::IdResultType idResultType,
                               spirv::IdResult idResult,
                               spirv::IdRef operand1,
                               spirv::IdRef operand2);
using WriteTernaryOp    = void (*)(spirv::Blob *blob,
                                spirv::IdResultType idResultType,
                                spirv::IdResult idResult,
                                spirv::IdRef operand1,
                                spirv::IdRef operand2,
                                spirv::IdRef operand3);
using WriteQuaternaryOp = void (*)(spirv::Blob *blob,
                                   spirv::IdResultType idResultType,
                                   spirv::IdResult idResult,
                                   spirv::IdRef operand1,
                                   spirv::IdRef operand2,
                                   spirv::IdRef operand3,
                                   spirv::IdRef operand4);
using WriteAtomicOp     = void (*)(spirv::Blob *blob,
                               spirv::IdResultType idResultType,
                               spirv::IdResult idResult,
                               spirv::IdRef pointer,
                               spirv::IdScope scope,
                               spirv::IdMemorySemantics semantics,
                               spirv::IdRef value);

spirv::IdRef OutputSPIRVTraverser::visitOperator(TIntermOperator *node, spirv::IdRef resultTypeId)
{
    // Handle special groups.
    const TOperator op = node->getOp();
    if (op == EOpPostIncrement || op == EOpPreIncrement || op == EOpPostDecrement ||
        op == EOpPreDecrement)
    {
        return createIncrementDecrement(node, resultTypeId);
    }
    if (BuiltInGroup::IsAtomicMemory(op) || BuiltInGroup::IsImageAtomic(op))
    {
        return createAtomicBuiltIn(node, resultTypeId);
    }
    if (BuiltInGroup::IsTexture(op))
    {
        return createTextureBuiltIn(node, resultTypeId);
    }
    if (BuiltInGroup::IsImage(op))
    {
        return createImageBuiltIn(node, resultTypeId);
    }

    const size_t childCount  = node->getChildCount();
    TIntermTyped *firstChild = node->getChildNode(0)->getAsTyped();

    const TType &firstOperandType = firstChild->getType();
    const TBasicType basicType    = firstOperandType.getBasicType();
    const bool isFloat            = basicType == EbtFloat || basicType == EbtDouble;
    const bool isUnsigned         = basicType == EbtUInt;
    const bool isBool             = basicType == EbtBool;
    // Whether the operation needs to be applied column by column.
    TIntermBinary *asBinary = node->getAsBinaryNode();
    bool operateOnColumns   = asBinary && (asBinary->getLeft()->getType().isMatrix() ||
                                         asBinary->getRight()->getType().isMatrix());
    // Whether the operands need to be swapped in the (binary) instruction
    bool binarySwapOperands = false;
    // Whether the scalar operand needs to be extended to match the other operand which is a vector
    // (in a binary operation).
    bool binaryExtendScalarToVector = true;

    WriteUnaryOp writeUnaryOp           = nullptr;
    WriteBinaryOp writeBinaryOp         = nullptr;
    WriteTernaryOp writeTernaryOp       = nullptr;
    WriteQuaternaryOp writeQuaternaryOp = nullptr;

    // Some operators are implemented with an extended instruction.
    spv::GLSLstd450 extendedInst = spv::GLSLstd450Bad;

    switch (op)
    {
        case EOpNegative:
            if (isFloat)
                writeUnaryOp = spirv::WriteFNegate;
            else
                writeUnaryOp = spirv::WriteSNegate;
            break;
        case EOpPositive:
            // This is a noop.
            return accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(firstOperandType));

        case EOpLogicalNot:
        case EOpNotComponentWise:
            writeUnaryOp = spirv::WriteLogicalNot;
            break;
        case EOpBitwiseNot:
            writeUnaryOp = spirv::WriteNot;
            break;

        case EOpAdd:
        case EOpAddAssign:
            if (isFloat)
                writeBinaryOp = spirv::WriteFAdd;
            else
                writeBinaryOp = spirv::WriteIAdd;
            break;
        case EOpSub:
        case EOpSubAssign:
            if (isFloat)
                writeBinaryOp = spirv::WriteFSub;
            else
                writeBinaryOp = spirv::WriteISub;
            break;
        case EOpMul:
        case EOpMulAssign:
        case EOpMatrixCompMult:
            if (isFloat)
                writeBinaryOp = spirv::WriteFMul;
            else
                writeBinaryOp = spirv::WriteIMul;
            break;
        case EOpDiv:
        case EOpDivAssign:
            if (isFloat)
                writeBinaryOp = spirv::WriteFDiv;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteUDiv;
            else
                writeBinaryOp = spirv::WriteSDiv;
            break;
        case EOpIMod:
        case EOpIModAssign:
            if (isFloat)
                writeBinaryOp = spirv::WriteFMod;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteUMod;
            else
                writeBinaryOp = spirv::WriteSMod;
            break;

        case EOpEqual:
        case EOpEqualComponentWise:
            // TODO: handle vector, matrix and other complex comparisons.  EOpEqual must use OpAll
            // to reduce to a bool.  http://anglebug.com/4889.
            if (isFloat)
                writeBinaryOp = spirv::WriteFOrdEqual;
            else if (isBool)
                writeBinaryOp = spirv::WriteLogicalEqual;
            else
                writeBinaryOp = spirv::WriteIEqual;
            break;
        case EOpNotEqual:
        case EOpNotEqualComponentWise:
            // TODO: handle vector, matrix and other complex comparisons.  EOpNotEqual must use
            // OpAny to reduce to a bool.  http://anglebug.com/4889.
            if (isFloat)
                writeBinaryOp = spirv::WriteFUnordNotEqual;
            else if (isBool)
                writeBinaryOp = spirv::WriteLogicalNotEqual;
            else
                writeBinaryOp = spirv::WriteINotEqual;
            break;
        case EOpLessThan:
        case EOpLessThanComponentWise:
            if (isFloat)
                writeBinaryOp = spirv::WriteFOrdLessThan;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteULessThan;
            else
                writeBinaryOp = spirv::WriteSLessThan;
            break;
        case EOpGreaterThan:
        case EOpGreaterThanComponentWise:
            if (isFloat)
                writeBinaryOp = spirv::WriteFOrdGreaterThan;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteUGreaterThan;
            else
                writeBinaryOp = spirv::WriteSGreaterThan;
            break;
        case EOpLessThanEqual:
        case EOpLessThanEqualComponentWise:
            if (isFloat)
                writeBinaryOp = spirv::WriteFOrdLessThanEqual;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteULessThanEqual;
            else
                writeBinaryOp = spirv::WriteSLessThanEqual;
            break;
        case EOpGreaterThanEqual:
        case EOpGreaterThanEqualComponentWise:
            if (isFloat)
                writeBinaryOp = spirv::WriteFOrdGreaterThanEqual;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteUGreaterThanEqual;
            else
                writeBinaryOp = spirv::WriteSGreaterThanEqual;
            break;

        case EOpVectorTimesScalar:
        case EOpVectorTimesScalarAssign:
            if (isFloat)
            {
                writeBinaryOp      = spirv::WriteVectorTimesScalar;
                binarySwapOperands = node->getChildNode(1)->getAsTyped()->getType().isVector();
                binaryExtendScalarToVector = false;
            }
            else
                writeBinaryOp = spirv::WriteIMul;
            break;
        case EOpVectorTimesMatrix:
        case EOpVectorTimesMatrixAssign:
            writeBinaryOp    = spirv::WriteVectorTimesMatrix;
            operateOnColumns = false;
            break;
        case EOpMatrixTimesVector:
            writeBinaryOp    = spirv::WriteMatrixTimesVector;
            operateOnColumns = false;
            break;
        case EOpMatrixTimesScalar:
        case EOpMatrixTimesScalarAssign:
            writeBinaryOp      = spirv::WriteMatrixTimesScalar;
            binarySwapOperands = asBinary->getRight()->getType().isMatrix();
            operateOnColumns   = false;
            break;
        case EOpMatrixTimesMatrix:
        case EOpMatrixTimesMatrixAssign:
            writeBinaryOp    = spirv::WriteMatrixTimesMatrix;
            operateOnColumns = false;
            break;

        case EOpLogicalOr:
            ASSERT(!IsShortCircuitNeeded(node));
            binaryExtendScalarToVector = false;
            writeBinaryOp              = spirv::WriteLogicalOr;
            break;
        case EOpLogicalXor:
            binaryExtendScalarToVector = false;
            writeBinaryOp              = spirv::WriteLogicalNotEqual;
            break;
        case EOpLogicalAnd:
            ASSERT(!IsShortCircuitNeeded(node));
            binaryExtendScalarToVector = false;
            writeBinaryOp              = spirv::WriteLogicalAnd;
            break;

        case EOpBitShiftLeft:
        case EOpBitShiftLeftAssign:
            writeBinaryOp = spirv::WriteShiftLeftLogical;
            break;
        case EOpBitShiftRight:
        case EOpBitShiftRightAssign:
            if (isUnsigned)
                writeBinaryOp = spirv::WriteShiftRightLogical;
            else
                writeBinaryOp = spirv::WriteShiftRightArithmetic;
            break;
        case EOpBitwiseAnd:
        case EOpBitwiseAndAssign:
            writeBinaryOp = spirv::WriteBitwiseAnd;
            break;
        case EOpBitwiseXor:
        case EOpBitwiseXorAssign:
            writeBinaryOp = spirv::WriteBitwiseXor;
            break;
        case EOpBitwiseOr:
        case EOpBitwiseOrAssign:
            writeBinaryOp = spirv::WriteBitwiseOr;
            break;

        case EOpRadians:
            extendedInst = spv::GLSLstd450Radians;
            break;
        case EOpDegrees:
            extendedInst = spv::GLSLstd450Degrees;
            break;
        case EOpSin:
            extendedInst = spv::GLSLstd450Sin;
            break;
        case EOpCos:
            extendedInst = spv::GLSLstd450Cos;
            break;
        case EOpTan:
            extendedInst = spv::GLSLstd450Tan;
            break;
        case EOpAsin:
            extendedInst = spv::GLSLstd450Asin;
            break;
        case EOpAcos:
            extendedInst = spv::GLSLstd450Acos;
            break;
        case EOpAtan:
            extendedInst = spv::GLSLstd450Atan;
            break;
        case EOpSinh:
            extendedInst = spv::GLSLstd450Sinh;
            break;
        case EOpCosh:
            extendedInst = spv::GLSLstd450Cosh;
            break;
        case EOpTanh:
            extendedInst = spv::GLSLstd450Tanh;
            break;
        case EOpAsinh:
            extendedInst = spv::GLSLstd450Asinh;
            break;
        case EOpAcosh:
            extendedInst = spv::GLSLstd450Acosh;
            break;
        case EOpAtanh:
            extendedInst = spv::GLSLstd450Atanh;
            break;

        case EOpPow:
            extendedInst = spv::GLSLstd450Pow;
            break;
        case EOpExp:
            extendedInst = spv::GLSLstd450Exp;
            break;
        case EOpLog:
            extendedInst = spv::GLSLstd450Log;
            break;
        case EOpExp2:
            extendedInst = spv::GLSLstd450Exp2;
            break;
        case EOpLog2:
            extendedInst = spv::GLSLstd450Log2;
            break;
        case EOpSqrt:
            extendedInst = spv::GLSLstd450Sqrt;
            break;
        case EOpInversesqrt:
            extendedInst = spv::GLSLstd450InverseSqrt;
            break;

        case EOpAbs:
            if (isFloat)
                extendedInst = spv::GLSLstd450FAbs;
            else
                extendedInst = spv::GLSLstd450SAbs;
            break;
        case EOpSign:
            if (isFloat)
                extendedInst = spv::GLSLstd450FSign;
            else
                extendedInst = spv::GLSLstd450SSign;
            break;
        case EOpFloor:
            extendedInst = spv::GLSLstd450Floor;
            break;
        case EOpTrunc:
            extendedInst = spv::GLSLstd450Trunc;
            break;
        case EOpRound:
            extendedInst = spv::GLSLstd450Round;
            break;
        case EOpRoundEven:
            extendedInst = spv::GLSLstd450RoundEven;
            break;
        case EOpCeil:
            extendedInst = spv::GLSLstd450Ceil;
            break;
        case EOpFract:
            extendedInst = spv::GLSLstd450Fract;
            break;
        case EOpMod:
            if (isFloat)
                writeBinaryOp = spirv::WriteFMod;
            else if (isUnsigned)
                writeBinaryOp = spirv::WriteUMod;
            else
                writeBinaryOp = spirv::WriteSMod;
            break;
        case EOpMin:
            if (isFloat)
                extendedInst = spv::GLSLstd450FMin;
            else if (isUnsigned)
                extendedInst = spv::GLSLstd450UMin;
            else
                extendedInst = spv::GLSLstd450SMin;
            break;
        case EOpMax:
            if (isFloat)
                extendedInst = spv::GLSLstd450FMax;
            else if (isUnsigned)
                extendedInst = spv::GLSLstd450UMax;
            else
                extendedInst = spv::GLSLstd450SMax;
            break;
        case EOpClamp:
            if (isFloat)
                extendedInst = spv::GLSLstd450FClamp;
            else if (isUnsigned)
                extendedInst = spv::GLSLstd450UClamp;
            else
                extendedInst = spv::GLSLstd450SClamp;
            break;
        case EOpMix:
            if (node->getChildNode(childCount - 1)->getAsTyped()->getType().getBasicType() ==
                EbtBool)
            {
                writeTernaryOp = spirv::WriteSelect;
            }
            else
            {
                ASSERT(isFloat);
                extendedInst = spv::GLSLstd450FMix;
            }
            break;
        case EOpStep:
            extendedInst = spv::GLSLstd450Step;
            break;
        case EOpSmoothstep:
            extendedInst = spv::GLSLstd450SmoothStep;
            break;
        case EOpModf:
            // TODO: modf has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;
        case EOpIsnan:
            writeUnaryOp = spirv::WriteIsNan;
            break;
        case EOpIsinf:
            writeUnaryOp = spirv::WriteIsInf;
            break;
        case EOpFloatBitsToInt:
        case EOpFloatBitsToUint:
        case EOpIntBitsToFloat:
        case EOpUintBitsToFloat:
            writeUnaryOp = spirv::WriteBitcast;
            break;
        case EOpFma:
            extendedInst = spv::GLSLstd450Fma;
            break;
        case EOpFrexp:
            // TODO: frexp has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;
        case EOpLdexp:
            extendedInst = spv::GLSLstd450Ldexp;
            break;
        case EOpPackSnorm2x16:
            extendedInst = spv::GLSLstd450PackSnorm2x16;
            break;
        case EOpPackUnorm2x16:
            extendedInst = spv::GLSLstd450PackUnorm2x16;
            break;
        case EOpPackHalf2x16:
            extendedInst = spv::GLSLstd450PackHalf2x16;
            break;
        case EOpUnpackSnorm2x16:
            extendedInst = spv::GLSLstd450UnpackSnorm2x16;
            break;
        case EOpUnpackUnorm2x16:
            extendedInst = spv::GLSLstd450UnpackUnorm2x16;
            break;
        case EOpUnpackHalf2x16:
            extendedInst = spv::GLSLstd450UnpackHalf2x16;
            break;
        case EOpPackUnorm4x8:
            extendedInst = spv::GLSLstd450PackUnorm4x8;
            break;
        case EOpPackSnorm4x8:
            extendedInst = spv::GLSLstd450PackSnorm4x8;
            break;
        case EOpUnpackUnorm4x8:
            extendedInst = spv::GLSLstd450UnpackUnorm4x8;
            break;
        case EOpUnpackSnorm4x8:
            extendedInst = spv::GLSLstd450UnpackSnorm4x8;
            break;
        case EOpPackDouble2x32:
        case EOpUnpackDouble2x32:
            // TODO: support desktop GLSL.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        case EOpLength:
            extendedInst = spv::GLSLstd450Length;
            break;
        case EOpDistance:
            extendedInst = spv::GLSLstd450Distance;
            break;
        case EOpDot:
            // Use normal multiplication for scalars.
            if (firstOperandType.isScalar())
            {
                if (isFloat)
                    writeBinaryOp = spirv::WriteFMul;
                else
                    writeBinaryOp = spirv::WriteIMul;
            }
            else
            {
                writeBinaryOp = spirv::WriteDot;
            }
            break;
        case EOpCross:
            extendedInst = spv::GLSLstd450Cross;
            break;
        case EOpNormalize:
            extendedInst = spv::GLSLstd450Normalize;
            break;
        case EOpFaceforward:
            extendedInst = spv::GLSLstd450FaceForward;
            break;
        case EOpReflect:
            extendedInst = spv::GLSLstd450Reflect;
            break;
        case EOpRefract:
            extendedInst = spv::GLSLstd450Refract;
            break;

        case EOpFtransform:
            // TODO: support desktop GLSL.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        case EOpOuterProduct:
            writeBinaryOp = spirv::WriteOuterProduct;
            break;
        case EOpTranspose:
            writeUnaryOp = spirv::WriteTranspose;
            break;
        case EOpDeterminant:
            extendedInst = spv::GLSLstd450Determinant;
            break;
        case EOpInverse:
            extendedInst = spv::GLSLstd450MatrixInverse;
            break;

        case EOpAny:
            writeUnaryOp = spirv::WriteAny;
            break;
        case EOpAll:
            writeUnaryOp = spirv::WriteAll;
            break;

        case EOpBitfieldExtract:
            if (isUnsigned)
                writeTernaryOp = spirv::WriteBitFieldUExtract;
            else
                writeTernaryOp = spirv::WriteBitFieldSExtract;
            break;
        case EOpBitfieldInsert:
            writeQuaternaryOp = spirv::WriteBitFieldInsert;
            break;
        case EOpBitfieldReverse:
            writeUnaryOp = spirv::WriteBitReverse;
            break;
        case EOpBitCount:
            writeUnaryOp = spirv::WriteBitCount;
            break;
        case EOpFindLSB:
            extendedInst = spv::GLSLstd450FindILsb;
            break;
        case EOpFindMSB:
            if (isUnsigned)
                extendedInst = spv::GLSLstd450FindUMsb;
            else
                extendedInst = spv::GLSLstd450FindSMsb;
            break;
        case EOpUaddCarry:
            // TODO: uaddCarry has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;
        case EOpUsubBorrow:
            // TODO: usubBorrow has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;
        case EOpUmulExtended:
            // TODO: umulExtended has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;
        case EOpImulExtended:
            // TODO: imulExtended has an out parameter.  http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;

        case EOpRgb_2_yuv:
        case EOpYuv_2_rgb:
            // TODO: There doesn't seem to be an equivalent in SPIR-V, and should likley be emulated
            // as an AST transformation.  Not supported by the Vulkan at the moment.
            // http://anglebug.com/4889.
            UNIMPLEMENTED();
            break;

        case EOpDFdx:
            writeUnaryOp = spirv::WriteDPdx;
            break;
        case EOpDFdy:
            writeUnaryOp = spirv::WriteDPdy;
            break;
        case EOpFwidth:
            writeUnaryOp = spirv::WriteFwidth;
            break;
        case EOpDFdxFine:
            writeUnaryOp = spirv::WriteDPdxFine;
            break;
        case EOpDFdyFine:
            writeUnaryOp = spirv::WriteDPdyFine;
            break;
        case EOpDFdxCoarse:
            writeUnaryOp = spirv::WriteDPdxCoarse;
            break;
        case EOpDFdyCoarse:
            writeUnaryOp = spirv::WriteDPdyCoarse;
            break;
        case EOpFwidthFine:
            writeUnaryOp = spirv::WriteFwidthFine;
            break;
        case EOpFwidthCoarse:
            writeUnaryOp = spirv::WriteFwidthCoarse;
            break;

            // TODO: for the EOpInterpolate* built-ins, must convert interpolateX(vec.yz) to
            // interpolate(vec).yz.  This can either be done apriori by an AST transformation, or
            // simply by taking the base id only when generating the instruction and keeping the
            // indices/swizzle intact.  http://anglebug.com/4889.
        case EOpInterpolateAtCentroid:
            extendedInst = spv::GLSLstd450InterpolateAtCentroid;
            break;
        case EOpInterpolateAtSample:
            extendedInst = spv::GLSLstd450InterpolateAtSample;
            break;
        case EOpInterpolateAtOffset:
            extendedInst = spv::GLSLstd450InterpolateAtOffset;
            break;

        case EOpNoise1:
        case EOpNoise2:
        case EOpNoise3:
        case EOpNoise4:
            // TODO: support desktop GLSL.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        case EOpSubpassLoad:
            // TODO: support framebuffer fetch.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        case EOpAnyInvocation:
        case EOpAllInvocations:
        case EOpAllInvocationsEqual:
            // TODO: support desktop GLSL.  http://anglebug.com/4889
            break;

        default:
            UNREACHABLE();
    }

    const SpirvDecorations decorations = mBuilder.getDecorations(node->getType());
    spirv::IdRef result                = mBuilder.getNewId(decorations);

    // Load the parameters.
    spirv::IdRefList parameters = loadAllParams(node);

    if (operateOnColumns)
    {
        // If negating a matrix or multiplying them, do that column by column.
        spirv::IdRefList columnIds;

        const SpirvDecorations operandDecorations = mBuilder.getDecorations(firstOperandType);

        SpirvType columnType            = mBuilder.getSpirvType(firstOperandType, EbsUnspecified);
        columnType.primarySize          = columnType.secondarySize;
        columnType.secondarySize        = 1;
        const spirv::IdRef columnTypeId = mBuilder.getSpirvTypeData(columnType, nullptr).id;

        if (binarySwapOperands)
        {
            std::swap(parameters[0], parameters[1]);
        }

        // Extract and apply the operator to each column.
        for (int columnIndex = 0; columnIndex < firstOperandType.getCols(); ++columnIndex)
        {
            const spirv::IdRef columnIdA = mBuilder.getNewId(operandDecorations);
            spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                         columnIdA, parameters[0],
                                         {spirv::LiteralInteger(columnIndex)});

            columnIds.push_back(mBuilder.getNewId(decorations));

            if (writeUnaryOp)
            {
                writeUnaryOp(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                             columnIds.back(), columnIdA);
            }
            else
            {
                ASSERT(writeBinaryOp);

                const spirv::IdRef columnIdB = mBuilder.getNewId(operandDecorations);
                spirv::WriteCompositeExtract(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                                             columnIdB, parameters[1],
                                             {spirv::LiteralInteger(columnIndex)});

                writeBinaryOp(mBuilder.getSpirvCurrentFunctionBlock(), columnTypeId,
                              columnIds.back(), columnIdA, columnIdB);
            }
        }

        // Construct the result.
        spirv::WriteCompositeConstruct(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId,
                                       result, columnIds);
    }
    else if (writeUnaryOp)
    {
        ASSERT(parameters.size() == 1);
        writeUnaryOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result, parameters[0]);
    }
    else if (writeBinaryOp)
    {
        ASSERT(parameters.size() == 2);

        // For vector<op>scalar operations that require it, turn the scalar into a vector of the
        // same size.
        if (binaryExtendScalarToVector)
        {
            const TType &leftType  = node->getChildNode(0)->getAsTyped()->getType();
            const TType &rightType = node->getChildNode(1)->getAsTyped()->getType();

            if (leftType.isScalar() && rightType.isVector())
            {
                parameters[0] =
                    createConstructorVectorFromScalar(rightType, resultTypeId, {{parameters[0]}});
            }
            else if (rightType.isScalar() && leftType.isVector())
            {
                parameters[1] =
                    createConstructorVectorFromScalar(leftType, resultTypeId, {{parameters[1]}});
            }
        }

        if (binarySwapOperands)
        {
            std::swap(parameters[0], parameters[1]);
        }

        // Write the operation that combines the left and right values.
        writeBinaryOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result, parameters[0],
                      parameters[1]);
    }
    else if (writeTernaryOp)
    {
        ASSERT(parameters.size() == 3);

        // mix(a, b, bool) is the same as bool ? b : a;
        if (op == EOpMix)
        {
            std::swap(parameters[0], parameters[2]);
        }

        writeTernaryOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result, parameters[0],
                       parameters[1], parameters[2]);
    }
    else if (writeQuaternaryOp)
    {
        ASSERT(parameters.size() == 4);

        writeQuaternaryOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result,
                          parameters[0], parameters[1], parameters[2], parameters[3]);
    }
    else
    {
        // It's an extended instruction.
        ASSERT(extendedInst != spv::GLSLstd450Bad);

        spirv::WriteExtInst(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result,
                            mBuilder.getExtInstImportIdStd(),
                            spirv::LiteralExtInstInteger(extendedInst), parameters);
    }

    // If it's an assignment, store the calculated value.
    if (IsAssignment(node->getOp()))
    {
        ASSERT(mNodeData.size() >= 2);
        ASSERT(parameters.size() == 2);
        accessChainStore(&mNodeData[mNodeData.size() - 2], result);
    }

    return result;
}

spirv::IdRef OutputSPIRVTraverser::createIncrementDecrement(TIntermOperator *node,
                                                            spirv::IdRef resultTypeId)
{
    TIntermTyped *operand = node->getChildNode(0)->getAsTyped();

    const TType &operandType   = operand->getType();
    const TBasicType basicType = operandType.getBasicType();
    const bool isFloat         = basicType == EbtFloat || basicType == EbtDouble;

    // ++ and -- are implemented with binary SPIR-V ops.
    WriteBinaryOp writeBinaryOp = nullptr;

    switch (node->getOp())
    {
        case EOpPostIncrement:
        case EOpPreIncrement:
            if (isFloat)
                writeBinaryOp = spirv::WriteFAdd;
            else
                writeBinaryOp = spirv::WriteIAdd;
            break;
        case EOpPostDecrement:
        case EOpPreDecrement:
            if (isFloat)
                writeBinaryOp = spirv::WriteFSub;
            else
                writeBinaryOp = spirv::WriteISub;
            break;
        default:
            UNREACHABLE();
    }

    // Load the operand.
    spirv::IdRef value =
        accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(operand->getType()));

    spirv::IdRef result    = mBuilder.getNewId(mBuilder.getDecorations(operandType));
    const spirv::IdRef one = isFloat ? mBuilder.getFloatConstant(1) : mBuilder.getIntConstant(1);

    writeBinaryOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result, value, one);

    // The result is always written back.
    accessChainStore(&mNodeData.back(), result);

    // Initialize the access chain with either the result or the value based on whether pre or
    // post increment/decrement was used.  The result is always an rvalue.
    if (node->getOp() == EOpPostIncrement || node->getOp() == EOpPostDecrement)
    {
        result = value;
    }

    return result;
}

spirv::IdRef OutputSPIRVTraverser::createAtomicBuiltIn(TIntermOperator *node,
                                                       spirv::IdRef resultTypeId)
{
    // Most atomic instructions are in the form of:
    //
    //     %result = OpAtomicX %pointer Scope MemorySemantics %value
    //
    // OpAtomicCompareSwap is exceptionally different (note that compare and value are in different
    // order than in GLSL):
    //
    //     %result = OpAtomicCompareExchange %pointer
    //                                       Scope MemorySemantics MemorySemantics
    //                                       %value %comparator
    //
    // In all cases, the first parameter is the pointer, and the rest are rvalues.
    const size_t parameterCount = node->getChildCount();
    spirv::IdRef pointerId;
    spirv::IdRefList parameters;

    ASSERT(parameterCount >= 2);

    pointerId = accessChainCollapse(&mNodeData[mNodeData.size() - parameterCount]);
    for (size_t paramIndex = 1; paramIndex < parameterCount; ++paramIndex)
    {
        NodeData &param = mNodeData[mNodeData.size() - parameterCount + paramIndex];
        parameters.push_back(accessChainLoad(
            &param,
            mBuilder.getDecorations(node->getChildNode(paramIndex)->getAsTyped()->getType())));
    }

    // The scope of the operation is always Device as we don't enable the Vulkan memory model
    // extension.
    const spirv::IdScope scopeId = mBuilder.getUintConstant(spv::ScopeDevice);

    // The memory semantics is always relaxed as we don't enable the Vulkan memory model extension.
    const spirv::IdMemorySemantics semanticsId =
        mBuilder.getUintConstant(spv::MemorySemanticsMaskNone);

    WriteAtomicOp writeAtomicOp = nullptr;

    const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));

    // TODO: determine isUnsigned correctly for image types.  Should rearrange TBasicType enums to
    // group images based on basic type and do range check.  http://anglebug.com/4889.
    const bool isUnsigned =
        node->getChildNode(0)->getAsTyped()->getType().getBasicType() == EbtUInt;

    switch (node->getOp())
    {
        case EOpAtomicAdd:
        case EOpImageAtomicAdd:
            writeAtomicOp = spirv::WriteAtomicIAdd;
            break;
        case EOpAtomicMin:
        case EOpImageAtomicMin:
            writeAtomicOp = isUnsigned ? spirv::WriteAtomicUMin : spirv::WriteAtomicSMin;
            break;
        case EOpAtomicMax:
        case EOpImageAtomicMax:
            writeAtomicOp = isUnsigned ? spirv::WriteAtomicUMax : spirv::WriteAtomicSMax;
            break;
        case EOpAtomicAnd:
        case EOpImageAtomicAnd:
            writeAtomicOp = spirv::WriteAtomicAnd;
            break;
        case EOpAtomicOr:
        case EOpImageAtomicOr:
            writeAtomicOp = spirv::WriteAtomicOr;
            break;
        case EOpAtomicXor:
        case EOpImageAtomicXor:
            writeAtomicOp = spirv::WriteAtomicXor;
            break;
        case EOpAtomicExchange:
        case EOpImageAtomicExchange:
            writeAtomicOp = spirv::WriteAtomicExchange;
            break;
        case EOpAtomicCompSwap:
        case EOpImageAtomicCompSwap:
            // Generate this special instruction right here and early out.  Note again that the
            // value and compare parameters of OpAtomicCompareExchange are in the opposite order
            // from GLSL.
            ASSERT(parameters.size() == 2);
            spirv::WriteAtomicCompareExchange(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId,
                                              result, pointerId, scopeId, semanticsId, semanticsId,
                                              parameters[1], parameters[0]);
            return result;
        default:
            UNREACHABLE();
    }

    // Write the instruction.
    ASSERT(parameters.size() == 1);
    writeAtomicOp(mBuilder.getSpirvCurrentFunctionBlock(), resultTypeId, result, pointerId, scopeId,
                  semanticsId, parameters[0]);

    return result;
}

spirv::IdRef OutputSPIRVTraverser::createTextureBuiltIn(TIntermOperator *node,
                                                        spirv::IdRef resultTypeId)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();
    return spirv::IdRef{};
}

spirv::IdRef OutputSPIRVTraverser::createImageBuiltIn(TIntermOperator *node,
                                                      spirv::IdRef resultTypeId)
{
    // TODO: http://anglebug.com/4889
    UNIMPLEMENTED();
    return spirv::IdRef{};
}

void OutputSPIRVTraverser::visitSymbol(TIntermSymbol *node)
{
    // Constants are expected to be folded.
    ASSERT(!node->hasConstantValue());

    // No-op visits to symbols that are being declared.  They are handled in visitDeclaration.
    if (mIsSymbolBeingDeclared)
    {
        // Make sure this does not affect other symbols, for example in the initializer expression.
        mIsSymbolBeingDeclared = false;
        return;
    }

    mNodeData.emplace_back();

    // The symbol is either:
    //
    // - A specialization constant
    // - A variable (local, varying etc)
    // - An interface block
    // - A field of an unnamed interface block
    //
    // Specialization constants in SPIR-V are treated largely like constants, in which case make
    // this behave like visitConstantUnion().

    const TType &type                     = node->getType();
    const TInterfaceBlock *interfaceBlock = type.getInterfaceBlock();
    const TSymbol *symbol                 = interfaceBlock;
    if (interfaceBlock == nullptr)
    {
        symbol = &node->variable();
    }

    // Track the block storage; it's needed to determine the derived type in an access chain, but is
    // not promoted in intermediate nodes' TType.
    TLayoutBlockStorage blockStorage = EbsUnspecified;
    if (interfaceBlock)
    {
        blockStorage = mBuilder.getBlockStorage(type);
    }

    const spirv::IdRef typeId = mBuilder.getTypeData(type, blockStorage).id;

    // If the symbol is a const variable, such as a const function parameter or specialization
    // constant, create an rvalue.
    if (type.getQualifier() == EvqConst || type.getQualifier() == EvqSpecConst)
    {
        ASSERT(mSymbolIdMap.count(symbol) > 0);
        nodeDataInitRValue(&mNodeData.back(), mSymbolIdMap[symbol], typeId);
        return;
    }

    // Otherwise create an lvalue.
    spv::StorageClass storageClass;
    const spirv::IdRef symbolId = getSymbolIdAndStorageClass(symbol, type, &storageClass);

    nodeDataInitLValue(&mNodeData.back(), symbolId, typeId, storageClass, blockStorage);

    // If a field of a nameless interface block, create an access chain.
    if (interfaceBlock && !type.isInterfaceBlock())
    {
        uint32_t fieldIndex = static_cast<uint32_t>(type.getInterfaceBlockFieldIndex());
        accessChainPushLiteral(&mNodeData.back(), spirv::LiteralInteger(fieldIndex), typeId);
    }
}

void OutputSPIRVTraverser::visitConstantUnion(TIntermConstantUnion *node)
{
    mNodeData.emplace_back();

    const TType &type = node->getType();

    // Find out the expected type for this constant, so it can be cast right away and not need an
    // instruction to do that.
    TIntermNode *parent     = getParentNode();
    const size_t childIndex = getParentChildIndex(PreVisit);

    TBasicType expectedBasicType = type.getBasicType();
    if (parent->getAsAggregate())
    {
        TIntermAggregate *parentAggregate = parent->getAsAggregate();

        // There are three possibilities:
        //
        // - It's a struct constructor: The basic type must match that of the corresponding field of
        //   the struct.
        // - It's a non struct constructor: The basic type must match that of the the type being
        //   constructed.
        // - It's a function call: The basic type must match that of the corresponding argument.
        if (parentAggregate->isConstructor())
        {
            const TStructure *structure = parentAggregate->getType().getStruct();
            if (structure != nullptr)
            {
                expectedBasicType = structure->fields()[childIndex]->type()->getBasicType();
            }
            else
            {
                expectedBasicType = parentAggregate->getType().getBasicType();
            }
        }
        else
        {
            expectedBasicType =
                parentAggregate->getFunction()->getParam(childIndex)->getType().getBasicType();
        }
    }
    // TODO: other node types such as binary, ternary etc.  http://anglebug.com/4889

    const spirv::IdRef typeId  = mBuilder.getTypeData(type, EbsUnspecified).id;
    const spirv::IdRef constId = createConstant(type, expectedBasicType, node->getConstantValue());

    nodeDataInitRValue(&mNodeData.back(), constId, typeId);
}

bool OutputSPIRVTraverser::visitSwizzle(Visit visit, TIntermSwizzle *node)
{
    // Constants are expected to be folded.
    ASSERT(!node->hasConstantValue());

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
    // Constants are expected to be folded.
    ASSERT(!node->hasConstantValue());

    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The left child will create one, which we won't pop.
        return true;
    }

    // If this is a variable initialization node, defer any code generation to visitDeclaration.
    if (node->getOp() == EOpInitialize)
    {
        ASSERT(getParentNode()->getAsDeclarationNode() != nullptr);
        return true;
    }

    if (IsShortCircuitNeeded(node))
    {
        // For && and ||, if short-circuiting behavior is needed, we need to emulate it with an
        // |if| construct.  At this point, the left-hand side is already evaluated, so we need to
        // create an appropriate conditional on in-visit and visit the right-hand-side inside the
        // conditional block.  On post-visit, OpPhi is used to calculate the result.
        if (visit == InVisit)
        {
            startShortCircuit(node);
            return true;
        }

        spirv::IdRef typeId;
        const spirv::IdRef result = endShortCircuit(node, &typeId);

        // Replace the access chain with an rvalue that's the result.
        nodeDataInitRValue(&mNodeData.back(), result, typeId);

        return true;
    }

    if (visit == InVisit)
    {
        // Left child visited.  Take the entry it created as the current node's.
        ASSERT(mNodeData.size() >= 1);

        // As an optimization, if the index is EOpIndexDirect*, take the constant index directly and
        // add it to the access chain as literal.
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

    TLayoutBlockStorage blockStorage = EbsUnspecified;
    if (node->getOp() == EOpIndexIndirect || node->getOp() == EOpAssign)
    {
        blockStorage = mNodeData[mNodeData.size() - 2].accessChain.baseBlockStorage;
    }
    const spirv::IdRef resultTypeId = mBuilder.getTypeData(node->getType(), blockStorage).id;

    // For EOpIndex* operations, push the right value as an index to the left value's access chain.
    // For the other operations, evaluate the expression.
    switch (node->getOp())
    {
        case EOpIndexDirect:
        case EOpIndexDirectStruct:
        case EOpIndexDirectInterfaceBlock:
            UNREACHABLE();
            break;
        case EOpIndexIndirect:
        {
            // Load the index.
            const spirv::IdRef rightValue = accessChainLoad(
                &mNodeData.back(), mBuilder.getDecorations(node->getRight()->getType()));
            mNodeData.pop_back();

            if (!node->getLeft()->getType().isArray() && node->getLeft()->getType().isVector())
            {
                accessChainPushDynamicComponent(&mNodeData.back(), rightValue, resultTypeId);
            }
            else
            {
                accessChainPush(&mNodeData.back(), rightValue, resultTypeId);
            }
            break;
        }

        case EOpAssign:
        {
            // Load the right hand side of assignment.
            const spirv::IdRef rightValue = accessChainLoad(
                &mNodeData.back(), mBuilder.getDecorations(node->getRight()->getType()));
            mNodeData.pop_back();

            // Store into the access chain.  Since the result of the (a = b) expression is b, change
            // the access chain to an unindexed rvalue which is |rightValue|.
            // TODO: handle mismatching types.  http://anglebug.com/4889.
            accessChainStore(&mNodeData.back(), rightValue);
            nodeDataInitRValue(&mNodeData.back(), rightValue, resultTypeId);
            break;
        }

        case EOpComma:
            // When the expression a,b is visited, all side effects of a and b are already
            // processed.  What's left is to to replace the expression with the result of b.  This
            // is simply done by dropping the left node and placing the right node as the result.
            mNodeData.erase(mNodeData.begin() + mNodeData.size() - 2);
            break;

        default:
            const spirv::IdRef result = visitOperator(node, resultTypeId);
            mNodeData.pop_back();
            nodeDataInitRValue(&mNodeData.back(), result, resultTypeId);
            // TODO: Handle NoContraction decoration.  http://anglebug.com/4889
            break;
    }

    return true;
}

bool OutputSPIRVTraverser::visitUnary(Visit visit, TIntermUnary *node)
{
    // Constants are expected to be folded.
    ASSERT(!node->hasConstantValue());

    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The child will create one, which we won't pop.
        return true;
    }

    // It's a unary operation, so there can't be an InVisit.
    ASSERT(visit != InVisit);

    // There is at least on entry for the child.
    ASSERT(mNodeData.size() >= 1);

    // Special case EOpArrayLength.  .length() on sized arrays is already constant folded, so this
    // operation only applies to ssbo.last_member.length().  OpArrayLength takes the ssbo block
    // *type* and the field index of last_member, so those need to be extracted from the access
    // chain.  Additionally, OpArrayLength produces an unsigned int while GLSL produces an int, so a
    // final cast is necessary.
    if (node->getOp() == EOpArrayLength)
    {
        // The access chain must only include the base ssbo + one literal field index.
        ASSERT(mNodeData.back().idList.size() == 1 && !mNodeData.back().idList.back().id.valid());
        const spirv::LiteralInteger fieldIndex = mNodeData.back().idList.back().literal;

        // Get the interface block type from the operand, which is either a symbol or a binary
        // operator based on whether the interface block is nameless or not.
        TIntermTyped *operand = node->getOperand();
        TIntermTyped *ssbo =
            operand->getAsBinaryNode() ? operand->getAsBinaryNode()->getLeft() : operand;
        const spirv::IdRef typeId = mBuilder.getTypeData(ssbo->getType(), EbsUnspecified).id;

        // Get the int and uint type ids.
        SpirvType intType;
        intType.type                  = EbtInt;
        const spirv::IdRef intTypeId  = mBuilder.getSpirvTypeData(intType, nullptr).id;
        intType.type                  = EbtUInt;
        const spirv::IdRef uintTypeId = mBuilder.getSpirvTypeData(intType, nullptr).id;

        // Generate the instruction.
        const spirv::IdRef resultId = mBuilder.getNewId({});
        spirv::WriteArrayLength(mBuilder.getSpirvCurrentFunctionBlock(), uintTypeId, resultId,
                                typeId, fieldIndex);

        // Cast to int.
        const spirv::IdRef castResultId = mBuilder.getNewId({});
        spirv::WriteBitcast(mBuilder.getSpirvCurrentFunctionBlock(), intTypeId, castResultId,
                            resultId);

        // Replace the access chain with an rvalue that's the result.
        nodeDataInitRValue(&mNodeData.back(), castResultId, intTypeId);

        return true;
    }

    const spirv::IdRef resultTypeId = mBuilder.getTypeData(node->getType(), EbsUnspecified).id;
    const spirv::IdRef result       = visitOperator(node, resultTypeId);

    // Keep the result as rvalue.
    nodeDataInitRValue(&mNodeData.back(), result, resultTypeId);

    return true;
}

bool OutputSPIRVTraverser::visitTernary(Visit visit, TIntermTernary *node)
{
    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The condition will create one, which we won't pop.
        return true;
    }

    size_t lastChildIndex = getLastTraversedChildIndex(visit);

    // If the condition was just visited, evaluate it and decide if OpSelect could be used or an
    // if-else must be emitted.  OpSelect is only used if the type is scalar or vector (required by
    // OpSelect) and if neither side has a side effect.
    const TType &type         = node->getType();
    const bool canUseOpSelect = (type.isScalar() || type.isVector()) &&
                                !node->getTrueExpression()->hasSideEffects() &&
                                !node->getFalseExpression()->hasSideEffects();

    if (lastChildIndex == 0)
    {
        spirv::IdRef typeId         = getAccessChainTypeId(&mNodeData.back());
        spirv::IdRef conditionValue = accessChainLoad(
            &mNodeData.back(), mBuilder.getDecorations(node->getCondition()->getType()));

        // If OpSelect can be used, keep the condition for later usage.
        if (canUseOpSelect)
        {
            // SPIR-V 1.0 requires that the condition value have as many components as the result.
            // So when selecting between vectors, we must replicate the condition scalar.
            if (type.isVector())
            {
                SpirvType spirvType;
                spirvType.type        = node->getCondition()->getType().getBasicType();
                spirvType.primarySize = static_cast<uint8_t>(type.getNominalSize());

                typeId = mBuilder.getSpirvTypeData(spirvType, nullptr).id;
                conditionValue =
                    createConstructorVectorFromScalar(type, typeId, {{conditionValue}});
            }
            nodeDataInitRValue(&mNodeData.back(), conditionValue, typeId);
            return true;
        }

        // Otherwise generate an if-else construct.

        // Three blocks necessary; the true, false and merge.
        mBuilder.startConditional(3, false, false);

        // Generate the branch instructions.
        const SpirvConditional *conditional = mBuilder.getCurrentConditional();

        const spirv::IdRef trueBlockId  = conditional->blockIds[0];
        const spirv::IdRef falseBlockId = conditional->blockIds[1];
        const spirv::IdRef mergeBlockId = conditional->blockIds.back();

        mBuilder.writeBranchConditional(conditionValue, trueBlockId, falseBlockId, mergeBlockId);
        return true;
    }

    // Load the result of the true or false part, and keep it for the end.  It's either used in
    // OpSelect or OpPhi.
    // TODO: handle mismatching types.  http://anglebug.com/4889.
    const spirv::IdRef typeId = getAccessChainTypeId(&mNodeData.back());
    const spirv::IdRef value  = accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(type));
    mNodeData.pop_back();
    mNodeData.back().idList.push_back(value);

    if (!canUseOpSelect)
    {
        // Move on to the next block.
        mBuilder.writeBranchConditionalBlockEnd();
    }

    // When done, generate either OpSelect or OpPhi.
    if (visit == PostVisit)
    {
        const spirv::IdRef result = mBuilder.getNewId(mBuilder.getDecorations(node->getType()));

        ASSERT(mNodeData.back().idList.size() == 2);
        const spirv::IdRef trueValue  = mNodeData.back().idList[0].id;
        const spirv::IdRef falseValue = mNodeData.back().idList[1].id;

        if (canUseOpSelect)
        {
            const spirv::IdRef conditionValue = mNodeData.back().baseId;

            spirv::WriteSelect(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                               conditionValue, trueValue, falseValue);
        }
        else
        {
            const SpirvConditional *conditional = mBuilder.getCurrentConditional();

            const spirv::IdRef trueBlockId  = conditional->blockIds[0];
            const spirv::IdRef falseBlockId = conditional->blockIds[1];

            spirv::WritePhi(mBuilder.getSpirvCurrentFunctionBlock(), typeId, result,
                            {spirv::PairIdRefIdRef{trueValue, trueBlockId},
                             spirv::PairIdRefIdRef{falseValue, falseBlockId}});

            mBuilder.endConditional();
        }

        // Replace the access chain with an rvalue that's the result.
        nodeDataInitRValue(&mNodeData.back(), result, typeId);
    }

    return true;
}

bool OutputSPIRVTraverser::visitIfElse(Visit visit, TIntermIfElse *node)
{
    if (visit == PreVisit)
    {
        // Don't add an entry to the stack.  The condition will create one, which we won't pop.
        return true;
    }

    const size_t lastChildIndex = getLastTraversedChildIndex(visit);

    // If the condition was just visited, evaluate it and create the branch instructions.
    if (lastChildIndex == 0)
    {
        const spirv::IdRef conditionValue = accessChainLoad(
            &mNodeData.back(), mBuilder.getDecorations(node->getCondition()->getType()));

        // Create a conditional with maximum 3 blocks, one for the true block (if any), one for the
        // else block (if any), and one for the merge block.  getChildCount() works here as it
        // produces an identical count.
        mBuilder.startConditional(node->getChildCount(), false, false);

        // Generate the branch instructions.
        const SpirvConditional *conditional = mBuilder.getCurrentConditional();

        const spirv::IdRef mergeBlock = conditional->blockIds.back();
        spirv::IdRef trueBlock        = mergeBlock;
        spirv::IdRef falseBlock       = mergeBlock;

        size_t nextBlockIndex = 0;
        if (node->getTrueBlock())
        {
            trueBlock = conditional->blockIds[nextBlockIndex++];
        }
        if (node->getFalseBlock())
        {
            falseBlock = conditional->blockIds[nextBlockIndex++];
        }

        mBuilder.writeBranchConditional(conditionValue, trueBlock, falseBlock, mergeBlock);
        return true;
    }

    // Otherwise move on to the next block, inserting a branch to the merge block at the end of each
    // block.
    mBuilder.writeBranchConditionalBlockEnd();

    // Pop from the conditional stack when done.
    if (visit == PostVisit)
    {
        mBuilder.endConditional();
    }

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

    // Any construct that needs code blocks must have already handled creating the necessary blocks
    // and setting the right one "current".  If there's a block opened in GLSL for scoping reasons,
    // it's ignored here as there are no scopes within a function in SPIR-V.
    if (visit == PreVisit)
    {
        return node->getChildCount() > 0;
    }

    // Any node that needed to generate code has already done so, just clean up its data.  If
    // the child node has no effect, it's automatically discarded (such as variable.field[n].x,
    // side effects of n already having generated code).
    mNodeData.pop_back();

    return true;
}

bool OutputSPIRVTraverser::visitFunctionDefinition(Visit visit, TIntermFunctionDefinition *node)
{
    if (visit == PreVisit)
    {
        return true;
    }

    // After the prototype is visited, generate the initial code for the function.
    if (visit == InVisit)
    {
        const TFunction *function = node->getFunction();

        ASSERT(mFunctionIdMap.count(function) > 0);
        const FunctionIds &ids = mFunctionIdMap[function];

        // Declare the function.
        spirv::WriteFunction(mBuilder.getSpirvFunctions(), ids.returnTypeId, ids.functionId,
                             spv::FunctionControlMaskNone, ids.functionTypeId);

        for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
        {
            const TVariable *paramVariable = function->getParam(paramIndex);

            const spirv::IdRef paramId =
                mBuilder.getNewId(mBuilder.getDecorations(paramVariable->getType()));
            spirv::WriteFunctionParameter(mBuilder.getSpirvFunctions(),
                                          ids.parameterTypeIds[paramIndex], paramId);

            // Remember the id of the variable for future look up.
            ASSERT(mSymbolIdMap.count(paramVariable) == 0);
            mSymbolIdMap[paramVariable] = paramId;

            spirv::WriteName(mBuilder.getSpirvDebug(), paramId,
                             mBuilder.hashName(paramVariable).data());
        }

        mBuilder.startNewFunction(ids.functionId, function);

        return true;
    }

    // If no explicit return was specified, add one automatically here.
    if (!mBuilder.isCurrentFunctionBlockTerminated())
    {
        // Only meaningful if the function returns void.  Otherwise it must have had a return
        // value.
        ASSERT(node->getFunction()->getReturnType().getBasicType() == EbtVoid);
        spirv::WriteReturn(mBuilder.getSpirvCurrentFunctionBlock());
        mBuilder.terminateCurrentFunctionBlock();
    }

    mBuilder.assembleSpirvFunctionBlocks();

    // End the function
    spirv::WriteFunctionEnd(mBuilder.getSpirvFunctions());

    return true;
}

bool OutputSPIRVTraverser::visitGlobalQualifierDeclaration(Visit visit,
                                                           TIntermGlobalQualifierDeclaration *node)
{
    if (node->isPrecise())
    {
        // TODO: handle precise.  http://anglebug.com/4889.
        UNIMPLEMENTED();
        return false;
    }

    // Global qualifier declarations apply to variables that are already declared.  Invariant simply
    // adds a decoration to the variable declaration, which can be done right away.  Note that
    // invariant cannot be applied to block members like this, except for gl_PerVertex built-ins,
    // which are applied to the members directly by DeclarePerVertexBlocks.
    ASSERT(node->isInvariant());

    const TVariable *variable = &node->getSymbol()->variable();
    ASSERT(mSymbolIdMap.count(variable) > 0);

    const spirv::IdRef variableId = mSymbolIdMap[variable];

    spirv::WriteDecorate(mBuilder.getSpirvDecorations(), variableId, spv::DecorationInvariant, {});

    return false;
}

void OutputSPIRVTraverser::visitFunctionPrototype(TIntermFunctionPrototype *node)
{
    const TFunction *function = node->getFunction();

    // If the function was previously forward declared, skip this.
    if (mFunctionIdMap.count(function) > 0)
    {
        return;
    }

    FunctionIds ids;

    // Declare the function type
    ids.returnTypeId = mBuilder.getTypeData(function->getReturnType(), EbsUnspecified).id;

    spirv::IdRefList paramTypeIds;
    for (size_t paramIndex = 0; paramIndex < function->getParamCount(); ++paramIndex)
    {
        const TType &paramType = function->getParam(paramIndex)->getType();

        spirv::IdRef paramId = mBuilder.getTypeData(paramType, EbsUnspecified).id;

        // const function parameters are intermediate values, while the rest are "variables"
        // with the Function storage class.
        if (paramType.getQualifier() != EvqConst)
        {
            paramId = mBuilder.getTypePointerId(paramId, spv::StorageClassFunction);
        }

        ids.parameterTypeIds.push_back(paramId);
    }

    ids.functionTypeId = mBuilder.getFunctionTypeId(ids.returnTypeId, ids.parameterTypeIds);

    // Allocate an id for the function up-front.
    //
    // Apply decorations to the return value of the function by applying them to the OpFunction
    // instruction.
    ids.functionId = mBuilder.getNewId(mBuilder.getDecorations(function->getReturnType()));

    // Remember the ID of main() for the sake of OpEntryPoint.
    if (function->isMain())
    {
        mBuilder.setEntryPointId(ids.functionId);
    }

    // Remember the id of the function for future look up.
    mFunctionIdMap[function] = ids;
}

bool OutputSPIRVTraverser::visitAggregate(Visit visit, TIntermAggregate *node)
{
    // Constants are expected to be folded.  However, large constructors (such as arrays) are not
    // folded and are handled here.
    ASSERT(node->getOp() == EOpConstruct || !node->hasConstantValue());

    if (visit == PreVisit)
    {
        mNodeData.emplace_back();
        return true;
    }

    // Keep the parameters on the stack.  If a function call contains out or inout parameters, we
    // need to know the access chains for the eventual write back to them.
    if (visit == InVisit)
    {
        return true;
    }

    // Expect to have accumulated as many parameters as the node requires.
    ASSERT(mNodeData.size() > node->getChildCount());

    const spirv::IdRef resultTypeId = mBuilder.getTypeData(node->getType(), EbsUnspecified).id;
    spirv::IdRef result;

    switch (node->getOp())
    {
        case EOpConstruct:
            // Construct a value out of the accumulated parameters.
            result = createConstructor(node, resultTypeId);
            break;
        case EOpCallFunctionInAST:
            // Create a call to the function.
            result = createFunctionCall(node, resultTypeId);
            break;

        case EOpMemoryBarrier:
        case EOpMemoryBarrierAtomicCounter:
        case EOpMemoryBarrierBuffer:
        case EOpMemoryBarrierImage:
        case EOpBarrier:
        case EOpMemoryBarrierShared:
        case EOpGroupMemoryBarrier:
        case EOpBarrierTCS:
            // TODO: support barriers.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        case EOpEmitVertex:
        case EOpEndPrimitive:
        case EOpEmitStreamVertex:
        case EOpEndStreamPrimitive:
            // TODO: support geometry shaders.  http://anglebug.com/4889
            UNIMPLEMENTED();
            break;

        default:
            result = visitOperator(node, resultTypeId);
            break;
    }

    // Pop the parameters.
    mNodeData.resize(mNodeData.size() - node->getChildCount());

    // Keep the result as rvalue.
    nodeDataInitRValue(&mNodeData.back(), result, resultTypeId);

    return false;
}

bool OutputSPIRVTraverser::visitDeclaration(Visit visit, TIntermDeclaration *node)
{
    const TIntermSequence &sequence = *node->getSequence();

    // Enforced by ValidateASTOptions::validateMultiDeclarations.
    ASSERT(sequence.size() == 1);

    // Declare specialization constants especially; they don't require processing the left and right
    // nodes, and they are like constant declarations with special instructions and decorations.
    if (sequence.front()->getAsTyped()->getType().getQualifier() == EvqSpecConst)
    {
        declareSpecConst(node);
        return false;
    }

    if (!mInGlobalScope && visit == PreVisit)
    {
        mNodeData.emplace_back();
    }

    mIsSymbolBeingDeclared = visit == PreVisit;

    if (visit != PostVisit)
    {
        return true;
    }

    TIntermSymbol *symbol = sequence.front()->getAsSymbolNode();
    spirv::IdRef initializerId;
    bool initializeWithDeclaration = false;

    // Handle declarations with initializer.
    if (symbol == nullptr)
    {
        TIntermBinary *assign = sequence.front()->getAsBinaryNode();
        ASSERT(assign != nullptr && assign->getOp() == EOpInitialize);

        symbol = assign->getLeft()->getAsSymbolNode();
        ASSERT(symbol != nullptr);

        // In SPIR-V, it's only possible to initialize a variable together with its declaration if
        // the initializer is a constant or a global variable.  We ignore the global variable case
        // to avoid tracking whether the variable has been modified since the beginning of the
        // function.  Since variable declarations are always placed at the beginning of the function
        // in SPIR-V, it would be wrong for example to initialize |var| below with the global
        // variable at declaration time:
        //
        //     vec4 global = A;
        //     void f()
        //     {
        //         global = B;
        //         {
        //             vec4 var = global;
        //         }
        //     }
        //
        // So the initializer is only used when declarating a variable when it's a constant
        // expression.  Note that if the variable being declared is itself global (and the
        // initializer is not constant), a previous AST transformation (DeferGlobalInitializers)
        // makes sure their initialization is deferred to the beginning of main.
        //
        // Additionally, if the variable is being defined inside a loop, the initializer is not used
        // as that would prevent it from being reintialized in the next iteration of the loop.

        TIntermTyped *initializer = assign->getRight();
        initializeWithDeclaration =
            !mBuilder.isInLoop() &&
            (initializer->getAsConstantUnion() != nullptr || initializer->hasConstantValue());

        if (initializeWithDeclaration)
        {
            // If a constant, take the Id directly.
            initializerId = mNodeData.back().baseId;
        }
        else
        {
            // Otherwise generate code to load from right hand side expression.
            initializerId =
                accessChainLoad(&mNodeData.back(), mBuilder.getDecorations(initializer->getType()));
        }

        // TODO: handle mismatching types.  http://anglebug.com/4889.

        // Clean up the initializer data.
        mNodeData.pop_back();
    }

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

    spv::StorageClass storageClass = GetStorageClass(type);

    SpirvDecorations decorations = mBuilder.getDecorations(type);
    if (mBuilder.isInvariantOutput(type))
    {
        // Apply the Invariant decoration to output variables if specified or if globally enabled.
        decorations.push_back(spv::DecorationInvariant);
    }

    const spirv::IdRef variableId = mBuilder.declareVariable(
        typeId, storageClass, decorations, initializeWithDeclaration ? &initializerId : nullptr,
        mBuilder.hashName(variable).data());

    if (!initializeWithDeclaration && initializerId.valid())
    {
        // If not initializing at the same time as the declaration, issue a store instruction.
        spirv::WriteStore(mBuilder.getSpirvCurrentFunctionBlock(), variableId, initializerId,
                          nullptr);
    }

    const bool isShaderInOut = IsShaderIn(type.getQualifier()) || IsShaderOut(type.getQualifier());
    const bool isInterfaceBlock = type.getBasicType() == EbtInterfaceBlock;

    // Add decorations, which apply to the element type of arrays, if array.
    spirv::IdRef nonArrayTypeId = typeId;
    if (type.isArray() && (isShaderInOut || isInterfaceBlock))
    {
        SpirvType elementType  = mBuilder.getSpirvType(type, EbsUnspecified);
        elementType.arraySizes = {};
        nonArrayTypeId         = mBuilder.getSpirvTypeData(elementType, nullptr).id;
    }

    if (isShaderInOut)
    {
        // Add in and out variables to the list of interface variables.
        mBuilder.addEntryPointInterfaceVariableId(variableId);

        if (IsShaderIoBlock(type.getQualifier()) && type.isInterfaceBlock())
        {
            // For gl_PerVertex in particular, write the necessary BuiltIn decorations
            if (type.getQualifier() == EvqPerVertexIn || type.getQualifier() == EvqPerVertexOut)
            {
                mBuilder.writePerVertexBuiltIns(type, nonArrayTypeId);
            }

            // I/O blocks are decorated with Block
            spirv::WriteDecorate(mBuilder.getSpirvDecorations(), nonArrayTypeId,
                                 spv::DecorationBlock, {});
        }
    }
    else if (isInterfaceBlock)
    {
        // For uniform and buffer variables, add Block and BufferBlock decorations respectively.
        const spv::Decoration decoration =
            type.getQualifier() == EvqUniform ? spv::DecorationBlock : spv::DecorationBufferBlock;
        spirv::WriteDecorate(mBuilder.getSpirvDecorations(), nonArrayTypeId, decoration, {});
    }

    // Write DescriptorSet, Binding, Location etc decorations if necessary.
    mBuilder.writeInterfaceVariableDecorations(type, variableId);

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

void GetLoopBlocks(const SpirvConditional *conditional,
                   TLoopType loopType,
                   bool hasCondition,
                   spirv::IdRef *headerBlock,
                   spirv::IdRef *condBlock,
                   spirv::IdRef *bodyBlock,
                   spirv::IdRef *continueBlock,
                   spirv::IdRef *mergeBlock)
{
    // The order of the blocks is for |for| and |while|:
    //
    //     %header %cond [optional] %body %continue %merge
    //
    // and for |do-while|:
    //
    //     %header %body %cond %merge
    //
    // Note that the |break| target is always the last block and the |continue| target is the one
    // before last.
    //
    // If %continue is not present, all jumps are made to %cond (which is necessarily present).
    // If %cond is not present, all jumps are made to %body instead.

    size_t nextBlock = 0;
    *headerBlock     = conditional->blockIds[nextBlock++];
    // %cond, if any is after header except for |do-while|.
    if (loopType != ELoopDoWhile && hasCondition)
    {
        *condBlock = conditional->blockIds[nextBlock++];
    }
    *bodyBlock = conditional->blockIds[nextBlock++];
    // After the block is either %cond or %continue based on |do-while| or not.
    if (loopType != ELoopDoWhile)
    {
        *continueBlock = conditional->blockIds[nextBlock++];
    }
    else
    {
        *condBlock = conditional->blockIds[nextBlock++];
    }
    *mergeBlock = conditional->blockIds[nextBlock++];

    ASSERT(nextBlock == conditional->blockIds.size());

    if (!continueBlock->valid())
    {
        ASSERT(condBlock->valid());
        *continueBlock = *condBlock;
    }
    if (!condBlock->valid())
    {
        *condBlock = *bodyBlock;
    }
}

bool OutputSPIRVTraverser::visitLoop(Visit visit, TIntermLoop *node)
{
    // There are three kinds of loops, and they translate as such:
    //
    // for (init; cond; expr) body;
    //
    //               // pre-loop block
    //               init
    //               OpBranch %header
    //
    //     %header = OpLabel
    //               OpLoopMerge %merge %continue None
    //               OpBranch %cond
    //
    //               // Note: if cond doesn't exist, this section is not generated.  The above
    //               // OpBranch would jump directly to %body.
    //       %cond = OpLabel
    //          %v = cond
    //               OpBranchConditional %v %body %merge None
    //
    //       %body = OpLabel
    //               body
    //               OpBranch %continue
    //
    //   %continue = OpLabel
    //               expr
    //               OpBranch %header
    //
    //               // post-loop block
    //       %merge = OpLabel
    //
    //
    // while (cond) body;
    //
    //               // pre-for block
    //               OpBranch %header
    //
    //     %header = OpLabel
    //               OpLoopMerge %merge %continue None
    //               OpBranch %cond
    //
    //       %cond = OpLabel
    //          %v = cond
    //               OpBranchConditional %v %body %merge None
    //
    //       %body = OpLabel
    //               body
    //               OpBranch %continue
    //
    //   %continue = OpLabel
    //               OpBranch %header
    //
    //               // post-loop block
    //       %merge = OpLabel
    //
    //
    // do body; while (cond);
    //
    //               // pre-for block
    //               OpBranch %header
    //
    //     %header = OpLabel
    //               OpLoopMerge %merge %cond None
    //               OpBranch %body
    //
    //       %body = OpLabel
    //               body
    //               OpBranch %cond
    //
    //       %cond = OpLabel
    //          %v = cond
    //               OpBranchConditional %v %header %merge None
    //
    //               // post-loop block
    //       %merge = OpLabel
    //

    // The order of the blocks is not necessarily the same as traversed, so it's much simpler if
    // this function enforces traversal in the right order.
    ASSERT(visit == PreVisit);
    mNodeData.emplace_back();

    const TLoopType loopType = node->getType();

    // The init statement of a for loop is placed in the previous block, so continue generating code
    // as-is until that statement is done.
    if (node->getInit())
    {
        ASSERT(loopType == ELoopFor);
        node->getInit()->traverse(this);
        mNodeData.pop_back();
    }

    const bool hasCondition = node->getCondition() != nullptr;

    // Once the init node is visited, if any, we need to set up the loop.
    //
    // For |for| and |while|, we need %header, %body, %continue and %merge.  For |do-while|, we
    // need %header, %body and %merge.  If condition is present, an additional %cond block is
    // needed in each case.
    const size_t blockCount = (loopType == ELoopDoWhile ? 3 : 4) + (hasCondition ? 1 : 0);
    mBuilder.startConditional(blockCount, true, true);

    // Generate the %header block.
    const SpirvConditional *conditional = mBuilder.getCurrentConditional();

    spirv::IdRef headerBlock, condBlock, bodyBlock, continueBlock, mergeBlock;
    GetLoopBlocks(conditional, loopType, hasCondition, &headerBlock, &condBlock, &bodyBlock,
                  &continueBlock, &mergeBlock);

    mBuilder.writeLoopHeader(loopType == ELoopDoWhile ? bodyBlock : condBlock, continueBlock,
                             mergeBlock);

    // %cond, if any is after header except for |do-while|.
    if (loopType != ELoopDoWhile && hasCondition)
    {
        node->getCondition()->traverse(this);

        // Generate the branch at the end of the %cond block.
        const spirv::IdRef conditionValue = accessChainLoad(
            &mNodeData.back(), mBuilder.getDecorations(node->getCondition()->getType()));
        mBuilder.writeLoopConditionEnd(conditionValue, bodyBlock, mergeBlock);

        mNodeData.pop_back();
    }

    // Next comes %body.
    {
        node->getBody()->traverse(this);

        // Generate the branch at the end of the %body block.
        mBuilder.writeLoopBodyEnd(continueBlock);
    }

    switch (loopType)
    {
        case ELoopFor:
            // For |for| loops, the expression is placed after the body and acts as the continue
            // block.
            if (node->getExpression())
            {
                node->getExpression()->traverse(this);
                mNodeData.pop_back();
            }

            // Generate the branch at the end of the %continue block.
            mBuilder.writeLoopContinueEnd(headerBlock);
            break;

        case ELoopWhile:
            // |for| loops have the expression in the continue block and |do-while| loops have their
            // condition block act as the loop's continue block.  |while| loops need a branch-only
            // continue loop, which is generated here.
            mBuilder.writeLoopContinueEnd(headerBlock);
            break;

        case ELoopDoWhile:
            // For |do-while|, %cond comes last.
            ASSERT(hasCondition);
            node->getCondition()->traverse(this);

            // Generate the branch at the end of the %cond block.
            const spirv::IdRef conditionValue = accessChainLoad(
                &mNodeData.back(), mBuilder.getDecorations(node->getCondition()->getType()));
            mBuilder.writeLoopConditionEnd(conditionValue, headerBlock, mergeBlock);

            mNodeData.pop_back();
            break;
    }

    // Pop from the conditional stack when done.
    mBuilder.endConditional();

    // Don't traverse the children, that's done already.
    return false;
}

bool OutputSPIRVTraverser::visitBranch(Visit visit, TIntermBranch *node)
{
    if (visit == PreVisit)
    {
        mNodeData.emplace_back();
        return true;
    }

    // There is only ever one child at most.
    ASSERT(visit != InVisit);

    switch (node->getFlowOp())
    {
        case EOpKill:
            spirv::WriteKill(mBuilder.getSpirvCurrentFunctionBlock());
            mBuilder.terminateCurrentFunctionBlock();
            break;
        case EOpBreak:
            // TODO: http://anglebug.com/4889
            UNIMPLEMENTED();
            break;
        case EOpContinue:
            // TODO: http://anglebug.com/4889
            UNIMPLEMENTED();
            break;
        case EOpReturn:
            // Evaluate the expression if any, and return.
            if (node->getExpression() != nullptr)
            {
                ASSERT(mNodeData.size() >= 1);

                const spirv::IdRef expressionValue = accessChainLoad(
                    &mNodeData.back(), mBuilder.getDecorations(node->getExpression()->getType()));
                mNodeData.pop_back();

                // TODO: handle mismatching types.  http://anglebug.com/6000

                spirv::WriteReturnValue(mBuilder.getSpirvCurrentFunctionBlock(), expressionValue);
                mBuilder.terminateCurrentFunctionBlock();
            }
            else
            {
                spirv::WriteReturn(mBuilder.getSpirvCurrentFunctionBlock());
                mBuilder.terminateCurrentFunctionBlock();
            }
            break;
        default:
            UNREACHABLE();
    }

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

#if ANGLE_DEBUG_SPIRV_GENERATION
    // Disassemble and log the generated SPIR-V for debugging.
    spvtools::SpirvTools spirvTools(SPV_ENV_VULKAN_1_1);
    std::string readableSpirv;
    spirvTools.Disassemble(result, &readableSpirv, 0);
    fprintf(stderr, "%s\n", readableSpirv.c_str());
#endif  // ANGLE_DEBUG_SPIRV_GENERATION

    return result;
}
}  // anonymous namespace

bool OutputSPIRV(TCompiler *compiler,
                 TIntermBlock *root,
                 ShCompileOptions compileOptions,
                 bool forceHighp)
{
    // Traverse the tree and generate SPIR-V instructions
    OutputSPIRVTraverser traverser(compiler, compileOptions, forceHighp);
    root->traverse(&traverser);

    // Generate the final SPIR-V and store in the sink
    spirv::Blob spirvBlob = traverser.getSpirv();
    compiler->getInfoSink().obj.setBinary(std::move(spirvBlob));

    return true;
}
}  // namespace sh

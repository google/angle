//
// Copyright 2020 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// DriverUniform.cpp: Add code to support driver uniforms
//

#include "compiler/translator/tree_util/DriverUniform.h"

#include "compiler/translator/Compiler.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/tree_util/FindMain.h"
#include "compiler/translator/tree_util/IntermNode_util.h"
#include "compiler/translator/tree_util/IntermTraverse.h"
#include "compiler/translator/util.h"

namespace sh
{

namespace
{
constexpr ImmutableString kEmulatedDepthRangeParams = ImmutableString("ANGLEDepthRangeParams");

constexpr const char kViewport[]             = "viewport";
constexpr const char kHalfRenderArea[]       = "halfRenderArea";
constexpr const char kFlipXY[]               = "flipXY";
constexpr const char kNegFlipXY[]            = "negFlipXY";
constexpr const char kClipDistancesEnabled[] = "clipDistancesEnabled";
constexpr const char kXfbActiveUnpaused[]    = "xfbActiveUnpaused";
constexpr const char kXfbVerticesPerDraw[]   = "xfbVerticesPerDraw";
constexpr const char kXfbBufferOffsets[]     = "xfbBufferOffsets";
constexpr const char kAcbBufferOffsets[]     = "acbBufferOffsets";
constexpr const char kDepthRange[]           = "depthRange";
constexpr const char kPreRotation[]          = "preRotation";
constexpr const char kFragRotation[]         = "fragRotation";

constexpr size_t kNumGraphicsDriverUniforms                                                = 12;
constexpr std::array<const char *, kNumGraphicsDriverUniforms> kGraphicsDriverUniformNames = {
    {kViewport, kHalfRenderArea, kFlipXY, kNegFlipXY, kClipDistancesEnabled, kXfbActiveUnpaused,
     kXfbVerticesPerDraw, kXfbBufferOffsets, kAcbBufferOffsets, kDepthRange, kPreRotation,
     kFragRotation}};

constexpr size_t kNumComputeDriverUniforms                                               = 1;
constexpr std::array<const char *, kNumComputeDriverUniforms> kComputeDriverUniformNames = {
    {kAcbBufferOffsets}};

}  // anonymous namespace

bool DriverUniform::addComputeDriverUniformsToShader(TIntermBlock *root, TSymbolTable *symbolTable)
{
    ASSERT(!mDriverUniforms);
    // This field list mirrors the structure of ComputeDriverUniforms in ContextVk.cpp.
    TFieldList *driverFieldList = new TFieldList;

    const std::array<TType *, kNumComputeDriverUniforms> kDriverUniformTypes = {{
        new TType(EbtUInt, 4),
    }};

    for (size_t uniformIndex = 0; uniformIndex < kNumComputeDriverUniforms; ++uniformIndex)
    {
        TField *driverUniformField =
            new TField(kDriverUniformTypes[uniformIndex],
                       ImmutableString(kComputeDriverUniformNames[uniformIndex]), TSourceLoc(),
                       SymbolType::AngleInternal);
        driverFieldList->push_back(driverUniformField);
    }

    // Define a driver uniform block "ANGLEUniformBlock" with instance name "ANGLEUniforms".
    mDriverUniforms = DeclareInterfaceBlock(
        root, symbolTable, driverFieldList, EvqUniform, TMemoryQualifier::Create(), 0,
        ImmutableString(vk::kDriverUniformsBlockName), ImmutableString(vk::kDriverUniformsVarName));
    return mDriverUniforms != nullptr;
}

TFieldList *DriverUniform::createUniformFields(TSymbolTable *symbolTable) const
{
    // This field list mirrors the structure of GraphicsDriverUniforms in ContextVk.cpp.
    TFieldList *driverFieldList = new TFieldList;

    const std::array<TType *, kNumGraphicsDriverUniforms> kDriverUniformTypes = {{
        new TType(EbtFloat, 4),
        new TType(EbtFloat, 2),
        new TType(EbtFloat, 2),
        new TType(EbtFloat, 2),
        new TType(EbtUInt),  // uint clipDistancesEnabled;  // 32 bits for 32 clip distances max
        new TType(EbtUInt),
        new TType(EbtUInt),
        // NOTE: There's a vec3 gap here that can be used in the future
        new TType(EbtInt, 4),
        new TType(EbtUInt, 4),
        createEmulatedDepthRangeType(symbolTable),
        new TType(EbtFloat, 2, 2),
        new TType(EbtFloat, 2, 2),
    }};

    for (size_t uniformIndex = 0; uniformIndex < kNumGraphicsDriverUniforms; ++uniformIndex)
    {
        TField *driverUniformField =
            new TField(kDriverUniformTypes[uniformIndex],
                       ImmutableString(kGraphicsDriverUniformNames[uniformIndex]), TSourceLoc(),
                       SymbolType::AngleInternal);
        driverFieldList->push_back(driverUniformField);
    }

    return driverFieldList;
}

TType *DriverUniform::createEmulatedDepthRangeType(TSymbolTable *symbolTable) const
{
    // Init the depth range type.
    TFieldList *depthRangeParamsFields = new TFieldList();
    depthRangeParamsFields->push_back(new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1, 1),
                                                 ImmutableString("near"), TSourceLoc(),
                                                 SymbolType::AngleInternal));
    depthRangeParamsFields->push_back(new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1, 1),
                                                 ImmutableString("far"), TSourceLoc(),
                                                 SymbolType::AngleInternal));
    depthRangeParamsFields->push_back(new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1, 1),
                                                 ImmutableString("diff"), TSourceLoc(),
                                                 SymbolType::AngleInternal));
    // This additional field might be used by subclass such as TranslatorMetal.
    depthRangeParamsFields->push_back(new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1, 1),
                                                 ImmutableString("reserved"), TSourceLoc(),
                                                 SymbolType::AngleInternal));

    TStructure *emulatedDepthRangeParams = new TStructure(
        symbolTable, kEmulatedDepthRangeParams, depthRangeParamsFields, SymbolType::AngleInternal);

    TType *emulatedDepthRangeType = new TType(emulatedDepthRangeParams, false);

    return emulatedDepthRangeType;
}

// The Add*DriverUniformsToShader operation adds an internal uniform block to a shader. The driver
// block is used to implement Vulkan-specific features and workarounds. Returns the driver uniforms
// variable.
//
// There are Graphics and Compute variations as they require different uniforms.
bool DriverUniform::addGraphicsDriverUniformsToShader(TIntermBlock *root, TSymbolTable *symbolTable)
{
    ASSERT(!mDriverUniforms);

    TType *emulatedDepthRangeType = createEmulatedDepthRangeType(symbolTable);
    // Declare a global depth range variable.
    TVariable *depthRangeVar =
        new TVariable(symbolTable->nextUniqueId(), kEmptyImmutableString, SymbolType::Empty,
                      TExtension::UNDEFINED, emulatedDepthRangeType);

    DeclareGlobalVariable(root, depthRangeVar);

    TFieldList *driverFieldList = createUniformFields(symbolTable);
    // Define a driver uniform block "ANGLEUniformBlock" with instance name "ANGLEUniforms".
    mDriverUniforms = DeclareInterfaceBlock(
        root, symbolTable, driverFieldList, EvqUniform, TMemoryQualifier::Create(), 0,
        ImmutableString(vk::kDriverUniformsBlockName), ImmutableString(vk::kDriverUniformsVarName));

    return mDriverUniforms != nullptr;
}

TIntermBinary *DriverUniform::createDriverUniformRef(const char *fieldName) const
{
    size_t fieldIndex =
        FindFieldIndex(mDriverUniforms->getType().getInterfaceBlock()->fields(), fieldName);

    TIntermSymbol *angleUniformsRef = new TIntermSymbol(mDriverUniforms);
    TConstantUnion *uniformIndex    = new TConstantUnion;
    uniformIndex->setIConst(static_cast<int>(fieldIndex));
    TIntermConstantUnion *indexRef =
        new TIntermConstantUnion(uniformIndex, *StaticType::GetBasic<EbtInt>());
    return new TIntermBinary(EOpIndexDirectInterfaceBlock, angleUniformsRef, indexRef);
}

TIntermBinary *DriverUniform::getFlipXYRef() const
{
    return createDriverUniformRef(kFlipXY);
}

TIntermBinary *DriverUniform::getNegFlipXYRef() const
{
    return createDriverUniformRef(kNegFlipXY);
}

TIntermBinary *DriverUniform::getFragRotationMatrixRef() const
{
    return createDriverUniformRef(kFragRotation);
}

TIntermBinary *DriverUniform::getPreRotationMatrixRef() const
{
    return createDriverUniformRef(kPreRotation);
}

TIntermBinary *DriverUniform::getViewportRef() const
{
    return createDriverUniformRef(kViewport);
}

TIntermBinary *DriverUniform::getHalfRenderAreaRef() const
{
    return createDriverUniformRef(kHalfRenderArea);
}

TIntermBinary *DriverUniform::getAbcBufferOffsets() const
{
    return createDriverUniformRef(kAcbBufferOffsets);
}

TIntermBinary *DriverUniform::getClipDistancesEnabled() const
{
    return createDriverUniformRef(kClipDistancesEnabled);
}

TIntermBinary *DriverUniform::getDepthRangeRef() const
{
    return createDriverUniformRef(kDepthRange);
}

TIntermSwizzle *DriverUniform::getNegFlipYRef() const
{
    // Create a swizzle to "negFlipXY.y"
    TIntermBinary *negFlipXY    = createDriverUniformRef(kNegFlipXY);
    TVector<int> swizzleOffsetY = {1};
    TIntermSwizzle *negFlipY    = new TIntermSwizzle(negFlipXY, swizzleOffsetY);
    return negFlipY;
}

TIntermBinary *DriverUniform::getDepthRangeReservedFieldRef() const
{
    TIntermBinary *depthRange = createDriverUniformRef(kDepthRange);

    return new TIntermBinary(EOpIndexDirectStruct, depthRange, CreateIndexNode(3));
}

}  // namespace sh

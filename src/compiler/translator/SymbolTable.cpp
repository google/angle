//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// Symbol table for parsing. The design principles and most of the functionality are documented in
// the header file.
//

#if defined(_MSC_VER)
#pragma warning(disable : 4718)
#endif

#include "compiler/translator/SymbolTable.h"

#include <algorithm>
#include <set>

#include "angle_gl.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/StaticType.h"

namespace sh
{

class TSymbolTable::TSymbolTableLevel
{
  public:
    TSymbolTableLevel() : mGlobalInvariant(false) {}

    bool insert(TSymbol *symbol);

    // Insert a function using its unmangled name as the key.
    void insertUnmangled(TFunction *function);

    TSymbol *find(const ImmutableString &name) const;

    void addInvariantVarying(const ImmutableString &name) { mInvariantVaryings.insert(name); }

    bool isVaryingInvariant(const ImmutableString &name)
    {
        return (mGlobalInvariant || mInvariantVaryings.count(name) > 0);
    }

    void setGlobalInvariant(bool invariant) { mGlobalInvariant = invariant; }

  private:
    using tLevel        = TUnorderedMap<ImmutableString,
                                 TSymbol *,
                                 ImmutableString::FowlerNollVoHash<sizeof(size_t)>>;
    using tLevelPair    = const tLevel::value_type;
    using tInsertResult = std::pair<tLevel::iterator, bool>;

    tLevel level;

    std::set<ImmutableString> mInvariantVaryings;
    bool mGlobalInvariant;
};

class TSymbolTable::TSymbolTableBuiltInLevel
{
  public:
    TSymbolTableBuiltInLevel() = default;

    void insert(const TSymbol *symbol);
    const TSymbol *find(const ImmutableString &name) const;

  private:
    using tLevel        = TUnorderedMap<ImmutableString,
                                 const TSymbol *,
                                 ImmutableString::FowlerNollVoHash<sizeof(size_t)>>;
    using tLevelPair    = const tLevel::value_type;
    tLevel mLevel;
};

bool TSymbolTable::TSymbolTableLevel::insert(TSymbol *symbol)
{
    // returning true means symbol was added to the table
    tInsertResult result = level.insert(tLevelPair(symbol->getMangledName(), symbol));
    return result.second;
}

void TSymbolTable::TSymbolTableLevel::insertUnmangled(TFunction *function)
{
    level.insert(tLevelPair(function->name(), function));
}

TSymbol *TSymbolTable::TSymbolTableLevel::find(const ImmutableString &name) const
{
    tLevel::const_iterator it = level.find(name);
    if (it == level.end())
        return nullptr;
    else
        return (*it).second;
}

void TSymbolTable::TSymbolTableBuiltInLevel::insert(const TSymbol *symbol)
{
    mLevel.insert(tLevelPair(symbol->getMangledName(), symbol));
}

const TSymbol *TSymbolTable::TSymbolTableBuiltInLevel::find(const ImmutableString &name) const
{
    tLevel::const_iterator it = mLevel.find(name);
    if (it == mLevel.end())
        return nullptr;
    else
        return (*it).second;
}

TSymbolTable::TSymbolTable()
    : mUniqueIdCounter(0), mUserDefinedUniqueIdsStart(-1), mShaderType(GL_FRAGMENT_SHADER)
{
}

TSymbolTable::~TSymbolTable() = default;

bool TSymbolTable::isEmpty() const
{
    return mTable.empty();
}

bool TSymbolTable::atGlobalLevel() const
{
    return mTable.size() == 1u;
}

void TSymbolTable::pushBuiltInLevel()
{
    mBuiltInTable.push_back(
        std::unique_ptr<TSymbolTableBuiltInLevel>(new TSymbolTableBuiltInLevel));
}

void TSymbolTable::push()
{
    mTable.push_back(std::unique_ptr<TSymbolTableLevel>(new TSymbolTableLevel));
    mPrecisionStack.push_back(std::unique_ptr<PrecisionStackLevel>(new PrecisionStackLevel));
}

void TSymbolTable::pop()
{
    mTable.pop_back();
    mPrecisionStack.pop_back();
}

const TFunction *TSymbolTable::markFunctionHasPrototypeDeclaration(
    const ImmutableString &mangledName,
    bool *hadPrototypeDeclarationOut)
{
    TFunction *function         = findUserDefinedFunction(mangledName);
    *hadPrototypeDeclarationOut = function->hasPrototypeDeclaration();
    function->setHasPrototypeDeclaration();
    return function;
}

const TFunction *TSymbolTable::setFunctionParameterNamesFromDefinition(const TFunction *function,
                                                                       bool *wasDefinedOut)
{
    TFunction *firstDeclaration = findUserDefinedFunction(function->getMangledName());
    ASSERT(firstDeclaration);
    // Note: 'firstDeclaration' could be 'function' if this is the first time we've seen function as
    // it would have just been put in the symbol table. Otherwise, we're looking up an earlier
    // occurance.
    if (function != firstDeclaration)
    {
        // The previous declaration should have the same parameters as the function definition
        // (parameter names may differ).
        firstDeclaration->shareParameters(*function);
    }

    *wasDefinedOut = firstDeclaration->isDefined();
    firstDeclaration->setDefined();
    return firstDeclaration;
}

const TSymbol *TSymbolTable::find(const ImmutableString &name, int shaderVersion) const
{
    int userDefinedLevel = static_cast<int>(mTable.size()) - 1;
    while (userDefinedLevel >= 0)
    {
        const TSymbol *symbol = mTable[userDefinedLevel]->find(name);
        if (symbol)
        {
            return symbol;
        }
        userDefinedLevel--;
    }

    return findBuiltIn(name, shaderVersion, false);
}

TFunction *TSymbolTable::findUserDefinedFunction(const ImmutableString &name) const
{
    // User-defined functions are always declared at the global level.
    ASSERT(!mTable.empty());
    return static_cast<TFunction *>(mTable[0]->find(name));
}

const TSymbol *TSymbolTable::findGlobal(const ImmutableString &name) const
{
    ASSERT(!mTable.empty());
    return mTable[0]->find(name);
}

const TSymbol *TSymbolTable::findBuiltIn(const ImmutableString &name, int shaderVersion) const
{
    return findBuiltIn(name, shaderVersion, false);
}

const TSymbol *TSymbolTable::findBuiltIn(const ImmutableString &name,
                                         int shaderVersion,
                                         bool includeGLSLBuiltins) const
{
    for (int level = LAST_BUILTIN_LEVEL; level >= 0; level--)
    {
        if (level == GLSL_BUILTINS && !includeGLSLBuiltins)
            level--;
        if (level == ESSL3_1_BUILTINS && shaderVersion != 310)
            level--;
        if (level == ESSL3_BUILTINS && shaderVersion < 300)
            level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100)
            level--;

        const TSymbol *symbol = mBuiltInTable[level]->find(name);

        if (symbol)
            return symbol;
    }

    return nullptr;
}


bool TSymbolTable::declare(TSymbol *symbol)
{
    ASSERT(!mTable.empty());
    ASSERT(symbol->symbolType() == SymbolType::UserDefined);
    ASSERT(!symbol->isFunction());
    return mTable.back()->insert(symbol);
}

void TSymbolTable::declareUserDefinedFunction(TFunction *function, bool insertUnmangledName)
{
    ASSERT(!mTable.empty());
    if (insertUnmangledName)
    {
        // Insert the unmangled name to detect potential future redefinition as a variable.
        mTable[0]->insertUnmangled(function);
    }
    mTable[0]->insert(function);
}

void TSymbolTable::insertVariable(ESymbolLevel level,
                                  const ImmutableString &name,
                                  const TType *type)
{
    ASSERT(type->isRealized());
    TVariable *var = new TVariable(this, name, type, SymbolType::BuiltIn);
    insertBuiltIn(level, var);
}

void TSymbolTable::insertVariableExt(ESymbolLevel level,
                                     TExtension ext,
                                     const ImmutableString &name,
                                     const TType *type)
{
    ASSERT(type->isRealized());
    TVariable *var = new TVariable(this, name, type, SymbolType::BuiltIn, ext);
    insertBuiltIn(level, var);
}

void TSymbolTable::insertBuiltIn(ESymbolLevel level, const TSymbol *symbol)
{
    ASSERT(symbol);
    ASSERT(level <= LAST_BUILTIN_LEVEL);

    mBuiltInTable[level]->insert(symbol);
}

template <TPrecision precision>
void TSymbolTable::insertConstInt(ESymbolLevel level, const ImmutableString &name, int value)
{
    TVariable *constant = new TVariable(
        this, name, StaticType::Get<EbtInt, precision, EvqConst, 1, 1>(), SymbolType::BuiltIn);
    TConstantUnion *unionArray = new TConstantUnion[1];
    unionArray[0].setIConst(value);
    constant->shareConstPointer(unionArray);
    insertBuiltIn(level, constant);
}

template <TPrecision precision>
void TSymbolTable::insertConstIntExt(ESymbolLevel level,
                                     TExtension ext,
                                     const ImmutableString &name,
                                     int value)
{
    TVariable *constant = new TVariable(
        this, name, StaticType::Get<EbtInt, precision, EvqConst, 1, 1>(), SymbolType::BuiltIn, ext);
    TConstantUnion *unionArray = new TConstantUnion[1];
    unionArray[0].setIConst(value);
    constant->shareConstPointer(unionArray);
    insertBuiltIn(level, constant);
}

template <TPrecision precision>
void TSymbolTable::insertConstIvec3(ESymbolLevel level,
                                    const ImmutableString &name,
                                    const std::array<int, 3> &values)
{
    TVariable *constantIvec3 = new TVariable(
        this, name, StaticType::Get<EbtInt, precision, EvqConst, 3, 1>(), SymbolType::BuiltIn);

    TConstantUnion *unionArray = new TConstantUnion[3];
    for (size_t index = 0u; index < 3u; ++index)
    {
        unionArray[index].setIConst(values[index]);
    }
    constantIvec3->shareConstPointer(unionArray);

    insertBuiltIn(level, constantIvec3);
}

void TSymbolTable::setDefaultPrecision(TBasicType type, TPrecision prec)
{
    int indexOfLastElement = static_cast<int>(mPrecisionStack.size()) - 1;
    // Uses map operator [], overwrites the current value
    (*mPrecisionStack[indexOfLastElement])[type] = prec;
}

TPrecision TSymbolTable::getDefaultPrecision(TBasicType type) const
{
    if (!SupportsPrecision(type))
        return EbpUndefined;

    // unsigned integers use the same precision as signed
    TBasicType baseType = (type == EbtUInt) ? EbtInt : type;

    int level = static_cast<int>(mPrecisionStack.size()) - 1;
    ASSERT(level >= 0);  // Just to be safe. Should not happen.
    // If we dont find anything we return this. Some types don't have predefined default precision.
    TPrecision prec = EbpUndefined;
    while (level >= 0)
    {
        PrecisionStackLevel::iterator it = mPrecisionStack[level]->find(baseType);
        if (it != mPrecisionStack[level]->end())
        {
            prec = (*it).second;
            break;
        }
        level--;
    }
    return prec;
}

void TSymbolTable::addInvariantVarying(const ImmutableString &originalName)
{
    ASSERT(atGlobalLevel());
    mTable.back()->addInvariantVarying(originalName);
}

bool TSymbolTable::isVaryingInvariant(const ImmutableString &originalName) const
{
    ASSERT(atGlobalLevel());
    return mTable.back()->isVaryingInvariant(originalName);
}

void TSymbolTable::setGlobalInvariant(bool invariant)
{
    ASSERT(atGlobalLevel());
    mTable.back()->setGlobalInvariant(invariant);
}

void TSymbolTable::markBuiltInInitializationFinished()
{
    mUserDefinedUniqueIdsStart = mUniqueIdCounter;
}

void TSymbolTable::clearCompilationResults()
{
    mUniqueIdCounter = mUserDefinedUniqueIdsStart;

    // User-defined scopes should have already been cleared when the compilation finished.
    ASSERT(mTable.size() == 0u);
}

int TSymbolTable::nextUniqueIdValue()
{
    ASSERT(mUniqueIdCounter < std::numeric_limits<int>::max());
    return ++mUniqueIdCounter;
}

void TSymbolTable::initializeBuiltIns(sh::GLenum type,
                                      ShShaderSpec spec,
                                      const ShBuiltInResources &resources)
{
    mShaderType = type;

    ASSERT(isEmpty());
    pushBuiltInLevel();  // COMMON_BUILTINS
    pushBuiltInLevel();  // ESSL1_BUILTINS
    pushBuiltInLevel();  // ESSL3_BUILTINS
    pushBuiltInLevel();  // ESSL3_1_BUILTINS
    pushBuiltInLevel();  // GLSL_BUILTINS

    // We need just one precision stack level for predefined precisions.
    mPrecisionStack.push_back(std::unique_ptr<PrecisionStackLevel>(new PrecisionStackLevel));

    switch (type)
    {
        case GL_FRAGMENT_SHADER:
            setDefaultPrecision(EbtInt, EbpMedium);
            break;
        case GL_VERTEX_SHADER:
        case GL_COMPUTE_SHADER:
        case GL_GEOMETRY_SHADER_EXT:
            setDefaultPrecision(EbtInt, EbpHigh);
            setDefaultPrecision(EbtFloat, EbpHigh);
            break;
        default:
            UNREACHABLE();
    }
    // Set defaults for sampler types that have default precision, even those that are
    // only available if an extension exists.
    // New sampler types in ESSL3 don't have default precision. ESSL1 types do.
    initSamplerDefaultPrecision(EbtSampler2D);
    initSamplerDefaultPrecision(EbtSamplerCube);
    // SamplerExternalOES is specified in the extension to have default precision.
    initSamplerDefaultPrecision(EbtSamplerExternalOES);
    // SamplerExternal2DY2YEXT is specified in the extension to have default precision.
    initSamplerDefaultPrecision(EbtSamplerExternal2DY2YEXT);
    // It isn't specified whether Sampler2DRect has default precision.
    initSamplerDefaultPrecision(EbtSampler2DRect);

    setDefaultPrecision(EbtAtomicCounter, EbpHigh);

    insertBuiltInFunctions(type);
    mUniqueIdCounter = kLastStaticBuiltInId + 1;

    initializeBuiltInVariables(type, spec, resources);
    markBuiltInInitializationFinished();
}

void TSymbolTable::initSamplerDefaultPrecision(TBasicType samplerType)
{
    ASSERT(samplerType > EbtGuardSamplerBegin && samplerType < EbtGuardSamplerEnd);
    setDefaultPrecision(samplerType, EbpLow);
}


void TSymbolTable::initializeBuiltInVariables(sh::GLenum type,
                                              ShShaderSpec spec,
                                              const ShBuiltInResources &resources)
{
    const TSourceLoc zeroSourceLoc = {0, 0, 0, 0};

    //
    // Depth range in window coordinates
    //
    TFieldList *fields = new TFieldList();
    auto highpFloat1   = new TType(EbtFloat, EbpHigh, EvqGlobal, 1);
    TField *near       = new TField(highpFloat1, ImmutableString("near"), zeroSourceLoc);
    TField *far        = new TField(highpFloat1, ImmutableString("far"), zeroSourceLoc);
    TField *diff       = new TField(highpFloat1, ImmutableString("diff"), zeroSourceLoc);
    fields->push_back(near);
    fields->push_back(far);
    fields->push_back(diff);
    TStructure *depthRangeStruct = new TStructure(this, ImmutableString("gl_DepthRangeParameters"),
                                                  fields, SymbolType::BuiltIn);
    insertBuiltIn(COMMON_BUILTINS, depthRangeStruct);
    TType *depthRangeType = new TType(depthRangeStruct);
    depthRangeType->setQualifier(EvqUniform);
    depthRangeType->realize();
    insertVariable(COMMON_BUILTINS, ImmutableString("gl_DepthRange"), depthRangeType);

    //
    // Implementation dependent built-in constants.
    //
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxVertexAttribs"),
                              resources.MaxVertexAttribs);
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxVertexUniformVectors"),
                              resources.MaxVertexUniformVectors);
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxVertexTextureImageUnits"),
                              resources.MaxVertexTextureImageUnits);
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxCombinedTextureImageUnits"),
                              resources.MaxCombinedTextureImageUnits);
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxTextureImageUnits"),
                              resources.MaxTextureImageUnits);
    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxFragmentUniformVectors"),
                              resources.MaxFragmentUniformVectors);

    insertConstInt<EbpMedium>(ESSL1_BUILTINS, ImmutableString("gl_MaxVaryingVectors"),
                              resources.MaxVaryingVectors);

    insertConstInt<EbpMedium>(COMMON_BUILTINS, ImmutableString("gl_MaxDrawBuffers"),
                              resources.MaxDrawBuffers);
    insertConstIntExt<EbpMedium>(COMMON_BUILTINS, TExtension::EXT_blend_func_extended,
                                 ImmutableString("gl_MaxDualSourceDrawBuffersEXT"),
                                 resources.MaxDualSourceDrawBuffers);

    insertConstInt<EbpMedium>(ESSL3_BUILTINS, ImmutableString("gl_MaxVertexOutputVectors"),
                              resources.MaxVertexOutputVectors);
    insertConstInt<EbpMedium>(ESSL3_BUILTINS, ImmutableString("gl_MaxFragmentInputVectors"),
                              resources.MaxFragmentInputVectors);
    insertConstInt<EbpMedium>(ESSL3_BUILTINS, ImmutableString("gl_MinProgramTexelOffset"),
                              resources.MinProgramTexelOffset);
    insertConstInt<EbpMedium>(ESSL3_BUILTINS, ImmutableString("gl_MaxProgramTexelOffset"),
                              resources.MaxProgramTexelOffset);

    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxImageUnits"),
                              resources.MaxImageUnits);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxVertexImageUniforms"),
                              resources.MaxVertexImageUniforms);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxFragmentImageUniforms"),
                              resources.MaxFragmentImageUniforms);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeImageUniforms"),
                              resources.MaxComputeImageUniforms);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxCombinedImageUniforms"),
                              resources.MaxCombinedImageUniforms);

    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS,
                              ImmutableString("gl_MaxCombinedShaderOutputResources"),
                              resources.MaxCombinedShaderOutputResources);

    insertConstIvec3<EbpHigh>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeWorkGroupCount"),
                              resources.MaxComputeWorkGroupCount);
    insertConstIvec3<EbpHigh>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeWorkGroupSize"),
                              resources.MaxComputeWorkGroupSize);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeUniformComponents"),
                              resources.MaxComputeUniformComponents);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeTextureImageUnits"),
                              resources.MaxComputeTextureImageUnits);

    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxComputeAtomicCounters"),
                              resources.MaxComputeAtomicCounters);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS,
                              ImmutableString("gl_MaxComputeAtomicCounterBuffers"),
                              resources.MaxComputeAtomicCounterBuffers);

    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxVertexAtomicCounters"),
                              resources.MaxVertexAtomicCounters);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxFragmentAtomicCounters"),
                              resources.MaxFragmentAtomicCounters);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxCombinedAtomicCounters"),
                              resources.MaxCombinedAtomicCounters);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxAtomicCounterBindings"),
                              resources.MaxAtomicCounterBindings);

    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxVertexAtomicCounterBuffers"),
                              resources.MaxVertexAtomicCounterBuffers);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS,
                              ImmutableString("gl_MaxFragmentAtomicCounterBuffers"),
                              resources.MaxFragmentAtomicCounterBuffers);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS,
                              ImmutableString("gl_MaxCombinedAtomicCounterBuffers"),
                              resources.MaxCombinedAtomicCounterBuffers);
    insertConstInt<EbpMedium>(ESSL3_1_BUILTINS, ImmutableString("gl_MaxAtomicCounterBufferSize"),
                              resources.MaxAtomicCounterBufferSize);

    {
        TExtension ext = TExtension::EXT_geometry_shader;
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryInputComponents"),
                                     resources.MaxGeometryInputComponents);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryOutputComponents"),
                                     resources.MaxGeometryOutputComponents);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryImageUniforms"),
                                     resources.MaxGeometryImageUniforms);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryTextureImageUnits"),
                                     resources.MaxGeometryTextureImageUnits);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryOutputVertices"),
                                     resources.MaxGeometryOutputVertices);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryTotalOutputComponents"),
                                     resources.MaxGeometryTotalOutputComponents);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryUniformComponents"),
                                     resources.MaxGeometryUniformComponents);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryAtomicCounters"),
                                     resources.MaxGeometryAtomicCounters);
        insertConstIntExt<EbpMedium>(ESSL3_1_BUILTINS, ext,
                                     ImmutableString("gl_MaxGeometryAtomicCounterBuffers"),
                                     resources.MaxGeometryAtomicCounterBuffers);
    }

    //
    // Insert some special built-in variables that are not in
    // the built-in header files.
    //

    if (resources.OVR_multiview && type != GL_COMPUTE_SHADER)
    {
        const TType *viewIDType = StaticType::Get<EbtUInt, EbpHigh, EvqViewIDOVR, 1, 1>();
        insertVariableExt(ESSL3_BUILTINS, TExtension::OVR_multiview,
                          ImmutableString("gl_ViewID_OVR"), viewIDType);

        // ESSL 1.00 doesn't have unsigned integers, so gl_ViewID_OVR is a signed integer in ESSL
        // 1.00. This is specified in the WEBGL_multiview spec.
        const TType *viewIDIntType = StaticType::Get<EbtInt, EbpHigh, EvqViewIDOVR, 1, 1>();
        insertVariableExt(ESSL1_BUILTINS, TExtension::OVR_multiview,
                          ImmutableString("gl_ViewID_OVR"), viewIDIntType);
    }

    const TType *positionType    = StaticType::Get<EbtFloat, EbpHigh, EvqPosition, 4, 1>();
    const TType *primitiveIDType = StaticType::Get<EbtInt, EbpHigh, EvqPrimitiveID, 1, 1>();
    const TType *layerType       = StaticType::Get<EbtInt, EbpHigh, EvqLayer, 1, 1>();

    switch (type)
    {
        case GL_FRAGMENT_SHADER:
        {
            const TType *fragCoordType = StaticType::Get<EbtFloat, EbpMedium, EvqFragCoord, 4, 1>();
            insertVariable(COMMON_BUILTINS, ImmutableString("gl_FragCoord"), fragCoordType);
            const TType *frontFacingType = StaticType::GetQualified<EbtBool, EvqFrontFacing>();
            insertVariable(COMMON_BUILTINS, ImmutableString("gl_FrontFacing"), frontFacingType);
            const TType *pointCoordType =
                StaticType::Get<EbtFloat, EbpMedium, EvqPointCoord, 2, 1>();
            insertVariable(COMMON_BUILTINS, ImmutableString("gl_PointCoord"), pointCoordType);

            const TType *fragColorType = StaticType::Get<EbtFloat, EbpMedium, EvqFragColor, 4, 1>();
            insertVariable(ESSL1_BUILTINS, ImmutableString("gl_FragColor"), fragColorType);

            TType *fragDataType = new TType(EbtFloat, EbpMedium, EvqFragData, 4);
            if (spec != SH_WEBGL2_SPEC && spec != SH_WEBGL3_SPEC)
            {
                fragDataType->makeArray(resources.MaxDrawBuffers);
            }
            else
            {
                fragDataType->makeArray(1u);
            }
            fragDataType->realize();
            insertVariable(ESSL1_BUILTINS, ImmutableString("gl_FragData"), fragDataType);

            if (resources.EXT_blend_func_extended)
            {
                const TType *secondaryFragColorType =
                    StaticType::Get<EbtFloat, EbpMedium, EvqSecondaryFragColorEXT, 4, 1>();
                insertVariableExt(ESSL1_BUILTINS, TExtension::EXT_blend_func_extended,
                                  ImmutableString("gl_SecondaryFragColorEXT"),
                                  secondaryFragColorType);
                TType *secondaryFragDataType =
                    new TType(EbtFloat, EbpMedium, EvqSecondaryFragDataEXT, 4, 1);
                secondaryFragDataType->makeArray(resources.MaxDualSourceDrawBuffers);
                secondaryFragDataType->realize();
                insertVariableExt(ESSL1_BUILTINS, TExtension::EXT_blend_func_extended,
                                  ImmutableString("gl_SecondaryFragDataEXT"),
                                  secondaryFragDataType);
            }

            if (resources.EXT_frag_depth)
            {
                TType *fragDepthEXTType =
                    new TType(EbtFloat, resources.FragmentPrecisionHigh ? EbpHigh : EbpMedium,
                              EvqFragDepthEXT, 1);
                fragDepthEXTType->realize();
                insertVariableExt(ESSL1_BUILTINS, TExtension::EXT_frag_depth,
                                  ImmutableString("gl_FragDepthEXT"), fragDepthEXTType);
            }

            const TType *fragDepthType = StaticType::Get<EbtFloat, EbpHigh, EvqFragDepth, 1, 1>();
            insertVariable(ESSL3_BUILTINS, ImmutableString("gl_FragDepth"), fragDepthType);

            const TType *lastFragColorType =
                StaticType::Get<EbtFloat, EbpMedium, EvqLastFragColor, 4, 1>();

            if (resources.EXT_shader_framebuffer_fetch || resources.NV_shader_framebuffer_fetch)
            {
                TType *lastFragDataType = new TType(EbtFloat, EbpMedium, EvqLastFragData, 4, 1);
                lastFragDataType->makeArray(resources.MaxDrawBuffers);
                lastFragDataType->realize();

                if (resources.EXT_shader_framebuffer_fetch)
                {
                    insertVariableExt(ESSL1_BUILTINS, TExtension::EXT_shader_framebuffer_fetch,
                                      ImmutableString("gl_LastFragData"), lastFragDataType);
                }
                else if (resources.NV_shader_framebuffer_fetch)
                {
                    insertVariableExt(ESSL1_BUILTINS, TExtension::NV_shader_framebuffer_fetch,
                                      ImmutableString("gl_LastFragColor"), lastFragColorType);
                    insertVariableExt(ESSL1_BUILTINS, TExtension::NV_shader_framebuffer_fetch,
                                      ImmutableString("gl_LastFragData"), lastFragDataType);
                }
            }
            else if (resources.ARM_shader_framebuffer_fetch)
            {
                insertVariableExt(ESSL1_BUILTINS, TExtension::ARM_shader_framebuffer_fetch,
                                  ImmutableString("gl_LastFragColorARM"), lastFragColorType);
            }

            if (resources.EXT_geometry_shader)
            {
                TExtension extension = TExtension::EXT_geometry_shader;
                insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_PrimitiveID"),
                                  primitiveIDType);
                insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_Layer"),
                                  layerType);
            }

            break;
        }
        case GL_VERTEX_SHADER:
        {
            insertVariable(COMMON_BUILTINS, ImmutableString("gl_Position"), positionType);
            const TType *pointSizeType = StaticType::Get<EbtFloat, EbpMedium, EvqPointSize, 1, 1>();
            insertVariable(COMMON_BUILTINS, ImmutableString("gl_PointSize"), pointSizeType);
            const TType *instanceIDType = StaticType::Get<EbtInt, EbpHigh, EvqInstanceID, 1, 1>();
            insertVariable(ESSL3_BUILTINS, ImmutableString("gl_InstanceID"), instanceIDType);
            const TType *vertexIDType = StaticType::Get<EbtInt, EbpHigh, EvqVertexID, 1, 1>();
            insertVariable(ESSL3_BUILTINS, ImmutableString("gl_VertexID"), vertexIDType);

            // For internal use by ANGLE - not exposed to the parser.
            const TType *viewportIndexType =
                StaticType::Get<EbtInt, EbpHigh, EvqViewportIndex, 1, 1>();
            insertVariable(GLSL_BUILTINS, ImmutableString("gl_ViewportIndex"), viewportIndexType);
            // gl_Layer exists in other shader stages in ESSL, but not in vertex shader so far.
            insertVariable(GLSL_BUILTINS, ImmutableString("gl_Layer"), layerType);
            break;
        }
        case GL_COMPUTE_SHADER:
        {
            const TType *numWorkGroupsType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqNumWorkGroups, 3, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_NumWorkGroups"),
                           numWorkGroupsType);
            const TType *workGroupSizeType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqWorkGroupSize, 3, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_WorkGroupSize"),
                           workGroupSizeType);
            const TType *workGroupIDType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqWorkGroupID, 3, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_WorkGroupID"), workGroupIDType);
            const TType *localInvocationIDType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqLocalInvocationID, 3, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_LocalInvocationID"),
                           localInvocationIDType);
            const TType *globalInvocationIDType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqGlobalInvocationID, 3, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_GlobalInvocationID"),
                           globalInvocationIDType);
            const TType *localInvocationIndexType =
                StaticType::Get<EbtUInt, EbpUndefined, EvqLocalInvocationIndex, 1, 1>();
            insertVariable(ESSL3_1_BUILTINS, ImmutableString("gl_LocalInvocationIndex"),
                           localInvocationIndexType);
            break;
        }

        case GL_GEOMETRY_SHADER_EXT:
        {
            TExtension extension = TExtension::EXT_geometry_shader;

            // Add built-in interface block gl_PerVertex and the built-in array gl_in.
            // TODO(jiawei.shao@intel.com): implement GL_EXT_geometry_point_size.
            TFieldList *glPerVertexFieldList = new TFieldList();
            TField *glPositionField =
                new TField(new TType(*positionType), ImmutableString("gl_Position"), zeroSourceLoc);
            glPerVertexFieldList->push_back(glPositionField);

            const ImmutableString glPerVertexString("gl_PerVertex");
            TInterfaceBlock *glPerVertexInBlock =
                new TInterfaceBlock(this, glPerVertexString, glPerVertexFieldList,
                                    TLayoutQualifier::Create(), SymbolType::BuiltIn, extension);
            insertBuiltIn(ESSL3_1_BUILTINS, glPerVertexInBlock);

            // The array size of gl_in is undefined until we get a valid input primitive
            // declaration.
            TType *glInType =
                new TType(glPerVertexInBlock, EvqPerVertexIn, TLayoutQualifier::Create());
            glInType->makeArray(0u);
            glInType->realize();
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_in"), glInType);

            TInterfaceBlock *glPerVertexOutBlock =
                new TInterfaceBlock(this, glPerVertexString, glPerVertexFieldList,
                                    TLayoutQualifier::Create(), SymbolType::BuiltIn);
            TType *glPositionInBlockType = new TType(EbtFloat, EbpHigh, EvqPosition, 4);
            glPositionInBlockType->setInterfaceBlock(glPerVertexOutBlock);
            glPositionInBlockType->realize();
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_Position"),
                              glPositionInBlockType);

            const TType *primitiveIDInType =
                StaticType::Get<EbtInt, EbpHigh, EvqPrimitiveIDIn, 1, 1>();
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_PrimitiveIDIn"),
                              primitiveIDInType);
            const TType *invocationIDType =
                StaticType::Get<EbtInt, EbpHigh, EvqInvocationID, 1, 1>();
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_InvocationID"),
                              invocationIDType);
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_PrimitiveID"),
                              primitiveIDType);
            insertVariableExt(ESSL3_1_BUILTINS, extension, ImmutableString("gl_Layer"), layerType);

            break;
        }
        default:
            UNREACHABLE();
    }
}

}  // namespace sh

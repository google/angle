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

#include "angle_gl.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/IntermNode.h"

#include <stdio.h>
#include <algorithm>

namespace sh
{

class TSymbolTable::TSymbolTableLevel
{
  public:
    TSymbolTableLevel() : mGlobalInvariant(false) {}

    bool insert(TSymbol *symbol);

    // Insert a function using its unmangled name as the key.
    bool insertUnmangled(TFunction *function);

    TSymbol *find(const ImmutableString &name) const;

    void addInvariantVarying(const std::string &name) { mInvariantVaryings.insert(name); }

    bool isVaryingInvariant(const std::string &name)
    {
        return (mGlobalInvariant || mInvariantVaryings.count(name) > 0);
    }

    void setGlobalInvariant(bool invariant) { mGlobalInvariant = invariant; }

    void insertUnmangledBuiltInName(const char *name);
    bool hasUnmangledBuiltIn(const char *name) const;

  private:
    using tLevel        = TUnorderedMap<ImmutableString,
                                 TSymbol *,
                                 ImmutableString::FowlerNollVoHash<sizeof(size_t)>>;
    using tLevelPair    = const tLevel::value_type;
    using tInsertResult = std::pair<tLevel::iterator, bool>;

    tLevel level;
    std::set<std::string> mInvariantVaryings;
    bool mGlobalInvariant;

    std::set<ImmutableString> mUnmangledBuiltInNames;
};

bool TSymbolTable::TSymbolTableLevel::insert(TSymbol *symbol)
{
    // returning true means symbol was added to the table
    tInsertResult result = level.insert(tLevelPair(symbol->getMangledName(), symbol));

    return result.second;
}

bool TSymbolTable::TSymbolTableLevel::insertUnmangled(TFunction *function)
{
    // returning true means symbol was added to the table
    tInsertResult result = level.insert(tLevelPair(function->name(), function));

    return result.second;
}

TSymbol *TSymbolTable::TSymbolTableLevel::find(const ImmutableString &name) const
{
    tLevel::const_iterator it = level.find(name);
    if (it == level.end())
        return 0;
    else
        return (*it).second;
}

void TSymbolTable::TSymbolTableLevel::insertUnmangledBuiltInName(const char *name)
{
    mUnmangledBuiltInNames.insert(ImmutableString(name));
}

bool TSymbolTable::TSymbolTableLevel::hasUnmangledBuiltIn(const char *name) const
{
    return mUnmangledBuiltInNames.count(ImmutableString(name)) > 0;
}

void TSymbolTable::push()
{
    table.push_back(new TSymbolTableLevel);
    precisionStack.push_back(new PrecisionStackLevel);
}

void TSymbolTable::pop()
{
    delete table.back();
    table.pop_back();

    delete precisionStack.back();
    precisionStack.pop_back();
}

const TFunction *TSymbolTable::markUserDefinedFunctionHasPrototypeDeclaration(
    const ImmutableString &mangledName,
    bool *hadPrototypeDeclarationOut)
{
    TFunction *function         = findUserDefinedFunction(mangledName);
    *hadPrototypeDeclarationOut = function->hasPrototypeDeclaration();
    function->setHasPrototypeDeclaration();
    return function;
}

const TFunction *TSymbolTable::setUserDefinedFunctionParameterNamesFromDefinition(
    const TFunction *function,
    bool *wasDefinedOut)
{
    TFunction *firstDeclaration = findUserDefinedFunction(function->getMangledName());
    ASSERT(firstDeclaration);
    // Note: 'firstDeclaration' could be 'function' if this is the first time we've seen function as
    // it would have just been put in the symbol table. Otherwise, we're looking up an earlier
    // occurance.
    if (function != firstDeclaration)
    {
        // Swap the parameters of the previous declaration to the parameters of the function
        // definition (parameter names may differ).
        firstDeclaration->swapParameters(*function);
    }

    *wasDefinedOut = firstDeclaration->isDefined();
    firstDeclaration->setDefined();
    return firstDeclaration;
}

const TSymbol *TSymbolTable::find(const ImmutableString &name, int shaderVersion) const
{
    int level       = currentLevel();
    TSymbol *symbol = nullptr;
    do
    {
        if (level == GLSL_BUILTINS)
            level--;
        if (level == ESSL3_1_BUILTINS && shaderVersion != 310)
            level--;
        if (level == ESSL3_BUILTINS && shaderVersion < 300)
            level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100)
            level--;

        symbol = table[level]->find(name);
        level--;
    } while (symbol == nullptr && level >= 0);

    return symbol;
}

TFunction *TSymbolTable::findUserDefinedFunction(const ImmutableString &name) const
{
    // User-defined functions are always declared at the global level.
    ASSERT(currentLevel() >= GLOBAL_LEVEL);
    return static_cast<TFunction *>(table[GLOBAL_LEVEL]->find(name));
}

const TSymbol *TSymbolTable::findGlobal(const ImmutableString &name) const
{
    ASSERT(table.size() > GLOBAL_LEVEL);
    return table[GLOBAL_LEVEL]->find(name);
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

        TSymbol *symbol = table[level]->find(name);

        if (symbol)
            return symbol;
    }

    return nullptr;
}

TSymbolTable::~TSymbolTable()
{
    while (table.size() > 0)
        pop();
}

constexpr bool IsGenType(const TType *type)
{
    if (type)
    {
        TBasicType basicType = type->getBasicType();
        return basicType == EbtGenType || basicType == EbtGenIType || basicType == EbtGenUType ||
               basicType == EbtGenBType;
    }

    return false;
}

constexpr bool IsVecType(const TType *type)
{
    if (type)
    {
        TBasicType basicType = type->getBasicType();
        return basicType == EbtVec || basicType == EbtIVec || basicType == EbtUVec ||
               basicType == EbtBVec;
    }

    return false;
}

constexpr const TType *SpecificType(const TType *type, int size)
{
    ASSERT(size >= 1 && size <= 4);

    if (!type)
    {
        return nullptr;
    }

    ASSERT(!IsVecType(type));

    switch (type->getBasicType())
    {
        case EbtGenType:
            return StaticType::GetForVec<EbtFloat>(type->getQualifier(),
                                                            static_cast<unsigned char>(size));
        case EbtGenIType:
            return StaticType::GetForVec<EbtInt>(type->getQualifier(),
                                                          static_cast<unsigned char>(size));
        case EbtGenUType:
            return StaticType::GetForVec<EbtUInt>(type->getQualifier(),
                                                           static_cast<unsigned char>(size));
        case EbtGenBType:
            return StaticType::GetForVec<EbtBool>(type->getQualifier(),
                                                           static_cast<unsigned char>(size));
        default:
            return type;
    }
}

constexpr const TType *VectorType(const TType *type, int size)
{
    ASSERT(size >= 2 && size <= 4);

    if (!type)
    {
        return nullptr;
    }

    ASSERT(!IsGenType(type));

    switch (type->getBasicType())
    {
        case EbtVec:
            return StaticType::GetForVecMat<EbtFloat>(static_cast<unsigned char>(size));
        case EbtIVec:
            return StaticType::GetForVecMat<EbtInt>(static_cast<unsigned char>(size));
        case EbtUVec:
            return StaticType::GetForVecMat<EbtUInt>(static_cast<unsigned char>(size));
        case EbtBVec:
            return StaticType::GetForVecMat<EbtBool>(static_cast<unsigned char>(size));
        default:
            return type;
    }
}

bool TSymbolTable::declareVariable(TVariable *variable)
{
    ASSERT(variable->symbolType() == SymbolType::UserDefined);
    return insertVariable(currentLevel(), variable);
}

bool TSymbolTable::declareStructType(TStructure *str)
{
    return insertStructType(currentLevel(), str);
}

bool TSymbolTable::declareInterfaceBlock(TInterfaceBlock *interfaceBlock)
{
    return insert(currentLevel(), interfaceBlock);
}

void TSymbolTable::declareUserDefinedFunction(TFunction *function, bool insertUnmangledName)
{
    ASSERT(currentLevel() >= GLOBAL_LEVEL);
    if (insertUnmangledName)
    {
        // Insert the unmangled name to detect potential future redefinition as a variable.
        table[GLOBAL_LEVEL]->insertUnmangled(function);
    }
    table[GLOBAL_LEVEL]->insert(function);
}

TVariable *TSymbolTable::insertVariable(ESymbolLevel level,
                                        const ImmutableString &name,
                                        const TType *type)
{
    ASSERT(level <= LAST_BUILTIN_LEVEL);
    ASSERT(type->isRealized());
    return insertVariable(level, name, type, SymbolType::BuiltIn);
}

TVariable *TSymbolTable::insertVariable(ESymbolLevel level,
                                        const ImmutableString &name,
                                        const TType *type,
                                        SymbolType symbolType)
{
    ASSERT(level > LAST_BUILTIN_LEVEL || type->isRealized());
    TVariable *var = new TVariable(this, name, type, symbolType);
    if (insert(level, var))
    {
        return var;
    }
    return nullptr;
}

TVariable *TSymbolTable::insertVariableExt(ESymbolLevel level,
                                           TExtension ext,
                                           const ImmutableString &name,
                                           const TType *type)
{
    ASSERT(level <= LAST_BUILTIN_LEVEL);
    ASSERT(type->isRealized());
    TVariable *var = new TVariable(this, name, type, SymbolType::BuiltIn, ext);
    if (insert(level, var))
    {
        return var;
    }
    return nullptr;
}

bool TSymbolTable::insertVariable(ESymbolLevel level, TVariable *variable)
{
    ASSERT(variable);
    ASSERT(level > LAST_BUILTIN_LEVEL || variable->getType().isRealized());
    return insert(level, variable);
}

bool TSymbolTable::insert(ESymbolLevel level, TSymbol *symbol)
{
    ASSERT(level > LAST_BUILTIN_LEVEL || mUserDefinedUniqueIdsStart == -1);
    return table[level]->insert(symbol);
}

bool TSymbolTable::insertStructType(ESymbolLevel level, TStructure *str)
{
    ASSERT(str);
    return insert(level, str);
}

bool TSymbolTable::insertInterfaceBlock(ESymbolLevel level, TInterfaceBlock *interfaceBlock)
{
    ASSERT(interfaceBlock);
    return insert(level, interfaceBlock);
}

template <TPrecision precision>
bool TSymbolTable::insertConstInt(ESymbolLevel level, const ImmutableString &name, int value)
{
    TVariable *constant = new TVariable(
        this, name, StaticType::Get<EbtInt, precision, EvqConst, 1, 1>(), SymbolType::BuiltIn);
    TConstantUnion *unionArray = new TConstantUnion[1];
    unionArray[0].setIConst(value);
    constant->shareConstPointer(unionArray);
    return insert(level, constant);
}

template <TPrecision precision>
bool TSymbolTable::insertConstIntExt(ESymbolLevel level,
                                     TExtension ext,
                                     const ImmutableString &name,
                                     int value)
{
    TVariable *constant = new TVariable(
        this, name, StaticType::Get<EbtInt, precision, EvqConst, 1, 1>(), SymbolType::BuiltIn, ext);
    TConstantUnion *unionArray = new TConstantUnion[1];
    unionArray[0].setIConst(value);
    constant->shareConstPointer(unionArray);
    return insert(level, constant);
}

template <TPrecision precision>
bool TSymbolTable::insertConstIvec3(ESymbolLevel level,
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

    return insert(level, constantIvec3);
}

void TSymbolTable::insertBuiltIn(ESymbolLevel level,
                                 TOperator op,
                                 TExtension ext,
                                 const TType *rvalue,
                                 const char *name,
                                 const TType *ptype1,
                                 const TType *ptype2,
                                 const TType *ptype3,
                                 const TType *ptype4,
                                 const TType *ptype5)
{
    if (ptype1->getBasicType() == EbtGSampler2D)
    {
        insertUnmangledBuiltInName(name, level);
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtFloat, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtSampler2D>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtISampler2D>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtUInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtUSampler2D>(), ptype2, ptype3, ptype4, ptype5);
    }
    else if (ptype1->getBasicType() == EbtGSampler3D)
    {
        insertUnmangledBuiltInName(name, level);
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtFloat, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtSampler3D>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtISampler3D>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtUInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtUSampler3D>(), ptype2, ptype3, ptype4, ptype5);
    }
    else if (ptype1->getBasicType() == EbtGSamplerCube)
    {
        insertUnmangledBuiltInName(name, level);
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtFloat, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtSamplerCube>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtISamplerCube>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtUInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtUSamplerCube>(), ptype2, ptype3, ptype4, ptype5);
    }
    else if (ptype1->getBasicType() == EbtGSampler2DArray)
    {
        insertUnmangledBuiltInName(name, level);
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtFloat, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtSampler2DArray>(), ptype2, ptype3, ptype4,
                      ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtISampler2DArray>(), ptype2, ptype3, ptype4,
                      ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtUInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtUSampler2DArray>(), ptype2, ptype3, ptype4,
                      ptype5);
    }
    else if (ptype1->getBasicType() == EbtGSampler2DMS)
    {
        insertUnmangledBuiltInName(name, level);
        bool gvec4 = (rvalue->getBasicType() == EbtGVec4);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtFloat, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtSampler2DMS>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtISampler2DMS>(), ptype2, ptype3, ptype4, ptype5);
        insertBuiltIn(level, gvec4 ? StaticType::GetBasic<EbtUInt, 4>() : rvalue, name,
                      StaticType::GetBasic<EbtUSampler2DMS>(), ptype2, ptype3, ptype4, ptype5);
    }
    else if (IsGImage(ptype1->getBasicType()))
    {
        insertUnmangledBuiltInName(name, level);

        const TType *floatType    = StaticType::GetBasic<EbtFloat, 4>();
        const TType *intType      = StaticType::GetBasic<EbtInt, 4>();
        const TType *unsignedType = StaticType::GetBasic<EbtUInt, 4>();

        const TType *floatImage    = StaticType::GetForFloatImage(ptype1->getBasicType());
        const TType *intImage      = StaticType::GetForIntImage(ptype1->getBasicType());
        const TType *unsignedImage = StaticType::GetForUintImage(ptype1->getBasicType());

        // GLSL ES 3.10, Revision 4, 8.12 Image Functions
        if (rvalue->getBasicType() == EbtGVec4)
        {
            // imageLoad
            insertBuiltIn(level, floatType, name, floatImage, ptype2, ptype3, ptype4, ptype5);
            insertBuiltIn(level, intType, name, intImage, ptype2, ptype3, ptype4, ptype5);
            insertBuiltIn(level, unsignedType, name, unsignedImage, ptype2, ptype3, ptype4, ptype5);
        }
        else if (rvalue->getBasicType() == EbtVoid)
        {
            // imageStore
            insertBuiltIn(level, rvalue, name, floatImage, ptype2, floatType, ptype4, ptype5);
            insertBuiltIn(level, rvalue, name, intImage, ptype2, intType, ptype4, ptype5);
            insertBuiltIn(level, rvalue, name, unsignedImage, ptype2, unsignedType, ptype4, ptype5);
        }
        else
        {
            // imageSize
            insertBuiltIn(level, rvalue, name, floatImage, ptype2, ptype3, ptype4, ptype5);
            insertBuiltIn(level, rvalue, name, intImage, ptype2, ptype3, ptype4, ptype5);
            insertBuiltIn(level, rvalue, name, unsignedImage, ptype2, ptype3, ptype4, ptype5);
        }
    }
    else if (IsGenType(rvalue) || IsGenType(ptype1) || IsGenType(ptype2) || IsGenType(ptype3) ||
             IsGenType(ptype4))
    {
        ASSERT(!ptype5);
        insertUnmangledBuiltInName(name, level);
        insertBuiltIn(level, op, ext, SpecificType(rvalue, 1), name, SpecificType(ptype1, 1),
                      SpecificType(ptype2, 1), SpecificType(ptype3, 1), SpecificType(ptype4, 1));
        insertBuiltIn(level, op, ext, SpecificType(rvalue, 2), name, SpecificType(ptype1, 2),
                      SpecificType(ptype2, 2), SpecificType(ptype3, 2), SpecificType(ptype4, 2));
        insertBuiltIn(level, op, ext, SpecificType(rvalue, 3), name, SpecificType(ptype1, 3),
                      SpecificType(ptype2, 3), SpecificType(ptype3, 3), SpecificType(ptype4, 3));
        insertBuiltIn(level, op, ext, SpecificType(rvalue, 4), name, SpecificType(ptype1, 4),
                      SpecificType(ptype2, 4), SpecificType(ptype3, 4), SpecificType(ptype4, 4));
    }
    else if (IsVecType(rvalue) || IsVecType(ptype1) || IsVecType(ptype2) || IsVecType(ptype3))
    {
        ASSERT(!ptype4 && !ptype5);
        insertUnmangledBuiltInName(name, level);
        insertBuiltIn(level, op, ext, VectorType(rvalue, 2), name, VectorType(ptype1, 2),
                      VectorType(ptype2, 2), VectorType(ptype3, 2));
        insertBuiltIn(level, op, ext, VectorType(rvalue, 3), name, VectorType(ptype1, 3),
                      VectorType(ptype2, 3), VectorType(ptype3, 3));
        insertBuiltIn(level, op, ext, VectorType(rvalue, 4), name, VectorType(ptype1, 4),
                      VectorType(ptype2, 4), VectorType(ptype3, 4));
    }
    else
    {
        TFunction *function =
            new TFunction(this, ImmutableString(name), rvalue, SymbolType::BuiltIn, false, op, ext);

        function->addParameter(TConstParameter(ptype1));

        if (ptype2)
        {
            function->addParameter(TConstParameter(ptype2));
        }

        if (ptype3)
        {
            function->addParameter(TConstParameter(ptype3));
        }

        if (ptype4)
        {
            function->addParameter(TConstParameter(ptype4));
        }

        if (ptype5)
        {
            function->addParameter(TConstParameter(ptype5));
        }

        ASSERT(hasUnmangledBuiltInAtLevel(name, level));
        insert(level, function);
    }
}

void TSymbolTable::insertBuiltInOp(ESymbolLevel level,
                                   TOperator op,
                                   const TType *rvalue,
                                   const TType *ptype1,
                                   const TType *ptype2,
                                   const TType *ptype3,
                                   const TType *ptype4,
                                   const TType *ptype5)
{
    const char *name = GetOperatorString(op);
    ASSERT(strlen(name) > 0);
    insertUnmangledBuiltInName(name, level);
    insertBuiltIn(level, op, TExtension::UNDEFINED, rvalue, name, ptype1, ptype2, ptype3, ptype4,
                  ptype5);
}

void TSymbolTable::insertBuiltInOp(ESymbolLevel level,
                                   TOperator op,
                                   TExtension ext,
                                   const TType *rvalue,
                                   const TType *ptype1,
                                   const TType *ptype2,
                                   const TType *ptype3,
                                   const TType *ptype4,
                                   const TType *ptype5)
{
    const char *name = GetOperatorString(op);
    insertUnmangledBuiltInName(name, level);
    insertBuiltIn(level, op, ext, rvalue, name, ptype1, ptype2, ptype3, ptype4, ptype5);
}

void TSymbolTable::insertBuiltInFunctionNoParameters(ESymbolLevel level,
                                                     TOperator op,
                                                     const TType *rvalue,
                                                     const char *name)
{
    insertUnmangledBuiltInName(name, level);
    insert(level,
           new TFunction(this, ImmutableString(name), rvalue, SymbolType::BuiltIn, false, op));
}

void TSymbolTable::insertBuiltInFunctionNoParametersExt(ESymbolLevel level,
                                                        TExtension ext,
                                                        TOperator op,
                                                        const TType *rvalue,
                                                        const char *name)
{
    insertUnmangledBuiltInName(name, level);
    insert(level,
           new TFunction(this, ImmutableString(name), rvalue, SymbolType::BuiltIn, false, op, ext));
}

TPrecision TSymbolTable::getDefaultPrecision(TBasicType type) const
{
    if (!SupportsPrecision(type))
        return EbpUndefined;

    // unsigned integers use the same precision as signed
    TBasicType baseType = (type == EbtUInt) ? EbtInt : type;

    int level = static_cast<int>(precisionStack.size()) - 1;
    assert(level >= 0);  // Just to be safe. Should not happen.
    // If we dont find anything we return this. Some types don't have predefined default precision.
    TPrecision prec = EbpUndefined;
    while (level >= 0)
    {
        PrecisionStackLevel::iterator it = precisionStack[level]->find(baseType);
        if (it != precisionStack[level]->end())
        {
            prec = (*it).second;
            break;
        }
        level--;
    }
    return prec;
}

void TSymbolTable::addInvariantVarying(const std::string &originalName)
{
    ASSERT(atGlobalLevel());
    table[currentLevel()]->addInvariantVarying(originalName);
}

bool TSymbolTable::isVaryingInvariant(const std::string &originalName) const
{
    ASSERT(atGlobalLevel());
    return table[currentLevel()]->isVaryingInvariant(originalName);
}

void TSymbolTable::setGlobalInvariant(bool invariant)
{
    ASSERT(atGlobalLevel());
    table[currentLevel()]->setGlobalInvariant(invariant);
}

void TSymbolTable::insertUnmangledBuiltInName(const char *name, ESymbolLevel level)
{
    ASSERT(level >= 0 && level < static_cast<ESymbolLevel>(table.size()));
    ASSERT(mUserDefinedUniqueIdsStart == -1);
    table[level]->insertUnmangledBuiltInName(name);
}

bool TSymbolTable::hasUnmangledBuiltInAtLevel(const char *name, ESymbolLevel level)
{
    ASSERT(level >= 0 && level < static_cast<ESymbolLevel>(table.size()));
    return table[level]->hasUnmangledBuiltIn(name);
}

bool TSymbolTable::hasUnmangledBuiltInForShaderVersion(const char *name, int shaderVersion)
{
    ASSERT(static_cast<ESymbolLevel>(table.size()) > LAST_BUILTIN_LEVEL);

    for (int level = LAST_BUILTIN_LEVEL; level >= 0; --level)
    {
        if (level == ESSL3_1_BUILTINS && shaderVersion != 310)
        {
            --level;
        }
        if (level == ESSL3_BUILTINS && shaderVersion < 300)
        {
            --level;
        }
        if (level == ESSL1_BUILTINS && shaderVersion != 100)
        {
            --level;
        }

        if (table[level]->hasUnmangledBuiltIn(name))
        {
            return true;
        }
    }
    return false;
}

void TSymbolTable::markBuiltInInitializationFinished()
{
    mUserDefinedUniqueIdsStart = mUniqueIdCounter;
}

void TSymbolTable::clearCompilationResults()
{
    mUniqueIdCounter = mUserDefinedUniqueIdsStart;

    // User-defined scopes should have already been cleared when the compilation finished.
    ASSERT(table.size() == LAST_BUILTIN_LEVEL + 1u);
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
    ASSERT(isEmpty());
    push();  // COMMON_BUILTINS
    push();  // ESSL1_BUILTINS
    push();  // ESSL3_BUILTINS
    push();  // ESSL3_1_BUILTINS
    push();  // GLSL_BUILTINS

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

    initializeBuiltInFunctions(type, spec, resources);
    initializeBuiltInVariables(type, spec, resources);
    markBuiltInInitializationFinished();
}

void TSymbolTable::initSamplerDefaultPrecision(TBasicType samplerType)
{
    ASSERT(samplerType > EbtGuardSamplerBegin && samplerType < EbtGuardSamplerEnd);
    setDefaultPrecision(samplerType, EbpLow);
}

void TSymbolTable::initializeBuiltInFunctions(sh::GLenum type,
                                              ShShaderSpec spec,
                                              const ShBuiltInResources &resources)
{
    const TType *voidType = StaticType::GetBasic<EbtVoid>();
    const TType *float1   = StaticType::GetBasic<EbtFloat>();
    const TType *float2   = StaticType::GetBasic<EbtFloat, 2>();
    const TType *float3   = StaticType::GetBasic<EbtFloat, 3>();
    const TType *float4   = StaticType::GetBasic<EbtFloat, 4>();
    const TType *int1     = StaticType::GetBasic<EbtInt>();
    const TType *int2     = StaticType::GetBasic<EbtInt, 2>();
    const TType *int3     = StaticType::GetBasic<EbtInt, 3>();
    const TType *uint1    = StaticType::GetBasic<EbtUInt>();
    const TType *bool1    = StaticType::GetBasic<EbtBool>();
    const TType *genType  = StaticType::GetBasic<EbtGenType>();
    const TType *genIType = StaticType::GetBasic<EbtGenIType>();
    const TType *genUType = StaticType::GetBasic<EbtGenUType>();
    const TType *genBType = StaticType::GetBasic<EbtGenBType>();

    //
    // Angle and Trigonometric Functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpRadians, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpDegrees, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpSin, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpCos, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpTan, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpAsin, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpAcos, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpAtan, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpAtan, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpSinh, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpCosh, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTanh, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpAsinh, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpAcosh, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpAtanh, genType, genType);

    //
    // Exponential Functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpPow, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpExp, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpLog, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpExp2, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpLog2, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpSqrt, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpInverseSqrt, genType, genType);

    //
    // Common Functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpAbs, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpAbs, genIType, genIType);
    insertBuiltInOp(COMMON_BUILTINS, EOpSign, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpSign, genIType, genIType);
    insertBuiltInOp(COMMON_BUILTINS, EOpFloor, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTrunc, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpRound, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpRoundEven, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpCeil, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpFract, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpMod, genType, genType, float1);
    insertBuiltInOp(COMMON_BUILTINS, EOpMod, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpMin, genType, genType, float1);
    insertBuiltInOp(COMMON_BUILTINS, EOpMin, genType, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMin, genIType, genIType, genIType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMin, genIType, genIType, int1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMin, genUType, genUType, genUType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMin, genUType, genUType, uint1);
    insertBuiltInOp(COMMON_BUILTINS, EOpMax, genType, genType, float1);
    insertBuiltInOp(COMMON_BUILTINS, EOpMax, genType, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMax, genIType, genIType, genIType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMax, genIType, genIType, int1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMax, genUType, genUType, genUType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMax, genUType, genUType, uint1);
    insertBuiltInOp(COMMON_BUILTINS, EOpClamp, genType, genType, float1, float1);
    insertBuiltInOp(COMMON_BUILTINS, EOpClamp, genType, genType, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpClamp, genIType, genIType, int1, int1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpClamp, genIType, genIType, genIType, genIType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpClamp, genUType, genUType, uint1, uint1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpClamp, genUType, genUType, genUType, genUType);
    insertBuiltInOp(COMMON_BUILTINS, EOpMix, genType, genType, genType, float1);
    insertBuiltInOp(COMMON_BUILTINS, EOpMix, genType, genType, genType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMix, genType, genType, genType, genBType);
    insertBuiltInOp(COMMON_BUILTINS, EOpStep, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpStep, genType, float1, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpSmoothStep, genType, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpSmoothStep, genType, float1, float1, genType);

    const TType *outGenType  = StaticType::GetQualified<EbtGenType, EvqOut>();
    const TType *outGenIType = StaticType::GetQualified<EbtGenIType, EvqOut>();

    insertBuiltInOp(ESSL3_BUILTINS, EOpModf, genType, genType, outGenType);

    insertBuiltInOp(ESSL3_BUILTINS, EOpIsNan, genBType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpIsInf, genBType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpFloatBitsToInt, genIType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpFloatBitsToUint, genUType, genType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpIntBitsToFloat, genType, genIType);
    insertBuiltInOp(ESSL3_BUILTINS, EOpUintBitsToFloat, genType, genUType);

    insertBuiltInOp(ESSL3_1_BUILTINS, EOpFrexp, genType, genType, outGenIType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpLdexp, genType, genType, genIType);

    insertBuiltInOp(ESSL3_BUILTINS, EOpPackSnorm2x16, uint1, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpPackUnorm2x16, uint1, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpPackHalf2x16, uint1, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpUnpackSnorm2x16, float2, uint1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpUnpackUnorm2x16, float2, uint1);
    insertBuiltInOp(ESSL3_BUILTINS, EOpUnpackHalf2x16, float2, uint1);

    insertBuiltInOp(ESSL3_1_BUILTINS, EOpPackUnorm4x8, uint1, float4);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpPackSnorm4x8, uint1, float4);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpUnpackUnorm4x8, float4, uint1);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpUnpackSnorm4x8, float4, uint1);

    //
    // Geometric Functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpLength, float1, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpDistance, float1, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpDot, float1, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpCross, float3, float3, float3);
    insertBuiltInOp(COMMON_BUILTINS, EOpNormalize, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpFaceforward, genType, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpReflect, genType, genType, genType);
    insertBuiltInOp(COMMON_BUILTINS, EOpRefract, genType, genType, genType, float1);

    const TType *mat2   = StaticType::GetBasic<EbtFloat, 2, 2>();
    const TType *mat3   = StaticType::GetBasic<EbtFloat, 3, 3>();
    const TType *mat4   = StaticType::GetBasic<EbtFloat, 4, 4>();
    const TType *mat2x3 = StaticType::GetBasic<EbtFloat, 2, 3>();
    const TType *mat3x2 = StaticType::GetBasic<EbtFloat, 3, 2>();
    const TType *mat2x4 = StaticType::GetBasic<EbtFloat, 2, 4>();
    const TType *mat4x2 = StaticType::GetBasic<EbtFloat, 4, 2>();
    const TType *mat3x4 = StaticType::GetBasic<EbtFloat, 3, 4>();
    const TType *mat4x3 = StaticType::GetBasic<EbtFloat, 4, 3>();

    //
    // Matrix Functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpMulMatrixComponentWise, mat2, mat2, mat2);
    insertBuiltInOp(COMMON_BUILTINS, EOpMulMatrixComponentWise, mat3, mat3, mat3);
    insertBuiltInOp(COMMON_BUILTINS, EOpMulMatrixComponentWise, mat4, mat4, mat4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat2x3, mat2x3, mat2x3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat3x2, mat3x2, mat3x2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat2x4, mat2x4, mat2x4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat4x2, mat4x2, mat4x2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat3x4, mat3x4, mat3x4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpMulMatrixComponentWise, mat4x3, mat4x3, mat4x3);

    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat2, float2, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat3, float3, float3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat4, float4, float4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat2x3, float3, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat3x2, float2, float3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat2x4, float4, float2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat4x2, float2, float4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat3x4, float4, float3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpOuterProduct, mat4x3, float3, float4);

    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat2, mat2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat3, mat3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat4, mat4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat2x3, mat3x2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat3x2, mat2x3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat2x4, mat4x2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat4x2, mat2x4);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat3x4, mat4x3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpTranspose, mat4x3, mat3x4);

    insertBuiltInOp(ESSL3_BUILTINS, EOpDeterminant, float1, mat2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpDeterminant, float1, mat3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpDeterminant, float1, mat4);

    insertBuiltInOp(ESSL3_BUILTINS, EOpInverse, mat2, mat2);
    insertBuiltInOp(ESSL3_BUILTINS, EOpInverse, mat3, mat3);
    insertBuiltInOp(ESSL3_BUILTINS, EOpInverse, mat4, mat4);

    const TType *vec  = StaticType::GetBasic<EbtVec>();
    const TType *ivec = StaticType::GetBasic<EbtIVec>();
    const TType *uvec = StaticType::GetBasic<EbtUVec>();
    const TType *bvec = StaticType::GetBasic<EbtBVec>();

    //
    // Vector relational functions.
    //
    insertBuiltInOp(COMMON_BUILTINS, EOpLessThanComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpLessThanComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpLessThanComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpLessThanEqualComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpLessThanEqualComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpLessThanEqualComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpGreaterThanComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpGreaterThanComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpGreaterThanComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpGreaterThanEqualComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpGreaterThanEqualComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpGreaterThanEqualComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpEqualComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpEqualComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpEqualComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpEqualComponentWise, bvec, bvec, bvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpNotEqualComponentWise, bvec, vec, vec);
    insertBuiltInOp(COMMON_BUILTINS, EOpNotEqualComponentWise, bvec, ivec, ivec);
    insertBuiltInOp(ESSL3_BUILTINS, EOpNotEqualComponentWise, bvec, uvec, uvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpNotEqualComponentWise, bvec, bvec, bvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpAny, bool1, bvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpAll, bool1, bvec);
    insertBuiltInOp(COMMON_BUILTINS, EOpLogicalNotComponentWise, bvec, bvec);

    //
    // Integer functions
    //
    const TType *outGenUType = StaticType::GetQualified<EbtGenUType, EvqOut>();

    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldExtract, genIType, genIType, int1, int1);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldExtract, genUType, genUType, int1, int1);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldInsert, genIType, genIType, genIType, int1, int1);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldInsert, genUType, genUType, genUType, int1, int1);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldReverse, genIType, genIType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitfieldReverse, genUType, genUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitCount, genIType, genIType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpBitCount, genIType, genUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpFindLSB, genIType, genIType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpFindLSB, genIType, genUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpFindMSB, genIType, genIType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpFindMSB, genIType, genUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpUaddCarry, genUType, genUType, genUType, outGenUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpUsubBorrow, genUType, genUType, genUType, outGenUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpUmulExtended, voidType, genUType, genUType, outGenUType,
                    outGenUType);
    insertBuiltInOp(ESSL3_1_BUILTINS, EOpImulExtended, voidType, genIType, genIType, outGenIType,
                    outGenIType);

    const TType *sampler2D   = StaticType::GetBasic<EbtSampler2D>();
    const TType *samplerCube = StaticType::GetBasic<EbtSamplerCube>();

    //
    // Texture Functions for GLSL ES 1.0
    //
    insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", sampler2D, float2);
    insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float3);
    insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float4);
    insertBuiltIn(ESSL1_BUILTINS, float4, "textureCube", samplerCube, float3);

    if (resources.OES_EGL_image_external || resources.NV_EGL_stream_consumer_external)
    {
        const TType *samplerExternalOES = StaticType::GetBasic<EbtSamplerExternalOES>();

        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", samplerExternalOES, float2);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", samplerExternalOES, float3);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", samplerExternalOES, float4);
    }

    if (resources.ARB_texture_rectangle)
    {
        const TType *sampler2DRect = StaticType::GetBasic<EbtSampler2DRect>();

        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRect", sampler2DRect, float2);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRectProj", sampler2DRect, float3);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRectProj", sampler2DRect, float4);
    }

    if (resources.EXT_shader_texture_lod)
    {
        /* The *Grad* variants are new to both vertex and fragment shaders; the fragment
         * shader specific pieces are added separately below.
         */
        insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                      "texture2DGradEXT", sampler2D, float2, float2, float2);
        insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                      "texture2DProjGradEXT", sampler2D, float3, float2, float2);
        insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                      "texture2DProjGradEXT", sampler2D, float4, float2, float2);
        insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                      "textureCubeGradEXT", samplerCube, float3, float3, float3);
    }

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", sampler2D, float2, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float3, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float4, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "textureCube", samplerCube, float3, float1);

        if (resources.OES_standard_derivatives)
        {
            insertBuiltInOp(ESSL1_BUILTINS, EOpDFdx, TExtension::OES_standard_derivatives, genType,
                            genType);
            insertBuiltInOp(ESSL1_BUILTINS, EOpDFdy, TExtension::OES_standard_derivatives, genType,
                            genType);
            insertBuiltInOp(ESSL1_BUILTINS, EOpFwidth, TExtension::OES_standard_derivatives,
                            genType, genType);
        }

        if (resources.EXT_shader_texture_lod)
        {
            insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                          "texture2DLodEXT", sampler2D, float2, float1);
            insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                          "texture2DProjLodEXT", sampler2D, float3, float1);
            insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                          "texture2DProjLodEXT", sampler2D, float4, float1);
            insertBuiltIn(ESSL1_BUILTINS, TExtension::EXT_shader_texture_lod, float4,
                          "textureCubeLodEXT", samplerCube, float3, float1);
        }
    }

    if (type == GL_VERTEX_SHADER)
    {
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DLod", sampler2D, float2, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLod", sampler2D, float3, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLod", sampler2D, float4, float1);
        insertBuiltIn(ESSL1_BUILTINS, float4, "textureCubeLod", samplerCube, float3, float1);
    }

    const TType *gvec4 = StaticType::GetBasic<EbtGVec4>();

    const TType *gsampler2D      = StaticType::GetBasic<EbtGSampler2D>();
    const TType *gsamplerCube    = StaticType::GetBasic<EbtGSamplerCube>();
    const TType *gsampler3D      = StaticType::GetBasic<EbtGSampler3D>();
    const TType *gsampler2DArray = StaticType::GetBasic<EbtGSampler2DArray>();
    const TType *gsampler2DMS    = StaticType::GetBasic<EbtGSampler2DMS>();

    //
    // Texture Functions for GLSL ES 3.0
    //
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2D, float2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler3D, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsamplerCube, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2DArray, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float4);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler3D, float4);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler2D, float2, float1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler3D, float3, float1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsamplerCube, float3, float1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler2DArray, float3, float1);

    if (resources.OES_EGL_image_external_essl3)
    {
        const TType *samplerExternalOES = StaticType::GetBasic<EbtSamplerExternalOES>();

        insertBuiltIn(ESSL3_BUILTINS, float4, "texture", samplerExternalOES, float2);
        insertBuiltIn(ESSL3_BUILTINS, float4, "textureProj", samplerExternalOES, float3);
        insertBuiltIn(ESSL3_BUILTINS, float4, "textureProj", samplerExternalOES, float4);
    }

    if (resources.EXT_YUV_target)
    {
        const TType *samplerExternal2DY2YEXT = StaticType::GetBasic<EbtSamplerExternal2DY2YEXT>();

        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "texture",
                      samplerExternal2DY2YEXT, float2);
        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "textureProj",
                      samplerExternal2DY2YEXT, float3);
        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "textureProj",
                      samplerExternal2DY2YEXT, float4);

        const TType *yuvCscStandardEXT = StaticType::GetBasic<EbtYuvCscStandardEXT>();

        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float3, "rgb_2_yuv", float3,
                      yuvCscStandardEXT);
        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float3, "yuv_2_rgb", float3,
                      yuvCscStandardEXT);
    }

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2D, float2, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler3D, float3, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsamplerCube, float3, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2DArray, float3, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float3, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float4, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler3D, float4, float1);

        if (resources.OES_EGL_image_external_essl3)
        {
            const TType *samplerExternalOES = StaticType::GetBasic<EbtSamplerExternalOES>();

            insertBuiltIn(ESSL3_BUILTINS, float4, "texture", samplerExternalOES, float2, float1);
            insertBuiltIn(ESSL3_BUILTINS, float4, "textureProj", samplerExternalOES, float3,
                          float1);
            insertBuiltIn(ESSL3_BUILTINS, float4, "textureProj", samplerExternalOES, float4,
                          float1);
        }

        if (resources.EXT_YUV_target)
        {
            const TType *samplerExternal2DY2YEXT =
                StaticType::GetBasic<EbtSamplerExternal2DY2YEXT>();

            insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "texture",
                          samplerExternal2DY2YEXT, float2, float1);
            insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "textureProj",
                          samplerExternal2DY2YEXT, float3, float1);
            insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "textureProj",
                          samplerExternal2DY2YEXT, float4, float1);
        }
    }

    const TType *sampler2DShadow      = StaticType::GetBasic<EbtSampler2DShadow>();
    const TType *samplerCubeShadow    = StaticType::GetBasic<EbtSamplerCubeShadow>();
    const TType *sampler2DArrayShadow = StaticType::GetBasic<EbtSampler2DArrayShadow>();

    insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DShadow, float3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "texture", samplerCubeShadow, float4);
    insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DArrayShadow, float4);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProj", sampler2DShadow, float4);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureLod", sampler2DShadow, float3, float1);

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DShadow, float3, float1);
        insertBuiltIn(ESSL3_BUILTINS, float1, "texture", samplerCubeShadow, float4, float1);
        insertBuiltIn(ESSL3_BUILTINS, float1, "textureProj", sampler2DShadow, float4, float1);
    }

    insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", gsampler2D, int1);
    insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", gsampler3D, int1);
    insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", gsamplerCube, int1);
    insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", gsampler2DArray, int1);
    insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", sampler2DShadow, int1);
    insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", samplerCubeShadow, int1);
    insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", sampler2DArrayShadow, int1);
    insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", gsampler2DMS);

    if (resources.OES_EGL_image_external_essl3)
    {
        const TType *samplerExternalOES = StaticType::GetBasic<EbtSamplerExternalOES>();

        insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", samplerExternalOES, int1);
    }

    if (resources.EXT_YUV_target)
    {
        const TType *samplerExternal2DY2YEXT = StaticType::GetBasic<EbtSamplerExternal2DY2YEXT>();

        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, int2, "textureSize",
                      samplerExternal2DY2YEXT, int1);
    }

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltInOp(ESSL3_BUILTINS, EOpDFdx, genType, genType);
        insertBuiltInOp(ESSL3_BUILTINS, EOpDFdy, genType, genType);
        insertBuiltInOp(ESSL3_BUILTINS, EOpFwidth, genType, genType);
    }

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2D, float2, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler3D, float3, int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureOffset", sampler2DShadow, float3, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2DArray, float3, int2);

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2D, float2, int2, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler3D, float3, int3, float1);
        insertBuiltIn(ESSL3_BUILTINS, float1, "textureOffset", sampler2DShadow, float3, int2,
                      float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2DArray, float3, int2,
                      float1);
    }

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float3, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float4, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler3D, float4, int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjOffset", sampler2DShadow, float4, int2);

    if (type == GL_FRAGMENT_SHADER)
    {
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float3, int2, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float4, int2, float1);
        insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler3D, float4, int3, float1);
        insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjOffset", sampler2DShadow, float4, int2,
                      float1);
    }

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler2D, float2, float1, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler3D, float3, float1, int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureLodOffset", sampler2DShadow, float3, float1,
                  int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler2DArray, float3, float1, int2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler2D, float3, float1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler2D, float4, float1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler3D, float4, float1);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjLod", sampler2DShadow, float4, float1);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler2D, float3, float1, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler2D, float4, float1, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler3D, float4, float1, int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjLodOffset", sampler2DShadow, float4, float1,
                  int2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler2D, int2, int1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler3D, int3, int1);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler2DArray, int3, int1);

    if (resources.OES_EGL_image_external_essl3)
    {
        const TType *samplerExternalOES = StaticType::GetBasic<EbtSamplerExternalOES>();

        insertBuiltIn(ESSL3_BUILTINS, float4, "texelFetch", samplerExternalOES, int2, int1);
    }

    if (resources.EXT_YUV_target)
    {
        const TType *samplerExternal2DY2YEXT = StaticType::GetBasic<EbtSamplerExternal2DY2YEXT>();

        insertBuiltIn(ESSL3_BUILTINS, TExtension::EXT_YUV_target, float4, "texelFetch",
                      samplerExternal2DY2YEXT, int2, int1);
    }

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler2D, int2, int1, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler3D, int3, int1, int3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler2DArray, int3, int1, int2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler2D, float2, float2, float2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler3D, float3, float3, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsamplerCube, float3, float3, float3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", sampler2DShadow, float3, float2, float2);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", samplerCubeShadow, float4, float3, float3);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler2DArray, float3, float2, float2);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", sampler2DArrayShadow, float4, float2,
                  float2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler2D, float2, float2, float2,
                  int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler3D, float3, float3, float3,
                  int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureGradOffset", sampler2DShadow, float3, float2,
                  float2, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler2DArray, float3, float2,
                  float2, int2);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureGradOffset", sampler2DArrayShadow, float4, float2,
                  float2, int2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler2D, float3, float2, float2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler2D, float4, float2, float2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler3D, float4, float3, float3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjGrad", sampler2DShadow, float4, float2,
                  float2);

    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler2D, float3, float2,
                  float2, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler2D, float4, float2,
                  float2, int2);
    insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler3D, float4, float3,
                  float3, int3);
    insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjGradOffset", sampler2DShadow, float4, float2,
                  float2, int2);

    const TType *atomicCounter = StaticType::GetBasic<EbtAtomicCounter>();
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicCounter", atomicCounter);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicCounterIncrement", atomicCounter);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicCounterDecrement", atomicCounter);

    // Insert all atomic memory functions
    const TType *int1InOut  = StaticType::GetQualified<EbtInt, EvqInOut>();
    const TType *uint1InOut = StaticType::GetQualified<EbtUInt, EvqInOut>();
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicAdd", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicAdd", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicMin", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicMin", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicMax", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicMax", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicAnd", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicAnd", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicOr", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicOr", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicXor", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicXor", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicExchange", uint1InOut, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicExchange", int1InOut, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, uint1, "atomicCompSwap", uint1InOut, uint1, uint1);
    insertBuiltIn(ESSL3_1_BUILTINS, int1, "atomicCompSwap", int1InOut, int1, int1);

    const TType *gimage2D      = StaticType::GetBasic<EbtGImage2D>();
    const TType *gimage3D      = StaticType::GetBasic<EbtGImage3D>();
    const TType *gimage2DArray = StaticType::GetBasic<EbtGImage2DArray>();
    const TType *gimageCube    = StaticType::GetBasic<EbtGImageCube>();

    insertBuiltIn(ESSL3_1_BUILTINS, voidType, "imageStore", gimage2D, int2, gvec4);
    insertBuiltIn(ESSL3_1_BUILTINS, voidType, "imageStore", gimage3D, int3, gvec4);
    insertBuiltIn(ESSL3_1_BUILTINS, voidType, "imageStore", gimage2DArray, int3, gvec4);
    insertBuiltIn(ESSL3_1_BUILTINS, voidType, "imageStore", gimageCube, int3, gvec4);

    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "imageLoad", gimage2D, int2);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "imageLoad", gimage3D, int3);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "imageLoad", gimage2DArray, int3);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "imageLoad", gimageCube, int3);

    insertBuiltIn(ESSL3_1_BUILTINS, int2, "imageSize", gimage2D);
    insertBuiltIn(ESSL3_1_BUILTINS, int3, "imageSize", gimage3D);
    insertBuiltIn(ESSL3_1_BUILTINS, int3, "imageSize", gimage2DArray);
    insertBuiltIn(ESSL3_1_BUILTINS, int2, "imageSize", gimageCube);

    insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpMemoryBarrier, voidType,
                                      "memoryBarrier");
    insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpMemoryBarrierAtomicCounter, voidType,
                                      "memoryBarrierAtomicCounter");
    insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpMemoryBarrierBuffer, voidType,
                                      "memoryBarrierBuffer");
    insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpMemoryBarrierImage, voidType,
                                      "memoryBarrierImage");

    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "texelFetch", gsampler2DMS, int2, int1);

    // Insert all variations of textureGather.
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsampler2D, float2);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsampler2D, float2, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsampler2DArray, float3);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsampler2DArray, float3, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsamplerCube, float3);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGather", gsamplerCube, float3, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", sampler2DShadow, float2);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", sampler2DShadow, float2, float1);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", sampler2DArrayShadow, float3);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", sampler2DArrayShadow, float3, float1);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", samplerCubeShadow, float3);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGather", samplerCubeShadow, float3, float1);

    // Insert all variations of textureGatherOffset.
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGatherOffset", gsampler2D, float2, int2);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGatherOffset", gsampler2D, float2, int2, int1);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGatherOffset", gsampler2DArray, float3, int2);
    insertBuiltIn(ESSL3_1_BUILTINS, gvec4, "textureGatherOffset", gsampler2DArray, float3, int2,
                  int1);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGatherOffset", sampler2DShadow, float2, float1,
                  int2);
    insertBuiltIn(ESSL3_1_BUILTINS, float4, "textureGatherOffset", sampler2DArrayShadow, float3,
                  float1, int2);

    if (type == GL_COMPUTE_SHADER)
    {
        insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpBarrier, voidType, "barrier");
        insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpMemoryBarrierShared, voidType,
                                          "memoryBarrierShared");
        insertBuiltInFunctionNoParameters(ESSL3_1_BUILTINS, EOpGroupMemoryBarrier, voidType,
                                          "groupMemoryBarrier");
    }

    if (type == GL_GEOMETRY_SHADER_EXT)
    {
        TExtension extension = TExtension::EXT_geometry_shader;
        insertBuiltInFunctionNoParametersExt(ESSL3_1_BUILTINS, extension, EOpEmitVertex, voidType,
                                             "EmitVertex");
        insertBuiltInFunctionNoParametersExt(ESSL3_1_BUILTINS, extension, EOpEndPrimitive, voidType,
                                             "EndPrimitive");
    }
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
    insertStructType(COMMON_BUILTINS, depthRangeStruct);
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
    if (resources.EXT_blend_func_extended)
    {
        insertConstIntExt<EbpMedium>(COMMON_BUILTINS, TExtension::EXT_blend_func_extended,
                                     ImmutableString("gl_MaxDualSourceDrawBuffersEXT"),
                                     resources.MaxDualSourceDrawBuffers);
    }

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

    if (resources.EXT_geometry_shader)
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
            insertInterfaceBlock(ESSL3_1_BUILTINS, glPerVertexInBlock);

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

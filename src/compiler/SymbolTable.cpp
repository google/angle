//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Symbol table for parsing.  Most functionaliy and main ideas
// are documented in the header file.
//

#if defined(_MSC_VER)
#pragma warning(disable: 4718)
#endif

#include "compiler/SymbolTable.h"

#include <stdio.h>
#include <algorithm>

#include "common/angleutils.h"

TType::TType(const TPublicType &p) :
            type(p.type), precision(p.precision), qualifier(p.qualifier), primarySize(p.primarySize), secondarySize(p.secondarySize), array(p.array), layoutQualifier(p.layoutQualifier), arraySize(p.arraySize),
            maxArraySize(0), arrayInformationType(0), interfaceBlockType(0), structure(0), structureSize(0), deepestStructNesting(0), fieldName(0), mangled(0), typeName(0)
{
    if (p.userDef) {
        structure = p.userDef->getStruct();
        typeName = NewPoolTString(p.userDef->getTypeName().c_str());
        computeDeepestStructNesting();
    }
}

//
// Recursively generate mangled names.
//
void TType::buildMangledName(TString& mangledName)
{
    if (isMatrix())
        mangledName += 'm';
    else if (isVector())
        mangledName += 'v';

    switch (type) {
    case EbtFloat:              mangledName += 'f';      break;
    case EbtInt:                mangledName += 'i';      break;
    case EbtUInt:               mangledName += 'u';      break;
    case EbtBool:               mangledName += 'b';      break;
    case EbtSampler2D:          mangledName += "s2";     break;
    case EbtSamplerCube:        mangledName += "sC";     break;
    case EbtStruct:
        mangledName += "struct-";
        if (typeName)
            mangledName += *typeName;
        {// support MSVC++6.0
            for (unsigned int i = 0; i < structure->size(); ++i) {
                mangledName += '-';
                (*structure)[i].type->buildMangledName(mangledName);
            }
        }
        break;
    case EbtInterfaceBlock:
        {
            mangledName += "interface-block-";
            if (typeName)
            {
                mangledName += *typeName;
            }
            for (unsigned int i = 0; i < structure->size(); ++i)
            {
                mangledName += '-';
                (*structure)[i].type->buildMangledName(mangledName);
            }
        }
        break;
    default:
        break;
    }

    if (isMatrix())
    {
        mangledName += static_cast<char>('0' + getCols());
        mangledName += static_cast<char>('x');
        mangledName += static_cast<char>('0' + getRows());
    }
    else
    {
        mangledName += static_cast<char>('0' + getNominalSize());
    }
    if (isArray()) {
        char buf[20];
        snprintf(buf, sizeof(buf), "%d", arraySize);
        mangledName += '[';
        mangledName += buf;
        mangledName += ']';
    }
}

int TType::getStructSize() const
{
    if (!getStruct()) {
        assert(false && "Not a struct");
        return 0;
    }

    if (structureSize == 0)
        for (TTypeList::const_iterator tl = getStruct()->begin(); tl != getStruct()->end(); tl++)
            structureSize += ((*tl).type)->getObjectSize();

    return structureSize;
}

void TType::computeDeepestStructNesting()
{
    if (!getStruct()) {
        return;
    }

    int maxNesting = 0;
    for (TTypeList::const_iterator tl = getStruct()->begin(); tl != getStruct()->end(); ++tl) {
        maxNesting = std::max(maxNesting, ((*tl).type)->getDeepestStructNesting());
    }

    deepestStructNesting = 1 + maxNesting;
}

bool TType::isStructureContainingArrays() const
{
    if (!structure)
    {
        return false;
    }

    for (TTypeList::const_iterator member = structure->begin(); member != structure->end(); member++)
    {
        if (member->type->isArray() ||
            member->type->isStructureContainingArrays())
        {
            return true;
        }
    }

    return false;
}

//
// Functions have buried pointers to delete.
//
TFunction::~TFunction()
{
    for (TParamList::iterator i = parameters.begin(); i != parameters.end(); ++i)
        delete (*i).type;
}

//
// Symbol table levels are a map of pointers to symbols that have to be deleted.
//
TSymbolTableLevel::~TSymbolTableLevel()
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it)
        delete (*it).second;
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in operation.  This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToOperator(const char* name, TOperator op)
{
    tLevel::iterator it;
    for (it = level.begin(); it != level.end(); ++it) {
        if ((*it).second->isFunction()) {
            TFunction* function = static_cast<TFunction*>((*it).second);
            if (function->getName() == name)
                function->relateToOperator(op);
        }
    }
}

//
// Change all function entries in the table with the non-mangled name
// to be related to the provided built-in extension. This is a low
// performance operation, and only intended for symbol tables that
// live across a large number of compiles.
//
void TSymbolTableLevel::relateToExtension(const char* name, const TString& ext)
{
    for (tLevel::iterator it = level.begin(); it != level.end(); ++it) {
        if (it->second->isFunction()) {
            TFunction* function = static_cast<TFunction*>(it->second);
            if (function->getName() == name)
                function->relateToExtension(ext);
        }
    }
}

TSymbol::TSymbol(const TSymbol& copyOf)
{
    name = NewPoolTString(copyOf.name->c_str());
    uniqueId = copyOf.uniqueId;
}

TSymbol *TSymbolTable::find(const TString &name, int shaderVersion, bool *builtIn, bool *sameScope)
{
    int level = currentLevel();
    TSymbol *symbol;

    do
    {
        if (level == ESSL3_BUILTINS && shaderVersion != 300) level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100) level--;

        symbol = table[level]->find(name);
    }
    while (symbol == 0 && --level >= 0);

    if (builtIn)
        *builtIn = (level <= LAST_BUILTIN_LEVEL);
    if (sameScope)
        *sameScope = (level == currentLevel());

    return symbol;
}

TSymbol *TSymbolTable::findBuiltIn(const TString &name, int shaderVersion)
{
    for (int level = LAST_BUILTIN_LEVEL; level >= 0; level--)
    {
        if (level == ESSL3_BUILTINS && shaderVersion != 300) level--;
        if (level == ESSL1_BUILTINS && shaderVersion != 100) level--;

        TSymbol *symbol = table[level]->find(name);

        if (symbol)
            return symbol;
    }

    return 0;
}

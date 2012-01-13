//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/MapLongVariableNames.h"

namespace {

TString mapLongName(int id, const TString& name, bool global)
{
    ASSERT(name.size() > MAX_SHORTENED_IDENTIFIER_SIZE);
    TStringStream stream;
    stream << "webgl_";
    if (global)
        stream << "g";
    stream << id << "_";
    stream << name.substr(0, MAX_SHORTENED_IDENTIFIER_SIZE - stream.str().size());
    return stream.str();
}

MapLongVariableNames* gMapLongVariableNamesInstance = NULL;

}  // anonymous namespace

MapLongVariableNames::MapLongVariableNames()
    : refCount(0)
{
}

MapLongVariableNames::~MapLongVariableNames()
{
}

// static
MapLongVariableNames* MapLongVariableNames::GetInstance()
{
    if (gMapLongVariableNamesInstance == NULL)
        gMapLongVariableNamesInstance = new MapLongVariableNames;
    gMapLongVariableNamesInstance->refCount++;
    return gMapLongVariableNamesInstance;
}

void MapLongVariableNames::Release()
{
    ASSERT(gMapLongVariableNamesInstance == this);
    ASSERT(refCount > 0);
    refCount--;
    if (refCount == 0) {
        delete gMapLongVariableNamesInstance;
        gMapLongVariableNamesInstance = NULL;
    }
}

void MapLongVariableNames::visitSymbol(TIntermSymbol* symbol)
{
    ASSERT(symbol != NULL);
    if (symbol->getSymbol().size() > MAX_SHORTENED_IDENTIFIER_SIZE) {
        switch (symbol->getQualifier()) {
          case EvqVaryingIn:
          case EvqVaryingOut:
          case EvqInvariantVaryingIn:
          case EvqInvariantVaryingOut:
          case EvqUniform:
            symbol->setSymbol(
                mapLongGlobalName(symbol->getSymbol()));
            break;
          default:
            symbol->setSymbol(
                mapLongName(symbol->getId(), symbol->getSymbol(), false));
            break;
        };
    }
}

bool MapLongVariableNames::visitLoop(Visit, TIntermLoop* node)
{
    if (node->getInit())
        node->getInit()->traverse(this);
    return true;
}

TString MapLongVariableNames::mapLongGlobalName(const TString& name)
{
    std::map<std::string, std::string>::const_iterator it = longGlobalNameMap.find(name.c_str());
    if (it != longGlobalNameMap.end())
        return (*it).second.c_str();

    int id = longGlobalNameMap.size();
    TString mappedName = mapLongName(id, name, true);
    longGlobalNameMap.insert(
        std::map<std::string, std::string>::value_type(name.c_str(), mappedName.c_str()));
    return mappedName;
}

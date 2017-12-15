//
// Copyright (c) 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/translator/HashNames.h"

#include "compiler/translator/IntermNode.h"
#include "compiler/translator/Symbol.h"

namespace sh
{

namespace
{

// GLSL ES 3.00.6 section 3.9: the maximum length of an identifier is 1024 characters.
static const unsigned int kESSLMaxIdentifierLength = 1024u;

static const char *kHashedNamePrefix = "webgl_";

// Can't prefix with just _ because then we might introduce a double underscore, which is not safe
// in GLSL (ESSL 3.00.6 section 3.8: All identifiers containing a double underscore are reserved for
// use by the underlying implementation). u is short for user-defined.
static const char *kUnhashedNamePrefix              = "_u";
static const unsigned int kUnhashedNamePrefixLength = 2u;

TString HashName(const TString &name, ShHashFunction64 hashFunction)
{
    ASSERT(!name.empty());
    ASSERT(hashFunction);
    khronos_uint64_t number = (*hashFunction)(name.c_str(), name.length());
    TStringStream stream;
    stream << kHashedNamePrefix << std::hex << number;
    TString hashedName = stream.str();
    return hashedName;
}

}  // anonymous namespace

TString HashName(const TString &name, ShHashFunction64 hashFunction, NameMap *nameMap)
{
    if (hashFunction == nullptr)
    {
        if (name.length() + kUnhashedNamePrefixLength > kESSLMaxIdentifierLength)
        {
            // If the identifier length is already close to the limit, we can't prefix it. This is
            // not a problem since there are no builtins or ANGLE's internal variables that would
            // have as long names and could conflict.
            return name;
        }
        return kUnhashedNamePrefix + name;
    }
    if (nameMap)
    {
        NameMap::const_iterator it = nameMap->find(name.c_str());
        if (it != nameMap->end())
            return it->second.c_str();
    }
    TString hashedName = HashName(name, hashFunction);
    if (nameMap)
    {
        (*nameMap)[name.c_str()] = hashedName.c_str();
    }
    return hashedName;
}

TString HashName(const TSymbol *symbol, ShHashFunction64 hashFunction, NameMap *nameMap)
{
    if (symbol->symbolType() == SymbolType::Empty)
    {
        return TString();
    }
    if (symbol->symbolType() == SymbolType::AngleInternal ||
        symbol->symbolType() == SymbolType::BuiltIn)
    {
        return symbol->name();
    }
    return HashName(symbol->name(), hashFunction, nameMap);
}

}  // namespace sh

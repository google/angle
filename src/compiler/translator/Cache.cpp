//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Cache.cpp: Implements a cache for various commonly created objects.

#include "common/angleutils.h"
#include "compiler/translator/Cache.h"

namespace
{

class TScopedAllocator : angle::NonCopyable
{
  public:
    TScopedAllocator(TPoolAllocator *allocator)
        : mPreviousAllocator(GetGlobalPoolAllocator())
    {
        SetGlobalPoolAllocator(allocator);
    }
    ~TScopedAllocator()
    {
        SetGlobalPoolAllocator(mPreviousAllocator);
    }

  private:
    TPoolAllocator *mPreviousAllocator;
};

}  // namespace

TCache::TypeKey::TypeKey(TBasicType basicType,
                         TPrecision precision,
                         TQualifier qualifier,
                         unsigned char primarySize,
                         unsigned char secondarySize)
{
    static_assert(sizeof(components) <= sizeof(value),
                  "TypeKey::value is too small");

    // Can't use numeric_limits::max() here because of MSVC 2013
    const size_t MaxEnumValue = ~EnumComponentType(0);

    static_assert(MaxEnumValue >= EbtLast &&
                  MaxEnumValue >= EbpLast &&
                  MaxEnumValue >= EvqLast,
                  "TypeKey::EnumComponentType is too small");

    value = 0;
    components.basicType = basicType;
    components.precision = precision;
    components.qualifier = qualifier;
    components.primarySize = primarySize;
    components.secondarySize = secondarySize;
}

TCache *TCache::sCache = nullptr;

void TCache::initialize()
{
    if (sCache == nullptr)
    {
        sCache = new TCache();
    }
}

void TCache::destroy()
{
    SafeDelete(sCache);
}

const TType *TCache::getType(TBasicType basicType,
                             TPrecision precision,
                             TQualifier qualifier,
                             unsigned char primarySize,
                             unsigned char secondarySize)
{
    TypeKey key(basicType, precision, qualifier,
                primarySize, secondarySize);
    auto it = sCache->mTypes.find(key);
    if (it != sCache->mTypes.end())
    {
        return it->second;
    }

    TScopedAllocator scopedAllocator(&sCache->mAllocator);

    TType *type = new TType(basicType, precision, qualifier,
                            primarySize, secondarySize);
    type->realize();
    sCache->mTypes.insert(std::make_pair(key, type));

    return type;
}

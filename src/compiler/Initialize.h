//
// Copyright (c) 2002-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef _INITIALIZE_INCLUDED_
#define _INITIALIZE_INCLUDED_

#include "compiler/Common.h"
#include "compiler/ShHandle.h"
#include "compiler/SymbolTable.h"

typedef TVector<TString> TBuiltInStrings;

class TBuiltIns {
public:
    POOL_ALLOCATOR_NEW_DELETE(GlobalPoolAllocator)

    void initialize(ShShaderType type, ShShaderSpec spec,
                    const ShBuiltInResources& resources,
                    const TExtensionBehavior& extensionBehavior);
    const TBuiltInStrings &getCommonBuiltIns() const { return commonBuiltIns; }
    const TBuiltInStrings &getEssl1BuiltIns() const { return essl1BuiltIns; }
    const TBuiltInStrings &getEssl3BuiltIns() const { return essl3BuiltIns; }

protected:
    TBuiltInStrings commonBuiltIns;
    TBuiltInStrings essl1BuiltIns;
    TBuiltInStrings essl3BuiltIns;
};

void IdentifyBuiltIns(ShShaderType type, ShShaderSpec spec,
                      const ShBuiltInResources& resources,
                      TSymbolTable& symbolTable);

void InitExtensionBehavior(const ShBuiltInResources& resources,
                           TExtensionBehavior& extensionBehavior);

#endif // _INITIALIZE_INCLUDED_

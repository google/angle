//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_MAP_LONG_VARIABLE_NAMES_H_
#define COMPILER_MAP_LONG_VARIABLE_NAMES_H_

#include "GLSLANG/ShaderLang.h"

#include "compiler/intermediate.h"
#include "compiler/VariableInfo.h"

// This size does not include '\0' in the end.
#define MAX_SHORTENED_IDENTIFIER_SIZE 32

// MapLongVariableNames is implemented as a ref-counted singleton.  The first
// call of GetInstance() will create an instance and return it; latter calls
// will return the same instance, with ref-count increased.  Release() will
// reduce the ref-count, and when no more reference, release the instance.

// Traverses intermediate tree to map attributes and uniforms names that are
// longer than MAX_SHORTENED_IDENTIFIER_SIZE to MAX_SHORTENED_IDENTIFIER_SIZE.
class MapLongVariableNames : public TIntermTraverser {
public:
    static MapLongVariableNames* GetInstance();
    void Release();

    virtual void visitSymbol(TIntermSymbol*);
    virtual bool visitLoop(Visit, TIntermLoop*);

private:
    MapLongVariableNames();
    virtual ~MapLongVariableNames();

    TString mapLongGlobalName(const TString& name);

    // Pair of long global varibale name <originalName, mappedName>.
    std::map<std::string, std::string> longGlobalNameMap;
    size_t refCount;
};

#endif  // COMPILER_MAP_LONG_VARIABLE_NAMES_H_

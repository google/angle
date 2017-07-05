//
// Copyright (c) 2002-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_HASHNAMES_H_
#define COMPILER_TRANSLATOR_HASHNAMES_H_

#include <map>

#include "GLSLANG/ShaderLang.h"
#include "compiler/translator/Common.h"

#define HASHED_NAME_PREFIX "webgl_"

namespace sh
{

typedef std::map<TPersistString, TPersistString> NameMap;

// Return the original name if hash function pointer is NULL;
// otherwise return the hashed name.
TString HashName(const TString &name, ShHashFunction64 hashFunction);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_HASHNAMES_H_

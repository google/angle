//
// Copyright (c) 2002-2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// The ArrayReturnValueToOutParameter function changes return values of an array type to out
// parameters in
// function definitions, prototypes and call sites.

#ifndef COMPILER_TRANSLATOR_ARRAYRETURNVALUETOOUTPARAMETER_H_
#define COMPILER_TRANSLATOR_ARRAYRETURNVALUETOOUTPARAMETER_H_

namespace sh
{

class TIntermNode;
class TSymbolUniqueId;

void ArrayReturnValueToOutParameter(TIntermNode *root, TSymbolUniqueId *temporaryId);

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_ARRAYRETURNVALUETOOUTPARAMETER_H_

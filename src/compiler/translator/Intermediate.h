//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_TRANSLATOR_INTERMEDIATE_H_
#define COMPILER_TRANSLATOR_INTERMEDIATE_H_

#include "compiler/translator/IntermNode.h"

namespace sh
{

//
// Set of helper functions to help build the tree.
// TODO(oetuaho@nvidia.com): Clean this up, it doesn't need to be a class.
//
class TIntermediate
{
  public:
    static TIntermBlock *EnsureBlock(TIntermNode *node);

  private:
    TIntermediate(){};
};

}  // namespace sh

#endif  // COMPILER_TRANSLATOR_INTERMEDIATE_H_

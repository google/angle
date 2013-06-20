//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_
#define TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_

#include "compiler/BlockLayoutEncoder.h"

namespace sh
{

// Block layout packed according to the default D3D11 register packing rules
// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx

class HLSLBlockEncoder : public BlockLayoutEncoder
{
  public:
    HLSLBlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

  protected:
    virtual void enterAggregateType();
    virtual void exitAggregateType();
    virtual void getBlockLayoutInfo(const sh::Uniform &uniform, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(const sh::Uniform &uniform, int arrayStride, int matrixStride);
};

}

#endif // TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_
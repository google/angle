//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_
#define TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_

#include "compiler/translator/BlockLayoutEncoder.h"

namespace sh
{

struct Varying;
struct Uniform;

// Block layout packed according to the default D3D11 register packing rules
// See http://msdn.microsoft.com/en-us/library/windows/desktop/bb509632(v=vs.85).aspx

class HLSLBlockEncoder : public BlockLayoutEncoder
{
  public:
    HLSLBlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

    virtual void enterAggregateType();
    virtual void exitAggregateType();

  protected:
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);
};

// This method assigns values to the variable's "registerIndex" and "elementIndex" fields.
// "elementIndex" is only used for structures.
void HLSLVariableGetRegisterInfo(unsigned int baseRegisterIndex, Uniform *variable);

// This method returns the number of used registers for a ShaderVariable. It is dependent on the HLSLBlockEncoder
// class to count the number of used registers in a struct (which are individually packed according to the same rules).
unsigned int HLSLVariableRegisterCount(const Varying &variable);
unsigned int HLSLVariableRegisterCount(const Uniform &variable);

}

#endif // TRANSLATOR_HLSL_HLSLLAYOUTENCODER_H_

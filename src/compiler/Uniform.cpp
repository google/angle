//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/Uniform.h"

namespace sh
{

Uniform::Uniform(GLenum type, GLenum precision, const char *name, unsigned int arraySize, unsigned int registerIndex)
{
    this->type = type;
    this->precision = precision;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
}

BlockMemberInfo::BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix)
    : offset(offset),
      arrayStride(arrayStride),
      matrixStride(matrixStride),
      isRowMajorMatrix(isRowMajorMatrix)
{
}

const BlockMemberInfo BlockMemberInfo::defaultBlockInfo(-1, -1, -1, false);

InterfaceBlock::InterfaceBlock(const char *name, unsigned int arraySize, unsigned int registerIndex)
    : name(name),
      arraySize(arraySize),
      registerIndex(registerIndex)
{
}

}

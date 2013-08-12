//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/Uniform.h"

namespace sh
{

ShaderVariable::ShaderVariable()
    : type(GL_NONE),
      precision(GL_NONE),
      arraySize(0),
      location(-1)
{
}

ShaderVariable::ShaderVariable(GLenum type, GLenum precision, const char *name, unsigned int arraySize, int location)
    : type(type),
      precision(precision),
      name(name),
      arraySize(arraySize),
      location(location)
{
}

Uniform::Uniform(GLenum type, GLenum precision, const char *name, unsigned int arraySize, unsigned int registerIndex, bool isRowMajorMatrix)
{
    this->type = type;
    this->precision = precision;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
    this->isRowMajorMatrix = isRowMajorMatrix;
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
      layout(BLOCKLAYOUT_SHARED),
      registerIndex(registerIndex),
      isRowMajorLayout(false)
{
}

}

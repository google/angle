//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef COMPILER_UNIFORM_H_
#define COMPILER_UNIFORM_H_

#include <string>
#include <vector>

#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

namespace sh
{

struct ShaderVariable
{
    ShaderVariable(GLenum type, GLenum precision, const char *name, unsigned int arraySize, int location);

    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;
    int location;
};

typedef std::vector<ShaderVariable> ActiveShaderVariables;

struct Uniform
{
    Uniform(GLenum type, GLenum precision, const char *name, unsigned int arraySize, unsigned int registerIndex);

    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;

    unsigned int registerIndex;

    std::vector<Uniform> fields;
};

typedef std::vector<Uniform> ActiveUniforms;

struct BlockMemberInfo
{
    BlockMemberInfo(int offset, int arrayStride, int matrixStride, bool isRowMajorMatrix);

    int offset;
    int arrayStride;
    int matrixStride;
    bool isRowMajorMatrix;

    static const BlockMemberInfo defaultBlockInfo;
};

typedef std::vector<BlockMemberInfo> BlockMemberInfoArray;

enum BlockLayoutType
{
    BLOCKLAYOUT_STANDARD,
    BLOCKLAYOUT_PACKED,
    BLOCKLAYOUT_SHARED
};

struct InterfaceBlock
{
    InterfaceBlock(const char *name, unsigned int arraySize, unsigned int registerIndex);

    std::string name;
    unsigned int arraySize;
    ActiveUniforms activeUniforms;
    size_t dataSize;
    std::vector<BlockMemberInfo> blockInfo;
    BlockLayoutType layout;

    unsigned int registerIndex;

    void setBlockLayout(BlockLayoutType newLayout);

  private:
    void getBlockLayoutInfo(const sh::ActiveUniforms &fields, unsigned int *currentOffset);
    bool getBlockLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut);
    void getD3DLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut);
    void getStandardLayoutInfo(const sh::Uniform &uniform, unsigned int *currentOffset, int *arrayStrideOut, int *matrixStrideOut);
};

typedef std::vector<InterfaceBlock> ActiveInterfaceBlocks;

}

#endif   // COMPILER_UNIFORM_H_

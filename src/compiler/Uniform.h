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
    GLenum type;
    GLenum precision;
    std::string name;
    unsigned int arraySize;

    ShaderVariable(GLenum type, GLenum precision, const char *name, unsigned int arraySize);
};

struct Uniform : public ShaderVariable
{
    unsigned int registerIndex;
    std::vector<Uniform> fields;

    Uniform(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, unsigned int registerIndexIn);

    bool isStruct() const { return !fields.empty(); }
};

struct Attribute : public ShaderVariable
{
    int location;

    Attribute();
    Attribute(GLenum type, GLenum precision, const char *name, unsigned int arraySize, int location);
};

struct InterfaceBlockField : public ShaderVariable
{
    bool isRowMajorMatrix;
    std::vector<InterfaceBlockField> fields;

    InterfaceBlockField(GLenum typeIn, GLenum precisionIn, const char *nameIn, unsigned int arraySizeIn, bool isRowMajorMatrix);

    bool isStruct() const { return !fields.empty(); }
};

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
    size_t dataSize;
    BlockLayoutType layout;
    bool isRowMajorLayout;
    std::vector<InterfaceBlockField> fields;
    std::vector<BlockMemberInfo> blockInfo;

    unsigned int registerIndex;
};

typedef std::vector<InterfaceBlock> ActiveInterfaceBlocks;

}

#endif   // COMPILER_UNIFORM_H_

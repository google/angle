//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef TRANSLATOR_COMMON_BLOCKLAYOUTENCODER_H_
#define TRANSLATOR_COMMON_BLOCKLAYOUTENCODER_H_

#include <vector>
#define GL_APICALL
#include <GLES3/gl3.h>
#include <GLES2/gl2.h>

namespace sh
{

struct Uniform;
struct BlockMemberInfo;

class BlockLayoutEncoder
{
  public:
    BlockLayoutEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

    void encodeFields(const std::vector<Uniform> &fields);
    void encodeType(const Uniform &uniform);
    void encodeType(GLenum type, unsigned int arraySize, bool isRowMajorMatrix);
    size_t getBlockSize() { return mCurrentOffset * ComponentSize; }

    static const size_t ComponentSize = 4u;
    static const unsigned int RegisterSize = 4u;

  protected:
    size_t mCurrentOffset;

    void nextRegister();

    virtual void enterAggregateType() = 0;
    virtual void exitAggregateType() = 0;
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut) = 0;
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride) = 0;

  private:
    std::vector<BlockMemberInfo> *mBlockInfoOut;
};

// Block layout according to the std140 block layout
// See "Standard Uniform Block Layout" in Section 2.11.6 of the OpenGL ES 3.0 specification

class Std140BlockEncoder : public BlockLayoutEncoder
{
  public:
    Std140BlockEncoder(std::vector<BlockMemberInfo> *blockInfoOut);

  protected:
    virtual void enterAggregateType();
    virtual void exitAggregateType();
    virtual void getBlockLayoutInfo(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int *arrayStrideOut, int *matrixStrideOut);
    virtual void advanceOffset(GLenum type, unsigned int arraySize, bool isRowMajorMatrix, int arrayStride, int matrixStride);
};

}

#endif // TRANSLATOR_COMMON_BLOCKLAYOUTENCODER_H_

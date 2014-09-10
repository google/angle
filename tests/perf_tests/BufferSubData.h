//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SimpleBenchmark.h"

struct BufferSubDataParams : public BenchmarkParams
{
    EGLint requestedRenderer;
    GLenum vertexType;
    GLint vertexComponentCount;
    GLboolean vertexNormalized;
    GLsizeiptr updateSize;
    GLsizeiptr bufferSize;
    unsigned int iterations;
    unsigned int updatesEveryNFrames;

    virtual std::string name() const;
};

class BufferSubDataBenchmark : public SimpleBenchmark
{
  public:
    BufferSubDataBenchmark(const BufferSubDataParams &params);

    virtual bool initializeBenchmark();
    virtual void destroyBenchmark();
    virtual void beginDrawBenchmark();
    virtual void drawBenchmark();

  private:
    DISALLOW_COPY_AND_ASSIGN(BufferSubDataBenchmark);

    GLuint mProgram;
    GLuint mBuffer;
    uint8_t *mUpdateData;
    int mNumTris;

    const BufferSubDataParams mParams;
};


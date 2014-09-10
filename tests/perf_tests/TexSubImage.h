//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SimpleBenchmark.h"

struct TexSubImageParams : public BenchmarkParams
{
    int imageWidth;
    int imageHeight;
    int subImageWidth;
    int subImageHeight;
    unsigned int iterations;

    virtual std::string name() const;
};

class TexSubImageBenchmark : public SimpleBenchmark
{
  public:
    TexSubImageBenchmark(const TexSubImageParams &params);

    virtual bool initializeBenchmark();
    virtual void destroyBenchmark();
    virtual void beginDrawBenchmark();
    virtual void drawBenchmark();

  private:
    GLuint createTexture();

    TexSubImageParams mParams;

    // Handle to a program object
    GLuint mProgram;

    // Attribute locations
    GLint mPositionLoc;
    GLint mTexCoordLoc;

    // Sampler location
    GLint mSamplerLoc;

    // Texture handle
    GLuint mTexture;

    // Buffer handle
    GLuint mVertexBuffer;
    GLuint mIndexBuffer;

    GLubyte *mPixels;
};

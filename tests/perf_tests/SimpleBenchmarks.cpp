//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "SimpleBenchmark.h"
#include "BufferSubData.h"
#include "TexSubImage.h"

EGLint platforms[] =
{
    EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE,
    EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE
};

GLenum vertexTypes[] = { GL_FLOAT };
GLint componentCounts[] = { 4 };
GLboolean vertexNorms[] = { GL_FALSE };
GLsizeiptr updateSizes[] = { 0, 300 };
GLsizeiptr bufferSizes[] = { 1024 * 1024 };
unsigned int iterationCounts[] = { 10 };
unsigned int updatesEveryNFrames[] = { 1, 4 };

int main(int argc, char **argv)
{
    std::vector<BufferSubDataParams> subDataParams;

    for (size_t platIt = 0; platIt < ArraySize(platforms); platIt++)
    {
        for (size_t typeIt = 0; typeIt < ArraySize(vertexTypes); typeIt++)
        {
            for (size_t compIt = 0; compIt < ArraySize(componentCounts); compIt++)
            {
                for (size_t normIt = 0; normIt < ArraySize(vertexNorms); normIt++)
                {
                    // No normalized float data
                    if (vertexTypes[typeIt] == GL_FLOAT && vertexNorms[normIt] == GL_TRUE)
                    {
                        continue;
                    }

                    for (size_t updateIt = 0; updateIt < ArraySize(updateSizes); updateIt++)
                    {
                        for (size_t bufszIt = 0; bufszIt < ArraySize(bufferSizes); bufszIt++)
                        {
                            for (size_t itIt = 0; itIt < ArraySize(iterationCounts); itIt++)
                            {
                                for (size_t nfrIt = 0; nfrIt < ArraySize(updatesEveryNFrames); nfrIt++)
                                {
                                    BufferSubDataParams params;
                                    params.requestedRenderer = platforms[platIt];
                                    params.vertexType = vertexTypes[typeIt];
                                    params.vertexComponentCount = componentCounts[compIt];
                                    params.vertexNormalized = vertexNorms[normIt];
                                    params.updateSize = updateSizes[updateIt];
                                    params.bufferSize = bufferSizes[bufszIt];
                                    params.iterations = iterationCounts[itIt];
                                    params.updatesEveryNFrames = updatesEveryNFrames[nfrIt];

                                    if (updateSizes[updateIt] == 0)
                                    {
                                        if (nfrIt > 0)
                                        {
                                            continue;
                                        }
                                        else
                                        {
                                            params.updatesEveryNFrames = 1;
                                        }
                                    }

                                    subDataParams.push_back(params);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Enumerates permutations
    RunBenchmarks<BufferSubDataBenchmark>(subDataParams);

    std::vector<TexSubImageParams> subImageParams;

    for (size_t platIt = 0; platIt < ArraySize(platforms); platIt++)
    {
        TexSubImageParams params;

        params.requestedRenderer = platforms[platIt];
        params.imageWidth = 1024;
        params.imageHeight = 1024;
        params.subImageHeight = 64;
        params.subImageWidth = 64;
        params.iterations = 10;

        subImageParams.push_back(params);
    }

    RunBenchmarks<TexSubImageBenchmark>(subImageParams);
}

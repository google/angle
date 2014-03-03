//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "geometry_utils.h"

#define _USE_MATH_DEFINES
#include <math.h>

void CreateSphereGeometry(size_t sliceCount, float radius, SphereGeometry *result)
{
    size_t parellelCount = sliceCount / 2;
    size_t vertexCount = (parellelCount + 1) * (sliceCount + 1);
    size_t indexCount = parellelCount * sliceCount * 6;
    float angleStep = (2.0f * M_PI) / sliceCount;

    result->positions.resize(vertexCount);
    result->normals.resize(vertexCount);
    for (size_t i = 0; i < parellelCount + 1; i++)
    {
        for (size_t j = 0; j < sliceCount + 1; j++)
        {
            Vector3 direction(sinf(angleStep * i) * sinf(angleStep * j),
                              cosf(angleStep * i),
                              sinf(angleStep * i) * cosf(angleStep * j));

            size_t vertexIdx = i * (sliceCount + 1) + j;
            result->positions[vertexIdx] = direction * radius;
            result->normals[vertexIdx] = direction;
        }
    }

    result->indices.clear();
    result->indices.reserve(indexCount);
    for (size_t i = 0; i < parellelCount; i++)
    {
        for (size_t j = 0; j < sliceCount; j++)
        {
            result->indices.push_back( i      * (sliceCount + 1) +  j     );
            result->indices.push_back((i + 1) * (sliceCount + 1) +  j     );
            result->indices.push_back((i + 1) * (sliceCount + 1) + (j + 1));

            result->indices.push_back( i      * (sliceCount + 1) +  j     );
            result->indices.push_back((i + 1) * (sliceCount + 1) + (j + 1));
            result->indices.push_back( i      * (sliceCount + 1) + (j + 1));
        }
    }
}

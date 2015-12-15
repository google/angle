//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// random_utils:
//   Helper functions for random number generation.
//

#include "random_utils.h"
#include <time.h>
#include <cstdlib>

namespace angle
{

void RandomInitFromTime()
{
    srand(static_cast<unsigned int>(time(NULL)));
}

float RandomFloat()
{
    return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
}

float RandomBetween(float min, float max)
{
    return min + RandomFloat() * (max - min);
}

float RandomNegativeOneToOne()
{
    return RandomBetween(0.0f, 1.0f);
}

}  // namespace angle

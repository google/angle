//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// random_utils:
//   Helper functions for random number generation.
//

#ifndef UTIL_RANDOM_UTILS_H
#define UTIL_RANDOM_UTILS_H

namespace angle
{

// TODO(jmadill): Should make this a class
void RandomInitFromTime();
float RandomFloat();
float RandomBetween(float min, float max);
float RandomNegativeOneToOne();

}  // namespace angle

#endif // UTIL_RANDOM_UTILS_H

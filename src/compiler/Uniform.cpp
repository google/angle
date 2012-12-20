//
// Copyright (c) 2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "compiler/Uniform.h"

namespace sh
{

Uniform::Uniform(GLenum type, const char *name, int arraySize, int registerIndex)
{
    this->type = type;
    this->name = name;
    this->arraySize = arraySize;
    this->registerIndex = registerIndex;
}

}

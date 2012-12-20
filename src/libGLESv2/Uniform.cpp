//
// Copyright (c) 2010-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/Uniform.h"

#include "libGLESv2/utilities.h"

namespace gl
{

Uniform::Uniform(GLenum type, const std::string &name, unsigned int arraySize)
    : type(type), name(name), arraySize(arraySize)
{
    int bytes = gl::UniformInternalSize(type) * elementCount();
    data = new unsigned char[bytes];
    memset(data, 0, bytes);
    dirty = true;
}

Uniform::~Uniform()
{
    delete[] data;
}

bool Uniform::isArray() const
{
    return arraySize > 0;
}

unsigned int Uniform::elementCount() const
{
    return arraySize > 0 ? arraySize : 1;
}

unsigned int Uniform::registerCount() const
{
    return VariableRowCount(type) * elementCount();
}

}

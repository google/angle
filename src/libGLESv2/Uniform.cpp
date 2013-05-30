#include "precompiled.h"
//
// Copyright (c) 2010-2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/Uniform.h"

#include "libGLESv2/utilities.h"

namespace gl
{

Uniform::Uniform(GLenum type, GLenum precision, const std::string &name, unsigned int arraySize)
    : type(type), precision(precision), name(name), arraySize(arraySize)
{
    int bytes = gl::UniformInternalSize(type) * elementCount();
    data = new unsigned char[bytes];
    memset(data, 0, bytes);
    dirty = true;

    psRegisterIndex = GL_INVALID_INDEX;
    vsRegisterIndex = GL_INVALID_INDEX;
    registerCount = VariableRowCount(type) * elementCount();
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

bool Uniform::isReferencedByVertexShader() const
{
    return vsRegisterIndex != GL_INVALID_INDEX;
}

bool Uniform::isReferencedByFragmentShader() const
{
    return psRegisterIndex != GL_INVALID_INDEX;
}

}

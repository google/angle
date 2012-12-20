//
// Copyright (c) 2010-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#include "libGLESv2/Uniform.h"

#include "libGLESv2/utilities.h"

namespace gl
{

Uniform::Uniform(GLenum type, const std::string &_name, unsigned int arraySize)
    : type(type), _name(_name), name(undecorate(_name)), arraySize(arraySize)
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
    if (name != _name)   // D3D9_REPLACE
    {
        size_t dot = _name.find_last_of('.');
        if (dot == std::string::npos) dot = -1;

        return _name.compare(dot + 1, dot + 4, "ar_") == 0;
    }
    else
    {
        return arraySize > 0;
    }
}

unsigned int Uniform::elementCount() const
{
    return arraySize > 0 ? arraySize : 1;
}

unsigned int Uniform::registerCount() const
{
    return VariableRowCount(type) * elementCount();
}

std::string Uniform::undecorate(const std::string &_name)
{
    std::string name = _name;
    
    // Remove any structure field decoration
    size_t pos = 0;
    while ((pos = name.find("._", pos)) != std::string::npos)
    {
        name.replace(pos, 2, ".");
    }

    // Remove the leading decoration
    if (name[0] == '_')
    {
        return name.substr(1);
    }
    else if (name.compare(0, 3, "ar_") == 0)
    {
        return name.substr(3);
    }
    
    return name;
}

}

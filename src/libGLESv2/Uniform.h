//
// Copyright (c) 2010-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBGLESV2_UNIFORM_H_
#define LIBGLESV2_UNIFORM_H_

#include <string>
#include <vector>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"

namespace gl
{

// Helper struct representing a single shader uniform
struct Uniform
{
    Uniform(GLenum type, const std::string &_name, unsigned int arraySize);

    ~Uniform();

    bool isArray() const;
    unsigned int elementCount() const;
    unsigned int registerCount() const;
    static std::string Uniform::undecorate(const std::string &_name);

    const GLenum type;
    const std::string _name;   // Decorated name
    const std::string name;    // Undecorated name
    const unsigned int arraySize;

    unsigned char *data;
    bool dirty;

    struct RegisterInfo
    {
        RegisterInfo()
        {
            registerIndex = -1;
            registerCount = 0;
        }

        int registerIndex;
        unsigned int registerCount;
    };

    RegisterInfo ps;
    RegisterInfo vs;
};

typedef std::vector<Uniform*> UniformArray;

}

#endif   // LIBGLESV2_UNIFORM_H_

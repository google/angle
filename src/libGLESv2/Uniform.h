//
// Copyright (c) 2010-2012 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

#ifndef LIBGLESV2_UNIFORM_H_
#define LIBGLESV2_UNIFORM_H_

#include <string>

#define GL_APICALL
#include <GLES2/gl2.h>

#include "common/debug.h"
#include "libGLESv2/renderer/D3DConstantTable.h"

namespace gl
{

// Helper struct representing a single shader uniform
struct Uniform
{
    Uniform(GLenum type, const std::string &_name, unsigned int arraySize);

    ~Uniform();

    bool isArray();
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
            float4Index = -1;
            samplerIndex = -1;
            boolIndex = -1;
            registerCount = 0;
        }

        void set(const rx::D3DConstant *constant)
        {
            switch(constant->registerSet)
            {
              case rx::D3DConstant::RS_BOOL:    boolIndex = constant->registerIndex;    break;
              case rx::D3DConstant::RS_FLOAT4:  float4Index = constant->registerIndex;  break;
              case rx::D3DConstant::RS_SAMPLER: samplerIndex = constant->registerIndex; break;
              default: UNREACHABLE();
            }
            
            ASSERT(registerCount == 0 || registerCount == (int)constant->registerCount);
            registerCount = constant->registerCount;
        }

        int float4Index;
        int samplerIndex;
        int boolIndex;

        int registerCount;
    };

    RegisterInfo ps;
    RegisterInfo vs;
};

typedef std::vector<Uniform*> UniformArray;

}

#endif   // LIBGLESV2_UNIFORM_H_

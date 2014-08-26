//
// Copyright 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramImpl.h: Defines the abstract rx::ProgramImpl class.

#ifndef LIBGLESV2_RENDERER_PROGRAMIMPL_H_
#define LIBGLESV2_RENDERER_PROGRAMIMPL_H_

#include "common/angleutils.h"
#include "libGLESv2/Constants.h"
#include "libGLESv2/ProgramBinary.h"

namespace rx
{

class DynamicHLSL;
class Renderer;

class ProgramImpl
{
public:
    virtual ~ProgramImpl() { }

    // TODO: Temporary interfaces to ease migration. Remove soon!
    virtual Renderer *getRenderer() = 0;
    virtual DynamicHLSL *getDynamicHLSL() = 0;
    virtual void initializeUniformStorage(const std::vector<gl::LinkedUniform*> &uniforms) = 0;

    virtual void reset() = 0;
};

}

#endif // LIBGLESV2_RENDERER_PROGRAMIMPL_H_

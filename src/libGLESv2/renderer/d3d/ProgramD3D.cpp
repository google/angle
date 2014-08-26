//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ProgramD3D.cpp: Defines the rx::ProgramD3D class which implements rx::ProgramImpl.

#include "libGLESv2/renderer/d3d/ProgramD3D.h"

#include "common/utilities.h"
#include "libGLESv2/ProgramBinary.h"
#include "libGLESv2/renderer/Renderer.h"
#include "libGLESv2/renderer/ShaderExecutable.h"
#include "libGLESv2/renderer/d3d/DynamicHLSL.h"
#include "libGLESv2/main.h"

namespace rx
{

ProgramD3D::ProgramD3D(rx::Renderer *renderer)
    : ProgramImpl(),
      mRenderer(renderer),
      mDynamicHLSL(NULL),
      mVertexUniformStorage(NULL),
      mFragmentUniformStorage(NULL)
{
    mDynamicHLSL = new rx::DynamicHLSL(renderer);
}

ProgramD3D::~ProgramD3D()
{
    reset();
    SafeDelete(mDynamicHLSL);
}

ProgramD3D *ProgramD3D::makeProgramD3D(ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(ProgramD3D*, impl));
    return static_cast<ProgramD3D*>(impl);
}

const ProgramD3D *ProgramD3D::makeProgramD3D(const ProgramImpl *impl)
{
    ASSERT(HAS_DYNAMIC_TYPE(const ProgramD3D*, impl));
    return static_cast<const ProgramD3D*>(impl);
}

void ProgramD3D::initializeUniformStorage(const std::vector<gl::LinkedUniform*> &uniforms)
{
    // Compute total default block size
    unsigned int vertexRegisters = 0;
    unsigned int fragmentRegisters = 0;
    for (size_t uniformIndex = 0; uniformIndex < uniforms.size(); uniformIndex++)
    {
        const gl::LinkedUniform &uniform = *uniforms[uniformIndex];

        if (!gl::IsSampler(uniform.type))
        {
            if (uniform.isReferencedByVertexShader())
            {
                vertexRegisters = std::max(vertexRegisters, uniform.vsRegisterIndex + uniform.registerCount);
            }
            if (uniform.isReferencedByFragmentShader())
            {
                fragmentRegisters = std::max(fragmentRegisters, uniform.psRegisterIndex + uniform.registerCount);
            }
        }
    }

    mVertexUniformStorage = mRenderer->createUniformStorage(vertexRegisters * 16u);
    mFragmentUniformStorage = mRenderer->createUniformStorage(fragmentRegisters * 16u);
}

void ProgramD3D::reset()
{
    SafeDelete(mVertexUniformStorage);
    SafeDelete(mFragmentUniformStorage);
}

}

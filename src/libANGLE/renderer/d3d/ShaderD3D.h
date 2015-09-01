//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderD3D.h: Defines the rx::ShaderD3D class which implements rx::ShaderImpl.

#ifndef LIBANGLE_RENDERER_D3D_SHADERD3D_H_
#define LIBANGLE_RENDERER_D3D_SHADERD3D_H_

#include "libANGLE/renderer/d3d/WorkaroundsD3D.h"
#include "libANGLE/renderer/ShaderImpl.h"
#include "libANGLE/Shader.h"

#include <map>

namespace rx
{
class DynamicHLSL;
class RendererD3D;

struct PackedVarying
{
    PackedVarying(const sh::Varying &varyingIn)
        : varying(&varyingIn), registerIndex(GL_INVALID_INDEX), columnIndex(0)
    {
    }

    bool registerAssigned() const { return registerIndex != GL_INVALID_INDEX; }

    void resetRegisterAssignment() { registerIndex = GL_INVALID_INDEX; }

    const sh::Varying *varying;

    // TODO(jmadill): Make these D3D-only or otherwise clearly separate from GL.
    unsigned int registerIndex;  // Assigned during link
    unsigned int columnIndex;    // Assigned during link, defaults to 0
};

class ShaderD3D : public ShaderImpl
{
    friend class DynamicHLSL;

  public:
    ShaderD3D(GLenum type, RendererD3D *renderer);
    virtual ~ShaderD3D();

    // ShaderImpl implementation
    virtual std::string getDebugInfo() const;

    // D3D-specific methods
    virtual void uncompile();
    void resetVaryingsRegisterAssignment();
    unsigned int getUniformRegister(const std::string &uniformName) const;
    unsigned int getInterfaceBlockRegister(const std::string &blockName) const;
    void appendDebugInfo(const std::string &info) { mDebugInfo += info; }

    void generateWorkarounds(D3DCompilerWorkarounds *workarounds) const;
    int getShaderVersion() const { return mShaderVersion; }
    bool usesDepthRange() const { return mUsesDepthRange; }
    bool usesPointSize() const { return mUsesPointSize; }
    bool usesDeferredInit() const { return mUsesDeferredInit; }
    bool usesFrontFacing() const { return mUsesFrontFacing; }

    GLenum getShaderType() const;
    ShShaderOutput getCompilerOutputType() const;

    virtual bool compile(gl::Compiler *compiler, const std::string &source);

    const std::vector<PackedVarying> &getPackedVaryings() const { return mPackedVaryings; }

    // TODO(jmadill): remove this
    std::vector<PackedVarying> &getPackedVaryings() { return mPackedVaryings; }

  private:
    void compileToHLSL(ShHandle compiler, const std::string &source);
    void parseVaryings(ShHandle compiler);

    void parseAttributes(ShHandle compiler);

    GLenum mShaderType;

    bool mUsesMultipleRenderTargets;
    bool mUsesFragColor;
    bool mUsesFragData;
    bool mUsesFragCoord;
    bool mUsesFrontFacing;
    bool mUsesPointSize;
    bool mUsesPointCoord;
    bool mUsesDepthRange;
    bool mUsesFragDepth;
    bool mUsesDiscardRewriting;
    bool mUsesNestedBreak;
    bool mUsesDeferredInit;
    bool mRequiresIEEEStrictCompiling;

    ShShaderOutput mCompilerOutputType;
    std::string mDebugInfo;
    std::map<std::string, unsigned int> mUniformRegisterMap;
    std::map<std::string, unsigned int> mInterfaceBlockRegisterMap;
    RendererD3D *mRenderer;
    std::vector<PackedVarying> mPackedVaryings;
};

}

#endif // LIBANGLE_RENDERER_D3D_SHADERD3D_H_

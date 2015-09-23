//
// Copyright (c) 2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderD3D.h: Defines the rx::ShaderD3D class which implements rx::ShaderImpl.

#ifndef LIBANGLE_RENDERER_D3D_SHADERD3D_H_
#define LIBANGLE_RENDERER_D3D_SHADERD3D_H_

#include "libANGLE/renderer/ShaderSh.h"

#include <map>

namespace rx
{
class DynamicHLSL;
class RendererD3D;
struct D3DCompilerWorkarounds;

class ShaderD3D : public ShaderSh
{
    friend class DynamicHLSL;

  public:
    ShaderD3D(GLenum type, const gl::Limitations &limitations);
    virtual ~ShaderD3D();

    // ShaderImpl implementation
    bool compile(gl::Compiler *compiler, const std::string &source, int additionalOptions) override;
    std::string getDebugInfo() const override;

    // D3D-specific methods
    void uncompile();
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

  private:
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
};

}

#endif // LIBANGLE_RENDERER_D3D_SHADERD3D_H_

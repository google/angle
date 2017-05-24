//
// Copyright (c) 2012-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// ShaderExecutable11.h: Defines a D3D11-specific class to contain shader
// executable implementation details.

#ifndef LIBANGLE_RENDERER_D3D_D3D11_SHADEREXECUTABLE11_H_
#define LIBANGLE_RENDERER_D3D_D3D11_SHADEREXECUTABLE11_H_

#include "libANGLE/renderer/d3d/ShaderExecutableD3D.h"
#include "libANGLE/renderer/d3d/d3d11/ResourceManager11.h"

namespace rx
{
class Renderer11;
class UniformStorage11;

class ShaderExecutable11 : public ShaderExecutableD3D
{
  public:
    ShaderExecutable11(const void *function, size_t length, ID3D11PixelShader *executable);
    ShaderExecutable11(const void *function, size_t length, ID3D11VertexShader *executable, ID3D11GeometryShader *streamOut);
    ShaderExecutable11(const void *function, size_t length, ID3D11GeometryShader *executable);
    ShaderExecutable11(const void *function, size_t length, ID3D11ComputeShader *executable);

    virtual ~ShaderExecutable11();

    ID3D11PixelShader *getPixelShader() const;
    ID3D11VertexShader *getVertexShader() const;
    ID3D11GeometryShader *getGeometryShader() const;
    ID3D11GeometryShader *getStreamOutShader() const;
    ID3D11ComputeShader *getComputeShader() const;

  private:
    ID3D11PixelShader *mPixelExecutable;
    ID3D11VertexShader *mVertexExecutable;
    ID3D11GeometryShader *mGeometryExecutable;
    ID3D11GeometryShader *mStreamOutExecutable;
    ID3D11ComputeShader *mComputeExecutable;
};

class UniformStorage11 : public UniformStorageD3D
{
  public:
    UniformStorage11(size_t initialSize);
    virtual ~UniformStorage11();

    gl::Error getConstantBuffer(Renderer11 *renderer, const d3d11::Buffer **bufferOut);

  private:
    d3d11::Buffer mConstantBuffer;
};

}  // namespace rx

#endif // LIBANGLE_RENDERER_D3D_D3D11_SHADEREXECUTABLE11_H_

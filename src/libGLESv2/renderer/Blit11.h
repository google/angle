//
// Copyright (c) 2013 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Blit11.cpp: Texture copy utility class.

#ifndef LIBGLESV2_BLIT11_H_
#define LIBGLESV2_BLIT11_H_

#include "common/angleutils.h"
#include "libGLESv2/angletypes.h"

namespace rx
{
class Renderer11;

enum Filter
{
    Point,
    Linear,
};

class Blit11
{
  public:
    explicit Blit11(Renderer11 *renderer);
    ~Blit11();

    bool copyTexture(ID3D11ShaderResourceView *source, const gl::Box &sourceArea, const gl::Extents &sourceSize,
                     ID3D11RenderTargetView *dest, const gl::Box &destArea, const gl::Extents &destSize,
                     GLenum destFormat, GLenum filter);

  private:
    rx::Renderer11 *mRenderer;

    struct BlitParameters
    {
        GLenum mDestinationFormat;
        bool mSignedInteger;
        bool m3DBlit;
    };

    static bool compareBlitParameters(const BlitParameters &a, const BlitParameters &b);

    typedef void (*WriteVertexFunction)(const gl::Box &sourceArea, const gl::Extents &sourceSize,
                                        const gl::Box &destArea, const gl::Extents &destSize,
                                        void *outVertices, unsigned int *outStride, unsigned int *outVertexCount,
                                        D3D11_PRIMITIVE_TOPOLOGY *outTopology);

    struct BlitShader
    {
        WriteVertexFunction mVertexWriteFunction;
        ID3D11InputLayout *mInputLayout;
        ID3D11VertexShader *mVertexShader;
        ID3D11GeometryShader *mGeometryShader;
        ID3D11PixelShader *mPixelShader;
    };

    typedef bool (*BlitParametersComparisonFunction)(const BlitParameters&, const BlitParameters &);
    typedef std::map<BlitParameters, BlitShader, BlitParametersComparisonFunction> BlitShaderMap;
    BlitShaderMap mShaderMap;

    void add2DShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps);
    void add3DShaderToMap(GLenum destFormat, bool signedInteger, ID3D11PixelShader *ps);

    void buildShaderMap();
    void clearShaderMap();

    ID3D11Buffer *mVertexBuffer;
    ID3D11SamplerState *mPointSampler;
    ID3D11SamplerState *mLinearSampler;

    ID3D11InputLayout *mQuad2DIL;
    ID3D11VertexShader *mQuad2DVS;

    ID3D11InputLayout *mQuad3DIL;
    ID3D11VertexShader *mQuad3DVS;
    ID3D11GeometryShader *mQuad3DGS;

    DISALLOW_COPY_AND_ASSIGN(Blit11);
};
}

#endif   // LIBGLESV2_BLIT11_H_

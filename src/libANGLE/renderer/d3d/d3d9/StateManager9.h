//
// Copyright (c) 2015 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// StateManager9.h: Defines a class for caching D3D9 state

#ifndef LIBANGLE_RENDERER_D3D9_STATEMANAGER9_H_
#define LIBANGLE_RENDERER_D3D9_STATEMANAGER9_H_

#include "libANGLE/renderer/d3d/StateManagerD3D.h"

namespace rx
{

class StateManager9 final : public StateManagerD3D
{
  public:
    StateManager9(IDirect3DDevice9 *device, D3DADAPTER_IDENTIFIER9 &adapterIdentifier);

    ~StateManager9() override;

    gl::Error setBlendState(const gl::Framebuffer *framebuffer,
                            const gl::BlendState &blendState,
                            const gl::ColorF &blendColor,
                            unsigned int sampleMask,
                            const gl::State::DirtyBits &dirtyBits) override;

    gl::Error setDepthStencilState(const gl::DepthStencilState &depthStencilState,
                                   int stencilRef,
                                   int stencilBackRef,
                                   bool frontFaceCCW,
                                   const gl::State::DirtyBits &dirtyBits) override;

    gl::Error setRasterizerState(const gl::RasterizerState &rasterizerState,
                                 const gl::State::DirtyBits &dirtyBits) override;

    void setCurDepthSize(unsigned int size);
    unsigned int getCurDepthSize() const;

  private:
    static const D3DRENDERSTATETYPE D3DRS_CCW_STENCILREF       = D3DRS_STENCILREF;
    static const D3DRENDERSTATETYPE D3DRS_CCW_STENCILMASK      = D3DRS_STENCILMASK;
    static const D3DRENDERSTATETYPE D3DRS_CCW_STENCILWRITEMASK = D3DRS_STENCILWRITEMASK;

    VendorID getVendorId() const;

    // Blend state setting functions
    void setDepthMask(bool depthMask);
    void setDepthTestAndFunc(bool depthTest, GLenum depthFunc);
    void setStencilTestEnabled(bool stencilTest);
    void setStencilFuncsFront(GLenum stencilFunc,
                              GLuint stencilMask,
                              int stencilRef,
                              unsigned int maxStencil,
                              bool frontFaceCCW);
    void setStencilFuncsBack(GLenum stencilBackFunc,
                             GLuint stencilBackMask,
                             int stencilBackRef,
                             unsigned int maxStencil,
                             bool frontFaceCCW);

    void setStencilWriteMaskFront(GLuint stencilWritemask, bool frontFaceCCW);
    void setStencilWriteMaskBack(GLuint stencilBackWritemask, bool frontFaceCCW);
    void setStencilOpsFront(GLenum stencilFail,
                            GLenum stencilPassDepthFail,
                            GLenum stencilPassDepthPass,
                            bool frontFaceCCW);
    void setStencilOpsBack(GLenum stencilBackFail,
                           GLenum stencilBackPassDepthFail,
                           GLenum stencilBackPassDepthPass,
                           bool frontFaceCCW);

    // Depth stencil state setting functions
    void setBlendEnableFuncsEquations(const gl::BlendState &blendState,
                                      const gl::ColorF &blendColor);
    void setBlendColor(const gl::ColorF &blendColor, const gl::BlendState &blendState);
    void setBlendEnabled(bool blendEnabled);
    void setSampleAlphaToCoverageEnabled(bool sampleAlphaToCoverage);
    void setDitherEnabled(bool ditherEnabled);
    void setBlendColorMask(const gl::BlendState &blendState, const gl::Framebuffer *framebuffer);
    void setSampleMask(unsigned int sampleMask);

    // Rasterizer state setting functions
    void setRasterizerMode(bool cullFace, GLenum cullMode, GLenum frontFace);
    void setRasterizerPolygonOffset(bool polygonOffsetFill,
                                    GLfloat polygonOffsetFactor,
                                    GLfloat polygonOffsetUnits);

    IDirect3DDevice9 *mDevice;
    const D3DADAPTER_IDENTIFIER9 &mAdapterIdentifier;

    unsigned int mCurDepthSize;
    bool mCurFrontFaceCCW;
};

}  // namespace rx
#endif
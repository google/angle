//
// Copyright 2017 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
// ConformanceTests.cpp:
//   GLES1 conformance tests.
//   Function prototypes taken from tproto.h and turned into gtest tests using a macro.
//

#include "test_utils/ANGLETest.h"
#include "test_utils/gl_raii.h"

#ifdef __cplusplus
extern "C" {
#endif

// ES 1.0
extern long AmbLightExec(void);
extern long AmbMatExec(void);
extern long AmbSceneExec(void);
extern long APFuncExec(void);
extern long AtnConstExec(void);
extern long AtnPosExec(void);
extern long BClearExec(void);
extern long BColorExec(void);
extern long BCornerExec(void);
extern long BlendExec(void);
extern long ClipExec(void);
extern long ColRampExec(void);
extern long CopyTexExec(void);
extern long DifLightExec(void);
extern long DifMatExec(void);
extern long DifMatNormExec(void);
extern long DifMatPosExec(void);
extern long DitherExec(void);
extern long DivZeroExec(void);
extern long EmitMatExec(void);
extern long FogExpExec(void);
extern long FogLinExec(void);
extern long LineAntiAliasExec(void);
extern long LineHVExec(void);
extern long LineRasterExec(void);
extern long LogicOpExec(void);
extern long MipExec(void);
extern long MipLevelsExec(void);
extern long MipLinExec(void);
extern long MipSelectExec(void);
extern long MaskExec(void);
extern long MatrixStackExec(void);
extern long MultiTexExec(void);
extern long MustPassExec(void);
extern long PackedPixelsExec(void);
extern long PointAntiAliasExec(void);
extern long PointRasterExec(void);
extern long PolyCullExec(void);
extern long ReadFormatExec(void);
extern long RescaleNormalExec(void);
extern long ScissorExec(void);
extern long SPClearExec(void);
extern long SPCornerExec(void);
extern long SpecExpExec(void);
extern long SpecExpNormExec(void);
extern long SpecLightExec(void);
extern long SpecMatExec(void);
extern long SpecNormExec(void);
extern long SPFuncExec(void);
extern long SPOpExec(void);
extern long SpotPosExec(void);
extern long SpotExpPosExec(void);
extern long SpotExpDirExec(void);
extern long TexDecalExec(void);
extern long TexPaletExec(void);
extern long TextureEdgeClampExec(void);
extern long TriRasterExec(void);
extern long TriTileExec(void);
extern long VertexOrderExec(void);
extern long ViewportClampExec(void);
extern long XFormExec(void);
extern long XFormMixExec(void);
extern long XFormNormalExec(void);
extern long XFormViewportExec(void);
extern long XFormHomogenousExec(void);
extern long ZBClearExec(void);
extern long ZBFuncExec(void);

// GL_OES_draw_texture
extern long DrawTexExec(void);

// GL_OES_query_matrix
extern long MatrixQueryExec(void);

// ES 1.1
extern long BufferObjectExec(void);
extern long PointSizeArrayExec(void);
extern long PointSpriteExec(void);
extern long UserClipExec(void);
extern long MatrixGetTestExec(void);
extern long GetsExec(void);
extern long TexCombineExec(void);

// GL_OES_matrix_palette
extern long MatrixPaletteExec(void);

// Test driver setup
extern void ExtTestDriverSetup(void);

#define CONFORMANCE_TEST_ERROR (-1)

#ifdef __cplusplus
}

#endif
namespace angle
{
class GLES1ConformanceTest : public ANGLETest
{
  protected:
    GLES1ConformanceTest()
    {
        setWindowWidth(48);
        setWindowHeight(48);
        setConfigRedBits(8);
        setConfigGreenBits(8);
        setConfigBlueBits(8);
        setConfigAlphaBits(8);
        setConfigDepthBits(24);
        setConfigStencilBits(8);
    }

    void SetUp() override
    {
        ANGLETest::SetUp();
        ExtTestDriverSetup();
    }
};

TEST_P(GLES1ConformanceTest, AmbLight)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, AmbLightExec());
}

TEST_P(GLES1ConformanceTest, AmbMat)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, AmbMatExec());
}

TEST_P(GLES1ConformanceTest, AmbScene)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, AmbSceneExec());
}

TEST_P(GLES1ConformanceTest, APFunc)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, APFuncExec());
}

TEST_P(GLES1ConformanceTest, AtnConst)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, AtnConstExec());
}

TEST_P(GLES1ConformanceTest, AtnPos)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, AtnPosExec());
}

TEST_P(GLES1ConformanceTest, BClear)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, BClearExec());
}

TEST_P(GLES1ConformanceTest, BColor)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, BColorExec());
}

TEST_P(GLES1ConformanceTest, BCorner)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, BCornerExec());
}

TEST_P(GLES1ConformanceTest, Blend)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, BlendExec());
}

TEST_P(GLES1ConformanceTest, Clip)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ClipExec());
}

TEST_P(GLES1ConformanceTest, ColRamp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ColRampExec());
}

TEST_P(GLES1ConformanceTest, CopyTex)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, CopyTexExec());
}

TEST_P(GLES1ConformanceTest, DifLight)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DifLightExec());
}

TEST_P(GLES1ConformanceTest, DifMat)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DifMatExec());
}

TEST_P(GLES1ConformanceTest, DifMatNorm)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DifMatNormExec());
}

TEST_P(GLES1ConformanceTest, DifMatPos)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DifMatPosExec());
}

TEST_P(GLES1ConformanceTest, Dither)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DitherExec());
}

TEST_P(GLES1ConformanceTest, DivZero)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DivZeroExec());
}

TEST_P(GLES1ConformanceTest, EmitMat)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, EmitMatExec());
}

TEST_P(GLES1ConformanceTest, FogExp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, FogExpExec());
}

TEST_P(GLES1ConformanceTest, FogLin)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, FogLinExec());
}

TEST_P(GLES1ConformanceTest, LineAntiAlias)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, LineAntiAliasExec());
}

TEST_P(GLES1ConformanceTest, LineHV)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, LineHVExec());
}

TEST_P(GLES1ConformanceTest, LineRaster)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, LineRasterExec());
}

TEST_P(GLES1ConformanceTest, LogicOp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, LogicOpExec());
}

TEST_P(GLES1ConformanceTest, Mip)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MipExec());
}

TEST_P(GLES1ConformanceTest, MipLevels)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MipLevelsExec());
}

TEST_P(GLES1ConformanceTest, MipLin)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MipLinExec());
}

TEST_P(GLES1ConformanceTest, MipSelect)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MipSelectExec());
}

TEST_P(GLES1ConformanceTest, Mask)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MaskExec());
}

TEST_P(GLES1ConformanceTest, MatrixStack)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MatrixStackExec());
}

TEST_P(GLES1ConformanceTest, MultiTex)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MultiTexExec());
}

TEST_P(GLES1ConformanceTest, MustPass)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MustPassExec());
}

TEST_P(GLES1ConformanceTest, PackedPixels)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PackedPixelsExec());
}

TEST_P(GLES1ConformanceTest, PointAntiAlias)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PointAntiAliasExec());
}

TEST_P(GLES1ConformanceTest, PointRaster)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PointRasterExec());
}

TEST_P(GLES1ConformanceTest, PolyCull)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PolyCullExec());
}

TEST_P(GLES1ConformanceTest, ReadFormat)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ReadFormatExec());
}

TEST_P(GLES1ConformanceTest, RescaleNormal)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, RescaleNormalExec());
}

TEST_P(GLES1ConformanceTest, Scissor)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ScissorExec());
}

TEST_P(GLES1ConformanceTest, SPClear)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SPClearExec());
}

TEST_P(GLES1ConformanceTest, SPCorner)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SPCornerExec());
}

TEST_P(GLES1ConformanceTest, SpecExp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpecExpExec());
}

TEST_P(GLES1ConformanceTest, SpecExpNorm)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpecExpNormExec());
}

TEST_P(GLES1ConformanceTest, SpecLight)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpecLightExec());
}

TEST_P(GLES1ConformanceTest, SpecMat)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpecMatExec());
}

TEST_P(GLES1ConformanceTest, SpecNorm)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpecNormExec());
}

TEST_P(GLES1ConformanceTest, SPFunc)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SPFuncExec());
}

TEST_P(GLES1ConformanceTest, SPOp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SPOpExec());
}

TEST_P(GLES1ConformanceTest, SpotPos)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpotPosExec());
}

TEST_P(GLES1ConformanceTest, SpotExpPos)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpotExpPosExec());
}

TEST_P(GLES1ConformanceTest, SpotExpDir)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, SpotExpDirExec());
}

TEST_P(GLES1ConformanceTest, TexDecal)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TexDecalExec());
}

TEST_P(GLES1ConformanceTest, TexPalet)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TexPaletExec());
}

TEST_P(GLES1ConformanceTest, TextureEdgeClamp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TextureEdgeClampExec());
}

TEST_P(GLES1ConformanceTest, TriRaster)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TriRasterExec());
}

TEST_P(GLES1ConformanceTest, TriTile)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TriTileExec());
}

TEST_P(GLES1ConformanceTest, VertexOrder)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, VertexOrderExec());
}

TEST_P(GLES1ConformanceTest, ViewportClamp)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ViewportClampExec());
}

TEST_P(GLES1ConformanceTest, XForm)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, XFormExec());
}

TEST_P(GLES1ConformanceTest, XFormMix)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, XFormMixExec());
}

TEST_P(GLES1ConformanceTest, XFormNormal)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, XFormNormalExec());
}

TEST_P(GLES1ConformanceTest, XFormViewport)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, XFormViewportExec());
}

TEST_P(GLES1ConformanceTest, XFormHomogenous)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, XFormHomogenousExec());
}

TEST_P(GLES1ConformanceTest, ZBClear)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ZBClearExec());
}

TEST_P(GLES1ConformanceTest, ZBFunc)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, ZBFuncExec());
}

TEST_P(GLES1ConformanceTest, DrawTex)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, DrawTexExec());
}

TEST_P(GLES1ConformanceTest, MatrixQuery)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MatrixQueryExec());
}

TEST_P(GLES1ConformanceTest, BufferObject)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, BufferObjectExec());
}

TEST_P(GLES1ConformanceTest, PointSizeArray)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PointSizeArrayExec());
}

TEST_P(GLES1ConformanceTest, PointSprite)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, PointSpriteExec());
}

TEST_P(GLES1ConformanceTest, UserClip)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, UserClipExec());
}

TEST_P(GLES1ConformanceTest, MatrixGetTest)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MatrixGetTestExec());
}

TEST_P(GLES1ConformanceTest, Gets)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, GetsExec());
}

TEST_P(GLES1ConformanceTest, TexCombine)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, TexCombineExec());
}

TEST_P(GLES1ConformanceTest, MatrixPalette)
{
    ANGLE_SKIP_TEST_IF(true);
    ASSERT_NE(CONFORMANCE_TEST_ERROR, MatrixPaletteExec());
}

ANGLE_INSTANTIATE_TEST(GLES1ConformanceTest, ES1_OPENGL());
}

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// GLES1Renderer.h: Defines GLES1 emulation rendering operations on top of a GLES3
// context. Used by Context.h.

#ifndef LIBANGLE_GLES1_RENDERER_H_
#define LIBANGLE_GLES1_RENDERER_H_

#include "angle_gl.h"
#include "common/angleutils.h"
#include "libANGLE/angletypes.h"

#include <memory>
#include <string>
#include <unordered_map>

namespace gl
{
class Context;
class GLES1State;
class Program;
class State;
class Shader;
class ShaderProgramManager;

enum class GLES1StateEnables : uint64_t
{
    Lighting                  = 0,
    Fog                       = 1,
    ClipPlanes                = 2,
    DrawTexture               = 3,
    PointRasterization        = 4,
    PointSprite               = 5,
    RescaleNormal             = 6,
    Normalize                 = 7,
    AlphaTest                 = 8,
    ShadeModelFlat            = 9,
    ColorMaterial             = 10,
    LightModelTwoSided        = 11,
    Tex2d0                    = 12,
    Tex2d1                    = 13,
    Tex2d2                    = 14,
    Tex2d3                    = 15,
    TexCube0                  = 16,
    TexCube1                  = 17,
    TexCube2                  = 18,
    TexCube3                  = 19,
    PointSpriteCoordReplaces0 = 20,
    PointSpriteCoordReplaces1 = 21,
    PointSpriteCoordReplaces2 = 22,
    PointSpriteCoordReplaces3 = 23,
    Light0                    = 24,
    Light1                    = 25,
    Light2                    = 26,
    Light3                    = 27,
    Light4                    = 28,
    Light5                    = 29,
    Light6                    = 30,
    Light7                    = 31,
    ClipPlane0                = 32,
    ClipPlane1                = 33,
    ClipPlane2                = 34,
    ClipPlane3                = 35,
    ClipPlane4                = 36,
    ClipPlane5                = 37,

    InvalidEnum = 38,
    EnumCount   = 38,
};

class GLES1Renderer final : angle::NonCopyable
{
  public:
    GLES1Renderer();
    ~GLES1Renderer();

    void onDestroy(Context *context, State *state);

    angle::Result prepareForDraw(PrimitiveMode mode, Context *context, State *glState);

    static int VertexArrayIndex(ClientVertexArrayType type, const GLES1State &gles1);
    static ClientVertexArrayType VertexArrayType(int attribIndex);
    static int TexCoordArrayIndex(unsigned int unit);

    void drawTexture(Context *context,
                     State *glState,
                     float x,
                     float y,
                     float z,
                     float width,
                     float height);

    static constexpr int kTexUnitCount = 4;

  private:
    using Mat4Uniform = float[16];
    using Vec4Uniform = float[4];
    using Vec3Uniform = float[3];

    Shader *getShader(ShaderProgramID handle) const;
    Program *getProgram(ShaderProgramID handle) const;

    angle::Result compileShader(Context *context,
                                ShaderType shaderType,
                                const char *src,
                                ShaderProgramID *shaderOut);
    angle::Result linkProgram(Context *context,
                              State *glState,
                              ShaderProgramID vshader,
                              ShaderProgramID fshader,
                              const angle::HashMap<GLint, std::string> &attribLocs,
                              ShaderProgramID *programOut);
    angle::Result initializeRendererProgram(Context *context, State *glState);

    void setUniform1i(Context *context,
                      Program *programObject,
                      UniformLocation location,
                      GLint value);
    void setUniform1iv(Context *context,
                       Program *programObject,
                       UniformLocation location,
                       GLint count,
                       const GLint *value);
    void setUniformMatrix4fv(Program *programObject,
                             UniformLocation location,
                             GLint count,
                             GLboolean transpose,
                             const GLfloat *value);
    void setUniform4fv(Program *programObject,
                       UniformLocation location,
                       GLint count,
                       const GLfloat *value);
    void setUniform3fv(Program *programObject,
                       UniformLocation location,
                       GLint count,
                       const GLfloat *value);
    void setUniform2fv(Program *programObject,
                       UniformLocation location,
                       GLint count,
                       const GLfloat *value);
    void setUniform1f(Program *programObject, UniformLocation location, GLfloat value);
    void setUniform1fv(Program *programObject,
                       UniformLocation location,
                       GLint count,
                       const GLfloat *value);

    void setAttributesEnabled(Context *context, State *glState, AttributesMask mask);

    static constexpr int kLightCount     = 8;
    static constexpr int kClipPlaneCount = 6;

    static constexpr int kVertexAttribIndex           = 0;
    static constexpr int kNormalAttribIndex           = 1;
    static constexpr int kColorAttribIndex            = 2;
    static constexpr int kPointSizeAttribIndex        = 3;
    static constexpr int kTextureCoordAttribIndexBase = 4;

    bool mRendererProgramInitialized;
    ShaderProgramManager *mShaderPrograms;

    using GLES1StateEnabledBitSet = angle::PackedEnumBitSet<GLES1StateEnables, uint64_t>;

    GLES1StateEnabledBitSet mGLES1StateEnabled;

    const char *getShaderBool(GLES1StateEnables state);
    void addShaderDefine(std::stringstream &outStream,
                         GLES1StateEnables state,
                         const char *enableString);
    void addVertexShaderDefs(std::stringstream &outStream);
    void addFragmentShaderDefs(std::stringstream &outStream);

    struct GLES1ProgramState
    {
        ShaderProgramID program;

        UniformLocation projMatrixLoc;
        UniformLocation modelviewMatrixLoc;
        UniformLocation textureMatrixLoc;
        UniformLocation modelviewInvTrLoc;

        // Texturing
        std::array<UniformLocation, kTexUnitCount> tex2DSamplerLocs;
        std::array<UniformLocation, kTexUnitCount> texCubeSamplerLocs;

        UniformLocation textureFormatLoc;

        UniformLocation textureEnvModeLoc;
        UniformLocation combineRgbLoc;
        UniformLocation combineAlphaLoc;
        UniformLocation src0rgbLoc;
        UniformLocation src0alphaLoc;
        UniformLocation src1rgbLoc;
        UniformLocation src1alphaLoc;
        UniformLocation src2rgbLoc;
        UniformLocation src2alphaLoc;
        UniformLocation op0rgbLoc;
        UniformLocation op0alphaLoc;
        UniformLocation op1rgbLoc;
        UniformLocation op1alphaLoc;
        UniformLocation op2rgbLoc;
        UniformLocation op2alphaLoc;
        UniformLocation textureEnvColorLoc;
        UniformLocation rgbScaleLoc;
        UniformLocation alphaScaleLoc;

        // Alpha test
        UniformLocation alphaFuncLoc;
        UniformLocation alphaTestRefLoc;

        // Shading, materials, and lighting
        UniformLocation materialAmbientLoc;
        UniformLocation materialDiffuseLoc;
        UniformLocation materialSpecularLoc;
        UniformLocation materialEmissiveLoc;
        UniformLocation materialSpecularExponentLoc;

        UniformLocation lightModelSceneAmbientLoc;

        UniformLocation lightAmbientsLoc;
        UniformLocation lightDiffusesLoc;
        UniformLocation lightSpecularsLoc;
        UniformLocation lightPositionsLoc;
        UniformLocation lightDirectionsLoc;
        UniformLocation lightSpotlightExponentsLoc;
        UniformLocation lightSpotlightCutoffAnglesLoc;
        UniformLocation lightAttenuationConstsLoc;
        UniformLocation lightAttenuationLinearsLoc;
        UniformLocation lightAttenuationQuadraticsLoc;

        // Fog
        UniformLocation fogModeLoc;
        UniformLocation fogDensityLoc;
        UniformLocation fogStartLoc;
        UniformLocation fogEndLoc;
        UniformLocation fogColorLoc;

        // Clip planes
        UniformLocation clipPlanesLoc;

        // Point rasterization
        UniformLocation pointSizeMinLoc;
        UniformLocation pointSizeMaxLoc;
        UniformLocation pointDistanceAttenuationLoc;

        // Draw texture
        UniformLocation drawTextureCoordsLoc;
        UniformLocation drawTextureDimsLoc;
        UniformLocation drawTextureNormalizedCropRectLoc;
    };

    struct GLES1UniformBuffers
    {
        std::array<Mat4Uniform, kTexUnitCount> textureMatrices;
        std::array<GLint, kTexUnitCount> tex2DEnables;
        std::array<GLint, kTexUnitCount> texCubeEnables;

        std::array<GLint, kTexUnitCount> texEnvModes;
        std::array<GLint, kTexUnitCount> texCombineRgbs;
        std::array<GLint, kTexUnitCount> texCombineAlphas;

        std::array<GLint, kTexUnitCount> texCombineSrc0Rgbs;
        std::array<GLint, kTexUnitCount> texCombineSrc0Alphas;
        std::array<GLint, kTexUnitCount> texCombineSrc1Rgbs;
        std::array<GLint, kTexUnitCount> texCombineSrc1Alphas;
        std::array<GLint, kTexUnitCount> texCombineSrc2Rgbs;
        std::array<GLint, kTexUnitCount> texCombineSrc2Alphas;
        std::array<GLint, kTexUnitCount> texCombineOp0Rgbs;
        std::array<GLint, kTexUnitCount> texCombineOp0Alphas;
        std::array<GLint, kTexUnitCount> texCombineOp1Rgbs;
        std::array<GLint, kTexUnitCount> texCombineOp1Alphas;
        std::array<GLint, kTexUnitCount> texCombineOp2Rgbs;
        std::array<GLint, kTexUnitCount> texCombineOp2Alphas;
        std::array<Vec4Uniform, kTexUnitCount> texEnvColors;
        std::array<GLfloat, kTexUnitCount> texEnvRgbScales;
        std::array<GLfloat, kTexUnitCount> texEnvAlphaScales;

        // Lighting
        std::array<Vec4Uniform, kLightCount> lightAmbients;
        std::array<Vec4Uniform, kLightCount> lightDiffuses;
        std::array<Vec4Uniform, kLightCount> lightSpeculars;
        std::array<Vec4Uniform, kLightCount> lightPositions;
        std::array<Vec3Uniform, kLightCount> lightDirections;
        std::array<GLfloat, kLightCount> spotlightExponents;
        std::array<GLfloat, kLightCount> spotlightCutoffAngles;
        std::array<GLfloat, kLightCount> attenuationConsts;
        std::array<GLfloat, kLightCount> attenuationLinears;
        std::array<GLfloat, kLightCount> attenuationQuadratics;

        // Clip planes
        std::array<Vec4Uniform, kClipPlaneCount> clipPlanes;

        // Texture crop rectangles
        std::array<Vec4Uniform, kTexUnitCount> texCropRects;
    };

    angle::HashMap<uint64_t, GLES1UniformBuffers> mUniformBuffers;
    angle::HashMap<uint64_t, GLES1ProgramState> mProgramStates;

    bool mDrawTextureEnabled      = false;
    GLfloat mDrawTextureCoords[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    GLfloat mDrawTextureDims[2]   = {0.0f, 0.0f};
};

}  // namespace gl

#endif  // LIBANGLE_GLES1_RENDERER_H_

//
// Copyright 2018 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// GLES1State.cpp: Implements the GLES1State class, tracking state
// for GLES1 contexts.

#include "libANGLE/GLES1State.h"

#include "libANGLE/Context.h"

namespace gl
{

GLES1State::GLES1State()
    : mVertexArrayEnabled(false),
      mNormalArrayEnabled(false),
      mColorArrayEnabled(false),
      mPointSizeArrayEnabled(false),
      mLineSmoothEnabled(false),
      mPointSmoothEnabled(false),
      mPointSpriteEnabled(false),
      mAlphaTestEnabled(false),
      mLogicOpEnabled(false),
      mLightingEnabled(false),
      mFogEnabled(false),
      mRescaleNormalEnabled(false),
      mNormalizeEnabled(false),
      mColorMaterialEnabled(false),
      mReflectionMapEnabled(false),
      mCurrentColor({0.0f, 0.0f, 0.0f, 0.0f}),
      mCurrentNormal({0.0f, 0.0f, 0.0f}),
      mCurrMatrixMode(MatrixType::Modelview),
      mShadeModel(ShadingModel::Smooth),
      mAlphaTestFunc(AlphaTestFunc::AlwaysPass),
      mAlphaTestRef(0.0f),
      mLogicOp(LogicalOperation::Copy),
      mLineSmoothHint(HintSetting::DontCare),
      mPointSmoothHint(HintSetting::DontCare),
      mPerspectiveCorrectionHint(HintSetting::DontCare),
      mFogHint(HintSetting::DontCare)
{
}

GLES1State::~GLES1State() = default;

// Taken from the GLES 1.x spec which specifies all initial state values.
void GLES1State::initialize(const Context *context)
{
    const Caps &caps = context->getCaps();

    mTexUnitEnables.resize(caps.maxMultitextureUnits);
    for (auto &enables : mTexUnitEnables)
    {
        enables.enable2D      = false;
        enables.enableCubeMap = false;
    }

    mVertexArrayEnabled    = false;
    mNormalArrayEnabled    = false;
    mColorArrayEnabled     = false;
    mPointSizeArrayEnabled = false;
    mTexCoordArrayEnabled.resize(caps.maxMultitextureUnits, false);

    mLineSmoothEnabled    = false;
    mPointSmoothEnabled   = false;
    mPointSpriteEnabled   = false;
    mLogicOpEnabled       = false;
    mAlphaTestEnabled     = false;
    mLightingEnabled      = false;
    mFogEnabled           = false;
    mRescaleNormalEnabled = false;
    mNormalizeEnabled     = false;
    mColorMaterialEnabled = false;
    mReflectionMapEnabled = false;

    mCurrMatrixMode = MatrixType::Modelview;

    mCurrentColor  = {1.0f, 1.0f, 1.0f, 1.0f};
    mCurrentNormal = {0.0f, 0.0f, 1.0f};

    mCurrentTextureCoords.resize(caps.maxMultitextureUnits);

    mTextureEnvironments.resize(caps.maxMultitextureUnits);

    mProjMatrices.resize(caps.maxProjectionMatrixStackDepth);
    mModelviewMatrices.resize(caps.maxModelviewMatrixStackDepth);
    mTextureMatrices.resize(caps.maxMultitextureUnits);
    for (auto &textureMatrixStack : mTextureMatrices)
    {
        textureMatrixStack.resize(caps.maxTextureMatrixStackDepth);
    }

    mMaterial.ambient  = {0.2f, 0.2f, 0.2f, 1.0f};
    mMaterial.diffuse  = {0.8f, 0.8f, 0.8f, 1.0f};
    mMaterial.specular = {0.0f, 0.0f, 0.0f, 1.0f};
    mMaterial.emissive = {0.0f, 0.0f, 0.0f, 1.0f};

    mMaterial.specularExponent = 0.0f;

    mLightModel.color    = {0.2f, 0.2f, 0.2f, 1.0f};
    mLightModel.twoSided = false;

    mLights.resize(caps.maxLights);

    // GL_LIGHT0 is special and has default state that avoids all-black renderings.
    mLights[0].diffuse  = {1.0f, 1.0f, 1.0f, 1.0f};
    mLights[0].specular = {1.0f, 1.0f, 1.0f, 1.0f};

    mFog.mode    = FogMode::Exp;
    mFog.density = 1.0f;
    mFog.start   = 0.0f;
    mFog.end     = 1.0f;

    mFog.color = {0.0f, 0.0f, 0.0f, 0.0f};

    mShadeModel = ShadingModel::Smooth;

    mAlphaTestFunc = AlphaTestFunc::AlwaysPass;
    mAlphaTestRef  = 0.0f;

    mLogicOp = LogicalOperation::Copy;

    mClipPlaneEnabled.resize(caps.maxClipPlanes, false);

    mClipPlanes.resize(caps.maxClipPlanes, angle::Vector4(0.0f, 0.0f, 0.0f, 0.0f));

    mPointParameters.pointSizeMin                = 0.1f;
    mPointParameters.pointSizeMax                = 100.0f;
    mPointParameters.pointFadeThresholdSize      = 0.1f;
    mPointParameters.pointDistanceAttenuation[0] = 1.0f;
    mPointParameters.pointDistanceAttenuation[1] = 0.0f;
    mPointParameters.pointDistanceAttenuation[2] = 0.0f;

    mPointParameters.pointSize = 1.0f;

    mLineSmoothHint            = HintSetting::DontCare;
    mPointSmoothHint           = HintSetting::DontCare;
    mPerspectiveCorrectionHint = HintSetting::DontCare;
    mFogHint                   = HintSetting::DontCare;
}

void GLES1State::setAlphaFunc(AlphaTestFunc func, GLfloat ref)
{
    mAlphaTestFunc = func;
    mAlphaTestRef  = ref;
}

}  // namespace gl

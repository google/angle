//
// Copyright (c) 2002-2014 The ANGLE Project Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

//
// Create symbols that declare built-in definitions, add built-ins that
// cannot be expressed in the files, and establish mappings between 
// built-in functions and operators.
//

#include "compiler/translator/Initialize.h"

#include "compiler/translator/IntermNode.h"
#include "angle_gl.h"

void InsertBuiltInFunctions(sh::GLenum type, ShShaderSpec spec, const ShBuiltInResources &resources, TSymbolTable &symbolTable)
{
    TType *float1 = new TType(EbtFloat);
    TType *float2 = new TType(EbtFloat, 2);
    TType *float3 = new TType(EbtFloat, 3);
    TType *float4 = new TType(EbtFloat, 4);
    TType *int1 = new TType(EbtInt);
    TType *int2 = new TType(EbtInt, 2);
    TType *int3 = new TType(EbtInt, 3);
    TType *uint1 = new TType(EbtUInt);
    TType *bool1 = new TType(EbtBool);
    TType *genType = new TType(EbtGenType);
    TType *genIType = new TType(EbtGenIType);
    TType *genUType = new TType(EbtGenUType);
    TType *genBType = new TType(EbtGenBType);

    //
    // Angle and Trigonometric Functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "radians", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "degrees", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "sin", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "cos", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "tan", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "asin", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "acos", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "atan", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "atan", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "sinh", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "cosh", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "tanh", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "asinh", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "acosh", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "atanh", genType);

    //
    // Exponential Functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "pow", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "exp", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "log", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "exp2", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "log2", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "sqrt", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "inversesqrt", genType);

    //
    // Common Functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "abs", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "abs", genIType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "sign", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "sign", genIType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "floor", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "ceil", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "fract", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "mod", genType, float1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "mod", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "min", genType, float1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "min", genType, genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "min", genIType, genIType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "min", genIType, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "min", genIType, genIType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "min", genUType, uint1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "max", genType, float1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "max", genType, genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "max", genIType, genIType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "max", genIType, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "max", genUType, genUType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "max", genUType, uint1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "clamp", genType, float1, float1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "clamp", genType, genType, genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "clamp", genIType, int1, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "clamp", genIType, genIType, genIType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "clamp", genUType, uint1, uint1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "clamp", genUType, genUType, genUType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "mix", genType, genType, float1);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "mix", genType, genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "step", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "step", float1, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "smoothstep", genType, genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "smoothstep", float1, float1, genType);

    TType *outFloat1 = new TType(EbtFloat);
    TType *outFloat2 = new TType(EbtFloat, 2);
    TType *outFloat3 = new TType(EbtFloat, 3);
    TType *outFloat4 = new TType(EbtFloat, 4);
    outFloat1->setQualifier(EvqOut);
    outFloat2->setQualifier(EvqOut);
    outFloat3->setQualifier(EvqOut);
    outFloat4->setQualifier(EvqOut);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "modf", float1, outFloat1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float2, "modf", float2, outFloat2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float3, "modf", float3, outFloat3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float4, "modf", float4, outFloat4);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genBType, "isnan", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genBType, "isinf", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genIType, "floatBitsToInt", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genUType, "floatBitsToUint", genType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "intBitsToFloat", genIType);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "uintBitsToFloat", genUType);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, uint1, "packSnorm2x16", float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, uint1, "packUnorm2x16", float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, uint1, "packHalf2x16", float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float2, "unpackSnorm2x16", uint1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float2, "unpackUnorm2x16", uint1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float2, "unpackHalf2x16", uint1);

    //
    // Geometric Functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, float1, "length", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, float1, "distance", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, float1, "dot", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, float3, "cross", float3, float3);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "normalize", genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "faceforward", genType, genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "reflect", genType, genType);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, genType, "refract", genType, genType, float1);

    TType *mat2 = new TType(EbtFloat, 2, 2);
    TType *mat3 = new TType(EbtFloat, 3, 3);
    TType *mat4 = new TType(EbtFloat, 4, 4);
    TType *mat2x3 = new TType(EbtFloat, 2, 3);
    TType *mat3x2 = new TType(EbtFloat, 3, 2);
    TType *mat2x4 = new TType(EbtFloat, 2, 4);
    TType *mat4x2 = new TType(EbtFloat, 4, 2);
    TType *mat3x4 = new TType(EbtFloat, 3, 4);
    TType *mat4x3 = new TType(EbtFloat, 4, 3);

    //
    // Matrix Functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, mat2, "matrixCompMult", mat2, mat2);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, mat3, "matrixCompMult", mat3, mat3);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, mat4, "matrixCompMult", mat4, mat4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x3, "matrixCompMult", mat2x3, mat2x3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x2, "matrixCompMult", mat3x2, mat3x2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x4, "matrixCompMult", mat2x4, mat2x4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x2, "matrixCompMult", mat4x2, mat4x2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x4, "matrixCompMult", mat3x4, mat3x4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x3, "matrixCompMult", mat4x3, mat4x3);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2, "outerProduct", float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3, "outerProduct", float3, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4, "outerProduct", float4, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x3, "outerProduct", float3, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x2, "outerProduct", float2, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x4, "outerProduct", float4, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x2, "outerProduct", float2, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x4, "outerProduct", float4, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x3, "outerProduct", float3, float4);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2, "transpose", mat2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3, "transpose", mat3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4, "transpose", mat4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x3, "transpose", mat3x2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x2, "transpose", mat2x3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2x4, "transpose", mat4x2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x2, "transpose", mat2x4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3x4, "transpose", mat4x3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4x3, "transpose", mat3x4);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "determinant", mat2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "determinant", mat3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "determinant", mat4);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat2, "inverse", mat2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat3, "inverse", mat3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, mat4, "inverse", mat4);

    TType *vec = new TType(EbtVec);
    TType *ivec = new TType(EbtIVec);
    TType *uvec = new TType(EbtUVec);
    TType *bvec = new TType(EbtBVec);

    //
    // Vector relational functions.
    //
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "lessThan", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "lessThan", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "lessThan", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "lessThanEqual", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "lessThanEqual", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "lessThanEqual", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "greaterThan", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "greaterThan", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "greaterThan", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "greaterThanEqual", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "greaterThanEqual", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "greaterThanEqual", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "equal", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "equal", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "equal", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "equal", bvec, bvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "notEqual", vec, vec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "notEqual", ivec, ivec);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, bvec, "notEqual", uvec, uvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "notEqual", bvec, bvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bool1, "any", bvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bool1, "all", bvec);
    symbolTable.insertBuiltIn(COMMON_BUILTINS, bvec, "not", bvec);

    TType *sampler2D = new TType(EbtSampler2D);
    TType *samplerCube = new TType(EbtSamplerCube);

    //
    // Texture Functions for GLSL ES 1.0
    //
    symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", sampler2D, float2);
    symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float3);
    symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float4);
    symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "textureCube", samplerCube, float3);

    if (resources.OES_EGL_image_external)
    {
        TType *samplerExternalOES = new TType(EbtSamplerExternalOES);

        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", samplerExternalOES, float2);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", samplerExternalOES, float3);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", samplerExternalOES, float4);
    }

    if (resources.ARB_texture_rectangle)
    {
        TType *sampler2DRect = new TType(EbtSampler2DRect);

        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRect", sampler2DRect, float2);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRectProj", sampler2DRect, float3);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DRectProj", sampler2DRect, float4);
    }

    if (resources.EXT_shader_texture_lod)
    {
        /* The *Grad* variants are new to both vertex and fragment shaders; the fragment
         * shader specific pieces are added separately below.
         */
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DGradEXT", sampler2D, float2, float2, float2);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjGradEXT", sampler2D, float3, float2, float2);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjGradEXT", sampler2D, float4, float2, float2);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "textureCubeGradEXT", samplerCube, float3, float3, float3);
    }

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2D", sampler2D, float2, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float3, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProj", sampler2D, float4, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "textureCube", samplerCube, float3, float1);

        if (resources.OES_standard_derivatives)
        {
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, genType, "dFdx", genType);
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, genType, "dFdy", genType);
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, genType, "fwidth", genType);
        }

        if (resources.EXT_shader_texture_lod)
        {
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DLodEXT", sampler2D, float2, float1);
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLodEXT", sampler2D, float3, float1);
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLodEXT", sampler2D, float4, float1);
            symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "textureCubeLodEXT", samplerCube, float3, float1);
        }
    }

    if (type == GL_VERTEX_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DLod", sampler2D, float2, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLod", sampler2D, float3, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "texture2DProjLod", sampler2D, float4, float1);
        symbolTable.insertBuiltIn(ESSL1_BUILTINS, float4, "textureCubeLod", samplerCube, float3, float1);
    }

    TType *gvec4 = new TType(EbtGVec4);

    TType *gsampler2D = new TType(EbtGSampler2D);
    TType *gsamplerCube = new TType(EbtGSamplerCube);
    TType *gsampler3D = new TType(EbtGSampler3D);
    TType *gsampler2DArray = new TType(EbtGSampler2DArray);

    //
    // Texture Functions for GLSL ES 3.0
    //
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2D, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler3D, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsamplerCube, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2DArray, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler3D, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler2D, float2, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler3D, float3, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsamplerCube, float3, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLod", gsampler2DArray, float3, float1);

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2D, float2, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler3D, float3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsamplerCube, float3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texture", gsampler2DArray, float3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler2D, float4, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProj", gsampler3D, float4, float1);
    }

    TType *sampler2DShadow = new TType(EbtSampler2DShadow);
    TType *samplerCubeShadow = new TType(EbtSamplerCubeShadow);
    TType *sampler2DArrayShadow = new TType(EbtSampler2DArrayShadow);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DShadow, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "texture", samplerCubeShadow, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DArrayShadow, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProj", sampler2DShadow, float4);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureLod", sampler2DShadow, float3, float1);

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "texture", sampler2DShadow, float3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "texture", samplerCubeShadow, float4, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProj", sampler2DShadow, float4, float1);
    }

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", gsampler2D, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", gsampler3D, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", gsamplerCube, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", gsampler2DArray, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", sampler2DShadow, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int2, "textureSize", samplerCubeShadow, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, int3, "textureSize", sampler2DArrayShadow, int1);

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "dFdx", genType);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "dFdy", genType);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, genType, "fwidth", genType);
    }

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2D, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler3D, float3, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureOffset", sampler2DShadow, float3, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2DArray, float3, int2);

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2D, float2, int2, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler3D, float3, int3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureOffset", sampler2DShadow, float3, int2, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureOffset", gsampler2DArray, float3, int2, float1);
    }

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float3, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float4, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler3D, float4, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjOffset", sampler2DShadow, float4, int2);

    if (type == GL_FRAGMENT_SHADER)
    {
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float3, int2, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler2D, float4, int2, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjOffset", gsampler3D, float4, int3, float1);
        symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjOffset", sampler2DShadow, float4, int2, float1);
    }

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler2D, float2, float1, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler3D, float3, float1, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureLodOffset", sampler2DShadow, float3, float1, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureLodOffset", gsampler2DArray, float3, float1, int2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler2D, float3, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler2D, float4, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLod", gsampler3D, float4, float1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjLod", sampler2DShadow, float4, float1);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler2D, float3, float1, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler2D, float4, float1, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjLodOffset", gsampler3D, float4, float1, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjLodOffset", sampler2DShadow, float4, float1, int2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler2D, int2, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler3D, int3, int1);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetch", gsampler2DArray, int3, int1);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler2D, int2, int1, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler3D, int3, int1, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "texelFetchOffset", gsampler2DArray, int3, int1, int2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler2D, float2, float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler3D, float3, float3, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsamplerCube, float3, float3, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", sampler2DShadow, float3, float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", samplerCubeShadow, float4, float3, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGrad", gsampler2DArray, float3, float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureGrad", sampler2DArrayShadow, float4, float2, float2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler2D, float2, float2, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler3D, float3, float3, float3, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureGradOffset", sampler2DShadow, float3, float2, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureGradOffset", gsampler2DArray, float3, float2, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureGradOffset", sampler2DArrayShadow, float4, float2, float2, int2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler2D, float3, float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler2D, float4, float2, float2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGrad", gsampler3D, float4, float3, float3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjGrad", sampler2DShadow, float4, float2, float2);

    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler2D, float3, float2, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler2D, float4, float2, float2, int2);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, gvec4, "textureProjGradOffset", gsampler3D, float4, float3, float3, int3);
    symbolTable.insertBuiltIn(ESSL3_BUILTINS, float1, "textureProjGradOffset", sampler2DShadow, float4, float2, float2, int2);

    //
    // Depth range in window coordinates
    //
    TFieldList *fields = NewPoolTFieldList();
    TSourceLoc zeroSourceLoc = {0, 0, 0, 0};
    TField *near = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("near"), zeroSourceLoc);
    TField *far = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("far"), zeroSourceLoc);
    TField *diff = new TField(new TType(EbtFloat, EbpHigh, EvqGlobal, 1), NewPoolTString("diff"), zeroSourceLoc);
    fields->push_back(near);
    fields->push_back(far);
    fields->push_back(diff);
    TStructure *depthRangeStruct = new TStructure(NewPoolTString("gl_DepthRangeParameters"), fields);
    TVariable *depthRangeParameters = new TVariable(&depthRangeStruct->name(), depthRangeStruct, true);
    symbolTable.insert(COMMON_BUILTINS, depthRangeParameters);
    TVariable *depthRange = new TVariable(NewPoolTString("gl_DepthRange"), TType(depthRangeStruct));
    depthRange->setQualifier(EvqUniform);
    symbolTable.insert(COMMON_BUILTINS, depthRange);

    //
    // Implementation dependent built-in constants.
    //
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxVertexAttribs", resources.MaxVertexAttribs);
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxVertexUniformVectors", resources.MaxVertexUniformVectors);
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxVertexTextureImageUnits", resources.MaxVertexTextureImageUnits);
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxCombinedTextureImageUnits", resources.MaxCombinedTextureImageUnits);
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxTextureImageUnits", resources.MaxTextureImageUnits);
    symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxFragmentUniformVectors", resources.MaxFragmentUniformVectors);

    symbolTable.insertConstInt(ESSL1_BUILTINS, "gl_MaxVaryingVectors", resources.MaxVaryingVectors);

    if (spec != SH_CSS_SHADERS_SPEC)
    {
        symbolTable.insertConstInt(COMMON_BUILTINS, "gl_MaxDrawBuffers", resources.MaxDrawBuffers);
    }

    symbolTable.insertConstInt(ESSL3_BUILTINS, "gl_MaxVertexOutputVectors", resources.MaxVertexOutputVectors);
    symbolTable.insertConstInt(ESSL3_BUILTINS, "gl_MaxFragmentInputVectors", resources.MaxFragmentInputVectors);
    symbolTable.insertConstInt(ESSL3_BUILTINS, "gl_MinProgramTexelOffset", resources.MinProgramTexelOffset);
    symbolTable.insertConstInt(ESSL3_BUILTINS, "gl_MaxProgramTexelOffset", resources.MaxProgramTexelOffset);
}

void IdentifyBuiltIns(sh::GLenum type, ShShaderSpec spec,
                      const ShBuiltInResources &resources,
                      TSymbolTable &symbolTable)
{
    //
    // First, insert some special built-in variables that are not in 
    // the built-in header files.
    //
    switch(type) {
    case GL_FRAGMENT_SHADER:
        symbolTable.insert(COMMON_BUILTINS, new TVariable(NewPoolTString("gl_FragCoord"), TType(EbtFloat, EbpMedium, EvqFragCoord,   4)));
        symbolTable.insert(COMMON_BUILTINS, new TVariable(NewPoolTString("gl_FrontFacing"), TType(EbtBool,  EbpUndefined, EvqFrontFacing, 1)));
        symbolTable.insert(COMMON_BUILTINS, new TVariable(NewPoolTString("gl_PointCoord"), TType(EbtFloat, EbpMedium, EvqPointCoord,  2)));

        //
        // In CSS Shaders, gl_FragColor, gl_FragData, and gl_MaxDrawBuffers are not available.
        // Instead, css_MixColor and css_ColorMatrix are available.
        //
        if (spec != SH_CSS_SHADERS_SPEC) {
            symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_FragColor"), TType(EbtFloat, EbpMedium, EvqFragColor,   4)));
            symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_FragData[gl_MaxDrawBuffers]"), TType(EbtFloat, EbpMedium, EvqFragData,    4)));
            if (resources.EXT_frag_depth) {
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_FragDepthEXT"), TType(EbtFloat, resources.FragmentPrecisionHigh ? EbpHigh : EbpMedium, EvqFragDepth, 1)));
                symbolTable.relateToExtension(ESSL1_BUILTINS, "gl_FragDepthEXT", "GL_EXT_frag_depth");
            }
            if (resources.EXT_shader_framebuffer_fetch)
            {
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_LastFragData[gl_MaxDrawBuffers]"), TType(EbtFloat, EbpMedium, EvqLastFragData,   4)));
            }
            else if (resources.NV_shader_framebuffer_fetch)
            {
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_LastFragColor"), TType(EbtFloat, EbpMedium, EvqLastFragColor,   4)));
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_LastFragData[gl_MaxDrawBuffers]"), TType(EbtFloat, EbpMedium, EvqLastFragData,   4)));
            }
            else if (resources.ARM_shader_framebuffer_fetch)
            {
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_LastFragColorARM"), TType(EbtFloat, EbpMedium, EvqLastFragColor,   4)));
            }
        } else {
            symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("css_MixColor"), TType(EbtFloat, EbpMedium, EvqGlobal,      4)));
            symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("css_ColorMatrix"), TType(EbtFloat, EbpMedium, EvqGlobal,      4, 4)));
        }

        break;

    case GL_VERTEX_SHADER:
        symbolTable.insert(COMMON_BUILTINS, new TVariable(NewPoolTString("gl_Position"), TType(EbtFloat, EbpHigh, EvqPosition,    4)));
        symbolTable.insert(COMMON_BUILTINS, new TVariable(NewPoolTString("gl_PointSize"), TType(EbtFloat, EbpMedium, EvqPointSize,   1)));
        symbolTable.insert(ESSL3_BUILTINS, new TVariable(NewPoolTString("gl_InstanceID"), TType(EbtInt, EbpHigh, EvqInstanceID,   1)));
        break;

    default: assert(false && "Language not supported");
    }

    //
    // Next, identify which built-ins from the already loaded headers have
    // a mapping to an operator.  Those that are not identified as such are
    // expected to be resolved through a library of functions, versus as
    // operations.
    //
    symbolTable.relateToOperator(COMMON_BUILTINS, "matrixCompMult",   EOpMul);
    symbolTable.relateToOperator(ESSL3_BUILTINS,  "matrixCompMult",   EOpMul);

    symbolTable.relateToOperator(COMMON_BUILTINS, "equal",            EOpVectorEqual);
    symbolTable.relateToOperator(COMMON_BUILTINS, "notEqual",         EOpVectorNotEqual);
    symbolTable.relateToOperator(COMMON_BUILTINS, "lessThan",         EOpLessThan);
    symbolTable.relateToOperator(COMMON_BUILTINS, "greaterThan",      EOpGreaterThan);
    symbolTable.relateToOperator(COMMON_BUILTINS, "lessThanEqual",    EOpLessThanEqual);
    symbolTable.relateToOperator(COMMON_BUILTINS, "greaterThanEqual", EOpGreaterThanEqual);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "equal",             EOpVectorEqual);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "notEqual",          EOpVectorNotEqual);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "lessThan",          EOpLessThan);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "greaterThan",       EOpGreaterThan);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "lessThanEqual",     EOpLessThanEqual);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "greaterThanEqual",  EOpGreaterThanEqual);

    symbolTable.relateToOperator(COMMON_BUILTINS, "radians",      EOpRadians);
    symbolTable.relateToOperator(COMMON_BUILTINS, "degrees",      EOpDegrees);
    symbolTable.relateToOperator(COMMON_BUILTINS, "sin",          EOpSin);
    symbolTable.relateToOperator(COMMON_BUILTINS, "cos",          EOpCos);
    symbolTable.relateToOperator(COMMON_BUILTINS, "tan",          EOpTan);
    symbolTable.relateToOperator(COMMON_BUILTINS, "asin",         EOpAsin);
    symbolTable.relateToOperator(COMMON_BUILTINS, "acos",         EOpAcos);
    symbolTable.relateToOperator(COMMON_BUILTINS, "atan",         EOpAtan);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "sinh",          EOpSinh);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "cosh",          EOpCosh);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "tanh",          EOpTanh);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "asinh",         EOpAsinh);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "acosh",         EOpAcosh);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "atanh",         EOpAtanh);

    symbolTable.relateToOperator(COMMON_BUILTINS, "pow",          EOpPow);
    symbolTable.relateToOperator(COMMON_BUILTINS, "exp2",         EOpExp2);
    symbolTable.relateToOperator(COMMON_BUILTINS, "log",          EOpLog);
    symbolTable.relateToOperator(COMMON_BUILTINS, "exp",          EOpExp);
    symbolTable.relateToOperator(COMMON_BUILTINS, "log2",         EOpLog2);
    symbolTable.relateToOperator(COMMON_BUILTINS, "sqrt",         EOpSqrt);
    symbolTable.relateToOperator(COMMON_BUILTINS, "inversesqrt",  EOpInverseSqrt);

    symbolTable.relateToOperator(COMMON_BUILTINS, "abs",          EOpAbs);
    symbolTable.relateToOperator(COMMON_BUILTINS, "sign",         EOpSign);
    symbolTable.relateToOperator(COMMON_BUILTINS, "floor",        EOpFloor);
    symbolTable.relateToOperator(COMMON_BUILTINS, "ceil",         EOpCeil);
    symbolTable.relateToOperator(COMMON_BUILTINS, "fract",        EOpFract);
    symbolTable.relateToOperator(COMMON_BUILTINS, "mod",          EOpMod);
    symbolTable.relateToOperator(COMMON_BUILTINS, "min",          EOpMin);
    symbolTable.relateToOperator(COMMON_BUILTINS, "max",          EOpMax);
    symbolTable.relateToOperator(COMMON_BUILTINS, "clamp",        EOpClamp);
    symbolTable.relateToOperator(COMMON_BUILTINS, "mix",          EOpMix);
    symbolTable.relateToOperator(COMMON_BUILTINS, "step",         EOpStep);
    symbolTable.relateToOperator(COMMON_BUILTINS, "smoothstep",   EOpSmoothStep);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "abs",           EOpAbs);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "sign",          EOpSign);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "modf",          EOpModf);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "min",           EOpMin);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "max",           EOpMax);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "clamp",         EOpClamp);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "isnan",         EOpIsNan);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "isinf",         EOpIsInf);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "floatBitsToInt",  EOpFloatBitsToInt);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "floatBitsToUint", EOpFloatBitsToUint);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "intBitsToFloat",  EOpIntBitsToFloat);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "uintBitsToFloat", EOpUintBitsToFloat);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "packSnorm2x16", EOpPackSnorm2x16);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "packUnorm2x16", EOpPackUnorm2x16);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "packHalf2x16",  EOpPackHalf2x16);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "unpackSnorm2x16", EOpUnpackSnorm2x16);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "unpackUnorm2x16", EOpUnpackUnorm2x16);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "unpackHalf2x16",  EOpUnpackHalf2x16);

    symbolTable.relateToOperator(COMMON_BUILTINS, "length",       EOpLength);
    symbolTable.relateToOperator(COMMON_BUILTINS, "distance",     EOpDistance);
    symbolTable.relateToOperator(COMMON_BUILTINS, "dot",          EOpDot);
    symbolTable.relateToOperator(COMMON_BUILTINS, "cross",        EOpCross);
    symbolTable.relateToOperator(COMMON_BUILTINS, "normalize",    EOpNormalize);
    symbolTable.relateToOperator(COMMON_BUILTINS, "faceforward",  EOpFaceForward);
    symbolTable.relateToOperator(COMMON_BUILTINS, "reflect",      EOpReflect);
    symbolTable.relateToOperator(COMMON_BUILTINS, "refract",      EOpRefract);

    symbolTable.relateToOperator(ESSL3_BUILTINS, "outerProduct",  EOpOuterProduct);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "transpose",     EOpTranspose);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "determinant",   EOpDeterminant);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "inverse",       EOpInverse);

    symbolTable.relateToOperator(COMMON_BUILTINS, "any",          EOpAny);
    symbolTable.relateToOperator(COMMON_BUILTINS, "all",          EOpAll);
    symbolTable.relateToOperator(COMMON_BUILTINS, "not",          EOpVectorLogicalNot);

    // Map language-specific operators.
    switch(type) {
    case GL_VERTEX_SHADER:
        break;
    case GL_FRAGMENT_SHADER:
        if (resources.OES_standard_derivatives)
        {
            symbolTable.relateToOperator(ESSL1_BUILTINS, "dFdx",   EOpDFdx);
            symbolTable.relateToOperator(ESSL1_BUILTINS, "dFdy",   EOpDFdy);
            symbolTable.relateToOperator(ESSL1_BUILTINS, "fwidth", EOpFwidth);

            symbolTable.relateToExtension(ESSL1_BUILTINS, "dFdx", "GL_OES_standard_derivatives");
            symbolTable.relateToExtension(ESSL1_BUILTINS, "dFdy", "GL_OES_standard_derivatives");
            symbolTable.relateToExtension(ESSL1_BUILTINS, "fwidth", "GL_OES_standard_derivatives");
        }
        if (resources.EXT_shader_texture_lod)
        {
            symbolTable.relateToExtension(ESSL1_BUILTINS, "texture2DLodEXT", "GL_EXT_shader_texture_lod");
            symbolTable.relateToExtension(ESSL1_BUILTINS, "texture2DProjLodEXT", "GL_EXT_shader_texture_lod");
            symbolTable.relateToExtension(ESSL1_BUILTINS, "textureCubeLodEXT", "GL_EXT_shader_texture_lod");
        }
        if (resources.NV_shader_framebuffer_fetch)
        {
            symbolTable.relateToExtension(ESSL1_BUILTINS, "gl_LastFragColor", "GL_NV_shader_framebuffer_fetch");
        }
        else if (resources.ARM_shader_framebuffer_fetch)
        {
            symbolTable.relateToExtension(ESSL1_BUILTINS, "gl_LastFragColorARM", "GL_ARM_shader_framebuffer_fetch");
        }
        break;
    default: break;
    }

    symbolTable.relateToOperator(ESSL3_BUILTINS, "dFdx",   EOpDFdx);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "dFdy",   EOpDFdy);
    symbolTable.relateToOperator(ESSL3_BUILTINS, "fwidth", EOpFwidth);

    if (resources.EXT_shader_texture_lod)
    {
        symbolTable.relateToExtension(ESSL1_BUILTINS, "texture2DGradEXT", "GL_EXT_shader_texture_lod");
        symbolTable.relateToExtension(ESSL1_BUILTINS, "texture2DProjGradEXT", "GL_EXT_shader_texture_lod");
        symbolTable.relateToExtension(ESSL1_BUILTINS, "textureCubeGradEXT", "GL_EXT_shader_texture_lod");
    }

    // Finally add resource-specific variables.
    switch(type) {
    case GL_FRAGMENT_SHADER:
        if (spec != SH_CSS_SHADERS_SPEC) {
            // Set up gl_FragData.  The array size.
            TType fragData(EbtFloat, EbpMedium, EvqFragData, 4, 1, true);
            fragData.setArraySize(resources.MaxDrawBuffers);
            symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_FragData"), fragData));

            if (resources.EXT_shader_framebuffer_fetch || resources.NV_shader_framebuffer_fetch) {
                // Set up gl_LastFragData.  The array size.
                TType lastFragData(EbtFloat, EbpMedium, EvqLastFragData, 4, 1, true);
                lastFragData.setArraySize(resources.MaxDrawBuffers);
                symbolTable.insert(ESSL1_BUILTINS, new TVariable(NewPoolTString("gl_LastFragData"), lastFragData));

                if (resources.EXT_shader_framebuffer_fetch)
                {
                    symbolTable.relateToExtension(ESSL1_BUILTINS, "gl_LastFragData", "GL_EXT_shader_framebuffer_fetch");
                }
                else if (resources.NV_shader_framebuffer_fetch)
                {
                    symbolTable.relateToExtension(ESSL1_BUILTINS, "gl_LastFragData", "GL_NV_shader_framebuffer_fetch");
                }
            }
        }
        break;
    default: break;
    }
}

void InitExtensionBehavior(const ShBuiltInResources& resources,
                           TExtensionBehavior& extBehavior)
{
    if (resources.OES_standard_derivatives)
        extBehavior["GL_OES_standard_derivatives"] = EBhUndefined;
    if (resources.OES_EGL_image_external)
        extBehavior["GL_OES_EGL_image_external"] = EBhUndefined;
    if (resources.ARB_texture_rectangle)
        extBehavior["GL_ARB_texture_rectangle"] = EBhUndefined;
    if (resources.EXT_draw_buffers)
        extBehavior["GL_EXT_draw_buffers"] = EBhUndefined;
    if (resources.EXT_frag_depth)
        extBehavior["GL_EXT_frag_depth"] = EBhUndefined;
    if (resources.EXT_shader_texture_lod)
        extBehavior["GL_EXT_shader_texture_lod"] = EBhUndefined;
    if (resources.EXT_shader_framebuffer_fetch)
        extBehavior["GL_EXT_shader_framebuffer_fetch"] = EBhUndefined;
    if (resources.NV_shader_framebuffer_fetch)
        extBehavior["GL_NV_shader_framebuffer_fetch"] = EBhUndefined;
    if (resources.ARM_shader_framebuffer_fetch)
        extBehavior["GL_ARM_shader_framebuffer_fetch"] = EBhUndefined;
}

void ResetExtensionBehavior(TExtensionBehavior &extBehavior)
{
    for (auto ext_iter = extBehavior.begin();
         ext_iter != extBehavior.end();
         ++ext_iter)
    {
        ext_iter->second = EBhUndefined;
    }
}

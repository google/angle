//
// Copyright (c) 2017 The ANGLE Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Clear11.hlsl: Vertex and Pixel shaders for clearing RTVs and DSVs using
// draw calls and specifying float depth values and clear colors as either
// float, uint or sint. Notes:
//  - UINT & SINT clears can only be compiled with FL10+.
//  - VS_ClearAnyType_FL9 requires a VB to be bound with vertices
//    defining a quad covering entire surface (in clip co-ordinates)
//  - VS_ClearAnyType_FL9 used for all pixel shaders defined here


// Vertex Shader
// TODO (Shahmeer Esmail): Use SV_VertexID to generate quad (for FL10+) to
// avoid having to rely on a VB to be generated/bound
void VS_ClearAnyType( in float4 inPosition : POSITION,
                          out float4 outPosition : SV_POSITION)
{
    outPosition = inPosition;
}

// Pixel Shader Constant Buffers
cbuffer ColorAndDepthDataFloat : register(b0)
{
    float4 color_Float   : packoffset(c0);
    float  zValueF_Float : packoffset(c1.x);
    float  a1_Float      : packoffset(c1.y);
    float  a2_Float      : packoffset(c1.z);
    float  a3_Float      : packoffset(c1.w);
    float  a4_Float      : packoffset(c2.x);
    float  a5_Float      : packoffset(c2.y);
    float  a6_Float      : packoffset(c2.z);
    float  a7_Float      : packoffset(c2.w);
}

cbuffer ColorAndDepthDataSint : register(b0)
{
    int4 color_Sint    : packoffset(c0);
    float zValueF_Sint : packoffset(c1.x);
}

cbuffer ColorAndDepthDataUint : register(b0)
{
    uint4 color_Uint   : packoffset(c0);
    float zValueF_Uint : packoffset(c1.x);
}

// Pixel Shader Output Structs
struct PS_OutputFloat_FL9
{
    float4 color0 : SV_TARGET0;
    float4 color1 : SV_TARGET1;
    float4 color2 : SV_TARGET2;
    float4 color3 : SV_TARGET3;
    float  depth  : SV_DEPTH;
};

struct PS_OutputFloat
{
    float4 color0 : SV_TARGET0;
    float4 color1 : SV_TARGET1;
    float4 color2 : SV_TARGET2;
    float4 color3 : SV_TARGET3;
    float4 color4 : SV_TARGET4;
    float4 color5 : SV_TARGET5;
    float4 color6 : SV_TARGET6;
    float4 color7 : SV_TARGET7;
    float  depth  : SV_DEPTH;
};

struct PS_OutputUint
{
    uint4 color0 : SV_TARGET0;
    uint4 color1 : SV_TARGET1;
    uint4 color2 : SV_TARGET2;
    uint4 color3 : SV_TARGET3;
    uint4 color4 : SV_TARGET4;
    uint4 color5 : SV_TARGET5;
    uint4 color6 : SV_TARGET6;
    uint4 color7 : SV_TARGET7;
    float depth  : SV_DEPTH;
};

struct PS_OutputSint
{
    int4 color0 : SV_TARGET0;
    int4 color1 : SV_TARGET1;
    int4 color2 : SV_TARGET2;
    int4 color3 : SV_TARGET3;
    int4 color4 : SV_TARGET4;
    int4 color5 : SV_TARGET5;
    int4 color6 : SV_TARGET6;
    int4 color7 : SV_TARGET7;
    float depth : SV_DEPTH;
};

// Pixel Shaders (FL_9)
PS_OutputFloat_FL9 PS_ClearFloat_FL9(in float4 inPosition : SV_POSITION)
{
    PS_OutputFloat_FL9 outColorsAndDepth;
    outColorsAndDepth.color0 = color_Float;
    outColorsAndDepth.color1 = float4(color_Float.xyz, a1_Float);
    outColorsAndDepth.color2 = float4(color_Float.xyz, a2_Float);
    outColorsAndDepth.color3 = float4(color_Float.xyz, a3_Float);
    outColorsAndDepth.depth  = zValueF_Float;
    return outColorsAndDepth;
}

// Pixel Shaders (FL_10+)
PS_OutputFloat PS_ClearFloat(in float4 inPosition : SV_POSITION)
{
    PS_OutputFloat outColorsAndDepth;
    outColorsAndDepth.color0 = color_Float;
    outColorsAndDepth.color1 = float4(color_Float.xyz, a1_Float);
    outColorsAndDepth.color2 = float4(color_Float.xyz, a2_Float);
    outColorsAndDepth.color3 = float4(color_Float.xyz, a3_Float);
    outColorsAndDepth.color4 = float4(color_Float.xyz, a4_Float);
    outColorsAndDepth.color5 = float4(color_Float.xyz, a5_Float);
    outColorsAndDepth.color6 = float4(color_Float.xyz, a6_Float);
    outColorsAndDepth.color7 = float4(color_Float.xyz, a7_Float);
    outColorsAndDepth.depth  = zValueF_Float;
    return outColorsAndDepth;
}

PS_OutputUint PS_ClearUint(in float4 inPosition : SV_POSITION)
{
    PS_OutputUint outData;
    outData.color0 = color_Uint;
    outData.color1 = color_Uint;
    outData.color2 = color_Uint;
    outData.color3 = color_Uint;
    outData.color4 = color_Uint;
    outData.color5 = color_Uint;
    outData.color6 = color_Uint;
    outData.color7 = color_Uint;
    outData.depth = zValueF_Uint;
    return outData;
}

PS_OutputSint PS_ClearSint(in float4 inPosition : SV_POSITION)
{
    PS_OutputSint outData;
    outData.color0 = color_Sint;
    outData.color1 = color_Sint;
    outData.color2 = color_Sint;
    outData.color3 = color_Sint;
    outData.color4 = color_Sint;
    outData.color5 = color_Sint;
    outData.color6 = color_Sint;
    outData.color7 = color_Sint;
    outData.depth = zValueF_Sint;
    return outData;
}

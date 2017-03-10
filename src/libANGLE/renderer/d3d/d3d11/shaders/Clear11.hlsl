//
// Copyright (c) 2017 The ANGLE Project. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//

// Clear11.hlsl: Shaders for clearing RTVs and DSVs using draw calls and
// specifying float depth values and either float, uint or sint clear colors.
// Notes:
//  - UINT & SINT clears can only be compiled with FL10+
//  - VS_Clear_FL9 requires a VB to be bound with vertices to create
//    a primitive covering the entire surface (in clip co-ordinates)

// Constants
static const float2 g_Corners[6] =
{
    float2(-1.0f,  1.0f),
    float2( 1.0f, -1.0f),
    float2(-1.0f, -1.0f),
    float2(-1.0f,  1.0f),
    float2( 1.0f,  1.0f),
    float2( 1.0f, -1.0f),
};

// Vertex Shaders
void VS_Clear(in uint id : SV_VertexID,
              out float4 outPosition : SV_POSITION)
{
    float2 corner = g_Corners[id];
    outPosition = float4(corner.x, corner.y, 0.0f, 1.0f);
}

void VS_Clear_FL9( in float4 inPosition : POSITION,
                   out float4 outPosition : SV_POSITION)
{
    outPosition = inPosition;
}

// Pixel Shader Constant Buffers
cbuffer ColorAndDepthDataFloat : register(b0)
{
    float4 color_Float   : packoffset(c0);
    float  zValueF_Float : packoffset(c1);
}

cbuffer ColorAndDepthDataSint : register(b0)
{
    int4  color_Sint   : packoffset(c0);
    float zValueF_Sint : packoffset(c1);
}

cbuffer ColorAndDepthDataUint : register(b0)
{
    uint4 color_Uint   : packoffset(c0);
    float zValueF_Uint : packoffset(c1);
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

// Pixel Shaders
PS_OutputFloat_FL9 PS_ClearFloat_FL9(in float4 inPosition : SV_POSITION)
{
    PS_OutputFloat_FL9 outData;
    outData.color0 = color_Float;
    outData.color1 = color_Float;
    outData.color2 = color_Float;
    outData.color3 = color_Float;
    outData.depth  = zValueF_Float;
    return outData;
}

PS_OutputFloat PS_ClearFloat(in float4 inPosition : SV_POSITION)
{
    PS_OutputFloat outData;
    outData.color0 = color_Float;
    outData.color1 = color_Float;
    outData.color2 = color_Float;
    outData.color3 = color_Float;
    outData.color4 = color_Float;
    outData.color5 = color_Float;
    outData.color6 = color_Float;
    outData.color7 = color_Float;
    outData.depth  = zValueF_Float;
    return outData;
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
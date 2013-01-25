void VS_Clear( in float3  inPosition :    POSITION,  in float4  inColor : COLOR,
              out float4 outPosition : SV_POSITION, out float4 outColor : COLOR)
{
    outPosition = float4(inPosition, 1.0f);
    outColor = inColor;
}

float4 PS_Clear(in float4 inPosition : SV_POSITION, in float4 inColor : COLOR) : SV_TARGET0
{
    return inColor;
}

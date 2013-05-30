Texture3D Texture : register(t0);
SamplerState Sampler : register(s0);

struct VS_INPUT
{
    float2 Position : POSITION;
    uint   Layer    : LAYER;
    float3 TexCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 Position : SV_POSITION;
    uint   Layer    : LAYER;
    float3 TexCoord : TEXCOORD;
};

struct GS_OUTPUT
{
    float4 Position : SV_POSITION;
    uint   Layer    : SV_RENDERTARGETARRAYINDEX;
    float3 TexCoord : TEXCOORD;
};

VS_OUTPUT VS_Passthrough3D(VS_INPUT input)
{
    VS_OUTPUT output;

    output.Position = float4(input.Position, 0.0f, 1.0f);
    output.Layer = input.Layer;
    output.TexCoord = input.TexCoord;

    return output;
}

[maxvertexcount(3)]
void GS_Passthrough3D(triangle VS_OUTPUT input[3], inout TriangleStream<GS_OUTPUT> outputStream)
{
    GS_OUTPUT output;

    for (int i = 0; i < 3; i++)
    {
        output.Position = input[i].Position;
        output.Layer = input[i].Layer;
        output.TexCoord = input[i].TexCoord;

        outputStream.Append(output);
    }
}

float4 PS_PassthroughRGBA3D(GS_OUTPUT input) : SV_TARGET0
{
    return Texture.Sample(Sampler, input.TexCoord).rgba;
}

float4 PS_PassthroughRGB3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(Texture.Sample(Sampler, input.TexCoord).rgb, 1.0f);
}

float4 PS_PassthroughLum3D(GS_OUTPUT input) : SV_TARGET0
{
    return float4(Texture.Sample(Sampler, input.TexCoord).rrr, 1.0f);
}

float4 PS_PassthroughLumAlpha3D(GS_OUTPUT input) : SV_TARGET0
{
    return Texture.Sample(Sampler, input.TexCoord).rrra;
}

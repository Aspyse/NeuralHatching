Texture2D inputTexture : register(t0);
SamplerState samplerState : register(s0);

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

PixelInputType BaseVertexShader(uint vertexID : SV_VertexID)
{
    PixelInputType output;
    
    float2 pos = float2((vertexID << 1) & 2, vertexID & 2);
    output.uv = pos;

    float2 ndc = pos * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f);
    output.position = float4(ndc, 0.0f, 1.0f);

    return output;
}

float4 PostprocessShader(PixelInputType input) : SV_Target
{
    return inputTexture.Sample(samplerState, input.uv);
}
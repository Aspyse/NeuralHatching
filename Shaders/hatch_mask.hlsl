// modulates one traced hatch layer with the lit/unlit mask: lit fades to
// blank white, unlit/shadowed keeps the hatching. run once per hatch field.

Texture2D hatchTexture : register(t0);
Texture2D litMaskTexture : register(t1);

SamplerState pointClamp : register(s0);

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float4 HatchMaskPixelShader(PixelInputType input) : SV_TARGET
{
    float3 hatch = hatchTexture.Sample(pointClamp, input.uv).rgb;
    float lit = litMaskTexture.Sample(pointClamp, input.uv).r;

    // lit 1 -> blank white, lit 0 -> full hatch texture
    float3 result = lerp(hatch, float3(1, 1, 1), lit);
    return float4(result, 1);
}

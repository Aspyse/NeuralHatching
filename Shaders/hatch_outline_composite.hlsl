Texture2D hatchTexture : register(t0);
Texture2D edgeTexture : register(t1);

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float4 OutlineCompositePixelShader(PixelInputType input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    float3 hatch = hatchTexture.Load(int3(pixel, 0)).rgb;
    float edge = edgeTexture.Load(int3(pixel, 0)).r;

    return float4(min(hatch, edge.xxx), 1);
}

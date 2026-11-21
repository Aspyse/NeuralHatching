// Flat material view: albedo supplies the surface color; one lit/shadow-masked
// hatch layer and the silhouette/crease detector contribute only black ink.
Texture2D albedoTexture : register(t0);
Texture2D edgeTexture : register(t1);
Texture2D hatchTexture : register(t2);
Texture2D depthTexture : register(t3);

SamplerState pointClamp : register(s0);

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float4 FlatCompositePixelShader(PixelInputType input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    if (depthTexture.Load(int3(pixel, 0)).r >= 0.99999f)
        return float4(0.3f, 0.3f, 0.3f, 1.0f);

    float3 albedo = albedoTexture.Load(int3(pixel, 0)).rgb;
    float edge = edgeTexture.Load(int3(pixel, 0)).r;
    float hatch = hatchTexture.Load(int3(pixel, 0)).r;

    float ink = min(edge, hatch);
    return float4(albedo * ink, 1);
}

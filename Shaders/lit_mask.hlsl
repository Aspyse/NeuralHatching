// combines NdotL with shadow visibility into one mask:
// 1 = lit, 0 = unlit (facing away or shadowed). feeds hatch_mask.hlsl

Texture2D normalTexture : register(t0);
Texture2D depthTexture : register(t1);
Texture2D shadowMap : register(t2);

SamplerState pointClamp : register(s0);
SamplerComparisonState shadowSampler : register(s1); // D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, ComparisonFunc = LESS_EQUAL

cbuffer LitMaskBuffer : register(b0)
{
    float4x4 invProj;
    float4x4 invView;
    float4x4 lightViewProj;
    float3 lightDirectionVS;
    float shadowBias;
};

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

// same unprojection as matcap.hlsl
float3 ReconstructViewPos(float2 uv)
{
    float depth = depthTexture.Sample(pointClamp, uv).r;
    uv = uv * 2 - 1;
    uv.y = -uv.y;
    float4 clipPos = float4(uv, depth, 1);
    float4 viewPosH = mul(clipPos, invProj);
    return viewPosH.xyz / viewPosH.w;
}

float ShadowVisibility(float4 worldPos)
{
    float4 lightPos = mul(worldPos, lightViewProj);
    float3 uv = lightPos.xyz / lightPos.w;
    float2 shadowMapUV = float2(0.5f + uv.x * 0.5f, 0.5f - uv.y * 0.5f);
    return shadowMap.SampleCmpLevelZero(shadowSampler, shadowMapUV, uv.z - shadowBias).r;
}

float4 LitMaskPixelShader(PixelInputType input) : SV_TARGET
{
    float depth = depthTexture.Sample(pointClamp, input.uv).r;
    // background, treat as lit so it blends to blank in hatch_mask.hlsl
    if (depth == 1)
        return float4(1, 1, 1, 1);

    float3 normal = normalize(normalTexture.Sample(pointClamp, input.uv).rgb * 2 - 1);
    float3 viewPos = ReconstructViewPos(input.uv);
    float3 L = normalize(-lightDirectionVS);
    float NdotL = dot(normal, L);

    float4 worldPos = mul(float4(viewPos, 1), invView);
    float vis = ShadowVisibility(worldPos);

    // hard threshold both terms -- no grays, just facing-the-light AND not-in-shadow
    float lit = (NdotL > 0.0f && vis > 0.5f) ? 1.0f : 0.0f;
    return float4(lit.xxx, 1);
}

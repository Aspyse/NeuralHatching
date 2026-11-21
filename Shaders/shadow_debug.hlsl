// false-color shadow debug view: green = lit, red = shadowed, blue = outside the light's ortho frustum.
// same reconstruction + lookup as lit_mask.hlsl, just visualizes the classification instead
// of collapsing it straight to lit/unlit -- point of this is to see if "unlit" spots are actually
// landing outside the light frustum instead of genuinely being in shadow.

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
    uint shadowMappingEnabled;
    float3 shadowPad;
};

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

// same unprojection as matcap.hlsl / lit_mask.hlsl
float3 ReconstructViewPos(float2 uv)
{
    float depth = depthTexture.Sample(pointClamp, uv).r;
    uv = uv * 2 - 1;
    uv.y = -uv.y;
    float4 clipPos = float4(uv, depth, 1);
    float4 viewPosH = mul(clipPos, invProj);
    return viewPosH.xyz / viewPosH.w;
}

float4 ShadowDebugPixelShader(PixelInputType input) : SV_TARGET
{
    float depth = depthTexture.Sample(pointClamp, input.uv).r;
    if (depth == 1)
        return float4(0, 0, 0, 1); // background, stays black

    float3 normalVS = normalize(normalTexture.Sample(pointClamp, input.uv).rgb * 2 - 1);
    float3 viewPos = ReconstructViewPos(input.uv);
    float3 L = normalize(-lightDirectionVS);
    float NdotL = dot(normalVS, L);

    if (shadowMappingEnabled == 0)
        return NdotL > 0.0f ? float4(0, 1, 0, 1) : float4(1, 0, 0, 1);

    float4 worldPos = mul(float4(viewPos, 1), invView);
    float4 lightPos = mul(worldPos, lightViewProj);
    float3 lp = lightPos.xyz / lightPos.w;

    // outside the ortho box -- the frustum just doesn't cover this point, distinct from "in shadow"
    if (abs(lp.x) > 1.0f || abs(lp.y) > 1.0f || lp.z < 0.0f || lp.z > 1.0f)
        return float4(0, 0.3f, 1, 1); // blue: out of bounds

    float2 shadowMapUV = float2(0.5f + lp.x * 0.5f, 0.5f - lp.y * 0.5f);
    float vis = shadowMap.SampleCmpLevelZero(shadowSampler, shadowMapUV, lp.z - shadowBias).r;

    bool lit = NdotL > 0.0f && vis > 0.5f;
    return lit ? float4(0, 1, 0, 1) : float4(1, 0, 0, 1); // green: lit, red: shadowed
}

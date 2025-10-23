Texture2D normalTexture : register(t0);
Texture2D depthTexture : register(t1);

SamplerState pointClamp : register(s0);

cbuffer MatrixBuffer : register(b0)
{
    float4x4 invProj;
    float4x4 invView;
    
    float3 lightDirectionVS;
    float pad;
};

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float3 ReconstructViewPos(float2 uv)
{
    float depth = depthTexture.Sample(pointClamp, uv).r;
    uv = uv * 2 - 1;
    uv.y = -uv.y;
    float4 clipPos = float4(uv, depth, 1);

    float4 viewPosH = mul(clipPos, invProj);
    return viewPosH.xyz / viewPosH.w;
}

float4 PostprocessShader(PixelInputType input) : SV_TARGET
{
    const float PI = 3.14159;
    const float4 COLOR = float4(2, 2, 2, 1);
    const float3 AMBIENT = float3(0.1, 0.3, 0.3);
    const float ROUGHNESS = 0.2;
    
    float4 albedo = COLOR;
    float4 nr = normalTexture.Sample(pointClamp, input.uv);
    float3 normal = normalize(nr.rgb * 2 - 1);
    float alpha = max(ROUGHNESS, 0.01);
    alpha *= alpha;

    float3 viewPos = ReconstructViewPos(input.uv);
    float3 V = normalize(-viewPos);
    float3 L = normalize(-lightDirectionVS);
    float3 H = normalize(V + L);

	// DEBUG
	//return float4(albedo, 1.0);
	//return float4(viewPos.rgb, 1);
	//return float4(shadowMap.SampleLevel(pointClamp, input.uv, 0).rrr,1);

	// AMBIENT
    //float4 ambient = albedo * float4(AMBIENT, 1);
    float4 ambient = float4(AMBIENT, 1);
    float depth = depthTexture.Sample(pointClamp, input.uv).r;
    if (depth == 1)
        return ambient;

	// COOK-TORRANCE BSDF: https://learnopengl.com/PBR/Theory
    float NdotL = saturate(dot(normal, L));
    float NdotV = saturate(dot(normal, V));
    float NdotH = saturate(dot(normal, H));
    float VdotH = saturate(dot(V, H));

	// schlick fresnel
    float F0 = 0.04;
    float F = F0 + (1.0 - F0) * pow(1.0 - VdotH, 5.0);

	// ndf
    float alpha2 = alpha * alpha;
    float denomD = (NdotH * NdotH) * (alpha2 - 1.0) + 1.0;
    float D = alpha2 / (PI * denomD * denomD);

    // geometry function
    float k = (alpha + 1.0) * (alpha + 1.0) / 8.0;
    float G_V = NdotV / (NdotV * (1.0 - k) + k);
    float G_L = NdotL / (NdotL * (1.0 - k) + k);
    float G = G_V * G_L;

    float specular = (D * G * F) / (4.0 * NdotV * NdotL + 0.001);

    // lambertian diffuse
    float3 diffuse = (1.0 - F) * albedo / PI;

    float3 color = (diffuse + specular) * NdotL;

    //return normalTexture.Sample(pointClamp, input.uv);
    return float4(color, 1) + ambient;
};
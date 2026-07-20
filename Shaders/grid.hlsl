Texture2D depthTexture : register(t0);

SamplerState pointClamp : register(s0);

cbuffer GridBuffer : register(b0)
{
    float4x4 invViewProj;
    float4x4 viewProj;

    float3 cameraPosWS;
    float cellSize;

    float3 axisColorX; // drawn where worldPos.y == 0
    float majorLineEvery;

    float3 axisColorY; // drawn where worldPos.x == 0
    float fadeDistance;

    float3 lineColor;
    float pad;
};

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

// Unprojects this pixel's NDC xy at the given NDC depth back to world space.
// Used to build a camera-to-pixel ray for the ground-plane intersection below.
float3 UnprojectToWorld(float2 uv, float ndcZ)
{
    uv = uv * 2 - 1;
    uv.y = -uv.y;
    float4 clipPos = float4(uv, ndcZ, 1);
    float4 worldPosH = mul(clipPos, invViewProj);
    return worldPosH.xyz / worldPosH.w;
}

float4 PostprocessShader(PixelInputType input) : SV_TARGET
{
    // Build a world-space ray through this pixel and intersect it with the
    // world Z = 0 ground plane. (Swap .z for .y below if your scene is Y-up.)
    float3 farWS = UnprojectToWorld(input.uv, 1.0);
    float3 rayDir = normalize(farWS - cameraPosWS);

    if (abs(rayDir.z) < 1e-5)
        discard; // ray runs parallel to the ground plane

    float t = -cameraPosWS.z / rayDir.z;
    if (t < 0)
        discard; // ground plane is behind the camera

    float3 worldPos = cameraPosWS + rayDir * t;

    // Occlude the grid behind existing scene geometry by comparing depths
    float4 gridClip = mul(float4(worldPos, 1), viewProj);
    float gridDepth = gridClip.z / gridClip.w;
    float sceneDepth = depthTexture.Sample(pointClamp, input.uv).r;
    if (gridDepth > sceneDepth + 0.0001)
        discard;

    // Anti-aliased grid lines (Ben Golus' "Best Darn Grid Shader" approach)
    float2 coord = worldPos.xy / cellSize;
    float2 deriv = fwidth(coord);
    float2 gridLines = abs(frac(coord - 0.5) - 0.5) / max(deriv, 1e-6);
    float minorLine = 1.0 - saturate(min(gridLines.x, gridLines.y));

    float2 majorCoord = coord / majorLineEvery;
    float2 majorDeriv = fwidth(majorCoord);
    float2 majorLines = abs(frac(majorCoord - 0.5) - 0.5) / max(majorDeriv, 1e-6);
    float majorLine = 1.0 - saturate(min(majorLines.x, majorLines.y));

    float3 color = lerp(lineColor * 0.5, lineColor, majorLine);
    float alpha = max(minorLine, majorLine);

    // Highlight the X and Y axes
    float2 axisDeriv = max(fwidth(worldPos.xy), 1e-6);
    float xAxis = 1.0 - saturate(abs(worldPos.y) / axisDeriv.y - 1.0);
    float yAxis = 1.0 - saturate(abs(worldPos.x) / axisDeriv.x - 1.0);

    color = lerp(color, axisColorX, xAxis);
    color = lerp(color, axisColorY, yAxis);
    alpha = max(alpha, max(xAxis, yAxis));

    // Fade out towards the far plane so the grid doesn't hard-cut
    float dist = length(worldPos - cameraPosWS);
    float fade = saturate(1.0 - dist / fadeDistance);
    alpha *= fade * fade;

    if (alpha <= 0.001)
        discard;

    return float4(color, alpha);
};

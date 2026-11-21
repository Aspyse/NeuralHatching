Texture2D normalTexture : register(t0);
Texture2D depthTexture : register(t1);

cbuffer EdgeBuffer : register(b0)
{
    float2 screenSize;
    float normalThreshold;
    float depthThreshold;
    float2 nearFar;
    float2 pad;
};

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float3 LoadNormal(int2 pixel)
{
    return normalize(normalTexture.Load(int3(pixel, 0)).rgb * 2.0f - 1.0f);
}

float ViewDepth(float deviceDepth)
{
    return (nearFar.x * nearFar.y) /
        max(nearFar.y - deviceDepth * (nearFar.y - nearFar.x), 1e-4f);
}

float4 PostprocessShader(PixelInputType input) : SV_TARGET
{
    int2 pixel = int2(input.position.xy);
    int2 maxPixel = int2(screenSize) - 1;
    float centerDepth = depthTexture.Load(int3(pixel, 0)).r;

    if (centerDepth >= 0.99999f)
        return float4(1, 1, 1, 1);

    int2 left  = int2(max(pixel.x - 1, 0), pixel.y);
    int2 right = int2(min(pixel.x + 1, maxPixel.x), pixel.y);
    int2 up    = int2(pixel.x, max(pixel.y - 1, 0));
    int2 down  = int2(pixel.x, min(pixel.y + 1, maxPixel.y));

    float depthL = depthTexture.Load(int3(left, 0)).r;
    float depthR = depthTexture.Load(int3(right, 0)).r;
    float depthU = depthTexture.Load(int3(up, 0)).r;
    float depthD = depthTexture.Load(int3(down, 0)).r;

    if (depthL >= 0.99999f || depthR >= 0.99999f || depthU >= 0.99999f || depthD >= 0.99999f)
        return float4(0, 0, 0, 1);

    float3 centerNormal = LoadNormal(pixel);

    // 1/z is linear across a planar projection.
    float invCenterDepth = rcp(max(ViewDepth(centerDepth), 1e-4f));
    float invDepthL = rcp(max(ViewDepth(depthL), 1e-4f));
    float invDepthR = rcp(max(ViewDepth(depthR), 1e-4f));
    float invDepthU = rcp(max(ViewDepth(depthU), 1e-4f));
    float invDepthD = rcp(max(ViewDepth(depthD), 1e-4f));
    float depthBendX = abs(invDepthL + invDepthR - 2.0f * invCenterDepth) / invCenterDepth;
    float depthBendY = abs(invDepthU + invDepthD - 2.0f * invCenterDepth) / invCenterDepth;
    float depthInk = smoothstep(depthThreshold, depthThreshold * 2.0f, max(depthBendX, depthBendY));

    float normalJump = max(
        max(1.0f - saturate(dot(centerNormal, LoadNormal(left))),
            1.0f - saturate(dot(centerNormal, LoadNormal(right)))),
        max(1.0f - saturate(dot(centerNormal, LoadNormal(up))),
            1.0f - saturate(dot(centerNormal, LoadNormal(down)))));
    float normalInk = smoothstep(normalThreshold, normalThreshold * 1.5f, normalJump);
    float edge = max(depthInk, normalInk);

    return float4((1.0f - edge).xxx, 1);
}

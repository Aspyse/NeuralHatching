struct LineVertex
{
    float2 pos;
    float active;
    float padding;
};

// traced polyline vertices, written by hatch.hlsl's CSMain
StructuredBuffer<LineVertex> LineVertices : register(t0);

cbuffer HatchLineBuffer : register(b0)
{
    float2 ScreenSize;
    float LineWidthPx;
    int StepsPerSeed; // MaxSteps - 1, i.e. segments per seed's polyline

    float3 LineColor;
    float pad;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float coverageDist : TEXCOORD0; // signed distance from the centerline, in pixels
};

// Same UV -> clip convention as the grid shader's unprojection, just in reverse.
float4 UVToClip(float2 uv)
{
    float2 ndc = uv * 2.0 - 1.0;
    ndc.y = -ndc.y;
    return float4(ndc, 0.0, 1.0);
}

// Vertex-pulls one traced segment per 6 verts (two triangles, no vertex/index
// buffers) and expands it into a screen-space quad, same no-input-layout
// pattern as the rest of the pipeline's fullscreen passes.
PixelInputType HatchLineVertexShader(uint vertexID : SV_VertexID)
{
    PixelInputType output;

    uint quadVert = vertexID % 6;
    uint segmentID = vertexID / 6;
    uint seedID = segmentID / StepsPerSeed;
    uint stepID = segmentID % StepsPerSeed;

    uint vertsPerSeed = StepsPerSeed + 1;
    LineVertex a = LineVertices[seedID * vertsPerSeed + stepID];
    LineVertex b = LineVertices[seedID * vertsPerSeed + stepID + 1];

    // Dead segments (trace never reached here, or died on this step)
    // collapse to a zero-area triangle so nothing gets rasterized.
    if (a.active < 0.5 || b.active < 0.5)
    {
        // Collapse to a valid zero-area point (w=1, not w=0 -- a zero w is
        // undefined during clip-space clipping and can corrupt rasterization
        // instead of cleanly discarding).
        output.position = float4(0, 0, 0, 1);
        output.coverageDist = 0.0;
        return output;
    }

    float2 pixelA = (UVToClip(a.pos).xy * 0.5 + 0.5) * ScreenSize;
    float2 pixelB = (UVToClip(b.pos).xy * 0.5 + 0.5) * ScreenSize;

    float2 dir = normalize(pixelB - pixelA);
    float2 normal = float2(-dir.y, dir.x);

    // Widen a little past the nominal width so the AA falloff below has
    // room to fade to zero instead of getting hard-clipped by the quad edge.
    float halfWidth = LineWidthPx * 0.5 + 1.0;

    float2 corner;
    float side;
    if (quadVert == 0)      { corner = pixelA; side = -1.0; }
    else if (quadVert == 1) { corner = pixelA; side =  1.0; }
    else if (quadVert == 2) { corner = pixelB; side = -1.0; }
    else if (quadVert == 3) { corner = pixelA; side =  1.0; }
    else if (quadVert == 4) { corner = pixelB; side =  1.0; }
    else                    { corner = pixelB; side = -1.0; }

    float2 pixelPos = corner + normal * halfWidth * side;

    // UVToClip's Y-flip only survives the round trip to pixel space and back
    // if something re-applies it here -- otherwise it's canceled out by the
    // GPU's own implicit NDC->screen flip during rasterization, which is what
    // was leaving every traced line upside-down.
    float2 clipPos = pixelPos / ScreenSize * 2.0 - 1.0;
    //clipPos.y = -clipPos.y;

    output.position = float4(clipPos, 0.0, 1.0);
    output.coverageDist = halfWidth * side;

    return output;
}

float4 HatchLinePixelShader(PixelInputType input) : SV_TARGET
{
    // Anti-aliased edge falloff over ~1 pixel, same derivative-based
    // coverage test as the grid shader's line rendering.
    float aa = fwidth(input.coverageDist);
    float coverage = 1.0 - saturate((abs(input.coverageDist) - LineWidthPx * 0.5) / max(aa, 1e-5));

    if (coverage <= 0.001)
        discard;

    return float4(LineColor, coverage);
}

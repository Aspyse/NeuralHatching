Texture2D<float2> CurvatureField : register(t0); // g-buffer SRV
SamplerState LinearSampler : register(s0);

struct LineVertex
{
    float2 pos;
    float active;
    float padding;
};

// output UAV (u0)
RWStructuredBuffer<LineVertex> OutputBuffer : register(u0);

// collision UAV (u1)
RWTexture2D<uint> OccupancyGrid : register(u1);

cbuffer TraceParams : register(b0)
{
    int MaxSteps; // 50
    float StepSize; // 0.01
    float2 BoundsMin;
    float2 BoundsMax;
    int TotalSeeds;
    int OccGridWidth;
    int OccGridHeight;
    float pad;
};

// seed points SRV
StructuredBuffer<float2> SeedPoints : register(t1);

[numthreads(64, 1, 1)]
void CSMain(uint3 DTid : SV_DispatchThreadID)
{
    uint id = DTid.x;
    if (id >= TotalSeeds)
        return; // This return is safe (out of bounds of seed array)
    
    float2 currentPos = SeedPoints[id];
    uint offset = id * MaxSteps;
    bool isAlive = true;
    
    // Check initial bounds. Don't return! We still need to write inactive verts.
    if (any(currentPos < BoundsMin) || any(currentPos > BoundsMax))
    {
        isAlive = false;
    }

    float2 uv = (currentPos - BoundsMin) / (BoundsMax - BoundsMin);
    float2 prevVelocity = float2(1.0f, 0.0f); // Fallback
    
    if (isAlive)
    {
        prevVelocity = CurvatureField.SampleLevel(LinearSampler, uv, 0).xy * 2.0f - 1.0f;
        if (abs(prevVelocity.x) >= 1.0f || abs(prevVelocity.y) >= 1.0f)
        {
            isAlive = false;
        }
    }

    // FIX: Force the occupancy check to run on the very first iteration
    int2 prevGridPos = int2(-1, -1);

    for (int i = 0; i < MaxSteps; i++)
    {
        // write current vertex (safely pads the rest of the buffer with zeros if dead)
        OutputBuffer[offset + i].pos = currentPos;
        OutputBuffer[offset + i].active = isAlive ? 1.0 : 0.0;
        
        if (!isAlive)
            continue;

        uv = (currentPos - BoundsMin) / (BoundsMax - BoundsMin);
        
        // COLLISION CHECK
        int2 gridPos = int2(uv.x * OccGridWidth, uv.y * OccGridHeight);
        
        if (gridPos.x != prevGridPos.x || gridPos.y != prevGridPos.y)
        {
            if (gridPos.x >= 0 && gridPos.x < OccGridWidth &&
                gridPos.y >= 0 && gridPos.y < OccGridHeight)
            {
                uint cellCount;
                InterlockedAdd(OccupancyGrid[gridPos], 1, cellCount);
            
                if (cellCount > 0)
                {
                    isAlive = false;
                    continue;
                }
            }
            prevGridPos = gridPos;
        }

        // MIDPOINT INTEGRATION
        float2 newVelocity = CurvatureField.SampleLevel(LinearSampler, uv, 0).xy * 2.0f - 1.0f;

        // 2-RoSy check
        if (dot(prevVelocity, newVelocity) < 0.0f)
            newVelocity = -newVelocity;
        
        // step
        float2 lastPos = currentPos;
        float2 midPos = currentPos + newVelocity * StepSize * 0.5f;
        float2 midUV = (midPos - BoundsMin) / (BoundsMax - BoundsMin);
        float2 midVelocity = CurvatureField.SampleLevel(LinearSampler, midUV, 0).xy * 2.0f - 1.0f;
        
        // 2-RoSy check
        if (dot(prevVelocity, midVelocity) < 0.0f)
            midVelocity = -midVelocity;
        
        // safeguard FIX: Kill the line if it hits garbage velocity data
        if (abs(midVelocity.x) >= 1.0f || abs(midVelocity.y) >= 1.0f)
        {
            isAlive = false;
            continue;
        }
        
        currentPos += midVelocity * StepSize;
        prevVelocity = midVelocity;

        // bounds check FIX: Remove 'break' to allow inactive writes
        if (any(currentPos < BoundsMin) || any(currentPos > BoundsMax))
        {
            currentPos = lastPos;
            isAlive = false;
        }
    }
}
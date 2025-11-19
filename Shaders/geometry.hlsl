cbuffer MatrixBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VertexInputType
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
    float3 hatch : TEXCOORD1;
    float3 hatch2 : TEXCOORD2;
    float reliable : TEXCOORD3;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
    nointerpolation float3 hatch : TEXCOORD1;
    nointerpolation float3 hatch2 : TEXCOORD2;
    nointerpolation float reliable : TEXCOORD3;
    //float3 hatch : TEXCOORD1;
    float3 normal : NORMAL;
};

PixelInputType GeometryVertexShader(VertexInputType input)
{
    PixelInputType output;
    
    float4 pos = float4(input.position, 1);

    output.position = mul(pos, worldMatrix);
    output.position = mul(output.position, viewMatrix);
    output.position = mul(output.position, projectionMatrix);

    float3 N = normalize(input.normal);
    
    output.normal = N;
    
    // output cross field
    output.hatch = normalize(input.hatch);
    output.hatch2 = normalize(input.hatch2);
    output.reliable = input.reliable;
    
    return output;
}

struct PixelOutputType
{
    float3 normal : SV_Target0;
    float3 hatch : SV_Target1;
    float3 hatch2 : SV_Target2;
    float reliable : SV_Target3;
};

PixelOutputType GeometryPixelShader(PixelInputType input) : SV_TARGET
{
    PixelOutputType output;

    float3 normal = mul((float3x3) viewMatrix, normalize(input.normal));
    normal = normal * 0.5f + 0.5f;
    
    // hatch field placeholder
    //float hatch = input.hatch;
    float3 hatch = mul((float3x3) viewMatrix, normalize(input.hatch));
    hatch = hatch * 0.5f + 0.5f;
    
    float3 hatch2 = mul((float3x3) viewMatrix, normalize(input.hatch2));
    hatch2 = hatch2 * 0.5f + 0.5f;
    
    output.normal = normal;
    output.hatch = hatch;
    output.hatch2 = hatch2;
    output.reliable = input.reliable;

    return output;
}
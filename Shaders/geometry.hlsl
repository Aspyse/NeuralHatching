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
    float hatch : TEXCOORD1;
    float3 normal : NORMAL;
    float4 tangent : TANGENT;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
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
    
    return output;
}

struct PixelOutputType
{
    float4 normal : SV_Target0;
};

PixelOutputType GeometryPixelShader(PixelInputType input) : SV_TARGET
{
    PixelOutputType output;

    float3 normal = mul(normalize(input.normal), (float3x3) viewMatrix);
    normal = normal * 0.5f + 0.5f;
    output.normal = float4(normal, 1);

    return output;
}
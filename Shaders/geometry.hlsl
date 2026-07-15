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
    //nointerpolation float3 hatch : TEXCOORD1;
    //nointerpolation float3 hatch2 : TEXCOORD2;
    float3 hatch : TEXCOORD1;
    float3 hatch2 : TEXCOORD2;
    nointerpolation float reliable : TEXCOORD3;
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
    
    float3 objHatch = input.hatch;
    float3 objHatch2 = input.hatch2;
    
    float3 referenceRight = float3(1.0f, 0.0f, 0.0f);
    
    if (dot(objHatch, referenceRight) < 0.0f)
        objHatch = -objHatch;
    if (dot(objHatch2, referenceRight) < 0.0f)
        objHatch2 = -objHatch2;
    
    output.hatch = normalize(objHatch);
    output.hatch2 = normalize(objHatch2);
    
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
    
    float3x3 worldView = (float3x3) mul(worldMatrix, viewMatrix);

    float3 normal = normalize(input.normal);
    //normal = mul((float3x3) worldView, normal);
    normal = mul(normal, worldView);
    normal = normalize(normal);
    normal = normal * 0.5f + 0.5f;
    
    /*
    float3 objHatch = input.hatch;
    float3 objHatch2 = input.hatch2;
    
    float3 referenceRight = float3(1.0f, 0.0f, 0.0f);
    
    if (dot(objHatch, referenceRight) < 0.0f)
        objHatch = -objHatch;
    if (dot(objHatch2, referenceRight) < 0.0f)
        objHatch2 = -objHatch2;
    */
    
    float3 hatch = normalize(mul(input.hatch, worldView));
    float3 hatch2 = normalize(mul(input.hatch2, worldView));

    hatch = hatch * 0.5f + 0.5f;
    hatch2 = hatch2 * 0.5f + 0.5f;
    
    output.normal = normal;
    output.hatch = hatch;
    output.hatch2 = hatch2;
    output.reliable = input.reliable;

    return output;
}
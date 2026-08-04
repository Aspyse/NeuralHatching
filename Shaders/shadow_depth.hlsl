// renders scene depth from the light's POV into the shadow map, no color output

cbuffer ShadowMatrixBuffer : register(b0)
{
    float4x4 worldMatrix;
    float4x4 lightViewMatrix;
    float4x4 lightProjectionMatrix;
};

// same semantics as GeometryNode's input layout (POSITION/TEXCOORD0-3/NORMAL)
// so it can be reused as-is -- only position gets used below
struct VertexInputType
{
    float3 position : POSITION;
    float2 uv0 : TEXCOORD0;
    float3 uv1 : TEXCOORD1;
    float3 uv2 : TEXCOORD2;
    float uv3 : TEXCOORD3;
    float3 normal : NORMAL;
};

struct PixelInputType
{
    float4 position : SV_POSITION;
};

PixelInputType ShadowVertexShader(VertexInputType input)
{
    PixelInputType output;
    float4 worldPos = mul(float4(input.position, 1.0f), worldMatrix);
    float4 viewPos = mul(worldPos, lightViewMatrix);
    output.position = mul(viewPos, lightProjectionMatrix);
    return output;
}

// no RTV bound during this pass, just depth -- nothing to write
void ShadowPixelShader(PixelInputType input)
{
}

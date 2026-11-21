Texture2D rawPredictionTexture : register(t0); // rgb = raw model output, a = foreground mask

SamplerState pointClamp : register(s0);

struct PixelInputType
{
    float2 uv : TEXCOORD0;
    float4 position : SV_POSITION;
};

float4 PostprocessShader(PixelInputType input) : SV_TARGET
{
    float4 raw = rawPredictionTexture.Sample(pointClamp, input.uv);
    float3 pred = raw.rgb;
    float mask = raw.a;

    // py: val_pred = val_pred / (norm(val_pred) + 7e-5); val_pred = (val_pred+1)/2; val_pred *= mask
    pred = pred / (length(pred) + 7e-5);
    pred = (pred + 1.0) * 0.5;
    pred *= mask;

    return float4(pred, 1);
};

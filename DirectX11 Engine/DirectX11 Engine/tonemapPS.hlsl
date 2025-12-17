cbuffer TonemapParams : register(b0) {
    float exposure;
    float gamma;
    int tonemapOperator;
    float padding;
};

Texture2D hdrTexture : register(t0);
SamplerState samplerState : register(s0);

float3 Reinhard(float3 color) 
{
    return color / (1.0f + color);
}

float3 ACES(float3 x) 
{
    const float a = 2.51f;
    const float b = 0.03f;
    const float c = 2.43f;
    const float d = 0.59f;
    const float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

float3 Uncharted2Tonemap(float3 x) 
{
    float A = 0.15f;
    float B = 0.50f;
    float C = 0.10f;
    float D = 0.20f;
    float E = 0.02f;
    float F = 0.30f;
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 Uncharted2(float3 color) 
{
    const float W = 11.2f;
    color = Uncharted2Tonemap(color);
    float3 whiteScale = 1.0f / Uncharted2Tonemap(W);
    return color * whiteScale;
}

float4 main(float4 position : SV_POSITION, float2 texcoord : TEXCOORD) : SV_Target
{
    float3 hdrColor = hdrTexture.Sample(samplerState, texcoord).rgb;
    
    hdrColor *= exposure;
    
    float3 mapped;

    switch (tonemapOperator)
    {
        case 0:
            mapped = Reinhard(hdrColor);
            break;
        case 1:
            mapped = ACES(hdrColor);
            break;
        case 2:
            mapped = Uncharted2(hdrColor);
            break;
        default:
            mapped = hdrColor;
            break;
    }
    
    mapped = pow(mapped, 1.0f / gamma);
    return float4(mapped, 1.0f);
}
cbuffer lightBlobColor : register(b1)
{
    float4 lightColor;
};

struct PS_INPUT
{
    float4 inPosition : SV_Position;
    float2 inTexCoord : TEXCOORD;
    float3 inNormal : Normal;
    float3 inWorldPos : WORLD_POSITION;
};

Texture2D objTexture : TEXTURE : register(t0);
SamplerState objSamplerState : SAMPLER : register(s0);


float4 main(PS_INPUT input) : SV_Target
{
    float3 sampleColor = objTexture.Sample(objSamplerState, input.inTexCoord);
    return float4(sampleColor * lightColor.xyz, lightColor.w);
}
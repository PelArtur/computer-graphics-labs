cbuffer lightBuffer : register(b0)
{
    float3 ambientLightColor;
    float ambientLightStrength;
    float3 dynamicLightColor;
    float dynamicLightStrength;
    float3 dynamicLightPosition;
}
  
  
struct PS_INPUT
{
    float4 inPosition : SV_Position;
    float2 inTexCoord : TEXCOORD;
    float3 inNormal   : Normal;
    float3 inWorldPos : WORLD_POSITION;
};

Texture2D objTexture : TEXTURE : register(t0);
SamplerState objSamplerState : SAMPLER : register(s0);


float4 main(PS_INPUT input) : SV_Target
{
    float3 sampleColor = objTexture.Sample(objSamplerState, input.inTexCoord);
    
    //ambient light
    float3 appliedLight = ambientLightColor * ambientLightStrength;
    
    //diffuse light
    float3 vectorToLight = normalize(dynamicLightPosition - input.inWorldPos);
    float3 diffuseLightIntensity = max(dot(vectorToLight, input.inNormal), 0.0f);
    float3 diffuseLight = diffuseLightIntensity * dynamicLightStrength * dynamicLightColor;
    appliedLight += diffuseLight;
    
    float3 finalColor = sampleColor * appliedLight;
    return float4(finalColor, 1.0f);
}
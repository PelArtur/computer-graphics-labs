#define MAX_LIGHTS 4

struct LightData
{
    float3 color;
    float strength;

    float3 position;
    int type; // 0=Directional, 1=Point, 2=Spot

    float3 direction;
    float attenuation_a;

    float attenuation_b;
    float attenuation_c;
    float spotInnerAngle;
    float spotOuterAngle;
   
    int turnOnBlinn;
    int shininess;
    int lightOn;
    float pad;
    
    row_major float4x4 lightViewProj;
};


cbuffer lightBuffer : register(b0)
{
    float3 ambientLightColor;
    float ambientLightStrength;

    LightData lights[MAX_LIGHTS];
    int numLights;
    float3 cameraPos;
    float shadowBias;
    float2 texelSize;
    int pcfKernalSize;
};


cbuffer shadowBuffer : register(b1)
{
    row_major float4x4 lightViewProj;
}


struct PS_INPUT
{
    float4 inPosition : SV_Position;
    float2 inTexCoord : TEXCOORD;
    float3 inNormal : Normal;
    float3 inWorldPos : WORLD_POSITION;
};

Texture2D objTexture : TEXTURE : register(t0);
SamplerState objSamplerState : SAMPLER : register(s0);
Texture2DArray shadowMaps : register(t1);
SamplerComparisonState shadowSampler : register(s1);

float CalculateShadow(float3 worldPos, float4x4 lightVP, int shadowIndex)
{
    float4 shadowPos = mul(float4(worldPos, 1.0), lightVP);
    
    shadowPos.xyz /= shadowPos.w;
    
    shadowPos.xy = shadowPos.xy * 0.5 + 0.5;
    shadowPos.y = 1.0 - shadowPos.y;
    
    if (shadowPos.x < 0.0 || shadowPos.x > 1.0 ||
        shadowPos.y < 0.0 || shadowPos.y > 1.0 ||
        shadowPos.z < 0.0 || shadowPos.z > 1.0)
    {
        return 1.0f;
    }

    uint width, height, arraySize;
    shadowMaps.GetDimensions(width, height, arraySize);
    
    uint2 texelPos = uint2(shadowPos.x * width, shadowPos.y * height);
    
    float shadow = 0.0f;
    float power = pow(pcfKernalSize, 2);
    int bounds = pcfKernalSize / 2;
    
    for (int x = -bounds; x <= bounds; ++x)
    {
        for (int y = -bounds; y <= bounds; ++y)
        {
            uint2 samplePos = texelPos + uint2(x, y);
            
            if (samplePos.x >= width || samplePos.y >= height)
                continue;
                
            float sampledDepth = shadowMaps.Load(int4(samplePos.x, samplePos.y, shadowIndex, 0)).r;
            float currentDepth = shadowPos.z - shadowBias;
            
            if (currentDepth <= sampledDepth)
                shadow += 1.0f;
        }
    }
    
    return shadow / power;
}

float4 main(PS_INPUT input) : SV_Target
{
    float3 sampleColor = objTexture.Sample(objSamplerState, input.inTexCoord);
    float3 appliedLight = ambientLightColor * ambientLightStrength;

    float3 N = normalize(input.inNormal);
    float3 V = normalize(cameraPos - input.inWorldPos);
    
    float shadowFactor = 1.0f;
    
    for (int i = 0; i < numLights; i++)
    {
        LightData currentLight = lights[i];
        
        if (!currentLight.lightOn || currentLight.strength == 0.0f)
            continue;
            
        float3 L; // Light vector
        float att = 1.0f; // Attenuation
        
        if (currentLight.type == 0) // Directional
        {
            L = normalize(-currentLight.direction);
            shadowFactor = CalculateShadow(input.inWorldPos, lights[i].lightViewProj, i);
        }
        else
        {
            L = currentLight.position - input.inWorldPos;
            float distanceToLight = length(L);
            L = normalize(L);

            att = 1.0f / (currentLight.attenuation_a +
                          currentLight.attenuation_b * distanceToLight +
                          currentLight.attenuation_c * distanceToLight * distanceToLight);

            if (currentLight.type == 2) // Spot
            {
                float3 spotDir = normalize(-currentLight.direction);
                float theta = dot(L, spotDir);
                float innerCos = cos(currentLight.spotInnerAngle);
                float outerCos = cos(currentLight.spotOuterAngle);
                float epsilon = innerCos - outerCos;
                float smoothEdge = saturate((theta - outerCos) / epsilon);
                shadowFactor = CalculateShadow(input.inWorldPos, lights[i].lightViewProj, i);
                att *= smoothEdge;
            }
        }

        float diff = max(dot(N, L), 0.0f);
        float3 H = normalize(L + V);
        
        float spec;
        if (currentLight.turnOnBlinn)
            spec = pow(max(dot(N, H), 0.0f), currentLight.shininess);
        else
        {
            float3 R = reflect(-L, N);
            spec = pow(max(dot(R, V), 0.0f), currentLight.shininess);
        }

        float3 diffuse = diff * currentLight.color;
        float3 specular = spec * currentLight.color;

        appliedLight += (diffuse + specular) * att * currentLight.strength * shadowFactor;
        shadowFactor = 1.0f;
    }

    float3 finalColor = sampleColor * appliedLight;
    return float4(finalColor, 1.0f);
}

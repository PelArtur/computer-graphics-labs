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
};


cbuffer lightBuffer : register(b0)
{
    float3 ambientLightColor;
    float ambientLightStrength;

    LightData lights[MAX_LIGHTS];
    int numLights;
    float3 cameraPos;
};

cbuffer terrainBuffer : register(b2)
{
    float minHeight;
    float maxHeight;
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

float4 GetTerrainColor(float normalizedHeight)
{
    if (normalizedHeight < 0.1f)
        return float4(0.0f, 0.1f, 0.5f, 1.0f); // Deep water
    if (normalizedHeight < 0.15f)
        return float4(0.0f, 0.3f, 0.8f, 1.0f); // Shallow water
    if (normalizedHeight < 0.2f)
        return float4(0.9f, 0.9f, 0.5f, 1.0f); // Sand
    if (normalizedHeight < 0.35f)
        return float4(0.1f, 0.6f, 0.1f, 1.0f); // Grass
    if (normalizedHeight < 0.8f)
        return float4(0.4f, 0.3f, 0.2f, 1.0f); // Rock
    return float4(1.0f, 1.0f, 1.0f, 1.0f);     // Snow
}


float4 main(PS_INPUT input) : SV_Target
{
    float3 sampleColor = objTexture.Sample(objSamplerState, input.inTexCoord);
    float3 appliedLight = ambientLightColor * ambientLightStrength;

    float3 N = normalize(input.inNormal);
    float3 V = normalize(cameraPos - input.inWorldPos);
    
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

        appliedLight += (diffuse + specular) * att * currentLight.strength;
    }
    
    float height = input.inWorldPos.y;
    float normalizedHeight = saturate((height - minHeight) / (maxHeight - minHeight));
    
    float4 heightColor = GetTerrainColor(normalizedHeight);
    float3 finalColor = heightColor.xyz * appliedLight;
    return float4(finalColor, 1.0f);
}
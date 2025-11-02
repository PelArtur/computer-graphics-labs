cbuffer lightBuffer : register(b0)
{
    float3 ambientLightColor;
    float ambientLightStrength;
 
    float3 dynamicLightColor;
    float dynamicLightStrength;
 
    float3 dynamicLightPosition;
    int lightType;

    // Directional light
    float3 dynamicLightDirection;
    float pad;

    // Point light attenuation
    float dynamicLightAttenuation_a;
    float dynamicLightAttenuation_b;
    float dynamicLightAttenuation_c;

    // Spot light
    float spotInnerAngle;
    float spotOuterAngle;
    int turnOnBlinn;
    int shininess;
    
    float3 cameraPos;
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
    float3 appliedLight = ambientLightColor * ambientLightStrength;

    float3 N = normalize(input.inNormal);
    float3 V = normalize(cameraPos - input.inWorldPos);

    float3 L;
    float att = 1.0f;
    float intensity = 1.0f;

    // --- Determine light direction/attenuation/intensity ---
    if (lightType == 0) // Directional
    {
        L = normalize(-dynamicLightDirection);
    }
    else
    {
        L = dynamicLightPosition - input.inWorldPos;
        float distanceToLight = length(L);
        L = normalize(L);

        att = 1.0f / (dynamicLightAttenuation_a +
                      dynamicLightAttenuation_b * distanceToLight +
                      dynamicLightAttenuation_c * distanceToLight * distanceToLight);

        if (lightType == 2) // Spot
        {
            float3 spotDir = normalize(-dynamicLightDirection);
            float theta = dot(L, spotDir);
            float innerCos = cos(spotInnerAngle);
            float outerCos = cos(spotOuterAngle);
            float epsilon = innerCos - outerCos;
            float smoothEdge = saturate((theta - outerCos) / epsilon);
            att *= smoothEdge;
        }
    }

    // --- Lighting computation ---
    if (turnOnBlinn)
    {
        // Blinn–Phong shading
        float diff = max(dot(N, L), 0.0f);

        float3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0f), shininess);

        float3 diffuse = diff * dynamicLightColor;
        float3 specular = spec * dynamicLightColor;

        appliedLight += (diffuse + specular) * att * dynamicLightStrength;
    }
    else
    {
        float diff = max(dot(N, L), 0.0f);
        float3 R = reflect(-L, N);
        float spec = pow(max(dot(R, V), 0.0f), shininess);

        float3 diffuse = diff * dynamicLightColor;
        float3 specular = spec * dynamicLightColor;

        appliedLight += (diffuse + specular) * att * dynamicLightStrength;
    }

    float3 finalColor = sampleColor * appliedLight;
    return float4(finalColor, 1.0f);
}

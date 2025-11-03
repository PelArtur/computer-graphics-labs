#pragma once
#include <DirectXMath.h>

#define MAX_LIGHTS 4 

struct CB_VS_vertexShader
{
	DirectX::XMMATRIX wvpMatrix;
	DirectX::XMMATRIX worldMatrix;
};


struct LightData
{
	DirectX::XMFLOAT3 color;
	float strength;

	DirectX::XMFLOAT3 position;
	int type; // 0=Directional, 1=Point, 2=Spot

	DirectX::XMFLOAT3 direction;
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


struct CB_PS_LightsData
{
	DirectX::XMFLOAT3 ambientLightColor;
	float ambientLightStrength;

	LightData lights[MAX_LIGHTS];
	int numLights;
	DirectX::XMFLOAT3 cameraPos;
};


struct CB_PS_LightColor
{
	DirectX::XMFLOAT3 lightColor;
	float padding;
};


struct CB_PS_light
{
	DirectX::XMFLOAT3 ambientLightColor;
	float ambientLightStrength;

	DirectX::XMFLOAT3 dynamicLightColor;
	float dynamicLightStrength;

	DirectX::XMFLOAT3 dynamicLightPosition;
	int lightType;

	DirectX::XMFLOAT3 dynamicLightDirection;
	float pad;

	float dynamicLightAttenuation_a;
	float dynamicLightAttenuation_b;
	float dynamicLightAttenuation_c;

	float spotInnerAngle;
	float spotOuterAngle;
	int turnOnBlinn;
	int shininess;

	DirectX::XMFLOAT3 cameraPos;
};


struct Voronoise_pixelShader
{
	DirectX::XMFLOAT4 iResolution;
	float iTime;
	float padding[3];
	DirectX::XMFLOAT4 iMouse;
};


struct Warp_pixelShader
{
	DirectX::XMFLOAT4 iResolution;
	float iTime;
};

#pragma once
#include <DirectXMath.h>

//constrant buffer, vertex shader
struct CB_VS_vertexShader
{
	DirectX::XMMATRIX wvpMatrix;
	DirectX::XMMATRIX worldMatrix;
};


struct CB_PS_light
{
	DirectX::XMFLOAT3 ambientLightColor;
	float ambientLightStrength;

	DirectX::XMFLOAT3 dynamicLightColor;
	float dynamicLightStrength;
	DirectX::XMFLOAT3 dynamicLightPosition;
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

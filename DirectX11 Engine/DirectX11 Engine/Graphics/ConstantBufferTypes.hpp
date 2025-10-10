#pragma once
#include <DirectXMath.h>

//constrant buffer, vertex shader
struct CB_VS_vertexShader
{
	DirectX::XMFLOAT4X4 mat;
};


struct CB_PS_pixelShader
{
	float alpha = 1.0f;
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

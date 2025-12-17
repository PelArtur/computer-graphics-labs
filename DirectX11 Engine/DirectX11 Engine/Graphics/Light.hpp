#pragma once
#include "RenderableGameObject.hpp"
#include "ConstantBufferTypes.hpp"

enum class LightType : int
{
	Directional = 0,
	Point = 1,
	Spot = 2
};


class Light : public RenderableGameObject
{
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader, LightType type);
	void Draw(const XMMATRIX& viewProjectionMatrix);

	LightType type = LightType::Point;
	DirectX::XMFLOAT3 lightColor = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	DirectX::XMFLOAT3 lightPosition = DirectX::XMFLOAT3(1.0f, 1.0f, 1.0f);
	float lightStrength = 1.0f;
	bool lightOn = true;
};

XMMATRIX CalculateDirectionalLightVP(XMFLOAT3 direction);
XMMATRIX CalculateSpotlightVP(const LightData& light);

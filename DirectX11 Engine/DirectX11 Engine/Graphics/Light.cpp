#include "Light.hpp"

bool Light::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader)
{
	if (!model.Initialize("D:/UCU Fourth Year/Graphics/DirectX11 Engine/DirectX11 Engine/Data/Models/Light/light.fbx", device, deviceContext, cb_vertexShader))
		return false;

	this->SetPosition(0.0f, 0.0f, 0.0f);
	this->SetRotation(0.0f, 0.0f, 0.0f);
	this->UpdateMatrix();
	return true;
}

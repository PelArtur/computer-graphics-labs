#include "Light.hpp"

bool Light::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader, LightType type)
{
	if (!model.Initialize("Data/Models/Light Ball/white_ball.glb", device, deviceContext, cb_vertexShader))
		return false;

	this->type = type;
	this->SetPosition(0.0f, 0.0f, 0.0f);
	this->SetRotation(0.0f, 0.0f, 0.0f);
	this->SetScale(1.0f, 1.0f, 1.0f);
	this->UpdateMatrix();
	return true;
}

void Light::Draw(const XMMATRIX& viewProjectionMatrix)
{
	this->SetPosition(lightPosition);
	model.Draw(this->worldMatrix, viewProjectionMatrix);
}

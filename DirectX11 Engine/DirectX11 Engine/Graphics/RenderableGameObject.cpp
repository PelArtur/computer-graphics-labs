#include "RenderableGameObject.hpp"


bool RenderableGameObject::Initialize(const std::string& filePath, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader)
{
	if (!model.Initialize(filePath, device, deviceContext, cb_vertexShader))
		return false;

	this->UpdateMatrix();
	return true;
}


bool RenderableGameObject::Initialize(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, std::vector<Texture>& textures, const XMMATRIX& transform, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader)
{
	if (!model.Initialize(vertices, indices, textures, transform, device, deviceContext, cb_vertexShader))
		return false;

	this->UpdateMatrix();
	return true;
}


void RenderableGameObject::Draw(const XMMATRIX& viewProjectionMatrix)
{
	model.Draw(this->worldMatrix, viewProjectionMatrix);
}


void RenderableGameObject::SetInstanceData(const std::vector<InstanceMatrixData>& data, ID3D11Device* device)
{
	this->model.SetInstanceData(data, device);
}


void RenderableGameObject::UpdateMatrix()
{
	this->worldMatrix = XMMatrixRotationRollPitchYaw(this->rot.x, this->rot.y, this->rot.z) * XMMatrixTranslation(this->pos.x, this->pos.y, this->pos.z);
	XMMATRIX vecRotationMatrix = XMMatrixRotationRollPitchYaw(0.0f, this->rot.y, 0.0f);
	this->vec_forward = XMVector3TransformCoord(this->DEFAULT_FORWARD_VECTOR, vecRotationMatrix);
	this->vec_backward = XMVector3TransformCoord(this->DEFAULT_BACKWARD_VECTOR, vecRotationMatrix);
	this->vec_left = XMVector3TransformCoord(this->DEFAULT_LEFT_VECTOR, vecRotationMatrix);
	this->vec_right = XMVector3TransformCoord(this->DEFAULT_RIGHT_VECTOR, vecRotationMatrix);
}

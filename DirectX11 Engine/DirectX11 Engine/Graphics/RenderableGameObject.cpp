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
	this->worldMatrix = XMMatrixScaling(this->scl.x, this->scl.y, this->scl.z) *  XMMatrixRotationRollPitchYaw(this->rot.x, this->rot.y, this->rot.z) * XMMatrixTranslation(this->pos.x, this->pos.y, this->pos.z);
	this->UpdateDirectionVectors();
}


bool FullscreenQuad::Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext)
{
	this->device = device;
	this->deviceContext = deviceContext;

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.ByteWidth = sizeof(FullscreenVertex) * vertices.size();;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices.data();

	HRESULT hr = this->device->CreateBuffer(&vbDesc, &vbData, fullscreenQuadVertexBuffer.GetAddressOf());
	COM_ERROR_IF_FAILED(hr, "Failed to create fullscreen triangle");
	return true;
}


void FullscreenQuad::Draw()
{
	UINT stride = sizeof(FullscreenVertex);
	UINT offset = 0;
	this->deviceContext->IASetVertexBuffers(0, 1, fullscreenQuadVertexBuffer.GetAddressOf(), &stride, &offset);
	this->deviceContext->Draw(3, 0);
}

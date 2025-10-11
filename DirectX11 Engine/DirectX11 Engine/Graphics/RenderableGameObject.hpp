#pragma once
#include "GameObject.hpp"


class RenderableGameObject : public GameObject
{
public:
	bool Initialize(const std::string& filePath, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader);
	bool Initialize(
		std::vector<Vertex>& vertices,
		std::vector<DWORD>& indices,
		std::vector<Texture>& textures,
		const XMMATRIX& transform,
		ID3D11Device* device,
		ID3D11DeviceContext* deviceContext,
		ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader
	);
	void Draw(const XMMATRIX& viewProjectionMatrix);
	void SetInstanceData(const std::vector<InstanceMatrixData>& data, ID3D11Device* device);
private:
	Model model;
	void UpdateMatrix() override;

	XMMATRIX worldMatrix = XMMatrixIdentity();
};

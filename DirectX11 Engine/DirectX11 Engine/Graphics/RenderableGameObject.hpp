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
protected:
	Model model;
	void UpdateMatrix() override;

	XMMATRIX worldMatrix = XMMatrixIdentity();
};


class FullscreenQuad
{
public:
	bool Initialize(ID3D11Device* device, ID3D11DeviceContext* deviceContext);
	void Draw();

private:
	std::vector<FullscreenVertex> vertices = {
		FullscreenVertex(- 1.0f, -1.0f, 0.0f, 0.0f, 2.0f),  // Bottom-left
		FullscreenVertex(3.0f, -1.0f, 0.0f, 2.0f, 2.0f),    // Bottom-right
		FullscreenVertex(-1.0f,  3.0f, 0.0f, 0.0f, 0.0f)    // Top-left
	};

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> fullscreenQuadVertexBuffer;
};
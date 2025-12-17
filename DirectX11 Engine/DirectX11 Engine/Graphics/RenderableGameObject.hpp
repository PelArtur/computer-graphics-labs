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


static std::vector<Vertex> skyboxVertices = {
	Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),

	Vertex(-1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),

	Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f,  1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f,  1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),

	Vertex(-1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),

	Vertex(-1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f,  1.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f, -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(-1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),

	Vertex(1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f,  1.0f,  1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
	Vertex(1.0f, -1.0f,  1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f),
};


static std::vector<Vertex> planeVertices = {
	Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f),
	Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f),
	Vertex(0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f),
	Vertex(0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f),

	Vertex(0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f),
	Vertex(0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f),
	Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f),
	Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f),

	Vertex(0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f),
	Vertex(0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f),
	Vertex(-0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f),
	Vertex(-0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f),

	Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f),
	Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
	Vertex(-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f),
	Vertex(-0.5f,  0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f),

	Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f),
	Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f),
	Vertex(0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f),

	Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f),
	Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f),
	Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f),
	Vertex(0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f)
};


static std::vector<DWORD> planeIndices = {
	0, 1, 2,
	4, 5, 6,
	9, 11, 10,
	13, 15, 14,
	16, 17, 18,
	21, 23, 22,
	0, 2, 3,
	4, 6, 7,
	8, 9, 10,
	12, 13, 14,
	16, 18, 19,
	20, 21, 22
};

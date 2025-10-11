#pragma once
#include "Mesh.hpp"
#include "InstanceBuffer.hpp"

using namespace DirectX;


struct InstanceMatrixData
{
	DirectX::XMMATRIX instanceMatrix;
};


class Model
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
	void Draw(const XMMATRIX& worldMatrix, const XMMATRIX& viewProjectionMatrix);
	const std::vector<InstanceMatrixData> getDefaultInstanceMatrix();
	void SetInstanceData(const std::vector<InstanceMatrixData>& data, ID3D11Device* device);
	
	UINT GetInstanceCount() const { return (UINT)instanceData.size(); }
	ID3D11Buffer* const* GetInstanceBufferAddressOf() const { return instanceBuffer.GetAddressOf(); }
	const UINT* GetInstanceBufferStridePtr() const { return instanceBuffer.StridePtr(); }

private:
	std::vector<Mesh> meshes;
	bool LoadModel(const std::string& filePath);
	void ProcessNode(aiNode* node, const aiScene* scene, const XMMATRIX& parentTransformMatrix);
	Mesh ProcessMesh(aiMesh* mesh, const aiScene* scene, const XMMATRIX& transformMatrix);
	TextureStorageType DetermineTextureStorageType(const aiScene* pScene, aiMaterial* pMat, unsigned int index, aiTextureType textureType);
	std::vector<Texture> LoadMaterialTextures(aiMaterial* pMaterial, aiTextureType textureType, const aiScene* pScene);
	int GetTextureIndex(aiString* pStr);

	ID3D11Device* device = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	ConstantBuffer<CB_VS_vertexShader>* cb_vertexShader = nullptr;
	std::string directory = "";

	std::vector<InstanceMatrixData> instanceData;
	InstanceBuffer<InstanceMatrixData> instanceBuffer;
};

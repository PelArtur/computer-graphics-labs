#include "Model.hpp"


bool Model::Initialize(const std::string& filePath, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader)
{
	this->device = device;
	this->deviceContext = deviceContext;
	this->cb_vertexShader = &cb_vertexShader;
	auto defaultInstanceMatrix = getDefaultInstanceMatrix();
	SetInstanceData(defaultInstanceMatrix, device);

	try
	{
		if (!this->LoadModel(filePath))
			return false;
	}
	catch (COMException& exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}

	return true;
}


bool Model::Initialize(std::vector<Vertex>& vertices, std::vector<DWORD>& indices, std::vector<Texture>& textures, const XMMATRIX& transform, ID3D11Device* device, ID3D11DeviceContext* deviceContext, ConstantBuffer<CB_VS_vertexShader>& cb_vertexShader)
{
	this->device = device;
	this->deviceContext = deviceContext;
	this->cb_vertexShader = &cb_vertexShader;

	try
	{
		meshes.emplace_back(Mesh(device, deviceContext, vertices, indices, textures, transform));
	}
	catch (COMException& exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}

	return true;
}


void Model::Draw(const XMMATRIX& worldMatrix, const XMMATRIX& viewProjectionMatrix)
{
	this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vertexShader->GetAddressOf());
	XMMATRIX WVP = worldMatrix * viewProjectionMatrix;

	for (int i = 0; i < meshes.size(); i++)
	{
		this->cb_vertexShader->data.mat = meshes[i].GetTransformMatrix() * WVP;
		this->cb_vertexShader->ApplyChanges();

		if (this->GetInstanceCount() == 0)
		{
			meshes[i].Draw();
		}
		else
		{
			meshes[i].DrawInstanced(
				this->GetInstanceBufferAddressOf(),
				this->GetInstanceBufferStridePtr(),
				this->GetInstanceCount()
			);
		}
	}
}


const std::vector<InstanceMatrixData> Model::getDefaultInstanceMatrix()
{
	std::vector<InstanceMatrixData> instanceBasicMatrix;
	InstanceMatrixData data;
	data.instanceMatrix = XMMatrixIdentity();
	instanceBasicMatrix.push_back(data);
	return instanceBasicMatrix;
}


void Model::SetInstanceData(const std::vector<InstanceMatrixData>& data, ID3D11Device* device)
{
	if (data.size() > 0)
		this->instanceData = data;
	else 
		this->instanceData = getDefaultInstanceMatrix();

	HRESULT hr = this->instanceBuffer.Initialize(device, this->instanceData.data(), (UINT)this->instanceData.size());
	COM_ERROR_IF_FAILED(hr, "Failed to initialize instance buffer for model.");
}


bool Model::LoadModel(const std::string& filePath)
{
	this->directory = StringHelper::GetDirectoryFromPath(filePath);
	Assimp::Importer importer;

	const aiScene* pScene = importer.ReadFile(filePath,
		aiProcess_Triangulate | aiProcess_ConvertToLeftHanded);

	if (pScene == nullptr)
		return false;

	this->ProcessNode(pScene->mRootNode, pScene, DirectX::XMMatrixIdentity());
	return true;
}


void Model::ProcessNode(aiNode* node, const aiScene* scene, const XMMATRIX& parentTransformMatrix)
{
	XMMATRIX nodeTransformMatrix = XMMatrixTranspose(XMMATRIX(&node->mTransformation.a1)) * parentTransformMatrix;

	for (UINT i = 0; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.emplace_back(this->ProcessMesh(mesh, scene, nodeTransformMatrix));
	}

	for (UINT i = 0; i < node->mNumChildren; ++i)
	{
		this->ProcessNode(node->mChildren[i], scene, nodeTransformMatrix);
	}
}


Mesh Model::ProcessMesh(aiMesh* mesh, const aiScene* scene, const XMMATRIX& transformMatrix)
{
	std::vector<Vertex> vertices;
	std::vector<DWORD> indices;

	for (UINT i = 0; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex;

		vertex.pos.x = mesh->mVertices[i].x;
		vertex.pos.y = mesh->mVertices[i].y;
		vertex.pos.z = mesh->mVertices[i].z;

		if (mesh->mTextureCoords[0])
		{
			vertex.texCoord.x = (float)mesh->mTextureCoords[0][i].x;
			vertex.texCoord.y = (float)mesh->mTextureCoords[0][i].y;
		}

		vertices.emplace_back(vertex);
	}

	for (UINT i = 0; i < mesh->mNumFaces; ++i)
	{
		aiFace face = mesh->mFaces[i];

		for (UINT j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	std::vector<Texture> textures;
	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
	std::vector<Texture> diffuseTextures = LoadMaterialTextures(material, aiTextureType::aiTextureType_DIFFUSE, scene);
	textures.insert(textures.end(), diffuseTextures.begin(), diffuseTextures.end());

	return Mesh(this->device, this->deviceContext, vertices, indices, textures, transformMatrix);
}


std::vector<Texture> Model::LoadMaterialTextures(aiMaterial* pMaterial, aiTextureType textureType, const aiScene* pScene)
{
	std::vector<Texture> materialTextures;
	TextureStorageType storetype = TextureStorageType::Invalid;
	unsigned int textureCount = pMaterial->GetTextureCount(textureType);

	if (textureCount == 0)
	{
		storetype = TextureStorageType::None;
		aiColor3D aiColor(0.0f, 0.0f, 0.0f);
		switch (textureType)
		{
		case aiTextureType_DIFFUSE:
			pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, aiColor);
			if (aiColor.IsBlack())
			{
				materialTextures.emplace_back(Texture(this->device, Colors::UnloadedTextureColor, textureType));
				return materialTextures;
			}
			materialTextures.emplace_back(Texture(this->device, Color(aiColor.r * 255, aiColor.g * 255, aiColor.b * 255), textureType));
			return materialTextures;
		}
	}
	else
	{
		for (UINT i = 0; i < textureCount; i++)
		{
			aiString path;
			pMaterial->GetTexture(textureType, i, &path);
			TextureStorageType storetype = DetermineTextureStorageType(pScene, pMaterial, i, textureType);
			switch (storetype)
			{
				case TextureStorageType::Disk:
				{
					std::string filename = this->directory + '/' + path.C_Str();
					//std::string filename = path.C_Str();
					Texture diskTexture(this->device, filename, textureType);
					materialTextures.emplace_back(diskTexture);
					break;
				}
				case TextureStorageType::EmbeddedCompressed:
				{
					const aiTexture* pTexture = pScene->GetEmbeddedTexture(path.C_Str());
					Texture embeddedTexture(this->device,
											reinterpret_cast<uint8_t*>(pTexture->pcData),
											pTexture->mWidth,
											textureType);
					materialTextures.emplace_back(embeddedTexture);
					break;
				}
				case TextureStorageType::EmbeddedIndexCompressed:
				{
					int index = GetTextureIndex(&path);
					Texture embeddedIndexedTexture(this->device,
												   reinterpret_cast<uint8_t*>(pScene->mTextures[index]->pcData),
												   pScene->mTextures[index]->mWidth,
												   textureType);
					materialTextures.push_back(embeddedIndexedTexture);
					break;
				}
			}
		}
	}

	if (materialTextures.size() == 0)
		materialTextures.emplace_back(Texture(this->device, Colors::UnhandledTextureColor, aiTextureType::aiTextureType_DIFFUSE));

	return materialTextures;
}


TextureStorageType Model::DetermineTextureStorageType(const aiScene* pScene, aiMaterial* pMat, unsigned int index, aiTextureType textureType)
{
	if (pMat->GetTextureCount(textureType) == 0)
		return TextureStorageType::None;

	aiString path;
	pMat->GetTexture(textureType, index, &path);
	std::string texturePath = path.C_Str();
	if (texturePath[0] == '*')
	{
		if (pScene->mTextures[0]->mHeight == 0)
		{
			return TextureStorageType::EmbeddedIndexCompressed;
		}
		else
		{
			assert("SUPPORT DOES NOT EXIST YET FOR INDEXED NON COMPRESSED TEXTURES!" && 0);
			return TextureStorageType::EmbeddedIndexNonCompressed;
		}
	}
	
	if (auto pTex = pScene->GetEmbeddedTexture(texturePath.c_str()))
	{
		if (pTex->mHeight == 0)
		{
			return TextureStorageType::EmbeddedCompressed;
		}
		else
		{
			assert("SUPPORT DOES NOT EXIST YET FOR EMBEDDED NON COMPRESSED TEXTURES!" && 0);
			return TextureStorageType::EmbeddedNonCompressed;
		}
	}

	if (texturePath.find('.') != std::string::npos)
	{
		return TextureStorageType::Disk;
	}

	return TextureStorageType::None;
}


int Model::GetTextureIndex(aiString* pStr)
{
	assert(pStr->length >= 2);
	return atoi(&pStr->C_Str()[1]);
}

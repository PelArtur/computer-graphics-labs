#pragma once
#include <DirectXMath.h>

struct Vertex
{
	Vertex()
		: pos(0.0f, 0.0f, 0.0f), texCoord(0.0f, 0.0f), normal(0.0f, 0.0f, 0.0f) {}
	Vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) 
		: pos(x, y, z), texCoord(u, v), normal(nx, ny, nz) {}

	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;
};


struct FullscreenVertex 
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 texCoord;

	FullscreenVertex() : pos(0.0f, 0.0f, 0.0f), texCoord(0.0f, 0.0f) {}
	FullscreenVertex(float x, float y, float z, float u, float v) : pos(x, y, z), texCoord(u, v) {}
};

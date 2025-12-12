#pragma once
#include <vector>
#include <cmath>
#include <random>
#include <d3d11.h>
#include "Graphics/Vertex.hpp"
#define DB_PERLIN_IMPL
#include "db_perlin.hpp"
#include <DirectXMath.h>
#include <array>
#include <algorithm>
#include <wrl.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")
using namespace DirectX;


struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<DWORD> indices;
    std::vector<XMFLOAT3> colors;
    int width;
    int height;
};


struct HeightTextureLayer
{
    float minHeight;
    float maxHeight;
    XMFLOAT3 color;
    std::string textureName;
    float blendRange;
};


class Terrain
{
public:
    MeshData GenerateTerrain(int meshWidth, int meshDepth, float heightScale);
    float GetHeight(float x, float z,
        float scale = 10.0f,
        int octaves = 4,
        float persistence = 0.5f,
        float lacunarity = 2.0f);
    float GetTerrainHeight(
        float x, float z,
        float scale, int octaves,
        float persistence, float lacunarity,
        const std::vector<XMFLOAT2>& octaveOffsets);
    std::vector<XMFLOAT2> GenerateOffsets(int octaves, int seed, XMFLOAT2 offset);
    int GetCurveMode();
    void SetCurveMode(int mode);
    XMFLOAT3 GetColorForHeight(float normalizedHeight) const;


    float noiseScale = 10.0f;
    int octaves = 16;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    int seed = 12232;
    XMFLOAT2 noiseOffset = { 1.00f, 1.00f };
    int curve = 4;
    float power = 4.0f;

    std::vector<HeightTextureLayer> textureLayers = {
        {0.0f, 0.2f,  {0.0f, 0.2f, 0.6f}, "water", 0.1f},      // Deep water
        {0.2f, 0.25f, {0.0f, 0.4f, 0.8f}, "shallow_water", 0.05f}, // Shallow water
        {0.25f, 0.3f, {0.9f, 0.8f, 0.6f}, "sand", 0.1f},       // Sand/beach
        {0.3f, 0.5f,  {0.2f, 0.6f, 0.3f}, "grass", 0.15f},     // Grass
        {0.5f, 0.7f,  {0.4f, 0.5f, 0.3f}, "forest", 0.1f},     // Forest
        {0.7f, 0.85f, {0.6f, 0.6f, 0.6f}, "rock", 0.1f},       // Rock
        {0.85f, 1.0f, {1.0f, 1.0f, 1.0f}, "snow", 0.05f}       // Snow
    };

private:
    float cellSize = 0.2f;
};

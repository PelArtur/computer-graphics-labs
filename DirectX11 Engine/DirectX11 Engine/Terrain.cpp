#include "Terrain.hpp"


float Perlin(float x, float y)
{
	return db::perlin(x, y, 0.0f);
}

static float clamp(float value, float min, float max)
{
	if (value < min) return min;
	if (value > max) return max;
	return value;
}

float Exponential(float x, float base = 2.0f) { return (pow(base, x) - 1.0f) / (base - 1.0f); }
float Logarithmic(float x, float base = 2.0f) { return log(1.0f + (base - 1.0f) * x) / log(base); }
float Quadratic(float x) { return x * x; }
float Cubic(float x) { return x * x * x; }
float Power(float x, float power = 4.0f) { return pow(x, power); }


float Terrain::GetTerrainHeight(
	float x, float z,
	float scale, int octaves,
	float persistence, float lacunarity,
	const std::vector<XMFLOAT2>& octaveOffsets)
{
	if (scale <= 0.0f) scale = 0.0001f;

	float amplitude = 1.0f;
	float frequency = 1.0f;
	float noiseHeight = 0.0f;

	for (int i = 0; i < octaves; i++)
	{
		float sampleX = x / scale * frequency + octaveOffsets[i].x;
		float sampleZ = z / scale * frequency + octaveOffsets[i].y;

		float perlinValue = Perlin(sampleX, sampleZ) * 2.0f - 1.0f;
		noiseHeight += perlinValue * amplitude;

		amplitude *= persistence;
		frequency *= lacunarity;
	}

	return noiseHeight;
}


std::vector<XMFLOAT2> Terrain::GenerateOffsets(int octaves, int seed, XMFLOAT2 offset)
{
	std::vector<XMFLOAT2> offsets;
	offsets.resize(octaves);

	std::mt19937 rng(seed);
	std::uniform_real_distribution<float> dist(-100000.0f, 100000.0f);

	for (int i = 0; i < octaves; i++)
	{
		float ox = dist(rng) + offset.x;
		float oz = dist(rng) + offset.y;
		offsets[i] = XMFLOAT2(ox, oz);
	}
	return offsets;
}


XMFLOAT3 Terrain::GetColorForHeight(float normalizedHeight) const
{
	if (textureLayers.empty())
	{
		return XMFLOAT3(normalizedHeight, normalizedHeight, normalizedHeight);
	}

	XMFLOAT3 finalColor = { 0.0f, 0.0f, 0.0f };
	float totalWeight = 0.0f;

	for (const auto& layer : textureLayers)
	{
		float layerCenter = (layer.minHeight + layer.maxHeight) * 0.5f;
		float distance = fabs(normalizedHeight - layerCenter);
		float layerRange = (layer.maxHeight - layer.minHeight) * 0.5f;

		float weight = 1.0f - clamp(distance / (layerRange + layer.blendRange), 0.0f, 1.0f);

		weight = weight * weight * (3.0f - 2.0f * weight);

		if (weight > 0.0f)
		{
			finalColor.x += layer.color.x * weight;
			finalColor.y += layer.color.y * weight;
			finalColor.z += layer.color.z * weight;
			totalWeight += weight;
		}
	}

	if (totalWeight > 0.0f)
	{
		finalColor.x /= totalWeight;
		finalColor.y /= totalWeight;
		finalColor.z /= totalWeight;
	}

	return finalColor;
}


MeshData Terrain::GenerateTerrain(int meshWidth, int meshDepth, float heightScale)
{
	MeshData data;

	float halfWidth = (meshWidth - 1) * cellSize * 0.5f;
	float halfDepth = (meshDepth - 1) * cellSize * 0.5f;

	auto offsets = GenerateOffsets(octaves, seed, noiseOffset);

	data.vertices.reserve(meshWidth * meshDepth);
	data.colors.reserve(meshWidth * meshDepth);
	data.width = meshWidth;
	data.height = meshDepth;
	float maxY = 0.0f;
	float minY = 100.0f;

	for (int z = 0; z < meshDepth; z++)
	{
		for (int x = 0; x < meshWidth; x++)
		{
			float worldX = x * cellSize - halfWidth;
			float worldZ = z * cellSize - halfDepth;

			float height = GetTerrainHeight(
				worldX, worldZ,
				noiseScale,
				octaves,
				persistence,
				lacunarity,
				offsets
			);


			float u = (float)x / (meshWidth - 1.0f);
			float v = (float)z / (meshDepth - 1.0f);

			data.vertices.emplace_back(
				worldX, height, worldZ,
				u, v,
				0.0f, 1.0f, 0.0f
			);

			if (maxY < height)
				maxY = height;
			if (minY > height)
				minY = height;
		}
	}

	// Indices generation
	for (int z = 0; z < meshDepth - 1; ++z) 
	{
		for (int x = 0; x < meshWidth - 1; ++x) 
		{
			DWORD A = z * meshWidth + x;
			DWORD B = z * meshWidth + x + 1;
			DWORD D = (z + 1) * meshWidth + x;
			DWORD C = (z + 1) * meshWidth + x + 1;

			data.indices.push_back(A);
			data.indices.push_back(D);
			data.indices.push_back(C);

			data.indices.push_back(A);
			data.indices.push_back(C);
			data.indices.push_back(B);
		}
	}

	// Normals computation
	for (auto& vert : data.vertices)
		vert.normal.x = vert.normal.y = vert.normal.z = 0.0f;

	for (size_t i = 0; i < data.indices.size(); i += 3) 
	{
		DWORD i0 = data.indices[i + 0];
		DWORD i1 = data.indices[i + 1];
		DWORD i2 = data.indices[i + 2];

		Vertex& v0 = data.vertices[i0];
		Vertex& v1 = data.vertices[i1];
		Vertex& v2 = data.vertices[i2];

		// (v1-v0 and v2-v0)
		float v10x = v1.pos.x - v0.pos.x;
		float v10y = v1.pos.y - v0.pos.y;
		float v10z = v1.pos.z - v0.pos.z;

		float v20x = v2.pos.x - v0.pos.x;
		float v20y = v2.pos.y - v0.pos.y;
		float v20z = v2.pos.z - v0.pos.z;

		// Cross product
		float Nx = (v10y * v20z) - (v10z * v20y);
		float Ny = (v10z * v20x) - (v10x * v20z);
		float Nz = (v10x * v20y) - (v10y * v20x);

		v0.normal.x += Nx; v0.normal.y += Ny; v0.normal.z += Nz;
		v1.normal.x += Nx; v1.normal.y += Ny; v1.normal.z += Nz;
		v2.normal.x += Nx; v2.normal.y += Ny; v2.normal.z += Nz;
	}

	// Normalize all vertex normals
	float range = maxY - minY;
	for (auto& vert : data.vertices) 
	{
		float mag = std::sqrt(vert.normal.x * vert.normal.x + vert.normal.y * vert.normal.y + vert.normal.z * vert.normal.z);
		if (mag > 1e-6f)
		{
			vert.normal.x /= mag;
			vert.normal.y /= mag;
			vert.normal.z /= mag;
		}
		else
		{
			vert.normal.x = 0.0f;
			vert.normal.y = 1.0f;
			vert.normal.z = 0.0f;
		}

		float heightNorm = (vert.pos.y - minY) / range;
		XMFLOAT3 color = GetColorForHeight(heightNorm);
		data.colors.push_back(color);
		if (curve == 0)
			vert.pos.y = Exponential(heightNorm);
		else if (curve == 1)
			vert.pos.y = Logarithmic(heightNorm);
		else if (curve == 2)
			vert.pos.y = Quadratic(heightNorm);
		else if (curve == 3)
			vert.pos.y = Cubic(heightNorm);
		else if (curve == 4)
			vert.pos.y = Power(heightNorm, power);

		vert.pos.y = vert.pos.y * heightScale;
	}

	return data;
}


int Terrain::GetCurveMode()
{
	return curve;
}


void Terrain::SetCurveMode(int mode)
{
	this->curve = mode;
}

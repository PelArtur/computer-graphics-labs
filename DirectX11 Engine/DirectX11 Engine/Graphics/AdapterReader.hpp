#pragma once

#include "../ErrorLogger.hpp"
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>


class AdapterData
{
public:
	AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter);
	Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
	DXGI_ADAPTER_DESC description;
};

class AdapterReader
{
public:
	static std::vector<AdapterData> GetAdapters();

private:
	static std::vector<AdapterData> adapters;
};
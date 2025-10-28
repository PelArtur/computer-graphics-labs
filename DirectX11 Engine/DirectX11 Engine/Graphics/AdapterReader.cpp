#include "AdapterReader.hpp"

std::vector<AdapterData> AdapterReader::adapters;


AdapterData::AdapterData(Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter)
{
	this->pAdapter = pAdapter;
	HRESULT hr = pAdapter->GetDesc(&this->description);
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to Get Description for IDXGIAdapter.");
	}
}


std::vector<AdapterData> AdapterReader::GetAdapters()
{
	if (adapters.size() > 0)
		return adapters;

	Microsoft::WRL::ComPtr<IDXGIFactory> pFactory;

	HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(pFactory.GetAddressOf()));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create DXGIFactory for enumerating adapters.");
		exit(-1);
	}

	Microsoft::WRL::ComPtr<IDXGIAdapter> pAdapter;
	UINT index = 0;
	while (SUCCEEDED(pFactory->EnumAdapters(index, &pAdapter)))
	{
		adapters.push_back(AdapterData(pAdapter));
		index += 1;
	}

	std::sort(adapters.begin(), adapters.end(), [](const AdapterData& a, const AdapterData& b)
	{
		return a.description.DedicatedVideoMemory > b.description.DedicatedVideoMemory;
	});
	return adapters;
}

#include "Graphics.hpp"

#include <sstream>
#include <string>
#include <windows.h>

bool Graphics::Initialize(HWND hwnd, int width, int height)
{
	this->windowWidth = width;
	this->windowHeight = width;
	this->fpsTimer.Start();
	this->shadersTimer.Start();

	if (!InitializeDirectX(hwnd))
		return false;
	if (!InitializeShaders())
		return false;
	if (!InitializeScene())
		return false;
	return true;
}


bool Graphics::InitializeDirectX(HWND hwnd)
{
	std::vector<AdapterData> adapters = AdapterReader::GetAdapters();

	if (adapters.empty())
	{
		ErrorLogger::Log("No IDXFI Adapters found.");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC scd;
	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

	scd.BufferDesc.Width = this->windowWidth;
	scd.BufferDesc.Height = this->windowHeight;
	scd.BufferDesc.RefreshRate.Numerator = 60;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;

	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 1;
	scd.OutputWindow = hwnd;
	scd.Windowed = TRUE;
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hr;
	hr = D3D11CreateDeviceAndSwapChain(
		adapters[0].pAdapter.Get(),			 //IDXGI Adapter, Get() give raw pointer from ComPtr
		D3D_DRIVER_TYPE_UNKNOWN,			 //For un                    specified adapter
		NULL,								 //FOR SOFTWARE DRIVER TYPE
		NULL,								 //FLAGS FOR RUNTIME LAYERS
		NULL,								 //FEATURE LEVELS ARRAY
		0,									 //# OF FEATURE LEVELS IN ARRAY
		D3D11_SDK_VERSION,
		&scd,                                //Swapchain description
		this->swapchain.GetAddressOf(),		 //Swapchain Address
		this->device.GetAddressOf(),		 //Device Address
		NULL,								 //Supported feature level
		this->deviceContext.GetAddressOf()); //Device Context Address

	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create device and swapchain.");
		return false;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
	hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "GetBuffer Failed.");
		return false;
	}

	hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create render target view.");
		return false;
	}

	//Describe our Depth/Stencil Buffer
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = this->windowWidth;
	depthStencilDesc.Height = this->windowHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	hr = this->device->CreateTexture2D(&depthStencilDesc, NULL, this->depthStencilBuffer.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil buffer.");
		return false;
	}

	hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil view.");
		return false;
	}

	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());

	//Create depth stencil state
	D3D11_DEPTH_STENCIL_DESC depthstencildesc;
	ZeroMemory(&depthstencildesc, sizeof(D3D11_DEPTH_STENCIL_DESC));

	depthstencildesc.DepthEnable = true;
	depthstencildesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL;
	depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

	hr = this->device->CreateDepthStencilState(&depthstencildesc, this->depthStencilState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create depth stencil state.");
		return false;
	}

	//Create the Viewport
	D3D11_VIEWPORT viewport;
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = this->windowWidth;
	viewport.Height = this->windowHeight;
	//z-depth
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	//Set the Viewport
	this->deviceContext->RSSetViewports(1, &viewport);

	D3D11_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D11_RASTERIZER_DESC));

	rasterizerDesc.FillMode = D3D11_FILL_MODE::D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_MODE::D3D11_CULL_BACK;
	hr = this->device->CreateRasterizerState(&rasterizerDesc, this->rasterizerState.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create rasterizer state.");
		return false;
	}

	spriteBatch = std::make_unique<DirectX::SpriteBatch>(this->deviceContext.Get());
	spriteFont = std::make_unique<DirectX::SpriteFont>(this->device.Get(), L"Data/Fonts/comic_sans_ms_16");

	//Sample descritption
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory(&sampDesc, sizeof(sampDesc));
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = this->device->CreateSamplerState(&sampDesc, this->samplerState.GetAddressOf()); //Create sampler state
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create sampler state.");
		return false;
	}

	return true;
}


std::wstring GetExecutablePath()
{
	wchar_t path[MAX_PATH];

	if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0)
	{
		ErrorLogger::Log("Failed to get executable path.");
		exit(-1);
	}

	return { path };
}


std::wstring GetExecutableFolder()
{
	const std::wstring exePath = GetExecutablePath();
	const size_t pos = exePath.find_last_of(L"\\/");

	if (pos != std::wstring::npos)
		return exePath.substr(0, pos + 1);

	ErrorLogger::Log("Failed to get executable folder.");
	exit(-1);
}


bool Graphics::InitializeShaders()
{
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT numElements = ARRAYSIZE(layout);

	if (!vertexShader.Initialize(device, GetExecutableFolder() + L"vertexShader.cso", layout, numElements))
		return false;
	if (!pixelShader.Initialize(device, GetExecutableFolder() + L"pixelShader.cso"))
		return false;
	if (!voronoiseShader.Initialize(device, GetExecutableFolder() + L"voronoisePixelShader.cso"))
		return false;
	if (!warpShader.Initialize(device, GetExecutableFolder() + L"WarpEffectPS.cso"))
		return false;
	return true;
}


bool Graphics::InitializeScene()
{
	Vertex v[] =
	{
		//Front
		Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex( 0.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU
		Vertex( 0.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//Right
		Vertex( 0.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex( 0.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex( 0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex( 0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		//Back
		Vertex( 0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex( 0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(-0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		//Left
		Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-0.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		Vertex(-0.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		//Up
		Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex( 0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex( 0.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//DOWN
		Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex( 0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex( 0.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU


		// --- CUBE 2 (New Cube: Indices 24-47) ---
		// Offset Cube 2 by 2.0 units on the X-axis.

		// Front (Indices 24, 25, 26, 27)
		Vertex(1.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD 
		Vertex(1.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(2.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU
		Vertex(2.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		// Right (Indices 28, 29, 30, 31)
		Vertex(2.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(2.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		// Back (Indices 32, 33, 34, 35)
		Vertex(2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		// Left (Indices 36, 37, 38, 39)
		Vertex(1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(1.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		Vertex(1.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		// Up (Indices 40, 41, 42, 43)
		Vertex(1.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(2.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		// DOWN (Indices 44, 45, 46, 47)
		Vertex(1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(1.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(2.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU


		// --- CUBE 3 (New Cube: Indices 48-71) ---
		// Offset Cube 3 by -2.0 units on the X-axis.

		// Front (Indices 48, 49, 50, 51)
		Vertex(-2.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD (-0.5 - 2.0 = -2.5)
		Vertex(-2.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(-1.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU ( 0.5 - 2.0 = -1.5)
		Vertex(-1.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		// Right (Indices 52, 53, 54, 55)
		Vertex(-1.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(-1.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(-1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(-1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		// Back (Indices 56, 57, 58, 59)
		Vertex(-1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(-2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		// Left (Indices 60, 61, 62, 63)
		Vertex(-2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-2.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		Vertex(-2.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		// Up (Indices 64, 65, 66, 67)
		Vertex(-2.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(-2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(-1.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		// DOWN (Indices 68, 69, 70, 71)
		Vertex(-2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-2.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(-1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(-1.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU

		// --- CUBE 4 (New Cube: Indices 72-95) ---
		// Offset Cube 4 by +2.0 units on the Y-axis.

		// Front (Indices 72, 73, 74, 75)
		Vertex(-0.5f, 1.5f, 0.0f, 0.0f, 1.0f),    //LD (-0.5 + 2.0 = 1.5)
		Vertex(-0.5f, 2.5f, 0.0f, 0.0f, 0.0f),    //LU ( 0.5 + 2.0 = 2.5)
		Vertex(0.5f, 2.5f, 0.0f, 1.0f, 0.0f),    //RU
		Vertex(0.5f, 1.5f, 0.0f, 1.0f, 1.0f),    //RD

		// Right (Indices 76, 77, 78, 79)
		Vertex(0.5f, 1.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(0.5f, 2.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD

		// Back (Indices 80, 81, 82, 83)
		Vertex(0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(-0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU

		// Left (Indices 84, 85, 86, 87)
		Vertex(-0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(-0.5f, 1.5f, 0.0f, 1.0f, 1.0f),    //RD
		Vertex(-0.5f, 2.5f, 0.0f, 1.0f, 0.0f),    //RU

		// Up (Indices 88, 89, 90, 91)
		Vertex(-0.5f, 2.5f, 0.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		Vertex(0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU
		Vertex(0.5f, 2.5f, 0.0f, 1.0f, 1.0f),    //RD

		// DOWN (Indices 92, 93, 94, 95)
		Vertex(-0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		Vertex(-0.5f, 1.5f, 0.0f, 0.0f, 0.0f),    //LU
		Vertex(0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD
		Vertex(0.5f, 1.5f, 0.0f, 1.0f, 0.0f),    //RU
	};

	DWORD indices[] =
	{
		0, 1, 2,     //Front1
		4, 5, 6,     //Right1
		9, 11, 10,   //Back2
		13, 15, 14,  //Left2
		16, 17, 18,  //Up1
		21, 23, 22,  //Bottom2

		0, 2, 3,     //Front2
		4, 6, 7,     //Right2
		8, 9, 10,    //Back1
		12, 13, 14,  //Left1
		16, 18, 19,  //Up2
		20, 21, 22,  //Bottom1

		//Front (24, 25, 26, 27)
	    24, 25, 26,  
		24, 26, 27,
		//Right (28, 29, 30, 31)
		28, 29, 30,
		28, 30, 31,
		//Back (32, 33, 34, 35)
		32, 33, 34,
		33, 35, 34,
		//Left (36, 37, 38, 39)
		36, 37, 38,
		37, 39, 38,
		//Up (40, 41, 42, 43)
		40, 41, 42,
		40, 42, 43,
		//Bottom (44, 45, 46, 47)
		44, 45, 46, 
		45, 47, 46,

		//Front (48, 49, 50, 51)
		48, 49, 50,
		48, 50, 51,
		//Right1 (52, 53, 54, 55)
		52, 53, 54,
		52, 54, 55,
		//Back (56, 57, 58, 59)
		56, 57, 58,
		57, 59, 58,
		//Left (60, 61, 62, 63)
		60, 61, 62,
		61, 63, 62,
		//Up (64, 65, 66, 67)
		64, 65, 66,
		64, 66, 67,
		//Bottom (68, 69, 70, 71)
		68, 69, 70,
		69, 71, 70,

		// Front (72, 73, 74, 75)
		72, 73, 74,
		72, 74, 75,
		// Right (76, 77, 78, 79)
		76, 77, 78,
		76, 78, 79,
		// Back (80, 81, 82, 83)
		80, 81, 82,
		81, 83, 82,
		// Left (84, 85, 86, 87)
		84, 85, 86,
		85, 87, 86,
		// Up (88, 89, 90, 91)
		88, 89, 90,
		88, 90, 91,
		// Bottom (92, 93, 94, 95)
		92, 93, 94,
		93, 95, 94,
	};

	//Vertex buffer
	HRESULT hr = this->vertexBuffer.Initialize(this->device.Get(), v, ARRAYSIZE(v));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create vertex buffer.");
		return false;
	}

	//Index buffer
	hr = this->indicesBuffer.Initialize(this->device.Get(), indices, ARRAYSIZE(indices));
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create indices buffer.");
		return false;
	}

	//Texture
	hr = DirectX::CreateWICTextureFromFile(this->device.Get(), L"Data/Textures/profile1.jpg", nullptr, myTexture.GetAddressOf());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to create wic texture from file.");
		return false;
	}

	//Constant buffers
	hr = this->constantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to initialize constant buffer.");
		return false;
	}

	hr = this->psConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to initialize pixel shader constant buffer.");
		return false;
	}

	hr = this->warpConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
	if (FAILED(hr))
	{
		ErrorLogger::Log(hr, "Failed to initialize pixel shader constant buffer.");
		return false;
	}

	this->psConstantBuffer.data.iResolution = {
		static_cast<float>(windowWidth),
		static_cast<float>(windowHeight),
		1.0f / static_cast<float>(windowWidth),
		1.0f / static_cast<float>(windowHeight)
	};

	this->warpConstantBuffer.data.iResolution = {
		static_cast<float>(windowWidth),
		static_cast<float>(windowHeight),
		1.0f / static_cast<float>(windowWidth),
		1.0f / static_cast<float>(windowHeight)
	};

	camera.SetPosition(0.0f, 0.0f, -2.0f);
	camera.SetProjectionValues(90.0f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 1000.0f);
	return true;
}


void Graphics::RenderFrame()
{
	//float backgroundColor[] = { 0.424f, 0.839f, 0.71f, 1.0f };
	float backgroundColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), backgroundColor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	this->deviceContext->IASetInputLayout(this->vertexShader.GetInputLayout());
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	//this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);

	UINT offset = 0;

	//Camera
	XMMATRIX worldMatrix = DirectX::XMMatrixIdentity();
	XMMATRIX viewMatrixXM = camera.GetViewMatrix();
	XMMATRIX projectionMatrixXM = camera.GetProjectionMatrix();
	DirectX::XMMATRIX wvp = worldMatrix * camera.GetViewMatrix() * camera.GetProjectionMatrix();
	DirectX::XMStoreFloat4x4(&constantBuffer.data.mat, wvp);

	//Update Constant Buffer
	if (!constantBuffer.ApplyChanges())
		return;

	//this->deviceContext->VSSetConstantBuffers(0, 1, this->constantBuffer.GetAddressOf());
	//this->deviceContext->PSSetShaderResources(0, 1, this->myTexture.GetAddressOf());
	//this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset);
	//this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	//this->deviceContext->DrawIndexed(indicesBuffer.BufferSize(), 0, 0);

	this->psConstantBuffer.data.iTime = (float)this->shadersTimer.GetMilisecondsElapsed() / 1000.0f;
	this->psConstantBuffer.data.iMouse = this->mouseData;
	if (!psConstantBuffer.ApplyChanges())
		return;

	this->warpConstantBuffer.data.iTime = (float)this->shadersTimer.GetMilisecondsElapsed() / 1000.0f;
	if (!warpConstantBuffer.ApplyChanges())
		return;

	this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset);
	this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	this->deviceContext->VSSetConstantBuffers(0, 1, this->constantBuffer.GetAddressOf());

	UINT boxVertices = indicesBuffer.BufferSize() / 4;

	//PixelShader1
	this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShaderResources(0, 1, this->myTexture.GetAddressOf());
	this->deviceContext->DrawIndexed(boxVertices, indicesBuffer.BufferSize() / 2 + boxVertices, 0);

	//PixelShader2
	this->deviceContext->PSSetShader(voronoiseShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetConstantBuffers(1, 1, this->psConstantBuffer.GetAddressOf());
	this->deviceContext->DrawIndexed(boxVertices, 0, 0);                                //first box
	this->deviceContext->DrawIndexed(boxVertices, indicesBuffer.BufferSize() / 8, 0);  //first box
	this->deviceContext->DrawIndexed(boxVertices, indicesBuffer.BufferSize() / 2, 0);  //third box

	//PixelShader3
	this->deviceContext->PSSetShader(warpShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetConstantBuffers(2, 1, this->psConstantBuffer.GetAddressOf());
	this->deviceContext->DrawIndexed(boxVertices, indicesBuffer.BufferSize() / 8, 0);  //first box
	this->deviceContext->DrawIndexed(boxVertices, indicesBuffer.BufferSize() / 4, 0);   //second box

	//FPS counter
	static int fpsCounter = 0;
	static std::string fpsString = "FPS: 0";
	fpsCounter += 1;
	if (fpsTimer.GetMilisecondsElapsed() > 1000.0)
	{
		fpsString = "FPS: " + std::to_string(fpsCounter);
		fpsCounter = 0;
		fpsTimer.Restart();
	}
	spriteBatch->Begin();
	spriteFont->DrawString(spriteBatch.get(), StringConverter::StringToWide(fpsString).c_str(), DirectX::XMFLOAT2(0, 0), DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	spriteBatch->End();

	this->swapchain->Present(0, NULL); //VSYNC ON -- 1, OFF -- 0
}

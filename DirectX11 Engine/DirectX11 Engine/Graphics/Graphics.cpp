#include "Graphics.hpp"

#include <sstream>
#include <string>
#include <windows.h>

bool Graphics::Initialize(HWND hwnd, int width, int height) 
{
	this->windowWidth = width;
	this->windowHeight = height;
	this->fpsTimer.Start();
	this->shadersTimer.Start();

	if (!InitializeDirectX(hwnd))
		return false;
	if (!InitializeShaders())
		return false;
	if (!InitializeScene())
		return false;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(this->device.Get(), this->deviceContext.Get());
	ImGui::StyleColorsDark();

	return true;
}


bool Graphics::InitializeDirectX(HWND hwnd)
{
	try
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
		COM_ERROR_IF_FAILED(hr, "Failed to create device and swapchain.");

		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		hr = this->swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		COM_ERROR_IF_FAILED(hr, "GetBuffer Failed.");

		hr = this->device->CreateRenderTargetView(backBuffer.Get(), NULL, this->renderTargetView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create render target view.");

		//Describe our Depth/Stencil Buffer
		CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, this->windowWidth, this->windowHeight);
		depthStencilTextureDesc.MipLevels = 1;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		hr = this->device->CreateTexture2D(&depthStencilTextureDesc, NULL, this->depthStencilBuffer.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil buffer.");

		hr = this->device->CreateDepthStencilView(this->depthStencilBuffer.Get(), NULL, this->depthStencilView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil view.");

		this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), this->depthStencilView.Get());

		//Create depth stencil state
		CD3D11_DEPTH_STENCIL_DESC depthstencildesc(D3D11_DEFAULT);
		depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

		hr = this->device->CreateDepthStencilState(&depthstencildesc, this->depthStencilState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil state.");

		//Create and set the Viewport
		CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(this->windowWidth), static_cast<float>(this->windowHeight));;
		this->deviceContext->RSSetViewports(1, &viewport);

		CD3D11_RASTERIZER_DESC rasterizerDesc(D3D11_DEFAULT);
		hr = this->device->CreateRasterizerState(&rasterizerDesc, this->rasterizerState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create rasterizer state.");

		//Create Blend State
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));

		D3D11_RENDER_TARGET_BLEND_DESC rtbd;
		ZeroMemory(&rtbd, sizeof(rtbd));

		rtbd.BlendEnable = true;
		rtbd.SrcBlend = D3D11_BLEND::D3D11_BLEND_SRC_ALPHA;
		rtbd.DestBlend = D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA;
		rtbd.BlendOp = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		rtbd.SrcBlendAlpha = D3D11_BLEND::D3D11_BLEND_ONE;
		rtbd.DestBlendAlpha = D3D11_BLEND::D3D11_BLEND_ZERO;
		rtbd.BlendOpAlpha = D3D11_BLEND_OP::D3D11_BLEND_OP_ADD;
		rtbd.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;

		blendDesc.RenderTarget[0] = rtbd;

		hr = this->device->CreateBlendState(&blendDesc, this->blendState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create blend state.");

		spriteBatch = std::make_unique<DirectX::SpriteBatch>(this->deviceContext.Get());
		spriteFont = std::make_unique<DirectX::SpriteFont>(this->device.Get(), L"Data/Fonts/comic_sans_ms_16");

		//Sample descritption
		CD3D11_SAMPLER_DESC sampDesc(D3D11_DEFAULT);
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		hr = this->device->CreateSamplerState(&sampDesc, this->samplerState.GetAddressOf()); //Create sampler state
		COM_ERROR_IF_FAILED(hr, "Failed to create sampler state.");
	}
	catch (COMException& exception)
	{
		ErrorLogger::Log(exception);
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
	try
	{
		{
		//	Vertex v[] = {
		//      //Front
		//		Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		//Right
		//		Vertex(0.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(0.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		//		//Back
		//		Vertex(0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		//		//Left
		//		Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-0.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-0.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		//Up
		//		Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		//DOWN
		//		Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(0.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU


		//		// --- CUBE 2 (New Cube: Indices 24-47) ---
		//		// Offset Cube 2 by 2.0 units on the X-axis.

		//		// Front (Indices 24, 25, 26, 27)
		//		Vertex(1.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD 
		//		Vertex(1.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(2.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU
		//		Vertex(2.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// Right (Indices 28, 29, 30, 31)
		//		Vertex(2.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(2.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		//		// Back (Indices 32, 33, 34, 35)
		//		Vertex(2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		//		// Left (Indices 36, 37, 38, 39)
		//		Vertex(1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(1.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		//		Vertex(1.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		// Up (Indices 40, 41, 42, 43)
		//		Vertex(1.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(2.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// DOWN (Indices 44, 45, 46, 47)
		//		Vertex(1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(1.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(2.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU


		//		// --- CUBE 3 (New Cube: Indices 48-71) ---
		//		// Offset Cube 3 by -2.0 units on the X-axis.

		//		// Front (Indices 48, 49, 50, 51)
		//		Vertex(-2.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD (-0.5 - 2.0 = -2.5)
		//		Vertex(-2.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-1.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU ( 0.5 - 2.0 = -1.5)
		//		Vertex(-1.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// Right (Indices 52, 53, 54, 55)
		//		Vertex(-1.5f, -0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-1.5f,  0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(-1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD

		//		// Back (Indices 56, 57, 58, 59)
		//		Vertex(-1.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-1.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-2.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-2.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU

		//		// Left (Indices 60, 61, 62, 63)
		//		Vertex(-2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-2.5f, -0.5f, 0.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-2.5f,  0.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		// Up (Indices 64, 65, 66, 67)
		//		Vertex(-2.5f,  0.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-2.5f,  0.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-1.5f,  0.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(-1.5f,  0.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// DOWN (Indices 68, 69, 70, 71)
		//		Vertex(-2.5f, -0.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-2.5f, -0.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-1.5f, -0.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-1.5f, -0.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		// --- CUBE 4 (New Cube: Indices 72-95) ---
		//		// Offset Cube 4 by +2.0 units on the Y-axis.

		//		// Front (Indices 72, 73, 74, 75)
		//		Vertex(-0.5f, 1.5f, 0.0f, 0.0f, 1.0f),    //LD (-0.5 + 2.0 = 1.5)
		//		Vertex(-0.5f, 2.5f, 0.0f, 0.0f, 0.0f),    //LU ( 0.5 + 2.0 = 2.5)
		//		Vertex(0.5f, 2.5f, 0.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f, 1.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// Right (Indices 76, 77, 78, 79)
		//		Vertex(0.5f, 1.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(0.5f, 2.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD

		//		// Back (Indices 80, 81, 82, 83)
		//		Vertex(0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU

		//		// Left (Indices 84, 85, 86, 87)
		//		Vertex(-0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(-0.5f, 1.5f, 0.0f, 1.0f, 1.0f),    //RD
		//		Vertex(-0.5f, 2.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		// Up (Indices 88, 89, 90, 91)
		//		Vertex(-0.5f, 2.5f, 0.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f, 2.5f, 1.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f, 2.5f, 1.0f, 1.0f, 0.0f),    //RU
		//		Vertex(0.5f, 2.5f, 0.0f, 1.0f, 1.0f),    //RD

		//		// DOWN (Indices 92, 93, 94, 95)
		//		Vertex(-0.5f, 1.5f, 1.0f, 0.0f, 1.0f),    //LD
		//		Vertex(-0.5f, 1.5f, 0.0f, 0.0f, 0.0f),    //LU
		//		Vertex(0.5f, 1.5f, 1.0f, 1.0f, 1.0f),    //RD
		//		Vertex(0.5f, 1.5f, 0.0f, 1.0f, 0.0f),    //RU

		//		// --- CUBE 5 (New Cube: Indices 96-119) ---
		//		// Offset Cube 4 by +2.0 units on the Y-axis.

		//		// Front 
		//		Vertex(-5.0f, -5.0f, 5.0f, 0.0f, 1.0f),   //LD
		//		Vertex(-5.0f, 5.0f, 5.0f, 0.0f, 0.0f),   //LU
		//		Vertex(5.0f, 5.0f, 5.0f, 1.0f, 0.0f),   //RU
		//		Vertex(5.0f, -5.0f, 5.0f, 1.0f, 1.0f),   //RD

		//		// Right
		//		Vertex(5.0f, -5.0f, 5.0f, 0.0f, 1.0f),   //LD
		//		Vertex(5.0f, 5.0f, 5.0f, 0.0f, 0.0f),   //LU
		//		Vertex(5.0f, 5.0f, 15.0f, 1.0f, 0.0f),  //RU
		//		Vertex(5.0f, -5.0f, 15.0f, 1.0f, 1.0f),  //RD

		//		// Back
		//		Vertex(5.0f, -5.0f, 15.0f, 0.0f, 1.0f),  //LD
		//		Vertex(5.0f, 5.0f, 15.0f, 0.0f, 0.0f),  //LU
		//		Vertex(-5.0f, -5.0f, 15.0f, 1.0f, 1.0f),  //RD
		//		Vertex(-5.0f, 5.0f, 15.0f, 1.0f, 0.0f),  //RU

		//		// Left
		//		Vertex(-5.0f, -5.0f, 15.0f, 0.0f, 1.0f),  //LD
		//		Vertex(-5.0f, 5.0f, 15.0f, 0.0f, 0.0f),  //LU
		//		Vertex(-5.0f, -5.0f, 5.0f, 1.0f, 1.0f),   //RD
		//		Vertex(-5.0f, 5.0f, 5.0f, 1.0f, 0.0f),   //RU

		//		// Up
		//		Vertex(-5.0f, 5.0f, 5.0f, 0.0f, 1.0f),   //LD
		//		Vertex(-5.0f, 5.0f, 15.0f, 0.0f, 0.0f),  //LU
		//		Vertex(5.0f, 5.0f, 15.0f, 1.0f, 0.0f),  //RU
		//		Vertex(5.0f, 5.0f, 5.0f, 1.0f, 1.0f),   //RD

		//		// Down
		//		Vertex(-5.0f, -5.0f, 15.0f, 0.0f, 1.0f),  //LD
		//		Vertex(-5.0f, -5.0f, 5.0f, 0.0f, 0.0f),   //LU
		//		Vertex(5.0f, -5.0f, 15.0f, 1.0f, 1.0f),  //RD
		//		Vertex(5.0f, -5.0f, 5.0f, 1.0f, 0.0f),   //RU
		//	};

		//	DWORD indices[] =
		//	{
		//		0, 1, 2,     //Front1
		//		4, 5, 6,     //Right1
		//		9, 11, 10,   //Back2
		//		13, 15, 14,  //Left2
		//		16, 17, 18,  //Up1
		//		21, 23, 22,  //Bottom2

		//		0, 2, 3,     //Front2
		//		4, 6, 7,     //Right2
		//		8, 9, 10,    //Back1
		//		12, 13, 14,  //Left1
		//		16, 18, 19,  //Up2
		//		20, 21, 22,  //Bottom1

		//		//Front (24, 25, 26, 27)
		//		24, 25, 26,
		//		24, 26, 27,
		//		//Right (28, 29, 30, 31)
		//		28, 29, 30,
		//		28, 30, 31,
		//		//Back (32, 33, 34, 35)
		//		32, 33, 34,
		//		33, 35, 34,
		//		//Left (36, 37, 38, 39)
		//		36, 37, 38,
		//		37, 39, 38,
		//		//Up (40, 41, 42, 43)
		//		40, 41, 42,
		//		40, 42, 43,
		//		//Bottom (44, 45, 46, 47)
		//		44, 45, 46,
		//		45, 47, 46,

		//		//Front (48, 49, 50, 51)
		//		48, 49, 50,
		//		48, 50, 51,
		//		//Right1 (52, 53, 54, 55)
		//		52, 53, 54,
		//		52, 54, 55,
		//		//Back (56, 57, 58, 59)
		//		56, 57, 58,
		//		57, 59, 58,
		//		//Left (60, 61, 62, 63)
		//		60, 61, 62,
		//		61, 63, 62,
		//		//Up (64, 65, 66, 67)
		//		64, 65, 66,
		//		64, 66, 67,
		//		//Bottom (68, 69, 70, 71)
		//		68, 69, 70,
		//		69, 71, 70,

		//		// Front (72, 73, 74, 75)
		//		72, 73, 74,
		//		72, 74, 75,
		//		// Right (76, 77, 78, 79)
		//		76, 77, 78,
		//		76, 78, 79,
		//		// Back (80, 81, 82, 83)
		//		80, 81, 82,
		//		81, 83, 82,
		//		// Left (84, 85, 86, 87)
		//		84, 85, 86,
		//		85, 87, 86,
		//		// Up (88, 89, 90, 91)
		//		88, 89, 90,
		//		88, 90, 91,
		//		// Bottom (92, 93, 94, 95)
		//		92, 93, 94,
		//		93, 95, 94,

		//		// Front (96, 97, 98, 99)
		//		96, 97, 98,
		//		96, 98, 99,
		//		// Right (100, 101, 102, 103)
		//		100, 101, 102,
		//		100, 102, 103,
		//		// Back (104, 105, 106, 107)
		//		104, 105, 106,
		//		105, 107, 106,
		//		// Left (108, 109, 110, 111)
		//		108, 109, 110,
		//		109, 111, 110,
		//		// Up (112, 113, 114, 115)
		//		112, 113, 114,
		//		112, 114, 115,
		//		// Bottom (116, 117, 118, 119)
		//		116, 117, 118,
		//		117, 119, 118,
		//	};

		//	//Vertex buffer
		//	HRESULT hr = this->vertexBuffer.Initialize(this->device.Get(), v, ARRAYSIZE(v));
		//	COM_ERROR_IF_FAILED(hr, "Failed to create vertex buffer.");

		//	//Index buffer
		//	hr = this->indicesBuffer.Initialize(this->device.Get(), indices, ARRAYSIZE(indices));
		//	COM_ERROR_IF_FAILED(hr, "Failed to create indices buffer.");
		}

		//Texture
		HRESULT hr = DirectX::CreateWICTextureFromFile(this->device.Get(), L"Data/Textures/profile1.jpg", nullptr, myTexture.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create wic texture from file.");

		hr = DirectX::CreateWICTextureFromFile(this->device.Get(), L"Data/Textures/profile2.jpg", nullptr, myTexture2.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create wic texture from file.");

		//Constant buffers
		hr = this->cb_vertexShader.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_pixelShader.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->psConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize voronoise pixel shader constant buffer.");

		hr = this->warpConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize warp pixel shader constant buffer.");

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

		//Initialize Model
		//if (!gameObject.Initialize("Data/Models/roblox_guy/source/RBLXTriGuard.fbx", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
		//	return false;
		if (!gameObject.Initialize("Data/Models/sentinel/rq170.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;

		camera.SetPosition(0.0f, 0.0f, -2.0f);
		camera.SetProjectionValues(90.0f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 1000.0f);
	}
	catch (COMException &exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}
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
	//this->deviceContext->OMSetBlendState(this->blendState.Get(), NULL, 0xFFFFFFFF);
	this->deviceContext->OMSetBlendState(NULL, NULL, 0xFFFFFFFF);
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);

	UINT offset = 0;

	//static float translationOffset1[3] = { 0, 0, 0 };
	//static float alpha1 = 1.0f;
	////Camera
	//XMMATRIX worldMatrix = XMMatrixTranslation(translationOffset1[0], translationOffset1[1], translationOffset1[2]);
	//XMMATRIX viewMatrixXM = camera.GetViewMatrix();
	//XMMATRIX projectionMatrixXM = camera.GetProjectionMatrix();
	//DirectX::XMMATRIX wvp = worldMatrix * camera.GetViewMatrix() * camera.GetProjectionMatrix();
	//cb_vertexShader.data.mat = wvp;

	////Update Constant Buffer
	//if (!cb_vertexShader.ApplyChanges())
	//	return;

	//this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vertexShader.GetAddressOf());
	//this->deviceContext->PSSetShaderResources(0, 1, this->myTexture.GetAddressOf());
	//this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset);
	//this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	//this->deviceContext->DrawIndexed(indicesBuffer.BufferSize(), 0, 0);

	//this->psConstantBuffer.data.iTime = (float)this->shadersTimer.GetMilisecondsElapsed() / 1000.0f;
	//this->psConstantBuffer.data.iMouse = this->mouseData;
	//if (!psConstantBuffer.ApplyChanges())
	//	return;

	//this->warpConstantBuffer.data.iTime = (float)this->shadersTimer.GetMilisecondsElapsed() / 1000.0f;
	//if (!warpConstantBuffer.ApplyChanges())
	//	return;

	//this->cb_pixelShader.data.alpha = alpha1;
	//if (!cb_pixelShader.ApplyChanges())
	//	return;

	//this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset);
	//this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	//this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vertexShader.GetAddressOf());
	//this->deviceContext->PSSetConstantBuffers(0, 1, this->cb_pixelShader.GetAddressOf());

	//UINT indexBufferSize4 = indicesBuffer.BufferSize() / 5 * 4;
	//UINT boxVertices = indexBufferSize4 / 4;

	////PixelShader1
	//this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);
	//this->deviceContext->PSSetShaderResources(0, 1, this->myTexture.GetAddressOf());
	//this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4 / 2 + boxVertices, 0);

	//////PixelShader2
	//this->deviceContext->PSSetShader(voronoiseShader.GetShader(), NULL, 0);
	//this->deviceContext->PSSetConstantBuffers(1, 1, this->psConstantBuffer.GetAddressOf());
	//this->deviceContext->DrawIndexed(boxVertices, 0, 0);                                //first box
	//this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4 / 8, 0);  //first box
	//this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4 / 2, 0);  //third box

	//////PixelShader3
	//this->deviceContext->PSSetShader(warpShader.GetShader(), NULL, 0);
	//this->deviceContext->PSSetConstantBuffers(2, 1, this->psConstantBuffer.GetAddressOf());
	//this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4 / 8, 0);  //first box
	//this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4 / 4, 0);   //second box

	static float translationOffset[3] = { 0, 0, 0 };
	static float rotationOffset[3] = { 0, 0, 0 };
	static float alpha = 1.0f;
	{ //back vox/
		/*worldMatrix = XMMatrixTranslation(translationOffset2[0], translationOffset2[1], translationOffset2[2]);
		viewMatrixXM = camera.GetViewMatrix();
		projectionMatrixXM = camera.GetProjectionMatrix();
		wvp = worldMatrix * camera.GetViewMatrix() * camera.GetProjectionMatrix();
		cb_vertexShader.data.mat = wvp;

		if (!cb_vertexShader.ApplyChanges())
			return;

		this->cb_pixelShader.data.alpha = alpha2;
		if (!cb_pixelShader.ApplyChanges())
			return;

		this->deviceContext->IASetVertexBuffers(0, 1, vertexBuffer.GetAddressOf(), vertexBuffer.StridePtr(), &offset);
		this->deviceContext->IASetIndexBuffer(indicesBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
		this->deviceContext->VSSetConstantBuffers(0, 1, this->cb_vertexShader.GetAddressOf());
		this->deviceContext->PSSetConstantBuffers(0, 1, this->cb_pixelShader.GetAddressOf());
		this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);
		this->deviceContext->PSSetShaderResources(0, 1, this->myTexture2.GetAddressOf());
		this->deviceContext->DrawIndexed(boxVertices, indexBufferSize4, 0);*/

		this->gameObject.SetPosition(translationOffset[0], translationOffset[1], translationOffset[2]);
		this->gameObject.SetRotation(rotationOffset[0], rotationOffset[1], rotationOffset[2]);
		this->gameObject.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
	}

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
	spriteFont->DrawString(spriteBatch.get(), StringHelper::StringToWide(fpsString).c_str(), DirectX::XMFLOAT2(0, 0), DirectX::Colors::White, 0.0f, DirectX::XMFLOAT2(0.0f, 0.0f), DirectX::XMFLOAT2(1.0f, 1.0f));
	spriteBatch->End();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	ImGui::Begin("Model");
	ImGui::DragFloat3("coords", translationOffset, 0.1f, -10.0f, 10.0f);
	ImGui::DragFloat3("rotation", rotationOffset, 0.1f, -10.0f, 10.0f);
	//ImGui::DragFloat("alpha1", &alpha, 0.01f, 0.0f, 1.0f);
	//ImGui::DragFloat3("coords2", translationOffset2, 0.1f, -10.0f, 10.0f);
	//ImGui::DragFloat("alpha2", &alpha2, 0.01f, 0.0f, 1.0f);
	ImGui::End();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	this->swapchain->Present(0, NULL); //VSYNC ON -- 1, OFF -- 0
}

#include "Graphics.hpp"

#include <sstream>
#include <string>
#include <windows.h>
#include <iomanip>

#define numSkulls 3

static std::vector<float> translationOffset(3 * numSkulls, 0.0f);
static std::vector<float> rotationOffset(3 * numSkulls, 0.0f);
static std::vector<float> scaleOffset(3 * numSkulls, 3.0f);
static bool showLights = true;
static bool useComparisonSampler = true;
static bool turnOnBlinn = true;
static int shininess = 32;
static float lightSphereRadius = 0.05f;
static int cameraMode = 0;

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
	if (!InitializeShadowResources())
		return false;
	if (!InitializeHDRResources())
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
		{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },

		// Matrix Row 1 (float4)
		{ "INSTANCE_MAT", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		// Matrix Row 2 (float4)
		{ "INSTANCE_MAT", 1, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		// Matrix Row 3 (float4)
		{ "INSTANCE_MAT", 2, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA, 1 },
		// Matrix Row 4 (float4)
		{ "INSTANCE_MAT", 3, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 1, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA, 1 },
	};
	UINT numElements = ARRAYSIZE(layout);

	if (!vertexShader.Initialize(device, GetExecutableFolder() + L"vertexShader.cso", layout, numElements))
		return false;
	if (!pixelShader.Initialize(device, GetExecutableFolder() + L"pixelShader.cso"))
		return false;
	if (!pixelShader_nolight.Initialize(device, GetExecutableFolder() + L"pixelShader_nolight.cso"))
		return false;
	if (!pixelShader_noComparisonSampler.Initialize(device, GetExecutableFolder() + L"pixelShader_noCompSampler.cso"))
		return false;
	if (!voronoiseShader.Initialize(device, GetExecutableFolder() + L"voronoisePixelShader.cso"))
		return false;
	if (!warpShader.Initialize(device, GetExecutableFolder() + L"WarpEffectPS.cso"))
		return false;

	D3D11_INPUT_ELEMENT_DESC fullscreenLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT fullscreenNumElements = ARRAYSIZE(fullscreenLayout);
	if (!fullscreenVS.Initialize(device, GetExecutableFolder() + L"fullscreenVS.cso", fullscreenLayout, fullscreenNumElements))
		return false;
	if (!tonemapPS.Initialize(device, GetExecutableFolder() + L"tonemapPS.cso"))
		return false;
	return true;
}


bool Graphics::InitializeHDRResources() {
	try {
		CD3D11_TEXTURE2D_DESC hdrTexDesc(
			DXGI_FORMAT_R16G16B16A16_FLOAT,
			windowWidth, windowHeight
		);
		hdrTexDesc.MipLevels = 1;
		hdrTexDesc.ArraySize = 1;
		hdrTexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = device->CreateTexture2D(&hdrTexDesc, nullptr, hdrTexture.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create HDR texture");

		hr = device->CreateRenderTargetView(hdrTexture.Get(), nullptr, hdrRTV.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create HDR RTV");

		hr = device->CreateShaderResourceView(hdrTexture.Get(), nullptr, hdrSRV.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create HDR SRV");

		D3D11_RASTERIZER_DESC rsDesc = {};
		rsDesc.FillMode = D3D11_FILL_SOLID;
		rsDesc.CullMode = D3D11_CULL_NONE;
		hr = device->CreateRasterizerState(&rsDesc, &fullscreenRS);
		COM_ERROR_IF_FAILED(hr, "Failed to create rasterizer state.");

		if (!fullscreenQuad.Initialize(this->device.Get(), this->deviceContext.Get()))
			return false;
		return true;
	}
	catch (COMException& exception) {
		ErrorLogger::Log(exception);
		return false;
	}
}


bool Graphics::InitializeShadowResources() 
{
	try {
		CD3D11_TEXTURE2D_DESC shadowTexDesc(
			DXGI_FORMAT_R24G8_TYPELESS,
			SHADOW_MAP_WIDTH, SHADOW_MAP_HEIGHT
		);
		shadowTexDesc.MipLevels = 1;
		shadowTexDesc.ArraySize = MAX_SHADOWS;
		shadowTexDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

		HRESULT hr = device->CreateTexture2D(&shadowTexDesc, nullptr, &shadowTextureArray);
		COM_ERROR_IF_FAILED(hr, "Failed to create shadow texture array");

		shadowDSVs.resize(MAX_SHADOWS);
		for (UINT i = 0; i < MAX_SHADOWS; i++) {
			CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(
				D3D11_DSV_DIMENSION_TEXTURE2DARRAY,
				DXGI_FORMAT_D24_UNORM_S8_UINT,
				0,
				i,
				1
			);

			hr = device->CreateDepthStencilView(
				shadowTextureArray.Get(),
				&dsvDesc,
				&shadowDSVs[i]
			);
			COM_ERROR_IF_FAILED(hr, "Failed to create shadow DSV for slice");
		}

		CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(
			D3D11_SRV_DIMENSION_TEXTURE2DARRAY,
			DXGI_FORMAT_R24_UNORM_X8_TYPELESS,
			0,
			1, 
			0, 
			MAX_SHADOWS
		);

		hr = device->CreateShaderResourceView(
			shadowTextureArray.Get(),
			&srvDesc,
			&shadowSRVArray
		);
		COM_ERROR_IF_FAILED(hr, "Failed to create shadow SRV array");

		D3D11_RASTERIZER_DESC rsDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
		rsDesc.CullMode = D3D11_CULL_FRONT;
		rsDesc.DepthBias = 1000;
		rsDesc.DepthBiasClamp = 0.0f;
		rsDesc.SlopeScaledDepthBias = 1.0f;
		rsDesc.FillMode = D3D11_FILL_SOLID;

		hr = device->CreateRasterizerState(&rsDesc, this->shadowRS.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create shadow rasterizer state.");

		D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
		samplerDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
		samplerDesc.BorderColor[0] = 1.0f;
		samplerDesc.BorderColor[1] = 1.0f;
		samplerDesc.BorderColor[2] = 1.0f;
		samplerDesc.BorderColor[3] = 1.0f;

		hr = device->CreateSamplerState(&samplerDesc, this->shadowSamplerState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create shadow sampler state.");
		return true;
	}
	catch (COMException& exception) {
		ErrorLogger::Log(exception);
		return false;
	}
}


bool Graphics::InitializeScene()
{
	try
	{
		//Constant buffers
		HRESULT hr = this->cb_vertexShader.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_ps_light.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_ps_lightModelColor.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		this->cb_ps_light.data.ambientLightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		this->cb_ps_light.data.ambientLightStrength = 0.1f;
		this->cb_ps_light.data.shadowBias = 0.001f;
		this->cb_ps_light.data.texelSize = XMFLOAT2(1.0f / (float) SHADOW_MAP_WIDTH, 1.0f / (float) SHADOW_MAP_HEIGHT);
		this->cb_ps_light.data.pcfKernelSize = 3;

		hr = this->psConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize voronoise pixel shader constant buffer.");

		hr = this->warpConstantBuffer.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize warp pixel shader constant buffer.");

		hr = this->cbTonemap.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize tonemap constant buffer");

		this->cbTonemap.data.exposure = 1.0f;
		this->cbTonemap.data.gamma = 1.0f;
		this->cbTonemap.data.tonemapOperator = 1;

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

		std::vector<Vertex> vertices = {
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

		std::vector<DWORD> indices = {
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

		std::vector<Texture> textures;
		textures.emplace_back(this->device.Get(), "Data/Textures/grid.jpg", aiTextureType::aiTextureType_DIFFUSE);

		if (!plane.Initialize(vertices, indices, textures, XMMatrixIdentity(), this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;
		{
			const float radius = 5.0f;

			for(int i = 0; i < numSkulls; ++i)
			{
				RenderableGameObject skull;
				if (!skull.Initialize("Data/Models/Skull/stylized_dragon_skull.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
					return false;

				float angle = XM_2PI * i / numSkulls;

				translationOffset[i * 3] = radius * cosf(angle);
				translationOffset[i * 3 + 2] = radius * sinf(angle);
				rotationOffset[i * 3 + 1] = angle;
				skulls.push_back(skull);
			}
		}
		{
			Light light1;
			if (!light1.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Directional))
				return false;
			light1.SetScale(0.0f, 0.0f, 0.0f);
			light1.lightColor = XMFLOAT3(1.0f, 0.5f, 0.0f);
			light1.lightStrength = 1.0f;
			this->cb_ps_light.data.lights[0].direction = { -1.0f, -15.0f, 27.0f };
			dynamicLights.push_back(std::move(light1));

			Light light2;
			if (!light2.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Point))
				return false;
			light2.lightPosition = XMFLOAT3(0.0f, 2.0f, 0.0f);
			light2.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light2.lightColor = XMFLOAT3(0.0f, 0.0f, 1.0f);
			light1.lightStrength = 2.0f;
			dynamicLights.push_back(std::move(light2));

			Light light3;
			if (!light3.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Spot))
				return false;
			light3.lightPosition = XMFLOAT3(4.9f, 4.0f, -3.6f);
			light3.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light3.lightColor = XMFLOAT3(1.0f, 0.0f, 0.0f);
			light3.lightStrength = 15.0f;
			this->cb_ps_light.data.lights[2].direction = { 0.46f, -4.46f, 5.39f };
			dynamicLights.push_back(std::move(light3));

			Light light4;
			if (!light4.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Spot))
				return false;
			light4.lightPosition = XMFLOAT3(0.0f, 5.0f, 10.0f);
			light4.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light4.lightColor = XMFLOAT3(1.0f, 0.0f, 0.0f);
			light4.lightStrength = 15.0f;
			this->cb_ps_light.data.lights[3].direction = { -1.03f, -1.8f, -2.04f };
			dynamicLights.push_back(std::move(light4));
		}

		for (size_t i = 0; i < dynamicLights.size() && i < MAX_LIGHTS; ++i)
		{
			const Light& currentLight = dynamicLights[i];
			LightData& targetData = this->cb_ps_light.data.lights[i];

			targetData.type = (int)currentLight.type;
			targetData.color = currentLight.lightColor;
			targetData.strength = currentLight.lightStrength;
			targetData.position = currentLight.GetPositionFloat3();
			targetData.attenuation_a = 1.0f;
			targetData.attenuation_b = 0.1f;
			targetData.attenuation_c = 0.1f;
			targetData.spotInnerAngle = XMConvertToRadians(15.0f);
			targetData.spotOuterAngle = XMConvertToRadians(25.0f);
		}

		this->cb_ps_light.data.lights[1].attenuation_a = 1.0f;
		this->cb_ps_light.data.lights[1].attenuation_b = 0.0f;
		this->cb_ps_light.data.lights[1].attenuation_c = 0.0f;

		plane.SetScale(200.0f, 1.0f, 200.0f);
		plane.SetPosition(0.0f, -1.0f, -100.0f);
		camera.SetPosition(0.0f, 0.0f, 0.0f);
		camera.SetProjectionValues(90.0f, static_cast<float>(windowWidth) / static_cast<float>(windowHeight), 0.1f, 1000.0f);
	}
	catch (COMException &exception)
	{
		ErrorLogger::Log(exception);
		return false;
	}
	return true;
}


DirectX::XMFLOAT3 GetDirectionFromRotation(const DirectX::XMFLOAT3& rotation)
{
	using namespace DirectX;

	float cp = cosf(-rotation.x + 3.14f);
	float sp = sinf(-rotation.x + 3.14f);
	float cy = cosf(-rotation.y);
	float sy = sinf(-rotation.y);

	XMFLOAT3 direction;
	direction.x = sy * cp;
	direction.y = -sp;
	direction.z = -cy * cp;

	XMVECTOR dirVec = XMVector3Normalize(XMLoadFloat3(&direction));
	XMStoreFloat3(&direction, dirVec);
	return direction;
}


void Graphics::ShadowPass()
{
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->IASetInputLayout(this->vertexShader.GetInputLayout());
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);

	this->deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(nullptr, NULL, 0);

	this->cb_ps_light.data.cameraPos = camera.GetPositionFloat3();
	this->cb_ps_light.data.numLights = (int)dynamicLights.size();

	for (size_t i = 0; i < dynamicLights.size() && i < MAX_LIGHTS; ++i)
	{
		const Light& currentLight = dynamicLights[i];
		LightData& targetData = this->cb_ps_light.data.lights[i];

		targetData.type = (int)currentLight.type;
		targetData.color = currentLight.lightColor;
		targetData.strength = currentLight.lightStrength;
		targetData.position = currentLight.GetPositionFloat3();
		targetData.turnOnBlinn = turnOnBlinn ? 1 : 0;
		targetData.shininess = shininess;
		targetData.lightOn = currentLight.lightOn && showLights;

		XMMATRIX lightWVP = XMMatrixIdentity();
		if (currentLight.type == LightType::Directional)
			lightWVP = CalculateDirectionalLightVP(cb_ps_light.data.lights[i].direction);
		else if (currentLight.type == LightType::Spot)
			lightWVP = CalculateSpotlightVP(targetData);
		else
			continue;
		targetData.lightWVP = lightWVP;

		D3D11_VIEWPORT shadowViewport = {};
		shadowViewport.Width = SHADOW_MAP_WIDTH;
		shadowViewport.Height = SHADOW_MAP_HEIGHT;
		shadowViewport.MinDepth = 0.0f;
		shadowViewport.MaxDepth = 1.0f;
		this->deviceContext->RSSetViewports(1, &shadowViewport);

		this->deviceContext->ClearDepthStencilView(this->shadowDSVs[i].Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
		this->deviceContext->OMSetRenderTargets(0, nullptr, this->shadowDSVs[i].Get());

		{
			this->plane.Draw(lightWVP);
			for (int i = 0; i < numSkulls; ++i)
			{
				skulls[i].SetPosition(translationOffset[i * 3], translationOffset[i * 3 + 1], translationOffset[i * 3 + 2]);
				skulls[i].SetRotation(rotationOffset[i * 3], rotationOffset[i * 3 + 1], rotationOffset[i * 3 + 2]);
				skulls[i].SetScale(scaleOffset[i * 3], scaleOffset[i * 3 + 1], scaleOffset[i * 3 + 2]);
				skulls[i].Draw(lightWVP);
			}
		}
	}

	this->cb_ps_light.ApplyChanges();
}


void Graphics::MainRenderPass()
{
	this->deviceContext->OMSetRenderTargets(1, this->hdrRTV.GetAddressOf(), this->depthStencilView.Get());

	this->deviceContext->IASetInputLayout(this->vertexShader.GetInputLayout());
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
	this->deviceContext->OMSetBlendState(this->blendState.Get(), NULL, 0xFFFFFFFF);
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->deviceContext->PSSetShaderResources(1, 1, shadowSRVArray.GetAddressOf());
	this->deviceContext->PSSetSamplers(1, 1, shadowSamplerState.GetAddressOf());
	this->deviceContext->PSSetConstantBuffers(0, 1, this->cb_ps_light.GetAddressOf());
	this->deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	if (useComparisonSampler)
		this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);
	else
		this->deviceContext->PSSetShader(pixelShader_noComparisonSampler.GetShader(), NULL, 0);

	D3D11_VIEWPORT mainViewport = {};
	mainViewport.Width = static_cast<float>(windowWidth);
	mainViewport.Height = static_cast<float>(windowHeight);
	mainViewport.MinDepth = 0.0f;
	mainViewport.MaxDepth = 1.0f;
	this->deviceContext->RSSetViewports(1, &mainViewport);

	XMMATRIX vp = camera.GetViewMatrix() * camera.GetProjectionMatrix();
	if (cameraMode == 1)
		vp = CalculateDirectionalLightVP(cb_ps_light.data.lights[0].direction);
	else if (cameraMode == 2)
		vp = CalculateSpotlightVP(cb_ps_light.data.lights[3]);
	{
		this->plane.Draw(vp);
		for (int i = 0; i < numSkulls; ++i)
		{
			skulls[i].SetPosition(translationOffset[i * 3], translationOffset[i * 3 + 1], translationOffset[i * 3 + 2]);
			skulls[i].SetRotation(rotationOffset[i * 3], rotationOffset[i * 3 + 1], rotationOffset[i * 3 + 2]);
			skulls[i].SetScale(scaleOffset[i * 3], scaleOffset[i * 3 + 1], scaleOffset[i * 3 + 2]);
			skulls[i].Draw(vp);
		}
	}
	{
		this->deviceContext->PSSetShader(pixelShader_nolight.GetShader(), NULL, 0);
		this->deviceContext->PSSetConstantBuffers(1, 1, this->cb_ps_lightModelColor.GetAddressOf());
		for (auto& light : dynamicLights)
		{
			if (light.lightOn && showLights)
			{
				this->cb_ps_lightModelColor.data.lightColor = light.lightColor;
				this->cb_ps_lightModelColor.ApplyChanges();
				if (light.type != LightType::Directional)
					light.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
				light.Draw(vp);
			}
		}
	}
}


void Graphics::ToneMappingPass()
{
	this->deviceContext->OMSetRenderTargets(1, this->renderTargetView.GetAddressOf(), nullptr);
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	this->deviceContext->IASetInputLayout(fullscreenVS.GetInputLayout());

	this->deviceContext->VSSetShader(fullscreenVS.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(tonemapPS.GetShader(), NULL, 0);

	this->deviceContext->PSSetShaderResources(0, 1, this->hdrSRV.GetAddressOf());
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->cbTonemap.ApplyChanges();
	this->deviceContext->PSSetConstantBuffers(0, 1, this->cbTonemap.GetAddressOf());

	deviceContext->RSSetState(this->fullscreenRS.Get());

	fullscreenQuad.Draw();
	ID3D11ShaderResourceView* nullSRV = nullptr;
	this->deviceContext->PSSetShaderResources(0, 1, &nullSRV);
}


void Graphics::ImGUIPass()
{
	//FPS counter
	ShowFPSstats();
	ShowCoords("Camera", this->camera.GetPositionVector(), 40.0f);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Models");
	for (int i = 0; i < numSkulls; ++i)
	{
		std::string ind = std::to_string(i + 1);
		std::string coords = "Coords " + ind;
		std::string rotation = "Rotation " + ind;
		std::string scale = "Scale " + ind;
		ImGui::DragFloat3(coords.c_str(), &translationOffset[i * 3], 0.1f, -1000.0f, 1000.0f);
		ImGui::DragFloat3(rotation.c_str(), &rotationOffset[i * 3], 0.1f, -1000.0f, 1000.0f);
		ImGui::DragFloat3(scale.c_str(), &scaleOffset[i * 3], 0.1f, -1000.0f, 1000.0f);
	}
	ImGui::End();

	ImGui::Begin("Light general");
	ImGui::DragFloat3("Ambient Light Color", &this->cb_ps_light.data.ambientLightColor.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Ambient Light Strength", &this->cb_ps_light.data.ambientLightStrength, 0.01f, 0.0f, 1.0f);
	ImGui::DragInt("Shininess", &shininess, 1, 1, 32);
	ImGui::DragFloat("Sphere radius", &lightSphereRadius, 0.01f, 0.0f, 100.0f);
	ImGui::Checkbox("Show lights", &showLights);
	ImGui::Checkbox("Turn on blinn", &turnOnBlinn);
	ImGui::DragFloat("Shadow bias", &this->cb_ps_light.data.shadowBias, 0.00001f, 0.0f, 0.01f, "%.5f");
	ImGui::DragInt("PCF Kernel size", &this->cb_ps_light.data.pcfKernelSize, 2, 1, 11);
	ImGui::Checkbox("Use Comparison Sampler", &useComparisonSampler);
	const char* cameraSource[] = {
		"Default",
		"Directional",
		"Spot",
	};
	ImGui::Combo("Camera source", &cameraMode, cameraSource, IM_ARRAYSIZE(cameraSource));
	ImGui::End();

	for (size_t i = 0; i < dynamicLights.size(); ++i)
	{
		std::string lightName = std::to_string(i + 1) + ". ";
		if (dynamicLights[i].type == LightType::Directional)
			lightName += "Directional";
		else if (dynamicLights[i].type == LightType::Point)
			lightName += "Point";
		else if (dynamicLights[i].type == LightType::Spot)
			lightName += "Spot";
		else
			lightName += "Unknown";

		lightName += " light";

		ImGui::Begin(lightName.c_str());

		ImGui::DragFloat3("Color", &this->dynamicLights[i].lightColor.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Strength", &this->dynamicLights[i].lightStrength, 0.1f, 0.0f, 100.0f);

		if (dynamicLights[i].type != LightType::Directional)
		{
			ImGui::DragFloat3("Position", &this->dynamicLights[i].lightPosition.x, 0.1f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Attenuation a", &this->cb_ps_light.data.lights[i].attenuation_a, 0.01f, 0.01f, 100.0f);
			ImGui::DragFloat("Attenuation b", &this->cb_ps_light.data.lights[i].attenuation_b, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Attenuation c", &this->cb_ps_light.data.lights[i].attenuation_c, 0.01f, 0.0f, 100.0f);
		}
		if (dynamicLights[i].type != LightType::Point)
		{
			ImGui::DragFloat3("Light direction", &this->cb_ps_light.data.lights[i].direction.x, 0.01f, -100.0f, 100.0f);
		}
		if (dynamicLights[i].type == LightType::Spot)
		{
			ImGui::DragFloat("Inner angle", &this->cb_ps_light.data.lights[i].spotInnerAngle, 0.01f, -XM_2PI, XM_2PI);
			ImGui::DragFloat("Outer angle", &this->cb_ps_light.data.lights[i].spotOuterAngle, 0.01f, -XM_2PI, XM_2PI);
		}
		ImGui::Checkbox("Light on", &this->dynamicLights[i].lightOn);
		ImGui::End();
	}

	ImGui::Begin("HDR Tone Mapping");
	ImGui::DragFloat("Exposure", &this->cbTonemap.data.exposure, 0.01f, 0.1f, 5.0);
	ImGui::DragFloat("Gamma", &this->cbTonemap.data.gamma, 0.01f, 1.0f, 3.0f);
	const char* tonemapOperators[] = {
		"Reinhard",      // 0
		"ACES",          // 1  
		"Uncharted 2",   // 2
		"OFF"
	};
	ImGui::Combo("Tonemap Operator", &this->cbTonemap.data.tonemapOperator, tonemapOperators, IM_ARRAYSIZE(tonemapOperators));
	ImGui::End();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}
 

void Graphics::RenderFrame()
{
	float backgroundColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), backgroundColor);
	this->deviceContext->ClearRenderTargetView(this->hdrRTV.Get(), backgroundColor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	
	ShadowPass();
	MainRenderPass();
	ToneMappingPass();
	ImGUIPass();
	this->swapchain->Present(0, NULL); //VSYNC ON -- 1, OFF -- 0
}


void Graphics::ShowFPSstats()
{
	static int fpsCounter = 0;
	static float timeAccumulator = 0.0f;
	static std::string fpsString = "FPS: 0";
	static std::string msString = "MS/Frame: 0.0";

	float msPerFrame = fpsTimer.GetMilisecondsElapsed();
	fpsTimer.Restart();

	fpsCounter++;
	timeAccumulator += msPerFrame;

	const float UPDATE_INTERVAL_MS = 500.0f;

	if (timeAccumulator >= UPDATE_INTERVAL_MS)
	{
		float averageMs = timeAccumulator / fpsCounter;

		std::stringstream ss_ms;
		ss_ms << "MS/Frame: " << std::fixed << std::setprecision(5) << averageMs;
		msString = ss_ms.str();

		float averageFps = 1000.0f / averageMs;

		std::stringstream ss_fps;
		ss_fps << "FPS: " << std::fixed << std::setprecision(0) << averageFps;
		fpsString = ss_fps.str();

		fpsCounter = 0;
		timeAccumulator = 0.0f;
	}

	spriteBatch->Begin();

	spriteFont->DrawString(
		spriteBatch.get(),
		StringHelper::StringToWide(fpsString).c_str(),
		DirectX::XMFLOAT2(0, 0),
		DirectX::Colors::White
	);

	spriteFont->DrawString(
		spriteBatch.get(),
		StringHelper::StringToWide(msString).c_str(),
		DirectX::XMFLOAT2(0, 20),
		DirectX::Colors::White
	);

	spriteBatch->End();
}


void Graphics::ShowCoords(const std::string& objectName, const DirectX::XMVECTOR& position, float screenY)
{
	DirectX::XMFLOAT3 posFloat3;
	DirectX::XMStoreFloat3(&posFloat3, position);

	std::stringstream ss_coords;

	ss_coords << objectName << " coords: ";
	ss_coords << std::fixed << std::setprecision(2);

	ss_coords << "X: " << posFloat3.x;
	ss_coords << ", Y: " << posFloat3.y;
	ss_coords << ", Z: " << posFloat3.z;

	std::string coordsString = ss_coords.str();

	this->spriteBatch->Begin();

	this->spriteFont->DrawString(
		this->spriteBatch.get(),
		StringHelper::StringToWide(coordsString).c_str(),
		DirectX::XMFLOAT2(0, screenY),
		DirectX::Colors::White
	);

	this->spriteBatch->End();
}

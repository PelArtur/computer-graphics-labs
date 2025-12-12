#include "Graphics.hpp"

#include <sstream>
#include <string>
#include <windows.h>
#include <iomanip>

static int width = 1024;
static int depth = 1024;
static float height = 5.0f;
static std::vector<float> translationOffset(3, 0.0f);
static std::vector<float> rotationOffset(3, 0.0f);
static std::vector<float> scaleOffset(3, 3.0f);
static bool showLights = true;
static bool turnOnBlinn = true;
static int shininess = 32;
static float lightSphereRadius = 0.05f;
static float lastGenerationTime = 0.0f;
static int lastWidth = 1024;
static int lastDepth = 1024;

bool Graphics::Initialize(HWND hwnd, int width, int height) 
{
	this->windowWidth = width;
	this->windowHeight = height;
	this->fpsTimer.Start();
	this->shadersTimer.Start();
	this->terrainGenerationTimer.Start();

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
		{ "TEXCOORD", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0 },
		//{ "INSTANCEOFFSET", 0, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA, 1 },

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
		//Constant buffers
		HRESULT hr = this->cb_vertexShader.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_ps_light.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_ps_lightModelColor.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = this->cb_ps_terrain_params.Initialize(this->device.Get(), this->deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		this->cb_ps_light.data.ambientLightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		this->cb_ps_light.data.ambientLightStrength = 0.1f;

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

		scaleOffset[0] = scaleOffset[1] = scaleOffset[2] = 1.0f;
		translationOffset[0] = translationOffset[1] = translationOffset[2] = 0.0f;
		InitializePlane();
		std::vector<Texture> textures;
		textures.emplace_back(this->device.Get(), "Data/Textures/grid.jpg", aiTextureType::aiTextureType_DIFFUSE);

		//Initialize Model
		{
			Light light1;
			if (!light1.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Directional))
				return false;
			light1.SetScale(0.0f, 0.0f, 0.0f);
			light1.lightColor = XMFLOAT3(1.0f, 0.5f, 0.0f);
			light1.lightStrength = 1.0f;
			this->cb_ps_light.data.lights[0].direction = { 9.0f, -5.0f, 2.54f };
			dynamicLights.push_back(std::move(light1));

			Light light2;
			if (!light2.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Point))
				return false;
			light2.lightPosition = XMFLOAT3(0.0f, 2.0f, 0.0f);
			light2.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light2.lightColor = XMFLOAT3(0.0f, 0.0f, 1.0f);
			light1.lightStrength = 2.0f;
			//dynamicLights.push_back(std::move(light2));

			Light light3;
			if (!light3.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Point))
				return false;
			light3.lightPosition = XMFLOAT3(7.7f, 4.0f, 0.0f);
			light3.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light3.lightColor = XMFLOAT3(0.0f, 0.0f, 1.0f);
			light3.lightStrength = 5.0f;
			//dynamicLights.push_back(std::move(light3));

			Light light4;
			if (!light4.Initialize(this->device.Get(), this->deviceContext.Get(), cb_vertexShader, LightType::Spot))
				return false;
			light4.lightPosition = XMFLOAT3(0.0f, 5.0f, 10.0f);
			light4.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light4.lightColor = XMFLOAT3(1.0f, 0.0f, 0.0f);
			light4.lightStrength = 15.0f;
			this->cb_ps_light.data.lights[3].direction = { -1.03f, -1.8f, -2.04f };
			//dynamicLights.push_back(std::move(light4));
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



bool Graphics::InitializePlane()
{
	this->terrainGenerationTimer.Restart();
	lastWidth = width;
	lastDepth = depth;
	std::vector<Vertex> vertices;
	std::vector<DWORD> indices;

	auto terrainMesh = terrain.GenerateTerrain(width, depth, height);
	vertices = terrainMesh.vertices;
	indices = terrainMesh.indices;

	float maxY = vertices[0].pos.y;
	float minY = vertices[0].pos.y;
	for (auto& vert : vertices)
	{
		if (maxY < vert.pos.y)
			maxY = vert.pos.y;
		if (minY > vert.pos.y)
			minY = vert.pos.y;
	}

	this->cb_ps_terrain_params.data.minHeight = minY;
	this->cb_ps_terrain_params.data.maxHeight = maxY;
	this->cb_ps_terrain_params.ApplyChanges();
	std::vector<Texture> textures;
	textures.emplace_back(this->device.Get(), "Data/Textures/grid.jpg", aiTextureType::aiTextureType_DIFFUSE);

	//Initialize Model
	if (!plane.Initialize(vertices, indices, textures, XMMatrixIdentity(), this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
		return false;

	plane.SetRotation(rotationOffset[0], rotationOffset[1], rotationOffset[2]);
	plane.SetScale(scaleOffset[0], scaleOffset[1], scaleOffset[2]);
	plane.SetPosition(translationOffset[0], translationOffset[1], translationOffset[2]);
	lastGenerationTime = this->terrainGenerationTimer.GetMilisecondsElapsed();
	return true;
}


void Graphics::RenderFrame()
{
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
	}

	this->cb_ps_light.ApplyChanges();
	this->deviceContext->PSSetConstantBuffers(0, 1, this->cb_ps_light.GetAddressOf());

	float backgroundColor[] = { 154.0f / 255.0f, 189.0f / 255.0f, 254.0f / 255.0f, 1.0f };
	this->deviceContext->ClearRenderTargetView(this->renderTargetView.Get(), backgroundColor);
	this->deviceContext->ClearDepthStencilView(this->depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	this->deviceContext->IASetInputLayout(this->vertexShader.GetInputLayout());
	this->deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	this->deviceContext->RSSetState(this->rasterizerState.Get());
	this->deviceContext->OMSetDepthStencilState(this->depthStencilState.Get(), 0);
	this->deviceContext->OMSetBlendState(this->blendState.Get(), NULL, 0xFFFFFFFF);
	this->deviceContext->PSSetSamplers(0, 1, this->samplerState.GetAddressOf());
	this->deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	this->deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);

	{
		this->deviceContext->PSSetConstantBuffers(2, 1, this->cb_ps_terrain_params.GetAddressOf());
		this->plane.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
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
				light.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
			}
		}
	}

	//FPS counter
	ShowFPSstats();
	ShowCoords("Camera", this->camera.GetPositionVector(), 40.0f);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	ImGui::Begin("Terrain");

	ImGui::DragFloat3("Coords", &translationOffset[0], 0.1f, -1000.0f, 1000.0f);
	ImGui::DragFloat3("Rotation", &rotationOffset[0], 0.1f, -1000.0f, 1000.0f);
	ImGui::DragFloat3("Scale", &scaleOffset[0], 0.1f, -1000.0f, 1000.0f);
	ImGui::DragFloat("Noise Scale", &terrain.noiseScale, 0.01f, 0.0f, 1000.0f);
	ImGui::DragInt("Octaves", &terrain.octaves, 1, 1, 16);
	ImGui::DragFloat("Persistence", &terrain.persistence, 0.01f, 0.0f, 10.0f);
	ImGui::DragFloat("Lacunarity", &terrain.lacunarity, 0.01f, 1.0f, 10.0f);
	ImGui::DragFloat2("Noise offset", &terrain.noiseOffset.x, 0.01f, 1.0f, 10.0f);
	ImGui::DragInt("Width", &width, 1, 100, 1024);
	ImGui::DragInt("Depth", &depth, 1, 100, 1024);
	ImGui::DragFloat("Height", &height, 1.0f, 1.0f, 100.0f);
	const char* modes[] = {
		"Exponential",
		"Logarithmic",
		"Quadratic",
		"Cubic",
		"Power"
	};

	int currentMode = static_cast<int>(terrain.GetCurveMode());
	if (ImGui::Combo("Curve mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
		terrain.SetCurveMode(currentMode);
	if (currentMode == 4)
		ImGui::DragFloat("Power", &terrain.power, 0.01f, 0.0f, 10.0f);
	if (ImGui::Button("Rebuild Terrain"))
	{
		InitializePlane();
	}
	ImGui::End();
	
	
	{
		{
			ImGui::Begin("Light general");
			ImGui::DragFloat3("Ambient Light Color", &this->cb_ps_light.data.ambientLightColor.x, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Ambient Light Strength", &this->cb_ps_light.data.ambientLightStrength, 0.01f, 0.0f, 1.0f);
			ImGui::DragInt("Shininess", &shininess, 1, 1, 32);
			ImGui::DragFloat("Sphere radius", &lightSphereRadius, 0.01f, 0.0f, 100.0f);
			ImGui::Checkbox("Show lights", &showLights);
			ImGui::Checkbox("Turn on blinn", &turnOnBlinn);
			ImGui::End();
		}

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
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	this->swapchain->Present(0, NULL); //VSYNC ON -- 1, OFF -- 0
}


void Graphics::ShowFPSstats()
{
	static int fpsCounter = 0;
	static float timeAccumulator = 0.0f;
	static std::string fpsString = "FPS: 0";
	static std::string msString = "MS/Frame: 0.0";
	static std::string generationString = "(1024x1024: 1,048,576): 0.0";

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

	generationString = '(' +  std::to_string(lastWidth) + 'x' + std::to_string(lastDepth);
	generationString += ": " + std::to_string(lastWidth * lastDepth) + ") ";
	generationString += std::to_string(lastGenerationTime);

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

	spriteFont->DrawString(
		spriteBatch.get(),
		StringHelper::StringToWide(generationString).c_str(),
		DirectX::XMFLOAT2(0, 60),
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

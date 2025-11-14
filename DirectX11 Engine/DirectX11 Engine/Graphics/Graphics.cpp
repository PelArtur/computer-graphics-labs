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
static bool turnOnBlinn = true;
static int shininess = 32;
static float lightSphereRadius = 0.05f;

bool Graphics::Initialize(HWND hwnd, int width, int height) 
{
	windowWidth = width;
	windowHeight = height;
	fpsTimer.Start();
	shadersTimer.Start();

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
	ImGui_ImplDX11_Init(device.Get(), deviceContext.Get());
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

		scd.BufferDesc.Width = windowWidth;
		scd.BufferDesc.Height = windowHeight;
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
			swapchain.GetAddressOf(),		     //Swapchain Address
			device.GetAddressOf(),		         //Device Address
			NULL,								 //Supported feature level
			deviceContext.GetAddressOf());       //Device Context Address
		COM_ERROR_IF_FAILED(hr, "Failed to create device and swapchain.");

		Microsoft::WRL::ComPtr<ID3D11Texture2D> backBuffer;
		hr = swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));
		COM_ERROR_IF_FAILED(hr, "GetBuffer Failed.");

		hr = device->CreateRenderTargetView(backBuffer.Get(), NULL, renderTargetView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create render target view.");

		//Describe our Depth/Stencil Buffer
		CD3D11_TEXTURE2D_DESC depthStencilTextureDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, windowWidth, windowHeight);
		depthStencilTextureDesc.MipLevels = 1;
		depthStencilTextureDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;

		hr = device->CreateTexture2D(&depthStencilTextureDesc, NULL, depthStencilBuffer.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil buffer.");

		hr = device->CreateDepthStencilView(depthStencilBuffer.Get(), NULL, depthStencilView.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil view.");

		deviceContext->OMSetRenderTargets(1, renderTargetView.GetAddressOf(), depthStencilView.Get());

		//Create depth stencil state
		CD3D11_DEPTH_STENCIL_DESC depthstencildesc(D3D11_DEFAULT);
		depthstencildesc.DepthFunc = D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL;

		hr = device->CreateDepthStencilState(&depthstencildesc, depthStencilState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create depth stencil state.");

		//Create and set the Viewport
		CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(windowWidth), static_cast<float>(windowHeight));;
		deviceContext->RSSetViewports(1, &viewport);

		CD3D11_RASTERIZER_DESC rasterizerDesc(D3D11_DEFAULT);
		hr = device->CreateRasterizerState(&rasterizerDesc, rasterizerState.GetAddressOf());
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

		hr = device->CreateBlendState(&blendDesc, blendState.GetAddressOf());
		COM_ERROR_IF_FAILED(hr, "Failed to create blend state.");

		spriteBatch = std::make_unique<DirectX::SpriteBatch>(deviceContext.Get());
		spriteFont = std::make_unique<DirectX::SpriteFont>(device.Get(), L"Data/Fonts/comic_sans_ms_16");

		//Sample descritption
		CD3D11_SAMPLER_DESC sampDesc(D3D11_DEFAULT);
		sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
		sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
		hr = device->CreateSamplerState(&sampDesc, samplerState.GetAddressOf()); //Create sampler state
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
		HRESULT hr = cb_vertexShader.Initialize(device.Get(), deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = cb_ps_light.Initialize(device.Get(), deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		hr = cb_ps_lightModelColor.Initialize(device.Get(), deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize constant buffer.");

		cb_ps_light.data.ambientLightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		cb_ps_light.data.ambientLightStrength = 0.1f;

		hr = psConstantBuffer.Initialize(device.Get(), deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize voronoise pixel shader constant buffer.");

		hr = warpConstantBuffer.Initialize(device.Get(), deviceContext.Get());
		COM_ERROR_IF_FAILED(hr, "Failed to initialize warp pixel shader constant buffer.");

		psConstantBuffer.data.iResolution = {
			static_cast<float>(windowWidth),
			static_cast<float>(windowHeight),
			1.0f / static_cast<float>(windowWidth),
			1.0f / static_cast<float>(windowHeight)
		};

		warpConstantBuffer.data.iResolution = {
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
		textures.emplace_back(device.Get(), "Data/Textures/grid.jpg", aiTextureType::aiTextureType_DIFFUSE);

		//Initialize Model
		if (!plane.Initialize(vertices, indices, textures, XMMatrixIdentity(), device.Get(), deviceContext.Get(), cb_vertexShader))
			return false;
		{
			const float radius = 5.0f;

			for(int i = 0; i < numSkulls; ++i)
			{
				RenderableGameObject skull;
				if (!skull.Initialize("Data/Models/Skull/stylized_dragon_skull.glb", device.Get(), deviceContext.Get(), cb_vertexShader))
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
			if (!light1.Initialize(device.Get(), deviceContext.Get(), cb_vertexShader, LightType::Directional))
				return false;
			light1.SetScale(0.0f, 0.0f, 0.0f);
			light1.lightColor = XMFLOAT3(1.0f, 0.5f, 0.0f);
			light1.lightStrength = 1.0f;
			cb_ps_light.data.lights[0].direction = { 9.0f, -5.0f, 2.54f };
			dynamicLights.push_back(std::move(light1));

			Light light2;
			if (!light2.Initialize(device.Get(), deviceContext.Get(), cb_vertexShader, LightType::Point))
				return false;
			light2.lightPosition = XMFLOAT3(0.0f, 2.0f, 0.0f);
			light2.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light2.lightColor = XMFLOAT3(0.0f, 0.0f, 1.0f);
			light1.lightStrength = 2.0f;
			dynamicLights.push_back(std::move(light2));

			Light light3;
			if (!light3.Initialize(device.Get(), deviceContext.Get(), cb_vertexShader, LightType::Point))
				return false;
			light3.lightPosition = XMFLOAT3(7.7f, 4.0f, 0.0f);
			light3.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light3.lightColor = XMFLOAT3(0.0f, 0.0f, 1.0f);
			light3.lightStrength = 5.0f;
			dynamicLights.push_back(std::move(light3));

			Light light4;
			if (!light4.Initialize(device.Get(), deviceContext.Get(), cb_vertexShader, LightType::Spot))
				return false;
			light4.lightPosition = XMFLOAT3(0.0f, 5.0f, 10.0f);
			light4.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
			light4.lightColor = XMFLOAT3(1.0f, 0.0f, 0.0f);
			light4.lightStrength = 15.0f;
			cb_ps_light.data.lights[3].direction = { -1.03f, -1.8f, -2.04f };
			dynamicLights.push_back(std::move(light4));
		}

		for (size_t i = 0; i < dynamicLights.size() && i < MAX_LIGHTS; ++i)
		{
			const Light& currentLight = dynamicLights[i];
			LightData& targetData = cb_ps_light.data.lights[i];

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

		cb_ps_light.data.lights[1].attenuation_a = 1.0f;
		cb_ps_light.data.lights[1].attenuation_b = 0.0f;
		cb_ps_light.data.lights[1].attenuation_c = 0.0f;

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


void Graphics::RenderFrame()
{
	cb_ps_light.data.cameraPos = camera.GetPositionFloat3();
	cb_ps_light.data.numLights = (int)dynamicLights.size();

	for (size_t i = 0; i < dynamicLights.size() && i < MAX_LIGHTS; ++i)
	{
		const Light& currentLight = dynamicLights[i];
		LightData& targetData = cb_ps_light.data.lights[i];

		targetData.type = (int)currentLight.type;
		targetData.color = currentLight.lightColor;
		targetData.strength = currentLight.lightStrength;
		targetData.position = currentLight.GetPositionFloat3();
		targetData.turnOnBlinn = turnOnBlinn ? 1 : 0;
		targetData.shininess = shininess;
		targetData.lightOn = currentLight.lightOn && showLights;
	}

	cb_ps_light.ApplyChanges();
	deviceContext->PSSetConstantBuffers(0, 1, cb_ps_light.GetAddressOf());

	float backgroundColor[] = {0.0f, 0.0f, 0.0f, 1.0f};
	deviceContext->ClearRenderTargetView(renderTargetView.Get(), backgroundColor);
	deviceContext->ClearDepthStencilView(depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	deviceContext->IASetInputLayout(vertexShader.GetInputLayout());
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	deviceContext->RSSetState(rasterizerState.Get());
	deviceContext->OMSetDepthStencilState(depthStencilState.Get(), 0);
	deviceContext->OMSetBlendState(blendState.Get(), NULL, 0xFFFFFFFF);
	deviceContext->PSSetSamplers(0, 1, samplerState.GetAddressOf());
	deviceContext->VSSetShader(vertexShader.GetShader(), NULL, 0);
	deviceContext->PSSetShader(pixelShader.GetShader(), NULL, 0);

	{
		plane.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
		for(int i = 0; i < numSkulls; ++i)
		{
			skulls[i].SetPosition(translationOffset[i * 3], translationOffset[i * 3 + 1], translationOffset[i * 3 + 2]);
			skulls[i].SetRotation(rotationOffset[i * 3], rotationOffset[i * 3 + 1], rotationOffset[i * 3 + 2]);
			skulls[i].SetScale(scaleOffset[i * 3], scaleOffset[i * 3 + 1], scaleOffset[i * 3 + 2]);
			skulls[i].Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
		}
	}
	{
		deviceContext->PSSetShader(pixelShader_nolight.GetShader(), NULL, 0);
		deviceContext->PSSetConstantBuffers(1, 1, cb_ps_lightModelColor.GetAddressOf());
		for (auto& light : dynamicLights)
		{
			if (light.lightOn && showLights)
			{
				cb_ps_lightModelColor.data.lightColor = light.lightColor;
				cb_ps_lightModelColor.ApplyChanges();
				if (light.type != LightType::Directional)
					light.SetScale(lightSphereRadius, lightSphereRadius, lightSphereRadius);
				light.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
			}
		}
	}

	//FPS counter
	ShowFPSstats();
	ShowCoords("Camera", camera.GetPositionVector(), 40.0f);

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
	ImGui::DragFloat3("Ambient Light Color", &cb_ps_light.data.ambientLightColor.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Ambient Light Strength", &cb_ps_light.data.ambientLightStrength, 0.01f, 0.0f, 1.0f);
	ImGui::DragInt("Shininess", &shininess, 1, 1, 32);
	ImGui::DragFloat("Sphere radius", &lightSphereRadius, 0.01f, 0.0f, 100.0f);
	ImGui::Checkbox("Show lights", &showLights);
	ImGui::Checkbox("Turn on blinn", &turnOnBlinn);
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

		ImGui::DragFloat3("Color", &dynamicLights[i].lightColor.x, 0.01f, 0.0f, 1.0f);
		ImGui::DragFloat("Strength", &dynamicLights[i].lightStrength, 0.1f, 0.0f, 100.0f);

		if (dynamicLights[i].type != LightType::Directional)
		{
			ImGui::DragFloat3("Position", &dynamicLights[i].lightPosition.x, 0.1f, -1000.0f, 1000.0f);
			ImGui::DragFloat("Attenuation a", &cb_ps_light.data.lights[i].attenuation_a, 0.01f, 0.01f, 100.0f);
			ImGui::DragFloat("Attenuation b", &cb_ps_light.data.lights[i].attenuation_b, 0.01f, 0.0f, 100.0f);
			ImGui::DragFloat("Attenuation c", &cb_ps_light.data.lights[i].attenuation_c, 0.01f, 0.0f, 100.0f);
		}
		if (dynamicLights[i].type != LightType::Point)
		{
			ImGui::DragFloat3("Light direction", &cb_ps_light.data.lights[i].direction.x, 0.01f, -100.0f, 100.0f);
		}
		if (dynamicLights[i].type == LightType::Spot)
		{
			ImGui::DragFloat("Inner angle", &cb_ps_light.data.lights[i].spotInnerAngle, 0.01f, -XM_2PI, XM_2PI);
			ImGui::DragFloat("Outer angle", &cb_ps_light.data.lights[i].spotOuterAngle, 0.01f, -XM_2PI, XM_2PI);
		}
		ImGui::Checkbox("Light on", &dynamicLights[i].lightOn);
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	swapchain->Present(0, NULL); //VSYNC ON -- 1, OFF -- 0
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

	spriteBatch->Begin();

	spriteFont->DrawString(
		spriteBatch.get(),
		StringHelper::StringToWide(coordsString).c_str(),
		DirectX::XMFLOAT2(0, screenY),
		DirectX::Colors::White
	);

	spriteBatch->End();
}

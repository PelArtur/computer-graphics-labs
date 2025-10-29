#include "Graphics.hpp"

#include <sstream>
#include <string>
#include <windows.h>
#include <iomanip>


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

		this->cb_ps_light.data.ambientLightColor = XMFLOAT3(1.0f, 1.0f, 1.0f);
		this->cb_ps_light.data.ambientLightStrength = 1.0f;

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

		std::vector<Vertex> vertices = {
			Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),

			Vertex(0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),

			Vertex(0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),

			Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f, -0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f,  0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),

			Vertex(-0.5f,  0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f,  0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f,  0.5f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),

			Vertex(-0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(-0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f),
			Vertex(0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f)
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

		//Initialize Model
		//if (!gameObject.Initialize("Data/Models/roblox_guy/source/RBLXTriGuard.fbx", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
		//	return false;
		//if (!gameObject.Initialize("Data/Models/sentinel/rq170.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
		//	return false;
		if (!plane.Initialize(vertices, indices, textures, XMMatrixIdentity(), this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;
		if (!sentinels.Initialize("Data/Models/sentinel/rq170.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;
		if (!sentinel1.Initialize("Data/Models/sentinel/rq170.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;
		if (!sentinel2.Initialize("Data/Models/sentinel/rq170.glb", this->device.Get(), this->deviceContext.Get(), cb_vertexShader))
			return false;

		//Sentinels instancing
		{
			const int size_x = 50;
			const int size_y = 50;
			const int size_z = 1;

			int start_x = -(size_x - 1) / 2;
			int end_x = (size_x - 1) / 2;

			int start_y = -size_y / 2;
			int end_y = size_y / 2 - 1;

			int start_z = -(size_z - 1) / 2;
			int end_z = (size_z - 1) / 2;
			std::vector<InstanceMatrixData> sentinelsInstanceMatrices;

			for (int x = start_x; x <= end_x; ++x)
			{
				for (int y = start_y; y <= end_y; ++y)
				{
					for (int z = start_z; z <= end_z; ++z)
					{
						InstanceMatrixData data;

						DirectX::XMVECTOR offset = DirectX::XMVectorSet(
							(float)x * 15.0f,
							(float)y * 10.0f - 200.0f,
							(float)z * 6.0f,
							1.0f
						);

						DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslationFromVector(offset);

						data.instanceMatrix = translationMatrix;
						sentinelsInstanceMatrices.push_back(data);
					}
				}
			}
			sentinels.SetInstanceData(sentinelsInstanceMatrices, this->device.Get());
			sentinels.SetRotation(0.09f, 0.0f, 0.0f);
		}

		//Plane instancing
		{
			std::vector<InstanceMatrixData> planeInstanceMatrix;
			InstanceMatrixData data;
			float scale_x = 1000.0f;
			float scale_y = 1.0f;
			float scale_z = 1000.0f;

			float loc_x = 0.0f;
			float loc_y = -5.0f;
			float loc_z = -500.0f;

			DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale_x, scale_y, scale_z);

			DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(loc_x, loc_y, loc_z);

			data.instanceMatrix = scaleMatrix * translationMatrix;
			planeInstanceMatrix.push_back(data);
			plane.SetInstanceData(planeInstanceMatrix, this->device.Get());
		}

		//Sentine1 instancing
		{
			std::vector<InstanceMatrixData> sentinel1InstanceMatrix;
			InstanceMatrixData data;

			float loc_x = 0.0f;
			float loc_y = 75.0f;
			float loc_z = -4.4f;

			data.instanceMatrix = DirectX::XMMatrixTranslation(loc_x, loc_y, loc_z);
			sentinel1InstanceMatrix.push_back(data);
			sentinel1.SetInstanceData(sentinel1InstanceMatrix, this->device.Get());
		}

		//Sentine2 instancing
		{
			std::vector<InstanceMatrixData> sentinel2InstanceMatrix;
			InstanceMatrixData data;
			float scale_x = 5.0f;
			float scale_y = 5.0f;
			float scale_z = 5.0f;

			float loc_x = 0.0f;
			float loc_y = -100.0f;
			float loc_z = 40.0f;

			DirectX::XMMATRIX scaleMatrix = DirectX::XMMatrixScaling(scale_x, scale_y, scale_z);

			DirectX::XMMATRIX translationMatrix = DirectX::XMMatrixTranslation(loc_x, loc_y, loc_z);

			data.instanceMatrix = scaleMatrix * translationMatrix;
			sentinel2InstanceMatrix.push_back(data);
			sentinel2.SetInstanceData(sentinel2InstanceMatrix, this->device.Get());
			sentinel2.SetRotation(0.0f, 0.0f, 0.5f);
		}

		camera.SetPosition(0.0f, 0.0f, -100.0f);
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
	this->cb_ps_light.ApplyChanges();
	this->deviceContext->PSSetConstantBuffers(0, 1, this->cb_ps_light.GetAddressOf());

	//float backgroundColor[] = { 0.0f, 1.0f, 1.0f, 1.0f };
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

	static float translationOffset[3] = { 0, 0, 0 };
	static float rotationOffset[3] = { 0, 0, 0 };
	static float alpha = 1.0f;
	{
		//this->plane.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
		//this->sentinels.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());

		this->sentinel1.SetPosition(translationOffset[0], translationOffset[1], translationOffset[2]);
		this->sentinel1.SetRotation(rotationOffset[0] + 0.09f, rotationOffset[1], rotationOffset[2]);
		this->sentinel1.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
		//this->sentinel2.Draw(camera.GetViewMatrix() * camera.GetProjectionMatrix());
	}

	//FPS counter
	ShowFPSstats();

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	
	ImGui::Begin("Model");
	ImGui::DragFloat3("coords", translationOffset, 1.0f, -1000.0f, 1000.0f);
	ImGui::DragFloat3("rotation", rotationOffset, 0.01f, -XM_2PI, XM_2PI);
	ImGui::DragFloat3("Ambient Light Color", &this->cb_ps_light.data.ambientLightColor.x, 0.01f, 0.0f, 1.0f);
	ImGui::DragFloat("Ambient Light Strength", &this->cb_ps_light.data.ambientLightStrength, 0.01f, 0.0f, 1.0f);
	ImGui::End();
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

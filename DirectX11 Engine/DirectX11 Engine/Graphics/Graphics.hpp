#pragma once
#include "AdapterReader.hpp"
#include "Shaders.hpp"


class Graphics
{
public:
	bool Initialize(HWND hwnd, int width, int height);
	void RenderFrame();

private:
	bool InitializeDirectX(HWND hwnd, int width, int height);
	bool InitializeShaders();

	Microsoft::WRL::ComPtr<ID3D11Device> device;                       //buffers
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;         //shader resource for shaders
	Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;                  //swapping frames 
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;

	VertexShader vertexShader;
	PixelShader pixelShader;
};
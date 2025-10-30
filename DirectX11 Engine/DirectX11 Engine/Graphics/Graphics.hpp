#pragma once  
#include "AdapterReader.hpp"  
#include "Shaders.hpp"  
#include "Camera.hpp"
#include "RenderableGameObject.hpp"
#include "Light.hpp"
#include "../Timer.hpp"

//fonts 
#include <SpriteBatch.h>
#include <SpriteFont.h>
//textures
#include <WICTextureLoader.h>
//ImGUI
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_impl_dx11.h"


class Graphics  
{  
public:  
    bool Initialize(HWND hwnd, int width, int height);  
    void RenderFrame();
    Camera camera;
    RenderableGameObject sentinels;
    RenderableGameObject sentinel1;
    RenderableGameObject sentinel2;
    RenderableGameObject plane;
    Light light;

private:  
    bool InitializeDirectX(HWND hwnd);  
    bool InitializeShaders();  
    bool InitializeScene();
    void ShowFPSstats();
 
    Microsoft::WRL::ComPtr<ID3D11Device> device;                       //buffers  
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;         //shader resource for shaders  
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;                  //swapping frames   
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;  

    VertexShader vertexShader;  

    PixelShader pixelShader;
    PixelShader pixelShader_nolight;
    PixelShader voronoiseShader;
    PixelShader warpShader;
    
    ConstantBuffer<CB_VS_vertexShader> cb_vertexShader;
    ConstantBuffer<CB_PS_light> cb_ps_light;
    ConstantBuffer<Voronoise_pixelShader> psConstantBuffer;
    ConstantBuffer<Warp_pixelShader> warpConstantBuffer;

    DirectX::XMFLOAT4 mouseData = { 0.0f, 0.0f, 0.0f, 0.0f };

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;  
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;  
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;  

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;

    std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> spriteFont;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture2;

    int windowWidth = 0;
    int windowHeight = 0;
    Timer fpsTimer;
    Timer shadersTimer;
};

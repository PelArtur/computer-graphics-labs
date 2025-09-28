#pragma once  
#include "AdapterReader.hpp"  
#include "Shaders.hpp"  
#include "Vertex.hpp"
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "ConstantBuffer.hpp"
#include "Camera.hpp"
#include "../Timer.hpp"

//fonts 
#include <SpriteBatch.h>
#include <SpriteFont.h>
//textures
#include <WICTextureLoader.h>


class Graphics  
{  
public:  
    bool Initialize(HWND hwnd, int width, int height);  
    void RenderFrame();
    Camera camera;

private:  
    bool InitializeDirectX(HWND hwnd);  
    bool InitializeShaders();  
    bool InitializeScene();  

    Microsoft::WRL::ComPtr<ID3D11Device> device;                       //buffers  
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;         //shader resource for shaders  
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;                  //swapping frames   
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;  

    VertexShader vertexShader;  
    PixelShader pixelShader;  
    
    VertexBuffer<Vertex> vertexBuffer;
    IndexBuffer indicesBuffer;
    ConstantBuffer<CB_VS_vertexShader> constantBuffer;

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;  
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;  
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;  

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;

    std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> spriteFont;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture;

    int windowWidth = 0;
    int windowHeight = 0;
    Timer fpsTimer;
};
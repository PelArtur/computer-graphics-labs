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

#define MAX_SHADOWS 4

class Graphics  
{  
public:  
    bool Initialize(HWND hwnd, int width, int height);  
    void RenderFrame();
    Camera camera;
    std::vector<RenderableGameObject> skulls;
    RenderableGameObject plane;
    FullscreenQuad fullscreenQuad;
    std::vector<Light> dynamicLights;
    std::vector<RenderableGameObject> skybox;

private:  
    bool InitializeDirectX(HWND hwnd);  
    bool InitializeShaders();  
    bool InitializeScene();
    bool InitializeHDRResources();
    bool InitializeShadowResources();
    void RenderSkybox();
    void MainRenderPass();
    void ShadowPass();
    void ToneMappingPass();
    void ImGUIPass();
    
    void ShowFPSstats();
    void ShowCoords(const std::string& objectName, const DirectX::XMVECTOR& position, float screenY);
 
    Microsoft::WRL::ComPtr<ID3D11Device> device;                       //buffers  
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> deviceContext;         //shader resource for shaders  
    Microsoft::WRL::ComPtr<IDXGISwapChain> swapchain;                  //swapping frames   
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> renderTargetView;  

    VertexShader vertexShader;
    VertexShader fullscreenVS;

    PixelShader pixelShader;
    PixelShader pixelShader_nolight;
    PixelShader pixelShader_noComparisonSampler;
    PixelShader tonemapPS;
    PixelShader voronoiseShader;
    PixelShader warpShader;
    
    ConstantBuffer<CB_VS_vertexShader> cb_vertexShader;
    ConstantBuffer<CB_PS_LightsData> cb_ps_light;
    ConstantBuffer<CB_PS_LightColor> cb_ps_lightModelColor;
    ConstantBuffer<TonemapParams> cbTonemap;
    ConstantBuffer<Voronoise_pixelShader> psConstantBuffer;
    ConstantBuffer<Warp_pixelShader> warpConstantBuffer;

    DirectX::XMFLOAT4 mouseData = { 0.0f, 0.0f, 0.0f, 0.0f };

    Microsoft::WRL::ComPtr<ID3D11DepthStencilView> depthStencilView;  
    Microsoft::WRL::ComPtr<ID3D11Texture2D> depthStencilBuffer;  
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> depthStencilState;  

    Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizerState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> fullscreenRS;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> shadowRS;
    Microsoft::WRL::ComPtr<ID3D11BlendState> blendState;
    D3D11_VIEWPORT mainViewport;
    D3D11_VIEWPORT shadowViewport;

    std::unique_ptr<DirectX::SpriteBatch> spriteBatch;
    std::unique_ptr<DirectX::SpriteFont> spriteFont;

    Microsoft::WRL::ComPtr<ID3D11SamplerState> samplerState;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> shadowSamplerState;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> myTexture2;

    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> skyboxTextureSRV;
    Microsoft::WRL::ComPtr<ID3D11SamplerState> skyboxSamplerState;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilState> skyboxDepthState;
    Microsoft::WRL::ComPtr<ID3D11RasterizerState> skyboxRasterizerState;

    Microsoft::WRL::ComPtr<ID3D11Texture2D> hdrTexture;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> hdrRTV;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> hdrSRV;

    std::vector<Microsoft::WRL::ComPtr<ID3D11DepthStencilView>> shadowDSVs;
    Microsoft::WRL::ComPtr<ID3D11Texture2D> shadowTextureArray;
    Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> shadowSRVArray;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView> shadowRTV;

    int windowWidth = 0;
    int windowHeight = 0;
    Timer fpsTimer;
    Timer shadersTimer;

    const UINT SHADOW_MAP_WIDTH = 2048;
    const UINT SHADOW_MAP_HEIGHT = 2048;
};

// Pipeline.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <fstream>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")


class Render
{
private:
    struct RenderDevice
    {
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* deviceContext = nullptr;
        IDXGISwapChain* swapChain = nullptr;
        ID3D11RenderTargetView* renderTargetView = nullptr;

    } renderDevice;

    struct Pipeline
    {
        ID3D11VertexShader* vertexShader = nullptr;
        ID3D11PixelShader* pixelShader = nullptr;

    } pipeline;


public:
    Render()
    {

    }

    uint32_t m_Width { 1200 };
    uint32_t m_Height { 820 };
    uint32_t m_FrameCount { 2 };

    void Initialize(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = m_FrameCount;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hwnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = true;
        scd.BufferDesc.Width = m_Width;
        scd.BufferDesc.Height = m_Height;

        D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
        D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &scd, &renderDevice.swapChain, &renderDevice.device, nullptr, &renderDevice.deviceContext);


        ID3D11Texture2D* backBuffer;
        renderDevice.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        auto result = renderDevice.device->CreateRenderTargetView(backBuffer, nullptr, &renderDevice.renderTargetView);
        backBuffer->Release();

        renderDevice.deviceContext->OMSetRenderTargets(1, &renderDevice.renderTargetView, nullptr);

        CreateShaders();

    }
    void Loop()
    {
        // Renderizado básico
        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        renderDevice.deviceContext->ClearRenderTargetView(renderDevice.renderTargetView, color);

        // Viewport
        D3D11_VIEWPORT view = { 0, 0, m_Width, m_Height, 0.0f, 1.0f };
        renderDevice.deviceContext->RSSetViewports(1, &view);


        renderDevice.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderDevice.deviceContext->VSSetShader(pipeline.vertexShader, nullptr, 0);
        renderDevice.deviceContext->PSSetShader(pipeline.pixelShader, nullptr, 0);
        renderDevice.deviceContext->Draw(3, 0);

        renderDevice.swapChain->Present(1, 0);
    }




    // Función para compilar y crear shaders
    bool CreateShaders()
    {
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;


        CompileShaderFromFile(L"../../Assets/Shaders/ClearScreen/VertexShader.hlsl", "VS", "vs_5_0", &vsBlob);
        CompileShaderFromFile(L"../../Assets/Shaders/ClearScreen/PixelShader.hlsl", "PS", "ps_5_0", &psBlob);


        renderDevice.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &pipeline.vertexShader);
        renderDevice.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pipeline.pixelShader);
        //renderDevice.device->CreateInputLayout(NULL, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pipeline.inputLayout);

        vsBlob->Release();
        psBlob->Release();

        return true;
    }


    HRESULT CompileShaderFromFile(const wchar_t* filename, const char* entryPoint, const char* profile, ID3DBlob** blob)
    {
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(
            filename,         
            nullptr,          
            nullptr,          
            entryPoint,       
            profile,          
            0,                
            0,                
            blob,             
            &errorBlob        
        );

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                std::cerr << "Shader Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                errorBlob->Release();
            }
            else
            {
                std::cerr << "Fallo al compilar el shader. No se encontró el archivo o hay errores." << std::endl;
            }
            return hr;
        }

        return S_OK;
    }


    void Cleanup()
    {
        if (pipeline.vertexShader) 
            pipeline.vertexShader->Release();

        if (pipeline.pixelShader)
            pipeline.pixelShader->Release();

        if (renderDevice.swapChain)
            renderDevice.swapChain->Release();

        if (renderDevice.renderTargetView)
            renderDevice.renderTargetView->Release();

        if (renderDevice.deviceContext)
            renderDevice.deviceContext->Release();

        if (renderDevice.device)
            renderDevice.device->Release();
    }

};


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main()
{
    Render render = {};

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const wchar_t CLASS_NAME[] = L"DX11 Pipeline";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DX11 Pipeline",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, render.m_Width, render.m_Height,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        std::cerr << "Error:\n";
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    render.Initialize(hwnd);


    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        render.Loop();
    }

    render.Cleanup();
    return 0;
}
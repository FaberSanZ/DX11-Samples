// ClearScreen.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <windows.h>
#include <iostream>
#include <d3d11.h>

#pragma comment(lib, "d3d11.lib")


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

public:
    Render()
    {

    }

    uint32_t m_Width{ 1200 };
    uint32_t m_Height{ 820 };
    uint32_t m_FrameCount { 2 };

    void Init(HWND hwnd)
    {
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = m_FrameCount;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hwnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = TRUE;
        scd.BufferDesc.Width = m_Width;
        scd.BufferDesc.Height = m_Height;

        D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
        D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &scd, &renderDevice.swapChain, &renderDevice.device, nullptr, &renderDevice.deviceContext);


        ID3D11Texture2D* backBuffer;
        renderDevice.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        auto result = renderDevice.device->CreateRenderTargetView(backBuffer, nullptr, &renderDevice.renderTargetView);
        backBuffer->Release();
    }
    void Loop()
    {
        // Renderizado básico
        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        renderDevice.deviceContext->ClearRenderTargetView(renderDevice.renderTargetView, color);
        renderDevice.swapChain->Present(1, 0);
    }


    void Cleanup()
    {
        if (renderDevice.swapChain) renderDevice.swapChain->Release();
        if (renderDevice.renderTargetView) renderDevice.renderTargetView->Release();
        if (renderDevice.deviceContext) renderDevice.deviceContext->Release();
        if (renderDevice.device) renderDevice.device->Release();
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
    Render m_render = {};

    HINSTANCE hInstance = GetModuleHandle(nullptr);
    const wchar_t CLASS_NAME[] = L"DX11 ClearScreen";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DX11 ClearScreen",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_render.m_Width, m_render.m_Height,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwnd)
    {
        std::cerr << "Error:\n";
        return -1;
    }

    ShowWindow(hwnd, SW_SHOW);

    m_render.Init(hwnd);


    MSG msg = {};
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        m_render.Loop();
    }

    m_render.Cleanup();
    return 0;
}



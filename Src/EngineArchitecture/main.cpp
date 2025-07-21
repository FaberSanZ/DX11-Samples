// main.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <fstream>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "Graphics/Adapter.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/CommandList.h"
#include "Graphics/Buffer.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "D3DCompiler.lib")


class Render
{
private:

    // Render device: contains the Direct3D device, context, swap chain, etc..
    struct RenderDevice
    {
        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* deviceContext = nullptr;
        IDXGISwapChain* swapChain = nullptr;
        ID3D11RenderTargetView* renderTargetView = nullptr;
        ID3D11DepthStencilView* depthStencilView = nullptr;
        ID3D11Texture2D* depthStencilBuffer = nullptr;

    } renderDevice;

    // Pipeline: contains shaders, input layout, and depth stencil state
    struct Pipeline
    {
        ID3D11VertexShader* vertexShader = nullptr;
        ID3D11PixelShader* pixelShader = nullptr;
        ID3D11InputLayout* inputLayout = nullptr;
        ID3D11DepthStencilState* depthStencilState = nullptr;
        ID3D11RasterizerState* rasterState = nullptr;


    } pipeline;







    // Vertex structure: defines the layout of a vertex
    struct Vertex
    {
        DirectX::XMFLOAT4 position; // Position of the vertex in 3D space (x, y, z, w)
        DirectX::XMFLOAT4 color;    // Color of the vertex (r, g, b, a)
    };


    // Define a struct for the camera matrices (must match HLSL cbuffer layout)
    struct CameraBuffer
    {
        DirectX::XMMATRIX word;
        DirectX::XMMATRIX view;
        DirectX::XMMATRIX projection;
    } cameraData;

    float m_CubeRotation = 0.0f; // Rotation angle for the cube (tools for game)

public:
    Render()
    {
    }

    uint32_t m_Width { 1200 }; // Width of the render target
    uint32_t m_Height { 820 }; // Height of the render target


    Graphics::Adapter adapter;
    Graphics::Device device;
    Graphics::SwapChain swapChain;
    Graphics::CommandList list;
    Graphics::Buffer vertexBuffer;
    Graphics::Buffer indexBuffer;
    Graphics::Buffer constantBuffer;

    void Initialize(HWND hwnd)
    {

		adapter.Initialize(0); // Initialize the first GPU adapter (0)
		device.Initialize(adapter); // Initialize the Direct3D device using the adapter
		swapChain.Initialize(device, hwnd, m_Width, m_Height); // Initialize the swap chain with the device and window handle
		list.Initialize(device.GetContext()); // Initialize the command list with the device context




		renderDevice.device = device.GetDevice();
        renderDevice.swapChain = swapChain.GetSwapChain();
		renderDevice.deviceContext = device.GetContext();
		renderDevice.renderTargetView = swapChain.GetRenderTargetView();
		renderDevice.depthStencilView = swapChain.GetDepthStencilView();
		renderDevice.depthStencilBuffer = swapChain.GetDepthBuffer();


        D3D11_RASTERIZER_DESC rasterDesc = {};
        rasterDesc.FillMode = D3D11_FILL_SOLID;
        rasterDesc.CullMode = D3D11_CULL_NONE; // No culling
        rasterDesc.FrontCounterClockwise = FALSE;

        renderDevice.device->CreateRasterizerState(&rasterDesc, &pipeline.rasterState);
        renderDevice.deviceContext->RSSetState(pipeline.rasterState);



        D3D11_DEPTH_STENCIL_DESC depthStencilDesc = {};
        depthStencilDesc.DepthEnable = true;
        depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
        depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;
        depthStencilDesc.StencilEnable = false;
        depthStencilDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
        renderDevice.device->CreateDepthStencilState(&depthStencilDesc, &pipeline.depthStencilState);





        renderDevice.deviceContext->OMSetRenderTargets(1, &renderDevice.renderTargetView, renderDevice.depthStencilView);
        renderDevice.deviceContext->OMSetDepthStencilState(pipeline.depthStencilState, 1);



        CreateCamera();
        CreateShaders();
        CreateMesh();

        std::wcout << L"GPU: " << adapter.GetGpuName() << std::endl;
        std::wcout << L"Dedicated Video Memory: " << adapter.GetDedicatedVideoMemory() / (1024 * 1024) << L" MB" << std::endl;
        std::wcout << L"Dedicated System Memory: " << adapter.GetDedicatedSystemMemory() / (1024 * 1024) << L" MB" << std::endl;
        std::wcout << L"Shared System Memory: " << adapter.GetSharedSystemMemory() / (1024 * 1024) << L" MB" << std::endl;
    }


    void Loop()
    {
        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };


        list.Clear(renderDevice.renderTargetView, renderDevice.depthStencilView, color);

		list.SetViewport(m_Width, m_Height);

        renderDevice.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderDevice.deviceContext->IASetInputLayout(pipeline.inputLayout);

        //renderDevice.deviceContext->VSSetConstantBuffers(constantBuffer.slot, 1, &constantBuffer.buffer);
        renderDevice.deviceContext->VSSetShader(pipeline.vertexShader, nullptr, 0);
        renderDevice.deviceContext->PSSetShader(pipeline.pixelShader, nullptr, 0);
        renderDevice.deviceContext->DrawIndexed(36, 0, 0);

        renderDevice.swapChain->Present(1, 0);
    }


    bool CreateMesh()
    {
        Vertex vertices[] =
        {
            // Front face
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Right side face
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Left side face
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Back face
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Top face
            {{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{ 0.5f,  0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f,  0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f,  0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},

            // Bottom face
            {{ 0.5f, -0.5f,  0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            {{-0.5f, -0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 1.0f, 1.0f}},
            {{ 0.5f, -0.5f, -0.5f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
            {{-0.5f, -0.5f,  0.5f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
        };


        vertexBuffer.Initialize(device, Graphics::BufferType::VertexBuffer, vertices, sizeof(vertices), sizeof(Vertex));
        vertexBuffer.Bind(device.GetContext()); 



        uint32_t indices[] =
        {
            // front face
            0, 1, 2, // first triangle
            0, 3, 1, // second triangle

            // left face
            4, 5, 6, // first triangle
            4, 7, 5, // second triangle

            // right face
            8, 9, 10, // first triangle
            8, 11, 9, // second triangle

            // back face
            12, 13, 14, // first triangle
            12, 15, 13, // second triangle

            // top face
            16, 17, 18, // first triangle
            16, 19, 17, // second triangle

            // bottom face
            20, 21, 22, // first triangle
            20, 23, 21, // second triangle
        };



        indexBuffer.Initialize(device, Graphics::BufferType::IndexBuffer, indices, sizeof(indices));
        indexBuffer.Bind(device.GetContext()); 


        return true;
    }




    void CreateCamera()
    {
        DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH({ 0, 0, -3 }, { 0, 0, 0 }, { 0, 1, 0 });

        // Set up projection matrix (perspective)
        float fov = 45.0f * (3.14f / 180.0f);
        float aspect = static_cast<float>(m_Width) / static_cast<float>(m_Height);
        float nearZ = 0.1f;
        float farZ = 1000.0f;
        DirectX::XMMATRIX projection = DirectX::XMMatrixPerspectiveFovLH(fov, aspect, nearZ, farZ);

        // Transpose matrices for HLSL (row-major in C++, column-major in HLSL)
        cameraData.word = DirectX::XMMatrixIdentity();
        cameraData.view = DirectX::XMMatrixTranspose(view);
        cameraData.projection = DirectX::XMMatrixTranspose(projection);



        constantBuffer.Initialize(device, Graphics::BufferType::ConstantBuffer, &cameraData, sizeof(CameraBuffer));
        constantBuffer.Bind(device.GetContext(), 0);        // ConstantBuffer: slot 0, stage VS

    }



    void UpdateCamera()
    {
        
        m_CubeRotation += 0.01f;

		// Update camera matrices
        cameraData.word = XMMatrixTranspose(DirectX::XMMatrixRotationRollPitchYaw(m_CubeRotation, m_CubeRotation, m_CubeRotation));

        // Update GPU
		constantBuffer.Update(renderDevice.deviceContext, &cameraData, sizeof(CameraBuffer));

    }



    bool CreateShaders()
    {
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;


        CompileShaderFromFile(L"../../../../Assets/Shaders/EngineArchitecture/VertexShader.hlsl", "VS", "vs_5_0", &vsBlob);
        CompileShaderFromFile(L"../../../../Assets/Shaders/EngineArchitecture/PixelShader.hlsl", "PS", "ps_5_0", &psBlob);



        renderDevice.device->CreateVertexShader(vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), nullptr, &pipeline.vertexShader);
        renderDevice.device->CreatePixelShader(psBlob->GetBufferPointer(), psBlob->GetBufferSize(), nullptr, &pipeline.pixelShader);

        D3D11_INPUT_ELEMENT_DESC layout[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }
        };

        renderDevice.device->CreateInputLayout(layout, 2, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &pipeline.inputLayout);

        vsBlob->Release();
        psBlob->Release();

        return true;
    }


    HRESULT CompileShaderFromFile(const wchar_t* filename, const char* entryPoint, const char* profile, ID3DBlob** blob)
    {
        ID3DBlob* errorBlob = nullptr;
        HRESULT hr = D3DCompileFromFile(filename, nullptr, nullptr, entryPoint, profile, 0, 0, blob, &errorBlob);

        if (FAILED(hr))
        {
            if (errorBlob)
            {
                std::cerr << "Shader Error: " << (char*)errorBlob->GetBufferPointer() << std::endl;
                errorBlob->Release();
            }
            else
            {
                std::cerr << "Error." << std::endl;
            }
            return hr;
        }

        return S_OK;
    }


    void Cleanup()
    {
        //if (constantBuffer.data)
        //    constantBuffer.data = nullptr;

        //if (constantBuffer.buffer)
        //    constantBuffer.buffer->Release();

        if (pipeline.rasterState)
            pipeline.rasterState->Release();

        if (pipeline.depthStencilState)
            pipeline.depthStencilState->Release();

        if (renderDevice.depthStencilView)
            renderDevice.depthStencilView->Release();

        if (renderDevice.depthStencilBuffer)
            renderDevice.depthStencilBuffer->Release();

        //if (indexBuffer.buffer)
        //    indexBuffer.buffer->Release();

        //if (vertexBuffer.buffer)
        //    vertexBuffer.buffer->Release();

        if (pipeline.inputLayout)
            pipeline.inputLayout->Release();

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
    const wchar_t CLASS_NAME[] = L"DX11 EngineArchitecture";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DX11 EngineArchitecture",
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

    // Loop until there is a quit message from the window or the user.
    bool done { false };



    while (!done)
    {
        // Handle the windows messages.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        // If windows signals to end the application then exit out.
        if (msg.message == WM_QUIT)
        {
            done = true;
        }
        else
        {
            // Otherwise do the frame processing.
            render.Loop();
            render.UpdateCamera(); // Update camera matrices each frame
        }

    }


    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);

    }

    render.Cleanup();
    return 0;
}

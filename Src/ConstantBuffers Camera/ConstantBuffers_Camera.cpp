// ConstantBuffers Camera.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <windows.h>
#include <fstream>
#include <iostream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>


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

	// Vertex buffer: used for drawing vertices
    struct VertexBuffer
    {
        ID3D11Buffer* buffer = nullptr;
		uint32_t stride = { sizeof(float) * 8 };    // Stride of the vertex buffer (size of each vertex in bytes)
		uint32_t offset = { 0 };                    // Offset in bytes from the start of the vertex buffer to the first vertex to be used

    } vertexBuffer;

	// Index buffer: used for indexed drawing
    struct IndexBuffer
    {
        ID3D11Buffer* buffer = nullptr;
		uint32_t count;             // Number of indices in the index buffer
		uint32_t offset = { 0 };    // Offset in bytes from the start of the index buffer to the first index to be used

    } indexBuffer;


    // Camera constant buffer: word, view, projection matrices
    struct ConstantBuffer
    {
        ID3D11Buffer* buffer = nullptr; 
		uint32_t slot;      // Slot for binding the constant buffer
		uint32_t size;      // Size of the constant buffer in bytes
		uint32_t stride;    // Stride of the constant buffer (usually 16 bytes for matrices)
		void* data;         // Pointer to the data that will be uploaded to the constant buffer

	} constantBuffer; 

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
	uint32_t m_FrameCount { 2 }; // Number of frames in the swap chain

    void Initialize(HWND hwnd)
    {
		// Initialize the render device
        DXGI_SWAP_CHAIN_DESC scd = {};
        scd.BufferCount = m_FrameCount;
        scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        scd.OutputWindow = hwnd;
        scd.SampleDesc.Count = 1;
        scd.Windowed = true;
        scd.BufferDesc.Width = m_Width;
        scd.BufferDesc.Height = m_Height;

		// Create the Direct3D device and swap chain
        D3D_DRIVER_TYPE type = D3D_DRIVER_TYPE_HARDWARE;
        D3D11CreateDeviceAndSwapChain(nullptr, type, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &scd, &renderDevice.swapChain, &renderDevice.device, nullptr, &renderDevice.deviceContext);

		// Create a render target view for the back buffer
        ID3D11Texture2D* backBuffer;
        renderDevice.swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&backBuffer);
        auto result = renderDevice.device->CreateRenderTargetView(backBuffer, nullptr, &renderDevice.renderTargetView);
        backBuffer->Release();



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


        // Initialize and set up the description of the depth buffer.
        D3D11_TEXTURE2D_DESC depthbufferDesc = {};
        depthbufferDesc.Width = m_Width;
        depthbufferDesc.Height = m_Height;
        depthbufferDesc.MipLevels = 1;
        depthbufferDesc.ArraySize = 1;
        depthbufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        depthbufferDesc.SampleDesc.Count = 1;
        depthbufferDesc.SampleDesc.Quality = 0;
        depthbufferDesc.Usage = D3D11_USAGE_DEFAULT;
        depthbufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        depthbufferDesc.CPUAccessFlags = 0;
        depthbufferDesc.MiscFlags = 0;

        renderDevice.device->CreateTexture2D(&depthbufferDesc, NULL, &renderDevice.depthStencilBuffer);
        renderDevice.device->CreateDepthStencilView(renderDevice.depthStencilBuffer, NULL, &renderDevice.depthStencilView);


        renderDevice.deviceContext->OMSetRenderTargets(1, &renderDevice.renderTargetView, renderDevice.depthStencilView);
        renderDevice.deviceContext->OMSetDepthStencilState(pipeline.depthStencilState, 1);

        CreateCamera();
        CreateShaders();
        CreateMesh();
		CreateConstantBuffer();
    }


    void Loop()
    {
        float color[4] = { 0.0f, 0.2f, 0.4f, 1.0f };
        renderDevice.deviceContext->ClearRenderTargetView(renderDevice.renderTargetView, color);
        renderDevice.deviceContext->ClearDepthStencilView(renderDevice.depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

        // Viewport
        D3D11_VIEWPORT view = { };

        // Fill out the Viewport
        view.TopLeftX = 0;
        view.TopLeftY = 0;
        view.Width = m_Width;
        view.Height = m_Height;
        view.MinDepth = 0.0f;
        view.MaxDepth = 1.0f;

        renderDevice.deviceContext->RSSetViewports(1, &view);

        renderDevice.deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer.buffer, &vertexBuffer.stride, &vertexBuffer.offset);
        renderDevice.deviceContext->IASetIndexBuffer(indexBuffer.buffer, DXGI_FORMAT_R32_UINT, indexBuffer.offset);

        renderDevice.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        renderDevice.deviceContext->IASetInputLayout(pipeline.inputLayout);

        renderDevice.deviceContext->VSSetConstantBuffers(constantBuffer.slot, 1, &constantBuffer.buffer);
        renderDevice.deviceContext->VSSetShader(pipeline.vertexShader, nullptr, 0);
        renderDevice.deviceContext->PSSetShader(pipeline.pixelShader, nullptr, 0);
        renderDevice.deviceContext->DrawIndexed(indexBuffer.count, 0, 0);

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

        D3D11_BUFFER_DESC vertexBufferDesc = {};
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(vertices);
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

        // Fill in the subresource data.
        D3D11_SUBRESOURCE_DATA initData;
        initData.pSysMem = vertices;
        initData.SysMemPitch = 0;
        initData.SysMemSlicePitch = 0;

        if (FAILED(renderDevice.device->CreateBuffer(&vertexBufferDesc, &initData, &vertexBuffer.buffer)))
        {
            std::cerr << "Error\n";
            return false;
        }




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

        indexBuffer.count = _countof(indices);

        D3D11_BUFFER_DESC indexBufferDesc = {};
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(indices);
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

        // Fill in the subresource data.
        D3D11_SUBRESOURCE_DATA initDataIdices;
        initDataIdices.pSysMem = indices;
        initDataIdices.SysMemPitch = 0;
        initDataIdices.SysMemSlicePitch = 0;

        if (FAILED(renderDevice.device->CreateBuffer(&indexBufferDesc, &initDataIdices, &indexBuffer.buffer)))
        {
            std::cerr << "Error\n";
            return false;
        }


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

        constantBuffer.slot = 0; // Usually slot 0 for camera (hlsl cbuffer 0)
        constantBuffer.size = sizeof(CameraBuffer);
        constantBuffer.stride = sizeof(CameraBuffer);
        constantBuffer.data = &cameraData;
    }

    // Create the constant buffer for camera data
	void CreateConstantBuffer() 
    {
        // Describe the constant buffer
        D3D11_BUFFER_DESC cbDesc = {};
        cbDesc.Usage = D3D11_USAGE_DEFAULT;
        cbDesc.ByteWidth = sizeof(CameraBuffer);
        cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        cbDesc.CPUAccessFlags = 0;

        // Provide the initial data
        D3D11_SUBRESOURCE_DATA initData = {};
        initData.pSysMem = &cameraData;


        // Create the buffer
        renderDevice.device->CreateBuffer(&cbDesc, &initData, &constantBuffer.buffer);

	}

    void UpdateCamera()
    {
        m_CubeRotation += 0.01f;

        DirectX::XMMATRIX world = DirectX::XMMatrixRotationRollPitchYaw(m_CubeRotation, m_CubeRotation, m_CubeRotation);;

        cameraData.word = XMMatrixTranspose(world);

        // Update GPU
        renderDevice.deviceContext->UpdateSubresource(constantBuffer.buffer, 0, nullptr, &cameraData, 0, 0);
    }



    bool CreateShaders()
    {
        ID3DBlob* vsBlob = nullptr;
        ID3DBlob* psBlob = nullptr;


        CompileShaderFromFile(L"../../Assets/Shaders/ConstantBuffersCamera/VertexShader.hlsl", "VS", "vs_5_0", &vsBlob);
        CompileShaderFromFile(L"../../Assets/Shaders/ConstantBuffersCamera/PixelShader.hlsl", "PS", "ps_5_0", &psBlob);


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

        if (pipeline.depthStencilState)
            pipeline.depthStencilState->Release();

        if (renderDevice.depthStencilView)
            renderDevice.depthStencilView->Release();

        if (renderDevice.depthStencilBuffer)
            renderDevice.depthStencilBuffer->Release();

        if (indexBuffer.buffer)
            indexBuffer.buffer->Release();

        if (vertexBuffer.buffer)
            vertexBuffer.buffer->Release();

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
    const wchar_t CLASS_NAME[] = L"DX11 ConstantBuffers Camera";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"DX11 ConstantBuffers Camera",
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
    bool done {false};



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



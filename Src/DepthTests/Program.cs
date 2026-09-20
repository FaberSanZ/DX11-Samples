using System.Drawing;
using System.IO;
using System.Numerics;
using System.Runtime.CompilerServices;
using Vortice.D3DCompiler;
using Vortice.Direct3D;
using Vortice.Direct3D11;
using Vortice.DXGI;
using Vortice.Mathematics;
using Vultaik;

using static Vortice.Direct3D11.D3D11;
using static Vortice.DXGI.DXGI;

internal sealed class RenderSystem : IDisposable
{
    private ID3D11Device? _device;
    private ID3D11DeviceContext? _deviceContext;
    private IDXGIFactory2? _factory;
    private IDXGISwapChain1? _swapChain;
    private ID3D11Texture2D? _backBuffer;
    private ID3D11RenderTargetView? _renderTargetView;

    private ID3D11Texture2D? _depthStencilBuffer;
    private ID3D11DepthStencilView? _depthStencilView;
    private ID3D11DepthStencilState? _depthStencilState;

    private ID3D11VertexShader? _vertexShader;
    private ID3D11PixelShader? _pixelShader;
    private ID3D11InputLayout? _inputLayout;
    private ID3D11Buffer? _vertexBuffer;
    private ID3D11Buffer? _indexBuffer;

    private uint _indexCount;

    public uint Width { get; } = 1200;
    public uint Height { get; } = 820;
    public uint FrameCount { get; } = 2;

    private struct Vertex
    {
        public Vector4 Position;
        public Vector4 Color;

        public Vertex(Vector4 position, Vector4 color)
        {
            Position = position;
            Color = color;
        }
    }

    public void Initialize(nint hwnd)
    {
        FeatureLevel[] featureLevels =
        [
            FeatureLevel.Level_11_1,
            FeatureLevel.Level_11_0
        ];

        D3D11CreateDevice(IntPtr.Zero, DriverType.Hardware, DeviceCreationFlags.None, featureLevels, out _device, out FeatureLevel featureLevel, out _deviceContext);

        _factory = CreateDXGIFactory1<IDXGIFactory2>();

        SwapChainDescription1 swapChainDescription = new()
        {
            Width = Width,
            Height = Height,
            Format = Format.R8G8B8A8_UNorm,
            BufferCount = FrameCount,
            BufferUsage = Usage.RenderTargetOutput,
            SampleDescription = SampleDescription.Default,
            Scaling = Scaling.Stretch,
            SwapEffect = SwapEffect.FlipDiscard,
            AlphaMode = AlphaMode.Ignore
        };

        SwapChainFullscreenDescription fullscreenDescription = new()
        {
            Windowed = true
        };

        _swapChain = _factory.CreateSwapChainForHwnd(_device, hwnd, swapChainDescription, fullscreenDescription);

        _factory.MakeWindowAssociation(hwnd, WindowAssociationFlags.IgnoreAltEnter);

        _backBuffer = _swapChain.GetBuffer<ID3D11Texture2D>(0);
        _renderTargetView = _device.CreateRenderTargetView(_backBuffer);

        CreateDepthBuffer();
        CreateShaders();
        CreateMesh();
    }

    private void CreateDepthBuffer()
    {
        DepthStencilDescription depthStencilDescription = new(
            true,
            DepthWriteMask.All,
            ComparisonFunction.Less
        );

        _depthStencilState = _device!.CreateDepthStencilState(depthStencilDescription);

        _depthStencilBuffer = _device.CreateTexture2D(
            Format.D24_UNorm_S8_UInt,
            Width,
            Height,
            mipLevels: 1,
            bindFlags: BindFlags.DepthStencil
        );

        _depthStencilView = _device.CreateDepthStencilView(_depthStencilBuffer);

        _deviceContext!.OMSetDepthStencilState(_depthStencilState, 1);
    }

    private void CreateShaders()
    {
        ReadOnlyMemory<byte> vertexShaderByteCode = Compiler.CompileFromFile("Vertex.hlsl", "VS", "vs_5_0");
        ReadOnlyMemory<byte> pixelShaderByteCode = Compiler.CompileFromFile("Pixel.hlsl", "PS", "ps_5_0");

        _vertexShader = _device!.CreateVertexShader(vertexShaderByteCode.Span);
        _pixelShader = _device.CreatePixelShader(pixelShaderByteCode.Span);

        InputElementDescription[] inputElements =
        [
            new("POSITION", 0, Format.R32G32B32A32_Float, 0, 0),
            new("COLOR", 0, Format.R32G32B32A32_Float, 16, 0)
        ];

        _inputLayout = _device.CreateInputLayout(inputElements, vertexShaderByteCode.Span);
    }

    private void CreateMesh()
    {
        Vertex[] vertices =
        [
            new(new Vector4(-0.5f, 0.5f, 0.2f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
            new(new Vector4(0.5f, -0.5f, 0.2f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
            new(new Vector4(-0.5f, -0.5f, 0.2f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),
            new(new Vector4(0.5f, 0.5f, 0.2f, 1.0f), new Vector4(1.0f, 0.0f, 0.0f, 1.0f)),

            new(new Vector4(-0.75f, 0.75f, 0.3f, 1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f)),
            new(new Vector4(0.0f, 0.0f, 0.3f, 1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f)),
            new(new Vector4(-0.75f, 0.0f, 0.3f, 1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f)),
            new(new Vector4(0.0f, 0.75f, 0.3f, 1.0f), new Vector4(1.0f, 1.0f, 0.0f, 1.0f))
        ];

        uint[] indices =
        [
            0, 1, 2,
            0, 3, 1,

            4, 5, 6,
            4, 7, 5
        ];

        _vertexBuffer = _device!.CreateBuffer(vertices, BindFlags.VertexBuffer);
        _indexBuffer = _device.CreateBuffer(indices, BindFlags.IndexBuffer);

        _indexCount = (uint)indices.Length;
    }

    public void Loop()
    {
        Color4 color = new(0.0f, 0.2f, 0.4f, 1.0f);

        _deviceContext!.ClearRenderTargetView(_renderTargetView!, color);
        _deviceContext.ClearDepthStencilView(_depthStencilView!, DepthStencilClearFlags.Depth | DepthStencilClearFlags.Stencil, 1.0f, 0);

        _deviceContext.OMSetRenderTargets(_renderTargetView!, _depthStencilView);

        _deviceContext.RSSetViewport(new Viewport(Width, Height));

        _deviceContext.IASetVertexBuffer(0, _vertexBuffer, (uint)Unsafe.SizeOf<Vertex>());
        _deviceContext.IASetIndexBuffer(_indexBuffer, Format.R32_UInt, 0);
        _deviceContext.IASetInputLayout(_inputLayout);
        _deviceContext.IASetPrimitiveTopology(PrimitiveTopology.TriangleList);

        _deviceContext.VSSetShader(_vertexShader);
        _deviceContext.PSSetShader(_pixelShader);

        _deviceContext.DrawIndexed(_indexCount, 0, 0);

        _swapChain!.Present(1, PresentFlags.None);
    }

    public void Dispose()
    {
        _deviceContext?.ClearState();
        _deviceContext?.Flush();

        _depthStencilState?.Dispose();
        _depthStencilView?.Dispose();
        _depthStencilBuffer?.Dispose();

        _indexBuffer?.Dispose();
        _vertexBuffer?.Dispose();
        _inputLayout?.Dispose();
        _vertexShader?.Dispose();
        _pixelShader?.Dispose();

        _renderTargetView?.Dispose();
        _backBuffer?.Dispose();
        _swapChain?.Dispose();
        _factory?.Dispose();
        _deviceContext?.Dispose();
        _device?.Dispose();
    }
}

internal static class Program
{
    public static void Main()
    {
        using GameWindow window = new();
        using RenderSystem render = new();

        window.Initialize(new GameWindow.Config
        {
            Title = "DX11 DepthTests",
            Width = 1200,
            Height = 820,
            Resizable = true
        });

        render.Initialize(window.Handle);

        while (window.IsRunning)
        {
            window.PumpMessages();
            render.Loop();
        }
    }
}
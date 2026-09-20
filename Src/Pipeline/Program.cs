using System.IO;
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

    private ID3D11VertexShader? _vertexShader;
    private ID3D11PixelShader? _pixelShader;

    public uint Width { get; } = 1200;
    public uint Height { get; } = 820;
    public uint FrameCount { get; } = 2;

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


        CreateShaders();
    }

    private void CreateShaders()
    {
        ReadOnlyMemory<byte> vertexShaderByteCode = Compiler.CompileFromFile("Vertex.hlsl", "VS", "vs_5_0");
        ReadOnlyMemory<byte> pixelShaderByteCode = Compiler.CompileFromFile("Pixel.hlsl", "PS", "ps_5_0");

        _vertexShader = _device!.CreateVertexShader(vertexShaderByteCode.Span);
        _pixelShader = _device.CreatePixelShader(pixelShaderByteCode.Span);
    }

    public void Loop()
    {
        Color4 color = new(0.0f, 0.2f, 0.4f, 1.0f);

        _deviceContext!.ClearRenderTargetView(_renderTargetView!, color);
        _deviceContext.OMSetRenderTargets(_renderTargetView!, null);

        _deviceContext.RSSetViewport(new Viewport(Width, Height));

        _deviceContext.IASetPrimitiveTopology(PrimitiveTopology.TriangleList);

        _deviceContext.VSSetShader(_vertexShader);
        _deviceContext.PSSetShader(_pixelShader);

        _deviceContext.Draw(3, 0);

        _swapChain!.Present(1, PresentFlags.None);
    }

    public void Dispose()
    {
        _deviceContext?.ClearState();
        _deviceContext?.Flush();

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
            Title = "DX11 Pipeline",
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
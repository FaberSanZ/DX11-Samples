using System.Runtime.InteropServices;
using Vortice.Direct3D;
using Vortice.Direct3D11;
using Vortice.DXGI;
using Vortice.Mathematics;
using Vultaik;
using static Vortice.Direct3D11.D3D11;
using static Vortice.DXGI.DXGI;

internal sealed class RenderSystem : IDisposable
{
    private ID3D11Device? m_device;
    private ID3D11DeviceContext? m_deviceContext;
    private IDXGIFactory2? m_factory;
    private IDXGISwapChain1? m_swapChain;
    private ID3D11Texture2D? m_backBuffer;
    private ID3D11RenderTargetView? m_renderTargetView;

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

        D3D11CreateDevice(IntPtr.Zero, DriverType.Hardware, DeviceCreationFlags.None, featureLevels, out m_device, out FeatureLevel featureLevel, out m_deviceContext);

        m_factory = CreateDXGIFactory1<IDXGIFactory2>();

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

        m_swapChain = m_factory.CreateSwapChainForHwnd(m_device, hwnd, swapChainDescription, fullscreenDescription);

        m_factory.MakeWindowAssociation(hwnd, WindowAssociationFlags.IgnoreAltEnter);

        m_backBuffer = m_swapChain.GetBuffer<ID3D11Texture2D>(0);
        m_renderTargetView = m_device.CreateRenderTargetView(m_backBuffer);

    }

    public void Loop()
    {
        Color4 color = new(0.0f, 0.2f, 0.4f, 1.0f);

        m_deviceContext!.ClearRenderTargetView(m_renderTargetView!, color);
        m_swapChain!.Present(1, PresentFlags.None);
    }

    public void Dispose()
    {
        m_deviceContext?.ClearState();
        m_deviceContext?.Flush();

        m_renderTargetView?.Dispose();
        m_backBuffer?.Dispose();
        m_swapChain?.Dispose();
        m_factory?.Dispose();
        m_deviceContext?.Dispose();
        m_device?.Dispose();
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
            Title = "DX11 ClearScreen",
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
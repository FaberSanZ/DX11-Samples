using System.Buffers.Binary;
using System.Numerics;
using System.Runtime.CompilerServices;
using glTFLoader;
using glTFLoader.Schema;
using ImGuiNET;
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
    private ID3D11RasterizerState? _rasterizerState;

    private ID3D11VertexShader? _vertexShader;
    private ID3D11PixelShader? _pixelShader;
    private ID3D11InputLayout? _inputLayout;
    private ID3D11Buffer? _vertexBuffer;
    private ID3D11Buffer? _indexBuffer;
    private ID3D11Buffer? _constantBuffer;

    private CameraBuffer _cameraData;
    private uint _indexCount;
    private float _modelRotation;
    private Matrix4x4 _modelTransform = Matrix4x4.Identity;

    public uint Width { get; } = 1440;
    public uint Height { get; } = 820;
    public uint FrameCount { get; } = 2;

    public ID3D11Device Device => _device!;
    public ID3D11DeviceContext DeviceContext => _deviceContext!;

    private struct Vertex
    {
        public Vector3 Position;
        public Vector3 Normal;

        public Vertex(Vector3 position, Vector3 normal)
        {
            Position = position;
            Normal = normal;
        }
    }

    private struct CameraBuffer
    {
        public Matrix4x4 World;
        public Matrix4x4 View;
        public Matrix4x4 Projection;
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

        CreateRasterizer();
        CreateDepthBuffer();
        CreateCamera();
        CreateShaders();
        CreateMesh("Model.glb");
        CreateConstantBuffer();
    }

    private void CreateRasterizer()
    {
        _rasterizerState = _device!.CreateRasterizerState(RasterizerDescription.CullNone);
    }

    private void CreateDepthBuffer()
    {
        DepthStencilDescription depthStencilDescription = new(true, DepthWriteMask.All, ComparisonFunction.Less);

        _depthStencilState = _device!.CreateDepthStencilState(depthStencilDescription);

        _depthStencilBuffer = _device.CreateTexture2D(
            Format.D24_UNorm_S8_UInt,
            Width,
            Height,
            mipLevels: 1,
            bindFlags: BindFlags.DepthStencil
        );

        _depthStencilView = _device.CreateDepthStencilView(_depthStencilBuffer);
    }

    private void CreateCamera()
    {
        Matrix4x4 view = Matrix4x4.CreateLookAt(new Vector3(0.0f, 0.0f, 10.0f), Vector3.Zero, Vector3.UnitY);

        float fieldOfView = MathF.PI / 4.0f;
        float aspectRatio = Width / (float)Height;

        Matrix4x4 projection = Matrix4x4.CreatePerspectiveFieldOfView(
            fieldOfView,
            aspectRatio,
            0.1f,
            1000.0f
        );

        _cameraData.World = Matrix4x4.Transpose(Matrix4x4.Identity);
        _cameraData.View = Matrix4x4.Transpose(view);
        _cameraData.Projection = Matrix4x4.Transpose(projection);
    }

    private void CreateShaders()
    {
        ReadOnlyMemory<byte> vertexShaderByteCode = Compiler.CompileFromFile("Vertex.hlsl", "VS", "vs_5_0");
        ReadOnlyMemory<byte> pixelShaderByteCode = Compiler.CompileFromFile("Pixel.hlsl", "PS", "ps_5_0");

        _vertexShader = _device!.CreateVertexShader(vertexShaderByteCode.Span);
        _pixelShader = _device.CreatePixelShader(pixelShaderByteCode.Span);

        InputElementDescription[] inputElements =
        [
            new("POSITION", 0, Format.R32G32B32_Float, 0, 0),
            new("COLOR", 0, Format.R32G32B32_Float, 12, 0)
        ];

        _inputLayout = _device.CreateInputLayout(inputElements, vertexShaderByteCode.Span);
    }

    private void CreateMesh(string filePath)
    {
        Gltf model = Interface.LoadModel(filePath);

        if (model.Meshes is null || model.Meshes.Length == 0)
            throw new InvalidDataException("The glTF file has no meshes.");

        MeshPrimitive primitive = model.Meshes[0].Primitives[0];

        Node? meshNode = null;

        if (model.Nodes is not null)
        {
            foreach (Node node in model.Nodes)
            {
                if (node.Mesh == 0)
                {
                    meshNode = node;
                    break;
                }
            }
        }

        if (meshNode is not null)
            _modelTransform = CreateNodeTransform(meshNode);

        if (primitive.Mode != MeshPrimitive.ModeEnum.TRIANGLES)
            throw new NotSupportedException("This sample only supports TRIANGLES.");

        if (!primitive.Attributes.TryGetValue("POSITION", out int positionAccessorIndex))
            throw new InvalidDataException("The primitive has no POSITION attribute.");

        if (!primitive.Attributes.TryGetValue("NORMAL", out int normalAccessorIndex))
            throw new InvalidDataException("The primitive has no NORMAL attribute.");

        Vector3[] positions = ReadVector3Accessor(model, positionAccessorIndex, filePath);
        Vector3[] normals = ReadVector3Accessor(model, normalAccessorIndex, filePath);

        if (positions.Length != normals.Length)
            throw new InvalidDataException("POSITION and NORMAL counts do not match.");

        Vertex[] vertices = new Vertex[positions.Length];

        for (int i = 0; i < vertices.Length; i++)
            vertices[i] = new Vertex(positions[i], normals[i]);

        uint[] indices;

        if (primitive.Indices.HasValue)
        {
            indices = ReadIndexAccessor(model, primitive.Indices.Value, filePath);
        }
        else
        {
            indices = new uint[vertices.Length];

            for (uint i = 0; i < indices.Length; i++)
                indices[i] = i;
        }

        _vertexBuffer = _device!.CreateBuffer(vertices, BindFlags.VertexBuffer);
        _indexBuffer = _device.CreateBuffer(indices, BindFlags.IndexBuffer);
        _indexCount = (uint)indices.Length;
    }

    private static Matrix4x4 CreateNodeTransform(Node node)
    {
        if (node.ShouldSerializeMatrix())
        {
            float[] m = node.Matrix;

            return new Matrix4x4(
                m[0], m[1], m[2], m[3],
                m[4], m[5], m[6], m[7],
                m[8], m[9], m[10], m[11],
                m[12], m[13], m[14], m[15]
            );
        }

        Vector3 scale = new(node.Scale[0], node.Scale[1], node.Scale[2]);
        Quaternion rotation = new(node.Rotation[0], node.Rotation[1], node.Rotation[2], node.Rotation[3]);
        Vector3 translation = new(node.Translation[0], node.Translation[1], node.Translation[2]);

        return Matrix4x4.CreateScale(scale) *
               Matrix4x4.CreateFromQuaternion(rotation) *
               Matrix4x4.CreateTranslation(translation);
    }

    private static Vector3[] ReadVector3Accessor(Gltf model, int accessorIndex, string filePath)
    {
        Accessor accessor = model.Accessors[accessorIndex];

        if (!accessor.BufferView.HasValue)
            throw new NotSupportedException("Sparse-only accessors are not supported in this sample.");

        if (accessor.Sparse is not null)
            throw new NotSupportedException("Sparse accessors are not supported in this sample.");

        if (accessor.ComponentType != Accessor.ComponentTypeEnum.FLOAT || accessor.Type != Accessor.TypeEnum.VEC3)
            throw new NotSupportedException("POSITION and NORMAL must be FLOAT VEC3.");

        BufferView bufferView = model.BufferViews[accessor.BufferView.Value];
        byte[] buffer = model.LoadBinaryBuffer(bufferView.Buffer, filePath);

        int stride = bufferView.ByteStride ?? 12;
        int start = bufferView.ByteOffset + accessor.ByteOffset;

        Vector3[] values = new Vector3[accessor.Count];

        for (int i = 0; i < values.Length; i++)
        {
            int offset = start + i * stride;
            float x = BinaryPrimitives.ReadSingleLittleEndian(buffer.AsSpan(offset, 4));
            float y = BinaryPrimitives.ReadSingleLittleEndian(buffer.AsSpan(offset + 4, 4));
            float z = BinaryPrimitives.ReadSingleLittleEndian(buffer.AsSpan(offset + 8, 4));

            values[i] = new Vector3(x, y, z);
        }

        return values;
    }

    private static uint[] ReadIndexAccessor(Gltf model, int accessorIndex, string filePath)
    {
        Accessor accessor = model.Accessors[accessorIndex];

        if (!accessor.BufferView.HasValue)
            throw new NotSupportedException("Sparse-only index accessors are not supported in this sample.");

        if (accessor.Sparse is not null)
            throw new NotSupportedException("Sparse index accessors are not supported in this sample.");

        if (accessor.Type != Accessor.TypeEnum.SCALAR)
            throw new NotSupportedException("Indices must use a SCALAR accessor.");

        BufferView bufferView = model.BufferViews[accessor.BufferView.Value];
        byte[] buffer = model.LoadBinaryBuffer(bufferView.Buffer, filePath);

        int componentSize = accessor.ComponentType switch
        {
            Accessor.ComponentTypeEnum.UNSIGNED_BYTE => 1,
            Accessor.ComponentTypeEnum.UNSIGNED_SHORT => 2,
            Accessor.ComponentTypeEnum.UNSIGNED_INT => 4,
            _ => throw new NotSupportedException("Unsupported index component type.")
        };

        int stride = bufferView.ByteStride ?? componentSize;
        int start = bufferView.ByteOffset + accessor.ByteOffset;

        uint[] indices = new uint[accessor.Count];

        for (int i = 0; i < indices.Length; i++)
        {
            int offset = start + i * stride;

            indices[i] = accessor.ComponentType switch
            {
                Accessor.ComponentTypeEnum.UNSIGNED_BYTE => buffer[offset],
                Accessor.ComponentTypeEnum.UNSIGNED_SHORT => BinaryPrimitives.ReadUInt16LittleEndian(buffer.AsSpan(offset, 2)),
                Accessor.ComponentTypeEnum.UNSIGNED_INT => BinaryPrimitives.ReadUInt32LittleEndian(buffer.AsSpan(offset, 4)),
                _ => 0
            };
        }

        return indices;
    }

    private void CreateConstantBuffer()
    {
        BufferDescription constantBufferDescription = new()
        {
            ByteWidth = (uint)Unsafe.SizeOf<CameraBuffer>(),
            Usage = ResourceUsage.Default,
            BindFlags = BindFlags.ConstantBuffer,
            CPUAccessFlags = CpuAccessFlags.None
        };

        _constantBuffer = _device!.CreateBuffer(_cameraData, constantBufferDescription);
    }

    public void Update()
    {
        _modelRotation += 0.01f;

        Matrix4x4 world = _modelTransform * Matrix4x4.CreateRotationY(_modelRotation);
        _cameraData.World = Matrix4x4.Transpose(world);

        _deviceContext!.UpdateSubresource(_cameraData, _constantBuffer!);
    }

    public void Loop()
    {
        Color4 color = new(0.0f, 0.2f, 0.4f, 1.0f);

        _deviceContext!.ClearRenderTargetView(_renderTargetView!, color);
        _deviceContext.ClearDepthStencilView(_depthStencilView!, DepthStencilClearFlags.Depth | DepthStencilClearFlags.Stencil, 1.0f, 0);

        _deviceContext.OMSetRenderTargets(_renderTargetView!, _depthStencilView);
        _deviceContext.OMSetDepthStencilState(_depthStencilState, 1);

        _deviceContext.RSSetState(_rasterizerState);
        _deviceContext.RSSetViewport(new Viewport(Width, Height));

        _deviceContext.IASetVertexBuffer(0, _vertexBuffer, (uint)Unsafe.SizeOf<Vertex>());
        _deviceContext.IASetIndexBuffer(_indexBuffer, Format.R32_UInt, 0);
        _deviceContext.IASetInputLayout(_inputLayout);
        _deviceContext.IASetPrimitiveTopology(PrimitiveTopology.TriangleList);

        _deviceContext.VSSetConstantBuffer(0, _constantBuffer);
        _deviceContext.VSSetShader(_vertexShader);
        _deviceContext.PSSetShader(_pixelShader);

        _deviceContext.DrawIndexed(_indexCount, 0, 0);
    }

    public void Present()
    {
        _swapChain!.Present(1, PresentFlags.None);
    }

    public void Dispose()
    {
        _deviceContext?.ClearState();
        _deviceContext?.Flush();

        _constantBuffer?.Dispose();
        _rasterizerState?.Dispose();
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
            Title = "DX11 glTF + ImGui",
            Width = 1440,
            Height = 820,
            Resizable = true
        });

        render.Initialize(window.Handle);

        using ImGuiController imgui = new(render.Device, render.DeviceContext);
        window.MessageReceived += imgui.ProcessMessage;

        while (window.IsRunning)
        {
            window.PumpMessages();

            render.Update();

            imgui.BeginFrame(window.ClientWidth, window.ClientHeight);

            ImGui.Begin("glTF");
            ImGui.Text("DirectX 11 + C# + glTF2Loader + ImGui.NET");
            ImGui.Text("Mesh 0 / Primitive 0");
            ImGui.Text("POSITION + NORMAL + indices");
            ImGui.Text("Normals visualized as RGB");
            ImGui.Text("Node transform loaded from glTF");
            ImGui.End();


            render.Loop();
            imgui.Render();
            render.Present();
        }

        window.MessageReceived -= imgui.ProcessMessage;
    }
}

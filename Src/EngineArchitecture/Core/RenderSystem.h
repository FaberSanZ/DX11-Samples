#pragma once


#include <windows.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "../Graphics/Adapter.h"
#include "../Graphics/Device.h"
#include "../Graphics/SwapChain.h"
#include "../Graphics/CommandList.h"
#include "../Graphics/Buffer.h"
#include "../Graphics/Texture.h"
#include "../Graphics/Pipeline.h"
#include "../Graphics/EngineData.h"
#include "../Core/Windows.h"


namespace Core
{
    template <typename T>
    class MeshPart
    {
    public:
        MeshPart() = default;
        bool Create(Graphics::Device& device,const std::vector<TVertex>& vertices, const std::vector<uint32_t>& indices)
        {
            if (!m_VertexBuffer.Initialize(device, Graphics::BufferType::VertexBuffer, vertices.data(), sizeof(vertices), sizeof(T)))
                return false;

            if (!m_IndexBuffer.Initialize(device, Graphics::BufferType::IndexBuffer, indices.data(), sizeof(indices), sizeof(uint32_t)))
                return false;

            m_IndexCount = static_cast<uint32_t>(indices.size());

            return true;
        }
        void Bind(Graphics::CommandList& commandList, Graphics::Device& device)
        {
            m_VertexBuffer.Bind(device.GetContext());
			m_IndexBuffer.Bind(device.GetContext());
        }
        Graphics::Buffer m_VertexBuffer;
        Graphics::Buffer m_IndexBuffer;
        uint32_t m_IndexCount { 0 };

    };



    class IMesh
    {
    public:
        virtual ~IMesh() = default;
        virtual void Draw(Graphics::CommandList& cmdList, Graphics::Device& device) = 0;
		virtual void Initialize(Graphics::Device& device) = 0;
        bool m_IsLoop { false };
		virtual void SetPosition(const DirectX::XMFLOAT3& position) = 0;
		virtual void SetRotation(const DirectX::XMFLOAT3& rotation) = 0;
        virtual void SetScale(const DirectX::XMFLOAT3& scale) = 0;
        virtual Graphics::VertexInputElement GetVertexInputElement() const;

    };
    

    template <typename TVertex>
    class Mesh : public IMesh
    {
    public:
        Mesh() = default;
        Mesh(Graphics::Device& device, std::string filePath);
        Mesh(Graphics::Device& device, const std::vector<TVertex>& vertices, const std::vector<uint32_t>& indices);
        Mesh(Graphics::Device& device, TVertex vertices[], uint32_t indices[])
        {
            //m_Vertices.assign(vertices, vertices + sizeof(vertices) / sizeof(TVertex));
            //m_Indices.assign(indices, indices + sizeof(indices) / sizeof(uint32_t));
            Initialize(device);
		}

        void Initialize(Graphics::Device& device) override
        {
			m_MeshPart.Create(device, m_Vertices, m_Indices);   
			m_ConstantBuffer.Initialize(device, Graphics::BufferType::ConstantBuffer, &m_WorldMatrix, sizeof(DirectX::XMMATRIX));
		}

        void Draw(Graphics::CommandList& cmdList, Graphics::Device& device) override
        {
			m_ConstantBuffer.Bind(device.GetContext(), 1); // Bind constant buffer to slot 1 for vertex shader
            m_MeshPart.Bind(cmdList, device);

            cmdList.SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			cmdList.DrawIndexed(m_MeshPart.m_IndexCount, 0, 0); // Draw the mesh using indexed drawing
        }


        void SetPosition(const DirectX::XMFLOAT3& position)
        {
            m_WorldMatrix = DirectX::XMMatrixTranslation(position.x, position.y, position.z);
		}
        void SetRotation(const DirectX::XMFLOAT3& rotation)
        {
            m_WorldMatrix = DirectX::XMMatrixRotationRollPitchYaw(rotation.x, rotation.y, rotation.z);
        }
        void SetScale(const DirectX::XMFLOAT3& scale)
        {
            m_WorldMatrix = DirectX::XMMatrixScaling(scale.x, scale.y, scale.z);
		}

        Graphics::VertexType GetVertexInputElement() const override
        {
            Graphics::VertexInputElement layout {};

            if(std::is_same<TVertex, Graphics::VertexPosition>::value)
            {
                layout.Add(Graphics::VertexType::Position);
                return layout;
			}

            if (typedef(TVertex) == Graphics::VertexPositionColor)
            {
                layout.Add(Graphics::VertexType::Position);
                layout.Add(Graphics::VertexType::Color);
                return layout;
            }

            return layout;
		}

        Core::MeshPart<TVertex> m_MeshPart;
        DirectX::XMMATRIX m_WorldMatrix { DirectX::XMMatrixIdentity() };
		Graphics::Buffer m_ConstantBuffer; // Constant buffer for per-mesh data {camera, ligth, etc..} 
		bool m_IsLoop { false };
    };

	class RenderSystem
	{
	public:
		RenderSystem() = default;
        void Initialize(uint32_t width, uint32_t height)
        {
            m_Width = width;
            m_Height = height;


            m_Adapter.Initialize(0); // Initialize the first GPU adapter (0)
            m_Device.Initialize(m_Adapter); // Initialize the Direct3D device using the adapter
            m_SwapChain.Initialize(m_Device, nullptr, m_Width, m_Height); // Initialize the swap chain with the device and window handle
            m_CommandList.Initialize(m_Device.GetContext()); // Initialize the command list with the device context

            // Clear mesh list
            m_Meshes.clear();


            Graphics::VertexPositionColor vertices[] =
            {
                // Front face
                {{-0.5f,  0.5f, -0.5f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
            };



            uint32_t indices[] =
            {
                // front face
                0, 1, 2, // first triangle
            };


            Mesh<Graphics::VertexPositionColor> cube { m_Device, vertices, indices };
            m_Meshes.push_back(cube);

            Mesh<Graphics::VertexPositionColor> model { m_Device, "../model.gltf"};
			m_Meshes.push_back(model);




        }
        void RenderLoop(const std::function<void()>& on_thread_begin)
        {
            on_thread_begin();

            for (auto& mesh : m_Meshes)
            {
                if (!mesh.m_IsLoop)
                {
                    mesh.Initialize(m_Device);
                    mesh.m_IsLoop = true; // Set the mesh to loop
                }
                else
                {
                    m_CommandList.SetPipelineState(m_Pipeline);
                    mesh.Draw(m_CommandList, m_Device);

                    m_CommandList.DrawIndexed(12, 12, 12);
                }

            }

    //        for (auto& mesh : m_Meshes)
    //        {
    //            m_CommandList.SetPipelineState(m_Pipeline);
				//mesh.Draw(m_CommandList, m_Device);

    //            m_CommandList.DrawIndexed(12,12,12);
    //        }
        }
		void Cleanup();
		void RenderOneFrame();


        Graphics::Adapter m_Adapter;
        Graphics::Device m_Device;
        Graphics::SwapChain m_SwapChain;
        Graphics::CommandList m_CommandList;
        Graphics::Pipeline m_Pipeline;

        std::vector<Core::IMesh> m_Meshes;

        uint32_t m_Width { 1200 }; // Width of the render target
        uint32_t m_Height { 820 }; // Height of the render target
	};
}

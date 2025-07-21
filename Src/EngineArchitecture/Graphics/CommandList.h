#pragma once
#include "SwapChain.h"
#include "Device.h"
#include "Adapter.h"
#include "CommandList.h"

#include <iostream>
#include <dxgi.h>

#include <d3d11.h>
#include <cstdint>

#pragma comment(lib, "d3d11.lib")


namespace Graphics
{
	class CommandList
	{
	public:
		CommandList() = default;
		~CommandList() = default;
		void Initialize(ID3D11DeviceContext* context);
		void Release();
		void Clear(ID3D11RenderTargetView* rtv, const float color[4]);
		void Clear(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv, const float color[4]);
		void SetViewport(float width, float height);

		void SetVertexBuffer(ID3D11Buffer* buffer, uint32_t slot, uint32_t size, uint32_t stride, void* data);
		void SetIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format, uint32_t offset);
		void SetConstantBuffer(ID3D11Buffer* buffer, uint32_t slot, uint32_t size, uint32_t stride, void* data);


	private:
		ID3D11DeviceContext* m_Context = nullptr; // Direct3D device context for executing commands
	};
}
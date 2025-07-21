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
	void CommandList::Initialize(ID3D11DeviceContext* context)
	{

		m_Context = context;

	}
	void CommandList::Release()
	{
		if (m_Context)
		{
			m_Context->Release();
			m_Context = nullptr;
		}
	}

	void CommandList::Clear(ID3D11RenderTargetView* rtv, const float color[4])
	{
		if (m_Context && rtv)
		{
			m_Context->ClearRenderTargetView(rtv, color);
		}
	}
	void CommandList::Clear(ID3D11RenderTargetView* rtv, ID3D11DepthStencilView* dsv, const float color[4])
	{

		if (m_Context && rtv && dsv)
		{
			m_Context->ClearRenderTargetView(rtv, color);
			m_Context->ClearDepthStencilView(dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		}
	}
	void CommandList::SetViewport(float width, float height)
	{
		D3D11_VIEWPORT viewport = {};
		viewport.Width = width;
		viewport.Height = height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 1.0f;
		viewport.TopLeftX = 0.0f;
		viewport.TopLeftY = 0.0f;

		if (m_Context)
		{
			m_Context->RSSetViewports(1, &viewport);
		}
	}


	void CommandList::SetVertexBuffer(ID3D11Buffer* buffer, uint32_t slot, uint32_t size, uint32_t stride, void* data)
	{
		if (m_Context && buffer && data)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (SUCCEEDED(m_Context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			{
				memcpy(mappedResource.pData, data, size);
				m_Context->Unmap(buffer, 0);
				m_Context->IASetVertexBuffers(slot, 1, &buffer, &stride, nullptr);
			}
		}
	}

	void CommandList::SetIndexBuffer(ID3D11Buffer* buffer, DXGI_FORMAT format, uint32_t offset)
	{
		if (m_Context && buffer)
		{
			m_Context->IASetIndexBuffer(buffer, format, offset);
		}
	}
	void CommandList::SetConstantBuffer(ID3D11Buffer* buffer, uint32_t slot, uint32_t size, uint32_t stride, void* data)
	{
		if (m_Context && buffer && data)
		{
			D3D11_MAPPED_SUBRESOURCE mappedResource;
			if (SUCCEEDED(m_Context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource)))
			{
				memcpy(mappedResource.pData, data, size);
				m_Context->Unmap(buffer, 0);
				m_Context->VSSetConstantBuffers(slot, 1, &buffer);
			}
		}
	}


} // namespace Graphics
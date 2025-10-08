#pragma once

#include <d3d11.h>
#include <windows.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class Viewport
{
public:
	Viewport();
	~Viewport();

	bool Initialize(HWND hWnd, WNDCLASSEXW);
	void Shutdown();
	bool Render();

private:
	bool InitializeDeviceD3D(HWND hWnd);

	bool CreateRenderTarget();

	bool InitializeDepth();

private:
	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_deviceContext;

	ComPtr<ID3D11RenderTargetView> m_renderTargetView;

	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	ComPtr<ID3D11SamplerState> m_sampler;

	D3D11_VIEWPORT m_dvp = {};

	UINT m_screenWidth = 0, m_screenHeight = 0;
};
#include "Viewport.h"

#include <vector>
#include "imgui_impl_dx11.h"

Viewport::Viewport() {};
Viewport::~Viewport() {};

bool Viewport::Initialize(HWND hwnd, WNDCLASSEXW wc, float nearPlane, float farPlane)
{
	m_near = nearPlane;
	m_far = farPlane;
	
	RECT rect;
	GetClientRect(hwnd, &rect);

	m_screenWidth = static_cast<int>(rect.right - rect.left);
	m_screenHeight = static_cast<int>(rect.bottom - rect.top);

	// Initialize Direct3D
	if (!InitializeDeviceD3D(hwnd))
	{
		::UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return false;
	}

	ImGui_ImplDX11_Init(m_device.Get(), m_deviceContext.Get());

	CreateRenderTarget();
	InitializeDepth();

	//ID3D11RenderTargetView* rtvPtr = m_renderTargetView.Get();
	//m_deviceContext->OMSetRenderTargets(1, &rtvPtr, m_depthStencilView.Get());

	InitializeRasterizer();

	ResetViewport(static_cast<float>(m_screenWidth), static_cast<float>(m_screenHeight));

	CreateSampler();
	ID3D11SamplerState* samplerPtr = m_sampler.Get();
	m_deviceContext->PSSetSamplers(0, 1, &samplerPtr);

	m_pipeline = std::make_unique<Pipeline>();
	ID3D11RenderTargetView* rtvPtr = m_renderTargetView.Get();

	constexpr int G_BUFFER_WIDTH = 512;
	constexpr int G_BUFFER_HEIGHT = 512;
	m_pipeline->Initialize(m_device.Get(), rtvPtr, G_BUFFER_WIDTH, G_BUFFER_HEIGHT);

	SetLayoutMode(LayoutMode::Grid2x2);

	return true;
}

LayoutMode Viewport::GetLayoutMode() const
{
	return m_layoutMode;
}

void Viewport::SetLayoutMode(LayoutMode mode)
{
	m_layoutMode = mode;
	RebuildViews();
}

int Viewport::GetViewCount() const
{
	return static_cast<int>(m_views.size());
}

void Viewport::RebuildViews()
{
	std::vector<View> previousViews = std::move(m_views);

	if (m_layoutMode == LayoutMode::Single)
	{
		constexpr float size = 1024.0f;
		const float originX = (static_cast<float>(m_screenWidth) - size) * 0.5f;
		const float originY = (static_cast<float>(m_screenHeight) - size) * 0.5f;

		D3D11_VIEWPORT rect = {};
		rect.TopLeftX = originX;
		rect.TopLeftY = originY;
		rect.Width = size;
		rect.Height = size;
		rect.MinDepth = 0.0f;
		rect.MaxDepth = 1.0f;

		ShadingMode preservedMode = previousViews.empty() ? ShadingMode::Matcap : previousViews[0].shadingMode;
		m_views = { View{ rect, preservedMode } };
	}
	else // LayoutMode::Grid2x2
	{
		constexpr float cellSize = 512.0f;
		const float gridOriginX = (static_cast<float>(m_screenWidth) - 2.0f * cellSize) * 0.5f;
		const float gridOriginY = (static_cast<float>(m_screenHeight) - 2.0f * cellSize) * 0.5f;
		static const ShadingMode defaultModes[4] = { ShadingMode::Matcap, ShadingMode::Normal, ShadingMode::Depth, ShadingMode::Crossfield };

		m_views.resize(4);
		for (int cell = 0; cell < 4; cell++)
		{
			const int col = cell % 2;
			const int row = cell / 2;

			D3D11_VIEWPORT rect = {};
			rect.TopLeftX = gridOriginX + col * cellSize;
			rect.TopLeftY = gridOriginY + row * cellSize;
			rect.Width = cellSize;
			rect.Height = cellSize;
			rect.MinDepth = 0.0f;
			rect.MaxDepth = 1.0f;

			m_views[cell].rect = rect;
			m_views[cell].shadingMode = (cell < static_cast<int>(previousViews.size())) ? previousViews[cell].shadingMode : defaultModes[cell];
		}
	}
}

bool Viewport::Render(glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, Scene* scene)
{
	if (m_isSwapChainOccluded && m_swapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
	{
		::Sleep(10);
		return false;
	}
	m_isSwapChainOccluded = false;

	// Render 3D
	glm::vec3 lightNorm = glm::normalize(MATCAP_LIGHT);
	m_pipeline->Update(m_deviceContext.Get(), viewMatrix, projectionMatrix, lightNorm, m_near, m_far);
	//model->Render(m_deviceContext.Get());
	m_pipeline->RenderGeometryPass(m_deviceContext.Get(), scene, viewMatrix, projectionMatrix);
	for (const View& view : m_views)
		m_pipeline->CompositeView(m_deviceContext.Get(), static_cast<int>(view.shadingMode), view.rect);

	// Unbind
	//ID3D11RenderTargetView* nullRTV = nullptr;
	//m_deviceContext->OMSetRenderTargets(1, &nullRTV, nullptr);

	// Render UI
	ID3D11RenderTargetView* rtvPtr = m_renderTargetView.Get();
	m_deviceContext->OMSetRenderTargets(1, &rtvPtr, m_depthStencilView.Get());
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	// Present
	HRESULT hr = m_swapChain->Present(0, 0); // Without vsync
	m_isSwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
	return true;
}

ID3D11Device* Viewport::GetDevice()
{
	return m_device.Get();
}

ID3D11DeviceContext* Viewport::GetContext()
{
	return m_deviceContext.Get();
}

ShadingMode Viewport::GetShadingMode(int cellIndex)
{
	return m_views[cellIndex].shadingMode;
}

void Viewport::SetShadingMode(int cellIndex, ShadingMode mode)
{
	m_views[cellIndex].shadingMode = mode;
}

const D3D11_VIEWPORT& Viewport::GetViewRect(int cellIndex) const
{
	return m_views[cellIndex].rect;
}

void Viewport::CaptureDatapoint(std::wstring prefix)
{
	m_pipeline->CaptureDatapoint(m_deviceContext.Get(), prefix);
}



// ETC

void Viewport::ResetViewport(float width, float height)
{
	m_dvp.Width = width;
	m_dvp.Height = height;
	m_dvp.MinDepth = 0.0f;
	m_dvp.MaxDepth = 1.0f;
	m_dvp.TopLeftX = 0.0f;
	m_dvp.TopLeftY = 0.0f;

	m_deviceContext->RSSetViewports(1, &m_dvp);
}

bool Viewport::InitializeDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	//sd.Flags = 0;

	UINT createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;

	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &featureLevel, &m_deviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &m_swapChain, &m_device, &featureLevel, &m_deviceContext);
	if (res != S_OK)
		return false;

	return true;
}

bool Viewport::CreateRenderTarget()
{
	ComPtr<ID3D11Texture2D> pBackBuffer;

	HRESULT result = m_swapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (FAILED(result))
		return false;

	result = m_device->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &m_renderTargetView);
	if (FAILED(result))
		return false;

	return true;
}

bool Viewport::InitializeDepth()
{
	D3D11_DEPTH_STENCIL_DESC dsd;
	ZeroMemory(&dsd, sizeof(dsd));
	dsd.DepthEnable = true;
	dsd.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsd.DepthFunc = D3D11_COMPARISON_LESS;

	dsd.StencilEnable = true;
	dsd.StencilReadMask = 0xFF;
	dsd.StencilWriteMask = 0xFF;

	dsd.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	dsd.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	dsd.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	dsd.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	dsd.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	HRESULT result = m_device->CreateDepthStencilState(&dsd, &m_depthStencilState);
	if (FAILED(result))
		return false;

	D3D11_TEXTURE2D_DESC dbd;
	ZeroMemory(&dbd, sizeof(dbd));
	dbd.Width = m_screenWidth;
	dbd.Height = m_screenHeight;
	dbd.MipLevels = 1;
	dbd.ArraySize = 1;
	dbd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dbd.SampleDesc.Count = 1;
	dbd.SampleDesc.Quality = 0;
	dbd.Usage = D3D11_USAGE_DEFAULT;
	dbd.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	dbd.CPUAccessFlags = 0;
	dbd.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> depthStencilBuffer;
	result = m_device->CreateTexture2D(&dbd, nullptr, depthStencilBuffer.GetAddressOf());
	if (FAILED(result))
		return false;

	m_deviceContext->OMSetDepthStencilState(m_depthStencilState.Get(), 1);

	// Stencil view descriptioon
	D3D11_DEPTH_STENCIL_VIEW_DESC svd;
	ZeroMemory(&svd, sizeof(svd));
	svd.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	svd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	svd.Texture2D.MipSlice = 0;

	result = m_device->CreateDepthStencilView(depthStencilBuffer.Get(), &svd, &m_depthStencilView);
	if (FAILED(result))
		return false;

	return true;
}

bool Viewport::InitializeRasterizer()
{
	ComPtr<ID3D11RasterizerState> rasterState;
	D3D11_RASTERIZER_DESC rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.AntialiasedLineEnable = false;
	rd.CullMode = D3D11_CULL_BACK;
	rd.DepthBias = 0;
	rd.DepthBiasClamp = 0.0f;
	rd.DepthClipEnable = true;
	rd.FillMode = D3D11_FILL_SOLID;
	rd.MultisampleEnable = false;
	rd.ScissorEnable = false;
	rd.SlopeScaledDepthBias = 0.0f;

	HRESULT result = m_device->CreateRasterizerState(&rd, rasterState.GetAddressOf());
	if (FAILED(result))
		return false;

	m_deviceContext->RSSetState(rasterState.Get());

	return true;
}

bool Viewport::CreateSampler()
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.MipLODBias = 0.0f;
	sd.MaxAnisotropy = 1;
	sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sd.BorderColor[0] = 0;
	sd.BorderColor[1] = 0;
	sd.BorderColor[2] = 0;
	sd.BorderColor[3] = 0;
	sd.MinLOD = 0;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT result = m_device->CreateSamplerState(&sd, &m_sampler);
	if (FAILED(result))
		return false;

	return true;
}
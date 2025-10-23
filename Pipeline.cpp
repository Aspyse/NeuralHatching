#include "Pipeline.h"

void Pipeline::Initialize(ID3D11Device* device, ID3D11RenderTargetView* outRTV, int textureWidth, int textureHeight)
{
	m_outRTV = outRTV;

	bool result = CreateRenderTarget(device, m_normalRTV.GetAddressOf(), m_normalSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_hatchRTV.GetAddressOf(), m_hatchSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_matcapRTV.GetAddressOf(), m_matcapSRV.GetAddressOf(), textureWidth, textureHeight);

	m_geometryNode = std::make_unique<GeometryNode>();
	m_geometryNode->Initialize(device, L"Shaders/geometry.hlsl", L"Shaders/geometry.hlsl", "GeometryVertexShader", "GeometryPixelShader");

	m_matcapNode = std::make_unique<Node>();
	m_matcapNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/matcap.hlsl", "BaseVertexShader", "PostprocessShader");

	m_outNode = std::make_unique<Node>();
	m_outNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/fullscreen.hlsl", "BaseVertexShader", "PostprocessShader");

	m_geometryNode->AddVSConstantBuffer<MatrixBuffer>(device);
	m_matcapNode->AddPSConstantBuffer<MatcapBuffer>(device);

	InitializeDepthTarget(device, textureWidth, textureHeight);
}

// sidenote: probably the most elegant method i've tried so far
void Pipeline::Update(ID3D11DeviceContext* deviceContext, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, glm::vec3 lightDirection)
{
	MatrixBuffer matrixBuffer;
	matrixBuffer.worldMatrix = glm::transpose(glm::mat4x4(1.0f)); // TODO: replace identity matrix
	matrixBuffer.viewMatrix = glm::transpose(viewMatrix);
	matrixBuffer.projectionMatrix = glm::transpose(projectionMatrix);

	MatcapBuffer matcapBuffer;
	matcapBuffer.invProj = glm::transpose(glm::inverse(projectionMatrix));
	matcapBuffer.invView = glm::transpose(glm::inverse(viewMatrix));
	matcapBuffer.lightDirectionVS = glm::mat3(viewMatrix) * glm::normalize(lightDirection); // TODO: test correctness

	m_geometryNode->UpdateVSConstantBuffer<MatrixBuffer>(deviceContext, matrixBuffer, 0);
	m_matcapNode->UpdatePSConstantBuffer<MatcapBuffer>(deviceContext, matcapBuffer, 0);
}

void Pipeline::Render(ID3D11DeviceContext* deviceContext, int indexCount, int shadingMode)
{	
	deviceContext->OMSetDepthStencilState(nullptr, 0);
	float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	deviceContext->ClearDepthStencilView(m_depthSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	deviceContext->ClearRenderTargetView(m_normalRTV.Get(), clearColor);
	deviceContext->ClearRenderTargetView(m_hatchRTV.Get(), clearColor);
	deviceContext->ClearRenderTargetView(m_matcapRTV.Get(), clearColor);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, nullSRV);
	// Bind g-buffer RTV and depth
	ID3D11RenderTargetView* gbufferRTVPtr[] = {
		m_normalRTV.Get(),
		m_hatchRTV.Get()
	};
	deviceContext->OMSetRenderTargets(2, gbufferRTVPtr, m_depthSV.Get());
	// Set geometry shader
	m_geometryNode->Render(deviceContext);
	// Draw model indices
	deviceContext->DrawIndexed(indexCount, 0, 0);

	Unbind(deviceContext);

	// Bind matcap RTV
	ID3D11RenderTargetView* matcapRTVPtr = m_matcapRTV.Get();
	deviceContext->OMSetRenderTargets(1, &matcapRTVPtr, nullptr);
	// Bind g-buffer SRVs
	ID3D11ShaderResourceView* gbufferSRVPtr[] = {
		m_normalSRV.Get(),
		m_depthSRV.Get()
	};
	deviceContext->PSSetShaderResources(0, 2, gbufferSRVPtr);
	// Set matcap shader
	m_matcapNode->Render(deviceContext);
	// Draw fullscreen tri
	deviceContext->Draw(3, 0);

	Unbind(deviceContext);

	// Bind out RTV
	ID3D11RenderTargetView* outRTVPtr = m_outRTV.Get();
	deviceContext->OMSetRenderTargets(1, &outRTVPtr, nullptr);
	// Bind selected SRV
	ID3D11ShaderResourceView* selectedSRV = nullptr;
	switch (shadingMode)
	{
	case 0: // Matcap
		selectedSRV = m_matcapSRV.Get();
		break;
	case 1: // Normal
		selectedSRV = m_normalSRV.Get();
		break;
	case 2: // Depth
		selectedSRV = m_depthSRV.Get();
		break;
	case 3: // Cross field
		selectedSRV = m_hatchSRV.Get();
		break;
	}
	deviceContext->PSSetShaderResources(0, 1, &selectedSRV);
	// Set passthrough shader
	m_outNode->Render(deviceContext);
	// Draw fullscreen tri
	deviceContext->Draw(3, 0);

	Unbind(deviceContext);
}

void Pipeline::Unbind(ID3D11DeviceContext* deviceContext)
{
	ID3D11RenderTargetView* nullRTV[] = {nullptr, nullptr};
	deviceContext->OMSetRenderTargets(2, nullRTV, nullptr);
}

bool Pipeline::CreateRenderTarget(ID3D11Device* device, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv, int textureWidth, int textureHeight)
{
	ID3D11Texture2D* texture = nullptr;

	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = textureWidth;
	td.Height = textureHeight;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	HRESULT result = device->CreateTexture2D(&td, nullptr, &texture);
	if (FAILED(result))
		return false;

	// RTV
	result = device->CreateRenderTargetView(texture, nullptr, rtv);
	if (FAILED(result))
		return false;

	// SRV
	result = device->CreateShaderResourceView(texture, nullptr, srv);
	if (FAILED(result))
		return false;

	texture->Release();
	texture = nullptr;

	return true;
}

bool Pipeline::InitializeDepthTarget(ID3D11Device* device, int textureWidth, int textureHeight)
{
	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC depthDesc;
	ZeroMemory(&depthDesc, sizeof(depthDesc));
	depthDesc.Width = textureWidth;
	depthDesc.Height = textureHeight;
	depthDesc.MipLevels = 1;
	depthDesc.ArraySize = 1;
	depthDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // Allows both depth-stencil and shader resource views
	depthDesc.SampleDesc.Count = 1;
	depthDesc.SampleDesc.Quality = 0;
	depthDesc.Usage = D3D11_USAGE_DEFAULT;
	depthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthDesc.CPUAccessFlags = 0;
	depthDesc.MiscFlags = 0;

	ID3D11Texture2D* depthTexture;
	HRESULT result = device->CreateTexture2D(&depthDesc, nullptr, &depthTexture);
	if (FAILED(result))
		return false;

	// Create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	device->CreateDepthStencilView(depthTexture, &dsvDesc, &m_depthSV);

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(depthTexture, &srvDesc, m_depthSRV.GetAddressOf());

	depthTexture->Release();
	depthTexture = nullptr;

	return true;
}

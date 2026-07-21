#include "Pipeline.h"

void Pipeline::Initialize(ID3D11Device* device, ID3D11RenderTargetView* outRTV, int textureWidth, int textureHeight)
{
	m_outRTV = outRTV;
	m_bufferWidth = textureWidth;
	m_bufferHeight = textureHeight;

	bool result = CreateRenderTarget(device, m_normalRTV.GetAddressOf(), m_normalSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_depthPassthruRTV.GetAddressOf(), m_depthPassthruSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_hatchRTV.GetAddressOf(), m_hatchSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2RTV.GetAddressOf(), m_hatch2SRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_reliabilityRTV.GetAddressOf(), m_reliabilitySRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_matcapRTV.GetAddressOf(), m_matcapSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureRTV.GetAddressOf(), m_curvatureSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_hatchLinesRTV.GetAddressOf(), m_hatchLinesSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2LinesRTV.GetAddressOf(), m_hatch2LinesSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureLinesRTV.GetAddressOf(), m_curvatureLinesSRV.GetAddressOf(), textureWidth, textureHeight);

	m_geometryNode = std::make_unique<GeometryNode>();
	m_geometryNode->Initialize(device, L"Shaders/geometry.hlsl", L"Shaders/geometry.hlsl", "GeometryVertexShader", "GeometryPixelShader");

	m_depthPassthruNode = std::make_unique<Node>();
	m_depthPassthruNode->Initialize(device, L"Shaders/linearize.hlsl", L"Shaders/linearize.hlsl", "BaseVertexShader", "PostprocessShader");

	m_matcapNode = std::make_unique<Node>();
	m_matcapNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/matcap.hlsl", "BaseVertexShader", "PostprocessShader");

	m_outNode = std::make_unique<Node>();
	m_outNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/fullscreen.hlsl", "BaseVertexShader", "PostprocessShader");

	m_gridNode = std::make_unique<Node>();
	m_gridNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/grid.hlsl", "BaseVertexShader", "PostprocessShader");

	m_curvaturePostNode = std::make_unique<Node>();
	m_curvaturePostNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/curvature_post.hlsl", "BaseVertexShader", "PostprocessShader");

	m_inferenceNode = std::make_unique<InferenceNode>();
	m_inferenceNode->Initialize(device, L"Weights/neural_hatch_v3_fp16.onnx", textureWidth, textureHeight);

	// vertex-pulls whichever field is bound and draws them
	m_hatchLineNode = std::make_unique<Node>();
	m_hatchLineNode->Initialize(device, L"Shaders/hatch_lines.hlsl", L"Shaders/hatch_lines.hlsl", "HatchLineVertexShader", "HatchLinePixelShader");

	// one tracer per direction field
	m_hatchTraceNode = std::make_unique<HatchTraceNode>();
	m_hatchTraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 1u);

	m_hatch2TraceNode = std::make_unique<HatchTraceNode>();
	m_hatch2TraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 2u);

	m_curvatureTraceNode = std::make_unique<HatchTraceNode>();
	m_curvatureTraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 3u);

	m_geometryNode->AddVSConstantBuffer<MatrixBuffer>(device);
	m_geometryNode->AddPSConstantBuffer<MatrixBuffer>(device);
	m_depthPassthruNode->AddPSConstantBuffer<DepthBuffer>(device);
	m_matcapNode->AddPSConstantBuffer<MatcapBuffer>(device);
	m_gridNode->AddPSConstantBuffer<GridBuffer>(device);
	m_hatchLineNode->AddVSConstantBuffer<HatchLineBuffer>(device);
	m_hatchLineNode->AddPSConstantBuffer<HatchLineBuffer>(device);

	InitializeDepthTarget(device, textureWidth, textureHeight);
	InitializeBlendState(device);
}

// sidenote: probably the most elegant method i've tried so far
void Pipeline::Update(ID3D11DeviceContext* deviceContext, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, glm::vec3 lightDirection, float nearPlane, float farPlane)
{
	//TODO: make not constant
	DepthBuffer depthBuffer;
	depthBuffer.nearPlane = nearPlane;
	depthBuffer.farPlane = farPlane;

	glm::mat4x4 invView = glm::inverse(viewMatrix);

	MatcapBuffer matcapBuffer;
	matcapBuffer.invProj = glm::transpose(glm::inverse(projectionMatrix));
	matcapBuffer.invView = glm::transpose(invView);
	matcapBuffer.lightDirectionVS = glm::mat3(viewMatrix) * glm::normalize(lightDirection); // TODO: test correctness

	glm::mat4x4 viewProjection = projectionMatrix * viewMatrix;

	GridBuffer gridBuffer;
	gridBuffer.invViewProj = glm::transpose(glm::inverse(viewProjection));
	gridBuffer.viewProj = glm::transpose(viewProjection);
	gridBuffer.cameraPosWS = glm::vec3(invView[3]);
	gridBuffer.cellSize = 0.1f;
	gridBuffer.axisColorX = glm::vec3(0.85f, 0.2f, 0.2f);
	gridBuffer.majorLineEvery = 10.0f;
	gridBuffer.axisColorZ = glm::vec3(0.2f, 0.7f, 0.2f);
	gridBuffer.fadeDistance = farPlane;
	gridBuffer.lineColor = glm::vec3(0.4f, 0.4f, 0.4f);
	gridBuffer.pad = 0.0f;

	m_depthPassthruNode->UpdatePSConstantBuffer<DepthBuffer>(deviceContext, depthBuffer, 0);
	m_matcapNode->UpdatePSConstantBuffer<MatcapBuffer>(deviceContext, matcapBuffer, 0);
	m_gridNode->UpdatePSConstantBuffer<GridBuffer>(deviceContext, gridBuffer, 0);
}

void Pipeline::SceneUpdate(ID3D11DeviceContext* deviceContext, glm::mat4x4 worldMatrix, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix)
{
	MatrixBuffer matrixBuffer;
	//matrixBuffer.worldMatrix = glm::transpose(glm::mat4x4(1.0f));
	matrixBuffer.worldMatrix = glm::transpose(worldMatrix); // TODO: replace identity matrix
	matrixBuffer.viewMatrix = glm::transpose(viewMatrix);
	matrixBuffer.projectionMatrix = glm::transpose(projectionMatrix);

	m_geometryNode->UpdateVSConstantBuffer<MatrixBuffer>(deviceContext, matrixBuffer, 0);
	m_geometryNode->UpdatePSConstantBuffer<MatrixBuffer>(deviceContext, matrixBuffer, 0);
}

void Pipeline::RenderGeometryPass(ID3D11DeviceContext* deviceContext, Scene* scene, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix)
{
	// runs once per frame regardless of how many views composite from it
	D3D11_VIEWPORT fullViewport = {};
	fullViewport.TopLeftX = 0.0f;
	fullViewport.TopLeftY = 0.0f;
	fullViewport.Width = static_cast<float>(m_bufferWidth);
	fullViewport.Height = static_cast<float>(m_bufferHeight);
	fullViewport.MinDepth = 0.0f;
	fullViewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &fullViewport);

	deviceContext->OMSetDepthStencilState(nullptr, 0);
	deviceContext->ClearDepthStencilView(m_depthSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	deviceContext->ClearRenderTargetView(m_normalRTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_hatchRTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_matcapRTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_curvatureRTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_hatch2RTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_reliabilityRTV.Get(), CLEAR_COLOR);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, nullSRV);
	// Bind g-buffer RTV and depth
	ID3D11RenderTargetView* gbufferRTVPtr[] = {
		m_normalRTV.Get(),
		m_hatchRTV.Get(),
		m_hatch2RTV.Get(),
		m_reliabilityRTV.Get()
	};
	deviceContext->OMSetRenderTargets(4, gbufferRTVPtr, m_depthSV.Get());
	// Set geometry shader
	//m_geometryNode->Render(deviceContext);


	// Draw model indices
	for (auto& [id, model] : scene->GetModels()) {
		SceneUpdate(deviceContext, model->GetWorldMatrix(), viewMatrix, projectionMatrix);
		model->Render(deviceContext);
		m_geometryNode->Render(deviceContext);
		deviceContext->DrawIndexed(model->GetIndexCount(), 0, 0);
	}
	

	Unbind(deviceContext);

	// Bind depth passthrough
	ID3D11RenderTargetView* depthPassthruRTVPtr = m_depthPassthruRTV.Get();
	deviceContext->OMSetRenderTargets(1, &depthPassthruRTVPtr, nullptr);
	// Bind depth SRV
	ID3D11ShaderResourceView* depthSRVPtr = m_depthSRV.Get();
	deviceContext->PSSetShaderResources(0, 1, &depthSRVPtr);
	// Set depth passthru shader
	m_depthPassthruNode->Render(deviceContext);
	// Draw fullscreen tri
	deviceContext->Draw(3, 0);

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

	RenderCurvaturePass(deviceContext);
	RenderHatchingPass(deviceContext);
}

void Pipeline::RenderCurvaturePass(ID3D11DeviceContext* deviceContext)
{
	// Run the neural prediction for this frame's normal+depth, writing the raw
	// (unnormalized) result + mask into m_inferenceNode's output texture
	m_inferenceNode->Evaluate(deviceContext, m_normalSRV.Get(), m_depthPassthruSRV.Get());

	// Bind curvature RTV
	ID3D11RenderTargetView* curvatureRTVPtr = m_curvatureRTV.Get();
	deviceContext->OMSetRenderTargets(1, &curvatureRTVPtr, nullptr);
	// Bind the raw prediction as input
	ID3D11ShaderResourceView* rawPredictionSRVPtr = m_inferenceNode->GetOutputSRV();
	deviceContext->PSSetShaderResources(0, 1, &rawPredictionSRVPtr);
	// Set curvature postprocess shader (normalize/remap/mask, see curvature_post.hlsl)
	m_curvaturePostNode->Render(deviceContext);
	// Draw fullscreen tri
	deviceContext->Draw(3, 0);

	Unbind(deviceContext);
}

void Pipeline::RenderHatchingPass(ID3D11DeviceContext* deviceContext)
{
	// Each field gets its own trace + draw, sharing only the line shader.
	glm::vec3 inkColor = glm::vec3(0.0f, 0.0f, 0.0f);
	RenderHatchLines(deviceContext, m_hatchTraceNode.get(), m_hatchSRV.Get(), m_hatchLinesRTV.Get(), inkColor);
	RenderHatchLines(deviceContext, m_hatch2TraceNode.get(), m_hatch2SRV.Get(), m_hatch2LinesRTV.Get(), inkColor);
	RenderHatchLines(deviceContext, m_curvatureTraceNode.get(), m_curvatureSRV.Get(), m_curvatureLinesRTV.Get(), inkColor);
}

void Pipeline::RenderHatchLines(ID3D11DeviceContext* deviceContext, HatchTraceNode* tracer, ID3D11ShaderResourceView* fieldSRV, ID3D11RenderTargetView* targetRTV, glm::vec3 inkColor)
{
	// Trace this field's streamlines (writes into the tracer's own line
	// vertex buffer + occupancy grid, clearing them first).
	tracer->Trace(deviceContext, fieldSRV);

	const float whiteClear[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	deviceContext->ClearRenderTargetView(targetRTV, whiteClear); // was CLEAR_COLOR

	D3D11_VIEWPORT fullViewport = {};
	fullViewport.TopLeftX = 0.0f;
	fullViewport.TopLeftY = 0.0f;
	fullViewport.Width = static_cast<float>(m_bufferWidth);
	fullViewport.Height = static_cast<float>(m_bufferHeight);
	fullViewport.MinDepth = 0.0f;
	fullViewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &fullViewport);

	HatchLineBuffer lineBuffer;
	lineBuffer.screenSize = glm::vec2(static_cast<float>(m_bufferWidth), static_cast<float>(m_bufferHeight));
	lineBuffer.lineWidthPx = 0.1f;
	lineBuffer.stepsPerSeed = tracer->GetMaxSteps() - 1;
	lineBuffer.lineColor = inkColor;
	lineBuffer.pad = 0.0f;

	m_hatchLineNode->UpdateVSConstantBuffer<HatchLineBuffer>(deviceContext, lineBuffer, 0);
	m_hatchLineNode->UpdatePSConstantBuffer<HatchLineBuffer>(deviceContext, lineBuffer, 0);

	// Bind output RTV and this field's traced-line buffer for vertex-pulling
	ID3D11RenderTargetView* rtvPtr = targetRTV;
	deviceContext->OMSetRenderTargets(1, &rtvPtr, nullptr);
	ID3D11ShaderResourceView* lineVerticesSRV = tracer->GetLineVertexSRV();
	deviceContext->VSSetShaderResources(0, 1, &lineVerticesSRV);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xffffffff);

	// FIX: hatch_lines.hlsl generates each quad's two triangles with a fixed
	// winding order regardless of segment direction (see HatchLineVertexShader),
	// so with the global CULL_BACK state from Viewport::InitializeRasterizer(),
	// every hatch quad was being culled -- there's no live vs. dead distinction
	// at the rasterizer, they're ALL back-facing. There's no meaningful "back
	// face" for a 2D screen-space line quad, so just disable culling for this draw.
	static ComPtr<ID3D11RasterizerState> s_noCullState;
	if (!s_noCullState)
	{
		ComPtr<ID3D11Device> device;
		deviceContext->GetDevice(device.GetAddressOf());

		D3D11_RASTERIZER_DESC rd = {};
		rd.FillMode = D3D11_FILL_SOLID;
		rd.CullMode = D3D11_CULL_NONE;
		rd.DepthClipEnable = TRUE;
		device->CreateRasterizerState(&rd, s_noCullState.GetAddressOf());
	}
	ComPtr<ID3D11RasterizerState> prevRasterState;
	deviceContext->RSGetState(prevRasterState.GetAddressOf());
	deviceContext->RSSetState(s_noCullState.Get());

	m_hatchLineNode->Render(deviceContext);
	deviceContext->Draw(tracer->GetSegmentCount() * 6, 0);

	deviceContext->RSSetState(prevRasterState.Get());
	deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	ID3D11ShaderResourceView* nullSRV = nullptr;
	deviceContext->VSSetShaderResources(0, 1, &nullSRV);

	Unbind(deviceContext);
}

void Pipeline::CompositeView(ID3D11DeviceContext* deviceContext, int shadingMode, D3D11_VIEWPORT viewportRect, bool drawGrid)
{
	// composites one channel into one rect, call once per view
	ID3D11RenderTargetView* outRTVPtr = m_outRTV.Get();
	deviceContext->OMSetRenderTargets(1, &outRTVPtr, nullptr);
	deviceContext->RSSetViewports(1, &viewportRect);

	ID3D11ShaderResourceView* selectedSRV = GetSRVForShadingMode(shadingMode);
	deviceContext->PSSetShaderResources(0, 1, &selectedSRV);

	m_outNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	Unbind(deviceContext);

	if (drawGrid)
		RenderGrid(deviceContext, viewportRect);
}

void Pipeline::RenderGrid(ID3D11DeviceContext* deviceContext, D3D11_VIEWPORT viewportRect)
{
	// overlays the ground grid on top of the just-composited view, alpha blended and depth-occluded by the model
	ID3D11RenderTargetView* outRTVPtr = m_outRTV.Get();
	deviceContext->OMSetRenderTargets(1, &outRTVPtr, nullptr);
	deviceContext->RSSetViewports(1, &viewportRect);

	ID3D11ShaderResourceView* depthSRVPtr = m_depthSRV.Get();
	deviceContext->PSSetShaderResources(0, 1, &depthSRVPtr);

	float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	deviceContext->OMSetBlendState(m_alphaBlendState.Get(), blendFactor, 0xffffffff);

	m_gridNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	deviceContext->OMSetBlendState(nullptr, blendFactor, 0xffffffff);

	Unbind(deviceContext);
}

ID3D11ShaderResourceView* Pipeline::GetSRVForShadingMode(int shadingMode)
{
	switch (shadingMode)
	{
	case 0: // Matcap
		return m_matcapSRV.Get();
	case 1: // Normal
		return m_normalSRV.Get();
	case 2: // Depth
		return m_depthPassthruSRV.Get();
	case 3: // Cross field
		return m_hatchSRV.Get();
	case 4: // Cross field 2
		return m_hatch2SRV.Get();
	case 5: // Reliability
		return m_reliabilitySRV.Get();
	case 6: // Curvature (neural)
		return m_curvatureSRV.Get();
	case 7: // Cross-hatch (traced)
		return m_hatchLinesSRV.Get();
	case 8: // Cross-hatch 2 (traced)
		return m_hatch2LinesSRV.Get();
	case 9: // Curvature hatch (traced)
		return m_curvatureLinesSRV.Get();
	default:
		return nullptr;
	}
}

#include <filesystem>
#include <fstream>
#include <string>
#include <wincodec.h>
#include "ScreenGrab.h"

void Pipeline::CaptureDatapoint(ID3D11DeviceContext* deviceContext, std::wstring prefix)
{
	std::filesystem::path dir = L"data";
	std::filesystem::create_directories(dir);
	dir = (dir / prefix);
	std::filesystem::create_directories(dir);

	int max = 0;
	for (auto& p : std::filesystem::directory_iterator(dir))
		if (int n; sscanf_s(p.path().filename().string().c_str(), "%d_", &n) == 1)
			max = std::max(max, n);

	ID3D11ShaderResourceView* srv = m_normalSRV.Get();
	ComPtr<ID3D11Resource> res;
	srv->GetResource(&res);
	//if (!res) return false;
	HRESULT hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_normal.png")).c_str()
	);

	// OPTIONAL
	srv = m_matcapSRV.Get();
	srv->GetResource(&res);
	hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_matcap.png")).c_str()
	);

	srv = m_hatchSRV.Get();
	srv->GetResource(&res);
	hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_hatchMax.png")).c_str()
	);

	srv = m_hatch2SRV.Get();
	srv->GetResource(&res);
	hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_hatchMin.png")).c_str()
	);

	srv = m_depthPassthruSRV.Get();
	srv->GetResource(&res);
	hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_depth.png")).c_str()
	);

	/*
	srv = m_reliabilitySRV.Get();
	srv->GetResource(&res);
	hr = DirectX::SaveWICTextureToFile(
		deviceContext,
		res.Get(),
		GUID_ContainerFormatPng,
		(dir / (std::to_wstring(max + 1) + L"_reliable.png")).c_str()
	);
	*/
}

void Pipeline::Unbind(ID3D11DeviceContext* deviceContext)
{
	ID3D11RenderTargetView* nullRTV[] = {nullptr, nullptr};
	deviceContext->OMSetRenderTargets(2, nullRTV, nullptr);
}

bool Pipeline::CreateRenderTarget(ID3D11Device* device, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv, int textureWidth, int textureHeight)
{
	ComPtr<ID3D11Texture2D> texture;

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

	HRESULT result = device->CreateTexture2D(&td, nullptr, texture.GetAddressOf());
	if (FAILED(result))
		return false;

	// RTV
	result = device->CreateRenderTargetView(texture.Get(), nullptr, rtv);
	if (FAILED(result))
		return false;

	// SRV
	result = device->CreateShaderResourceView(texture.Get(), nullptr, srv);
	if (FAILED(result))
		return false;

	return true;
}

bool Pipeline::InitializeBlendState(ID3D11Device* device)
{
	D3D11_BLEND_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.RenderTarget[0].BlendEnable = TRUE;
	bd.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	bd.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	HRESULT result = device->CreateBlendState(&bd, m_alphaBlendState.GetAddressOf());
	if (FAILED(result))
		return false;

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

	ComPtr<ID3D11Texture2D> depthTexture;
	HRESULT result = device->CreateTexture2D(&depthDesc, nullptr, depthTexture.GetAddressOf());
	if (FAILED(result))
		return false;

	// Create depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
	ZeroMemory(&dsvDesc, sizeof(dsvDesc));
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	device->CreateDepthStencilView(depthTexture.Get(), &dsvDesc, &m_depthSV);

	// Create shader resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	device->CreateShaderResourceView(depthTexture.Get(), &srvDesc, m_depthSRV.GetAddressOf());

	return true;
}

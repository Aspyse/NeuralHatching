#include "Pipeline.h"
#include <glm/gtc/matrix_transform.hpp>

void Pipeline::Initialize(ID3D11Device* device, ID3D11RenderTargetView* outRTV, int textureWidth, int textureHeight)
{
	m_outRTV = outRTV;
	m_bufferWidth = textureWidth;
	m_bufferHeight = textureHeight;

	bool result = CreateRenderTarget(device, m_normalRTV.GetAddressOf(), m_normalSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_depthPassthruRTV.GetAddressOf(), m_depthPassthruSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_albedoRTV.GetAddressOf(), m_albedoSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_roughnessRTV.GetAddressOf(), m_roughnessSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_hatchRTV.GetAddressOf(), m_hatchSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2RTV.GetAddressOf(), m_hatch2SRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_reliabilityRTV.GetAddressOf(), m_reliabilitySRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_matcapRTV.GetAddressOf(), m_matcapSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureRTV.GetAddressOf(), m_curvatureSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_edgeRTV.GetAddressOf(), m_edgeSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_flatCompositeRTV.GetAddressOf(), m_flatCompositeSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_flatComposite2RTV.GetAddressOf(), m_flatComposite2SRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_flatCompositeCurvatureRTV.GetAddressOf(), m_flatCompositeCurvatureSRV.GetAddressOf(), textureWidth, textureHeight);

	result = CreateRenderTarget(device, m_hatchLinesRTV.GetAddressOf(), m_hatchLinesSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2LinesRTV.GetAddressOf(), m_hatch2LinesSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureLinesRTV.GetAddressOf(), m_curvatureLinesSRV.GetAddressOf(), textureWidth, textureHeight);

	// NdotL * shadow visibility, shared by the 3 masked hatch outputs below
	result = CreateRenderTarget(device, m_litMaskRTV.GetAddressOf(), m_litMaskSRV.GetAddressOf(), textureWidth, textureHeight);

	// each traced hatch layer x m_litMaskSRV
	result = CreateRenderTarget(device, m_hatchMaskedRTV.GetAddressOf(), m_hatchMaskedSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2MaskedRTV.GetAddressOf(), m_hatch2MaskedSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureMaskedRTV.GetAddressOf(), m_curvatureMaskedSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatchOutlinedRTV.GetAddressOf(), m_hatchOutlinedSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_hatch2OutlinedRTV.GetAddressOf(), m_hatch2OutlinedSRV.GetAddressOf(), textureWidth, textureHeight);
	result = CreateRenderTarget(device, m_curvatureOutlinedRTV.GetAddressOf(), m_curvatureOutlinedSRV.GetAddressOf(), textureWidth, textureHeight);

	// debug only, not part of the normal shading chain
	result = CreateRenderTarget(device, m_shadowDebugRTV.GetAddressOf(), m_shadowDebugSRV.GetAddressOf(), textureWidth, textureHeight);

	m_geometryNode = std::make_unique<GeometryNode>();
	m_geometryNode->Initialize(device, L"Shaders/geometry.hlsl", L"Shaders/geometry.hlsl", "GeometryVertexShader", "GeometryPixelShader");

	m_depthPassthruNode = std::make_unique<Node>();
	m_depthPassthruNode->Initialize(device, L"Shaders/linearize.hlsl", L"Shaders/linearize.hlsl", "BaseVertexShader", "PostprocessShader");

	m_matcapNode = std::make_unique<Node>();
	m_matcapNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/matcap.hlsl", "BaseVertexShader", "PostprocessShader");

	m_flatCompositeNode = std::make_unique<Node>();
	m_flatCompositeNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/flat_composite.hlsl", "BaseVertexShader", "FlatCompositePixelShader");

	m_outNode = std::make_unique<Node>();
	m_outNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/fullscreen.hlsl", "BaseVertexShader", "PostprocessShader");

	m_gridNode = std::make_unique<Node>();
	m_gridNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/grid.hlsl", "BaseVertexShader", "PostprocessShader");

	m_curvaturePostNode = std::make_unique<Node>();
	m_curvaturePostNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/curvature_post.hlsl", "BaseVertexShader", "PostprocessShader");

	m_edgeNode = std::make_unique<Node>();
	m_edgeNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/geometry_edges.hlsl", "BaseVertexShader", "PostprocessShader");

	m_inferenceNode = std::make_unique<InferenceNode>();
	m_inferenceNode->Initialize(device, L"Weights/neural_hatch_v3_fp16.onnx", textureWidth, textureHeight);

	// vertex-pulls whichever field is bound and draws them
	m_hatchLineNode = std::make_unique<Node>();
	m_hatchLineNode->Initialize(device, L"Shaders/hatch_lines.hlsl", L"Shaders/hatch_lines.hlsl", "HatchLineVertexShader", "HatchLinePixelShader");

	// depth-only pass from the light's POV, reuses GeometryNode's input layout
	m_shadowDepthNode = std::make_unique<GeometryNode>();
	m_shadowDepthNode->Initialize(device, L"Shaders/shadow_depth.hlsl", L"Shaders/shadow_depth.hlsl", "ShadowVertexShader", "ShadowPixelShader");

	// NdotL * shadow visibility, shared across all three hatch layers
	m_litMaskNode = std::make_unique<Node>();
	m_litMaskNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/lit_mask.hlsl", "BaseVertexShader", "LitMaskPixelShader");

	// masks one hatch layer's traced-line texture with m_litMaskSRV; run 3x
	m_hatchMaskNode = std::make_unique<Node>();
	m_hatchMaskNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/hatch_mask.hlsl", "BaseVertexShader", "HatchMaskPixelShader");

	m_hatchOutlineNode = std::make_unique<Node>();
	m_hatchOutlineNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/hatch_outline_composite.hlsl", "BaseVertexShader", "OutlineCompositePixelShader");

	// false-color lit/shadowed/out-of-bounds view, shares LitMaskBuffer with m_litMaskNode
	m_shadowDebugNode = std::make_unique<Node>();
	m_shadowDebugNode->Initialize(device, L"Shaders/fullscreen.hlsl", L"Shaders/shadow_debug.hlsl", "BaseVertexShader", "ShadowDebugPixelShader");

	// one tracer per direction field
	m_hatchTraceNode = std::make_unique<HatchTraceNode>();
	m_hatchTraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 1u);

	m_hatch2TraceNode = std::make_unique<HatchTraceNode>();
	m_hatch2TraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 2u);

	m_curvatureTraceNode = std::make_unique<HatchTraceNode>();
	m_curvatureTraceNode->Initialize(device, L"Shaders/hatch.hlsl", "CSMain", 0.01f, 0.008f, 48, 0.0035f, 256, 256, 3u);

	m_geometryNode->AddVSConstantBuffer<MatrixBuffer>(device);
	m_geometryNode->AddPSConstantBuffer<MatrixBuffer>(device);
	m_geometryNode->AddPSConstantBuffer<MaterialBuffer>(device);
	m_depthPassthruNode->AddPSConstantBuffer<DepthBuffer>(device);
	m_matcapNode->AddPSConstantBuffer<MatcapBuffer>(device);
	m_gridNode->AddPSConstantBuffer<GridBuffer>(device);
	m_edgeNode->AddPSConstantBuffer<EdgeBuffer>(device);
	m_hatchLineNode->AddVSConstantBuffer<HatchLineBuffer>(device);
	m_hatchLineNode->AddPSConstantBuffer<HatchLineBuffer>(device);

	m_shadowDepthNode->AddVSConstantBuffer<ShadowMatrixBuffer>(device);
	m_litMaskNode->AddPSConstantBuffer<LitMaskBuffer>(device);
	m_shadowDebugNode->AddPSConstantBuffer<LitMaskBuffer>(device);
	// m_hatchMaskNode takes no constant buffer -- both its inputs are textures

	InitializeDepthTarget(device, textureWidth, textureHeight);
	InitializeShadowMap(device, m_shadowMapSize);
	InitializeSamplers(device);
	InitializeShadowSampler(device);
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

	EdgeBuffer edgeBuffer;
	edgeBuffer.screenSize = glm::vec2(static_cast<float>(m_bufferWidth), static_cast<float>(m_bufferHeight));
	edgeBuffer.normalThreshold = 0.35f;
	edgeBuffer.depthThreshold = 0.015f;
	edgeBuffer.nearFar = glm::vec2(nearPlane, farPlane);
	edgeBuffer.pad = glm::vec2(0.0f);
	m_edgeNode->UpdatePSConstantBuffer<EdgeBuffer>(deviceContext, edgeBuffer, 0);

	// Directional light: orthographic frustum around the scene.
	// Model::Autonormalize() fits every model to a unit sphere at the origin (see Model.cpp),
	// so a radius-1 sphere at the origin bounds the scene exactly -- no more guessing.
	const float sceneRadius = 1.0f;
	glm::vec3 lightDirWS = glm::normalize(lightDirection);
	glm::vec3 sceneCenter = glm::vec3(0.0f);
	glm::vec3 up = (std::abs(lightDirWS.y) > 0.99f) ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 lightEye = sceneCenter - lightDirWS * (sceneRadius * 2.0f);

	m_lightViewMatrix = glm::lookAt(lightEye, sceneCenter, up);
	// eye sits at 2*radius from center, so the sphere spans [radius, 3*radius] along the view axis --
	// near/far follow that exactly instead of the old fixed 0.1f, tightening depth precision.
	m_lightProjectionMatrix = glm::orthoZO(
		-sceneRadius, sceneRadius,
		-sceneRadius, sceneRadius,
		sceneRadius * 0.5f, sceneRadius * 3.5f);

	LitMaskBuffer litMaskBuffer;
	litMaskBuffer.invProj = matcapBuffer.invProj;
	litMaskBuffer.invView = matcapBuffer.invView;
	litMaskBuffer.lightViewProj = glm::transpose(m_lightProjectionMatrix * m_lightViewMatrix);
	litMaskBuffer.lightDirectionVS = matcapBuffer.lightDirectionVS;
	litMaskBuffer.shadowBias = 0.003f;
	litMaskBuffer.shadowMappingEnabled = m_shadowMappingEnabled ? 1u : 0u;
	litMaskBuffer.shadowPad = glm::vec3(0.0f);

	m_litMaskNode->UpdatePSConstantBuffer<LitMaskBuffer>(deviceContext, litMaskBuffer, 0);
	m_shadowDebugNode->UpdatePSConstantBuffer<LitMaskBuffer>(deviceContext, litMaskBuffer, 0);
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
	if (m_shadowMappingEnabled)
		RenderShadowPass(deviceContext, scene);

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
	deviceContext->ClearRenderTargetView(m_albedoRTV.Get(), CLEAR_COLOR);
	deviceContext->ClearRenderTargetView(m_roughnessRTV.Get(), CLEAR_COLOR);

	ID3D11ShaderResourceView* nullSRV[] = { nullptr, nullptr, nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 4, nullSRV);
	// Bind g-buffer RTV and depth
	ID3D11RenderTargetView* gbufferRTVPtr[] = {
		m_normalRTV.Get(),
		m_hatchRTV.Get(),
		m_hatch2RTV.Get(),
		m_reliabilityRTV.Get(),
		m_albedoRTV.Get(),
		m_roughnessRTV.Get()
	};
	deviceContext->OMSetRenderTargets(6, gbufferRTVPtr, m_depthSV.Get());
	deviceContext->PSSetSamplers(0, 1, m_materialSampler.GetAddressOf());
	// Set geometry shader
	//m_geometryNode->Render(deviceContext);


	// Draw model indices
	for (auto& [id, model] : scene->GetModels()) {
		SceneUpdate(deviceContext, model->GetWorldMatrix(), viewMatrix, projectionMatrix);
		model->Render(deviceContext);
		m_geometryNode->Render(deviceContext);

		for (const auto& meshPart : model->GetMeshParts()) {
			if (!meshPart.opaque) continue;

			MaterialBuffer materialBuffer;
			materialBuffer.albedo = meshPart.albedo;
			materialBuffer.roughness = meshPart.roughness;
			materialBuffer.hasAlbedoTexture = meshPart.albedoTexture ? 1u : 0u;
			materialBuffer.hasRoughnessTexture = meshPart.roughnessTexture ? 1u : 0u;
			materialBuffer.pad = 0.0f;
			m_geometryNode->UpdatePSConstantBuffer<MaterialBuffer>(deviceContext, materialBuffer, 1);

			ID3D11ShaderResourceView* materialSRVPtr[] = {
				meshPart.albedoTexture.Get(),
				meshPart.roughnessTexture.Get()
			};
			deviceContext->PSSetShaderResources(0, 2, materialSRVPtr);
			deviceContext->DrawIndexed(meshPart.indexCount, meshPart.indexOffset, 0);
		}
	}
	

	Unbind(deviceContext);

	// Bind depth passthrough
	deviceContext->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
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

	RenderEdgePass(deviceContext);

	// Bind matcap RTV
	ID3D11RenderTargetView* matcapRTVPtr = m_matcapRTV.Get();
	deviceContext->OMSetRenderTargets(1, &matcapRTVPtr, nullptr);
	// Bind g-buffer SRVs
	ID3D11ShaderResourceView* gbufferSRVPtr[] = {
		m_normalSRV.Get(),
		m_depthSRV.Get(),
		m_albedoSRV.Get(),
		m_roughnessSRV.Get()
	};
	deviceContext->PSSetShaderResources(0, 4, gbufferSRVPtr);
	// Set matcap shader
	m_matcapNode->Render(deviceContext);
	// Draw fullscreen tri
	deviceContext->Draw(3, 0);

	Unbind(deviceContext);

	RenderCurvaturePass(deviceContext);
	RenderHatchingPass(deviceContext);

	// needs normal+depth from above and the shadow map from RenderShadowPass
	RenderLitMaskPass(deviceContext);
	// same inputs as above, just a debug view -- cheap enough to always run
	RenderShadowDebugPass(deviceContext);
	// needs the traced hatch textures + the mask from RenderLitMaskPass
	RenderHatchMaskPass(deviceContext);
	RenderHatchOutlinePass(deviceContext);
	RenderFlatCompositePass(deviceContext, m_hatchMaskedSRV.Get(), m_flatCompositeRTV.Get());
	RenderFlatCompositePass(deviceContext, m_hatch2MaskedSRV.Get(), m_flatComposite2RTV.Get());
	RenderFlatCompositePass(deviceContext, m_curvatureMaskedSRV.Get(), m_flatCompositeCurvatureRTV.Get());
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

void Pipeline::RenderEdgePass(ID3D11DeviceContext* deviceContext)
{
	ID3D11RenderTargetView* edgeRTVPtr = m_edgeRTV.Get();
	deviceContext->OMSetRenderTargets(1, &edgeRTVPtr, nullptr);
	ID3D11ShaderResourceView* edgeInputs[] = {
		m_normalSRV.Get(),
		m_depthSRV.Get()
	};
	deviceContext->PSSetShaderResources(0, 2, edgeInputs);
	m_edgeNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, nullSRVs);
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
	lineBuffer.lineWidthPx = 0.01f;
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

void Pipeline::RenderShadowPass(ID3D11DeviceContext* deviceContext, Scene* scene)
{
	D3D11_VIEWPORT shadowViewport = {};
	shadowViewport.TopLeftX = 0.0f;
	shadowViewport.TopLeftY = 0.0f;
	shadowViewport.Width = static_cast<float>(m_shadowMapSize);
	shadowViewport.Height = static_cast<float>(m_shadowMapSize);
	shadowViewport.MinDepth = 0.0f;
	shadowViewport.MaxDepth = 1.0f;
	deviceContext->RSSetViewports(1, &shadowViewport);

	deviceContext->ClearDepthStencilView(m_shadowDepthDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	// no RTV bound, ShadowPixelShader writes nothing
	deviceContext->OMSetRenderTargets(0, nullptr, m_shadowDepthDSV.Get());

	for (auto& [id, model] : scene->GetModels())
	{
		ShadowMatrixBuffer shadowBuffer;
		shadowBuffer.worldMatrix = glm::transpose(model->GetWorldMatrix());
		shadowBuffer.lightViewMatrix = glm::transpose(m_lightViewMatrix);
		shadowBuffer.lightProjectionMatrix = glm::transpose(m_lightProjectionMatrix);

		m_shadowDepthNode->UpdateVSConstantBuffer<ShadowMatrixBuffer>(deviceContext, shadowBuffer, 0);

		model->Render(deviceContext);
		m_shadowDepthNode->Render(deviceContext);
		deviceContext->DrawIndexed(model->GetIndexCount(), 0, 0);
	}

	deviceContext->OMSetRenderTargets(0, nullptr, nullptr);
}

void Pipeline::RenderLitMaskPass(ID3D11DeviceContext* deviceContext)
{
	ID3D11RenderTargetView* litMaskRTVPtr = m_litMaskRTV.Get();
	deviceContext->OMSetRenderTargets(1, &litMaskRTVPtr, nullptr);

	// t0 normal, t1 depth, t2 shadow map
	ID3D11ShaderResourceView* srvs[] = { m_normalSRV.Get(), m_depthSRV.Get(), m_shadowDepthSRV.Get() };
	deviceContext->PSSetShaderResources(0, 3, srvs);

	// s0 assumed already bound globally like the other postprocess passes
	deviceContext->PSSetSamplers(1, 1, m_shadowSampler.GetAddressOf());

	m_litMaskNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 3, nullSRVs);

	Unbind(deviceContext);
}

void Pipeline::RenderShadowDebugPass(ID3D11DeviceContext* deviceContext)
{
	ID3D11RenderTargetView* debugRTVPtr = m_shadowDebugRTV.Get();
	deviceContext->OMSetRenderTargets(1, &debugRTVPtr, nullptr);

	// t0 normal, t1 depth, t2 shadow map -- same as RenderLitMaskPass
	ID3D11ShaderResourceView* srvs[] = { m_normalSRV.Get(), m_depthSRV.Get(), m_shadowDepthSRV.Get() };
	deviceContext->PSSetShaderResources(0, 3, srvs);

	deviceContext->PSSetSamplers(1, 1, m_shadowSampler.GetAddressOf());

	m_shadowDebugNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 3, nullSRVs);

	Unbind(deviceContext);
}

void Pipeline::RenderHatchMaskPass(ID3D11DeviceContext* deviceContext)
{
	RenderHatchMask(deviceContext, m_hatchLinesSRV.Get(), m_hatchMaskedRTV.Get());
	RenderHatchMask(deviceContext, m_hatch2LinesSRV.Get(), m_hatch2MaskedRTV.Get());
	RenderHatchMask(deviceContext, m_curvatureLinesSRV.Get(), m_curvatureMaskedRTV.Get());
}

void Pipeline::RenderHatchMask(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchLinesSRV, ID3D11RenderTargetView* maskedRTV)
{
	ID3D11RenderTargetView* rtvPtr = maskedRTV;
	deviceContext->OMSetRenderTargets(1, &rtvPtr, nullptr);

	ID3D11ShaderResourceView* srvs[] = { hatchLinesSRV, m_litMaskSRV.Get() };
	deviceContext->PSSetShaderResources(0, 2, srvs);

	m_hatchMaskNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, nullSRVs);

	Unbind(deviceContext);
}

void Pipeline::RenderHatchOutlinePass(ID3D11DeviceContext* deviceContext)
{
	RenderHatchOutline(deviceContext, m_hatchMaskedSRV.Get(), m_hatchOutlinedRTV.Get());
	RenderHatchOutline(deviceContext, m_hatch2MaskedSRV.Get(), m_hatch2OutlinedRTV.Get());
	RenderHatchOutline(deviceContext, m_curvatureMaskedSRV.Get(), m_curvatureOutlinedRTV.Get());
}

void Pipeline::RenderFlatCompositePass(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchSRV, ID3D11RenderTargetView* targetRTV)
{
	ID3D11RenderTargetView* rtv = targetRTV;
	deviceContext->OMSetRenderTargets(1, &rtv, nullptr);

	ID3D11ShaderResourceView* inputs[] = {
		m_albedoSRV.Get(),
		m_edgeSRV.Get(),
		hatchSRV,
		m_depthSRV.Get()
	};
	deviceContext->PSSetShaderResources(0, 4, inputs);
	deviceContext->PSSetSamplers(0, 1, m_sampler.GetAddressOf());
	m_flatCompositeNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[4] = {};
	deviceContext->PSSetShaderResources(0, 4, nullSRVs);
	Unbind(deviceContext);
}

void Pipeline::RenderHatchOutline(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchSRV, ID3D11RenderTargetView* outlinedRTV)
{
	ID3D11RenderTargetView* rtvPtr = outlinedRTV;
	deviceContext->OMSetRenderTargets(1, &rtvPtr, nullptr);

	ID3D11ShaderResourceView* srvs[] = { hatchSRV, m_edgeSRV.Get() };
	deviceContext->PSSetShaderResources(0, 2, srvs);

	m_hatchOutlineNode->Render(deviceContext);
	deviceContext->Draw(3, 0);

	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	deviceContext->PSSetShaderResources(0, 2, nullSRVs);

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
	case 10: // Cross-hatch, lit/unlit masked
		return m_hatchMaskedSRV.Get();
	case 11: // Cross-hatch 2, lit/unlit masked
		return m_hatch2MaskedSRV.Get();
	case 12: // Curvature hatch, lit/unlit masked
		return m_curvatureMaskedSRV.Get();
	case 13: // Lit mask alone (NdotL * shadow visibility)
		return m_litMaskSRV.Get();
	case 14: // false-color shadow debug: green lit, red shadowed, blue out of the light frustum
		return m_shadowDebugSRV.Get();
	case 15: // geometry edges, black ink on white
		return m_edgeSRV.Get();
	case 16: // masked cross-hatch with geometry edges on top
		return m_hatchOutlinedSRV.Get();
	case 17: // masked secondary cross-hatch with geometry edges on top
		return m_hatch2OutlinedSRV.Get();
	case 18: // masked neural curvature hatch with geometry edges on top
		return m_curvatureOutlinedSRV.Get();
	case 19: // flat albedo with masked cross-hatch and geometry edges
		return m_flatCompositeSRV.Get();
	case 20: // flat albedo with masked secondary cross-hatch and geometry edges
		return m_flatComposite2SRV.Get();
	case 21: // flat albedo with masked neural curvature hatch and geometry edges
		return m_flatCompositeCurvatureSRV.Get();
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

bool Pipeline::InitializeShadowMap(ID3D11Device* device, int shadowMapSize)
{
	D3D11_TEXTURE2D_DESC shadowDesc = {};
	shadowDesc.Width = shadowMapSize;
	shadowDesc.Height = shadowMapSize;
	shadowDesc.MipLevels = 1;
	shadowDesc.ArraySize = 1;
	shadowDesc.SampleDesc.Count = 1;
	shadowDesc.Usage = D3D11_USAGE_DEFAULT;
	shadowDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	shadowDesc.Format = DXGI_FORMAT_R24G8_TYPELESS; // same split as InitializeDepthTarget above

	ComPtr<ID3D11Texture2D> shadowTexture;
	HRESULT result = device->CreateTexture2D(&shadowDesc, nullptr, shadowTexture.GetAddressOf());
	if (FAILED(result))
		return false;

	D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Texture2D.MipSlice = 0;

	result = device->CreateDepthStencilView(shadowTexture.Get(), &dsvDesc, m_shadowDepthDSV.GetAddressOf());
	if (FAILED(result))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.MostDetailedMip = 0;

	result = device->CreateShaderResourceView(shadowTexture.Get(), &srvDesc, m_shadowDepthSRV.GetAddressOf());
	if (FAILED(result))
		return false;

	m_shadowMapSize = shadowMapSize;
	return true;
}

bool Pipeline::InitializeSamplers(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	sd.MaxLOD = D3D11_FLOAT32_MAX;

	HRESULT result = device->CreateSamplerState(&sd, m_sampler.GetAddressOf());
	if (FAILED(result))
		return false;

	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	result = device->CreateSamplerState(&sd, m_materialSampler.GetAddressOf());
	return SUCCEEDED(result);
}

bool Pipeline::InitializeShadowSampler(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	// border = lit, so sampling outside the frustum doesn't read as shadowed
	sd.BorderColor[0] = 1.0f;
	sd.BorderColor[1] = 1.0f;
	sd.BorderColor[2] = 1.0f;
	sd.BorderColor[3] = 1.0f;
	sd.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;

	HRESULT result = device->CreateSamplerState(&sd, m_shadowSampler.GetAddressOf());
	return SUCCEEDED(result);
}

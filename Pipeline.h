#pragma once

#include <d3d11.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <wrl/client.h>
#include "Node.h"
#include "GeometryNode.h"
#include "InferenceNode.h"
#include "HatchTraceNode.h"
#include "Scene.h"

using Microsoft::WRL::ComPtr;

class Pipeline
{
private:
	struct MatrixBuffer
	{
		glm::mat4x4 worldMatrix;
		glm::mat4x4 viewMatrix;
		glm::mat4x4 projectionMatrix;
	};
	struct DepthBuffer
	{
		float nearPlane;
		float farPlane;
		glm::vec2 pad;
	};
	struct MatcapBuffer
	{
		glm::mat4x4 invProj;
		glm::mat4x4 invView;
		glm::vec3 lightDirectionVS;
		float pad;
	};
	struct MaterialBuffer
	{
		glm::vec4 albedo;
		float roughness;
		UINT hasAlbedoTexture;
		UINT hasRoughnessTexture;
		float pad;
	};
	struct GridBuffer
	{
		glm::mat4x4 invViewProj;
		glm::mat4x4 viewProj;

		glm::vec3 cameraPosWS;
		float cellSize;

		glm::vec3 axisColorX;
		float majorLineEvery;

		glm::vec3 axisColorZ;
		float fadeDistance;

		glm::vec3 lineColor;
		float pad;
	};
	struct HatchLineBuffer
	{
		glm::vec2 screenSize;
		float lineWidthPx;
		int stepsPerSeed;

		glm::vec3 lineColor;
		float pad;
	};
	struct ShadowMatrixBuffer
	{
		glm::mat4x4 worldMatrix;
		glm::mat4x4 lightViewMatrix;
		glm::mat4x4 lightProjectionMatrix;
	};
	struct LitMaskBuffer
	{
		glm::mat4x4 invProj;
		glm::mat4x4 invView;
		glm::mat4x4 lightViewProj;

		glm::vec3 lightDirectionVS;
		float shadowBias;
		UINT shadowMappingEnabled;
		glm::vec3 shadowPad;
	};
	struct EdgeBuffer
	{
		glm::vec2 screenSize;
		float normalThreshold;
		float depthThreshold;
		glm::vec2 nearFar;
		glm::vec2 pad;
	};

public:
	Pipeline() {};
	~Pipeline() {};

	void Initialize(ID3D11Device* device, ID3D11RenderTargetView* outRTV, int textureWidth, int textureHeight);

	void Update(ID3D11DeviceContext* deviceContext, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, glm::vec3 lightDirection, float nearPlane, float farPlane);
	//void Render(ID3D11DeviceContext* deviceContext, Scene* scene, int shadingMode, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix);
	void RenderGeometryPass(ID3D11DeviceContext* deviceContext, Scene* scene, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix);
	void CompositeView(ID3D11DeviceContext* deviceContext, int shadingMode, D3D11_VIEWPORT viewportRect, bool drawGrid = true);
	void SetShadowMappingEnabled(bool enabled) { m_shadowMappingEnabled = enabled; }

	void CaptureDatapoint(ID3D11DeviceContext* deviceContext, std::wstring prefix = L"");

private:
	// SRV RTV helper
	bool CreateRenderTarget(ID3D11Device* device, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv, int textureWidth, int textureHeight);

	bool InitializeDepthTarget(ID3D11Device* device, int textureWidth, int textureHeight);

	bool InitializeBlendState(ID3D11Device* device);

	bool InitializeShadowMap(ID3D11Device* device, int shadowMapSize);
	bool InitializeSamplers(ID3D11Device* device);
	bool InitializeShadowSampler(ID3D11Device* device);

	void SceneUpdate(ID3D11DeviceContext* deviceContext, glm::mat4x4 worldMatrix, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix);

	void RenderGrid(ID3D11DeviceContext* deviceContext, D3D11_VIEWPORT viewportRect);

	void RenderCurvaturePass(ID3D11DeviceContext* deviceContext);
	void RenderEdgePass(ID3D11DeviceContext* deviceContext);

	void RenderHatchingPass(ID3D11DeviceContext* deviceContext);
	void RenderHatchLines(ID3D11DeviceContext* deviceContext, HatchTraceNode* tracer, ID3D11ShaderResourceView* fieldSRV, ID3D11RenderTargetView* targetRTV, glm::vec3 inkColor);

	void RenderShadowPass(ID3D11DeviceContext* deviceContext, Scene* scene);
	void RenderLitMaskPass(ID3D11DeviceContext* deviceContext);
	void RenderShadowDebugPass(ID3D11DeviceContext* deviceContext);
	void RenderHatchMaskPass(ID3D11DeviceContext* deviceContext);
	void RenderHatchMask(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchLinesSRV, ID3D11RenderTargetView* maskedRTV);
	void RenderHatchOutlinePass(ID3D11DeviceContext* deviceContext);
	void RenderHatchOutline(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchSRV, ID3D11RenderTargetView* outlinedRTV);
	void RenderFlatCompositePass(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* hatchSRV, ID3D11RenderTargetView* targetRTV);

	void Unbind(ID3D11DeviceContext* deviceContext);

	ID3D11ShaderResourceView* GetSRVForShadingMode(int shadingMode);

private:
	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	int m_bufferWidth, m_bufferHeight;
	bool m_shadowMappingEnabled = true;

	ComPtr<ID3D11SamplerState> m_sampler, m_materialSampler;
	ComPtr<ID3D11BlendState> m_alphaBlendState;

	ComPtr<ID3D11RenderTargetView> m_normalRTV, m_hatchRTV, m_hatch2RTV, m_matcapRTV, m_outRTV, m_depthPassthruRTV, m_reliabilityRTV, m_albedoRTV, m_roughnessRTV, m_curvatureRTV, m_edgeRTV, m_flatCompositeRTV, m_flatComposite2RTV, m_flatCompositeCurvatureRTV;
	ComPtr<ID3D11ShaderResourceView> m_normalSRV, m_hatchSRV,m_hatch2SRV, m_depthSRV, m_matcapSRV, m_depthPassthruSRV, m_reliabilitySRV, m_albedoSRV, m_roughnessSRV, m_curvatureSRV, m_edgeSRV, m_flatCompositeSRV, m_flatComposite2SRV, m_flatCompositeCurvatureSRV;
	ComPtr<ID3D11DepthStencilView> m_depthSV;

	// Traced + drawn hatch lines, one output per direction field.
	ComPtr<ID3D11RenderTargetView> m_hatchLinesRTV, m_hatch2LinesRTV, m_curvatureLinesRTV;
	ComPtr<ID3D11ShaderResourceView> m_hatchLinesSRV, m_hatch2LinesSRV, m_curvatureLinesSRV;

	// NdotL * shadow visibility, and each hatch layer masked by it
	ComPtr<ID3D11RenderTargetView> m_litMaskRTV, m_hatchMaskedRTV, m_hatch2MaskedRTV, m_curvatureMaskedRTV;
	ComPtr<ID3D11ShaderResourceView> m_litMaskSRV, m_hatchMaskedSRV, m_hatch2MaskedSRV, m_curvatureMaskedSRV;

	ComPtr<ID3D11RenderTargetView> m_hatchOutlinedRTV, m_hatch2OutlinedRTV, m_curvatureOutlinedRTV;
	ComPtr<ID3D11ShaderResourceView> m_hatchOutlinedSRV, m_hatch2OutlinedSRV, m_curvatureOutlinedSRV;

	// false-color lit/shadowed/out-of-bounds view, debug only
	ComPtr<ID3D11RenderTargetView> m_shadowDebugRTV;
	ComPtr<ID3D11ShaderResourceView> m_shadowDebugSRV;

	// shadow map (light-space depth)
	ComPtr<ID3D11DepthStencilView> m_shadowDepthDSV;
	ComPtr<ID3D11ShaderResourceView> m_shadowDepthSRV;
	ComPtr<ID3D11SamplerState> m_shadowSampler;
	int m_shadowMapSize = 2048;
	glm::mat4x4 m_lightViewMatrix, m_lightProjectionMatrix;

	std::unique_ptr<GeometryNode> m_geometryNode, m_shadowDepthNode;
	std::unique_ptr<Node> m_matcapNode, m_flatCompositeNode, m_outNode, m_depthPassthruNode, m_gridNode, m_curvaturePostNode, m_edgeNode, m_hatchLineNode, m_litMaskNode, m_hatchMaskNode, m_hatchOutlineNode, m_shadowDebugNode;
	std::unique_ptr<InferenceNode> m_inferenceNode;
	std::unique_ptr<HatchTraceNode> m_hatchTraceNode, m_hatch2TraceNode, m_curvatureTraceNode;
};

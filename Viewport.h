#pragma once

#include <d3d11.h>
#include <glm/mat4x4.hpp>
#include <memory>
#include <windows.h>
#include <wrl/client.h>
#include "Model.h"
#include "Scene.h"
#include "Pipeline.h"

using Microsoft::WRL::ComPtr;

const int N_SHADING_MODES = 22;
inline const char* SHADING_MODE_NAMES[N_SHADING_MODES] = {
	"Matcap",
	"Normals",
	"Depth",
	"Curvature",
	"Curvature (Minimum)",
	"Reliability",
	"Curvature (Neural)",
	"Cross-Hatch",
	"Cross-Hatch (Minimum)",
	"Cross-Hatch (Neural)",
	"Masked Cross-Hatch",
	"Masked Cross-Hatch (Minimum)",
	"Masked Cross-Hatch (Neural)",
	"Lit-Mask",
	"Shadow False Color",
	"Geometry Edges",
	"Outlined Cross-Hatch",
	"Outlined Cross-Hatch (Minimum)",
	"Outlined Cross-Hatch (Neural)",
	"Albedo + Edges + Hatch",
	"Albedo + Edges + Hatch (Minimum)",
	"Albedo + Edges + Hatch (Neural)"
};
enum class ShadingMode : int
{
	Matcap = 0,
	Normal = 1,
	Depth = 2,
	Crossfield = 3,
	Crossfield2 = 4,
	Reliability = 5,
	Curvature = 6,
	CrossHatch = 7,
	CrossHatch2 = 8,
	CurvatureHatch = 9,
	CrossHatchMasked = 10,
	CrossHatch2Masked = 11,
	CurvatureHatchMasked = 12,
	LitMask = 13,
	ShadowDebug = 14,
	GeometryEdges = 15,
	CrossHatchOutlined = 16,
	CrossHatch2Outlined = 17,
	CurvatureHatchOutlined = 18,
	FlatComposite = 19,
	FlatComposite2 = 20,
	FlatCompositeCurvature = 21
};
enum class LayoutMode : int
{
	Single = 0,
	Grid2x2 = 1
};

struct View
{
	D3D11_VIEWPORT rect{};
	ShadingMode shadingMode = ShadingMode::Matcap;
};

class Viewport
{
public:
	Viewport();
	~Viewport();

	bool Initialize(HWND hWnd, WNDCLASSEXW, float nearPlane, float farPlane, int gBufferWidth, int gBufferHeight);
	void Shutdown();
	bool Render(glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, Scene* scene);

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetContext();

	ShadingMode GetShadingMode(int cellIndex);
	void SetShadingMode(int cellIndex, ShadingMode mode);

	const D3D11_VIEWPORT& GetViewRect(int cellIndex) const;

	LayoutMode GetLayoutMode() const;
	void SetLayoutMode(LayoutMode mode);

	int GetViewCount() const;

	void SetTopInset(float inset);

	const glm::vec3& GetLightDirection() const;
	void SetLightDirection(const glm::vec3& dir);
	bool IsShadowMappingEnabled() const;
	void SetShadowMappingEnabled(bool enabled);

	void CaptureDatapoint(std::wstring = L"");

private:
	bool InitializeDeviceD3D(HWND hWnd);

	bool CreateRenderTarget();

	bool InitializeDepth();

	bool InitializeRasterizer();

	bool CreateSampler();

	void ResetViewport(float width, float height);

	void RebuildViews();

private:
	// was a const, now editable from the UI -- still used as the light dir for matcap + shadows
	glm::vec3 m_lightDirection = { 0.1f, -1.0f, 0.05f };
	bool m_shadowMappingEnabled = true;

	bool m_isSwapChainOccluded = false;

	std::vector<View> m_views;
	LayoutMode m_layoutMode = LayoutMode::Grid2x2;

	std::unique_ptr<Pipeline> m_pipeline;

	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_deviceContext;

	ComPtr<ID3D11RenderTargetView> m_renderTargetView;

	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	ComPtr<ID3D11SamplerState> m_sampler;

	D3D11_VIEWPORT m_dvp = {};

	int m_screenWidth = 0, m_screenHeight = 0;
	int m_gBufferWidth = 0, m_gBufferHeight = 0;
	float m_near = 0, m_far = 0;

	float m_topInset = 0.0f;
};

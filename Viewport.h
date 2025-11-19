#pragma once

#include <d3d11.h>
#include <glm/mat4x4.hpp>
#include <memory>
#include <windows.h>
#include <wrl/client.h>
#include "Model.h"
#include "Pipeline.h"

using Microsoft::WRL::ComPtr;

const int N_SHADING_MODES = 6;
inline const char* SHADING_MODE_NAMES[N_SHADING_MODES] = {
	"Matcap",
	"Normals",
	"Depth",
	"Cross Field",
	"Cross Field (Minimum)",
	"Reliability"
};
enum class ShadingMode : int
{
	Matcap = 0,
	Normal = 1,
	Depth = 2,
	Crossfield = 3,
	Crossfield2 = 4,
	Reliability = 5
};

class Viewport
{
public:
	Viewport();
	~Viewport();

	bool Initialize(HWND hWnd, WNDCLASSEXW, float nearPlane, float farPlane);
	void Shutdown();
	bool Render(glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, Model* model);

	ID3D11Device* GetDevice();
	ID3D11DeviceContext* GetContext();
	ShadingMode* GetShadingMode();

	void CaptureDatapoint();

private:
	bool InitializeDeviceD3D(HWND hWnd);

	bool CreateRenderTarget();

	bool InitializeDepth();

	bool InitializeRasterizer();

	bool CreateSampler();

	void ResetViewport(float width, float height);

private:
	const glm::vec3 MATCAP_LIGHT = { 0.1f, -1.0f, 0.05f };

	bool m_isSwapChainOccluded = false;

	ShadingMode m_shadingMode = ShadingMode::Matcap;

	std::unique_ptr<Pipeline> m_pipeline;

	ComPtr<IDXGISwapChain> m_swapChain;
	ComPtr<ID3D11Device> m_device;
	ComPtr<ID3D11DeviceContext> m_deviceContext;

	ComPtr<ID3D11RenderTargetView> m_renderTargetView;

	ComPtr<ID3D11DepthStencilState> m_depthStencilState;
	ComPtr<ID3D11DepthStencilView> m_depthStencilView;

	ComPtr<ID3D11SamplerState> m_sampler;

	D3D11_VIEWPORT m_dvp = {};

	UINT m_screenWidth = 0, m_screenHeight = 0;
	float m_near = 0, m_far = 0;
};
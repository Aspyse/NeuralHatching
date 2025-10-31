#pragma once

#include <d3d11.h>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <wrl/client.h>
#include "Node.h"
#include "GeometryNode.h"

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
	struct MatcapBuffer
	{
		glm::mat4x4 invProj;
		glm::mat4x4 invView;
		glm::vec3 lightDirectionVS;
		float pad;
	};

public:
	Pipeline() {};
	~Pipeline() {};

	void Initialize(ID3D11Device* device, ID3D11RenderTargetView* outRTV, int textureWidth, int textureHeight);

	void Update(ID3D11DeviceContext* deviceContext, glm::mat4x4 viewMatrix, glm::mat4x4 projectionMatrix, glm::vec3 lightDirection);
	void Render(ID3D11DeviceContext* deviceContext, int indexCount, int shadingMode);

private:
	// SRV RTV helper
	bool CreateRenderTarget(ID3D11Device* device, ID3D11RenderTargetView** rtv, ID3D11ShaderResourceView** srv, int textureWidth, int textureHeight);

	bool InitializeDepthTarget(ID3D11Device* device, int textureWidth, int textureHeight);

	void Unbind(ID3D11DeviceContext* deviceContext);

private:
	const float CLEAR_COLOR[4] = { 0.0f, 0.0f, 0.0f, 1.0f };

	ComPtr<ID3D11SamplerState> m_sampler;

	ComPtr<ID3D11RenderTargetView> m_normalRTV, m_hatchRTV, m_matcapRTV, m_outRTV;
	ComPtr<ID3D11ShaderResourceView> m_normalSRV, m_hatchSRV, m_depthSRV, m_matcapSRV;
	ComPtr<ID3D11DepthStencilView> m_depthSV;

	std::unique_ptr<GeometryNode> m_geometryNode;
	std::unique_ptr<Node> m_matcapNode, m_outNode;
};
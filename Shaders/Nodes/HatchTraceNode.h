#pragma once

#include <d3d11.h>
#include <glm/vec2.hpp>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class HatchTraceNode
{
private:
	struct LineVertex
	{
		glm::vec2 pos;
		float active;
		float padding;
	};

	// Mirrors the TraceParams cbuffer in hatch.hlsl exactly (48 bytes, 3 rows).
	struct TraceParamsBuffer
	{
		int maxSteps;
		float stepSize;
		glm::vec2 boundsMin;

		glm::vec2 boundsMax;
		int totalSeeds;
		int occGridWidth;

		int occGridHeight;
		float pad;
		glm::vec2 pad2;
	};

public:
	HatchTraceNode() {};
	~HatchTraceNode() {};

	bool Initialize(ID3D11Device* device, const wchar_t* csFilename, LPCSTR csEntrypoint, float seedSpacing, float jitterAmount, int maxSteps, float stepSize, int occGridWidth, int occGridHeight, unsigned int rngSeed);

	// Dispatches the trace against the given direction field (t0 in hatch.hlsl).
	void Trace(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* directionFieldSRV);

	ID3D11ShaderResourceView* GetLineVertexSRV() const { return m_lineVertexSRV.Get(); }
	int GetMaxSteps() const { return m_maxSteps; }
	int GetSegmentCount() const { return m_totalSeeds * (m_maxSteps - 1); }

private:
	bool InitializeComputeShader(ID3D11Device* device, const wchar_t* csFilename, LPCSTR csEntrypoint);
	bool InitializeSeedBuffer(ID3D11Device* device, float seedSpacing, float jitterAmount, unsigned int rngSeed);
	bool InitializeOutputBuffers(ID3D11Device* device, int occGridWidth, int occGridHeight);
	bool InitializeTraceParamsBuffer(ID3D11Device* device, int maxSteps, float stepSize, int occGridWidth, int occGridHeight);
	bool InitializeSampler(ID3D11Device* device);

	// Builds a jittered grid of seed points over [0,1]^2 UV space.
	std::vector<glm::vec2> GenerateJitteredGrid(float spacing, float jitterAmount, unsigned int rngSeed);

	void OutputShaderErrorMessage(ID3D10Blob* errorMessage, const wchar_t* shaderFilename);

	// TEMP DEBUG: see matching comment in the .cpp. Remove once diagnosed.
	void DebugLogLiveVertexCount(ID3D11DeviceContext* deviceContext);

private:
	ComPtr<ID3D11ComputeShader> m_traceShader;
	ComPtr<ID3D11SamplerState> m_linearSampler;

	ComPtr<ID3D11Buffer> m_traceParamsBuffer;

	ComPtr<ID3D11Buffer> m_seedBuffer;
	ComPtr<ID3D11ShaderResourceView> m_seedSRV;

	ComPtr<ID3D11Buffer> m_lineVertexBuffer;
	ComPtr<ID3D11ShaderResourceView> m_lineVertexSRV;
	ComPtr<ID3D11UnorderedAccessView> m_lineVertexUAV;

	ComPtr<ID3D11Texture2D> m_occupancyTexture;
	ComPtr<ID3D11UnorderedAccessView> m_occupancyUAV;

	int m_maxSteps = 0;
	int m_totalSeeds = 0;
};
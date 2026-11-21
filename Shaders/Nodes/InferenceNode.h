#pragma once

#include <array>
#include <d3d11.h>
#include <wrl/client.h>
#include <vector>
#include <string>
#include <onnxruntime_cxx_api.h>
#include <dml_provider_factory.h> // OrtSessionOptionsAppendExecutionProvider_DML

using Microsoft::WRL::ComPtr;

class InferenceNode
{
public:
	InferenceNode() {};
	~InferenceNode() {};

	bool Initialize(ID3D11Device* device, const wchar_t* weightsPath, int width, int height);

	// issues current readback copy, runs inference on previous copy
	void Evaluate(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* normalSRV, ID3D11ShaderResourceView* depthSRV);

	ID3D11ShaderResourceView* GetOutputSRV() const { return m_outputSRV.Get(); }

private:
	bool CreateStagingTexture(ID3D11Device* device, ID3D11Texture2D** outTexture, int width, int height);
	bool CreateOutputTexture(ID3D11Device* device, int width, int height);

	void RunInferenceOnStaging(ID3D11DeviceContext* deviceContext, ID3D11Texture2D* normalStaging, ID3D11Texture2D* depthStaging);

private:
	static const int STAGING_BUFFER_COUNT = 2;

	Ort::Env m_env{ nullptr };
	Ort::Session m_session{ nullptr };
	std::string m_inputName, m_outputName;

	int m_width = 0, m_height = 0;
	std::vector<Ort::Float16_t> m_inputBuffer; // NCHW [1,4,H,W] fp16: normal.xyz in [-1,1], depth in [0,1]
	bool m_outputIsFp16 = false; // reflects the model's actual output tensor type, queried in Initialize()

	std::array<ComPtr<ID3D11Texture2D>, STAGING_BUFFER_COUNT> m_normalStaging, m_depthStaging;
	int m_frameIndex = 0;
	bool m_hasPendingFrame = false; // false until the first CopyResource has been issued

	ComPtr<ID3D11Texture2D> m_outputTexture;
	ComPtr<ID3D11ShaderResourceView> m_outputSRV;

	// rolling perf profile, logged every kProfileWindowFrames frames then reset (see RunInferenceOnStaging)
	static const int kProfileWindowFrames = 120;
	int m_profileFrameCount = 0;
	double m_mapStallMs = 0.0;   // Map() blocking on the GPU copy from last frame
	double m_packMs = 0.0;       // CPU loop building the input tensor
	double m_inferenceMs = 0.0;  // Ort::Session::Run only
	double m_writebackMs = 0.0;  // packing prediction + mask into the output texture
};
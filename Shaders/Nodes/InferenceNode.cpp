#include "InferenceNode.h"
#include <array>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include "Logging.h"

bool InferenceNode::Initialize(ID3D11Device* device, const wchar_t* weightsPath, int width, int height)
{
	m_width = width;
	m_height = height;
	m_inputBuffer.resize(static_cast<size_t>(4) * width * height);

	try
	{
		m_env = Ort::Env(ORT_LOGGING_LEVEL_WARNING, "NeuralHatch");

		Ort::SessionOptions sessionOptions;

		// DML doesn't support ORT memory pattern optimizer and wants sequential execution
		sessionOptions.DisableMemPattern();
		sessionOptions.SetExecutionMode(ExecutionMode::ORT_SEQUENTIAL);

		// route inference to the GPU
		Ort::ThrowOnError(OrtSessionOptionsAppendExecutionProvider_DML(sessionOptions, 0)); // 0 = deviceId

		m_session = Ort::Session(m_env, weightsPath, sessionOptions);

		Ort::AllocatorWithDefaultOptions allocator;
		Ort::AllocatedStringPtr inputNamePtr = m_session.GetInputNameAllocated(0, allocator);
		Ort::AllocatedStringPtr outputNamePtr = m_session.GetOutputNameAllocated(0, allocator);
		m_inputName = inputNamePtr.get();
		m_outputName = outputNamePtr.get();

		// model input is fp16 (see error this fixes: "expected: (tensor(float16))");
		// check the output too since it's the same fp16 export and may also be fp16
		Ort::TypeInfo outputTypeInfo = m_session.GetOutputTypeInfo(0);
		m_outputIsFp16 = (outputTypeInfo.GetTensorTypeAndShapeInfo().GetElementType() == ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT16);
	}
	catch (const Ort::Exception&)
	{
		Logging::DEBUG_LOG(L"InferenceNode: failed to load ONNX model");
		return false;
	}

	if (!CreateStagingTexture(device, m_normalStaging[0].GetAddressOf(), width, height))
		return false;
	if (!CreateStagingTexture(device, m_depthStaging[0].GetAddressOf(), width, height))
		return false;
	if (!CreateStagingTexture(device, m_normalStaging[1].GetAddressOf(), width, height))
		return false;
	if (!CreateStagingTexture(device, m_depthStaging[1].GetAddressOf(), width, height))
		return false;
	if (!CreateOutputTexture(device, width, height))
		return false;

	return true;
}

void InferenceNode::Evaluate(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* normalSRV, ID3D11ShaderResourceView* depthSRV)
{
	const int current = m_frameIndex % STAGING_BUFFER_COUNT;
	const int previous = (m_frameIndex + 1) % STAGING_BUFFER_COUNT; // copy issued last frame

	ComPtr<ID3D11Resource> normalRes, depthRes;
	normalSRV->GetResource(&normalRes);
	depthSRV->GetResource(&depthRes);
	deviceContext->CopyResource(m_normalStaging[current].Get(), normalRes.Get());
	deviceContext->CopyResource(m_depthStaging[current].Get(), depthRes.Get());

	// read back and run inference on the copy issued last frame
	if (m_hasPendingFrame)
	{
		RunInferenceOnStaging(deviceContext, m_normalStaging[previous].Get(), m_depthStaging[previous].Get());
	}
	m_hasPendingFrame = true;

	m_frameIndex++;
}

void InferenceNode::RunInferenceOnStaging(ID3D11DeviceContext* deviceContext, ID3D11Texture2D* normalStaging, ID3D11Texture2D* depthStaging)
{
	using Clock = std::chrono::high_resolution_clock;
	auto msSince = [](Clock::time_point start, Clock::time_point end) {
		return std::chrono::duration<double, std::milli>(end - start).count();
		};

	const auto tMapStart = Clock::now();

	D3D11_MAPPED_SUBRESOURCE normalMapped = {}, depthMapped = {};
	deviceContext->Map(normalStaging, 0, D3D11_MAP_READ, 0, &normalMapped);
	deviceContext->Map(depthStaging, 0, D3D11_MAP_READ, 0, &depthMapped);

	const auto tPackStart = Clock::now();

	const int pixelCount = m_width * m_height;
	std::vector<float> mask(pixelCount);

	// build the NCHW [1,4,H,W] input tensor: normal.xyz remapped to [-1,1], depth kept [0,1]
	for (int y = 0; y < m_height; y++)
	{
		const uint8_t* normalRow = static_cast<const uint8_t*>(normalMapped.pData) + static_cast<size_t>(y) * normalMapped.RowPitch;
		const uint8_t* depthRow = static_cast<const uint8_t*>(depthMapped.pData) + static_cast<size_t>(y) * depthMapped.RowPitch;

		for (int x = 0; x < m_width; x++)
		{
			float nr = normalRow[x * 4 + 0] / 255.0f;
			float ng = normalRow[x * 4 + 1] / 255.0f;
			float nb = normalRow[x * 4 + 2] / 255.0f;
			float d = depthRow[x * 4 + 0] / 255.0f;

			int pixelIndex = y * m_width + x;
			mask[pixelIndex] = (nr + ng + nb) > 0.0f ? 1.0f : 0.0f;

			m_inputBuffer[0 * pixelCount + pixelIndex] = Ort::Float16_t(nr * 2.0f - 1.0f);
			m_inputBuffer[1 * pixelCount + pixelIndex] = Ort::Float16_t(ng * 2.0f - 1.0f);
			m_inputBuffer[2 * pixelCount + pixelIndex] = Ort::Float16_t(nb * 2.0f - 1.0f);
			m_inputBuffer[3 * pixelCount + pixelIndex] = Ort::Float16_t(d);
		}
	}

	deviceContext->Unmap(normalStaging, 0);
	deviceContext->Unmap(depthStaging, 0);

	const auto tPackEnd = Clock::now();

	const float* pred = nullptr;
	bool ranInference = false;
	bool wroteOutput = false;
	Clock::time_point tInferEnd{}, tWriteEnd{};
	try
	{
		Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
		std::array<int64_t, 4> inputShape = { 1, 4, m_height, m_width };
		Ort::Value inputTensor = Ort::Value::CreateTensor<Ort::Float16_t>(memoryInfo, m_inputBuffer.data(), m_inputBuffer.size(), inputShape.data(), inputShape.size());

		const char* inputNames[] = { m_inputName.c_str() };
		const char* outputNames[] = { m_outputName.c_str() };

		auto outputTensors = m_session.Run(Ort::RunOptions{ nullptr }, inputNames, &inputTensor, 1, outputNames, 1);
		tInferEnd = Clock::now();
		ranInference = true;

		// expected shape [1,3,H,W]; convert back to float here if the model's output is also fp16
		std::vector<float> predFp32;
		if (m_outputIsFp16)
		{
			const Ort::Float16_t* predFp16 = outputTensors.front().GetTensorData<Ort::Float16_t>();
			predFp32.resize(static_cast<size_t>(3) * pixelCount);
			for (size_t i = 0; i < predFp32.size(); i++)
				predFp32[i] = predFp16[i].ToFloat();
			pred = predFp32.data();
		}
		else
		{
			pred = outputTensors.front().GetTensorData<float>();
		}

		// pack the raw prediction + mask into the upload texture
		D3D11_MAPPED_SUBRESOURCE outMapped = {};
		deviceContext->Map(m_outputTexture.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &outMapped);
		for (int y = 0; y < m_height; y++)
		{
			float* outRow = reinterpret_cast<float*>(static_cast<uint8_t*>(outMapped.pData) + static_cast<size_t>(y) * outMapped.RowPitch);
			for (int x = 0; x < m_width; x++)
			{
				int pixelIndex = y * m_width + x;
				outRow[x * 4 + 0] = pred[0 * pixelCount + pixelIndex];
				outRow[x * 4 + 1] = pred[1 * pixelCount + pixelIndex];
				outRow[x * 4 + 2] = pred[2 * pixelCount + pixelIndex];
				outRow[x * 4 + 3] = mask[pixelIndex];
			}
		}
		deviceContext->Unmap(m_outputTexture.Get(), 0);
		tWriteEnd = Clock::now();
		wroteOutput = true;
	}
	catch (const Ort::Exception& e)
	{
		//Logging::DEBUG_LOG(L"InferenceNode: inference failed, skipping this frame");
		Logging::DEBUG_LOG(L"InferenceNode: inference failed: %hs", e.what());
	}

	// rolling perf profile: mapStall/pack always ran, inference/writeback only count if they completed
	m_mapStallMs += msSince(tMapStart, tPackStart);
	m_packMs += msSince(tPackStart, tPackEnd);
	if (ranInference)
		m_inferenceMs += msSince(tPackEnd, tInferEnd);
	if (wroteOutput)
		m_writebackMs += msSince(tInferEnd, tWriteEnd);
	m_profileFrameCount++;

	if (m_profileFrameCount >= kProfileWindowFrames)
	{
		Logging::DEBUG_LOG(
			L"InferenceNode profile (avg ms over ", m_profileFrameCount, L" frames): ",
			std::fixed, std::setprecision(2),
			L"mapStall=", m_mapStallMs / m_profileFrameCount,
			L" pack=", m_packMs / m_profileFrameCount,
			L" inference=", m_inferenceMs / m_profileFrameCount,
			L" writeback=", m_writebackMs / m_profileFrameCount,
			L" total=", (m_mapStallMs + m_packMs + m_inferenceMs + m_writebackMs) / m_profileFrameCount);

		m_profileFrameCount = 0;
		m_mapStallMs = m_packMs = m_inferenceMs = m_writebackMs = 0.0;
	}
}

bool InferenceNode::CreateStagingTexture(ID3D11Device* device, ID3D11Texture2D** outTexture, int width, int height)
{
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_STAGING;
	td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	td.BindFlags = 0;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;

	HRESULT result = device->CreateTexture2D(&td, nullptr, outTexture);
	return SUCCEEDED(result);
}

bool InferenceNode::CreateOutputTexture(ID3D11Device* device, int width, int height)
{
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DYNAMIC;
	td.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT result = device->CreateTexture2D(&td, nullptr, m_outputTexture.GetAddressOf());
	if (FAILED(result))
		return false;

	result = device->CreateShaderResourceView(m_outputTexture.Get(), nullptr, m_outputSRV.GetAddressOf());
	return SUCCEEDED(result);
}
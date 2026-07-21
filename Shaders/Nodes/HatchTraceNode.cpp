#include "HatchTraceNode.h"

#include <algorithm>
#include <fstream>
#include <random>
#include <d3dcompiler.h>
#include "Logging.h" // TEMP DEBUG

bool HatchTraceNode::Initialize(ID3D11Device* device, const wchar_t* csFilename, LPCSTR csEntrypoint, float seedSpacing, float jitterAmount, int maxSteps, float stepSize, int occGridWidth, int occGridHeight, unsigned int rngSeed)
{
	m_maxSteps = maxSteps;

	if (!InitializeComputeShader(device, csFilename, csEntrypoint)) return false;
	if (!InitializeSeedBuffer(device, seedSpacing, jitterAmount, rngSeed)) return false;
	if (!InitializeOutputBuffers(device, occGridWidth, occGridHeight)) return false;
	if (!InitializeTraceParamsBuffer(device, maxSteps, stepSize, occGridWidth, occGridHeight)) return false;
	if (!InitializeSampler(device)) return false;

	return true;
}

void HatchTraceNode::Trace(ID3D11DeviceContext* deviceContext, ID3D11ShaderResourceView* directionFieldSRV)
{
	// Occupancy has to start empty every frame, otherwise every seed would
	// find its cell already claimed by last frame's lines.
	UINT clearValue[4] = { 0, 0, 0, 0 };
	deviceContext->ClearUnorderedAccessViewUint(m_occupancyUAV.Get(), clearValue);

	ID3D11ShaderResourceView* srvs[] = { directionFieldSRV, m_seedSRV.Get() };
	deviceContext->CSSetShaderResources(0, 2, srvs);
	deviceContext->CSSetSamplers(0, 1, m_linearSampler.GetAddressOf());

	ID3D11Buffer* cbuf = m_traceParamsBuffer.Get();
	deviceContext->CSSetConstantBuffers(0, 1, &cbuf);

	ID3D11UnorderedAccessView* uavs[] = { m_lineVertexUAV.Get(), m_occupancyUAV.Get() };
	deviceContext->CSSetUnorderedAccessViews(0, 2, uavs, nullptr);

	deviceContext->CSSetShader(m_traceShader.Get(), nullptr, 0);

	deviceContext->Dispatch((m_totalSeeds + 63) / 64, 1, 1);

	// TEMP DEBUG: read back the line vertex buffer and count how many
	// vertices came back marked active, to check whether the trace shader
	// is actually producing live streamlines. Remove once diagnosed.
	DebugLogLiveVertexCount(deviceContext);

	// Unbind so the field SRV isn't left bound to the CS stage if it's
	// needed elsewhere (e.g. as a render target again) next frame.
	ID3D11UnorderedAccessView* nullUAVs[] = { nullptr, nullptr };
	deviceContext->CSSetUnorderedAccessViews(0, 2, nullUAVs, nullptr);
	ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
	deviceContext->CSSetShaderResources(0, 2, nullSRVs);
	deviceContext->CSSetShader(nullptr, nullptr, 0);
}

std::vector<glm::vec2> HatchTraceNode::GenerateJitteredGrid(float spacing, float jitterAmount, unsigned int rngSeed)
{
	std::mt19937 rng(rngSeed);
	std::uniform_real_distribution<float> jitter(-jitterAmount, jitterAmount);

	std::vector<glm::vec2> seeds;
	for (float y = spacing * 0.5f; y < 1.0f; y += spacing)
	{
		for (float x = spacing * 0.5f; x < 1.0f; x += spacing)
		{
			glm::vec2 seed(x + jitter(rng), y + jitter(rng));
			seed.x = std::clamp(seed.x, 0.0f, 1.0f);
			seed.y = std::clamp(seed.y, 0.0f, 1.0f);
			seeds.push_back(seed);
		}
	}
	return seeds;
}

bool HatchTraceNode::InitializeComputeShader(ID3D11Device* device, const wchar_t* csFilename, LPCSTR csEntrypoint)
{
	ID3D10Blob* errorMessage;
	ID3D10Blob* computeShaderBuffer;

	wchar_t csFileCopy[128];
	int error = wcscpy_s(csFileCopy, 128, csFilename);
	if (error != 0)
		return false;

	// Compile trace compute shader code
	HRESULT result = D3DCompileFromFile(csFileCopy, nullptr, nullptr, csEntrypoint, "cs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &computeShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		if (errorMessage)
			OutputShaderErrorMessage(errorMessage, csFileCopy);
		return false;
	}

	// Create compute shader from buffer
	result = device->CreateComputeShader(computeShaderBuffer->GetBufferPointer(), computeShaderBuffer->GetBufferSize(), nullptr, &m_traceShader);
	if (FAILED(result))
		return false;

	computeShaderBuffer->Release();
	computeShaderBuffer = 0;

	return true;
}

bool HatchTraceNode::InitializeSeedBuffer(ID3D11Device* device, float seedSpacing, float jitterAmount, unsigned int rngSeed)
{
	std::vector<glm::vec2> seeds = GenerateJitteredGrid(seedSpacing, jitterAmount, rngSeed);
	m_totalSeeds = static_cast<int>(seeds.size());

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.ByteWidth = sizeof(glm::vec2) * m_totalSeeds;
	bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	bd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	bd.StructureByteStride = sizeof(glm::vec2);

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = seeds.data();

	HRESULT result = device->CreateBuffer(&bd, &sd, m_seedBuffer.GetAddressOf());
	if (FAILED(result))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements = m_totalSeeds;

	result = device->CreateShaderResourceView(m_seedBuffer.Get(), &srvDesc, m_seedSRV.GetAddressOf());
	if (FAILED(result))
		return false;

	return true;
}

bool HatchTraceNode::InitializeOutputBuffers(ID3D11Device* device, int occGridWidth, int occGridHeight)
{
	// Line vertex buffer: MaxSteps vertices per seed, written by the trace
	// shader and read back by hatch_lines.hlsl's vertex shader.
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(LineVertex) * m_totalSeeds * m_maxSteps;
	vbd.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	vbd.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	vbd.StructureByteStride = sizeof(LineVertex);

	HRESULT result = device->CreateBuffer(&vbd, nullptr, m_lineVertexBuffer.GetAddressOf());
	if (FAILED(result))
		return false;

	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements = m_totalSeeds * m_maxSteps;

	result = device->CreateShaderResourceView(m_lineVertexBuffer.Get(), &srvDesc, m_lineVertexSRV.GetAddressOf());
	if (FAILED(result))
		return false;

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory(&uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = m_totalSeeds * m_maxSteps;

	result = device->CreateUnorderedAccessView(m_lineVertexBuffer.Get(), &uavDesc, m_lineVertexUAV.GetAddressOf());
	if (FAILED(result))
		return false;

	// Occupancy grid: one uint per cell, used by the trace shader to space
	// streamlines apart so they don't all pile on top of each other.
	D3D11_TEXTURE2D_DESC td;
	ZeroMemory(&td, sizeof(td));
	td.Width = occGridWidth;
	td.Height = occGridHeight;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
	td.Format = DXGI_FORMAT_R32_UINT;

	result = device->CreateTexture2D(&td, nullptr, m_occupancyTexture.GetAddressOf());
	if (FAILED(result))
		return false;

	result = device->CreateUnorderedAccessView(m_occupancyTexture.Get(), nullptr, m_occupancyUAV.GetAddressOf());
	if (FAILED(result))
		return false;

	return true;
}

bool HatchTraceNode::InitializeTraceParamsBuffer(ID3D11Device* device, int maxSteps, float stepSize, int occGridWidth, int occGridHeight)
{
	TraceParamsBuffer params = {};
	params.maxSteps = maxSteps;
	params.stepSize = stepSize;
	params.boundsMin = glm::vec2(0.0f, 0.0f);
	params.boundsMax = glm::vec2(1.0f, 1.0f);
	params.totalSeeds = m_totalSeeds;
	params.occGridWidth = occGridWidth;
	params.occGridHeight = occGridHeight;

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeof(TraceParamsBuffer);
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	D3D11_SUBRESOURCE_DATA sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.pSysMem = &params;

	HRESULT result = device->CreateBuffer(&bd, &sd, m_traceParamsBuffer.GetAddressOf());
	if (FAILED(result))
		return false;

	return true;
}

bool HatchTraceNode::InitializeSampler(ID3D11Device* device)
{
	D3D11_SAMPLER_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;

	HRESULT result = device->CreateSamplerState(&sd, m_linearSampler.GetAddressOf());
	if (FAILED(result))
		return false;

	return true;
}

// TEMP DEBUG: copies m_lineVertexBuffer to a CPU-readable staging buffer and
// counts how many LineVertex entries have active > 0.5. Remove once the
// black-hatch-view issue is diagnosed -- this stalls the pipeline (Map blocks
// on GPU completion) so it's not something to ship.
void HatchTraceNode::DebugLogLiveVertexCount(ID3D11DeviceContext* deviceContext)
{
	ComPtr<ID3D11Device> device;
	deviceContext->GetDevice(device.GetAddressOf());

	D3D11_BUFFER_DESC desc;
	m_lineVertexBuffer->GetDesc(&desc);
	desc.Usage = D3D11_USAGE_STAGING;
	desc.BindFlags = 0;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	desc.MiscFlags = 0;

	ComPtr<ID3D11Buffer> stagingBuf;
	HRESULT hr = device->CreateBuffer(&desc, nullptr, stagingBuf.GetAddressOf());
	if (FAILED(hr))
	{
		Logging::DEBUG_LOG(L"[HatchTrace] staging buffer creation failed");
		return;
	}

	deviceContext->CopyResource(stagingBuf.Get(), m_lineVertexBuffer.Get());

	D3D11_MAPPED_SUBRESOURCE mapped;
	hr = deviceContext->Map(stagingBuf.Get(), 0, D3D11_MAP_READ, 0, &mapped);
	if (FAILED(hr))
	{
		Logging::DEBUG_LOG(L"[HatchTrace] staging buffer map failed");
		return;
	}

	const LineVertex* verts = static_cast<const LineVertex*>(mapped.pData);
	int totalVerts = m_totalSeeds * m_maxSteps;
	int liveCount = 0;
	int seedsWithAnyLiveVert = 0;
	for (int seed = 0; seed < m_totalSeeds; ++seed)
	{
		bool seedHasLive = false;
		for (int step = 0; step < m_maxSteps; ++step)
		{
			if (verts[seed * m_maxSteps + step].active > 0.5f)
			{
				liveCount++;
				seedHasLive = true;
			}
		}
		if (seedHasLive)
			seedsWithAnyLiveVert++;
	}
	deviceContext->Unmap(stagingBuf.Get(), 0);

	Logging::DEBUG_LOG(L"[HatchTrace] live verts: ", liveCount, L" / ", totalVerts,
		L"  seeds with >=1 live vert: ", seedsWithAnyLiveVert, L" / ", m_totalSeeds);
}

void HatchTraceNode::OutputShaderErrorMessage(ID3D10Blob* errorMessage, const wchar_t* shaderFilename)
{
	char* compileErrors = static_cast<char*>(errorMessage->GetBufferPointer());

	std::ofstream fout("hatch-trace-shader-error.txt");
	fout << compileErrors;
	fout.close();

	errorMessage->Release();
	errorMessage = 0;

	MessageBox(nullptr, L"Error compiling hatch trace shader. Check hatch-trace-shader-error.txt for message.", shaderFilename, MB_OK);
}
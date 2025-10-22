#include "Node.h"

bool Node::Initialize(ID3D11Device* device, const wchar_t* vsFilename, const wchar_t* psFilename, LPCSTR vsEntrypoint, LPCSTR psEntrypoint)
{
	ID3D10Blob* errorMessage;
	ID3D10Blob* vertexShaderBuffer;
	ID3D10Blob* pixelShaderBuffer;
	
	wchar_t vsFileCopy[128];
	wchar_t psFileCopy[128];

	int error = wcscpy_s(vsFileCopy, 128, vsFilename);
	if (error != 0)
		return false;

	error = wcscpy_s(psFileCopy, 128, psFilename);
	if (error != 0)
		return false;
	

	// Compile vertex shader code
	HRESULT result = D3DCompileFromFile(vsFileCopy, nullptr, nullptr, vsEntrypoint, "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &vertexShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		if (errorMessage)
			OutputShaderErrorMessage(errorMessage, vsFileCopy);
		return false;
	}

	// Compile pixel shader code
	result = D3DCompileFromFile(psFileCopy, nullptr, nullptr, psEntrypoint, "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, &pixelShaderBuffer, &errorMessage);
	if (FAILED(result))
	{
		if (errorMessage)
			OutputShaderErrorMessage(errorMessage, psFileCopy);
		return false;
	}

	// Create vertex shader from buffer
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), nullptr, &m_vertexShader);
	if (FAILED(result))
		return false;

	// Create pixel shader from buffer
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), nullptr, &m_pixelShader);
	if (FAILED(result))
		return false;

	if (!InitializeInputLayout(device, vertexShaderBuffer)) return false;

	// Release vertex and pixel shader buffers
	vertexShaderBuffer->Release();
	vertexShaderBuffer = 0;

	pixelShaderBuffer->Release();
	pixelShaderBuffer = 0;

	//InitializeConstantBuffer(device);

	return true;
}

bool Node::Render(ID3D11DeviceContext* deviceContext)
{
	for (UINT i = 0; i < m_psConstantBuffers.size(); ++i)
	{
		ID3D11Buffer* bufferPtr = m_psConstantBuffers[i].Get();
		deviceContext->PSSetConstantBuffers(i, 1, &bufferPtr);
	}
	for (UINT i = 0; i < m_vsConstantBuffers.size(); ++i)
	{
		ID3D11Buffer* bufferPtr = m_vsConstantBuffers[i].Get();
		deviceContext->VSSetConstantBuffers(i, 1, &bufferPtr);
	}

	deviceContext->IASetInputLayout(m_layout.Get());

	deviceContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	deviceContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	//deviceContext->Draw(3, 0);

	return true;
}
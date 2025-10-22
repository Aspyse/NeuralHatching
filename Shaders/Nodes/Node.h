#pragma once

#include <fstream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using std::ofstream;

class Node
{
public:
	Node() {};
	~Node() {};
	
	bool Initialize(ID3D11Device* device, const wchar_t* vsFilename, const wchar_t* psFilename, LPCSTR vsEntrypoint, LPCSTR psEntrypoint);
	virtual bool Render(ID3D11DeviceContext* deviceContext);

public:
	template <typename T>
	void AddPSConstantBuffer(ID3D11Device* device)
	{
		ComPtr<ID3D11Buffer> tempBuffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(T);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;

		HRESULT result = device->CreateBuffer(&bd, nullptr, tempBuffer.GetAddressOf());

		m_psConstantBuffers.push_back(tempBuffer);
	}
	template <typename T>
	void AddVSConstantBuffer(ID3D11Device* device)
	{
		ComPtr<ID3D11Buffer> tempBuffer;

		D3D11_BUFFER_DESC bd;
		ZeroMemory(&bd, sizeof(bd));
		bd.Usage = D3D11_USAGE_DYNAMIC;
		bd.ByteWidth = sizeof(T);
		bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		bd.MiscFlags = 0;
		bd.StructureByteStride = 0;

		HRESULT result = device->CreateBuffer(&bd, nullptr, tempBuffer.GetAddressOf());

		m_vsConstantBuffers.push_back(tempBuffer);
	}
	template <typename T>
	void UpdatePSConstantBuffer(ID3D11DeviceContext* deviceContext, T bufferData, int index)
	{
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		deviceContext->Map(m_psConstantBuffers[index].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		std::memcpy(mapped.pData, &bufferData, sizeof(bufferData));
		deviceContext->Unmap(m_psConstantBuffers[index].Get(), 0);
	}
	template <typename T>
	void UpdateVSConstantBuffer(ID3D11DeviceContext* deviceContext, T bufferData, int index)
	{
		D3D11_MAPPED_SUBRESOURCE mapped = {};
		deviceContext->Map(m_vsConstantBuffers[index].Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
		std::memcpy(mapped.pData, &bufferData, sizeof(bufferData));
		deviceContext->Unmap(m_vsConstantBuffers[index].Get(), 0);
	}

	static void OutputShaderErrorMessage(ID3D10Blob* errorMessage, WCHAR* shaderFilename)
	{
		char* compileErrors;
		unsigned long long bufferSize, i;
		ofstream fout;

		compileErrors = (char*)(errorMessage->GetBufferPointer());

		bufferSize = errorMessage->GetBufferSize();
		fout.open("shader-error.txt");
		for (i = 0; i < bufferSize; i++)
			fout << compileErrors[i];

		fout.close();

		errorMessage->Release();
		errorMessage = 0;
	}

protected:
	virtual bool InitializeInputLayout(ID3D11Device* device, ID3D10Blob* vertexShaderBuffer) { return true; };

protected:
	std::vector<ComPtr<ID3D11Buffer>> m_psConstantBuffers;
	std::vector<ComPtr<ID3D11Buffer>> m_vsConstantBuffers;
	std::vector<std::string> m_inputs;

	ComPtr<ID3D11VertexShader> m_vertexShader;
	ComPtr<ID3D11PixelShader> m_pixelShader;
	ComPtr<ID3D11InputLayout> m_layout; // Can be left as null by postprocessing nodes
};
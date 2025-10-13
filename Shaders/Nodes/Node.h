#pragma once

#include "render_target.h"
#include <fstream>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <variant>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;
using std::ofstream;
using DirectX::XMMATRIX;
using DirectX::XMVECTOR;
using DirectX::XMFLOAT3;

class RenderPass
{
public:
	virtual bool Initialize(ID3D11Device*, UINT, UINT);
	virtual bool Render(ID3D11Device*, ID3D11DeviceContext*, float*);
	//virtual void Update(ID3D11DeviceContext*, XMMATRIX, XMMATRIX, XMMATRIX, XMVECTOR, XMFLOAT3, UINT, UINT) = 0;

	std::vector<std::string> GetInputs() const;

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
	template <typename T>
	void AddCB(ID3D11Device* device)
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

		m_constantBuffers.push_back(tempBuffer);
	}

	virtual bool InitializeConstantBuffer(ID3D11Device* device) = 0;
	virtual const wchar_t* filename() const = 0;
	std::vector<std::string>& outputs()
	{
		return m_outputs;
	}
	std::vector<std::string> m_outputs;


protected:
	std::vector<ComPtr<ID3D11Buffer>> m_constantBuffers;
	std::vector<std::string> m_inputs;

	ID3D11VertexShader* m_vertexShader = nullptr;
	ID3D11PixelShader* m_pixelShader = nullptr;
};
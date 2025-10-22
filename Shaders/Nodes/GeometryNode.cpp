#include "GeometryNode.h"

bool GeometryNode::InitializeInputLayout(ID3D11Device* device, ID3D10Blob* vertexShaderBuffer)
{
	// Define input layout
    D3D11_INPUT_ELEMENT_DESC pl[4];
    pl[0].SemanticName = "POSITION";
    pl[0].SemanticIndex = 0;
    pl[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    pl[0].InputSlot = 0;
    pl[0].AlignedByteOffset = 0;
    pl[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    pl[0].InstanceDataStepRate = 0;

    pl[1].SemanticName = "TEXCOORD";
    pl[1].SemanticIndex = 0;
    pl[1].Format = DXGI_FORMAT_R32G32_FLOAT;
    pl[1].InputSlot = 0;
    pl[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    pl[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    pl[1].InstanceDataStepRate = 0;

    pl[2].SemanticName = "TEXCOORD";
    pl[2].SemanticIndex = 1;
    pl[2].Format = DXGI_FORMAT_R32_FLOAT;
    pl[2].InputSlot = 0;
    pl[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    pl[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    pl[2].InstanceDataStepRate = 0;

    pl[3].SemanticName = "NORMAL";
    pl[3].SemanticIndex = 0;
    pl[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    pl[3].InputSlot = 0;
    pl[3].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    pl[3].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    pl[3].InstanceDataStepRate = 0;

    UINT numElements = sizeof(pl) / sizeof(pl[0]);
    HRESULT result = device->CreateInputLayout(pl, numElements, vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), &m_layout);
    if (FAILED(result))
        return false;

    return true;
}
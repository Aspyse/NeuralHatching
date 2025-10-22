#include "Model.h"
#include <miniply.h>

Model::Model() {}
Model::~Model() {}

bool Model::Load(ID3D11Device* device, const char* filename)
{
	if (!LoadPLY(filename)) return false;
	CalculateNormals();

	// Dummy UVs
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].uv = glm::vec2(0.0f, 0.0f);

	if (!CreateBuffers(device)) return false;



	return true;
}

void Model::Render(ID3D11DeviceContext* deviceContext)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

	// Set type of primitive to be rendered
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

int Model::GetIndexCount()
{
	return static_cast<int>(m_indices.size());
}

bool Model::LoadPLY(const char* filename)
{
	miniply::PLYReader reader(filename);

	uint32_t faceIdxs[3];
	miniply::PLYElement* faceElem = reader.get_element(reader.find_element(miniply::kPLYFaceElement));
	if (faceElem == nullptr)
		return false;

	faceElem->convert_list_to_fixed_size(faceElem->find_property("vertex_indices"), 3, faceIdxs);

	uint32_t indexes[3];
	bool gotVerts = false, gotFaces = false;

	while (reader.has_element() && (!gotVerts || !gotFaces))
	{
		if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(indexes))
		{
			m_vertexCount = reader.num_rows();
			std::vector<float> pos(m_vertexCount * 3);
			m_vertices.resize(m_vertexCount);
			reader.extract_properties(indexes, 3, miniply::PLYPropertyType::Float, pos.data());

			for (uint32_t i = 0; i < m_vertexCount; ++i)
				m_vertices[i].position = glm::vec3(pos[i * 3 + 0], pos[i * 3 + 1], pos[i * 3 + 2]);

			gotVerts = true;
		}
		else if (!gotFaces && reader.element_is(miniply::kPLYFaceElement) && reader.load_element())
		{
			int indexCount = reader.num_rows() * 3;
			m_indices.resize(indexCount);
			reader.extract_properties(faceIdxs, 3, miniply::PLYPropertyType::Int, m_indices.data());

			gotFaces = true;
		}
		if (gotVerts && gotFaces)
			break;
		reader.next_element();
	}

	if (!gotVerts || !gotFaces)
		return false;

	return true;
}

void Model::CalculateNormals()
{
	// Initialize all normals to zero
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].normal = glm::vec3(0.0f, 0.0f, 0.0f);

	// Loop over each triangle
	for (uint32_t i = 0; i < m_indices.size(); i += 3) {
		uint32_t i0 = m_indices[i];
		uint32_t i1 = m_indices[i + 1];
		uint32_t i2 = m_indices[i + 2];

		// Calculate the two edge vectors
		glm::vec3 edge1 = m_vertices[i1].position - m_vertices[i0].position;
		glm::vec3 edge2 = m_vertices[i2].position - m_vertices[i0].position;

		// Compute the face normal via cross product
		glm::vec3 faceNormal = glm::cross(edge1, edge2);
		faceNormal = glm::normalize(faceNormal);

		// Accumulate the face normal into each vertex's normal
		for (uint32_t idx : { i0, i1, i2 })
			m_vertices[idx].normal += faceNormal;
	}

	// Normalize the accumulated normals for each vertex
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].normal = glm::normalize(m_vertices[i].normal);
}

bool Model::CreateBuffers(ID3D11Device* device)
{
	D3D11_BUFFER_DESC vbd;
	ZeroMemory(&vbd, sizeof(vbd));
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(Vertex) * m_vertexCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(vertexData));
	vertexData.pSysMem = m_vertices.data();
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	HRESULT result = device->CreateBuffer(&vbd, &vertexData, &m_vertexBuffer);
	if (FAILED(result))
		return false;

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(ibd));
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.ByteWidth = sizeof(uint32_t) * static_cast<UINT>(m_indices.size());
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	ibd.MiscFlags = 0;
	ibd.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(indexData));
	indexData.pSysMem = m_indices.data();
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	result = device->CreateBuffer(&ibd, &indexData, &m_indexBuffer);
	if (FAILED(result))
		return false;

	return true;
}
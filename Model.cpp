#include "Model.h"
#include "Curvature.h"
#include <miniply.h>
#include "rapidobj/rapidobj.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "Logging.h"

Model::Model() {}
Model::~Model() {}

void Model::Initialize(uint64_t uid)
{
	m_uid = uid;
}

bool Model::Load(ID3D11Device* device, const char* filename)
{
	std::string fullFilename = std::string("Models/") + filename;

	m_name = std::wstring(filename, filename + strlen(filename));

	std::filesystem::path p(filename);
	std::string ext = p.extension().string();
	
	if (ext == ".ply" || ext == ".PLY") {
		Logging::DEBUG_LOG(L"LOADING PLY MODEL ", m_name, L"...");
		if (!LoadPLY(fullFilename.c_str())) return false;
		CalculateNormals();
	}
	else if (ext == ".obj" || ext == ".OBJ") {
		Logging::DEBUG_LOG(L"LOADING OBJ MODEL ", m_name, L"...");
		if (!LoadOBJ(fullFilename.c_str())) return false;
	}
	else return false;

	// Dummy UVs
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].uv = glm::vec2(0.0f, 0.0f);

	// Scale and center
	Autonormalize();

	CalculateCrossField();


	if (!CreateBuffers(device)) return false;

	Logging::DEBUG_LOG(L"LOADED MODEL ", m_name, L".");
	return true;
}

void Model::Render(ID3D11DeviceContext* deviceContext)
{
	UINT stride = sizeof(Vertex);
	UINT offset = 0;

	ID3D11Buffer* vertexBufferPtr = m_vertexBuffer.Get();
	deviceContext->IASetVertexBuffers(0, 1, &vertexBufferPtr, &stride, &offset);

	// Set type of primitive to be rendered
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	
	deviceContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
}

int Model::GetIndexCount()
{
	return static_cast<int>(m_indices.size());
}

glm::mat4 Model::GetWorldMatrix()
{
	glm::mat4 worldMatrix = glm::mat4(1.0f);

	worldMatrix = glm::translate(worldMatrix, m_position);

	worldMatrix = glm::rotate(worldMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
	worldMatrix = glm::rotate(worldMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
	worldMatrix = glm::rotate(worldMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll

	worldMatrix = glm::scale(worldMatrix, m_scale);

	return worldMatrix;
}

glm::vec3 Model::GetPosition()
{
	return m_position;
}
glm::vec3 Model::GetRotation()
{
	return m_rotation;
}
glm::vec3 Model::GetScale()
{
	return m_scale;
}

void Model::SetPosition(glm::vec3 newPos)
{
	m_position = newPos;
}
void Model::SetRotation(glm::vec3 newRot)
{
	m_rotation = newRot;
}
void Model::SetScale(glm::vec3 newScl)
{
	m_scale = newScl;
}


std::wstring Model::GetName()
{
	return m_name;
}

uint64_t Model::GetUID()
{
	return m_uid;
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

			Logging::DEBUG_LOG(L"READING VERTICES...");
			Logging::DEBUG_START();
			for (uint32_t i = 0; i < m_vertexCount; ++i) {
				Logging::DEBUG_BAR(i, m_vertexCount);

				m_vertices[i].position = glm::vec3(pos[i * 3 + 0], pos[i * 3 + 1], pos[i * 3 + 2]);
			}

			gotVerts = true;
		}
		else if (!gotFaces && reader.element_is(miniply::kPLYFaceElement) && reader.load_element())
		{
			Logging::DEBUG_LOG(L"READING INDICES...");
			
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

bool Model::LoadOBJ(const char* filename)
{
	Logging::DEBUG_LOG("RAPIDOBJ PARSING...");
	m_vertices.clear();
	m_indices.clear();

	rapidobj::Result result = rapidobj::ParseFile(filename);

	if (result.error) {
		OutputDebugString(result.error.code.message().c_str());
		return false;
	}

	bool success = rapidobj::Triangulate(result);

	if (!success) return false;

	const auto& A = result.attributes; // rapidobj::Attributes

	// .obj position index -> vertex index
	std::unordered_map<size_t, size_t> uniqueVertices;
	std::unordered_map<size_t, int> normalCounts;
	bool flagNormal = false;

	Logging::DEBUG_LOG("READING VERTICES AND INDICES...");
	Logging::DEBUG_START();
	for (const auto& shape : result.shapes)
	{
		const auto& mesh = shape.mesh;
		size_t idxOff = 0;
		for (size_t f = 0; f < mesh.num_face_vertices.size(); ++f) {
			Logging::DEBUG_BAR(f, mesh.num_face_vertices.size());

			int fv = mesh.num_face_vertices[f]; // after Triangulate should be 3
			if (fv != 3) Logging::DEBUG_LOG(L"WARNING: NON-TRI FOUND!");
			for (int v = 0; v < fv; ++v) {
				const rapidobj::Index& ix = mesh.indices[idxOff + v];

				// read position (3 floats)
				size_t pp = size_t(ix.position_index);
				glm::vec3 pos = glm::vec3(
					A.positions[pp * 3 + 0],
					A.positions[pp * 3 + 1],
					A.positions[pp * 3 + 2]
				);

				Vertex* vtx = nullptr;
				size_t idx = m_vertices.size();
				auto it = uniqueVertices.find(pp);
				if (it != uniqueVertices.end()) {
					// already exists
					idx = it->second;
					vtx = &m_vertices[idx];
				}
				else {
					// new vertex
					vtx = new Vertex();
					vtx->position = pos;
					uniqueVertices[pp] = idx;

					m_vertices.push_back(*vtx);
				}


				// normal (may be -1)
				if (ix.normal_index >= 0) {
					normalCounts[idx]++;
					flagNormal = true;
					size_t nn = size_t(ix.normal_index) * 3;
					vtx->normal += glm::vec3(
						A.normals[nn + 0],
						A.normals[nn + 1],
						A.normals[nn + 2]
					);
				}

				m_indices.push_back(idx);
			}
			idxOff += fv;
		}
	}

	m_vertexCount = m_vertices.size();
	// Average the accumulated normals
	for (size_t i = 0; i < m_vertexCount; ++i) {
		if (normalCounts[i] > 0) {
			m_vertices[i].normal /= static_cast<float>(normalCounts[i]);
			m_vertices[i].normal = glm::normalize(m_vertices[i].normal);
		}
	}

	if (!flagNormal)
		CalculateNormals();

	return true;
}

void Model::CalculateNormals()
{
	// Initialize all normals to zero
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].normal = glm::vec3(0.0f, 0.0f, 0.0f);

	Logging::DEBUG_LOG(L"CALCULATING NORMALS...");
	Logging::DEBUG_START();
	// Loop over each triangle
	for (uint32_t i = 0; i < m_indices.size(); i += 3) {
		Logging::DEBUG_BAR(i, m_indices.size());

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

void Model::CalculateCrossField()
{
	// Initialize all cross angles to zero
	/*
	for (uint32_t i = 0; i < m_vertexCount; ++i)
		m_vertices[i].crossAngle = 0.66f;
	*/

	// TODO: check if idiomatic
	Curvature* c = new Curvature();
	c->InitializeField(*this, m_scale.x);

	for (uint32_t i = 0; i < m_vertexCount; ++i) {
		//m_vertices[i].crossAngle.x = m_curvatures[i].theta;
		m_vertices[i].reliability = m_curvatures[i].reliable;
		//m_vertices[i].reliability = m_curvatures[i].theta;
		m_vertices[i].crossAngle = m_curvatures[i].dir1;
		m_vertices[i].crossAngle2 = m_curvatures[i].dir2;
	}
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

void Model::Autonormalize(float diameter)
{
	glm::vec3 minBounds(FLT_MAX), maxBounds(-FLT_MAX);
	for (const auto& v : m_vertices) {
		minBounds = glm::min(minBounds, v.position);
		maxBounds = glm::max(maxBounds, v.position);
	}

	glm::vec3 center = (minBounds + maxBounds) * 0.5f;
	
	// Find farthest distance from center
	float maxDist = 0.0f;
	for (const auto& v : m_vertices) {
		float dist = glm::length(v.position - center);
		maxDist = glm::max(maxDist, dist);
	}

	float scale = diameter / maxDist;

	// Apply
	/*for (auto& v : m_vertices) {
		v.position = (v.position - center) * scale;
	}*/
	m_position = -center*scale;
	m_scale = glm::vec3(scale);
}

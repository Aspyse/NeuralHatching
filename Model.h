#pragma once

#include <d3d11.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

class Model
{
private:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 normal;
		glm::vec4 tangent;

		float crossAngle;
	};

public:
	Model();
	~Model();

	bool Load(ID3D11Device* device, const char* filename);
	
	void Render(ID3D11DeviceContext* context);

private:
	void CalculateNormals();
	bool CreateBuffers(ID3D11Device* device);
	bool LoadPLY(const char* filename);

private:
	uint32_t m_vertexCount = 0;
	ComPtr<ID3D11Buffer> m_vertexBuffer;
	ComPtr<ID3D11Buffer> m_indexBuffer;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

};
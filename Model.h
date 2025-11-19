#pragma once

#include <d3d11.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <wrl/client.h>
#include <vector>

using Microsoft::WRL::ComPtr;

struct CurvatureInfo;

class Model
{
private:
	struct Vertex
	{
		glm::vec3 position;
		glm::vec2 uv;
		glm::vec3 crossAngle;
		glm::vec3 crossAngle2;
		float reliability;
		glm::vec3 normal;
		//glm::vec4 tangent;
	};

	// TODO: strange, consider refactoring
	struct CurvatureInfo
	{
		bool reliable = false;
		//glm::vec3 hatchDir = glm::vec3(0.0f);
		float kappa1;        // Maximum principal curvature
		float kappa2;        // Minimum principal curvature
		float theta = 0;

		// Curvature directions (3D)
		glm::vec3 dir1;
		glm::vec3 dir2;

		glm::vec3 tangent_u; // Reference tangent direction for angle measurement
		glm::vec3 tangent_v; // Perpendicular tangent direction
	};

public:
	friend class Curvature;

	Model();
	~Model();

	bool Load(ID3D11Device* device, const char* filename);
	
	void Render(ID3D11DeviceContext* context);
	
	int GetIndexCount();

private:
	void CalculateNormals();
	void CalculateCrossField();
	bool CreateBuffers(ID3D11Device* device);
	bool LoadPLY(const char* filename);

private:
	uint32_t m_vertexCount = 0;
	ComPtr<ID3D11Buffer> m_vertexBuffer;
	ComPtr<ID3D11Buffer> m_indexBuffer;
	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;

	std::vector<CurvatureInfo> m_curvatures;
	glm::vec4 m_tangent;
};
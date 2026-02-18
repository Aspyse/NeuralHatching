#pragma once

#include <unordered_set>
#include <unordered_map>
#include <vector>
#include <Eigen/Dense>
#include <glm/glm.hpp> // TODO: trim
#include "Model.h"

class Curvature
{
public:
	using CurvatureInfo = Model::CurvatureInfo;
	using Vertex = Model::Vertex;

public:
	void InitializeField(Model& model);

private:
	const std::vector<uint32_t>& GetOneRing(uint32_t vertexIndex) const;
	void BuildOneRings(const std::vector<uint32_t>& indices);
	std::vector<uint32_t> OrderOneRing(uint32_t center, const std::unordered_set<uint32_t>& neighbors, const std::unordered_map<uint32_t, std::vector<uint32_t>>& adjacency);

	CurvatureInfo SolveCurvature(uint32_t currentIndex, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

	glm::vec3 CalculateControlPointLimit(uint32_t currentIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& oneRing);


	// Helpers
	float directionToAngle(const glm::vec3& dir, const glm::vec3& tangent_u, const glm::vec3& tangent_v);

public:
	const float RELIABLE_THRESHOLD = 0.6; //TODO: needs more testing
	const float CURVATURE_THRESHOLD = 40;

	std::unordered_map<uint32_t, std::vector<uint32_t>> m_oneRings;
};
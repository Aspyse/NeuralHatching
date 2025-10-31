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
	CurvatureInfo SolveCurvature(uint32_t currentIndex, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices);

	// Ordered
	std::vector<uint32_t> FindOneRing(uint32_t currentIndex, std::vector<uint32_t> indices);

	glm::vec3 CalculateControlPointLimit(uint32_t currentIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& oneRing);


	// Helpers
	float directionToAngle(const glm::vec3& dir, const glm::vec3& tangent_u, const glm::vec3& tangent_v);

public:
	const float RELIABLE_THRESHOLD = 0.5;
};
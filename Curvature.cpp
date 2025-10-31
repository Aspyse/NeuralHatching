#include "Curvature.h"
#include <glm/gtc/constants.hpp>
#include <vector>

void Curvature::InitializeField(Model& model)
{
	std::vector<CurvatureInfo> curvatures;
	curvatures.reserve(model.m_vertexCount);

	for (uint32_t v = 0; v < model.m_vertexCount; v++)
		curvatures.push_back(SolveCurvature(v, model.m_vertices, model.m_indices));

	model.m_curvatures = curvatures; // TODO: consider if proper return is better
}

// Find the principal direction/hatch angle at a given vertex
// Maximum and minimum curvatures are assumed to be perpendicular, thus equivalent
Model::CurvatureInfo Curvature::SolveCurvature(uint32_t currentIndex, const std::vector<Vertex> &vertices, const std::vector<uint32_t> &indices)
{
	CurvatureInfo result;
	
	std::vector<uint32_t> oneRing = FindOneRing(currentIndex, indices);
	int k = oneRing.size();

	// TODO: non-manifold check

	glm::vec3 a0 = CalculateControlPointLimit(currentIndex, vertices, oneRing);

	// Hertzmann and Zorin Appendix A
	glm::vec3 sum_pi(0.0f);
	glm::vec3 a11(0.0f), a12(0.0f), a21(0.0f), a22(0.0f);

	for (int i = 0; i < k; i++)
	{
		glm::vec3 pi = vertices[oneRing[i]].position;
		float angle1 = 2.0f * glm::pi<float>() * i / k;
		float angle2 = 4.0f * glm::pi<float>() * i / k;

		float sin1 = std::sin(angle1);
		float cos1 = std::cos(angle1);
		float sin2 = std::sin(angle2);
		float cos2 = std::cos(angle2);

		sum_pi += pi;
		a11 += pi * sin1;
		a12 += pi * cos1;
		a21 += pi * sin2;
		a22 += pi * cos2;
	}

	// Scaling factors
	float k_inv = 1.0f / k;
	glm::vec3 a20 = -a0 + sum_pi * k_inv;
	k_inv *= 2;
	a11 *= k_inv;
	a12 *= k_inv;
	a21 *= k_inv;
	a22 *= k_inv;

	// Build local coordinate frame
	// Fu = a11, Fv = a12 are tangent vectors
	glm::vec3 Fu = a11;
	glm::vec3 Fv = a12;
	glm::vec3 n = glm::normalize(glm::cross(Fu, Fv));

	// Create orthonormal tangent frame for angle encoding
	glm::vec3 tangent_u = glm::normalize(Fu);
	glm::vec3 tangent_v = glm::normalize(Fv - glm::dot(Fv, tangent_u) * tangent_u);

	result.tangent_u = tangent_u;
	result.tangent_v = tangent_v;

	// Second derivatives
	glm::vec3 Fuu = 2.0f * a20 + 2.0f * a22;  // Second derivative in u
	glm::vec3 Fuv = 2.0f * a21;                // Mixed derivative
	glm::vec3 Fvv = 2.0f * a20 - 2.0f * a22;  // Second derivative in v

	// First fundamental form: E, F, G
	float E = glm::dot(Fu, Fu);
	float F = glm::dot(Fu, Fv);
	float G = glm::dot(Fv, Fv);

	// Second fundamental form: L, M, N
	float L = glm::dot(Fuu, n);
	float M = glm::dot(Fuv, n);
	float N = glm::dot(Fvv, n);

	// Build matrices for equation (2): First * Second
	Eigen::Matrix2f first;
	first << E, F,
		F, G;

	Eigen::Matrix2f second;
	second << L, M,
		M, N;

	// Solve generalized eigenvalue problem: second * v = lambda * first * v
	// Or equivalently: first^-1 * second * v = lambda * v
	Eigen::Matrix2f A = first.inverse() * second;

	// Compute eigenvalues and eigenvectors
	Eigen::SelfAdjointEigenSolver<Eigen::Matrix2f> solver(A);

	if (solver.info() != Eigen::Success) {
		result.kappa1 = result.kappa2 = 0.0f;
		result.dir1 = tangent_u;
		result.dir2 = tangent_v;
		result.theta = 0.0f;
		return result;
	}

	// Principal curvatures (eigenvalues)
	float kappa_a = solver.eigenvalues()(0);
	float kappa_b = solver.eigenvalues()(1);

	// Principal directions (eigenvectors in parameter space)
	Eigen::Vector2f ev1 = solver.eigenvectors().col(0);
	Eigen::Vector2f ev2 = solver.eigenvectors().col(1);

	if (std::abs(kappa_a) > std::abs(kappa_b))
	{
		result.kappa1 = kappa_a; // Maximum
		result.kappa2 = kappa_b; // Minimum
		// Convert to 3D directions in tangent plane
		result.dir1 = glm::normalize(ev1(0) * Fu + ev1(1) * Fv);
		result.dir2 = glm::normalize(ev2(0) * Fu + ev2(1) * Fv);
	}
	else
	{
		result.kappa1 = kappa_b;
		result.kappa2 = kappa_a;
		result.dir1 = glm::normalize(ev2(0) * Fu + ev2(1) * Fv);
		result.dir2 = glm::normalize(ev1(0) * Fu + ev1(1) * Fv);
	}
	// Encode primary direction as angle in tangent plane
	// This is how the paper represents the cross field in Section 5
	//result.theta = directionToAngle(result.dir1, tangent_u, tangent_v);


	result.theta = result.kappa2 / result.kappa1;


	return result;
}

std::vector<uint32_t> Curvature::FindOneRing(uint32_t currentIndex, std::vector<uint32_t> indices)
{
	std::unordered_set<uint32_t> neighbors;
	std::unordered_map<uint32_t, std::vector<uint32_t>> adjacencyList;

	for (int i = 0; i < indices.size(); i += 3)
	{
		int local = -1;
		if (indices[i] == currentIndex) local = 0;
		else if (indices[i + 1] == currentIndex) local = 1;
		else if (indices[i + 2] == currentIndex) local = 2;

		if (local != -1)
		{
			int v1 = indices[i + (local + 1) % 3];
			int v2 = indices[i + (local + 2) % 3];

			neighbors.insert(v1);
			neighbors.insert(v2);
			adjacencyList[v1].push_back(v2);
			adjacencyList[v2].push_back(v1);
		}
	}

	// TODO: non-manifold check
	if (neighbors.size() < 3)
		return std::vector<uint32_t>(neighbors.begin(), neighbors.end());

	uint32_t start = *neighbors.begin();
	for (auto n : neighbors)
		if (adjacencyList[n].size() == 1)
			start = n;

	std::vector<uint32_t> ordered;
	ordered.reserve(neighbors.size());
	uint32_t prev = 0, curr = start;
	while (ordered.size() < neighbors.size())
	{
		ordered.push_back(curr);

		uint32_t next = 0;
		for (uint32_t adjacent : adjacencyList[curr])
		{
			if (adjacent != prev)
			{
				next = adjacent;
				break;
			}
		}

		prev = curr;
		if (next == start)
			break;
	}

	return ordered;
}

// Loop subdivision limit
glm::vec3 Curvature::CalculateControlPointLimit(uint32_t currentIndex, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& oneRing)
{
	int k = oneRing.size();

	// Loop subdivision weights
	float beta;
	if (k == 3)
		beta = 3.0f / 16.0f;
	else
		beta = 3.0f / (8.0f * k);

	glm::vec3 p0 = vertices[currentIndex].position;
	glm::vec3 sum(0.0f);

	for (uint32_t n : oneRing)
		sum += vertices[n].position;
	
	return (1.0f - k * beta) * p0 + beta * sum;
}

// Project direction onto tangent plane basis
float Curvature::directionToAngle(const glm::vec3& dir, const glm::vec3& tangent_u, const glm::vec3& tangent_v)
{
	float u = glm::dot(dir, tangent_u);
	float v = glm::dot(dir, tangent_v);
	return std::atan2(v, u);
}

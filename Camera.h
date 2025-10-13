#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

class Camera
{
public:
	Camera();
	~Camera();

	void SetAspect(float fov, float aspect);
	void SetAspect(float fov, float width, float height);
	void SetPlanes(float nearZ, float farZ);
	void SetPosition(float x, float y, float z);

	void Initialize();

	void Frame(float dx, float dy, int ds, bool isMiddleMouseDown, bool isShiftdown);

	void Orbit(float dx, float dy);
	void Pan(float dx, float dy);
	void Zoom(float ds);

	glm::mat4x4 GetProjectionMatrix() const;
	glm::mat4x4 GetViewMatrix() const;

private:
	const float ORBIT_SENSITIVITY = 0.23f;
	const float PAN_SENSITIVITY = 0.0006f;
	const float ZOOM_SENSITIVITY = 0.0005f;

	glm::vec3 m_position = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_rotation = { 0.0f, 0.0f, 0.0f };
	glm::vec3 m_orbitPos = { 0.0f, 0.0f, 0.0f };

	glm::mat4x4 m_viewMatrix = {};
	glm::mat4x4 m_projectionMatrix = {};

	float m_yaw = 0.0f, m_pitch = 0.0f;
	float m_distance = 1.0f;
	
	float m_aspect = 1.0f, m_fov = 1.4f;
	float m_nearZ = 1.0f, m_farZ = 1000.0f;
};
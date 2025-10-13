#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/euler_angles.hpp>

Camera::Camera() {}
Camera::~Camera() {}

void Camera::SetAspect(float fov, float aspect)
{
	m_fov = glm::radians(fov);
	m_aspect = aspect;
}

void Camera::SetAspect(float fov, float width, float height)
{
	m_fov = glm::radians(fov);
	m_aspect = width/height;
}

void Camera::SetPlanes(float nearZ, float farZ)
{
	m_nearZ = nearZ;
	m_farZ = farZ;
}

void Camera::SetPosition(float x, float y, float z)
{
	m_position.x = x;
	m_position.y = y;
	m_position.z = z;
}

void Camera::Initialize()
{
	m_projectionMatrix = glm::perspectiveLH_ZO(m_fov, m_aspect, m_nearZ, m_farZ);
}

void Camera::Frame(float dx, float dy, int ds, bool isMiddleMouseDown, bool isShiftDown)
{
	if (isMiddleMouseDown)
	{
		if (isShiftDown)
			Pan(dx * PAN_SENSITIVITY, dy * PAN_SENSITIVITY);
		else
			Orbit(dx * ORBIT_SENSITIVITY, -dy * ORBIT_SENSITIVITY);
	}
	if (ds != 0)
		Zoom(-ds * ZOOM_SENSITIVITY);

	glm::vec3 up(0.0f, 1.0f, 0.0f);
	glm::vec3 lookAt(0.0f, 0.0f, 1.0f);

	float pitch = glm::radians(m_rotation.x);
	float yaw = glm::radians(m_rotation.y);
	float roll = glm::radians(m_rotation.z);

	glm::mat4 rotationMatrix = glm::yawPitchRoll(yaw, pitch, roll);

	lookAt = glm::vec3(rotationMatrix * glm::vec4(lookAt, 0.0f));
	up = glm::vec3(rotationMatrix * glm::vec4(up, 0.0f));

	lookAt += m_position;

	m_viewMatrix = glm::lookAtLH(m_position, lookAt, up);
}

void Camera::Orbit(float dx, float dy)
{
	m_yaw += dx;
	m_pitch = glm::clamp(m_pitch + dy, -89.0f, 89.0f);

	float yawRad = glm::radians(m_yaw);
	float pitchRad = glm::radians(m_pitch);

	glm::vec3 dir(
		cosf(pitchRad) * sinf(yawRad),
		sinf(pitchRad),
		cosf(pitchRad) * cosf(yawRad)
	);

	m_position = m_orbitPos - dir * m_distance;

	glm::vec3 forward = glm::normalize(m_orbitPos - m_position);
	m_rotation = glm::vec3(
		glm::degrees(asinf(-forward.y)),
		glm::degrees(atan2f(forward.x, forward.z)),
		0.0f
	);
}

void Camera::Pan(float dx, float dy)
{
	dx *= m_distance;
	dy *= m_distance;

	float yawRad = glm::radians(m_yaw);
	float pitchRad = glm::radians(m_pitch);

	glm::vec3 forward(
		cosf(pitchRad) * sinf(yawRad),
		sinf(pitchRad),
		cosf(pitchRad) * cosf(yawRad)
	);

	glm::vec3 right = glm::normalize(glm::cross(glm::vec3(0, 1, 0), forward));
	glm::vec3 up = glm::normalize(glm::cross(forward, right));

	m_orbitPos += right * -dx + up * dy;
	m_position = m_orbitPos - forward * m_distance;
}

void Camera::Zoom(float ds)
{
	m_distance = glm::max(m_distance + ds * m_distance, 0.01f);

	float yawRad = glm::radians(m_yaw);
	float pitchRad = glm::radians(m_pitch);

	glm::vec3 forward(
		cosf(pitchRad) * sinf(yawRad),
		sinf(pitchRad),
		cosf(pitchRad) * cosf(yawRad)
	);

	m_position = m_orbitPos - forward * m_distance;
}

glm::mat4x4 Camera::GetProjectionMatrix() const
{
	return glm::mat4x4();
}

glm::mat4x4 Camera::GetViewMatrix() const
{
	return glm::mat4x4();
}


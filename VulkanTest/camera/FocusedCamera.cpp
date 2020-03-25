
#include "FocusedCamera.h"
#include <math.h>

FocusedCamera::FocusedCamera(
	float width,
	float height,
	float fovy,
	glm::vec3 center,
	float theta,
	float alpha,
	float radius
) :
	m_radius(radius),
	m_theta(theta),
	m_alpha(alpha),
	m_center(center),

	Camera(width, height, fovy, getEye(center, theta, alpha, radius), center)
{
}

void FocusedCamera::setCenter(const glm::vec3& center)
{
	m_center = center;
	setCoordinates(getEye(center, m_theta, m_alpha, m_radius), center);
}

void FocusedCamera::rotate(float x, float y)
{
	m_theta += x;

	if (m_theta < 0) 
	{
		m_theta += 2 * glm::pi<float>();
	}
	else if (m_theta > 2 * glm::pi<float>())
	{
		m_theta -= 2 * glm::pi<float>();
	}

	m_alpha += y;

	if (m_alpha < 0)
	{
		m_alpha = 0;
	}
	else if (m_alpha > glm::pi<float>() - 0.001)
	{
		m_alpha = glm::pi<float>() - 0.001;
	}

	setCenter(m_center);
}

const glm::vec3 FocusedCamera::getEye(const glm::vec3& center, float theta, float alpha, float radius)
{
	float x = cosf(theta) * sinf(alpha);
	float y = sinf(theta) * sinf(alpha);
	float z = cosf(alpha);
	return center + radius * glm::vec3(x, -z, y);
}

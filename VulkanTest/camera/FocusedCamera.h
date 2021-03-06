#include "Camera.h"

class FocusedCamera : public Camera
{
public:
	FocusedCamera(
		float width,
		float height,
		float fovy = glm::pi<float>() / 3,
		glm::vec3 center = glm::vec3(0),
		float theta = 1,
		float alpha = glm::pi<float>() / 2.0,
		float radius = 3
	);

	void setCenter(const glm::vec3& center);
	void rotate(float x, float y);

private:
	
	const glm::vec3 getEye(const glm::vec3& center, float theta, float alpha, float radius);

private:
	float m_radius;

	float m_theta;
	float m_alpha;

	glm::vec3 m_center;
};

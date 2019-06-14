#include "pch.h"
#include "Camera.h"


Camera::Camera()
{
	view = glm::lookAt(
		glm::vec3(0.0f, 2.2f, 5.2f), // position
		glm::vec3(0.0f, 0.0f, 0.0f), // camera center
		glm::vec3(0.0f, 0.0f, 1.0f) // up axis
	);
	proj = glm::perspective(glm::radians(45.0f), 1366.0f / 720.0f, 1.0f, 1000.0f);
}


Camera::~Camera()
{
}

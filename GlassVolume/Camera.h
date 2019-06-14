#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
	glm::mat4 view;
	glm::mat4 proj;
	glm::vec3 position;
	Camera();
	~Camera();
};


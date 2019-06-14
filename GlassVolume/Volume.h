#pragma once
#define GLEW_STATIC

#include <GL/glew.h>

#include <string>
#include <vector>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <boost/algorithm/string.hpp>

#include "tiny_obj_loader.h"

#include "Camera.h"

class Volume
{
	GLuint loadShader(const char *filepath, GLenum type);
	std::string modelPath = "assets/box.obj";
	unsigned int VAO, VBO, EBO;
	GLuint shaderProgram;
	GLint uniTrans, uniView, uniProj, uniColor;
	std::vector<GLuint> triangles;
	std::vector<float> vertices;
	std::vector<float> normals;

public:
	Volume();
	~Volume();
	void render(Camera &camera);
	glm::mat4 model;
};

